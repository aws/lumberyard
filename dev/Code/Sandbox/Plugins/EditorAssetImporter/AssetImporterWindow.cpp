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

#include <StdAfx.h>
#include <AssetImporterWindow.h>
#include <ui_AssetImporterWindow.h>
#include <AssetImporterPlugin.h>
#include <ImporterRootDisplay.h>
#include <TraceMessageAggregator.h>

#include <QFile>
#include <QTimer>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDesktopServices.h>
#include <QUrl>
#include <QDockWidget>
#include <QLabel>

class IXMLDOMDocumentPtr; // Needed for settings.h
class CXTPDockingPaneLayout; // Needed for settings.h
#include <Settings.h>

#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/functional.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>
#include <Util/PathUtil.h>
#include <ActionOutput.h>

#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/ReportCard.h>
#include <SceneAPI/SceneUI/CommonWidgets/ProcessingOverlayWidget.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ExportJobProcessingHandler.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphInspectWidget.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

const char* AssetImporterWindow::s_documentationWebAddress = "http://docs.aws.amazon.com/lumberyard/latest/userguide/char-fbx-importer.html";
const AZ::Uuid AssetImporterWindow::s_browseTag = AZ::Uuid::CreateString("{C240D2E1-BFD2-4FFA-BB5B-CC0FA389A5D3}");

void MakeUserFriendlySourceAssetPath(QString& out, const QString& sourcePath)
{
    char devAssetsRoot[AZ_MAX_PATH_LEN] = { 0 };
    if (!gEnv->pFileIO->ResolvePath("@devroot@", devAssetsRoot, AZ_MAX_PATH_LEN))
    {
        out = sourcePath;
        return;
    }

    AZStd::replace(devAssetsRoot, devAssetsRoot + AZ_MAX_PATH_LEN- 1, AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);
    if (!AzFramework::StringFunc::Path::IsASubFolderOfB(sourcePath.toUtf8(), devAssetsRoot))
    {
        out = sourcePath;
        return;
    }

    int offset = aznumeric_cast<int>(strlen(devAssetsRoot));
    if (sourcePath.at(offset) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
    {
        ++offset;
    }
    out = sourcePath.right(sourcePath.length() - offset);

}

AssetImporterWindow::AssetImporterWindow()
    : AssetImporterWindow(nullptr)
{
}

AssetImporterWindow::AssetImporterWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::AssetImporterWindow)
    , m_assetImporterDocument(new AssetImporterDocument())
    , m_serializeContext(nullptr)
    , m_rootDisplay(nullptr)
    , m_overlay(nullptr)
    , m_isClosed(false)
    , m_processingOverlayIndex(AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex)
{
    Init();
}

AssetImporterWindow::~AssetImporterWindow()
{
    AZ_Assert(m_processingOverlayIndex == AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex, 
        "Processing overlay (and potentially background thread) still active at destruction.");
    AZ_Assert(!m_processingOverlay, "Processing overlay (and potentially background thread) still active at destruction.");
}

void AssetImporterWindow::OpenFile(const AZStd::string& filePath)
{
    if (m_processingOverlay)
    {
        QMessageBox::warning(this, "In progress", "Please wait for the previous task to complete before opening a new file.");
        return;
    }

    if (!m_overlay->CanClose())
    {
        QMessageBox::warning(this, "In progress", "Unable to close one or more windows at this time.");
        return;
    }

    // Make sure we are not browsing *over* a current editing operation
    if (!IsAllowedToChangeSourceFile())
    {
        // Issue will already have been reported to the user.
        return;
    }

    if (!m_overlay->PopAllLayers())
    {
        QMessageBox::warning(this, "In progress", "Unable to close one or more windows at this time.");
        return;
    }
    
    OpenFileInternal(AZStd::make_shared<TraceMessageAggregator>(s_browseTag), filePath);
}

