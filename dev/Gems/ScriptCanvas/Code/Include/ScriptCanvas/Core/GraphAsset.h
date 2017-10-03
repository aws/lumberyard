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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Entity/EntityReference.h>

namespace ScriptCanvas
{
    struct ScriptCanvasData
    {
        AZ_TYPE_INFO(ScriptCanvasData, "{9F31DFFC-D8AB-4DB4-9562-43E5325A970E}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasData, AZ::SystemAllocator, 0);
        ScriptCanvasData() = default;

        AzFramework::EntityReference m_graph; ///< The graph
    };

    class GraphAsset : public AZ::Data::AssetData
    {
        friend class GraphAssetHandler;

    public:
        AZ_RTTI(GraphAsset, "{3E2AC8CD-713F-453E-967F-29517F331784}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(GraphAsset, AZ::SystemAllocator, 0);

        GraphAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded);
        ~GraphAsset() = default;

        GraphAsset(GraphAsset&& other);
        GraphAsset& operator=(GraphAsset&& other);

        static void Reflect(AZ::ReflectContext* context);

        static const char* GetFileExtension() { return "scriptcanvascompiled"; }
        static const char* GetFileFilter() { return "*.scriptcanvascompiled"; }

        AZ::Entity* GetGraph() const { return m_scriptCanvasData.m_graph.GetEntity(); }

        void SetGraph(AZ::Entity* graph);

    protected:
        GraphAsset(const GraphAsset&) = delete;
        
        ScriptCanvasData m_scriptCanvasData;
    };
}