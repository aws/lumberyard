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

#pragma once

#include <QObject>
#include <IAWSResourceManager.h>
#include <FileChangeMonitor.h>

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/map.h>

#include "PythonWorker.h"

struct IEditor;
class CloudCanvasLogger;
class AWSImporterModel;
class AWSProfileModel;
class AWSDeploymentModel;
class AWSProjectModel;
class CloudFormationTemplateModel;
class ProjectStatusModel;
class DeploymentStatusModel;
class ResourceGroupStatusModel;
class DeploymentListStatusModel;
class ResourceGroupListStatusModel;
class IFileSourceControlModel;

class AWSResourceManager
    : public IAWSResourceManager
{
    Q_OBJECT

public:

    AWSResourceManager(IEditor* editor);
    ~AWSResourceManager();

    QSharedPointer<IAWSImporterModel> GetImporterModel() override;
    QSharedPointer<IAWSDeploymentModel> GetDeploymentModel() override;
    QSharedPointer<IAWSProfileModel> GetProfileModel() override;
    QSharedPointer<IAWSProjectModel> GetProjectModel() override;

    QSharedPointer<AWSProjectModel> GetProjectModelInternal();

    QSharedPointer<ProjectStatusModel> GetProjectStatusModel();
    QSharedPointer<DeploymentStatusModel> GetDeploymentStatusModel(const QString& deployment);
    QSharedPointer<ResourceGroupStatusModel> GetResourceGroupStatusModel(const QString& resourceGroup);
    QSharedPointer<DeploymentListStatusModel> GetDeploymentListStatusModel();
    QSharedPointer<ResourceGroupListStatusModel> GetResourceGroupListStatusModel();
    QSharedPointer<IDeploymentStatusModel> GetActiveDeploymentStatusModel();

    void OnInit();
    void OnBeginGameMode();
    void CheckGameConfigurations();
    void RefreshGameConfigurations();

    bool IsProjectInitialized() const override
    {
        return m_initializationState == InitializedState;
    }

    InitializationState GetInitializationState() override
    {
        return m_initializationState;
    }

    QString GetErrorString() const override;

    void RetryLoading() override;

    // The ExecuteAsync and ExecuteAsyncWithRetry methods can be used to submit 
    // commands to the resource manager's Python implemeantion.
    //
    // There are two ways to get output. The prefered (and newer) way is to provide an
    // ExecuteAsyncCallback function when submitting the request. 
    //
    // Alturnatively you can provide a requestId produce by calling AllocateRequestId and
    // wire up a handler to the CommandOutput signal. In this case YOU MUST IGNORE
    // all invocations where the resourceId is not the one provided when submitting the
    // request. 
    //
    // The callback or signal function will be called once for each output produced by the 
    // operation. This may include output data with a key value unqiue to the command. It
    // may also include progress messages where key is "message". If the operation succeeds, 
    // the function will be called for a final time with key == "success". If the 
    // operation fails, the function will be called for a final time with key == "error".
    //
    // You can also submit commands with retry enabled. If these command fail, they are
    // put in failed command list and the project's state is set to ErrorLoadingState. The
    // failed command can then be retried by calling RetryLoading. When retry is enabled,
    // the callback or signal function will not be called with key == "error".

    using RequestId = PythonWorkerRequestId;
    RequestId AllocateRequestId() { return m_pythonWorker->AllocateRequestId(); }

    void ExecuteAsync(RequestId requestId, const char* command, const QVariantMap& args = QVariantMap{}, bool failOnLoadError = false);
    void ExecuteAsyncWithRetry(RequestId requestId, const char* command, const QVariantMap& args = QVariantMap{});

    using ExecuteAsyncCallback = AZStd::function<void(const QString& key, const QVariant& value)>;
    void ExecuteAsync(ExecuteAsyncCallback callback, const char* command, const QVariantMap& args = QVariantMap{}, bool failOnLoadError = false);
    void ExecuteAsyncWithRetry(ExecuteAsyncCallback callback, const char* command, const QVariantMap& args = QVariantMap{});

    void GetRegionList() override;
    bool InitializeProject(const QString& region, const QString& stackName, const QString& accessKey, const QString& secretKey) override;

    void RequestEditProjectSettings() override;
    void RequestEditDeploymentTemplate() override;
    void RequestEditGemsFile() override;
    void RequestAddFile(const QString& path) override;
    void RequestDeleteFile(const QString& path) override;

    static void RequestUpdateSourceModel(QSharedPointer<IFileSourceControlModel> sourceModel, QSharedPointer<IFileContentModel> contentModel);
    static void RequestEditSourceModel(QSharedPointer<IFileSourceControlModel> sourceModel, QSharedPointer<IFileContentModel> contentModel);

    bool ProjectSettingsNeedsCheckout() const override;
    bool DeploymentTemplateNeedsCheckout() const override;
    bool GemsFileNeedsCheckout() const override;

    QSharedPointer<IFileSourceControlModel> GetProjectSettingsSourceModel()  override { return m_projectSettingsSourceControlModel; }
    QSharedPointer<IFileSourceControlModel> GetDeploymentTemplateSourceModel()  override { return m_deploymentTemplateSourceControlModel; }
    QSharedPointer<IFileSourceControlModel> GetGemsFileSourceModel()  override { return m_gemsFileSourceControlModel; }

    // Status
    void UpdateSourceControlStates() override;

    bool IsOperationInProgress() override;
        
    void OperationStarted();
    void OperationFinished();

    void OpenCGP() override;

    /// Returns true if resource groups have been created locally. If not, it's likely that the stack contains no deployments.
    bool HasResourcGroups()
    {
        return m_hasResourceGroups;
    }

    /// This gets called when a stack resource model updates and makes sure HasResourceGroups returns the correct value
    void RefreshHasResourceGroupsStatus();

    /// Pointer to Cloud Canvas specific logger
    QSharedPointer<CloudCanvasLogger> GetLogger() { return m_logger; }
    
    void RefreshDeploymentList();
    void RefreshProjectDescription();
    void RefreshResourceGroupList();
    void RefreshProfileList();
    void RefreshMappingsList();
    void RefreshMappings();

    AZStd::string GetIdentityId();

signals:

    void ProcessOutputResourceList(RequestId requestId, const QVariant& value);
    void CommandOutput(RequestId requestId, const char* key, const QVariant& value);
    void ActiveDeploymentChanged(const QString& activeDeploymentName);

private slots:

    void ProcessPythonOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value);
    void OnFileChanged(FileChangeMonitor::MonitoredFileType fileType, QString fileName);