void AssetImporterWindow::closeEvent(QCloseEvent* ev)
{
    if (m_isClosed)
    {
        return;
    }

    if (m_processingOverlay)
    {
        AZ_Assert(m_processingOverlayIndex != AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex, 
            "Processing overlay present, but not the index in the overlay for it.");
        if (m_processingOverlay->HasProcessingCompleted())
        {
            if (m_overlay->PopLayer(m_processingOverlayIndex))
            {
                m_processingOverlayIndex = AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex;
                m_processingOverlay.reset(nullptr);
            }
            else
            {
                QMessageBox::critical(this, "Processing In Progress", "Unable to close the result window at this time.", 
                    QMessageBox::Ok, QMessageBox::Ok);
                ev->ignore();
                return;
            }
        }
        else
        {
            QMessageBox::critical(this, "Processing In Progress", "Please wait until processing has completed to try again.", 
                QMessageBox::Ok, QMessageBox::Ok);
            ev->ignore();
            return;
        }
    }

    if (!m_overlay->CanClose())
    {
        QMessageBox::critical(this, "Unable to close", "Unable to close one or more windows at this time.",
            QMessageBox::Ok, QMessageBox::Ok);
        ev->ignore();
        return;
    }

    if (!IsAllowedToChangeSourceFile())
    {
        ev->ignore();
        return;
    }

    ev->accept();
    m_isClosed = true;
}

void AssetImporterWindow::Init()
{
    // Serialization and reflection framework setup
    EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(m_serializeContext, "Serialization context not available");

    // Load the style sheets
    AzQtComponents::StylesheetPreprocessor styleSheetProcessor(nullptr);

    AZStd::string mainWindowQSSPath = Path::GetEditingRootFolder() + "\\Editor\\Styles\\AssetImporterWindow.qss";
    QFile mainWindowStyleSheetFile(mainWindowQSSPath.c_str());
    if (mainWindowStyleSheetFile.open(QFile::ReadOnly))
    {
        setStyleSheet(styleSheetProcessor.ProcessStyleSheet(mainWindowStyleSheetFile.readAll()));
    }

    ui->setupUi(this);

    if (!gSettings.enableSceneInspector)
    {
        ui->m_actionInspect->setVisible(false);
    }
    
    ResetMenuAccess(WindowState::InitialNothingLoaded);

    // Setup the overlay system, and set the root to be the root display. The root display has the browse,
    //  the Import button & the cancel button, which are handled here by the window.
    m_overlay.reset(aznew AZ::SceneAPI::UI::OverlayWidget(this));
    m_rootDisplay.reset(aznew ImporterRootDisplay(m_serializeContext));
    connect(m_rootDisplay.data(), &ImporterRootDisplay::UpdateClicked, this, &AssetImporterWindow::UpdateClicked);
    
    connect(m_overlay.data(), &AZ::SceneAPI::UI::OverlayWidget::LayerAdded, this, &AssetImporterWindow::OverlayLayerAdded);
    connect(m_overlay.data(), &AZ::SceneAPI::UI::OverlayWidget::LayerRemoved, this, &AssetImporterWindow::OverlayLayerRemoved);

    m_overlay->SetRoot(m_rootDisplay.data());
    ui->m_mainArea->layout()->addWidget(m_overlay.data());

    // Filling the initial browse prompt text to be programmatically set from available extensions
    AZStd::unordered_set<AZStd::string> extensions;
    EBUS_EVENT(AZ::SceneAPI::Events::AssetImportRequestBus, GetSupportedFileExtensions, extensions);
    AZ_Assert(!extensions.empty(), "No file extensions defined for assets.");
    if (!extensions.empty())
    {
        for (AZStd::string& extension : extensions)
        {
            extension = extension.substr(1);
            AZStd::to_upper(extension.begin(), extension.end());
        }

        AZStd::string joinedExtensions;
        AzFramework::StringFunc::Join(joinedExtensions, extensions.begin(), extensions.end(), " or ");

        AZStd::string firstLineText = 
            AZStd::string::format(
                "%s files are available for use after placing them in any folder within your game project. "
                "These files will automatically be processed and may be accessed via the Asset Browser. <a href=\"%s\">Learn more...</a>",
                joinedExtensions.c_str(), s_documentationWebAddress);

        ui->m_initialPromptFirstLine->setText(firstLineText.c_str());

        AZStd::string secondLineText = 
            AZStd::string::format("To adjust the %s settings, right-click the file in the Asset Browser and select \"Edit Settings\" from the context menu.", joinedExtensions.c_str());
        ui->m_initialPromptSecondLine->setText(secondLineText.c_str());
    }
    else
    {
        AZStd::string firstLineText = 
            AZStd::string::format(
                "Files are available for use after placing them in any folder within your game project. "
                "These files will automatically be processed and may be accessed via the Asset Browser. <a href=\"%s\">Learn more...</a>", s_documentationWebAddress);

        ui->m_initialPromptFirstLine->setText(firstLineText.c_str());

        AZStd::string secondLineText = "To adjust the settings, right-click the file in the Asset Browser and select \"Edit Settings\" from the context menu.";
        ui->m_initialPromptSecondLine->setText(secondLineText.c_str());

        // Hide the initial browse container so we can show the error (it will be shown again when the overlay pops)
        ui->m_initialBrowseContainer->hide();

        AZ::SceneAPI::UI::ReportWidget::ReportAssert(
            "No Extensions Detected",
            "No importable file types were detected. This likely means an internal error has taken place which has broken the "
                "registration of valid import types (e.g. FBX). This type of issue requires engineering support.",
            m_overlay.data());
    }
}

