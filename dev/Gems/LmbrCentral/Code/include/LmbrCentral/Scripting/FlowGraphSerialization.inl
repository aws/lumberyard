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

#include <AzCore/EBus/EBus.h>
#include <FlowGraphInformation.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/IO/TextStreamWriters.h>

namespace LmbrCentral
{
    //=========================================================================
    // Input ctor
    //=========================================================================
    inline bool SerializedFlowGraphInputConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        (void)context;
        (void)classElement;

        const int valueIndex = classElement.FindElement(AZ_CRC("Value", 0x1d775834));
        if (valueIndex >= 0)
        {
            // [Old Hierarchy]                                 [New Hierarchy]
            //  Input                                           Input
            //      Value (impl of InputValueBase)                  Value (intrusive_ptr to impl of InputValueBase)
            //          BaseClass1                                      element (intrusive_ptr is a container)
            //          Value (primitive value data)                        BaseClass1
            //                                                                  Value (primitive value data)
            //
            // So we're basically taking the old InputValueBase hierarchy and re-inserting it under a new intrusive_ptr/element.

            // Store off sub elements we need to re-inject.
            AZ::SerializeContext::DataElementNode valueNode = classElement.GetSubElement(valueIndex);

            // Remove the old value node.
            classElement.RemoveElement(valueIndex);

            const int newValueIndex = classElement.AddElement<AZStd::intrusive_ptr<SerializedFlowGraph::InputValueBase> >(context, "Value");
            AZ::SerializeContext::DataElementNode& newValueNode = classElement.GetSubElement(newValueIndex);

            // Add an 'element' sub-item to the intrusive_ptr container.
            AZ::SerializeContext::DataElementNode& valueElementNode = newValueNode.GetSubElement(newValueNode.AddElement(context, "element", valueNode.GetId()));

            for (int subElementIndex = 0; subElementIndex < valueNode.GetNumSubElements(); ++subElementIndex)
            {
                valueElementNode.AddElement(valueNode.GetSubElement(subElementIndex));
            }

            return true;
        }

