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
#include "Graph.h"

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace ScriptCanvas
{

    GraphAsset::GraphAsset(const AZ::Data::AssetId& assetId, AZ::Data::AssetData::AssetStatus status)
        : AZ::Data::AssetData(assetId, status)
    {

    }

    GraphAsset::GraphAsset(GraphAsset&& other)
        : m_scriptCanvasData(AZStd::move(other.m_scriptCanvasData))
    {

    }

    void GraphAsset::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptCanvasData>()
                ->Version(0)
                ->Field("m_graph", &ScriptCanvasData::m_graph)
                ;
        }
    }

    void GraphAsset::SetGraph(AZ::Entity* graph)
    {
        if (graph != m_scriptCanvasData.m_graph.GetEntity())
        {
            delete m_scriptCanvasData.m_graph.GetEntity();
        }
        m_scriptCanvasData.m_graph = graph;
    }

    GraphAsset& GraphAsset::operator=(GraphAsset&& other)
    {
        if (this != &other)
        {
            m_scriptCanvasData = AZStd::move(other.m_scriptCanvasData);
        }

        return *this;
    }
}
