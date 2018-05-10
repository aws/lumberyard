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

#include "precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Profiler.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Editor/Metrics.h>

#include <ScriptCanvas/Assets/ScriptCanvasDocumentContext.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/Graph.h>

namespace ScriptCanvasEditor
{
    AZStd::string MakeTemporaryFilePathForSave(AZStd::string_view targetFilename)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "File IO is not initialized.");

        AZStd::string tempFilename;
        AzFramework::StringFunc::Path::GetFullFileName(targetFilename.data(), tempFilename);
        AZStd::string tempPath = AZStd::string::format("@cache@/scriptcanvas/%s.temp", tempFilename.data());

        AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath{};
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(tempPath.data(), resolvedPath.data(), resolvedPath.size());
        return resolvedPath.data();
    }

    void DocumentContext::Activate()
    {
        DocumentContextRequestBus::Handler::BusConnect();
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();
    }

    void DocumentContext::Deactivate()
    {
        DocumentContextRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();
    }

    AZ::Data::Asset<ScriptCanvasAsset> DocumentContext::CreateScriptCanvasAsset(AZStd::string_view assetAbsolutePath)
    {
        ScriptCanvasAssetHandler assetHandler;
        AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset = assetHandler.CreateAsset(AZ::Uuid::CreateRandom(), ScriptCanvasAssetHandler::GetAssetTypeStatic());
        // The DocumentContext is a friend and therefore can set the AssetStatus to Ready after creating a ScriptCanvasAsset
        scriptCanvasAsset.Get()->m_status = static_cast<int>(AZ::Data::AssetData::AssetStatus::Ready);

        ScriptCanvasAssetFileInfo scFileInfo;
        scFileInfo.m_fileModificationState = ScriptCanvasFileState::NEW;
        scFileInfo.m_absolutePath = assetAbsolutePath;
        m_scriptCanvasAssetFileInfo[scriptCanvasAsset.GetId()] = scFileInfo;

        return scriptCanvasAsset;
    }

    void DocumentContext::SaveScriptCanvasAsset(AZStd::string_view assetAbsolutePath, AZ::Data::Asset<ScriptCanvasAsset> saveAsset, const DocumentContextRequests::SaveCB& saveCB, const DocumentContextRequests::SourceFileChangedCB& idChangedCB)
    {
        AZ::Data::AssetStreamInfo streamInfo;
        streamInfo.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
        streamInfo.m_isCustomStreamType = true;
        streamInfo.m_streamName = assetAbsolutePath;
        if (!streamInfo.IsValid())
        {
            return;
        }

        bool sourceControlActive = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlActive, &AzToolsFramework::SourceControlConnectionRequests::IsActive);
        // If Source Control is active then use it to check out the file before saving otherwise query the file info and save only if the file is not read-only
        if (sourceControlActive)
        {
            AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                streamInfo.m_streamName.c_str(),
                true,
                AZStd::bind(&DocumentContext::SaveAssetPostSourceControl, this, AZStd::placeholders::_1, AZStd::placeholders::_2, saveAsset, streamInfo, saveCB, idChangedCB)
            );
        }
        else
        {
            AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetFileInfo,
                streamInfo.m_streamName.c_str(),
                AZStd::bind(&DocumentContext::SaveAssetPostSourceControl, this, AZStd::placeholders::_1, AZStd::placeholders::_2, saveAsset, streamInfo, saveCB, idChangedCB));
        }
    }

    void DocumentContext::SaveAssetPostSourceControl(bool success, const AzToolsFramework::SourceControlFileInfo& fileInfo, AZ::Data::Asset<ScriptCanvasAsset> saveAsset,
        const AZ::Data::AssetStreamInfo& saveInfo, const DocumentContextRequests::SaveCB& saveCB, const DocumentContextRequests::SourceFileChangedCB& idChangedCB)
    {
        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileInfo.IsLockedByOther())
        {
            AZ_Error("Script Canvas", !fileInfo.IsLockedByOther(), "The file is already exclusively opened by another user: %s", fileInfo.m_filePath.data());
            if (saveCB)
            {
                saveCB(false);
            }
            return;
        }
        else if (fileInfo.IsReadOnly() && fileIO->Exists(fileInfo.m_filePath.c_str()))
        {
            AZ_Error("Script Canvas", !fileInfo.IsReadOnly(), "File %s is read-only. It cannot be saved."
                " If this file is in perforce it may not have been checked out by the Source Control API.", fileInfo.m_filePath.data());
            if (saveCB)
            {
                saveCB(false);
            }
            return;
        }

        AZStd::string normPath = saveInfo.m_streamName;
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, normPath);
        m_pendingSaveMap.emplace(normPath, idChangedCB);
        ScriptCanvasEditor::SystemRequestBus::Broadcast(&ScriptCanvasEditor::SystemRequests::AddAsyncJob, [this, saveAsset, saveInfo, saveCB]()
        {
            // ScriptCanvas Asset must be saved to a temporary location as the FileWatcher will pick up the file immediately if it detects any changes
            // and attempt to reload it

            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

            const AZStd::string tempPath = MakeTemporaryFilePathForSave(saveInfo.m_streamName);
            AZ::IO::FileIOStream stream(tempPath.data(), saveInfo.m_streamFlags);
            if (stream.IsOpen())
            {
                ScriptCanvasAssetHandler assetHandler;
                bool savedSuccess;
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvasAssetHandler::SaveAssetData");
                    savedSuccess = assetHandler.SaveAssetData(saveAsset.Get(), &stream);
                }
                stream.Close();
                if (savedSuccess)
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "DocumentContext::SaveAssetPostSourceControl : TempToTargetFileReplacement");

                    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                    const bool targetFileExists = fileIO->Exists(saveInfo.m_streamName.data());

                    bool removedTargetFile;
                    {
                        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "DocumentContext::SaveAssetPostSourceControl : TempToTargetFileReplacement : RemoveTarget");
                        removedTargetFile = fileIO->Remove(saveInfo.m_streamName.data());
                    }

                    if (targetFileExists && !removedTargetFile)
                    {
                        savedSuccess = false;
                    }
                    else
                    {
                        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "DocumentContext::SaveAssetPostSourceControl : TempToTargetFileReplacement : RenameTempFile");
                        AZ::IO::Result renameResult = fileIO->Rename(tempPath.data(), saveInfo.m_streamName.data());
                        if (!renameResult)
                        {
                            savedSuccess = false;
                        }
                    }
                }

                if (AZ::TickBus::IsFunctionQueuing())
                {
                    // Push to the main thread.
                    const AZ::Data::AssetId& savedAssetId = saveAsset.GetId();
                    AZ::TickBus::QueueFunction(AZStd::function<void()>([this, saveCB, savedSuccess, savedAssetId]()
                    {
                        this->SetScriptCanvasAssetModificationState(savedAssetId, ScriptCanvasFileState::UNMODIFIED);
                        if (saveCB)
                        {
                            saveCB(savedSuccess);
                        }
                    }));
                }

                if (savedSuccess)
                {
                    AZ_TracePrintf("Script Canvas", "Script Canvas successfully saved as Asset \"%s\"", saveInfo.m_streamName.data());
                }
            }
        });
    }

    AZ::Data::Asset<ScriptCanvasAsset> DocumentContext::LoadScriptCanvasAsset(const char* sourcePath, bool loadBlocking)
    {
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        bool foundPath = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundPath, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, sourcePath, assetInfo, watchFolder);
        if (foundPath)
        {
            if (!assetInfo.m_assetId.IsValid())
            {
                AZ_Warning("Script Canvas", false, "File with path %s has invalid Asset Id", sourcePath);
                return {};
            }

            return LoadScriptCanvasAssetById(assetInfo.m_assetId, loadBlocking);
        }

        return {};
    }

    AZ::Data::Asset<ScriptCanvasAsset> DocumentContext::LoadScriptCanvasAssetById(const AZ::Data::AssetId& assetId, bool loadBlocking)
    {
        if (!assetId.IsValid())
        {
            return {};
        }

        ScriptCanvasAssetFileInfo scFileInfo;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string rootFilePath;
        AzToolsFramework::AssetSystemRequestBus::Broadcast(&AzToolsFramework::AssetSystemRequestBus::Events::GetAssetInfoById, assetId, azrtti_typeid<ScriptCanvasAsset>(), assetInfo, rootFilePath);
        AzFramework::StringFunc::Path::Join(rootFilePath.data(), assetInfo.m_relativePath.data(), scFileInfo.m_absolutePath);
        m_scriptCanvasAssetFileInfo.emplace(assetId, AZStd::move(scFileInfo));

        AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        auto loadingAsset = AZ::Data::AssetManager::Instance().GetAsset(assetId, azrtti_typeid<ScriptCanvasAsset>(), true, &AZ::ObjectStream::AssetFilterDefault, loadBlocking);

        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendMetric, ScriptCanvasEditor::Metrics::Events::Canvas::OpenGraph);

        return loadingAsset;
    }

    void DocumentContext::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        const AZ::Data::AssetId* busId = AZ::Data::AssetBus::GetCurrentBusId();
        const AZ::Data::AssetId assetId = busId ? *busId : asset.GetId();

        ScriptCanvasAssetFileInfo scFileInfo;
        scFileInfo.m_fileModificationState = ScriptCanvasFileState::UNMODIFIED;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string rootFilePath;
        AzToolsFramework::AssetSystemRequestBus::Broadcast(&AzToolsFramework::AssetSystemRequestBus::Events::GetAssetInfoById, assetId, azrtti_typeid<ScriptCanvasAsset>(), assetInfo, rootFilePath);
        AzFramework::StringFunc::Path::Join(rootFilePath.data(), assetInfo.m_relativePath.data(), scFileInfo.m_absolutePath);
        m_scriptCanvasAssetFileInfo[assetId] = scFileInfo;
        DocumentContextNotificationBus::Event(assetId, &DocumentContextNotifications::OnAssetModificationStateChanged, scFileInfo.m_fileModificationState);

        DocumentContextNotificationBus::Event(asset.GetId(), &DocumentContextNotifications::OnScriptCanvasAssetReady, asset);
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);
    }

    void DocumentContext::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        auto assetFileInfoIt = m_scriptCanvasAssetFileInfo.find(asset.GetId());
        if (assetFileInfoIt != m_scriptCanvasAssetFileInfo.end() && assetFileInfoIt->second.m_reloadable)
        {
            DocumentContextNotificationBus::Event(asset.GetId(), &DocumentContextNotifications::OnScriptCanvasAssetReloaded, asset);
        }
    }

    void DocumentContext::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        auto assetFileInfoIt = m_scriptCanvasAssetFileInfo.find(asset.GetId());
        if (assetFileInfoIt != m_scriptCanvasAssetFileInfo.end())
        {
            AZ_Error("Script Canvas", asset.IsReady(), "Failed to load graph asset %s with AssetId: %s", assetFileInfoIt->second.m_absolutePath.c_str(), asset.ToString<AZStd::string>().c_str());
        }
        else
        {
            // backup strategy - we dont know what the actual absolute path is, so at least provide the asset info alone.
            AZ_Error("Script Canvas", asset.IsReady(), "Failed to load graph asset %s", asset.ToString<AZStd::string>().c_str());
        }
        
        const AZ::Data::AssetId* busId = AZ::Data::AssetBus::GetCurrentBusId();
        const AZ::Data::AssetId assetId = busId ? *busId : asset.GetId();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);
    }

    void DocumentContext::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType)
    {
        DocumentContextNotificationBus::Event(assetId, &DocumentContextNotifications::OnScriptCanvasAssetUnloaded, assetId);
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);
    }

    void DocumentContext::SourceFileChanged(AZStd::string relPath, AZStd::string scanFolder, AZ::Uuid sourceAssetId)
    {
        if (m_pendingSaveMap.empty())
        {
            return;
        }

        // This updates the asset Id registered with the DocumentContext with the canonical assetId on disk
        // This occurs for new ScriptCanvas assets because before the SC asset is saved to disk, the asset database
        // has no asset Id associated with it, so this uses the supplied source path to find the asset Id registered 
        // with the DocumentContext
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(scanFolder.data(), relPath.data(), fullPath);
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::NormalizePath, fullPath);
        auto assetPathIdIt = m_pendingSaveMap.find(fullPath);

        if (assetPathIdIt != m_pendingSaveMap.end())
        {
            const auto& idChangedCB = assetPathIdIt->second;
            idChangedCB(relPath, scanFolder, sourceAssetId);
            m_pendingSaveMap.erase(assetPathIdIt);
        }
    }

    bool DocumentContext::RegisterScriptCanvasAsset(const AZ::Data::AssetId& assetId, const ScriptCanvasAssetFileInfo& assetFileInfo)
    {
        auto registeredFileInfo = m_scriptCanvasAssetFileInfo.find(assetId);
        if (registeredFileInfo == m_scriptCanvasAssetFileInfo.end())
        {
            m_scriptCanvasAssetFileInfo.emplace(assetId, assetFileInfo);
            return true;
        }

        registeredFileInfo->second = assetFileInfo;
        return false;
    }

    bool DocumentContext::UnregisterScriptCanvasAsset(const AZ::Data::AssetId& assetId)
    {
        auto registeredFileInfo = m_scriptCanvasAssetFileInfo.find(assetId);
        if (registeredFileInfo != m_scriptCanvasAssetFileInfo.end())
        {
            m_scriptCanvasAssetFileInfo.erase(registeredFileInfo);
            return true;
        }

        return false;
    }

    ScriptCanvasFileState DocumentContext::GetScriptCanvasAssetModificationState(const AZ::Data::AssetId& assetId)
    {
        auto assetFileInfoIt = m_scriptCanvasAssetFileInfo.find(assetId);
        if (assetFileInfoIt != m_scriptCanvasAssetFileInfo.end())
        {
            return assetFileInfoIt->second.m_fileModificationState;
        }
        return ScriptCanvasFileState::INVALID;
    }

    void DocumentContext::SetScriptCanvasAssetModificationState(const AZ::Data::AssetId& assetId, ScriptCanvasFileState fileState)
    {
        auto assetFileInfoIt = m_scriptCanvasAssetFileInfo.find(assetId);
        if (assetFileInfoIt != m_scriptCanvasAssetFileInfo.end())
        {
            assetFileInfoIt->second.m_fileModificationState = fileState;
            DocumentContextNotificationBus::Event(assetId, &DocumentContextNotifications::OnAssetModificationStateChanged, fileState);
        }
    }

    AZ::Outcome<ScriptCanvasAssetFileInfo, AZStd::string> DocumentContext::GetFileInfo(const AZ::Data::AssetId& assetId) const
    {
        auto assetFileInfoIt = m_scriptCanvasAssetFileInfo.find(assetId);
        if (assetFileInfoIt == m_scriptCanvasAssetFileInfo.end())
        {
            return AZ::Failure(AZStd::string::format("Asset with id %s is not registered with DocumentContext", assetId.ToString<AZStd::string>().data()));
        }

        return AZ::Success(assetFileInfoIt->second);
    }

    AZ::Outcome<void, AZStd::string> DocumentContext::SetFileInfo(const AZ::Data::AssetId& assetId, const ScriptCanvasAssetFileInfo& fileInfo)
    {
        auto assetFileInfoIt = m_scriptCanvasAssetFileInfo.find(assetId);
        if (assetFileInfoIt == m_scriptCanvasAssetFileInfo.end())
        {
            return AZ::Failure(AZStd::string::format("Asset with id %s is not registered with DocumentContext", assetId.ToString<AZStd::string>().data()));
        }

        assetFileInfoIt->second = fileInfo;
        return AZ::Success();
    }
}