        return false;
    }

    //=========================================================================
    // Input ctor
    //=========================================================================
    inline SerializedFlowGraph::Input::Input()
        : m_type(FlowVariableType::Unknown)
        , m_value(nullptr)
        , m_valueTest(aznew InputValueBool(true))
        , m_persistentId(InvalidPersistentId)
    {
    }

    inline SerializedFlowGraph::Input::Input(const char* name, const char* humanName, FlowVariableType type, InputValueBase* value)
        : m_name(name)
        , m_humanName(humanName)
        , m_type(type)
        , m_value(value)
        , m_valueTest(aznew InputValueBool(true))
        , m_persistentId(InvalidPersistentId)
    {
    }

    inline SerializedFlowGraph::Input::~Input()
    {
    }

    //=========================================================================
    // Input::Reflect
    //=========================================================================
    inline void SerializedFlowGraph::Input::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<InputValueBase>()->Version(1);
            serialize->Class<InputValueVoid, InputValueBase>()
                ->Version(1);
            serialize->Class<InputValueInt, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueInt::m_value);
            serialize->Class<InputValueFloat, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueFloat::m_value);
            serialize->Class<InputValueEntityId, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueEntityId::m_value);
            serialize->Class<InputValueVec2, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueVec2::m_value);
            serialize->Class<InputValueVec3, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueVec3::m_value);
            serialize->Class<InputValueVec4, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueVec4::m_value);
            serialize->Class<InputValueQuat, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueQuat::m_value);
            serialize->Class<InputValueString, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueString::m_value);
            serialize->Class<InputValueBool, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueBool::m_value);
            serialize->Class<InputValueDouble, InputValueBase>()
                ->Version(1)
                ->Field("Value", &InputValueDouble::m_value);

            serialize->Class<Input>()
                ->Version(2, &SerializedFlowGraphInputConverter)
                ->PersistentId([](const void* instance) -> AZ::u64 { return reinterpret_cast<const Input*>(instance)->m_persistentId; })
                ->Field("Name", &Input::m_name)
                ->Field("HumanName", &Input::m_humanName)
                ->Field("DataType", &Input::m_type)
                ->Field("Value", &Input::m_value)
                ->Field("PersistentId", &Input::m_persistentId)
            ;
        }
    }

    //=========================================================================
    // Node ctor
    //=========================================================================
    inline SerializedFlowGraph::Node::Node()
        : m_id(InvalidFlowNodeId)
        , m_isGraphEntity(false)
        , m_position(AZ::Vector2::CreateZero())
        , m_size(AZ::Vector2::CreateZero())
        , m_borderRect(AZ::Vector2::CreateZero())
        , m_flags(0)
        , m_inputHideMask(0)
        , m_outputHideMask(0)
    {
    }

    //=========================================================================
    // Node::Reflect
    //=========================================================================
    inline void SerializedFlowGraph::Node::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<Node>()
                ->Version(1)
                ->PersistentId([](const void* instance) -> AZ::u64 { return reinterpret_cast<const Node*>(instance)->m_id; })
                ->Field("Id", &Node::m_id)
                ->Field("Name", &Node::m_name)
                ->Field("Class", &Node::m_class)
                ->Field("EntityId", &Node::m_entityId)
                ->Field("IsGraphEntity", &Node::m_isGraphEntity)
                ->Field("Position", &Node::m_position)
                ->Field("Size", &Node::m_size)
                ->Field("BorderRect", &Node::m_borderRect)
                ->Field("Flags", &Node::m_flags)
                ->Field("InputHideMask", &Node::m_inputHideMask)
                ->Field("OutputHideMask", &Node::m_outputHideMask)
                ->Field("Inputs", &Node::m_inputs)
            ;
        }
    }

    //=========================================================================
    // Edge ctor
    //=========================================================================
    inline SerializedFlowGraph::Edge::Edge()
        : m_nodeIn(0)
        , m_nodeOut(0)
        , m_isEnabled(true)
        , m_persistentId(InvalidPersistentId)
    {
    }

    //=========================================================================
    // Edge::Reflect
    //=========================================================================
    inline void SerializedFlowGraph::Edge::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<Edge>()
                ->Version(1)
                ->PersistentId([](const void* instance) -> AZ::u64 { return reinterpret_cast<const Edge*>(instance)->m_persistentId; })
                ->Field("NodeIn", &Edge::m_nodeIn)
                ->Field("NodeOut", &Edge::m_nodeOut)
                ->Field("PortIn", &Edge::m_portIn)
                ->Field("PortOut", &Edge::m_portOut)
                ->Field("IsEnabled", &Edge::m_isEnabled)
                ->Field("PersistentId", &Edge::m_persistentId)
            ;
        }
    }

    //=========================================================================
    // GraphToken ctor
    //=========================================================================
    inline SerializedFlowGraph::GraphToken::GraphToken()
        : m_type(eFDT_Any)
        , m_persistentId(InvalidPersistentId)
    {
    }

    //=========================================================================
    // GraphToken::Reflect
    //=========================================================================
    inline void SerializedFlowGraph::GraphToken::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GraphToken>()
                ->Version(1)
                ->PersistentId([](const void* instance) -> AZ::u64 { return reinterpret_cast<const GraphToken*>(instance)->m_persistentId; })
                ->Field("Name", &GraphToken::m_name)
                ->Field("Type", &GraphToken::m_type)
                ->Field("PersistentId", &GraphToken::m_persistentId)
            ;
        }
    }

    //=========================================================================
    // SerializedFlowGraph ctor
    //=========================================================================
    inline SerializedFlowGraph::SerializedFlowGraph()
        : m_isEnabled(true)
        , m_networkType(FlowGraphNetworkType::ClientOnly)
        , m_persistentId(InvalidPersistentId)
    {
    }

    //=========================================================================
    // SerializedFlowGraph::Reflect
    //=========================================================================
    inline void SerializedFlowGraph::Reflect(AZ::ReflectContext* context)
    {
        Input::Reflect(context);
        Node::Reflect(context);
        Edge::Reflect(context);
        GraphToken::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SerializedFlowGraph>()
                ->Version(1)
                ->PersistentId([](const void* instance) -> AZ::u64 { return reinterpret_cast<const SerializedFlowGraph*>(instance)->m_persistentId; })
                ->Field("Name", &SerializedFlowGraph::m_name)
                ->Field("Description", &SerializedFlowGraph::m_description)
                ->Field("Group", &SerializedFlowGraph::m_group)
                ->Field("IsEnabled", &SerializedFlowGraph::m_isEnabled)
                ->Field("NetworkType", &SerializedFlowGraph::m_networkType)
                ->Field("Nodes", &SerializedFlowGraph::m_nodes)
                ->Field("Edges", &SerializedFlowGraph::m_edges)
                ->Field("GraphTokens", &SerializedFlowGraph::m_graphTokens)
                ->Field("PersistentId", &SerializedFlowGraph::m_persistentId)
                ->Field("HyperGraphId", &SerializedFlowGraph::m_hypergraphId)
            ;
        }
    }

    //=========================================================================
    // SerializedFlowGraph::GenerateLegacyXml
    //=========================================================================
    inline AZStd::string SerializedFlowGraph::GenerateLegacyXml(const AZ::EntityId& entityId) const
    {
        char scratchBuffer[2048];

        AZ::rapidxml::xml_document<char> xmlDoc;
        AZ::rapidxml::xml_node<char>* fgNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "FlowGraph");
        {
            AZ::rapidxml::xml_attribute<char>* attrib = xmlDoc.allocate_attribute("Description", m_description.c_str());
            fgNode->append_attribute(attrib);
            attrib = xmlDoc.allocate_attribute("Name", m_name.c_str());
            fgNode->append_attribute(attrib);
            attrib = xmlDoc.allocate_attribute("Group", m_group.c_str());
            fgNode->append_attribute(attrib);
            attrib = xmlDoc.allocate_attribute("Enabled", m_isEnabled ? "1" : "0");
            fgNode->append_attribute(attrib);
            attrib = xmlDoc.allocate_attribute("Multiplayer",
                    m_networkType == FlowGraphNetworkType::ServerOnly ? "ServerOnly" :
                    (m_networkType == FlowGraphNetworkType::ClientOnly ? "ClientOnly" : "ClientServer"));
            fgNode->append_attribute(attrib);

            AZ::rapidxml::xml_node<char>* hypergraphIdNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "HyperGraphId");
            AZStd::string graphIdAsString = m_hypergraphId.ToString<AZStd::string>(true, true);
            attrib = xmlDoc.allocate_attribute("Id", xmlDoc.allocate_string(graphIdAsString.c_str()));
            hypergraphIdNode->append_attribute(attrib);
            fgNode->append_node(hypergraphIdNode);

            AZ::rapidxml::xml_node<char>* nodesNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "Nodes");

            for (const SerializedFlowGraph::Node& node : m_nodes)
            {
                AZ::rapidxml::xml_node<char>* nodeNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "Node");

                sprintf_s(scratchBuffer, "%u", node.m_id);
                attrib = xmlDoc.allocate_attribute("Id", xmlDoc.allocate_string(scratchBuffer));
                nodeNode->append_attribute(attrib);
                attrib = xmlDoc.allocate_attribute("Class", node.m_class.c_str());
                nodeNode->append_attribute(attrib);
                attrib = xmlDoc.allocate_attribute("Name", node.m_name.c_str());
                nodeNode->append_attribute(attrib);
                sprintf_s(scratchBuffer, "%f,%f,0", node.m_position.GetX(), node.m_position.GetY());
                attrib = xmlDoc.allocate_attribute("pos", xmlDoc.allocate_string(scratchBuffer));
                nodeNode->append_attribute(attrib);
                sprintf_s(scratchBuffer, "%u", node.m_flags);
                attrib = xmlDoc.allocate_attribute("flags", xmlDoc.allocate_string(scratchBuffer));
                nodeNode->append_attribute(attrib);
                sprintf_s(scratchBuffer, "%u", node.m_inputHideMask);
                attrib = xmlDoc.allocate_attribute("InHideMask", xmlDoc.allocate_string(scratchBuffer));
                nodeNode->append_attribute(attrib);
                sprintf_s(scratchBuffer, "%u", node.m_outputHideMask);
                attrib = xmlDoc.allocate_attribute("OutHideMask", xmlDoc.allocate_string(scratchBuffer));
                nodeNode->append_attribute(attrib);

                AZ::EntityId nodeEntityId = node.m_entityId;

                if (node.m_isGraphEntity)
                {
                    attrib = xmlDoc.allocate_attribute("GraphEntity", "0");
                    nodeNode->append_attribute(attrib);

                    if (!nodeEntityId.IsValid())
                    {
                        nodeEntityId = entityId;
                    }
                }

                if (nodeEntityId.IsValid())
                {
                    sprintf_s(scratchBuffer, "%" PRIX64, static_cast<AZ::u64>(nodeEntityId));
                    attrib = xmlDoc.allocate_attribute("ComponentEntityId", xmlDoc.allocate_string(scratchBuffer));
                    nodeNode->append_attribute(attrib);
                }

                if (!node.m_size.IsZero())
                {
                    AZ::rapidxml::xml_node<char>* nodeSizeNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "NodeSize");

                    sprintf_s(scratchBuffer, "%f", node.m_size.GetX());
                    attrib = xmlDoc.allocate_attribute("Width", xmlDoc.allocate_string(scratchBuffer));
                    nodeSizeNode->append_attribute(attrib);
                    sprintf_s(scratchBuffer, "%f", node.m_size.GetY());
                    attrib = xmlDoc.allocate_attribute("Height", xmlDoc.allocate_string(scratchBuffer));
                    nodeSizeNode->append_attribute(attrib);

                    nodeNode->append_node(nodeSizeNode);
                }

                if (!node.m_borderRect.IsZero())
                {
                    AZ::rapidxml::xml_node<char>* resizeBorderNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "ResizeBorder");

                    attrib = xmlDoc.allocate_attribute("X", "0");
                    resizeBorderNode->append_attribute(attrib);
                    attrib = xmlDoc.allocate_attribute("Y", "0");
                    resizeBorderNode->append_attribute(attrib);
                    sprintf_s(scratchBuffer, "%f", node.m_borderRect.GetX());
                    attrib = xmlDoc.allocate_attribute("Width", xmlDoc.allocate_string(scratchBuffer));
                    resizeBorderNode->append_attribute(attrib);
                    sprintf_s(scratchBuffer, "%f", node.m_borderRect.GetY());
                    attrib = xmlDoc.allocate_attribute("Height", xmlDoc.allocate_string(scratchBuffer));
                    resizeBorderNode->append_attribute(attrib);

                    nodeNode->append_node(resizeBorderNode);
                }

                AZ::rapidxml::xml_node<char>* inputsNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "Inputs");

                for (const SerializedFlowGraph::Input& input : node.m_inputs)
                {
                    scratchBuffer[0] = '\0';

                    if (!input.m_value)
                    {
                        continue;
                    }

                    switch (input.m_type)
                    {
                    case FlowVariableType::Unknown:
                    {
                        continue;
                    }
                    break;
                    case FlowVariableType::Int:
                    {
                        sprintf_s(scratchBuffer, "%d", static_cast<SerializedFlowGraph::InputValueInt*>(input.m_value.get())->m_value);
                    }
                    break;
                    case FlowVariableType::Bool:
                    {
                        sprintf_s(scratchBuffer, "%d", static_cast<SerializedFlowGraph::InputValueBool*>(input.m_value.get())->m_value ? 1 : 0);
                    }
                    break;
                    case FlowVariableType::Float:
                    {
                        sprintf_s(scratchBuffer, "%f", static_cast<SerializedFlowGraph::InputValueFloat*>(input.m_value.get())->m_value);
                    }
                    break;
                    case FlowVariableType::Vector2:
                    {
                        const AZ::Vector2& value = static_cast<SerializedFlowGraph::InputValueVec2*>(input.m_value.get())->m_value;
                        sprintf_s(scratchBuffer, "%f,%f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()));
                    }
                    break;
                    case FlowVariableType::Vector3:
                    {
                        const AZ::Vector3& value = static_cast<SerializedFlowGraph::InputValueVec3*>(input.m_value.get())->m_value;
                        sprintf_s(scratchBuffer, "%f,%f,%f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()));
                    }
                    break;
                    case FlowVariableType::Vector4:
                    {
                        const AZ::Vector4& value = static_cast<SerializedFlowGraph::InputValueVec4*>(input.m_value.get())->m_value;
                        sprintf_s(scratchBuffer, "%f,%f,%f,%f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
                    }
                    break;
                    case FlowVariableType::Quat:
                    {
                        const AZ::Quaternion& value = static_cast<SerializedFlowGraph::InputValueQuat*>(input.m_value.get())->m_value;
                        sprintf_s(scratchBuffer, "%f,%f,%f,%f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
                    }
                    break;
                    case FlowVariableType::String:
                    {
                        azstrcpy(scratchBuffer, AZ_ARRAY_SIZE(scratchBuffer), static_cast<SerializedFlowGraph::InputValueString*>(input.m_value.get())->m_value.c_str());
                    }
                    break;
                    case FlowVariableType::Double:
                    {
                        sprintf_s(scratchBuffer, "%f", static_cast<SerializedFlowGraph::InputValueDouble*>(input.m_value.get())->m_value);
                    }
                    break;
                    }

                    attrib = xmlDoc.allocate_attribute(input.m_name.c_str(), xmlDoc.allocate_string(scratchBuffer));
                    inputsNode->append_attribute(attrib);
                }

                nodeNode->append_node(inputsNode);

                nodesNode->append_node(nodeNode);
            }

            fgNode->append_node(nodesNode);

            AZ::rapidxml::xml_node<char>* edgesNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "Edges");

            for (const SerializedFlowGraph::Edge& edge : m_edges)
            {
                AZ::rapidxml::xml_node<char>* edgeNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "Edge");

                sprintf_s(scratchBuffer, "%u", edge.m_nodeIn);
                attrib = xmlDoc.allocate_attribute("nodeIn", xmlDoc.allocate_string(scratchBuffer));
                edgeNode->append_attribute(attrib);
                sprintf_s(scratchBuffer, "%u", edge.m_nodeOut);
                attrib = xmlDoc.allocate_attribute("nodeOut", xmlDoc.allocate_string(scratchBuffer));
                edgeNode->append_attribute(attrib);
                attrib = xmlDoc.allocate_attribute("portIn", edge.m_portIn.c_str());
                edgeNode->append_attribute(attrib);
                attrib = xmlDoc.allocate_attribute("portOut", edge.m_portOut.c_str());
                edgeNode->append_attribute(attrib);
                attrib = xmlDoc.allocate_attribute("enabled", edge.m_isEnabled ? "1" : "0");
                edgeNode->append_attribute(attrib);

                edgesNode->append_node(edgeNode);
            }

            fgNode->append_node(edgesNode);

            AZ::rapidxml::xml_node<char>* tokensNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "GraphTokens");

            for (const SerializedFlowGraph::GraphToken& token : m_graphTokens)
            {
                AZ::rapidxml::xml_node<char>* tokenNode = xmlDoc.allocate_node(AZ::rapidxml::node_element, "Token");

                attrib = xmlDoc.allocate_attribute("Name", token.m_name.c_str());
                tokenNode->append_attribute(attrib);
                sprintf_s(scratchBuffer, "%" PRIu64, token.m_type);
                attrib = xmlDoc.allocate_attribute("Type", xmlDoc.allocate_string(scratchBuffer));
                tokenNode->append_attribute(attrib);

                tokensNode->append_node(tokenNode);
            }

            fgNode->append_node(tokensNode);
        }

        xmlDoc.append_node(fgNode);

        AZStd::string textData;
        AZ::rapidxml::print(AZStd::back_inserter(textData), xmlDoc);
        return textData;
    }
} // namespace LmbrCentral
