/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "stdafx.h"
#include "AWSResourceManager.h"

#include <IEditor.h>

#include <QDebug>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QMainWindow>

#include <CloudCanvas/CloudCanvasIdentityBus.h>
#include <CloudCanvas/CloudCanvasMappingsBus.h>

#include <InitializeCloudCanvasProject.h>

#include "CloudCanvasLogger.h"

#include "AWSImporterModel.h"
#include "AWSDeploymentModel.h"
#include "AWSProfileModel.h"
#include "AWSProjectModel.h"
#include "FileContentModel.h"
#include "CloudFormationTemplateModel.h"
#include "ProjectStatusModel.h"
#include "DeploymentStatusModel.h"
#include "ResourceGroupStatusModel.h"
#include "DeploymentListStatusModel.h"
#include "ResourceGroupListStatusModel.h"
#include "PlaceholderDeploymentStatusModel.h"
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/core/utils/memory/stl/AWSString.h>

#include <LyIdentity/LyIdentity.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>

#include "FileSourceControlModel.h"

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Util/PathUtil.h>

#include <AzCore/std/parallel/atomic.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Results.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <AWSResourceManager.moc>
#include <IAWSResourceManager.moc>

using namespace AzFramework;

///////////////////////////////////////////////////////////////////////////////

QDebug operator<< (QDebug d, const QVariantMap &args) {

    QString result;
    result += "{";
    for (auto i = args.cbegin(); i != args.cend(); ++i)
    {
        if (i != args.cbegin())
        {
            result += ",";
        }
        result += " ";
        result += i.key();
        result += ": ";
        result += i.value().toString();
    }
    result += "}";

    d.noquote();
    d << result;
    d.quote();

    return d;
}

///////////////////////////////////////////////////////////////////////////////

AWSResourceManager::AWSResourceManager(IEditor* editor)
    : m_editor{editor}
    , m_logger{new CloudCanvasLogger {editor->GetSystem()->GetILog()}}
    , m_fileMonitor { new FileChangeMonitor {} }
    , m_initializationState { UnknownState }
    , m_startupComplete { false }
    , m_projectSettingsSourceControlModel { new SourceControlStatusModel }
    , m_deploymentTemplateSourceControlModel { new SourceControlStatusModel }
    , m_gemsFileSourceControlModel { new SourceControlStatusModel }
{
    assert(m_editor);

    connect(&*m_fileMonitor, &FileChangeMonitor::FileChanged, this, &AWSResourceManager::OnFileChanged);

    m_pythonWorker.reset(new PythonWorker(m_logger, GetRootDirectoryPath().c_str(), GetGameDirectoryPath().c_str(), GetUserDirectoryPath(editor).c_str()));
    connect(&*m_pythonWorker, &PythonWorker::Output, this, &AWSResourceManager::ProcessPythonOutput);

    m_describeProjectRequestId = AllocateRequestId();
    m_listDeploymentsRequestId = AllocateRequestId();
    m_listResourceGroupsRequestId = AllocateRequestId();
    m_listProfilesRequestId = AllocateRequestId();
    m_listMappingsRequestId = AllocateRequestId();
    m_updateMappingsRequestId = AllocateRequestId();

    m_editor->SetAWSResourceManager(this);
}

AZStd::string AWSResourceManager::GetRootDirectoryPath()
{
    // Request the engine root as the root directory
    const char* engineRoot = nullptr;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);

    if (!engineRoot)
    {
        // If unable to get the engine root from the bus, fallback to the executable's path
        return AZStd::string(::Path::GetExecutableParentDirectoryUnicode().toUtf8().data());
    }

    return AZStd::string(engineRoot);

}

AZStd::string AWSResourceManager::GetGameDirectoryPath()
{
    return Path::GetEditingGameDataFolder();
}

AZStd::string AWSResourceManager::GetUserDirectoryPath(IEditor* editor)
{
    AZStd::string user_directory_path;
    StringFunc::Path::Join(editor->GetEnv()->pFileIO->GetAlias("@user@"), "AWS", user_directory_path);
    return user_directory_path;
}

AWSResourceManager::~AWSResourceManager()
{
    if (m_editor && m_editor->GetAWSResourceManager() == this)
    {
        m_editor->SetAWSResourceManager(nullptr);
    }
}

void AWSResourceManager::ExecuteAsync(ExecuteAsyncCallback callback, const char* command, const QVariantMap& args, bool failOnLoadError)
{
    PythonWorkerRequestId requestId = m_pythonWorker->AllocateRequestId();
    m_pythonCallbacks[requestId] = callback;
    ExecuteAsync(requestId, command, args, failOnLoadError);
}

void AWSResourceManager::ExecuteAsyncWithRetry(ExecuteAsyncCallback callback, const char* command, const QVariantMap& args)
{
    PythonWorkerRequestId requestId = m_pythonWorker->AllocateRequestId();
    m_pythonCallbacks[requestId] = callback;
    ExecuteAsyncWithRetry(requestId, command, args);
}

void AWSResourceManager::ExecuteAsyncWithRetry(AWSResourceManager::RequestId requestId, const char* command, const QVariantMap& args)
{
    if (!m_retryableRequestMap.contains(requestId)) // only one at a time
    {

        qDebug() << "AWSResourceManager::ExecuteAsyncWithRetry - submitting" << "- requestId:" << requestId << "- command:" << command << "- args:" << args;

        m_retryableRequestMap[requestId] = RetryableRequest{ command, args };

        // Ignore commands issued before startup is complete. This can happen
        // when views are opened up automatically by the editor on startup.
        // Once startup is complete all retryable requests are resubmitted.
        if (m_startupComplete || requestId == m_describeProjectRequestId)
        {
            m_pythonWorker->ExecuteAsync(requestId, command, args);
        }

    }
    else
    {
        qDebug() << "AWSResourceManager::ExecuteAsyncWithRetry - duplicate" << "- requestId:" << requestId << "- command:" << command << "- args:" << args;
    }
}