private:

    // Editor

    IEditor* m_editor;

    static AZStd::string GetRootDirectoryPath();
    static AZStd::string GetGameDirectoryPath();
    static AZStd::string GetUserDirectoryPath(IEditor* editor);

    // Python

    QSharedPointer<PythonWorker> m_pythonWorker;
    void ProcessOutputMessage(RequestId requestId, const QVariant& value);
    void ProcessOutputError(RequestId requestId, const QVariant& value, bool log = true);
    void ProcessOutputErrorWithLogging(RequestId requestId, const QVariant& value);
    void ProcessProjectStackExists(RequestId requestId, const QVariant& value);
    void ProcessFrameworkNotEnabledError(RequestId requestId, const QVariant& value);
    void ProcessOutputSuccess(RequestId requestId, const QVariant& value);
    void ProcessOutputProjectDescription(RequestId requestId, const QVariant& value);
    void ProcessOutputMappingList(RequestId requestId, const QVariant& value);
    void ProcessOutputResourceGroupList(RequestId requestId, const QVariant& value);
    void ProcessOutputDeploymentList(RequestId requestId, const QVariant& value);
    void ProcessOutputProfileList(RequestId requestId, const QVariant& value);
    void ProcessOutputImportableResourceList(RequestId requestId, const QVariant& value);
    void ProcessOutputProjectStackDescription(RequestId requestId, const QVariant& value);
    void ProcessOutputDeploymentStackDescription(RequestId requestId, const QVariant& value);
    void ProcessOutputResourceGroupStackDescription(RequestId requestId, const QVariant& value);
    void ProcessOutputStackEventErrors(RequestId requestId, const QVariant& value);
    void ProcessOutputSupportedRegionList(RequestId requestId, const QVariant& value);
    void ProcessOutputCreateAdmin(RequestId requestId, const QVariant& value);

    QVariant m_lastProjectDescription;
    QVariant m_lastResourceGroupList;
    QVariant m_lastDeploymentList;
    QVariant m_lastProfileList;

    using PythonOutputHandlerMap = QMap<QString, void (AWSResourceManager::*)(RequestId, const QVariant&)>;
    static PythonOutputHandlerMap s_PythonOutputHandlers;

    bool m_startupComplete;
    bool m_hasResourceGroups = false;

    using PythonCallbackMap = QMap<RequestId, ExecuteAsyncCallback>;
    PythonCallbackMap m_pythonCallbacks;

    void SetProjectInitialized(bool isInitialized, bool isInitializing, const QString& defaultAWSProfile);
    InitializationState m_initializationState;

    // File Montoring and Refresh

    QSharedPointer<FileChangeMonitor> m_fileMonitor;

    // Logging

    QSharedPointer<CloudCanvasLogger> m_logger;

    // Models

    QSharedPointer<AWSImporterModel> m_importerModel;
    QSharedPointer<AWSProfileModel> m_profileModel;
    QSharedPointer<AWSDeploymentModel> m_deploymentModel;
    QSharedPointer<AWSProjectModel> m_projectModel;
    QSharedPointer<ProjectStatusModel> m_projectStatusModel;
    QMap<QString, QSharedPointer<DeploymentStatusModel> > m_deploymentStatusModels;
    QMap<QString, QSharedPointer<ResourceGroupStatusModel> > m_resourceGroupStatusModels;
    QSharedPointer<DeploymentListStatusModel> m_deploymentListStatusModel;
    QSharedPointer<ResourceGroupListStatusModel> m_resourceGroupListStatusModel;
    QSharedPointer<IDeploymentStatusModel> m_placeholderDeploymentStatusModel;

    QSharedPointer<IFileSourceControlModel> m_projectSettingsSourceControlModel;
    QSharedPointer<IFileSourceControlModel> m_deploymentTemplateSourceControlModel;
    QSharedPointer<IFileSourceControlModel> m_gemsFileSourceControlModel;
    QString m_defaultDeploymentName;

    int m_operationsInProgress {0};

    struct RetryableRequest
    {
        const char* command;
        QVariantMap args;
    };

    QMap<RequestId, RetryableRequest> m_retryableRequestMap;

    struct FailedRequest
    {
        QString errorMessage;
    };

    QMap<RequestId, FailedRequest> m_failedRequestMap;

    RequestId m_listDeploymentsRequestId;
    RequestId m_describeProjectRequestId;
    RequestId m_listResourceGroupsRequestId;
    RequestId m_listProfilesRequestId;
    RequestId m_listMappingsRequestId;
    RequestId m_updateMappingsRequestId;

};

