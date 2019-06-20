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

#include <LyMetricsProducer/LyMetricsAPI.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <Core/GraphBus.h>
#include <AzCore/Asset/AssetCommon.h>

#include <Editor/Include/ScriptCanvas/Components/EditorUtils.h>

namespace ScriptCanvasEditor
{
    namespace Metrics
    {
        static const char* EventName = "ScriptCanvas";

        namespace Actions
        {
            static const char* Operation = "Operation";
        }

        namespace Attributes
        {
            static const char* NodeType = "NodeType";
            static const char* AssetId= "AssetId";
        }

        namespace MetricKeys
        {
            static const char* TotalNodeCount = "TotalNodeCount";
        }

        namespace Events
        {
            // General editor
            namespace Editor
            {
                static const char* Open = "OpenEditor";
                static const char* Close = "CloseEditor";

                static const char* NewFile = "NewFile";
                static const char* SaveFile = "SaveFile";
                static const char* OpenFile = "OpenFile";

                static const char* FilterByLibrary = "FilterByLibrary";
            }

            // Canvas operations
            namespace Canvas
            {
                static const char* OpenGraph = "OpenGraph";
                static const char* CloseGraph = "CloseGraph";

                static const char* AddNode = "AddNode";
                static const char* DeleteNode = "DeleteNode";
                static const char* ConnectNode = "ConnectNode";
                static const char* DisconnectNode = "DisconnectNode";

                static const char* DropNode = "DropNode";
                static const char* DropSender = "DropSender";
                static const char* DropHandler = "DropHandler";
                static const char* DropObject = "DropObject";

                static const char* GraphStatistics = "GraphStatistics";
            }
            
            // Command Line
            namespace CommandLine
            {
                static const char* Open = "OpenCommandLine";
                static const char* Execute = "ExecuteCommand";
            }

            // Documentation
            namespace Documentation
            {
                static const char* MenuClick = "DocumentationMenu";
            }

        } // Events

        class MetricsEventRequests : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            virtual void SendMetric(const char* operation) = 0;
            virtual void SendEditorMetric(const char* operation, const AZ::Data::AssetId& assetId) = 0;
            virtual void SendNodeMetric(const char* operation, const AZ::Uuid& nodeId, AZ::EntityId graphId) = 0;
            virtual void SendGraphMetric(const char* operation, const AZ::Data::AssetId& assetId) = 0;
            virtual void SendGraphStatistics(const AZ::Data::AssetId& assetId, const GraphStatisticsHelper& graphStatistics) = 0;

        };

        using MetricsEventsBus = AZ::EBus<MetricsEventRequests>;

        class SystemComponent
            : public AZ::Component
            , protected MetricsEventsBus::Handler
            , protected ScriptCanvas::GraphNotificationBus::Router
        {

        public:

            AZ_COMPONENT(SystemComponent, "{C87407A5-FD5E-42E8-B85A-D4CC3A428213}", AZ::Component);

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            void Activate() override;
            void Deactivate() override;

            // MetricsEventsBus::Handler
            void SendMetric(const char* operation) override;
            void SendEditorMetric(const char* operation, const AZ::Data::AssetId& assetId) override;
            void SendNodeMetric(const char* operation, const AZ::Uuid& nodeTypeId, AZ::EntityId graphId) override;
            void SendGraphMetric(const char* operation, const AZ::Data::AssetId& assetId) override;
            void SendGraphStatistics(const AZ::Data::AssetId& assetId, const GraphStatisticsHelper& graphStatistics) override;

            // ScriptCanvas::GraphNotificationBus
            void OnNodeAdded(const AZ::EntityId&) override;
            void OnNodeRemoved(const AZ::EntityId&) override;
            void OnConnectionAdded(const AZ::EntityId&) override;
            void OnConnectionRemoved(const AZ::EntityId&) override;

        };

        static void SendMetric(const char* operation, const char* nodeTypeId = "", const char* assetId = "")
        {
            LyMetrics_SendEvent(EventName, {
                { Actions::Operation, operation },
                { Attributes::NodeType, nodeTypeId },
                { Attributes::AssetId, assetId } 
            });
        }

        // Example usage:
        // LyMetrics_SendEvent(ScriptCanvasEditor::Metrics::EventName, { { ScriptCanvasEditor::Metrics::Actions::Operation, ScriptCanvasEditor::Metrics::Events::Editor::Open } });
    } // Metrics
}