void AWSResourceManager::ExecuteAsync(AWSResourceManager::RequestId requestId, const char* command, const QVariantMap& args, bool failOnLoadError)
{

    AZ_Assert(m_startupComplete, "Attempt to submit a non-retryable command before startup is complete.");
    if (!m_startupComplete)
    {
        return;
    }

    if (failOnLoadError && m_initializationState == InitializationState::ErrorLoadingState)
    {
        qDebug() << "AWSResourceManager::ExecuteAsync - rejecting" << "- requestId:" << requestId << "- command:" << command << "- args:" << args;
        ProcessPythonOutput(requestId, "error", GetErrorString());
    }
    else
    {
        qDebug() << "AWSResourceManager::ExecuteAsync - submitting" << "- requestId:" << requestId << "- command:" << command << "- args:" << args;
        m_pythonWorker->ExecuteAsync(requestId, command, args);
    }

}

void AWSResourceManager::OnInit()
{
    m_pythonWorker->Start();
    RefreshProjectDescription();
}

void AWSResourceManager::OnBeginGameMode()
{
    RefreshGameConfigurations();
}

// If our initialization data completes loading while we're in game mode we may need to reinitialize our mappings
// This occurs both when we enter the game before the project has finished initializing on startup or when we
// switch deployments while in game mode
void AWSResourceManager::CheckGameConfigurations()
{
    bool shouldApplyMapping = true;
    bool mappingIsProtected{ false };
    EBUS_EVENT_RESULT(mappingIsProtected, CloudGemFramework::CloudCanvasMappingsBus, IsProtectedMapping);
    static const char* PROTECTED_MAPPING_MSG_TITLE = "AWS Mapping Is Protected";
    static const char* PROTECTED_MAPPING_MSG_TEXT = "Warning: The AWS resource mapping file is marked as protected and shouldn't be used for normal development work. Are you sure you want to continue?";
    if (mappingIsProtected)
    {
        shouldApplyMapping = CryMessageBox(PROTECTED_MAPPING_MSG_TEXT, PROTECTED_MAPPING_MSG_TITLE, MB_ICONEXCLAMATION | MB_YESNO) == IDYES ? true : false;
        // if this flag isn't set properly it's possible for a flow graph or other code to still apply the configuration
        EBUS_EVENT(CloudGemFramework::CloudCanvasMappingsBus, SetIgnoreProtection, shouldApplyMapping);
    }

    if (shouldApplyMapping)
    {
        EBUS_EVENT(CloudGemFramework::CloudCanvasPlayerIdentityBus, ApplyConfiguration);
    }

    if (!GetIEditor()->IsInGameMode())
    {
        return;
    }

    RefreshGameConfigurations();
}

void AWSResourceManager::RefreshGameConfigurations()
{
    RefreshHasResourceGroupsStatus();

    if (!GetDeploymentModel()->IsActiveDeploymentSet())
    {
        if (HasResourcGroups())
        {
            m_logger->LogError("[Cloud Canvas] No AWS deployment is active. Go to the Cloud Canvas Resource Manager and create or select a deployment.");
        }
        else
        {
            m_logger->LogWarning("[Cloud Canvas] Resource manager was invoked but there are no deployments or resource groups.");
        }
    }
}

AWSResourceManager::PythonOutputHandlerMap AWSResourceManager::s_PythonOutputHandlers {
    {
        "message", &AWSResourceManager::ProcessOutputMessage
    },
    {
        "error", &AWSResourceManager::ProcessOutputErrorWithLogging
    },
    {
        "project_stack_exists", &AWSResourceManager::ProcessProjectStackExists
    },
    {
        "framework-not-enabled-error", &AWSResourceManager::ProcessFrameworkNotEnabledError
    },
    {
        "success", &AWSResourceManager::ProcessOutputSuccess
    },
    {
        "project-description", &AWSResourceManager::ProcessOutputProjectDescription
    },
    {
        "mapping-list", &AWSResourceManager::ProcessOutputMappingList
    },
    {
        "deployment-list", &AWSResourceManager::ProcessOutputDeploymentList
    },
    {
        "importable-resource-list", &AWSResourceManager::ProcessOutputImportableResourceList
    },
    {
        "profile-list", &AWSResourceManager::ProcessOutputProfileList
    },
    {
        "resource-group-list", &AWSResourceManager::ProcessOutputResourceGroupList
    },
    {
        "project-stack-description", &AWSResourceManager::ProcessOutputProjectStackDescription
    },
    {
        "deployment-stack-description", &AWSResourceManager::ProcessOutputDeploymentStackDescription
    },
    {
        "resource-group-stack-description", &AWSResourceManager::ProcessOutputResourceGroupStackDescription
    },
    {
        "resource-list", &AWSResourceManager::ProcessOutputResourceList
    },
    {
        "stack-event-errors", &AWSResourceManager::ProcessOutputStackEventErrors
    },
    {
        "supported-region-list", &AWSResourceManager::ProcessOutputSupportedRegionList
    },
    {
        "create-admin", &AWSResourceManager::ProcessOutputCreateAdmin
    }
};

