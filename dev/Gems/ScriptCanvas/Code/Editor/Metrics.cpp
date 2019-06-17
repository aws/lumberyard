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

#include "Metrics.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/string/string.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Connection.h>

namespace ScriptCanvasEditor
{
    namespace Metrics
    {
        namespace
        {
            AZStd::vector<AZ::Uuid> GetNodeTypeIds(const AZ::EntityId& nodeId)
            {
                AZStd::vector<AZ::Uuid> nodeTypeIds;
                AZ::Entity* scriptCanvasNodeEntity{};
                AZ::ComponentApplicationBus::BroadcastResult(scriptCanvasNodeEntity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
                if (scriptCanvasNodeEntity)
                {
                    for (auto* scriptCanvasNode : AZ::EntityUtils::FindDerivedComponents(scriptCanvasNodeEntity, AZ::AzTypeInfo<ScriptCanvas::Node>::Uuid()))
                    {
                        nodeTypeIds.push_back(scriptCanvasNode->RTTI_GetType());
                    }
                }
                return nodeTypeIds;
            }
        }

        void SystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<SystemComponent, AZ::Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<SystemComponent>("ScriptCanvasMetrics", "Optionally send Lumberyard usage data to Amazon so that we can create a better user experience.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }
 
        void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasMetricsService", 0x8fc31c9a));
        }

        void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ScriptCanvasMetricsService", 0x8fc31c9a));
        }

        void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            (void)required;
        }

        void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        void SystemComponent::Activate()
        {
            MetricsEventsBus::Handler::BusConnect();
            ScriptCanvas::GraphNotificationBus::Router::BusRouterConnect();
        }

        void SystemComponent::Deactivate()
        {
            MetricsEventsBus::Handler::BusDisconnect();
            ScriptCanvas::GraphNotificationBus::Router::BusRouterDisconnect();
        }

        void SystemComponent::SendMetric(const char* operation)
        {
            Metrics::SendMetric(operation, "", "");
        }

        void SystemComponent::SendEditorMetric(const char* operation, const AZ::Data::AssetId& assetId)
        {
            Metrics::SendMetric(operation, "", assetId.ToString<AZStd::string>().c_str());
        }

        void SystemComponent::SendNodeMetric(const char* operation, const AZ::Uuid& nodeTypeId, AZ::EntityId graphId)
        {
            Metrics::SendMetric(operation, nodeTypeId.ToString<AZStd::string>().c_str(), graphId.ToString().c_str());
        }

        void SystemComponent::SendGraphMetric(const char* operation, const AZ::Data::AssetId& assetId)
        {
            Metrics::SendMetric(operation, "", assetId.ToString<AZStd::string>().c_str());
        }

        void SystemComponent::SendGraphStatistics(const AZ::Data::AssetId& assetId, const GraphStatisticsHelper& graphStatistics)
        {
            auto eventId = LyMetrics_CreateEvent(EventName);

            LyMetrics_AddAttribute(eventId, Actions::Operation, Events::Canvas::GraphStatistics);
            LyMetrics_AddAttribute(eventId, Attributes::AssetId, assetId.ToString<AZStd::string>().c_str());

            double totalNodeCount = 0.0;

            for (auto usagePair : graphStatistics.m_nodeIdentifierCount)
            {
                AZStd::string nodeIdentifier = AZStd::string::format("%zu", usagePair.first);
                LyMetrics_AddMetric(eventId, nodeIdentifier.c_str(), usagePair.second);

                totalNodeCount += usagePair.second;
            }

            LyMetrics_AddMetric(eventId, MetricKeys::TotalNodeCount, static_cast<double>(totalNodeCount));
            LyMetrics_SubmitEvent(eventId);
        }

        void SystemComponent::OnNodeAdded(const AZ::EntityId& nodeId)
        {
            for (auto nodeType : GetNodeTypeIds(nodeId))
            {
                SendNodeMetric(Events::Canvas::AddNode, nodeType, *ScriptCanvas::GraphNotificationBus::GetCurrentBusId());
            }
        }
        
        void SystemComponent::OnNodeRemoved(const AZ::EntityId& nodeId)
        {
            for (auto nodeType : GetNodeTypeIds(nodeId))
            {
                SendNodeMetric(Events::Canvas::DeleteNode, nodeType, *ScriptCanvas::GraphNotificationBus::GetCurrentBusId());
            }
        }

        void SystemComponent::OnConnectionAdded(const AZ::EntityId& connectionId)
        {
            if (connectionId.IsValid())
            {
                AZ::Entity* connectionEntity{};
                AZ::ComponentApplicationBus::BroadcastResult(connectionEntity, &AZ::ComponentApplicationRequests::FindEntity, connectionId);
                auto connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Connection>(connectionEntity) : nullptr;
                if (connection)
                {
                    for (auto nodeType : GetNodeTypeIds(connection->GetSourceNode()))
                    {
                        SendNodeMetric(Events::Canvas::ConnectNode, nodeType, *ScriptCanvas::GraphNotificationBus::GetCurrentBusId());
                    }

                    for (auto nodeType : GetNodeTypeIds(connection->GetTargetNode()))
                    {
                        SendNodeMetric(Events::Canvas::ConnectNode, nodeType, *ScriptCanvas::GraphNotificationBus::GetCurrentBusId());
                    }
                }
            }
        }

        void SystemComponent::OnConnectionRemoved(const AZ::EntityId& connectionId)
        {
            if (connectionId.IsValid())
            {
                AZ::Entity* connectionEntity{};
                AZ::ComponentApplicationBus::BroadcastResult(connectionEntity, &AZ::ComponentApplicationRequests::FindEntity, connectionId);
                auto connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Connection>(connectionEntity) : nullptr;
                if (connection)
                {
                    for (auto nodeType : GetNodeTypeIds(connection->GetSourceNode()))
                    {
                        SendNodeMetric(Events::Canvas::DisconnectNode, nodeType, *ScriptCanvas::GraphNotificationBus::GetCurrentBusId());
                    }

                    for (auto nodeType : GetNodeTypeIds(connection->GetTargetNode()))
                    {
                        SendNodeMetric(Events::Canvas::DisconnectNode, nodeType, *ScriptCanvas::GraphNotificationBus::GetCurrentBusId());
                    }
                }
            }
        }
    }
}
