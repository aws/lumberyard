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
#include <ScriptCanvas/Core/GraphData.h>

namespace ScriptCanvas
{
    class GraphAsset : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(GraphAsset, "{3E2AC8CD-713F-453E-967F-29517F331784}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(GraphAsset, AZ::SystemAllocator, 0);

        GraphAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded);
        ~GraphAsset() override;

        static const char* GetFileExtension() { return "scriptcanvascompiled"; }
        static const char* GetFileFilter() { return "*.scriptcanvascompiled"; }

        const GraphData& GetGraphData() const { return m_graphData; }
        GraphData& GetGraphData() { return m_graphData; }

        void SetGraphData(const GraphData& graphData); //< NOTE: This does not delete the entities stored on the old graph data

    protected:
        friend class GraphAssetHandler;
        GraphAsset(const GraphAsset&) = delete;

        GraphData m_graphData;
    };
}