void AWSResourceManager::ProcessPythonOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value)
{

    auto i = m_pythonCallbacks.find(requestId);
    if (i == m_pythonCallbacks.end())
    {

        qDebug() << "AWSResourceManager::ProcessPythonOutput- dispatching" << "- requestId:" << requestId << "- key:" << key << "- value:" << value;

        auto handler = s_PythonOutputHandlers[key];
        // if another system was using the python worker and the system is destroyed but there
        // are still messages being sent from python there may be no handler for them, so the
        // null check here is making sure this class can handle them, otherwise they are ignored.
        if (handler != nullptr)
        {
            (this->*handler)(requestId, value);
        }
    }
    else
    {
        qDebug() << "AWSResourceManager::ProcessPythonOutput- callback called" << "- requestId:" << requestId << "- key:" << key << "- value:" << value;

        if (key == "error") {
            QString message;
            if (value.type() == QVariant::Type::List)
            {
                auto list = value.value<QVariantList>();
                for (auto it = list.constBegin(); it != list.constEnd(); ++it)
                {
                    auto entry = *it;
                    if (!message.isEmpty())
                    {
                        message += "\n";
                    }
                    message += entry.toString();
                }
            }
            else
            {
                message = value.toString();
            }
            i.value()(key, QVariant{ message });
        }
        else
        {
            i.value()(key, value);
        }

        if (key == "error" || key == "success")
        {
            qDebug() << "AWSResourceManager::ProcessPythonOutput- callback erased" << "- requestId:" << requestId << "- key:" << key << "- value:" << value;
            m_pythonCallbacks.erase(i);
        }
    }
}

void AWSResourceManager::ProcessOutputMessage(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    CommandOutput(requestId, "message", value);
    m_logger->Log("%s", value.toString().replace("\n", "").toStdString().c_str());
}

void AWSResourceManager::ProcessProjectStackExists(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    GetIEditor()->OpenWinWidget(InitializeCloudCanvasProject::GetWWId());
}

void AWSResourceManager::ProcessFrameworkNotEnabledError(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    ProcessOutputError(requestId, value, false);
}

void AWSResourceManager::ProcessOutputErrorWithLogging(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    ProcessOutputError(requestId, value, true);
}

void AWSResourceManager::ProcessOutputError(AWSResourceManager::RequestId requestId, const QVariant& value, bool log)
{

    QString message;
    if (value.type() == QVariant::Type::List)
    {
        auto list = value.value<QVariantList>();
        for (auto it = list.constBegin(); it != list.constEnd(); ++it)
        {
            auto entry = *it;
            if (!message.isEmpty())
            {
                message += "\n";
            }
            message += entry.toString();
        }
    }
    else
    {
        message = value.toString();
        if (log)
        {
            m_logger->LogError("%s", message.toStdString().c_str());
        }
    }

    auto i = m_retryableRequestMap.find(requestId);
    if (i == m_retryableRequestMap.end())
    {
        qDebug() << "AWSResourceManager::ProcessOutputError - dispatching" << "- requestId:" << requestId << "- value:" << value << "- message:" << message;
        CommandOutput(requestId, "error", QVariant{ message });
    }
    else
    {
        qDebug() << "AWSResourceManager::ProcessOutputError - remembering" << "- requestId:" << requestId << "- value:" << value << "- message:" << message;
        m_failedRequestMap[requestId] = FailedRequest{ message };
        if (m_initializationState != InitializationState::ErrorLoadingState)
        {
            qDebug() << "AWSResourceManager::ProcessOutputError - set ErrorLoadingState" << "- requestId:" << requestId << "- value:" << value;
            m_initializationState = InitializationState::ErrorLoadingState;
            ProjectInitializedChanged();
        }
    }

}

QString AWSResourceManager::GetErrorString() const
{
    AZ_Assert(!m_failedRequestMap.isEmpty(), "GetErrorString called when there are no failed requests.");
    return m_failedRequestMap.first().errorMessage; // arbitary, but doesn't matter
}

void AWSResourceManager::RetryLoading()
{

    qDebug() << "AWSResourceManager::RetryLoading - set UnknownState";

    m_initializationState = InitializationState::UnknownState;
    ProjectInitializedChanged();

    for (auto i = m_failedRequestMap.cbegin(); i != m_failedRequestMap.cend(); ++i)
    {
        const RetryableRequest& request = m_retryableRequestMap.value(i.key());
        qDebug() << "AWSResourceManager::RetryLoading - submitting" << "- requestId:" << i.key() << "- command = " << request.command << "- args = " << request.args;
        ExecuteAsync(i.key(), request.command, request.args);
    }

}

void AWSResourceManager::ProcessOutputSuccess(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    qDebug() << "AWSResourceManager::ProcessOutputSuccess" << "- requestId:" << requestId << "- value:" << value;
    auto i = m_retryableRequestMap.find(requestId);
    if (i != m_retryableRequestMap.end())
    {

        qDebug() << "AWSResourceManager::ProcessOutputSuccess - erased retryable" << "- requestId:" << requestId << "- value:" << value;
        m_retryableRequestMap.erase(i);

        auto j = m_failedRequestMap.find(requestId);
        if (j != m_failedRequestMap.end())
        {
            qDebug() << "AWSResourceManager::ProcessOutputSuccess - erased failed" << "- requestId:" << requestId << "- value:" << value;
            m_failedRequestMap.erase(j);
            if (m_failedRequestMap.isEmpty() && (m_initializationState == InitializationState::UnknownState || m_initializationState == InitializationState::ErrorLoadingState))
            {
                // All failed requests have succeeded but we are still in an
                // unkown or error state. Refreshing the project description will
                // cause the initialization state to be updated.
                qDebug() << "AWSResourceManager::ProcessOutputSuccess - refreshing project description";
                RefreshProjectDescription();
            }
        }

    }
    CommandOutput(requestId, "success", value);
}

