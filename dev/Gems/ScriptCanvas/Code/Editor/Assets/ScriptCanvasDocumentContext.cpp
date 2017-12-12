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
    void DocumentContext::Activate()
    {
        DocumentContextRequestBus::Handler::BusConnect();
    }

    void DocumentContext::Deactivate()
    {
        DocumentContextRequestBus::Handler::BusDisconnect();
    }

    AZ::Data::Asset<ScriptCanvasAsset> DocumentContext::CreateScriptCanvasAsset(const AZ::Data::AssetId& assetId)
    {
        if (!assetId.IsValid())
        {
            AZ_Warning("Script Canvas", assetId.IsValid(), "Invalid AssetId supplied to CreateScriptCanvasAsset. An empty asset will be returned");
            return{};
        }

        ScriptCanvasAssetHandler assetHandler;
        AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset = assetHandler.CreateAsset(assetId, ScriptCanvasAssetHandler::GetAssetTypeStatic());
        AZ::Data::AssetManager::Instance().AssignAssetData(scriptCanvasAsset);
        AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        AZ::Data::AssetInfo assetInfo;
        assetInfo.m_assetId = scriptCanvasAsset.GetId();
        assetInfo.m_assetType = ScriptCanvasAssetHandler::GetAssetTypeStatic();
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::RegisterAsset, assetInfo.m_assetId, assetInfo);

        return scriptCanvasAsset;
    }

    void DocumentContext::SaveScriptCanvasAsset(const AZ::Data::AssetInfo& assetInfo, AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset, const DocumentContextRequests::SaveCB& saveCB)
    {
        if (!scriptCanvasAsset.IsReady())
        {
            AZ_Error("Script Canvas", scriptCanvasAsset.IsReady(), "Unable to save Script Canvas Asset with relative path %s and Asset Id %s", assetInfo.m_relativePath.data(), assetInfo.m_assetId.ToString<AZStd::string>().data());
            return;
        }

        AZ::Data::AssetInfo catalogAssetInfo = assetInfo;
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::RegisterAsset, assetInfo.m_assetId, catalogAssetInfo);

        AZ::Data::AssetStreamInfo saveInfo = AZ::Data::AssetManager::Instance().GetSaveStreamInfoForAsset(assetInfo.m_assetId, assetInfo.m_assetType);
        if (saveInfo.IsValid())
        {
            AZ::Data::Asset<ScriptCanvasAsset> saveAsset = new ScriptCanvasAsset;
            AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
            serializeContext->CloneObjectInplace(saveAsset.Get()->GetScriptCanvasData(), &scriptCanvasAsset.Get()->GetScriptCanvasData());
            saveAsset.Get()->SetPath(assetInfo.m_relativePath);

            bool sourceControlActive = false;
            AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlActive, &AzToolsFramework::SourceControlConnectionRequests::IsActive);
            // If Source Control is active then use it to check out the file before saving otherwise query the file info and save only if the file is not read-only
            if (sourceControlActive)
            {
                AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                    saveInfo.m_streamName.c_str(),
                    true,
                    AZStd::bind(&DocumentContext::SaveAssetPostSourceControl, this, AZStd::placeholders::_1, AZStd::placeholders::_2, saveAsset, assetInfo, saveInfo, saveCB)
                );
            }
            else
            {
                AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetFileInfo,
                    saveInfo.m_streamName.c_str(),
                    AZStd::bind(&DocumentContext::SaveAssetPostSourceControl, this, AZStd::placeholders::_1, AZStd::placeholders::_2, saveAsset, assetInfo, saveInfo, saveCB));
            }
        }
    }

    void DocumentContext::SaveAssetPostSourceControl(bool success, const AzToolsFramework::SourceControlFileInfo& fileInfo, AZ::Data::Asset<ScriptCanvasAsset> saveAsset,
        const AZ::Data::AssetInfo& assetInfo, const AZ::Data::AssetStreamInfo& saveInfo, const DocumentContextRequests::SaveCB& saveCB)
    {
        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileInfo.IsLockedByOther())
        {
            AZ_Error("Script Canvas", !fileInfo.IsLockedByOther(), "The file is already exclusively opened by another user: %s", fileInfo.m_filePath.data());
            if(saveCB)
            {
                saveCB(assetInfo.m_assetId, false);
            }
            AZ::Data::AssetBus::QueueEvent(assetInfo.m_assetId, &AZ::Data::AssetBus::Events::OnAssetSaved, saveAsset, false);
            return;
        }
        else if (fileInfo.IsReadOnly() && fileIO->Exists(fileInfo.m_filePath.c_str()))
        {
            AZ_Error("Script Canvas", !fileInfo.IsReadOnly(), "File %s is read-only. It cannot be saved."
                " If this file is in perforce it may not have been checked out by the Source Control API.", fileInfo.m_filePath.data());
            if(saveCB)
            {
                saveCB(assetInfo.m_assetId, false);
            }
            AZ::Data::AssetBus::QueueEvent(assetInfo.m_assetId, &AZ::Data::AssetBus::Events::OnAssetSaved, saveAsset, false);
            return;
        }

        AZ::Data::AssetBus::MultiHandler::BusConnect(assetInfo.m_assetId);
        ScriptCanvasEditor::SystemRequestBus::Broadcast(&ScriptCanvasEditor::SystemRequests::AddAsyncJob, [this, saveAsset, assetInfo, saveInfo, saveCB]()
        {
            AZ::IO::FileIOStream stream(saveInfo.m_streamName.c_str(), saveInfo.m_streamFlags);
            if (stream.IsOpen())
            {
                ScriptCanvasAssetHandler assetHandler;
                bool savedSuccess = assetHandler.SaveAssetData(saveAsset.Get(), &stream);

                if (AZ::TickBus::IsFunctionQueuing())
                {
                    // Push to the main thread.
                    AZ::TickBus::QueueFunction(AZStd::function<void()>([saveCB, assetInfo, savedSuccess]()
                    {
                        if(saveCB)
                        {
                            saveCB(assetInfo.m_assetId, savedSuccess);
                        }
                    }));
                }

                // Queue an OnAssetSaved message to the AssetBus, so that it fires on the Game Thread
                AZ::Data::AssetBus::QueueEvent(assetInfo.m_assetId, &AZ::Data::AssetBus::Events::OnAssetSaved, saveAsset, savedSuccess);
            }

            AZ_TracePrintf("Script Canvas", "Script Canvas successfully saved as Asset \"%s\"", stream.GetFilename());
        });

        ScriptCanvasEditor::Metrics::MetricsEventsBus::Broadcast(&ScriptCanvasEditor::Metrics::MetricsEventRequests::SendEditorMetric, ScriptCanvasEditor::Metrics::Events::Editor::SaveFile, assetInfo.m_assetId);
    }

    AZ::Data::Asset<ScriptCanvasAsset> DocumentContext::LoadScriptCanvasAsset(const char* sourcePath, bool loadBlocking)
    {
        AZStd::string relativePath;
        bool foundPath = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundPath, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, sourcePath, relativePath);
        if (foundPath)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, relativePath.data(), ScriptCanvasAssetHandler::GetAssetTypeStatic(), false);
            if (!assetId.IsValid())
            {
                AZ_Warning("Script Canvas", false, "File with path %s has invalid Asset Id", relativePath.data());
                return {};
            }

            return LoadScriptCanvasAssetById(assetId, loadBlocking);
        }

        return {};
    }

    AZ::Data::Asset<ScriptCanvasAsset> DocumentContext::LoadScriptCanvasAssetById(const AZ::Data::AssetId& assetId, bool loadBlocking)
    {
        if (!assetId.IsValid())
        {
            return {};
        }

        // If it's not loaded, load it in a non-blocking manner
        AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        auto loadingAsset = AZ::Data::AssetManager::Instance().GetAsset(assetId, ScriptCanvasAssetHandler::GetAssetTypeStatic(), true, &AZ::ObjectStream::AssetFilterDefault, loadBlocking);
        
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendMetric, ScriptCanvasEditor::Metrics::Events::Canvas::OpenGraph);

        return loadingAsset;
    }

    void DocumentContext::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        //Only set the File modification state of a Loaded Script Canvas asset to unmodified if it is not already registered
        auto registeredFileInfo = m_scriptCanvasAssetFileInfo.find(asset.GetId());
        if (registeredFileInfo == m_scriptCanvasAssetFileInfo.end())
        {
            ScriptCanvasAssetFileInfo scFileInfo;
            scFileInfo.m_fileModificationState = ScriptCanvasFileState::UNMODIFIED;

            RegisterScriptCanvasAsset(asset.GetId(), scFileInfo);
            SetScriptCanvasAssetModificationState(asset.GetId(), scFileInfo.m_fileModificationState);
        }

        DocumentContextNotificationBus::Event(asset.GetId(), &DocumentContextNotifications::OnScriptCanvasAssetReady, asset);
    }

    void DocumentContext::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        DocumentContextNotificationBus::Event(asset.GetId(), &DocumentContextNotifications::OnScriptCanvasAssetReloaded, asset);
    }

    void DocumentContext::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Error("Script Canvas", asset.IsReady(), "Failed to load graph asset with id \"%s\".", asset.GetId().ToString<AZStd::string>().data());
    }

    void DocumentContext::OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData> asset, bool success)
    {
        const AZ::Data::AssetId* busId = AZ::Data::AssetBus::GetCurrentBusId();
        const AZ::Data::AssetId assetId = busId ? *busId : asset.GetId();

        if (success)
        {
            ScriptCanvasAssetFileInfo scFileInfo;
            scFileInfo.m_fileModificationState = ScriptCanvasFileState::UNMODIFIED;
            RegisterScriptCanvasAsset(assetId, scFileInfo);
            SetScriptCanvasAssetModificationState(assetId, scFileInfo.m_fileModificationState);
        }

        DocumentContextNotificationBus::Event(assetId, &DocumentContextNotifications::OnScriptCanvasAssetSaved, asset, success);
    }

    void DocumentContext::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType)
    {
        UnregisterScriptCanvasAsset(assetId);
        DocumentContextNotificationBus::Event(assetId, &DocumentContextNotifications::OnScriptCanvasAssetUnloaded, assetId);
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
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);
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
}
