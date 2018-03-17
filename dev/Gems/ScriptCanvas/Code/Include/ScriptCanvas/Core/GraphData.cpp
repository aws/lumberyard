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

#include <AzCore/Component/EntityUtils.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/GraphData.h>

namespace AZ
{
    class Entity;
}

namespace ScriptCanvas
{
    class GraphDataEventHandler : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called to rebuild the Endpoint map
        void OnWriteEnd(void* classPtr) override
        {
            auto* graphData = reinterpret_cast<GraphData*>(classPtr);
            graphData->BuildEndpointMap();
        }
    };

    void GraphData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphData>()
                ->Version(1, &GraphData::VersionConverter)
                ->EventHandler<GraphDataEventHandler>()
                ->Field("m_nodes", &GraphData::m_nodes)
                ->Field("m_connections", &GraphData::m_connections)
                ;
        }
    }

    bool GraphData::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() == 0)
        {
            int connectionsIndex = rootElement.FindElement(AZ_CRC("m_connections", 0xdc357426));
            if (connectionsIndex < 0)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& entityElement = rootElement.GetSubElement(connectionsIndex);
            AZStd::unordered_set<AZ::Entity*> entitiesSet;
            if (!entityElement.GetData(entitiesSet))
            {
                return false;
            }

            AZStd::vector<AZ::Entity*> entitiesVector(entitiesSet.begin(), entitiesSet.end());
            rootElement.RemoveElement(connectionsIndex);
            if (rootElement.AddElementWithData(context, "m_connections", entitiesVector) == -1)
            {
                return false;
            }

            for (AZ::Entity* entity : entitiesSet)
            {
                delete entity;
            }
        }

        return true;
    }

    void GraphData::BuildEndpointMap()
    {
        m_endpointMap.clear();
        for (auto& connectionEntity : m_connections)
        {
            auto* connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Connection>(connectionEntity) : nullptr;
            if (connection)
            {
                m_endpointMap.emplace(connection->GetSourceEndpoint(), connection->GetTargetEndpoint());
                m_endpointMap.emplace(connection->GetTargetEndpoint(), connection->GetSourceEndpoint());
            }
        }
    }

    void GraphData::Clear()
    {
        m_endpointMap.clear();
        m_nodes.clear();
        m_connections.clear();
    }
}