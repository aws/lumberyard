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
#include "LmbrCentral_precompiled.h"
#include "FlowGraphComponent.h"

#include <IXml.h>
#include <FlowGraphInformation.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////

    void FlowGraphComponent::Reflect(AZ::ReflectContext* context)
    {
        FlowGraphData::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<FlowGraphComponent, AZ::Component>()
                ->Version(2)
                ->Field("FlowGraphData", &FlowGraphComponent::m_flowGraphConstructionData);
        }
    }

    void FlowGraphComponent::Activate()
    {
        // FlowGraphs get created from xml in this function, we should not have any before now
        AZ_Assert(m_flowGraphs.empty(), "Should not have flowgraphs created before Activate");

        if (!m_flowGraphs.empty())
        {
            m_flowGraphs.clear();
        }

        for (FlowGraphData& flowGraphData : m_flowGraphConstructionData)
        {
            const AZStd::string flowGraphXml = flowGraphData.m_flowGraphData.GenerateLegacyXml(GetEntityId());

            if (IFlowGraphPtr flowGraph = FlowGraphUtility::CreateFlowGraphFromXML(GetEntityId(), flowGraphXml))
            {
                m_flowGraphs.push_back(flowGraph);

                EBUS_EVENT_ID(flowGraph->GetControllingHyperGraphId(), ComponentHyperGraphRequestsBus, BindToFlowgraph, flowGraph);
            }
        }

        AZ::TickBus::Handler::BusConnect();
    }

    void FlowGraphComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();

        DeactivateFlowGraphs();
        m_flowGraphs.clear();
    }

    void FlowGraphComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        for (const auto& flowgraph : m_flowGraphs)
        {
            if (flowgraph)
            {
                flowgraph->Update();
            }
        }
    }

    void FlowGraphComponent::Init()
    {
    }

    void FlowGraphComponent::AddFlowGraphData(FlowGraphData data)
    {
        m_flowGraphConstructionData.emplace_back(data);
    }

    void FlowGraphComponent::DeactivateFlowGraphs()
    {
        for (const auto& flowgraph : m_flowGraphs)
        {
            if (flowgraph)
            {
                EBUS_EVENT_ID(flowgraph->GetControllingHyperGraphId(), ComponentHyperGraphRequestsBus, UnBindFromFlowgraph);
                flowgraph->Uninitialize();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void FlowGraphComponent::FlowGraphData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            SerializedFlowGraph::Reflect(context);

            serializeContext->Class<FlowGraphData>()
                ->Version(1)
                ->Field("FlowGraphData", &FlowGraphData::m_flowGraphData)
            ;
        }
    }
}

namespace FlowGraphUtility
{
    bool SerializeIntoFlowGraphFromXMLString(IFlowGraphPtr flowGraph, const AZStd::string& flowGraphXML)
    {
        IXmlParser* parser = GetISystem()->GetXmlUtils()->CreateXmlParser();
        if (!parser)
        {
            AZ_Assert(parser, "XmlUtils failed to create an XmlParser");
            return false;
        }
        XmlNodeRef xmlRootNode = parser->ParseBuffer(flowGraphXML.c_str(), static_cast<int>(flowGraphXML.length()), false);
        if (!xmlRootNode)
        {
            AZ_Assert(xmlRootNode, "ParseBuffer on FlowGraph xml failed to return a valid xml root node");
            return false;
        }
        bool serializationSucceeded = flowGraph->SerializeXML(xmlRootNode, true);
        AZ_Assert(serializationSucceeded, "Flowgraph serialization from xml failed");
        return serializationSucceeded;
    }

    IFlowGraphPtr CreateFlowGraph()
    {
        if (gEnv && gEnv->pFlowSystem)
        {
            IFlowGraphPtr flowGraph = gEnv->pFlowSystem->CreateFlowGraph();

            if (!flowGraph)
            {
                AZ_Assert(flowGraph, "Flow System did not give us a valid FlowGraph");
                return nullptr;
            }

            flowGraph->UnregisterFromFlowSystem();
            return flowGraph;
        }

        return nullptr;
    }

    IFlowGraphPtr CreateFlowGraphForEntity(const AZ::EntityId& entityId)
    {
        IFlowGraphPtr flowGraph = CreateFlowGraph();

        AssociateEntityWithGraph(entityId, flowGraph);

        return flowGraph;
    }

    IFlowGraphPtr CreateFlowGraphFromXML(const AZ::EntityId& entityId, const AZStd::string& flowGraphXML)
    {
        IFlowGraphPtr flowGraph = CreateFlowGraph();

        if (flowGraph)
        {
            AssociateEntityWithGraph(entityId, flowGraph);

            if (!SerializeIntoFlowGraphFromXMLString(flowGraph, flowGraphXML))
            {
                return nullptr;
            }
        }

        return flowGraph;
    }

    void AssociateEntityWithGraph(const AZ::EntityId& entityId, IFlowGraphPtr flowGraph)
    {
        flowGraph->SetGraphEntity(FlowEntityId(entityId));
        flowGraph->SetType(IFlowGraph::eFGT_FlowGraphComponent);
    }
} // namespace LmbrCentral
