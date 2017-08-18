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

#include <IFlowSystem.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LmbrCentral/Scripting/FlowGraphSerialization.h>

namespace FlowGraphUtility
{
    using PreSerializeCallback = AZStd::function<void(IFlowGraphPtr)>;

    IFlowGraphPtr CreateFlowGraph();
    IFlowGraphPtr CreateFlowGraphForEntity(const AZ::EntityId& entityId);

    IFlowGraphPtr CreateFlowGraphFromXML(const AZ::EntityId& entityId, const AZStd::string& flowGraphXML /*, PreSerializeCallback preSerializeCallback = 0*/);

    void AssociateEntityWithGraph(const AZ::EntityId& entityId, IFlowGraphPtr flowGraph);
}

namespace LmbrCentral
{
    class FlowGraphComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
    {
    public:

        friend class EditorFlowGraphComponent;

        AZ_COMPONENT(FlowGraphComponent, "{BE386CD9-4A9C-41AD-8A31-2073EF629A20}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        //! Construction data used during Init to create flowgraphs and update
        //! the node IDs to the serialized AZ::EntityIDs.
        struct FlowGraphData
        {
            AZ_TYPE_INFO(FlowGraphData, "{BEFF26BA-9B80-47A0-8291-2089EB4933B5}");

            SerializedFlowGraph m_flowGraphData;

            static void Reflect(AZ::ReflectContext* context);
        };

        void AddFlowGraphData(FlowGraphData data);

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("FlowGraphService", 0xd7933ebe));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void DeactivateFlowGraphs();

        //! The FlowGraphs that are managed by this component.
        AZStd::vector<IFlowGraphPtr> m_flowGraphs;

        //! Data used to create and initialize flowgraphs during Init.
        AZStd::vector<FlowGraphData> m_flowGraphConstructionData;
    };
} // namespace LmbrCentral