AZStd::string AWSResourceManager::GetIdentityId()
{
    auto identity = Ly::Identity::GetDeveloperCognitoSettings();
    /// helper class that gets Cognito credentials when needed
    Aws::Auth::PersistentCognitoIdentityProvider_JsonFileImpl persistentIdentityProvider(identity.GetIdentityPoolId(), identity.GetAccountId());
    return persistentIdentityProvider.GetIdentityId().c_str();
}

void AWSResourceManager::ProcessOutputCreateAdmin(RequestId requestId, const QVariant& value)
{
    QMap<QString, QVariant> message = value.toMap();

    QDialog loginInfo;
    loginInfo.setWindowFlags(Qt::WindowCloseButtonHint);
    loginInfo.setWindowTitle("Cloud Gem Portal Administrator Account Creation");
    loginInfo.setMinimumHeight(140);
    loginInfo.setFixedWidth(350);

    QVBoxLayout* loginInfoLayout = new QVBoxLayout;

    QLabel helpInfo("Please use the username and password below to log into the site as an administrator.  Do not lose these.");
    helpInfo.setWordWrap(true);
    loginInfoLayout->addWidget(&helpInfo);

    QString user("Username:  " + message["administrator_name"].toString());
    QString password("Password:  " + message["password"].toString());
    QLabel* content = new QLabel(user + "\n" + password);
    content->setTextInteractionFlags(Qt::TextSelectableByMouse);
    loginInfoLayout->addWidget(content);

    QPushButton* createButton = new QPushButton("OK");
    createButton->setProperty("class", "Primary");

    QDialogButtonBox* buttonBox = new QDialogButtonBox;
    buttonBox->addButton(createButton, QDialogButtonBox::ActionRole);
    connect(createButton, &QPushButton::clicked, &loginInfo, &QDialog::accept);
    loginInfoLayout->addWidget(buttonBox);

    loginInfo.setLayout(loginInfoLayout);

    loginInfo.exec();
    m_logger->LogWarning("The Cloud Gem Portal temporary administrator account credentials are:");
    m_logger->LogWarning("Username:  " + message["administrator_name"].toString());
    m_logger->LogWarning("Temporary password:  " + message["password"].toString());
}

void AWSResourceManager::ProcessOutputSupportedRegionList(RequestId requestId, const QVariant& value)
{
    QStringList regionList = value.toStringList();
    SupportedRegionsObtained(regionList);
}

void AWSResourceManager::ProcessOutputStackEventErrors(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    CommandOutput(requestId, "stack-event-errors", value);
}

void AWSResourceManager::ProcessOutputProjectDescription(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    m_lastProjectDescription = value;

    QVariantMap map = value.value<QVariantMap>();
    QVariantMap details = map["ProjectDetails"].value<QVariantMap>();

    if (!m_startupComplete)
    {

        m_startupComplete = true;

        m_fileMonitor->BeginMonitoring(FileChangeMonitor::MonitoredFileType::ProjectSettings, details["ProjectSettingsFilePath"].value<QString>());
        m_fileMonitor->BeginMonitoring(FileChangeMonitor::MonitoredFileType::UserSettings, details["UserSettingsFilePath"].value<QString>());
        m_fileMonitor->BeginMonitoring(FileChangeMonitor::MonitoredFileType::ProjectTemplate, details["ProjectTemplateFilePath"].value<QString>());
        m_fileMonitor->BeginMonitoring(FileChangeMonitor::MonitoredFileType::DeploymentTemplate, details["DeploymentTemplateFilePath"].value<QString>());
        m_fileMonitor->BeginMonitoring(FileChangeMonitor::MonitoredFileType::DeploymentAccessTemplate, details["DeploymentAccessTemplateFilePath"].value<QString>());
        m_fileMonitor->BeginMonitoring(FileChangeMonitor::MonitoredFileType::AWSCredentials, details["AWSCredentialsFilePath"].value<QString>());
        m_fileMonitor->BeginMonitoring(FileChangeMonitor::MonitoredFileType::GUIRefresh, details["GuiRefreshFilePath"].value<QString>());
        m_fileMonitor->BeginMonitoring(FileChangeMonitor::MonitoredFileType::GemsFile, details["GemsFilePath"].value<QString>());

        // If any of the views were opened before startup was
        // complete the requests for the data they need will
        // have been dropped. Re-request that data now.

        for (auto i = m_retryableRequestMap.cbegin(); i != m_retryableRequestMap.cend(); ++i)
        {
            if (i.key() != m_describeProjectRequestId)
            {
                qDebug() << "AWSResourceManager::ProcessOutputProjectDescription - resubmitting" << "- requestId:" << i.key() << "- command = " << i.value().command << "- args = " << i.value().args;
                ExecuteAsync(i.key(), i.value().command, i.value().args);
            }
        }

        if (details["ProjectInitialized"].value<bool>())
        {
            if (details["DefaultAWSProfile"].isValid())
            {
                RefreshMappings();
            }
            else
            {
                m_logger->LogWarning("The project uses AWS resources, but an AWS profile has not yet been selected. Select the \"AWS, Cloud Canvas, Cloud Canvas Resource Manager\" menu item, and follow the instructions or use the \"lmbr_aws default-profile\" command line to set a default profile for the project.");
            }
        }

    }

    m_hasResourceGroups = details["ProjectUsesAWS"].value<bool>();

    if (m_initializationState == IAWSResourceManager::ErrorLoadingState)
    {

        // If we are in an error state the first time Refresh() is called,
        // we need to manually refresh the status models.

        const bool FORCE = true;

        for (auto resourceGroupStatusModel : m_resourceGroupStatusModels)
        {
            resourceGroupStatusModel->Refresh(FORCE);
        }

        if(m_resourceGroupListStatusModel)
        {
            m_resourceGroupListStatusModel->Refresh(FORCE);
        }

        if(m_deploymentListStatusModel)
        {
            m_deploymentListStatusModel->Refresh(FORCE);
        }

        for (auto deploymentStatusModel : m_deploymentStatusModels)
        {
            deploymentStatusModel->Refresh(FORCE);
        }

        if(m_projectStatusModel)
        {
            m_projectStatusModel->Refresh(FORCE);
        }

    }

    SetProjectInitialized(details["ProjectInitialized"].value<bool>(), details["ProjectInitializing"].value<bool>(), details["DefaultAWSProfile"].toString());

    if (m_projectModel)
    {
        m_projectModel->ProcessOutputProjectDescription(value);
    }

    UpdateSourceControlStates();
}