void AssetImporterWindow::OpenFileInternal(AZStd::shared_ptr<TraceMessageAggregator>&& logger, const AZStd::string& filePath)
{
    setCursor(Qt::WaitCursor);
    
    auto asyncLoadHandler = AZStd::make_shared<AZ::SceneAPI::SceneUI::AsyncOperationProcessingHandler>(
        [this, filePath]()
        { 
            m_assetImporterDocument->LoadScene(filePath); 
        },
        [this, logger]()
        {
            HandleAssetLoadingCompleted(logger); 
        }
    );

    m_processingOverlay.reset(new AZ::SceneAPI::SceneUI::ProcessingOverlayWidget(m_overlay.data(), asyncLoadHandler));
    m_processingOverlay->SetAutoCloseOnSuccess(true);
    m_processingOverlay->AddInfo(AZStd::string::format("Loading source asset '%s'", filePath.c_str()));

    connect(m_processingOverlay.data(), &AZ::SceneAPI::SceneUI::ProcessingOverlayWidget::Closing, 
        [this]() 
        {
            m_processingOverlayIndex = AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex;
            m_processingOverlay.reset(nullptr);
        }
    );
    m_processingOverlayIndex = m_processingOverlay->PushToOverlay();
}

bool AssetImporterWindow::IsAllowedToChangeSourceFile()
{
    if (!m_rootDisplay->HasUnsavedChanges())
    {
        return true;
    }


    QMessageBox messageBox(QMessageBox::Icon::NoIcon, "Unsaved changes", 
        "You have unsaved changes. Do you want to discard those changes?",
        QMessageBox::StandardButton::Discard | QMessageBox::StandardButton::Cancel, this);
    messageBox.exec();
    QMessageBox::StandardButton choice = static_cast<QMessageBox::StandardButton>(messageBox.result());
    return choice == QMessageBox::StandardButton::Discard;
}

