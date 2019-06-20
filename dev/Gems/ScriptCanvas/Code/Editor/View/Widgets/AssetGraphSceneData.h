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

#include <AzCore/Math/Uuid.h>
#include <AzCore/EBus/EBus.h>
#include <Core/Core.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptCanvasEditor
{
    struct AssetGraphSceneId
    {
        AZ::Data::AssetId m_assetId;
        AZ::EntityId m_scriptCanvasGraphId;
        AZ::EntityId m_scriptCanvasEntityId;
    };

    struct AssetGraphSceneData
    {
        AZ_CLASS_ALLOCATOR(AssetGraphSceneData, AZ::SystemAllocator, 0);
        AssetGraphSceneData(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset);
        AssetGraphSceneData(const AssetGraphSceneData&) = delete;
        AssetGraphSceneData& operator=(const AssetGraphSceneData&) = delete;
        void Set(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset);

        AssetGraphSceneId m_tupleId;
        AZ::Data::Asset<ScriptCanvasAsset> m_asset; //< Maintains a reference to ScriptCanvasAsset that is open within the editor
                                               //< While a reference is maintained in the MainWindow, the ScriptCanvas Graph will remain open
                                               //< Even if the underlying asset is removed from disk
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_assetToEditorEntityIdMap;
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_editorToAssetEntityIdMap;
    };
    
    /*! AssetGraphSceneMapper
    The AssetGraphSceneMapper is a structure for allowing quick lookups of open ScriptCanvas
    graphs assetIds, GraphCanvas Scene EntityIds, ScriptCanvas Graphs Unique EntityIds and the ScriptCanvasAsset object
    */
    struct AssetGraphSceneMapper
    {
        AZ::Outcome<void, AZStd::string> Add(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset);
        void Remove(const AZ::Data::AssetId& assetId);
        void Clear();

        AssetGraphSceneData* GetByAssetId(const AZ::Data::AssetId& assetId) const;
        AssetGraphSceneData* GetByGraphId(AZ::EntityId graphId) const;
        AssetGraphSceneData* GetBySceneId(AZ::EntityId entityId) const;

        AZStd::unordered_map<AZ::Data::AssetId, AZStd::unique_ptr<AssetGraphSceneData>> m_assetIdToDataMap;
    };

}