void AWSResourceManager::UpdateSourceControlStates()
{
    if (m_projectModel)
    {

        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;

        QSharedPointer<IFileSourceControlModel> projectSettingsSource = m_projectSettingsSourceControlModel;
        SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo, m_projectModel->GetProjectSettingsFile().toStdString().c_str(), [projectSettingsSource](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
            {
                projectSettingsSource->SetFlags(fileInfo.m_flags);
                projectSettingsSource->SetStatus(fileInfo.m_status);
            }
            );

        QSharedPointer<IFileSourceControlModel> deploymentTemplateSource = m_deploymentTemplateSourceControlModel;
        SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo, m_projectModel->GetDeploymentTemplateFile().toStdString().c_str(), [deploymentTemplateSource](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
            {
                deploymentTemplateSource->SetFlags(fileInfo.m_flags);
                deploymentTemplateSource->SetStatus(fileInfo.m_status);
            }
            );

        QSharedPointer<IFileSourceControlModel> gemsFileSource = m_gemsFileSourceControlModel;
        SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo, m_projectModel->GetGemsFile().toStdString().c_str(), [gemsFileSource](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
            {
                gemsFileSource->SetFlags(fileInfo.m_flags);
                gemsFileSource->SetStatus(fileInfo.m_status);
            }
            );

    }
}

bool AWSResourceManager::ProjectSettingsNeedsCheckout() const
{
    return m_projectSettingsSourceControlModel->FileNeedsCheckout();
}

bool AWSResourceManager::DeploymentTemplateNeedsCheckout() const
{
    return m_deploymentTemplateSourceControlModel->FileNeedsCheckout();
}

bool AWSResourceManager::GemsFileNeedsCheckout() const
{
    return m_gemsFileSourceControlModel->FileNeedsCheckout();
}

void AWSResourceManager::ProcessOutputMappingList(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    QVariantMap map = value.value<QVariantMap>();
    EBUS_EVENT(CloudGemFramework::CloudCanvasMappingsBus, SetProtectedMapping, map["Protected"].value<bool>());
    QVariantList mappings = map["Mappings"].value<QVariantList>();
    for (auto it = mappings.constBegin(); it != mappings.constEnd(); ++it)
    {
        QVariantMap mapping = it->value<QVariantMap>();
        QString type = mapping["ResourceType"].value<QString>();
        QString logicalName = mapping["Name"].value<QString>();
        QString physicalName = mapping["PhysicalResourceId"].value<QString>();
        EBUS_EVENT(CloudGemFramework::CloudCanvasMappingsBus, SetLogicalMapping, type.toStdString().c_str() , logicalName.toStdString().c_str() ,physicalName.toStdString().c_str());

        if (type == "Custom::CognitoUserPool")
        {
            QVariantMap clientApps = mapping["UserPoolClients"].value<QVariantMap>();
            for (auto appIter = clientApps.begin(); appIter != clientApps.end(); ++appIter)
            {
                QString clientName = appIter.key();
                QVariantMap appMapping = appIter.value().value<QVariantMap>();
                QString clientId = appMapping["ClientId"].value<QString>();
                QString clientSecret = appMapping["ClientSecret"].value<QString>();
                EBUS_EVENT(CloudGemFramework::CloudCanvasUserPoolMappingsBus, SetLogicalUserPoolClientMapping, logicalName.toStdString().c_str(), clientName.toStdString().c_str(), clientId.toStdString().c_str(), clientSecret.toStdString().c_str());
            }
        }
    }
    CheckGameConfigurations();
}

void AWSResourceManager::ProcessOutputProfileList(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    m_lastProfileList = value;
    if (m_profileModel)
    {
        m_profileModel->ProcessOutputProfileList(value);
    }
}

void AWSResourceManager::SetProjectInitialized(bool isInitialized, bool isInitializing, const QString& defaultAWSProfile)
{
    auto oldState = m_initializationState;

    if (isInitialized)
    {
        if (defaultAWSProfile.isEmpty())
        {
            m_initializationState = NoProfileState;
        }
        else
        {
            m_initializationState = InitializedState;
        }
    }
    else
    {
        if (isInitializing)
        {
            m_initializationState = InitializingState;
        }
        else if (m_initializationState != InitializingState)
        {
            m_initializationState = UninitializedState;
        }
    }

    if (oldState != m_initializationState)
    {
        CheckGameConfigurations();
        ProjectInitializedChanged();
    }
}

void AWSResourceManager::ProcessOutputResourceGroupList(AWSResourceManager::RequestId requestId, const QVariant& value)
{

    m_lastResourceGroupList = value;
    if (m_projectModel)
    {
        m_projectModel->ProcessOutputResourceGroupList(value);
    }
    if (m_resourceGroupListStatusModel)
    {
        m_resourceGroupListStatusModel->ProcessOutputResourceGroupList(value);
    }

    auto list = value.toMap()["ResourceGroups"].toList();
    QSet<QString> names;
    for (auto it = list.cbegin(); it != list.cend(); ++it)
    {
        auto resourceGroup = it->toMap();
        names.insert(resourceGroup["Name"].toString());
    }

    QSet<QString> namesToDelete;
    for (auto it = m_resourceGroupStatusModels.constBegin(); it != m_resourceGroupStatusModels.constEnd(); ++it)
    {
        auto entry = *it;
        if(names.contains(entry->ResourceGroupName()))
        {
            entry->Refresh();
        }
        else
        {
            namesToDelete.insert(entry->ResourceGroupName());
        }
    }

    for(auto name : namesToDelete)
    {
        m_resourceGroupStatusModels.remove(name);
    }

}

