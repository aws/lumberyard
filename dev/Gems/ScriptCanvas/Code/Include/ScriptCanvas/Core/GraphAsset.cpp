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

#include "GraphAsset.h"

#include <AzCore/Component/Entity.h>

namespace ScriptCanvas
{
    GraphAsset::GraphAsset(const AZ::Data::AssetId& assetId, AZ::Data::AssetData::AssetStatus status)
        : AZ::Data::AssetData(assetId, status)
    {
    }

    GraphAsset::~GraphAsset()
    {
        for (auto& nodeRef : m_graphData.m_nodes)
        {
            delete nodeRef;
        }

        for (auto& connectionRef : m_graphData.m_connections)
        {
            delete connectionRef;
        }
    }

    void GraphAsset::SetGraphData(const GraphData& graphData)
    {
        m_graphData = graphData;
    }
}
