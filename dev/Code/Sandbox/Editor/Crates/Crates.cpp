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

#include "StdAfx.h"
#include "Crates.h"

#include <QAbstractButton>
#include <QMessageBox>
#include <QDir>
#include <QTemporaryDir>
#include <QDropEvent>
#include <QPushButton>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzQtComponents/DragAndDrop/MainWindowDragAndDrop.h>

#include <AzCore/IO/FileIO.h>

#include <Gems/GemsAPI.h>
#include <Projects/ProjectBus.h>

namespace AZ
{

    static const char * s_traceName = "Crates";

    void CratesHandler::Activate()
    {
        CratesRequestsBus::Handler::BusConnect();
        AzQtComponents::DragAndDropEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EditorMainWindow);
    }

    void CratesHandler::Deactivate()
    {
        CratesRequestsBus::Handler::BusDisconnect();
        AzQtComponents::DragAndDropEventsBus::Handler::BusDisconnect(AzQtComponents::DragAndDropContexts::EditorMainWindow);
    }

    void CratesHandler::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CratesHandler, AZ::Component>()
                ->Version(1);
        }
    }

    void CratesHandler::UnpackCrate(const AZStd::string & cratePath)
    {
        AZ_Printf(s_traceName, "Unpacking %s", cratePath.c_str());

        QWidget* mainWindow = nullptr;
        AzToolsFramework::EditorRequestBus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
        AZ_Assert(mainWindow != nullptr, "Expected a MainWindow to display user dialogs");

        const char* rootPath = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(rootPath, &AZ::ComponentApplicationRequests::GetAppRoot);
        AZStd::string engineRoot(rootPath);

        QString cratesTempPath(engineRoot.c_str());
        cratesTempPath.append("CratesTemp_");

        // This would be unique but we can't std::move to lambdas yet
        AZStd::shared_ptr<QTemporaryDir> cratesTempDir = AZStd::make_shared<QTemporaryDir>(cratesTempPath);
        if (!cratesTempDir || !cratesTempDir->isValid())
        {
            QMessageBox::critical(mainWindow, tr("Could not create temporary folder"), tr("We could not create a temporary folder to extract the Crate to. Please check you have enough disk space and proper file permissions."));
            return;
        }
        QString tempDir = cratesTempDir->path();

        QApplication::setOverrideCursor(Qt::BusyCursor);
        AZStd::string extractPath(tempDir.toUtf8().constData());
        AZ::Uuid handle = AZ::Uuid::Create();

        // This is just so the debugger is happy and will hit breakpoints in this lambda
        auto extractResponseLambda = [=](bool success) {
            AZ_Printf(s_traceName, "Crate extraction finished and %s for %s", success ? "succeeded" : "failed", cratePath.c_str());

            cratesTempDir->setAutoRemove(true); // referring to this var to make lambdas happy
            if (!success)
            {
                QMessageBox::critical(mainWindow, tr("Failed to extract Crate"), tr("We failed to extract the crate. The file may be invalid or corrupt. See the Console for more details.<br/><br/>%1.").arg(cratePath.c_str()));
                QApplication::restoreOverrideCursor();
                return;
            }

            Lyzard::StringOutcome loadOutcome = AZ::Failure<AZStd::string>("");
            Gems::GemsRequestBus::BroadcastResult(loadOutcome, &Gems::GemsRequests::LoadAllGemsFromDisk);
            if (!loadOutcome.IsSuccess())
            {
                QMessageBox::critical(mainWindow, tr("Gem system failure"), tr("Internal error.<br/><br/>%1").arg(loadOutcome.GetError().c_str()));
                QApplication::restoreOverrideCursor();
                return;
            }
            
            AZStd::string gemName;
            AzFramework::StringFunc::Path::GetFileName(cratePath.c_str(), gemName);

            AZStd::string gemAbsPath;
            AzFramework::StringFunc::Path::Join(extractPath.c_str(), gemName.c_str(), gemAbsPath);
            AzFramework::StringFunc::Path::Join(gemAbsPath.c_str(), "gem.json", gemAbsPath);
            AZ::Outcome<Gems::IGemDescriptionConstPtr, AZStd::string> gemDescOutcome = AZ::Failure<AZStd::string>("");
            Gems::GemsRequestBus::BroadcastResult(gemDescOutcome, &Gems::GemsRequests::ParseToGemDescriptionPtr, "", gemAbsPath.c_str());

            if (!gemDescOutcome.IsSuccess())
            {
                // TODO:: If no gem.json is found we might want to leave the temp directory and inform the user to perform the next steps manually
                // https://jira.agscollab.com/browse/LY-50333
                QMessageBox::critical(mainWindow, tr("Error parsing gem.json file"), tr("The Crate file does not contain a valid gem.json file at its root. See the Console for more details. <br/><br/>%1.").arg(cratePath.c_str()));
                QApplication::restoreOverrideCursor();
                return;
            }
            auto gemDescriptionPtr = gemDescOutcome.GetValue();
            Uuid newGemID = gemDescriptionPtr->GetID();
            auto newGemVersion = gemDescriptionPtr->GetVersion();
            bool newGemHasCode = !gemDescriptionPtr->GetModules().empty();

            // This load is to identify old gems before moving the new gem over
            AZStd::vector<Gems::IGemDescriptionConstPtr> oldGemDescriptions;
            Gems::GemsRequestBus::BroadcastResult(oldGemDescriptions, &Gems::GemsRequests::GetAllGemDescriptions);
            AZStd::multimap<AZ::Uuid, Gems::IGemDescriptionConstPtr> existingGems;
            for (auto gem : oldGemDescriptions) {
                existingGems.insert({ gem->GetID(), gem });
            }

            AZ::Outcome<AZStd::string, AZStd::string> outcome = AZ::Failure<AZStd::string>("");
            Projects::ProjectManagerRequestBus::BroadcastResult(outcome, &Projects::ProjectManagerRequests::GetActiveProjectName);
            if (!outcome.IsSuccess())
            {
                QMessageBox::critical(mainWindow, tr("No active project name"), tr("Internal error. We couldn't read the active project's name, which is required to import Gems.<br/><br/>%1").arg(outcome.GetError().c_str()));
                QApplication::restoreOverrideCursor();
                return;
            }
            AZStd::string projectName = outcome.GetValue();
            Projects::ProjectId projectId;
            Projects::ProjectManagerRequestBus::BroadcastResult(projectId, &Projects::ProjectManagerRequests::GetProjectByName, projectName);

            if (projectId.IsNull())
            {
                QMessageBox::critical(mainWindow, tr("No active project name"), tr("Internal error. We couldn't read the active project's name, which is required to import Gems."));
                QApplication::restoreOverrideCursor();
                return;
            }

            NewGemState newGemState = UNKNOWN;
            if (existingGems.count(newGemID) != 0) // GUID exists
            {
                auto existingGemsRange = existingGems.equal_range(newGemID);
                for (auto it = existingGemsRange.first; it != existingGemsRange.second; ++it) 
                {
                    auto existingGemDesc = it->second;
                    if (existingGemDesc->GetID() == newGemID && existingGemDesc->GetVersion() == newGemVersion)
                    {
                        Gems::ProjectGemSpecifier existingGemSpecifier(existingGemDesc->GetID(), existingGemDesc->GetVersion(), existingGemDesc->GetPath());
                        bool result = false;
                        Gems::GemsProjectRequestBus::EventResult(result, projectId, &Gems::GemsProjectRequests::IsGemEnabled, existingGemSpecifier);
                        if (result)
                        {
                            newGemState = NEW_GEM_EXISTS_AND_IS_ENABLED;
                        }
                        else
                        {
                            if (!existingGemDesc->GetModules().empty()) 
                            {
                                newGemState = NEW_CODE_GEM_EXISTS_AND_IS_NOT_ENABLED;
                            }
                            else
                            {
                                newGemState = NEW_GEM_EXISTS_AND_IS_NOT_ENABLED;
                            }
                        }
                        break;
                    }
                }

                if (newGemState == UNKNOWN) 
                {
                    if (newGemHasCode)
                    {
                        newGemState = NEW_CODE_GEM;
                    }
                    else
                    {
                        newGemState = NEW_GEM_VERSION;
                    }
                }
            }
            else // New gem GUID
            {
                if (newGemHasCode)
                {
                    newGemState = NEW_CODE_GEM;
                } 
                else
                {
                    newGemState = NEW_GEM;
                }
            }

            // There's more to do but stop the spinner now before we start showing dialogs
            QApplication::restoreOverrideCursor();

            auto makeMessageBox = [mainWindow](QMessageBox::Icon icon, const QString& title, const QString& text, const QString& informativeText) -> QMessageBox* 
            {
                QMessageBox* box = new QMessageBox(mainWindow);
                box->setIcon(icon);
                box->setTextFormat(Qt::RichText);
                box->setWindowTitle(title);
                box->setText(text);
                box->setInformativeText(informativeText);
                return box;
            };

            AZStd::bitset<IMPORT_FLAGS_SIZE> importFlags;
            switch (newGemState) {
                case NEW_GEM:
                {
                    QMessageBox* box = makeMessageBox(QMessageBox::NoIcon, tr("Extracting .crate file"),
                        tr("The .crate file contents will be extracted and added to Lumberyard as a Gem, and then enabled in your current project. <a href='http://docs.aws.amazon.com/lumberyard/latest/userguide/gems-system-gems.html'>Learn more</a>"),
                        tr("Once the Asset Processor is finished processing the contents, you can access them from the Gems folder in the Asset Browser."));

                    QPushButton* okayButton = box->addButton(tr("Okay"), QMessageBox::AcceptRole);
                    QPushButton* cancelButton = box->addButton(tr("Cancel"), QMessageBox::RejectRole);
                    box->setDefaultButton(okayButton);
                    box->exec();

                    if (box->clickedButton() == cancelButton)
                    {
                        importFlags.set(FLAG_ABORT);
                    }
                    else
                    {
                        importFlags.set(FLAG_IMPORT);
                        importFlags.set(FLAG_ENABLE);
                    }
                    break;
                }
                case NEW_CODE_GEM:
                {
                    QMessageBox* box = makeMessageBox(QMessageBox::Information, tr("Gem requires recompile"),
                        tr("We have unpacked your new Gem, but it contains code that requires you to recompile your project. <a href='http://docs.aws.amazon.com/lumberyard/latest/userguide/gems-system-gems.html'>Learn more</a>"),
                        tr("Please close the Lumberyard editor, enable the Gem in the Project Configurator, and then recompile your project."));

                    QPushButton* quitButton = box->addButton(tr("Quit and Enable"), QMessageBox::ActionRole);
                    QPushButton* laterButton = box->addButton(tr("Do it later"), QMessageBox::RejectRole);
                    box->setDefaultButton(quitButton);
                    box->exec();
                    importFlags.set(FLAG_IMPORT);

                    if (box->clickedButton() == quitButton)
                    {
                        importFlags.set(FLAG_OPEN_PROJECT_CONFIGURATOR);
                    }
                    break;
                }
                case NEW_GEM_VERSION:
                {
                    QMessageBox* box = makeMessageBox(QMessageBox::Information, tr("Updating existing Gem"),
                        tr("The Gem you're importing is a newer version of an existing Gem. If you continue, the old version will be disabled in favor of the new one. <a href='http://docs.aws.amazon.com/lumberyard/latest/userguide/gems-system-gems.html'>Learn more</a>"),
                        tr("Any overrides you've made will be preserved. No files will be overwritten."));

                    QPushButton* updateButton = box->addButton(tr("Update"), QMessageBox::AcceptRole);
                    QPushButton* cancelButton = box->addButton(tr("Cancel"), QMessageBox::RejectRole);
                    box->setDefaultButton(updateButton);
                    box->exec();

                    if (box->clickedButton() == updateButton)
                    {
                        importFlags.set(FLAG_IMPORT);
                        importFlags.set(FLAG_ENABLE);
                    }
                    else 
                    {
                        importFlags.set(FLAG_ABORT);
                    }
                    break;
                }
                case NEW_GEM_EXISTS_AND_IS_ENABLED:
                {
                    QMessageBox* box = makeMessageBox(QMessageBox::NoIcon, tr("Gem already exists"),
                        tr("An exact copy of this Gem already exists. You can access it by browsing to the Gems folder in the Asset Browser. <a href='http://docs.aws.amazon.com/lumberyard/latest/userguide/gems-system-gems.html'>Learn more</a>"),
                        tr(""));

                    QPushButton* closeButton = box->addButton(tr("Close"), QMessageBox::AcceptRole);
                    box->setDefaultButton(closeButton);
                    box->exec();

                    importFlags.set(FLAG_ABORT);
                    break;
                }
                case NEW_GEM_EXISTS_AND_IS_NOT_ENABLED:
                {
                    QMessageBox* box = makeMessageBox(QMessageBox::NoIcon, tr("Gem already exists"),
                        tr("An exact copy of this Gem already exists, but has not been enabled. Would you like to enable it for the current project? <a href='http://docs.aws.amazon.com/lumberyard/latest/userguide/gems-system-gems.html'>Learn more</a>"),
                        tr(""));

                    QPushButton* enableButton = box->addButton(tr("Enable"), QMessageBox::ActionRole);
                    QPushButton* cancelButton = box->addButton(tr("Cancel"), QMessageBox::RejectRole);
                    box->setDefaultButton(enableButton);
                    box->exec();

                    if (box->clickedButton() == enableButton)
                    {
                        importFlags.set(FLAG_ENABLE);
                    }
                    else
                    {
                        importFlags.set(FLAG_ABORT);
                    }
                    break;
                }
                case NEW_CODE_GEM_EXISTS_AND_IS_NOT_ENABLED:
                {
                    QMessageBox* box = makeMessageBox(QMessageBox::NoIcon, tr("Gem already exists"),
                        tr("An exact copy of this Gem already exists, but has not been enabled or compiled. Please close the Lumberyard editor, enable the Gem in the Project Configurator, and then recompile your project. <a href='http://docs.aws.amazon.com/lumberyard/latest/userguide/gems-system-gems.html'>Learn more</a>"),
                        tr(""));

                    QPushButton* enableButton = box->addButton(tr("Quit and Enable"), QMessageBox::ActionRole);
                    QPushButton* cancelButton = box->addButton(tr("Cancel"), QMessageBox::RejectRole);
                    box->setDefaultButton(enableButton);
                    box->exec();

                    if (box->clickedButton() == enableButton)
                    {
                        importFlags.set(FLAG_OPEN_PROJECT_CONFIGURATOR);
                    }
                    else
                    {
                        importFlags.set(FLAG_ABORT);
                    }
                    break;
                }
                case UNKNOWN:
                default:
                    importFlags.set(FLAG_ABORT);
            }

            if (importFlags.test(FLAG_ABORT))
            {
                if (importFlags.test(FLAG_OPEN_ASSET_BROWSER))
                {
                    // TODO: We'd like a way to go directly to the gem and not just the window
                    QtViewPaneManager::instance()->OpenPane(LyViewPane::AssetBrowser);
                }
                return;
            }

            AZStd::string realGemName = gemDescriptionPtr->GetName();
            AZStd::string gemVersion = gemDescriptionPtr->GetVersion().ToString();
            AzFramework::StringFunc::Replace(gemVersion, ".", "_");
            AZStd::string finalGemName = realGemName + "-" + gemVersion;

            if (importFlags.test(FLAG_IMPORT))
            {
                AZStd::string sourceDir = extractPath;
                AzFramework::StringFunc::Path::Join(sourceDir.c_str(), gemName.c_str(), sourceDir);

                AZStd::string targetPath;
                AzFramework::StringFunc::Path::Join(rootPath, "Gems", targetPath);
                AzFramework::StringFunc::Path::Join(targetPath.c_str(), finalGemName.c_str(), targetPath);

                AZ_Printf(s_traceName, "Importing Crate %s as %s", cratePath.c_str(), finalGemName.c_str());

                QDir dir;
                dir.rename(sourceDir.c_str(), targetPath.c_str());
            }

            if (importFlags.test(FLAG_OPEN_PROJECT_CONFIGURATOR))
            {
                QTimer::singleShot(0, mainWindow, []() {
                    CCryEditApp::instance()->OnOpenProjectConfiguratorGems();
                });
            }

            if (importFlags.test(FLAG_OPEN_ASSET_PROCESSOR))
            {
                AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::ShowAssetProcessor);
            }

            if (!importFlags.test(FLAG_ENABLE))
            {
                return;
            }

            AZ_Printf(s_traceName, "Enabling Gem %s", finalGemName.c_str());

            QString enableGemErrorString(tr("Internal error. We could not enable %1 automatically.").arg(finalGemName.c_str()));

            Gems::GemsRequestBus::BroadcastResult(loadOutcome, &Gems::GemsRequests::LoadAllGemsFromDisk);

            if (!loadOutcome.IsSuccess())
            {
                QMessageBox::critical(mainWindow, tr("Gem system failure"), enableGemErrorString + tr("<br/><br/>%1").arg(loadOutcome.GetError().c_str()));
                return;
            }

            Gems::IGemDescriptionConstPtr importedGemDescription;
            Gems::GemSpecifier importedGemSpecifier(newGemID, newGemVersion);
            Gems::GemsRequestBus::BroadcastResult(importedGemDescription, &Gems::GemsRequests::GetGemDescription, importedGemSpecifier);

            if (!importedGemDescription) 
            {
                QMessageBox::critical(mainWindow, tr("Failed to find Gem"), enableGemErrorString + tr("We could not find %1 in the Gems folder.").arg(finalGemName.c_str()));
                return;
            }

            AZStd::string gemPath;
            AzFramework::StringFunc::Path::Join("Gems", importedGemDescription->GetPath().c_str(), gemPath);
            Gems::ProjectGemSpecifier gemSpecifier(importedGemDescription->GetID(), importedGemDescription->GetVersion(), gemPath);
            bool result = false;
            Gems::GemsProjectRequestBus::EventResult(result, projectId, &Gems::GemsProjectRequests::EnableGem, gemSpecifier);
            if (!result)
            {
                QMessageBox::critical(mainWindow, tr("Failed to activate Gem"), enableGemErrorString);
                return;
            }

            Gems::GemsProjectRequestBus::Event(projectId, &Gems::GemsProjectRequests::Save, [mainWindow, projectName, enableGemErrorString](Lyzard::StringOutcome result) {
                if (!result.IsSuccess())
                {
                    QMessageBox::critical(mainWindow, tr("Failed to save changes to project"), enableGemErrorString);
                }
            });
            AZ_Printf(s_traceName, "Enable successful for Gem %s", finalGemName.c_str());
        };
        AzToolsFramework::ArchiveCommands::Bus::Broadcast(&AzToolsFramework::ArchiveCommands::ExtractArchive, cratePath, extractPath, handle, extractResponseLambda);
    }

    void CratesHandler::DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& /*context*/)
    {
        // Look into SetRouterProcessingState
        if (!event->mimeData()->hasUrls())
        {
            return;
        }
        bool shouldAccept = true;
        foreach(const QUrl& url, event->mimeData()->urls())
        {
            if (!url.isLocalFile())
            {
                return;
            }
            QString filename = url.toLocalFile();
            QFileInfo fileInfo(filename);
            if (QStringLiteral("crate").compare(fileInfo.suffix()) != 0)
            {
                shouldAccept = false;
                break;
            }
        }

        if (shouldAccept && event->possibleActions().testFlag(Qt::DropAction::CopyAction))
        {
            AZ_Assert(!event->isAccepted(), "CratesHandler should be the only DragAndDropEvents::Handler for .crate extensions");
            event->setDropAction(Qt::DropAction::CopyAction);
            event->accept();
        }
    }
    void CratesHandler::Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& /*context*/)
    {
        const QMimeData* data = event->mimeData();
        foreach(const QUrl& url, data->urls())
        {
            QString filename = url.toLocalFile();
            QFileInfo fileInfo(filename);

            if (QStringLiteral("crate").compare(fileInfo.suffix()) == 0)
            {
                event->accept();
                AZStd::string cratePath(fileInfo.absoluteFilePath().toUtf8().constData());
                UnpackCrate(cratePath);
            }
        }

    }

} // namespace AZ
