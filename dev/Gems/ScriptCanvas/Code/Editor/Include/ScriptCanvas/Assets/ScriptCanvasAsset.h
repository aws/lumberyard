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

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class Entity;
}
namespace ScriptCanvas
{
    class Graph;
}

namespace ScriptCanvasEditor
{
    struct ScriptCanvasData
    {
        AZ_TYPE_INFO(ScriptCanvasData, "{1072E894-0C67-4091-8B64-F7DB324AD13C}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasData, AZ::SystemAllocator, 0);
        ScriptCanvasData();
        ~ScriptCanvasData();
        ScriptCanvasData(ScriptCanvasData&& other);
        ScriptCanvasData& operator=(ScriptCanvasData&& other);

        static void Reflect(AZ::ReflectContext* reflectContext);

        AZ::Entity* GetScriptCanvasEntity() const { return m_scriptCanvasEntity.get(); }

        AZStd::unique_ptr<AZ::Entity> m_scriptCanvasEntity;
    private:
        ScriptCanvasData(const ScriptCanvasData&) = delete;
    };

    /**
    * Inherits from GraphCanvas SceneSliceAsset class so that it can be associated with new type uuid which is used by the asset processor to detect changes
    */
    class ScriptCanvasAsset
        : public AZ::Data::AssetData
    {
        // The Document Context is friended to allow it to set the AssetStauts state to Ready,
        // as it is responsible for creating new Script Canvas assets in the SC editor
        friend class DocumentContext; 
    public:
        AZ_RTTI(ScriptCanvasAsset, "{FA10C3DA-0717-4B72-8944-CD67D13DFA2B}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ScriptCanvasAsset, AZ::SystemAllocator, 0);

        ScriptCanvasAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom()),
            AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {
        }
        ~ScriptCanvasAsset() override = default;
      
        static const char* GetFileExtension() { return "scriptcanvas"; }
        static const char* GetFileFilter() { return "*.scriptcanvas"; }
        static const char* GetGroup() { return "ScriptCanvas"; }

        AZStd::string GetPath();
        void SetPath(const AZStd::string& path);
        
        AZ::Entity* GetScriptCanvasEntity() const;
        void SetScriptCanvasEntity(AZ::Entity* scriptCanvasEntity);

        ScriptCanvasData& GetScriptCanvasData();
        const ScriptCanvasData& GetScriptCanvasData() const;

    private:
        ScriptCanvasData m_data;
    };
} // namespace ScriptCanvasEditor