void AssetImporterWindow::UpdateClicked()
{
    // There are specific measures in place to block re-entrancy, applying asserts to be safe
    if (m_processingOverlay)
    {
        AZ_Assert(!m_processingOverlay, "Attempted to update asset while processing is in progress.");
        return;
    }

    TraceMessageAggregator logger(s_browseTag);
    AZ_TraceContext("Asset Importer Browse Tag", s_browseTag);

    auto saveAndWaitForJobsHandler = AZStd::make_shared<AZ::SceneAPI::SceneUI::ExportJobProcessingHandler>(m_fullSourcePath);
    m_processingOverlay.reset(new AZ::SceneAPI::SceneUI::ProcessingOverlayWidget(m_overlay.data(), saveAndWaitForJobsHandler));
    connect(m_processingOverlay.data(), &AZ::SceneAPI::SceneUI::ProcessingOverlayWidget::Closing, 
        [this]() 
        { 
            m_processingOverlayIndex = AZ::SceneAPI::UI::OverlayWidget::s_invalidOverlayIndex;
            m_processingOverlay.reset(nullptr);
        }
    );

    m_processingOverlayIndex = m_processingOverlay->PushToOverlay();

    bool isSourceControlActive = false;
    {
        using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
        SCRequestBus::BroadcastResult(isSourceControlActive, &SCRequestBus::Events::IsActive);
    }

    if (!isSourceControlActive)
    {
        m_processingOverlay->AddInfo("Waiting for saving to complete");
    }
    else
    {
        m_processingOverlay->AddInfo("Waiting for saving & source control to complete");
    }
    
    // We need to block closing of the overlay until source control operations are complete
    m_processingOverlay->BlockClosing();

    AZStd::shared_ptr<AZ::ActionOutput> output = AZStd::make_shared<AZ::ActionOutput>();
    m_assetImporterDocument->SaveScene(output,
        [output, this, isSourceControlActive](bool wasSuccessful)
        {
            if (output->HasAnyWarnings())
            {
                m_processingOverlay->AddWarning(output->BuildWarningMessage());
            }
            if (output->HasAnyErrors())
            {
                m_processingOverlay->AddError(output->BuildErrorMessage());
            }

            if (wasSuccessful)
            {
                if (!isSourceControlActive)
                {
                    m_processingOverlay->AddInfo("Saving complete");
                }
                else
                {
                    m_processingOverlay->AddInfo("Saving & source control operations complete");
                }

                m_rootDisplay->HandleSaveWasSuccessful();
            }
            else
            {
                // This kind of failure means that it's possible the jobs will never actually start,
                //  so we act like the processing is complete to make it so the user won't be stuck
                //  in the processing UI in that case.
                m_processingOverlay->OnProcessingComplete();
            }
            
            m_processingOverlay->UnblockClosing();
        }
    );
}

void AssetImporterWindow::OnSceneResetRequested()
{
    static const AZ::Uuid sceneResetTag = AZ::Uuid::CreateString("{232E21B6-58F2-4109-B4C2-0A7096237E22}");
    TraceMessageAggregator logger(sceneResetTag);
    AZ_TraceContext("Asset Importer Reset Tag", sceneResetTag);

    m_assetImporterDocument->GetScene()->GetManifest().Clear();

    AZ::SceneAPI::Events::ProcessingResultCombiner result;
    EBUS_EVENT_RESULT(result, AZ::SceneAPI::Events::AssetImportRequestBus, UpdateManifest, 
        *m_assetImporterDocument->GetScene(),
        AZ::SceneAPI::Events::AssetImportRequest::ManifestAction::ConstructDefault,
        AZ::SceneAPI::Events::AssetImportRequest::RequestingApplication::Editor);
    
    // Specifically using success, because ignore would be an invalid case.
    //  Whenever we do construct default, it should always be done
    if (result.GetResult() == AZ::SceneAPI::Events::ProcessingResult::Success)
    {
        m_rootDisplay->HandleSceneWasReset(m_assetImporterDocument->GetScene());
    }
    else
    {
        AZ_TracePrintf("Error", "Manifest reset returned in '%s'", result.GetResult() == AZ::SceneAPI::Events::ProcessingResult::Failure ? "Failure" : "Ignored");

        AZ::SceneAPI::UI::OverlayWidget* overlay = AZ::SceneAPI::UI::OverlayWidget::GetContainingOverlay(this);
        logger.Report("Failed to reset scene settings", overlay);
    }
}

