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

#include <ScriptCanvasDeveloper/ScriptCanvasDeveloperComponent.h>

#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvasDeveloper
{
    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasDeveloperService", 0xd2edba67));
    }


    void SystemComponent::Init()
    {
    }

    void SystemComponent::Activate()
    {
        ScriptCanvas::StatusRequestBus::Handler::BusConnect();
    }

    void SystemComponent::Deactivate()
    {
        ScriptCanvas::StatusRequestBus::Handler::BusDisconnect();
    }

    AZ::Outcome<void, ScriptCanvas::StatusErrors> SystemComponent::ValidateGraph(const ScriptCanvas::Graph& graph)
    {
        ScriptCanvas::StatusErrors statusErrors;
        const ScriptCanvas::GraphData& graphData = *graph.GetGraphDataConst();

        // Validate all connections and add invalid connections to the StatusError list of errors
        for (auto& connectionEntity : graphData.m_connections)
        {
            if (auto connection = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Connection>(connectionEntity))
            {
                const auto& sourceEndpoint = connection->GetSourceEndpoint();
                auto nodeIt = AZStd::find_if(graphData.m_nodes.begin(), graphData.m_nodes.end(), [sourceEndpoint](const AZ::Entity* entity) { return entity->GetId() == sourceEndpoint.GetNodeId(); });
                if (nodeIt == graphData.m_nodes.end())
                {
                    statusErrors.m_graphErrors.emplace_back(AZStd::string::format("Script Canvas connection (%s)%s has invalid source endpoint node(nodeId=%s)",
                        connectionEntity->GetName().data(), connectionEntity->GetId().ToString().data(), sourceEndpoint.GetNodeId().ToString().data()));
                    statusErrors.m_invalidConnectionIds.emplace_back(connectionEntity->GetId());
                }
                else
                {
                    auto sourceNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(*nodeIt);
                    AZStd::string sourceNodeName = sourceNode->GetNodeName();
                    auto sourceSlot = sourceNode->GetSlot(sourceEndpoint.GetSlotId());
                    if (!sourceSlot)
                    {
                        statusErrors.m_graphErrors.emplace_back(AZStd::string::format("Script Canvas connection (%s)%s has invalid source endpoint slot(node=%s, nodeId=%s, slotId=%s)",
                            connectionEntity->GetName().data(), connectionEntity->GetId().ToString().data(), sourceNodeName.data(), sourceEndpoint.GetNodeId().ToString().data(), sourceEndpoint.GetSlotId().m_id.ToString<AZStd::string>().data()));
                        statusErrors.m_invalidConnectionIds.emplace_back(connectionEntity->GetId());
                    }
                }

                const auto& targetEndpoint = connection->GetTargetEndpoint();
                nodeIt = AZStd::find_if(graphData.m_nodes.begin(), graphData.m_nodes.end(), [targetEndpoint](const AZ::Entity* entity) { return entity->GetId() == targetEndpoint.GetNodeId(); });
                if (nodeIt == graphData.m_nodes.end())
                {
                    statusErrors.m_graphErrors.emplace_back(AZStd::string::format("Script Canvas connection (%s)%s has invalid target endpoint node(nodeId=%s)",
                        connectionEntity->GetName().data(), connectionEntity->GetId().ToString().data(), targetEndpoint.GetNodeId().ToString().data()));
                    statusErrors.m_invalidConnectionIds.emplace_back(connectionEntity->GetId());
                }
                else
                {
                    auto targetNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(*nodeIt);
                    AZStd::string targetNodeName = targetNode->GetNodeName();
                    auto targetSlot = targetNode->GetSlot(targetEndpoint.GetSlotId());
                    if (!targetSlot)
                    {
                        statusErrors.m_graphErrors.emplace_back(AZStd::string::format("Script Canvas connection (%s)%s has invalid target endpoint slot(node=%s, nodeId=%s, slotId=%s)",
                            connectionEntity->GetName().data(), connectionEntity->GetId().ToString().data(), targetNodeName.data(), targetEndpoint.GetNodeId().ToString().data(), targetEndpoint.GetSlotId().m_id.ToString<AZStd::string>().data()));
                        statusErrors.m_invalidConnectionIds.emplace_back(connectionEntity->GetId());
                    }
                }
            }
        }

        if (!statusErrors.m_graphErrors.empty())
        {
            return AZ::Failure(AZStd::move(statusErrors));
        }
        
        return AZ::Success();
    }
}