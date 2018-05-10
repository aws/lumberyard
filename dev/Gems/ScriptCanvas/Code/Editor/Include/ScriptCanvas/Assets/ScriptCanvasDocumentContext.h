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

#include <ScriptCanvas/Bus/DocumentContextBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AZ
{
    namespace Data
    {
        class AssetStreamInfo;
    }
}

namespace ScriptCanvasEditor
{
    class DocumentContext
        : public DocumentContextRequestBus::Handler
        , protected AZ::Data::AssetBus::MultiHandler
        , protected AzToolsFramework::AssetSystemBus::Handler
    {
    public:
        AZ_TYPE_INFO(DocumentContext, "{50AE481D-9576-40E3-8084-1FCCDACCC09A}");
        AZ_CLASS_ALLOCATOR(DocumentContext, AZ::SystemAllocator, 0);

        DocumentContext() = default;
        ~DocumentContext() = default;

        void Activate();
        void Deactivate();

        ////////////////////////////////////////////////////////////////////////
        // DocumentRequestBus::Handler
        AZ::Data::Asset<ScriptCanvasAsset> CreateScriptCanvasAsset( AZStd::string_view assetAbsolutePath) override;

        void SaveScriptCanvasAsset(AZStd::string_view, AZ::Data::Asset<ScriptCanvasAsset>, const DocumentContextRequests::SaveCB& saveCB, const DocumentContextRequests::SourceFileChangedCB&) override;
        AZ::Data::Asset<ScriptCanvasAsset> LoadScriptCanvasAsset(const char* assetPath, bool loadBlocking) override;
        AZ::Data::Asset<ScriptCanvasAsset> LoadScriptCanvasAssetById(const AZ::Data::AssetId& assetId, bool loadBlocking) override;
        bool RegisterScriptCanvasAsset(const AZ::Data::AssetId& assetId, const ScriptCanvasAssetFileInfo& assetFileInfo) override;
        bool UnregisterScriptCanvasAsset(const AZ::Data::AssetId& assetId) override;
        ScriptCanvasFileState GetScriptCanvasAssetModificationState(const AZ::Data::AssetId& assetId) override;
        void SetScriptCanvasAssetModificationState(const AZ::Data::AssetId& assetId, ScriptCanvasFileState fileState) override;

        AZ::Outcome<ScriptCanvasAssetFileInfo, AZStd::string> GetFileInfo(const AZ::Data::AssetId& assetId) const override;
        AZ::Outcome<void, AZStd::string> SetFileInfo(const AZ::Data::AssetId& assetId, const ScriptCanvasAssetFileInfo& fileInfo) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        //SourceControl callback
        void SaveAssetPostSourceControl(bool success, const AzToolsFramework::SourceControlFileInfo& fileInfo, AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset,
            const AZ::Data::AssetStreamInfo& saveInfo, const DocumentContextRequests::SaveCB& saveCB, const DocumentContextRequests::SourceFileChangedCB& idChangedCB);

        //! AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;

        // AzToolsFramework::AssetSystemBus
        void SourceFileChanged(AZStd::string relPath, AZStd::string scanFolder, AZ::Uuid sourceAssetId);

        AZStd::unordered_map<AZ::Data::AssetId, ScriptCanvasAssetFileInfo> m_scriptCanvasAssetFileInfo;
        AZStd::unordered_map<AZStd::string, DocumentContextRequests::SourceFileChangedCB> m_pendingSaveMap;
    };
}
