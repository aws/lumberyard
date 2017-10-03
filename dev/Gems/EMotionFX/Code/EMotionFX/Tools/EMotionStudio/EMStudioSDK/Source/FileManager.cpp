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

#include "FileManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/FileSystem.h>
#include <QFileDialog>
#include <QString>
#include <QTranslator>
#include <QMessageBox>
#include <QDateTime>
#include "EMStudioManager.h"
#include "MainWindow.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <Source/Integration/Assets/ActorAsset.h>
#include <Source/Integration/Assets/MotionAsset.h>
#include <Source/Integration/Assets/MotionSetAsset.h>
#include <Source/Integration/Assets/AnimGraphAsset.h>

#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>


namespace EMStudio
{
    FileManager::FileManager(QWidget* parent)
        : QObject(parent)
    {
        mLastActorFolder        = EMotionFX::GetEMotionFX().GetAssetSourceFolder().c_str();
        mLastMotionSetFolder    = EMotionFX::GetEMotionFX().GetAssetSourceFolder().c_str();
        mLastAnimGraphFolder    = EMotionFX::GetEMotionFX().GetAssetSourceFolder().c_str();
        mLastWorkspaceFolder    = EMotionFX::GetEMotionFX().GetAssetSourceFolder().c_str();
        mLastNodeMapFolder      = EMotionFX::GetEMotionFX().GetAssetSourceFolder().c_str();

        // Connect to the asset catalog bus for product asset changes.
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();

        // Connect to the asset system bus for source asset changes.
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();
    }