void AWSResourceManager::ProcessOutputImportableResourceList(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    if (m_importerModel)
    {
        m_importerModel->ProcessOutputImportableResourceList(value);
    }
}

void AWSResourceManager::ProcessOutputDeploymentList(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    m_lastDeploymentList = value;

    auto map = value.toMap();
    auto list = map["Deployments"].toList();
    QString defaultDeploymentName;
    QSet<QString> names;
    for (auto it = list.constBegin(); it != list.constEnd(); ++it)
    {
        auto entry = it->toMap();
        names.insert(entry["Name"].toString());
        if (entry["Default"].toBool())
        {
            defaultDeploymentName = entry["Name"].toString();
        }
    }

    if (m_defaultDeploymentName != defaultDeploymentName)
    {
        m_defaultDeploymentName = defaultDeploymentName;

        const bool FORCE = true;

        for (auto it = m_resourceGroupStatusModels.constBegin(); it != m_resourceGroupStatusModels.constEnd(); ++it)
        {
            it.value()->Refresh(FORCE);
        }

        ActiveDeploymentChanged(m_defaultDeploymentName);
    }

    if (m_deploymentModel)
    {
        m_deploymentModel->ProcessOutputDeploymentList(value);
    }
    if (m_projectModel)
    {
        m_projectModel->ProcessOutputDeploymentList(value);
    }
    if (m_deploymentListStatusModel)
    {
        m_deploymentListStatusModel->ProcessOutputDeploymentList(value);
    }

    QSet<QString> namesToDelete;
    for (auto it = m_deploymentStatusModels.constBegin(); it != m_deploymentStatusModels.constEnd(); ++it)
    {
        auto entry = *it;
        if (names.contains(entry->DeploymentName()))
        {
            entry->Refresh();
        }
        else
        {
            namesToDelete.insert(entry->DeploymentName());
        }
    }

    for (auto name : namesToDelete)
    {
        m_resourceGroupStatusModels.remove(name);
    }

    CheckGameConfigurations();
}

void AWSResourceManager::ProcessOutputProjectStackDescription(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    if (m_projectStatusModel)
    {
        m_projectStatusModel->ProcessOutputProjectStackDescription(value.toMap());
    }
}

void AWSResourceManager::ProcessOutputDeploymentStackDescription(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    auto map = value.toMap();
    auto deployment = map["DeploymentName"].toString();
    if (m_deploymentStatusModels.contains(deployment))
    {
        m_deploymentStatusModels[deployment]->ProcessOutputDeploymentStackDescription(map);
    }
}

void AWSResourceManager::ProcessOutputResourceGroupStackDescription(AWSResourceManager::RequestId requestId, const QVariant& value)
{
    auto map = value.toMap();
    auto resourceGroup = map["ResourceGroupName"].toString();
    if (m_resourceGroupStatusModels.contains(resourceGroup))
    {
        m_resourceGroupStatusModels[resourceGroup]->ProcessOutputResourceGroupStackDescription(map);
    }
}

void AWSResourceManager::OnFileChanged(FileChangeMonitor::MonitoredFileType fileType, QString fileName)
{
    switch (fileType)
    {
    case FileChangeMonitor::MonitoredFileType::ProjectSettings:
        RefreshProjectDescription(); // project may have been initialized
        RefreshMappingsList(); // project may have been initialized
        RefreshDeploymentList(); // default deployment may have changed; deployments may be added or removed
        RefreshResourceGroupList(); // resource groups may have been enbabled/disabled
        break;

    case FileChangeMonitor::MonitoredFileType::UserSettings:
        RefreshMappingsList(); // default mappings may have changed
        RefreshDeploymentList(); // default deployment may have changed
        RefreshProfileList(); // default profile may have changed
        RefreshProjectDescription(); // default profile may have changed
        break;

    case FileChangeMonitor::MonitoredFileType::ProjectTemplate:
        if (m_projectStatusModel)
        {
            m_projectStatusModel->Refresh(true); // force
        }
        break;

    case FileChangeMonitor::MonitoredFileType::DeploymentTemplate:
        for (auto deploymentStatusModel : m_deploymentStatusModels)
        {
            deploymentStatusModel->Refresh(true);  // force
        }
        break;

    case FileChangeMonitor::MonitoredFileType::DeploymentAccessTemplate:
        for (auto deploymentStatusModel : m_deploymentStatusModels)
        {
            deploymentStatusModel->Refresh(true);  // force
        }
        break;

    case FileChangeMonitor::MonitoredFileType::AWSCredentials:
        RefreshProfileList(); // profiles may have changed
        break;

    case FileChangeMonitor::MonitoredFileType::GUIRefresh:
        RefreshMappingsList(); // mappings may have changed
        RefreshDeploymentList(); // default deployment may have changed; deployments may be added or removed
        RefreshResourceGroupList(); // groups may have been added/removed
        break;

    case FileChangeMonitor::MonitoredFileType::GemsFile:
        RefreshResourceGroupList(); // groups may have been added/removed
        break;

    default:
        assert(0);
        break;
    }

    if (m_initializationState == InitializationState::ErrorLoadingState)
    {
        RetryLoading();
    }

}