void AssetImporterWindow::ResetMenuAccess(WindowState state)
{
    if (state == WindowState::FileLoaded)
    {
        ui->m_actionResetSettings->setEnabled(true);
        ui->m_actionInspect->setEnabled(true);
    }
    else
    {
        ui->m_actionResetSettings->setEnabled(false);
        ui->m_actionInspect->setEnabled(false);
    }
}

void AssetImporterWindow::OnOpenDocumentation()
{
    QDesktopServices::openUrl(QString(s_documentationWebAddress));
}

void AssetImporterWindow::OnInspect()
{
    AZ::SceneAPI::UI::OverlayWidgetButtonList buttons;
    AZ::SceneAPI::UI::OverlayWidgetButton closeButton;
    closeButton.m_text = "Close";
    closeButton.m_triggersPop = true;
    buttons.push_back(&closeButton);

    QLabel* label = new QLabel("Please close the inspector to continue editing the settings.");
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    m_overlay->PushLayer(label, aznew AZ::SceneAPI::UI::SceneGraphInspectWidget(*m_assetImporterDocument->GetScene()), "Scene Inspector", buttons);
}

void AssetImporterWindow::OverlayLayerAdded()
{
    ResetMenuAccess(WindowState::OverlayShowing);
}

void AssetImporterWindow::OverlayLayerRemoved()
{
    if (m_isClosed && !m_overlay->IsAtRoot())
    {
        return;
    }

    // Reset menu access
    if (m_assetImporterDocument->GetScene())
    {
        ResetMenuAccess(WindowState::FileLoaded);
    }
    else
    {
        ResetMenuAccess(WindowState::InitialNothingLoaded);
        
        ui->m_initialBrowseContainer->show();
        m_rootDisplay->hide();
    }
}

void AssetImporterWindow::SetTitle(const char* filePath)
{
    QWidget* dock = parentWidget();
    while (dock)
    {
        QDockWidget* converted = qobject_cast<QDockWidget*>(dock);
        if (converted)
        {
            AZStd::string extension;
            if (AzFramework::StringFunc::Path::GetExtension(filePath, extension, false))
            {
                extension[0] = toupper(extension[0]);
                for (size_t i = 1; i < extension.size(); ++i)
                {
                    extension[i] = tolower(extension[i]);
                }
            }
            else
            {
                extension = "Scene";
            }
            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(filePath, fileName);
            converted->setWindowTitle(QString("%1 Settings (PREVIEW) - %2").arg(extension.c_str(), fileName.c_str()));
            break;
        }
        else
        {
            dock = dock->parentWidget();
        }
    }
}

void AssetImporterWindow::HandleAssetLoadingCompleted(AZStd::shared_ptr<TraceMessageAggregator> logger)
{
    setCursor(Qt::ArrowCursor);

    if (!m_assetImporterDocument->GetScene())
    {
        logger->Report(m_processingOverlay->GetReportWidget().data());
        m_processingOverlay->AddError("Failed to load scene.");
        return;
    }

    if (logger->HasWarnings())
    {
        logger->Report(m_processingOverlay->GetReportWidget().data());
        m_processingOverlay->AddWarning("Warnings found during scene loading.");
    }

    m_fullSourcePath = m_assetImporterDocument->GetScene()->GetSourceFilename();
    SetTitle(m_fullSourcePath.c_str());

    QString userFriendlyFileName;
    MakeUserFriendlySourceAssetPath(userFriendlyFileName, m_fullSourcePath.c_str());
    m_rootDisplay->SetSceneDisplay(userFriendlyFileName, m_assetImporterDocument->GetScene());

    ResetMenuAccess(WindowState::FileLoaded);
    
    // Once we've browsed to something successfully, we need to hide the initial browse button layer and
    //  show the main area where all the actual work takes place
    ui->m_initialBrowseContainer->hide();
    m_rootDisplay->show();
}

#include <AssetImporterWindow.moc>