    FileManager::~FileManager()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
    }


    AZStd::string FileManager::GetAssetFilenameFromAssetId(const AZ::Data::AssetId& assetId)
    {
        AZStd::string filename;

        AZStd::string assetCachePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@assets@");
        AzFramework::StringFunc::AssetDatabasePath::Normalize(assetCachePath);

        AZStd::string relativePath;
        EBUS_EVENT_RESULT(relativePath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);
        AzFramework::StringFunc::AssetDatabasePath::Join(assetCachePath.c_str(), relativePath.c_str(), filename, true);
        
        return filename;
    }


    bool FileManager::IsAssetLoaded(const char* filename)
    {
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filename, extension, false);

        if (AzFramework::StringFunc::Equal(extension.c_str(), "motion"))
        {
            const AZ::u32 motionCount = EMotionFX::GetMotionManager().GetNumMotions();
            for (AZ::u32 i = 0; i < motionCount; ++i)
            {
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);
                if (motion->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // Special case handling for motions that are part of a motion set.
                // Note: The command system is unable to remove motions that are part of a motion set before all motion entries are removed that refer to the given motion
                // Until this problem is solved, we'll need to keep this special case. We will not reload the motion in case it is part of a motion set.
                bool usedByMotionSet = false;
                const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
                for (uint32 i = 0; i < numMotionSets; ++i)
                {
                    EMotionFX::MotionSet*               motionSet   = EMotionFX::GetMotionManager().GetMotionSet(i);
                    EMotionFX::MotionSet::MotionEntry*  motionEntry = motionSet->FindMotionEntry(motion);
                    if (motionEntry)
                    {
                        usedByMotionSet = true;
                        break;
                    }
                }
                if (usedByMotionSet)
                {
                    continue;
                }

                if (AzFramework::StringFunc::Equal(filename, motion->GetFileName()))
                {
                    return true;
                }
            }
        }

        if (AzFramework::StringFunc::Equal(extension.c_str(), "actor"))
        {
            const AZ::u32 actorCount = EMotionFX::GetActorManager().GetNumActors();
            for (AZ::u32 i = 0; i < actorCount; ++i)
            {
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);
                if (actor->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (AzFramework::StringFunc::Equal(filename, actor->GetFileName()))
                {
                    return true;
                }
            }
        }

        return false;
    }


    void FileManager::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        const AZStd::string filename = GetAssetFilenameFromAssetId(assetId);

        // Skip re-loading the file, in case it not loaded currently.
        if (!IsAssetLoaded(filename.c_str()))
        {
            return;
        }

        AZ_Printf("", "OnCatalogAssetChanged: assetId='%s' file='%s'", assetId.ToString<AZStd::string>().c_str(), filename.c_str());

        // De-bounce cloned events for the same file.
        // Get the canonical asset id based on the filename to filter out all events for the legacy asset ids.
        // Multiple events get sent for the same file path as multiple asset ids can exist. The canonical asset id will be unique and is the latest version.
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        if (!assetInfo.m_assetId.IsValid() || assetInfo.m_assetId != assetId)
        {
            AZ_Printf("", "   + Skipping file. (Canonical assetId='%s')", assetInfo.m_assetId.ToString<AZStd::string>().c_str());
            return;
        }

        AZ_Printf("", "   + Reloading file. (Canonical assetId='%s')", assetInfo.m_assetId.ToString<AZStd::string>().c_str());

        // Spawn the command for reloading the file.
        GetMainWindow()->LoadFile(filename.c_str(), 0, 0, false, true);

        // Show notification.
        AZStd::string notificationMessage;
        AzFramework::StringFunc::Path::GetFileName(filename.c_str(), notificationMessage);
        notificationMessage += " updated";
        GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, notificationMessage.c_str());
    }


    void FileManager::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        // Do the same as for assets that changed because an asset could be loaded while the asset is temporarily gone.
        // Case: Re-exporting an fbx from Maya
        // 1. The fbx file first gets removed. The asset processor also removes all products then.
        // 2. Maya saves the new fbx. The asset processor generates a new product using the same file path.
        // What we do in this case is, ignoring the remove operation and re-linking the asset when it gets recreated.
        OnCatalogAssetChanged(assetId);
    }


    void FileManager::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId)
    {
    }


    bool FileManager::IsSourceAssetLoaded(const char* filename)
    {
        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filename, extension, false);

        if (AzFramework::StringFunc::Equal(extension.c_str(), "motionset"))
        {
            const AZ::u32 motionSetCount = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (AZ::u32 i = 0; i < motionSetCount; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (AzFramework::StringFunc::Equal(filename, motionSet->GetFilename()))
                {
                    return true;
                }
            }
        }

        if (AzFramework::StringFunc::Equal(extension.c_str(), "animgraph"))
        {
            const AZ::u32 animGraphCount = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
            for (AZ::u32 i = 0; i < animGraphCount; ++i)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
                if (animGraph->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (AzFramework::StringFunc::Equal(filename, animGraph->GetFileName()))
                {
                    return true;
                }
            }
        }

        return false;
    }


    void FileManager::SourceFileChanged(AZStd::string assetId, AZStd::string scanFolder, AZ::Uuid sourceUuid)
    {
        AZStd::string filename;
        AZStd::string assetSourcePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");
        AzFramework::StringFunc::AssetDatabasePath::Normalize(assetSourcePath);
        AzFramework::StringFunc::AssetDatabasePath::Join(assetSourcePath.c_str(), assetId.c_str(), filename, true);

        // Skip re-loading the file, in case it not loaded currently.
        if (!IsSourceAssetLoaded(filename.c_str()))
        {
            return;
        }

        // Spawn the command for reloading the file.
        GetMainWindow()->LoadFile(filename.c_str(), 0, 0, false, true);

        // Show notification.
        AZStd::string notificationMessage;
        AzFramework::StringFunc::Path::GetFileName(filename.c_str(), notificationMessage);
        notificationMessage += " updated";
        GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, notificationMessage.c_str());
    }


    bool FileManager::RelocateToAssetSourceFolder(AZStd::string& filename)
    {
        if (IsFileInAssetCache(filename))
        {
            const AZStd::string& assetCacheFolder = EMotionFX::GetEMotionFX().GetAssetCacheFolder();

            // Get the relative to asset cache filename.
            MCore::String relativeFilename = filename.c_str();
            EMotionFX::GetEMotionFX().GetFilenameRelativeTo(&relativeFilename, assetCacheFolder.c_str());

            bool found;
            EBUS_EVENT_RESULT(found, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, relativeFilename.AsChar(), filename);
            return found;
        }
        return true;
    }


    void FileManager::RelocateToAssetCacheFolder(AZStd::string& filename)
    {
        if (IsFileInAssetSource(filename))
        {
            const AZStd::string& assetSourceFolder = EMotionFX::GetEMotionFX().GetAssetSourceFolder();
            const AZStd::string& assetCacheFolder = EMotionFX::GetEMotionFX().GetAssetCacheFolder();

            // Get the relative to asset cache filename.
            MCore::String relativeFilename = filename.c_str();
            EMotionFX::GetEMotionFX().GetFilenameRelativeTo(&relativeFilename, assetSourceFolder.c_str());

            // Auto-relocate to the asset source folder.
            filename = assetCacheFolder + relativeFilename.AsChar();
        }
    }


    bool FileManager::IsFileInAssetCache(const AZStd::string& filename) const
    {
        AZStd::string folderPath;
        AzFramework::StringFunc::Path::GetFullPath(filename.c_str(), folderPath);

        AZStd::string assetCacheFolder = EMotionFX::GetEMotionFX().GetAssetCacheFolder();
        
        // TODO: Using the incorrect path normalization here, so that the slashes get converted to the wrong ones. Why? Because elsewise IsASubFolderOfB() doesn't work.
        AzFramework::StringFunc::Path::Normalize(folderPath);
        AzFramework::StringFunc::Path::Normalize(assetCacheFolder);

        // TODO: Use the case sensitive version here. Can't do that yet as the folder path returned for an alias returns wrong capitalization while the one from the file dialog from Qt is correct. But as they differ, the function can't detect subfolders correctly.
        if (AzFramework::StringFunc::Equal(folderPath.c_str(), assetCacheFolder.c_str(), false) || AzFramework::StringFunc::Path::IsASubFolderOfB(folderPath.c_str(), assetCacheFolder.c_str()))
        {
            return true;
        }

        return false;
    }


    bool FileManager::IsFileInAssetSource(const AZStd::string& filename) const
    {
        AZStd::string folderPath;
        AzFramework::StringFunc::Path::GetFullPath(filename.c_str(), folderPath);

        AZStd::string assetSourceFolder = EMotionFX::GetEMotionFX().GetAssetSourceFolder();

        // TODO: Using the incorrect path normalization here, so that the slashes get converted to the wrong ones. Why? Because elsewise IsASubFolderOfB() doesn't work.
        AzFramework::StringFunc::Path::Normalize(folderPath);
        AzFramework::StringFunc::Path::Normalize(assetSourceFolder);

        // TODO: Use the case sensitive version here. Can't do that yet as the folder path returned for an alias returns wrong capitalization while the one from the file dialog from Qt is correct. But as they differ, the function can't detect subfolders correctly.
        if (AzFramework::StringFunc::Equal(folderPath.c_str(), assetSourceFolder.c_str(), false) || AzFramework::StringFunc::Path::IsASubFolderOfB(folderPath.c_str(), assetSourceFolder.c_str()))
        {
            return true;
        }

        return false;
    }


    void FileManager::UpdateLastUsedFolder(const char* filename, QString& outLastFolder) const
    {
        // Update the default location for the load and save dialogs.
        AZStd::string folderPath;
        AzFramework::StringFunc::Path::GetFullPath(filename, folderPath);
        outLastFolder = folderPath.c_str();
    }


    QString FileManager::GetLastUsedFolder(const QString& lastUsedFolder) const
    {
        // In case we didn't open up the file dialog yet, default to the asset source folder.
        if (lastUsedFolder.isEmpty())
        {
            const AZStd::string& assetSourceFolder = EMotionFX::GetEMotionFX().GetAssetCacheFolder();
            if (!assetSourceFolder.empty())
            {
                return assetSourceFolder.c_str();
            }
        }

        return lastUsedFolder;
    }


    AZStd::vector<AZStd::string> FileManager::SelectProductsOfType(AZ::Data::AssetType assetType, bool multiSelect) const
    {
        AZStd::vector<AZStd::string> filenames;
        AzToolsFramework::AssetBrowser::AssetSelectionModel selection = AzToolsFramework::AssetBrowser::AssetSelectionModel::AssetTypeSelection(assetType);
        selection.SetMultiselect(multiSelect);

        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
        if (selection.IsValid())
        {
            AZStd::string filename;
            const AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& products = selection.GetResults();
            for (const AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetBrowserEntry : products)
            {
                const ProductAssetBrowserEntry* product = azrtti_cast<const ProductAssetBrowserEntry*>(assetBrowserEntry);

                filename.clear();
                AZStd::string cachePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@assets@");
                AzFramework::StringFunc::AssetDatabasePath::Normalize(cachePath);
                AzFramework::StringFunc::AssetDatabasePath::Join(cachePath.c_str(), product->GetRelativePath().c_str(), filename, true);

                filenames.push_back(filename.c_str());
            }
        }

        return filenames;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZStd::string FileManager::LoadActorFileDialog(QWidget* parent)
    {
        AZStd::vector<AZStd::string> filenames = SelectProductsOfType(AZ::AzTypeInfo<EMotionFX::Integration::ActorAsset>::Uuid(), false);
        if (filenames.empty())
        {
            return AZStd::string();
        }
     
        return filenames[0];
    }


    AZStd::vector<AZStd::string> FileManager::LoadActorsFileDialog(QWidget* parent)
    {
        return SelectProductsOfType(AZ::AzTypeInfo<EMotionFX::Integration::ActorAsset>::Uuid(), true);
    }


    AZStd::string FileManager::SaveActorFileDialog(QWidget* parent)
    {
        GetMainWindow()->StopRendering();

        QFileDialog::Options options;
        QString selectedFilter;
        const AZStd::string filename = QFileDialog::getSaveFileName(parent,                                 // parent
                                                                    "Save",                                 // caption
                                                                    GetLastUsedFolder(mLastActorFolder),    // directory
                                                                    "EMotion FX Actor Files (*.actor)",
                                                                    &selectedFilter,
                                                                    options).toUtf8().data();

        GetMainWindow()->StartRendering();

        UpdateLastUsedFolder(filename.c_str(), mLastActorFolder);

        return filename;
    }


    void FileManager::SaveActor(EMotionFX::Actor* actor)
    {
        AZStd::string command = AZStd::string::format("SaveActorAssetInfo -actorID %i", actor->GetID());

        AZStd::string result;
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Actor <font color=green>successfully</font> saved");
        }
        else
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Actor <font color=red>failed</font> to save");
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZStd::string FileManager::LoadWorkspaceFileDialog(QWidget* parent)
    {
        QFileDialog::Options options;
        QString selectedFilter;
        AZStd::string filename = QFileDialog::getOpenFileName(parent,                                                   // parent
                                                              "Open",                                                   // caption
                                                              GetLastUsedFolder(mLastWorkspaceFolder),                  // directory
                                                              "EMotionFX Editor Workspace Files (*.emfxworkspace);;All Files (*)",
                                                              &selectedFilter,
                                                              options).toUtf8().data();

        UpdateLastUsedFolder(filename.c_str(), mLastWorkspaceFolder);
        return filename;
    }


    AZStd::string FileManager::SaveWorkspaceFileDialog(QWidget* parent)
    {
        GetMainWindow()->StopRendering();

        QFileDialog::Options options;
        QString selectedFilter;
        AZStd::string filename = QFileDialog::getSaveFileName(parent,                                       // parent
                                                              "Save",                                       // caption
                                                              GetLastUsedFolder(mLastWorkspaceFolder),      // directory
                                                              "EMotionFX Editor Workspace Files (*.emfxworkspace)",
                                                              &selectedFilter,
                                                              options).toUtf8().data();

        GetMainWindow()->StartRendering();

        if (IsFileInAssetCache(filename))
        {
            QMessageBox::critical(GetMainWindow(), "Error", "Saving workspace in the asset cache folder is not allowed. Please select a different location.", QMessageBox::Ok);
            return AZStd::string();
        }

        UpdateLastUsedFolder(filename.c_str(), mLastWorkspaceFolder);
        return filename;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void FileManager::SaveMotion(EMotionFX::Motion* motion)
    {
        AZStd::string command = AZStd::string::format("SaveMotionAssetInfo -motionID %i", motion->GetID());

        AZStd::string result;
        if (EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Motion <font color=green>successfully</font> saved");
        }
        else
        {
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Motion <font color=red>failed</font> to save");
        }
    }


    AZStd::string FileManager::LoadMotionFileDialog(QWidget* parent)
    {
        AZStd::vector<AZStd::string> filenames = SelectProductsOfType(AZ::AzTypeInfo<EMotionFX::Integration::MotionAsset>::Uuid(), false);
        if (filenames.empty())
        {
            return AZStd::string();
        }

        return filenames[0];
    }


    AZStd::vector<AZStd::string> FileManager::LoadMotionsFileDialog(QWidget* parent)
    {
        return SelectProductsOfType(AZ::AzTypeInfo<EMotionFX::Integration::MotionAsset>::Uuid(), true);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZStd::string FileManager::LoadMotionSetFileDialog(QWidget* parent)
    {
        AZStd::vector<AZStd::string> filenames = SelectProductsOfType(AZ::AzTypeInfo<EMotionFX::Integration::MotionSetAsset>::Uuid(), false);
        if (filenames.empty())
        {
            return AZStd::string();
        }

        UpdateLastUsedFolder(filenames[0].c_str(), mLastMotionSetFolder);
        return filenames[0];
    }


    AZStd::string FileManager::SaveMotionSetFileDialog(QWidget* parent)
    {
        GetMainWindow()->StopRendering();

        QFileDialog::Options options;
        QString selectedFilter;
        AZStd::string filename = QFileDialog::getSaveFileName(parent,                                       // parent
                                                              "Save",                                       // caption
                                                              GetLastUsedFolder(mLastMotionSetFolder),      // directory
                                                              "EMotion FX Motion Set Files (*.motionset)",
                                                              &selectedFilter,
                                                              options).toUtf8().data();

        GetMainWindow()->StartRendering();

        UpdateLastUsedFolder(filename.c_str(), mLastMotionSetFolder);

        return filename;
    }


    void FileManager::SaveMotionSet(const char* filename, EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup)
    {
        AZStd::string command = AZStd::string::format("SaveMotionSet -motionSetID %i -filename \"%s\"", motionSet->GetID(), filename);

        if (commandGroup == nullptr)
        {
            AZStd::string result;
            if (GetCommandManager()->ExecuteCommand(command, result) == false)
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "MotionSet <font color=red>failed</font> to save");
            }
            else
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "MotionSet <font color=green>successfully</font> saved");
            }
        }
        else
        {
            commandGroup->AddCommandString(command);
        }
    }


    void FileManager::SaveMotionSet(QWidget* parent, EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup)
    {
        AZStd::string filename = motionSet->GetFilename();

        if (filename.empty())
        {
            filename = SaveMotionSetFileDialog(parent);
            if (filename.empty())
            {
                // In case we canceled the file save dialog the filename will be empty, so we'll also cancel the save operation.
                return;
            }
        }

        SaveMotionSet(filename.c_str(), motionSet, commandGroup);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    AZStd::string FileManager::LoadAnimGraphFileDialog(QWidget* parent)
    {
        AZStd::vector<AZStd::string> filenames = SelectProductsOfType(AZ::AzTypeInfo<EMotionFX::Integration::AnimGraphAsset>::Uuid(), false);
        if (filenames.empty())
        {
            return AZStd::string();
        }
        
        UpdateLastUsedFolder(filenames[0].c_str(), mLastAnimGraphFolder);
        return filenames[0];
    }


    AZStd::string FileManager::SaveAnimGraphFileDialog(QWidget* parent)
    {
        GetMainWindow()->StopRendering();

        QFileDialog::Options options;
        QString selectedFilter;
        AZStd::string filename = QFileDialog::getSaveFileName(parent,                                                   // parent
                                                              "Save",                                                   // caption
                                                              GetLastUsedFolder(mLastAnimGraphFolder),                 // directory
                                                              "EMotion FX Anim Graph Files (*.animgraph);;All Files (*)",
                                                              &selectedFilter,
                                                              options).toUtf8().data();

        GetMainWindow()->StartRendering();

        UpdateLastUsedFolder(filename.c_str(), mLastAnimGraphFolder);

        return filename;
    }


    // load a node map file
    AZStd::string FileManager::LoadNodeMapFileDialog(QWidget* parent)
    {
        GetMainWindow()->StopRendering();

        QFileDialog::Options options;
        QString selectedFilter;
        const AZStd::string filename = QFileDialog::getOpenFileName(parent,                                         // parent
                                                                    "Open",                                         // caption
                                                                    GetLastUsedFolder(mLastNodeMapFolder),          // directory
                                                                    "Node Map Files (*.nodeMap);;All Files (*)",
                                                                    &selectedFilter,
                                                                    options).toUtf8().data();

        GetMainWindow()->StartRendering();

        UpdateLastUsedFolder(filename.c_str(), mLastNodeMapFolder);
        return filename;
    }


    // save a node map file
    AZStd::string FileManager::SaveNodeMapFileDialog(QWidget* parent)
    {
        GetMainWindow()->StopRendering();

        QFileDialog::Options options;
        QString selectedFilter;
        const AZStd::string filename = QFileDialog::getSaveFileName(parent,                                         // parent
                                                                    "Save",                                         // caption
                                                                    GetLastUsedFolder(mLastNodeMapFolder),          // directory
                                                                    "Node Map Files (*.nodeMap);;All Files (*)",
                                                                    &selectedFilter,
                                                                    options).toUtf8().data();

        GetMainWindow()->StartRendering();

        UpdateLastUsedFolder(filename.c_str(), mLastNodeMapFolder);
        return filename;
    }


    MCore::String FileManager::LoadControllerPresetFileDialog(QWidget* parent, const char* defaultFolder)
    {
        MCore::String dir;
        if (defaultFolder)
        {
            dir = defaultFolder;
        }
        else
        {
            dir = EMotionFX::GetEMotionFX().GetAssetSourceFolder().c_str();
        }

        GetMainWindow()->StopRendering();

        QFileDialog::Options options;
        QString selectedFilter;
        QString filenameString = QFileDialog::getOpenFileName(parent,                                           // parent
                                                              "Load",                                           // caption
                                                              dir.AsChar(),                                     // directory
                                                              "EMotion FX Config Files (*.cfg);;All Files (*)",
                                                              &selectedFilter,
                                                              options);

        GetMainWindow()->StartRendering();

        MCore::String filename;
        FromQtString(filenameString, &filename);

        return filename;
    }


    MCore::String FileManager::SaveControllerPresetFileDialog(QWidget* parent, const char* defaultFolder)
    {
        MCore::String dir;
        if (defaultFolder)
        {
            dir = defaultFolder;
        }
        else
        {
            dir = EMotionFX::GetEMotionFX().GetAssetSourceFolder().c_str();
        }

        GetMainWindow()->StopRendering();

        QFileDialog::Options options;
        QString selectedFilter;
        QString filename = QFileDialog::getSaveFileName(parent,                                                 // parent
                                                        "Save",                                                 // caption
                                                        dir.AsChar(),                                           // directory
                                                        "EMotion FX Blend Config Files (*.cfg);;All Files (*)",
                                                        &selectedFilter,
                                                        options);

        GetMainWindow()->StartRendering();

        return FromQtString(filename);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/FileManager.moc>