QSharedPointer<IAWSImporterModel> AWSResourceManager::GetImporterModel()
{
    if (!m_importerModel)
    {
        m_importerModel.reset(new AWSImporterModel(this));
    }
    return m_importerModel;
}

QSharedPointer<IAWSDeploymentModel> AWSResourceManager::GetDeploymentModel()
{
    if (!m_deploymentModel)
    {
        m_deploymentModel.reset(new AWSDeploymentModel(this));
        if (m_lastDeploymentList.isNull())
        {
            RefreshDeploymentList();
        }
        else
        {
            m_deploymentModel->ProcessOutputDeploymentList(m_lastDeploymentList);
        }
    }
    return m_deploymentModel;
}

QSharedPointer<IAWSProfileModel> AWSResourceManager::GetProfileModel()
{
    if (!m_profileModel)
    {
        m_profileModel.reset(new AWSProfileModel(this));
        if (m_lastProfileList.isNull())
        {
            RefreshProfileList();
        }
        else
        {
            m_profileModel->ProcessOutputProfileList(m_lastProfileList);
        }
    }
    return m_profileModel;
}

QSharedPointer<AWSProjectModel> AWSResourceManager::GetProjectModelInternal()
{
    if (!m_projectModel)
    {
        m_projectModel.reset(new AWSProjectModel(this));
        if (m_lastDeploymentList.isNull())
        {
            RefreshDeploymentList();
        }
        else
        {
            m_projectModel->ProcessOutputDeploymentList(m_lastDeploymentList);
        }
        if (m_lastResourceGroupList.isNull())
        {
            RefreshResourceGroupList();
        }
        else
        {
            m_projectModel->ProcessOutputResourceGroupList(m_lastResourceGroupList);
        }
        if (!m_lastProjectDescription.isNull())
        {
            m_projectModel->ProcessOutputProjectDescription(m_lastProjectDescription);
        }
    }
    return m_projectModel;
}

QSharedPointer<IAWSProjectModel> AWSResourceManager::GetProjectModel()
{
    return GetProjectModelInternal();
}

QSharedPointer<ProjectStatusModel> AWSResourceManager::GetProjectStatusModel()
{
    if (!m_projectStatusModel)
    {
        m_projectStatusModel.reset(new ProjectStatusModel {this});
    }
    return m_projectStatusModel;
}

QSharedPointer<DeploymentStatusModel> AWSResourceManager::GetDeploymentStatusModel(const QString& deployment)
{
    auto result = m_deploymentStatusModels[deployment];
    if (!result)
    {
        m_deploymentStatusModels[deployment].reset(new DeploymentStatusModel {this, deployment});
        result = m_deploymentStatusModels[deployment];
    }
    return result;
}

QSharedPointer<IDeploymentStatusModel> AWSResourceManager::GetActiveDeploymentStatusModel()
{
    if (m_defaultDeploymentName.isEmpty())
    {
        if (!m_placeholderDeploymentStatusModel)
        {
            m_placeholderDeploymentStatusModel.reset(new PlaceholderDeploymentStatusModel {this});
        }
        return m_placeholderDeploymentStatusModel;
    }
    else
    {
        return GetDeploymentStatusModel(m_defaultDeploymentName);
    }
}

QSharedPointer<ResourceGroupStatusModel> AWSResourceManager::GetResourceGroupStatusModel(const QString& resourceGroup)
{
    auto result = m_resourceGroupStatusModels[resourceGroup];
    if (!result)
    {
        m_resourceGroupStatusModels[resourceGroup].reset(new ResourceGroupStatusModel {this, resourceGroup});
        result = m_resourceGroupStatusModels[resourceGroup];
    }
    return result;
}

QSharedPointer<DeploymentListStatusModel> AWSResourceManager::GetDeploymentListStatusModel()
{
    if (!m_deploymentListStatusModel)
    {
        m_deploymentListStatusModel.reset(new DeploymentListStatusModel {this});
        if (m_lastDeploymentList.isValid())
        {
            m_deploymentListStatusModel->ProcessOutputDeploymentList(m_lastDeploymentList);
        }
        m_deploymentListStatusModel->Refresh();
    }
    return m_deploymentListStatusModel;
}

QSharedPointer<ResourceGroupListStatusModel> AWSResourceManager::GetResourceGroupListStatusModel()
{
    if (!m_resourceGroupListStatusModel)
    {
        m_resourceGroupListStatusModel.reset(new ResourceGroupListStatusModel {this});
        if (m_lastResourceGroupList.isValid())
        {
            m_resourceGroupListStatusModel->ProcessOutputResourceGroupList(m_lastResourceGroupList);
        }
        m_resourceGroupListStatusModel->Refresh();
    }
    return m_resourceGroupListStatusModel;
}

void AWSResourceManager::RequestEditProjectSettings()
{
    QSharedPointer<IFileSourceControlModel> projectSettingsSource = m_projectSettingsSourceControlModel;

    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    QString path = GetProjectModel()->GetProjectSettingsFile();
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, path.toStdString().c_str(), true, [projectSettingsSource, path](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
        {
            projectSettingsSource->SetFlags(fileInfo.m_flags);
            projectSettingsSource->SetStatus(fileInfo.m_status);
            if (!wasSuccess) {
                QMessageBox::critical(GetIEditor()->GetEditorMainWindow(), "Checkout error", "Failed to check file out from source control: " + path);
            }
        }
        );
}

void AWSResourceManager::RequestEditDeploymentTemplate()
{
    QSharedPointer<IFileSourceControlModel> deploymentTemplateSource = m_deploymentTemplateSourceControlModel;

    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, GetProjectModel()->GetDeploymentTemplateFile().toStdString().c_str(), true, [deploymentTemplateSource](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
        {
            deploymentTemplateSource->SetFlags(fileInfo.m_flags);
            deploymentTemplateSource->SetStatus(fileInfo.m_status);
        }
        );
}


void AWSResourceManager::RequestEditGemsFile()
{
    QSharedPointer<IFileSourceControlModel> sourceControlModel = m_gemsFileSourceControlModel;
    QString path = GetProjectModel()->GetGemsFile();

    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, path.toStdString().c_str(), true, [sourceControlModel, path](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
    {
        sourceControlModel->SetFlags(fileInfo.m_flags);
        sourceControlModel->SetStatus(fileInfo.m_status);
        if (!wasSuccess)
        {
            QMessageBox::critical(GetIEditor()->GetEditorMainWindow(), "Checkout error", "Failed to check file out from source control: " + path);
        }
    }
    );
}

void AWSResourceManager::RequestAddFile(const QString& path)
{
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, path.toStdString().c_str(), true, [path](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
    {
        if (!wasSuccess)
        {
            QMessageBox::critical(GetIEditor()->GetEditorMainWindow(), "Mark for add error", "Failed to mark the file for add: " + path);
        }
    }
    );
}

void AWSResourceManager::RequestDeleteFile(const QString& path)
{
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestDelete, path.toStdString().c_str(), [path](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
    {
        if (!wasSuccess && fileInfo.HasFlag(AzToolsFramework::SCF_Tracked))
        {
            QMessageBox::critical(GetIEditor()->GetEditorMainWindow(), "Mark for delete error", "Failed to mark the file for delete: " + path);
        }
    }
    );
}


void AWSResourceManager::RequestUpdateSourceModel(QSharedPointer<IFileSourceControlModel> sourceModel, QSharedPointer<IFileContentModel> contentModel)
{
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo, contentModel->Path().toStdString().c_str(), [sourceModel, contentModel](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
        {
            sourceModel->SetFlags(fileInfo.m_flags);
            sourceModel->SetStatus(fileInfo.m_status);
        }
        );
}

void AWSResourceManager::RequestEditSourceModel(QSharedPointer<IFileSourceControlModel> sourceModel, QSharedPointer<IFileContentModel> contentModel)
{
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, contentModel->Path().toStdString().c_str(), true, [sourceModel, contentModel](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
        {
            sourceModel->SetFlags(fileInfo.m_flags);
            sourceModel->SetStatus(fileInfo.m_status);
        }
        );
}

void AWSResourceManager::GetRegionList()
{
    QVariantMap args;
    ExecuteAsync(AllocateRequestId(), "list-regions", args);     // TODO: error handling
}

bool AWSResourceManager::InitializeProject(const QString& region, const QString& stackName, const QString& accessKey, const QString& secretKey)
{
    m_initializationState = InitializingState;
    ProjectInitializedChanged();

    GetProjectModelInternal()->OnProjectStackInProgress();

    QVariantMap args;
    args["region"] = region;
    args["stack_name"] = stackName;
    if (!accessKey.isEmpty())
    {
        args["aws_access_key"] = accessKey;
    }
    if (!secretKey.isEmpty())
    {
        args["aws_secret_key"] = secretKey;
    }
    args["confirm_aws_usage"] = true;
    args["confirm_security_change"] = true;
    args["cognito_prod"] = GetIdentityId().c_str();
    ExecuteAsync(GetProjectStatusModel()->GetStackEventsModelInternal()->GetRequestId(), "create-project-stack", args);

    return true;
}

void AWSResourceManager::OperationStarted()
{
    ++m_operationsInProgress;
    if (m_operationsInProgress == 1)
    {
        OperationInProgressChanged();
    }
}

void AWSResourceManager::OperationFinished()
{
    --m_operationsInProgress;
    assert(m_operationsInProgress >= 0);
    if (m_operationsInProgress == 0)
    {
        OperationInProgressChanged();
    }
}

bool AWSResourceManager::IsOperationInProgress()
{
    return m_operationsInProgress > 0;
}

void AWSResourceManager::RefreshHasResourceGroupsStatus()
{
    if (m_lastResourceGroupList.isValid())
    {
        if (m_lastResourceGroupList.toMap()["ResourceGroups"].toList().count() > 0)
        {
            m_hasResourceGroups = true;
        }
        else
        {
            m_hasResourceGroups = false;
        }
    }
    else
    {
        m_hasResourceGroups = false;
    }
}

void AWSResourceManager::OpenCGP()
{

    auto callback = [](const QString& key, const QVariant& value)
    {
        if (key == "error")
        {
            QMessageBox::critical(GetIEditor()->GetEditorMainWindow(), "Cloud Gem Portal", value.toString());
        }
    };

    bool failOnLoadError = true;
    QVariantMap args;
    ExecuteAsync(callback, "cloud-gem-portal", args, failOnLoadError);

}

void AWSResourceManager::RefreshProjectDescription()
{
    ExecuteAsyncWithRetry(m_describeProjectRequestId, "describe-project");
}

void AWSResourceManager::RefreshResourceGroupList()
{
    ExecuteAsyncWithRetry(m_listResourceGroupsRequestId, "list-resource-groups");
}

void AWSResourceManager::RefreshDeploymentList()
{
    ExecuteAsyncWithRetry(m_listDeploymentsRequestId, "list-deployments");
}

void AWSResourceManager::RefreshProfileList()
{
    ExecuteAsyncWithRetry(m_listProfilesRequestId, "list-profiles");
}

void AWSResourceManager::RefreshMappingsList()
{
    ExecuteAsyncWithRetry(m_listMappingsRequestId, "list-mappings");
}

void AWSResourceManager::RefreshMappings()
{
    ExecuteAsyncWithRetry(m_updateMappingsRequestId, "update-mappings");
}
