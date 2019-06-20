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

#include "Node.h"

#include "Graph.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/IdUtils.h>
#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/ExclusivePureDataContract.h>
#include <ScriptCanvas/Core/Contracts/DynamicTypeContract.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/PureData.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Debugger/API.h>
#include <ScriptCanvas/Utils/NodeUtils.h>

namespace ScriptCanvas
{
    void VariableInfo::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VariableInfo>()
                ->Version(0)
                ->Field("ActiveVariableId", &VariableInfo::m_currentVariableId)
                ->Field("NodeVariableId", &VariableInfo::m_ownedVariableId)
                ->Field("DataType", &VariableInfo::m_dataType)
                ;
        }
    }

    VariableInfo::VariableInfo(const VariableId& nodeOwnedVarId)
        : m_currentVariableId(nodeOwnedVarId)
        , m_ownedVariableId(nodeOwnedVarId)
    {
        VariableRequestBus::EventResult(m_dataType, m_currentVariableId, &VariableRequests::GetType);
    }

    VariableInfo::VariableInfo(const Data::Type& dataType)
        : m_dataType(dataType)
    {
    }

    Node::Node()
        : AZ::Component()
        , m_executionUniqueId(InvalidUniqueRuntimeId)
    {}

    Node::Node(const Node&)
    {}

    Node& Node::operator=(const Node&)
    {
        return *this;
    }

    Node::~Node()
    {
        NodeRequestBus::Handler::BusDisconnect();
    }

    void Node::Init()
    {
        NodeRequestBus::Handler::BusConnect(GetEntityId());
        DatumNotificationBus::Handler::BusConnect(GetEntityId());
        EditorNodeRequestBus::Handler::BusConnect(GetEntityId());

        for (auto& slot : m_slots)
        {
            slot.SetNodeId(GetEntityId());
        }

        for (VariableDatumBase& varDatum : m_varDatums)
        {
            varDatum.GetData().SetNotificationsTarget(GetEntityId());
        }

        OnInit();        

        PopulateNodeType();
    }

    void Node::Activate()
    {
        SignalBus::Handler::BusConnect(GetEntityId());
        OnActivate();
        MarkDefaultableInput();
    }

    void Node::PopulateNodeType()
    {
        m_nodeType = NodeUtils::ConstructNodeType(this);
    }

    void Node::Configure()
    {
        ConfigureSlots();
    }

    void Node::Deactivate()
    {
        OnDeactivate();

        SignalBus::Handler::BusDisconnect();
    }

    AZStd::string Node::GetSlotName(const SlotId& slotId) const
    {
        if (slotId.IsValid())
        {
            auto slot = GetSlot(slotId);
            if (slot)
            {
                return slot->GetName();
            }
        }
        return "";
    }

    class NodeEventHandler
        : public AZ::SerializeContext::IEventHandler
    {
    public:
        void OnWriteEnd(void* objectPtr) override
        {
            auto node = reinterpret_cast<Node*>(objectPtr);
            node->RebuildSlotAndVariableIterators();
        }
    };

    bool NodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& nodeElementNode)
    {
        if (nodeElementNode.GetVersion() <= 5)
        {
            auto slotVectorElementNodes = AZ::Utils::FindDescendantElements(context, nodeElementNode, AZStd::vector<AZ::Crc32>{AZ_CRC("Slots", 0xc87435d0), AZ_CRC("m_slots", 0x84838ab4)});
            if (slotVectorElementNodes.empty())
            {
                AZ_Error("Script Canvas", false, "Node version %u is missing SlotContainer container structure", nodeElementNode.GetVersion());
                return false;
            }

            auto& slotVectorElementNode = slotVectorElementNodes.front();
            AZStd::vector<Slot> oldSlots;
            if (!slotVectorElementNode->GetData(oldSlots))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the SlotContainer AZStd::vector<Slot> structure from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            // Datum -> VarDatum
            int datumArrayElementIndex = nodeElementNode.FindElement(AZ_CRC("m_inputData", 0xba1b1449));
            if (datumArrayElementIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Unable to find the Datum array structure on Node class version %u", nodeElementNode.GetVersion());
                return false;
            }

            auto& datumArrayElementNode = nodeElementNode.GetSubElement(datumArrayElementIndex);
            AZStd::vector<Datum> oldData;
            if (!datumArrayElementNode.GetData(oldData))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Datum array structure from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            // Retrieve the old AZStd::vector<Data::Type>
            int dataTypeArrayElementIndex = nodeElementNode.FindElement(AZ_CRC("m_outputTypes", 0x6be6d8c2));
            if (dataTypeArrayElementIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Unable to find the Data::Type array structure on the Node class version %u", nodeElementNode.GetVersion());
                return false;
            }

            auto& dataTypeArrayElementNode = nodeElementNode.GetSubElement(dataTypeArrayElementIndex);
            AZStd::vector<Data::Type> oldDataTypes;
            if (!dataTypeArrayElementNode.GetData(oldDataTypes))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Data::Type array structure from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            // Retrieve the Slot index -> Datum index map
            int slotDatumIndexMapElementIndex = nodeElementNode.FindElement(AZ_CRC("m_inputIndexBySlotIndex", 0xf429c4e7));
            if (slotDatumIndexMapElementIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Unable to find the Slot Index to Data::Type Index Map on the Node class version %u", nodeElementNode.GetVersion());
                return false;
            }

            auto& slotDatumIndexMapElementNode = nodeElementNode.GetSubElement(slotDatumIndexMapElementIndex);
            AZStd::unordered_map<int, int> slotIndexToDatumIndexMap;
            if (!slotDatumIndexMapElementNode.GetData(slotIndexToDatumIndexMap))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Slot Index to Data::Type Index Map from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            // Retrieve the Slot index -> Data::Type index map
            int slotDataTypeIndexMapElementIndex = nodeElementNode.FindElement(AZ_CRC("m_outputTypeIndexBySlotIndex", 0xc51484b2));
            if (slotDataTypeIndexMapElementIndex == -1)
            {
                AZ_Error("Script Canvas", false, "Unable to find the Slot Index to Data::Type Index Map on the Node class version %u", nodeElementNode.GetVersion());
                return false;
            }

            auto& slotDataTypeIndexMapElementNode = nodeElementNode.GetSubElement(slotDataTypeIndexMapElementIndex);
            AZStd::unordered_map<int, int> slotIndexToDataTypeIndexMap;
            if (!slotDataTypeIndexMapElementNode.GetData(slotIndexToDataTypeIndexMap))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Slot Index to Data::Type Index Map from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            AZStd::vector<VariableDatum> newVariableData;
            newVariableData.reserve(oldData.size());
            for (const auto& oldDatum : oldData)
            {
                newVariableData.emplace_back(VariableDatum(oldDatum));
            }
            
            AZStd::unordered_map<SlotId, VariableInfo> slotIdVarInfoMap;
            for (const auto& slotIndexDatumIndexPair : slotIndexToDatumIndexMap)
            {
                const auto& varId = newVariableData[slotIndexDatumIndexPair.second].GetId();
                const auto& dataType = newVariableData[slotIndexDatumIndexPair.second].GetData().GetType();
                slotIdVarInfoMap[oldSlots[slotIndexDatumIndexPair.first].GetId()].m_ownedVariableId = varId;
                slotIdVarInfoMap[oldSlots[slotIndexDatumIndexPair.first].GetId()].m_currentVariableId = varId;
                slotIdVarInfoMap[oldSlots[slotIndexDatumIndexPair.first].GetId()].m_dataType = dataType;
            }

            for (const auto& slotIndexDataTypeIndexPair : slotIndexToDataTypeIndexMap)
            {
                slotIdVarInfoMap[oldSlots[slotIndexDataTypeIndexPair.first].GetId()].m_dataType = oldDataTypes[slotIndexDataTypeIndexPair.second];
            }

            // Remove all the version 5 and below DataElements
            nodeElementNode.RemoveElementByName(AZ_CRC("Slots", 0xc87435d0));
            nodeElementNode.RemoveElementByName(AZ_CRC("m_outputTypes", 0x6be6d8c2));
            nodeElementNode.RemoveElementByName(AZ_CRC("m_inputData", 0xba1b1449));
            nodeElementNode.RemoveElementByName(AZ_CRC("m_inputIndexBySlotIndex", 0xf429c4e7));
            nodeElementNode.RemoveElementByName(AZ_CRC("m_outputTypeIndexBySlotIndex", 0xc51484b2));

            // Move the old slots from the AZStd::vector to an AZStd::list
            Node::SlotList newSlots{ AZStd::make_move_iterator(oldSlots.begin()), AZStd::make_move_iterator(oldSlots.end()) };
            if (nodeElementNode.AddElementWithData(context, "Slots", newSlots) == -1)
            {
                AZ_Error("Script Canvas", false, "Failed to add Slot List container to the serialized node element");
                return false;
            }

            // The new variable datum structure is a AZStd::list
            AZStd::list<VariableDatum> newVarDatums{ AZStd::make_move_iterator(newVariableData.begin()), AZStd::make_move_iterator(newVariableData.end()) };
            if (nodeElementNode.AddElementWithData(context, "Variables", newVarDatums) == -1)
            {
                AZ_Error("Script Canvas", false, "Failed to add Variable List container to the serialized node element");
                return false;
            }

            // Add the SlotId, VariableId Pair array to the Node
            if (nodeElementNode.AddElementWithData(context, "SlotToVariableInfoMap", slotIdVarInfoMap) == -1)
            {
                AZ_Error("Script Canvas", false, "Failed to add SlotId, Variable Id Pair array to the serialized node element");
                return false;
            }
        }

        if (nodeElementNode.GetVersion() <= 6)
        {
            // Finds the AZStd::list<VariableDatum> and replaces that with an AZStd::list<VariableDatumBase> which does not have the exposure/or visibility options
            AZStd::list<VariableDatum> oldVarDatums;
            if (!nodeElementNode.GetChildData(AZ_CRC("Variables", 0x88cb7d11), oldVarDatums))
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the Variable Datum list structure from Node version %u. Node version conversion has failed", nodeElementNode.GetVersion());
                return false;
            }

            nodeElementNode.RemoveElementByName(AZ_CRC("Variables", 0x88cb7d11));

            AZStd::list<VariableDatumBase> newVarDatumBases;
            for (const auto& oldVarDatum : oldVarDatums)
            {
                newVarDatumBases.emplace_back(oldVarDatum);
            }

            if (nodeElementNode.AddElementWithData(context, "Variables", newVarDatumBases) == -1)
            {
                AZ_Error("Script Canvas", false, "Failed to add Variable Datum Base list to the node element");
                return false;
            }
        }

        return true;
    }

    void Node::Reflect(AZ::ReflectContext* context)
    {
        VariableInfo::Reflect(context);
        Slot::Reflect(context);
        ExclusivePureDataContract::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Needed to serialize in the AZStd::vector<Slot> from the old SlotContainer class 
            AZ::GenericClassInfo* genericInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<Slot>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(serializeContext);
            }
            // Needed to serialize in the AZStd::vector<Datum> from this class
            genericInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<Datum>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(serializeContext);
            }

            // Needed to serialize in the AZStd::vector<Data::Type> from version 5 and below
            genericInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<Data::Type>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(serializeContext);
            }

            // Needed to serialize in the AZStd::unordered<int, int> from version 5 and below
            genericInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_map<int, int>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(serializeContext);
            }

            // Needed to serialize in the AZStd::list<VariableDatum> from version 6 and below
            genericInfo = AZ::SerializeGenericTypeInfo<AZStd::list<VariableDatum>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(serializeContext);
            }

            serializeContext->Class<Node, AZ::Component>()
                ->EventHandler<NodeEventHandler>()
                ->Version(7, &NodeVersionConverter)
                ->Field("UniqueGraphID", &Node::m_executionUniqueId)
                ->Field("Slots", &Node::m_slots)
                ->Field("Variables", &Node::m_varDatums)
                ->Field("SlotToVariableInfoMap", &Node::m_slotIdVarInfoMap)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Node>("Node", "Node")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Node::m_varDatums, "Input", "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    void Node::RebuildSlotAndVariableIterators()
    {
        m_slotIdMap.clear();
        m_slotNameMap.clear();
        for (auto slotIter = m_slots.begin(); slotIter != m_slots.end(); ++slotIter)
        {
            m_slotIdMap.emplace(slotIter->GetId(), slotIter);
            m_slotNameMap.emplace(slotIter->GetName(), slotIter);
        }

        m_varIdMap.clear();
        for (auto varIter = m_varDatums.begin(); varIter != m_varDatums.end(); ++varIter)
        {
            m_varIdMap.emplace(varIter->GetId(), varIter);
        }
    }

    void Node::MarkDefaultableInput()
    {
        for (const auto& slotIdIterPair : m_slotIdMap)
        {
            const auto& inputSlot = *slotIdIterPair.second;
            const auto& slotId = slotIdIterPair.first;

            if (inputSlot.GetType() == SlotType::DataIn)
            {
                // for each output slot...
                // for each connected node...
                // remove the ability to default it...
                // ...and until a more viable solution is available, variable get input in another node must be exclusive   

                AZStd::vector<AZStd::pair<const Node*, const SlotId>> connections = GetConnectedNodes(inputSlot);
                if (!connections.empty())
                {
                    bool isConnectedToPureData = false;

                    for (auto& nodePtrSlotId : connections)
                    {
                        if (azrtti_cast<const PureData*>(nodePtrSlotId.first))
                        {
                            isConnectedToPureData = true;
                            break;
                        }
                    }

                    if (!isConnectedToPureData)
                    {
                        m_possiblyStaleInput.insert(slotId);
                    }
                }
            }
        }
    }

    bool Node::IsInEventHandlingScope(const ID& possibleEventHandler) const
    {
        Node* node{};
        RuntimeRequestBus::EventResult(node, m_executionUniqueId, &RuntimeRequests::FindNode, possibleEventHandler);
        if (auto eventHandler = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(node))
        {
            auto eventSlots = eventHandler->GetEventSlotIds();
            AZStd::unordered_set<ID> path;
            return IsInEventHandlingScope(possibleEventHandler, eventSlots, {}, path);
        }

        return false;
    }

    bool Node::IsInEventHandlingScope(const ID& eventHandler, const AZStd::vector<SlotId>& eventSlots, const SlotId& connectionSlot, AZStd::unordered_set<ID>& path) const
    {
        const ID candidateNodeId = GetEntityId();

        if (candidateNodeId == eventHandler)
        {
            return AZStd::find(eventSlots.begin(), eventSlots.end(), connectionSlot) != eventSlots.end();
        }
        else if (path.find(candidateNodeId) != path.end())
        {
            return false;
        }

        // prevent loops in the search
        path.insert(candidateNodeId);

        // check all parents of the candidate for a path to the handler
        auto connectedNodes = GetConnectedNodesAndSlotsByType(SlotType::ExecutionIn);

        //  for each connected parent
        for (auto& node : connectedNodes)
        {
            // return true if that parent is the event handler we're looking for, and we're connected to an event handling execution slot
            if (node.first->IsInEventHandlingScope(eventHandler, eventSlots, node.second, path))
            {
                return true;
            }
        }

        return false;
    }

    bool Node::IsTargetInDataFlowPath(const Node* targetNode) const
    {
        AZStd::unordered_set<ID> path;
        return azrtti_cast<const PureData*>(this)
            || azrtti_cast<const PureData*>(targetNode)
            || (targetNode && IsTargetInDataFlowPath(targetNode->GetEntityId(), path));
    }

    bool Node::IsTargetInDataFlowPath(const ID& targetNodeId, AZStd::unordered_set<ID>& path) const
    {
        const ID candidateNodeId = GetEntityId();

        if (!targetNodeId.IsValid() || !candidateNodeId.IsValid())
        {
            return false;
        }

        if (candidateNodeId == targetNodeId)
        {
            // an executable path from the source to the target has been found
            return true;
        }
        else if (IsInEventHandlingScope(targetNodeId)) // targetNodeId is handler, and this node resides in that event handlers event execution slots
        {
            // this node pushes data into handled event as results for that event
            return true;
        }
        else if (path.find(candidateNodeId) != path.end())
        {
            // a loop has been encountered, without yielding a path
            return false;
        }

        // If we are the first node in the chain, we want to explore our latent connections
        bool exploreLatentConnections = path.empty();

        // prevent loops in the search
        path.insert(candidateNodeId);

        // check all children of the candidate for a path to the target
        auto connectedNodes = GetConnectedNodesByType(SlotType::ExecutionOut);
        //  for each connected child
        for (auto& node : connectedNodes)
        {
            // return true if that child is in the data flow path of target node
            if (node->IsTargetInDataFlowPath(targetNodeId, path))
            {
                return true;
            }
        }

        // Check if our data is coming from the output of a latent node, which is a valid use case
        if (exploreLatentConnections)
        {
            auto latentNodes = GetConnectedNodesByType(SlotType::LatentOut);

            for (auto& node : latentNodes)
            {
                if (node->IsTargetInDataFlowPath(targetNodeId, path))
                {
                    return true;
                }
            }
        }

        return false;
    }

    void Node::RefreshInput()
    {
        for (const auto& slotID : m_possiblyStaleInput)
        {
            SetToDefaultValueOfType(slotID);
        }
    }

    void Node::SetToDefaultValueOfType(const SlotId& slotID)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::SetDefault");

        if (auto input = ModInput(slotID))
        {
            input->SetToDefaultValueOfType();
        }
    }

    SlotDataMap Node::CreateInputMap() const
    {
        SlotDataMap map;
        
        for (auto& slot : m_slots)
        {
            if (slot.GetType() == SlotType::DataIn)
            {
                if (auto* datum = GetActiveVariableDatum(slot.GetId()))
                {
                    NamedSlotId namedSlotId(slot.GetId(), slot.GetName());

                    if (!datum->GetData().IS_A(ScriptCanvas::Data::Type::EntityID()))
                    {
                        map.emplace(namedSlotId, DatumValue::Create(*datum));
                    }
                    else
                    {
                        const AZ::EntityId* entityId = datum->GetData().GetAs<AZ::EntityId>();
                        map.emplace(namedSlotId, DatumValue::Create(VariableDatum(Datum(AZ::NamedEntityId((*entityId))))));
                    }
                }
            }
        }

        return map;
    }

    SlotDataMap Node::CreateOutputMap() const
    {
        return SlotDataMap();
    }

    AZStd::string Node::CreateInputMapString(const SlotDataMap& map) const
    {
        AZStd::string result;
                
        for (auto& iter : map)
        {
            if (auto slot = GetSlot(iter.first))
            {
                result += slot->GetName();
            }
            else
            {
                result += iter.first.ToString();
            }
            
            result += ": ";
            result += iter.second.m_value.GetData().ToString();
            result += ", ";
        }

        return result;
    }

    bool Node::IsNodeType(const NodeTypeIdentifier& nodeIdentifier) const
    {
        return nodeIdentifier == GetNodeType();
    }

    NodeTypeIdentifier Node::GetNodeType() const
    {
        return m_nodeType;
    }

    NodeTypeIdentifier Node::GetOutputNodeType(const SlotId& slotId) const
    {
        AZ_UNUSED(slotId);
        return GetNodeType();
    }

    NodeTypeIdentifier Node::GetInputNodeType(const SlotId& slotId) const
    {
        AZ_UNUSED(slotId);
        return GetNodeType();
    }

    NamedEndpoint Node::CreateNamedEndpoint(AZ::EntityId editorNodeId, SlotId slotId) const
    {
        auto slot = GetSlot(slotId);
        return NamedEndpoint(editorNodeId, GetNodeName(), slotId, slot ? slot->GetName() : "");
    }

    Signal Node::CreateNodeInputSignal(const SlotId& slotId) const
    {
        AZ::EntityId assetNodeId{};
        RuntimeRequestBus::EventResult(assetNodeId, m_executionUniqueId, &RuntimeRequests::FindAssetNodeIdByRuntimeNodeId, GetEntityId());
        return Signal(CreateGraphInfo(m_executionUniqueId, GetGraphIdentifier()), GetInputNodeType(slotId), CreateNamedEndpoint(assetNodeId, slotId), CreateInputMap());
    }

    Signal Node::CreateNodeOutputSignal(const SlotId& slotId) const
    {
        AZ::EntityId assetNodeId{};
        RuntimeRequestBus::EventResult(assetNodeId, m_executionUniqueId, &RuntimeRequests::FindAssetNodeIdByRuntimeNodeId, GetEntityId());
        return Signal(CreateGraphInfo(m_executionUniqueId, GetGraphIdentifier()), GetOutputNodeType(slotId), CreateNamedEndpoint(assetNodeId, slotId), CreateOutputMap());
    }

    OutputDataSignal Node::CreateNodeOutputDataSignal(const SlotId& slotId, const Datum& datum) const
    {
        AZ::EntityId assetNodeId{};
        RuntimeRequestBus::EventResult(assetNodeId, m_executionUniqueId, &RuntimeRequests::FindAssetNodeIdByRuntimeNodeId, GetEntityId());
        return OutputDataSignal(CreateGraphInfo(m_executionUniqueId, GetGraphIdentifier()), GetOutputNodeType(slotId), CreateNamedEndpoint(assetNodeId, slotId), DatumValue(VariableDatum(datum)));
    }

    NodeStateChange Node::CreateNodeStateUpdate() const
    {
        return NodeStateChange();
    }

    VariableChange Node::CreateVariableChange(const VariableDatumBase& variable) const
    {
        return VariableChange(CreateGraphInfo(m_executionUniqueId, GetGraphIdentifier()), CreateDatumValue(m_executionUniqueId, variable));
    }

    void Node::SignalInput(const SlotId& slotId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);
        SC_EXECUTION_TRACE_SIGNAL_INPUT((*this), (InputSignal(CreateNodeInputSignal(slotId))));
        
        {
            AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::%s::SignalInput", GetNodeName().c_str());
            OnInputSignal(slotId);
        }
        
        RefreshInput();
        SCRIPTCANVAS_HANDLE_ERROR((*this));
    }

    void Node::SignalOutput(const SlotId& slotId, ExecuteMode mode)
    {
        SCRIPTCANVAS_RETURN_IF_ERROR_STATE((*this));

        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::SignalOutput");

        bool executionCheckRequired(false);

        if (mode == ExecuteMode::UntilNodeIsFoundInStack)
        {
            ExecutionRequestBus::Event(m_executionUniqueId, &ExecutionRequests::AddToExecutionStack, *this, SlotId{});
        }
        
        if (slotId.IsValid())
        {
            if (m_slotIdMap.count(slotId) != 0)
            {
                AZStd::vector<Endpoint> connectedEndpoints;
                RuntimeRequestBus::EventResult(connectedEndpoints, m_executionUniqueId, &RuntimeRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slotId });
                for (const Endpoint& endpoint : connectedEndpoints)
                {
                    Slot* slot = nullptr;
                    Node* connectedNode{};
                    RuntimeRequestBus::EventResult(connectedNode, m_executionUniqueId, &RuntimeRequests::FindNode, endpoint.GetNodeId());
                    if (connectedNode)
                    {
                        const auto& connectedSlotId = endpoint.GetSlotId();
                        slot = connectedNode->GetSlot(connectedSlotId);
                        ExecutionRequestBus::Event(m_executionUniqueId, &ExecutionRequests::AddToExecutionStack, *connectedNode, connectedSlotId);
                        executionCheckRequired = true;
                        SC_EXECUTION_TRACE_SIGNAL_OUTPUT( (*this), (OutputSignal(CreateNodeOutputSignal(slotId))) );
                    }
                    else
                    {
                        SCRIPTCANVAS_REPORT_ERROR((*this), "Out slot connected, but no connected node found. Node: %s, Slot: %s", GetNodeName().c_str(), slotId.m_id.ToString<AZStd::string>().data());
                    }
                }
            }
            else
            {
                AZ_Warning("Script Canvas", false, "Node does not have the output slot that was signaled. Node: %s Slot: %s", RTTI_GetTypeName(), slotId.m_id.ToString<AZStd::string>().data());
            }
        }

        if (executionCheckRequired || mode == ExecuteMode::UntilNodeIsFoundInStack)
        {
            if (mode == ExecuteMode::Normal)
            {
                ExecutionRequestBus::Event(m_executionUniqueId, &ExecutionRequests::Execute);
            }
            else
            {
                ExecutionRequestBus::Event(m_executionUniqueId, &ExecutionRequests::ExecuteUntilNodeIsTopOfStack, *this);
            }
        }
    }

    bool Node::SlotAcceptsType(const SlotId& slotID, const Data::Type& type) const
    {
        if (auto slot = GetSlot(slotID))
        {
            if (slot->GetType() == SlotType::DataIn
                || slot->GetType() == SlotType::DataOut)
            {
                return slot->IsTypeMatchFor(type);
            }
        }

        AZ_Error("ScriptCanvas", false, "SlotID not found in node");
        return false;
    }

    Data::Type Node::GetSlotDataType(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetSlotDataType");

        const auto* slot = GetSlot(slotId);

        if (slot && slot->HasDisplayType())
        {
            return slot->GetDisplayType();
        }

        auto slotIdVarInfoIt = m_slotIdVarInfoMap.find(slotId);
        if (slotIdVarInfoIt != m_slotIdVarInfoMap.end())
        {
            if (auto varInput = GetActiveVariableDatum(slotId))
            {
                return varInput->GetData().GetType();
            }
            return slotIdVarInfoIt->second.m_dataType;
        }

        return Data::Type::Invalid();
    }

    VariableId Node::GetSlotVariableId(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetSlotVariableId");

        auto slotIdVarInfoIt = m_slotIdVarInfoMap.find(slotId);
        return slotIdVarInfoIt != m_slotIdVarInfoMap.end() ? slotIdVarInfoIt->second.m_currentVariableId : VariableId();
    }

    void Node::SetSlotVariableId(const SlotId& slotId, const VariableId& variableId)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::SetSlotVariableId");

        auto slotIdVarInfoIt = m_slotIdVarInfoMap.find(slotId);
        if(slotIdVarInfoIt != m_slotIdVarInfoMap.end() && slotIdVarInfoIt->second.m_currentVariableId != variableId)
        {
            VariableId oldVariableId = slotIdVarInfoIt->second.m_currentVariableId;
            slotIdVarInfoIt->second.m_currentVariableId = variableId;
            NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotActiveVariableChanged, slotId, oldVariableId, variableId);
        }
    }

    void Node::ResetSlotVariableId(const SlotId& slotId)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::ResetSlotVariableId");

        auto slotIdVarInfoIt = m_slotIdVarInfoMap.find(slotId);
        if (slotIdVarInfoIt != m_slotIdVarInfoMap.end())
        {
            SetSlotVariableId(slotId, slotIdVarInfoIt->second.m_ownedVariableId);
        }
    }

    bool Node::IsOnPureDataThread(const SlotId& slotId) const
    {
        const auto slot = GetSlot(slotId);
        if (slot && slot->GetType() == SlotType::DataIn)
        {
            const auto& nodes = GetConnectedNodes(*slot);
            AZStd::unordered_set<ID> path;
            path.insert(GetEntityId());

            for (auto& node : nodes)
            {
                if (node.first->IsOnPureDataThreadHelper(path))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool Node::IsOnPureDataThreadHelper(AZStd::unordered_set<ID>& path) const
    {
        if (path.find(GetEntityId()) != path.end())
        {
            return false;
        }

        path.insert(GetEntityId());

        if (IsEventHandler())
        {   // data could have been routed back is the input to an event handler with a return value
            return false;
        }
        else if (IsPureData())
        {
            return true;
        }
        else
        {
            const auto& nodes = GetConnectedNodesByType(SlotType::DataIn);

            for (auto& node : nodes)
            {
                if (node->IsOnPureDataThreadHelper(path))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool Node::DynamicSlotAcceptsType(const SlotId& slotID, const Data::Type& type, DynamicTypeArity arity, const Slot& outputSlot, const AZStd::vector<Slot*>& inputSlots) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::DynamicSlotAcceptsType");

        if (!type.IsValid())
        {
            // this could be handled, technically, but might be more confusing than anything else)
            return false;
        }

        auto iter = AZStd::find_if(inputSlots.begin(), inputSlots.end(), [&slotID](Slot* slot) { return slot->GetId() == slotID; });

        if (iter != inputSlots.end())
        {
            for (Slot* slot : inputSlots)
            {
                if (!DynamicSlotInputAcceptsType(slotID, type, arity, *slot))
                {
                    return false;
                }
            }
        }
        else if (slotID == outputSlot.GetId())
        {
            for (Slot* inputSlot : inputSlots)
            {
                auto inputs = GetConnectedNodes(*inputSlot);

                for (const auto& input : inputs)
                {
                    if (!input.first->GetSlotDataType(input.second).IS_A(type))
                    {
                        // the new output doesn't match the previous inputs
                        return false;
                    }
                }
            }
        }

        const AZStd::vector<AZStd::pair<const Node*, const SlotId>> outputs = GetConnectedNodes(outputSlot);

        // check new input/output against previously existing output types
        for (const auto& output : outputs)
        {
            if (!type.IS_A(output.first->GetSlotDataType(output.second)))
            {
                return false;
            }
        }

        return true;
    }

    bool Node::DynamicSlotInputAcceptsType(const SlotId& slotID, const Data::Type& type, DynamicTypeArity arity, const Slot& inputSlot) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::DynamicSlotInputAcceptsType");

        const AZStd::vector<AZStd::pair<const Node*, const SlotId>> inputs = GetConnectedNodes(inputSlot);

        if (arity == DynamicTypeArity::Single && !inputs.empty())
        {
            // this input can only be connected to one source
            return false;
        }

        for (const auto& iter : inputs)
        {
            const auto& previousInputType = iter.first->GetSlotDataType(iter.second);
            if (!(previousInputType.IS_A(type) || type.IS_A(previousInputType)))
            {
                // no acceptable type relationship
                return false;
            }
        }

        return true;
    }

    SlotId Node::GetSlotId(AZStd::string_view slotName) const
    {
        auto slotNameIter = m_slotNameMap.find(slotName);
        return slotNameIter != m_slotNameMap.end() ? slotNameIter->second->GetId() : SlotId{};
    }

    AZStd::vector<const Slot*> Node::GetSlotsByType(SlotType slotType) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetSlotsByType");

        AZStd::vector<const Slot*> slots;

        for (const auto& slot : m_slots)
        {
            if (slot.GetType() == slotType)
            {
                slots.emplace_back(&slot);
            }
        }

        return slots;
    }

    SlotId Node::GetSlotIdByType(AZStd::string_view slotName, SlotType slotType) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetSlotIdByType");

        auto slotNameRange = m_slotNameMap.equal_range(slotName);
        auto nameSlotIt = AZStd::find_if(slotNameRange.first, slotNameRange.second, [slotType](const AZStd::pair<AZStd::string, SlotIterator>& nameSlotPair)
        {
            return slotType == nameSlotPair.second->GetType();
        });

        return nameSlotIt != slotNameRange.second ? nameSlotIt->second->GetId() : SlotId{};
    }

    AZStd::vector<SlotId> Node::GetSlotIds(AZStd::string_view slotName) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetSlotIds");

        auto nameSlotRange = m_slotNameMap.equal_range(slotName);
        AZStd::vector<SlotId> result;
        for (auto nameSlotIt = nameSlotRange.first; nameSlotIt != nameSlotRange.second; ++nameSlotIt)
        {
            result.push_back(nameSlotIt->second->GetId());
        }
        return result;
    }

    Slot* Node::GetSlot(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetSlot");

        if (slotId.IsValid())
        {
            auto findSlotOutcome = FindSlotIterator(slotId);
            if (findSlotOutcome)
            {
                return &*findSlotOutcome.GetValue();
            }

            AZ_Warning("Script Canvas", false, "%s", findSlotOutcome.GetError().data());
        }

        return {};
    }

    size_t Node::GetSlotIndex(const SlotId& slotId) const
    {
        size_t retVal = 0;
        auto slotIter = m_slots.begin();

        while (slotIter != m_slots.end())
        {
            if (slotIter->GetId() == slotId)
            {
                break;
            }

            retVal++;
            slotIter++;
        }

        if (slotIter == m_slots.end())
        {
            retVal = -1;
        }

        return retVal;
    }

    const Slot* Node::GetSlotByIndex(size_t index) const
    {
        return index < m_slots.size() ? &*AZStd::next(m_slots.begin(), index) : nullptr;
    }

    AZStd::vector<const Slot*> Node::GetAllSlots() const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetAllSlots");

        const SlotList& slots = GetSlots();

        AZStd::vector<const Slot*> retVal;
        retVal.reserve(slots.size());

        for (const auto& slot : slots)
        {
            retVal.push_back(&slot);
        }

        return retVal;
    }

    bool Node::SlotExists(AZStd::string_view name, SlotType type) const
    {
        SlotId unused;
        return SlotExists(name, type, unused);
    }

    bool Node::SlotExists(AZStd::string_view name, SlotType type, SlotId& out) const
    {
        out = GetSlotIdByType(name, type);
        return out.IsValid();
    }

    SlotId Node::InsertSlot(AZ::s64 index, const SlotConfiguration& slotConfig)
    {
        SlotIterator addSlotIter = m_slots.end();
        if (InsertSlotInternal(index, slotConfig, addSlotIter))
        {
            NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotAdded, addSlotIter->GetId());
        }
        return addSlotIter != m_slots.end() ? addSlotIter->GetId() : SlotId{};
    }
    
    SlotId Node::AddSlot(const SlotConfiguration& slotConfiguration)
    {
        return InsertSlot(-1, slotConfiguration);
    }

    SlotId Node::AddDataSlot(const DataSlotConfiguration& slotConfiguration)
    {
        SlotIterator slotIterOut = m_slots.end();
        if (InsertDataTypeSlotInternal(-1, slotConfiguration, slotIterOut))
        {
            NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotAdded, slotIterOut->GetId());
            return slotIterOut->GetId();
        }

        return slotIterOut != m_slots.end() ? slotIterOut->GetId() : SlotId{};        
    }

    SlotId Node::InsertInputDatumSlot(AZ::s64 insertIndex, const DataSlotConfiguration& slotConfig, Datum&& initialDatum)
    {
        SlotIterator slotIter = m_slots.end();
        VariableIterator varIter = m_varDatums.end();
        if (InsertInputDatumSlotInternal(insertIndex, slotConfig, AZStd::move(initialDatum), slotIter, varIter))
        {
            NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotAdded, slotIter->GetId());
        }

        return slotIter != m_slots.end() ? slotIter->GetId() : SlotId{};
    }

    SlotId Node::AddInputDatumSlot(const DataSlotConfiguration& slotConfig)
    {
        return AddInputDatumSlot(slotConfig, Datum(slotConfig.m_dataType, Datum::eOriginality::Original, nullptr, AZ::Uuid::CreateNull()));
    }

    SlotId Node::AddInputDatumSlot(const DataSlotConfiguration& slotConfig, Datum&& initialDatum)
    {
        return InsertInputDatumSlot(-1, slotConfig, AZStd::move(initialDatum));
    }

    SlotId Node::AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, const void* source, Datum::eOriginality originality, bool addUniqueSlotByNameAndType)
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = name;
        dataSlotConfiguration.m_toolTip = toolTip;

        dataSlotConfiguration.m_slotType = SlotType::DataIn;

        dataSlotConfiguration.m_addUniqueSlotByNameAndType = addUniqueSlotByNameAndType;

        return AddInputDatumSlot(dataSlotConfiguration, Datum(type, originality, source, AZ::Uuid::CreateNull()));
    }

    SlotId Node::AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, Datum::eOriginality originality, bool addUniqueSlotByNameAndType)
    {
        return AddInputDatumSlot(name, toolTip, type, nullptr, originality, addUniqueSlotByNameAndType);
    }

    SlotId Node::AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZ::BehaviorParameter& typeDesc, Datum::eOriginality originality, bool addUniqueSlotByNameAndType)
    {
        auto dataRegistry = GetDataRegistry();
        Data::Type scType = !AZ::BehaviorContextHelper::IsStringParameter(typeDesc) ? Data::FromAZType(typeDesc.m_typeId) : Data::Type::String();
        auto typeIter = dataRegistry->m_creatableTypes.find(scType);
        
        if (typeIter != dataRegistry->m_creatableTypes.end())
        {
            return AddInputDatumSlot(name, toolTip, scType, nullptr, originality, addUniqueSlotByNameAndType);
        }

        AZ_Error("Script Canvas", false, "BehaviorParameter %s with type %s is not creatible type in ScriptCanvas", typeDesc.m_name, typeDesc.m_typeId.ToString<AZStd::string>().data());
        return {};
    }

    SlotId Node::AddInputDatumDynamicTypedSlot(AZStd::string_view name, AZStd::string_view toolTip, bool addUniqueSlotByNameAndType)
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = name;
        dataSlotConfiguration.m_toolTip = toolTip;

        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_addUniqueSlotByNameAndType = addUniqueSlotByNameAndType;

        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

        dataSlotConfiguration.m_contractDescs = { { []() { return aznew DynamicTypeContract(); } } };        

        return AddInputDatumSlot(dataSlotConfiguration, Datum());
    }

    SlotId Node::AddInputDatumDynamicContainerTypedSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZStd::vector<ContractDescriptor>& contractsIn, bool addUniqueSlotByNameAndType)
    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = name;
        slotConfiguration.m_toolTip = toolTip;

        for (auto&& contract : contractsIn)
        {
            slotConfiguration.m_contractDescs.emplace_back(AZStd::move(contract));
        }

        slotConfiguration.m_slotType = SlotType::DataIn;
        slotConfiguration.m_addUniqueSlotByNameAndType = addUniqueSlotByNameAndType;

        slotConfiguration.m_dynamicDataType = DynamicDataType::Container;

        return AddInputDatumSlot(slotConfiguration, Datum());
    }

    SlotId Node::AddInputDatumOverloadedSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZStd::vector<ContractDescriptor>& contractsIn, bool addUniqueSlotByNameAndType)
    {
        DataSlotConfiguration dataSlotConfiguration;

        dataSlotConfiguration.m_name = name;
        dataSlotConfiguration.m_toolTip = toolTip;

        dataSlotConfiguration.m_slotType = SlotType::DataIn;
        dataSlotConfiguration.m_contractDescs = contractsIn;
        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

        dataSlotConfiguration.m_addUniqueSlotByNameAndType = addUniqueSlotByNameAndType;

        return AddInputDatumSlot(dataSlotConfiguration, Datum());
    }

    SlotId Node::AddInputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, InputTypeContract contractType, bool addUniqueSlotByNameAndType)
    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_slotType = SlotType::DataIn;
        slotConfiguration.m_name = name;
        slotConfiguration.m_dataType = type;

        slotConfiguration.m_addUniqueSlotByNameAndType = addUniqueSlotByNameAndType;

        if (contractType == InputTypeContract::CustomType)
        {
            slotConfiguration.m_contractDescs.emplace_back([type]() { return aznew RestrictedTypeContract{ type }; });
        }

        SlotIterator slotIterOut = m_slots.end();
        if (InsertDataTypeSlotInternal(-1, slotConfiguration, slotIterOut))
        {
            NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotAdded, slotIterOut->GetId());
        }

        return slotIterOut != m_slots.end() ? slotIterOut->GetId() : SlotId{};
    }

    SlotId Node::AddInputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZ::BehaviorParameter& typeDesc, InputTypeContract contractType, bool addUniqueSlotByNameAndType)
    {
        return AddInputTypeSlot(name, toolTip, AZ::BehaviorContextHelper::IsStringParameter(typeDesc) ? Data::Type::String() : Data::FromAZTypeChecked(typeDesc.m_typeId), contractType, addUniqueSlotByNameAndType);
    }

    SlotId Node::AddOutputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, OutputStorage, bool addUniqueSlotByNameAndType)
    {
        DataSlotConfiguration slotConfiguration;
        slotConfiguration.m_name = name;
        slotConfiguration.m_toolTip = toolTip;
        slotConfiguration.m_slotType = SlotType::DataOut;
        slotConfiguration.m_addUniqueSlotByNameAndType = addUniqueSlotByNameAndType;

        slotConfiguration.m_dataType = type;

        return AddDataSlot(slotConfiguration);
    }

    AZ::Outcome<void, AZStd::string> Node::InsertInputDatumSlotInternal(AZ::s64 index, const DataSlotConfiguration& slotConfig, Datum&& initialDatum, SlotIterator& slotIterOut, VariableIterator& varIterOut)
    {
        auto insertSlotOutcome = InsertSlotInternal(index, slotConfig, slotIterOut);
        if (insertSlotOutcome)
        {
            initialDatum.SetLabel(slotConfig.m_name);
            initialDatum.SetNotificationsTarget(GetEntityId());
            varIterOut = m_varDatums.emplace(m_varDatums.end(), AZStd::move(initialDatum));
            m_varIdMap.emplace(varIterOut->GetId(), varIterOut);
            m_slotIdVarInfoMap[slotIterOut->GetId()] = VariableInfo(varIterOut->GetId());
        }

        return insertSlotOutcome;
    }

    AZ::Outcome<void, AZStd::string> Node::InsertDataTypeSlotInternal(AZ::s64 index, const DataSlotConfiguration& slotConfig, SlotIterator& slotIterOut)
    {
        auto insertSlotOutcome = InsertSlotInternal(-1, slotConfig, slotIterOut);
        if (insertSlotOutcome)
        {
            m_slotIdVarInfoMap[slotIterOut->GetId()] = VariableInfo(slotConfig.m_dataType);
        }

        return insertSlotOutcome;
    }

    AZ::Outcome<void, AZStd::string> Node::InsertSlotInternal(AZ::s64 insertIndex, const SlotConfiguration& slotConfiguration, SlotIterator& iterOut)
    {
        if (slotConfiguration.m_name.empty())
        {
            return AZ::Failure(AZStd::string("attempting to add a slot with no name"));
        }

        if (slotConfiguration.m_slotType == SlotType::None)
        {
            return AZ::Failure(AZStd::string("Trying to add a slot with a SlotType of None"));
        }

        auto slotNameIter = m_slotNameMap.find(slotConfiguration.m_name);
        if (slotConfiguration.m_addUniqueSlotByNameAndType && slotNameIter != m_slotNameMap.end() && slotNameIter->second->GetType() == slotConfiguration.m_slotType)
        {
            iterOut = slotNameIter->second;
            return AZ::Failure(AZStd::string::format("Slot with name %s already exist", slotConfiguration.m_name.data()));
        }

        SlotIterator insertIter = (insertIndex < 0 || insertIndex >= azlossy_cast<AZ::s64>(m_slots.size())) ? m_slots.end() : AZStd::next(m_slots.begin(), insertIndex);
        iterOut = m_slots.emplace(insertIter, slotConfiguration);

        m_slotIdMap.emplace(iterOut->GetId(), iterOut);
        m_slotNameMap.emplace(iterOut->GetName(), iterOut);
        iterOut->SetNodeId(GetEntity() ? GetEntityId() : AZ::EntityId{});

        return AZ::Success();
    }

    bool Node::RemoveSlot(const SlotId& slotId, bool deleteConnections)
    {
        // If we are already removing the slot, early out with false since something else is doing the deleting.
        if (m_removingSlots.count(slotId) != 0)
        {
            return false;
        }

        auto slotIdIt = m_slotIdMap.find(slotId);
        if (slotIdIt != m_slotIdMap.end())
        {
            SlotIterator slotIt = slotIdIt->second;

            /// Disconnect connected endpoints
            if (deleteConnections)
            {
                // We want to avoid recursive calls into ourselves here(happens in the case of dynamically added slots)
                m_removingSlots.insert(slotId);
                RemoveConnectionsForSlot(slotId);
                m_removingSlots.erase(slotId);
            }

            auto slotIdVarIdIt = m_slotIdVarInfoMap.find(slotId);

            VariableId nodeVarId;
            if (slotIdVarIdIt != m_slotIdVarInfoMap.end())
            {
                nodeVarId = slotIdVarIdIt->second.m_ownedVariableId;
                auto varIdIt = m_varIdMap.find(nodeVarId);
                if (varIdIt != m_varIdMap.end())
                {
                    auto varDatumIter = varIdIt->second;
                    m_varDatums.erase(varDatumIter);
                    m_varIdMap.erase(varIdIt);
                }
                m_slotIdVarInfoMap.erase(slotIdVarIdIt);
            }

            m_slotIdMap.erase(slotIdIt);
            auto slotNameIter = AZStd::find_if(m_slotNameMap.begin(), m_slotNameMap.end(), [slotIt](const AZStd::pair<AZStd::string, SlotIterator>& nameSlotPair)
            {
                return nameSlotPair.second == slotIt;
            });
            if (slotNameIter != m_slotNameMap.end())
            {
                m_slotNameMap.erase(slotNameIter);
            }

            m_slots.erase(slotIt);
            SignalSlotRemoved(slotId);

            return true;
        }

        AZ_Warning("Script Canvas", "Cannot remove slot that does not exist! %s", slotId.m_id.ToString<AZStd::string>().c_str());
        return false;
    }

    void Node::RemoveConnectionsForSlot(const SlotId& slotId)
    {
        Graph* graph = GetGraph();

        if (graph)
        {
            Endpoint baseEndpoint{ GetEntityId(), slotId };
            for (const auto& connectedEndpoint : graph->GetConnectedEndpoints(baseEndpoint))
            {
                graph->DisconnectByEndpoint(baseEndpoint, connectedEndpoint);
            }
        }
    }

    void Node::SignalSlotRemoved(const SlotId& slotId)
    {
        NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnSlotRemoved, slotId);
    }

    AZStd::vector<Endpoint> Node::GetEndpointsByType(SlotType slotType) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetEndpointsByType");

        AZStd::vector<Endpoint> endpoints;
        for (auto& slot : m_slots)
        {
            if (slot.GetType() == slotType)
            {
                AZStd::vector<Endpoint> connectedEndpoints;
                RuntimeRequestBus::EventResult(connectedEndpoints, m_executionUniqueId, &RuntimeRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slot.GetId() });
                endpoints.insert(endpoints.end(), connectedEndpoints.begin(), connectedEndpoints.end());
            }
        }

        return endpoints;
    }

    void Node::SetGraphUniqueId(AZ::EntityId uniqueId)
    {
        m_executionUniqueId = uniqueId;

        OnGraphSet();
    }

    Graph* Node::GetGraph() const
    {
        Graph* graph{};
        GraphRequestBus::EventResult(graph, m_executionUniqueId, &GraphRequests::GetGraph);
        return graph;
    }

    NodePtrConstList Node::GetConnectedNodesByType(SlotType slotType) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetConnectedNodesByType");

        NodePtrConstList connectedNodes;

        for (const auto& endpoint : GetEndpointsByType(slotType))
        {
            Node* connectedNode{};
            RuntimeRequestBus::EventResult(connectedNode, m_executionUniqueId, &RuntimeRequests::FindNode, endpoint.GetNodeId());
            
            if (connectedNode)
            {
                connectedNodes.emplace_back(connectedNode);
            }
        }

        return connectedNodes;
    }

    AZStd::vector<AZStd::pair<const Node*, SlotId>> Node::GetConnectedNodesAndSlotsByType(SlotType slotType) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetConnectedNodesAndSlotsByType");

        AZStd::vector<AZStd::pair<const Node*, SlotId>> connectedNodes;

        for (const auto& endpoint : GetEndpointsByType(slotType))
        {
            Node* connectedNode{};
            RuntimeRequestBus::EventResult(connectedNode, m_executionUniqueId, &RuntimeRequests::FindNode, endpoint.GetNodeId());
            
            if (connectedNode)
            {
                connectedNodes.emplace_back(AZStd::make_pair(connectedNode, endpoint.GetSlotId()));
            }
        }

        return connectedNodes;
    }


    const Node* Node::GetNextExecutableNode() const
    {
        auto connectedNodes = GetConnectedNodesByType(SlotType::ExecutionOut);
        return connectedNodes.empty() ? nullptr : connectedNodes[0];
    }

    AZ::EntityId Node::GetGraphEntityId() const
    {
        AZ::EntityId graphEntityId;
        RuntimeRequestBus::EventResult(graphEntityId, m_executionUniqueId, &RuntimeRequests::GetRuntimeEntityId);
        return graphEntityId;
    }

    AZ::Data::AssetId Node::GetGraphAssetId() const
    {
        AZ::Data::AssetId assetId;
        RuntimeRequestBus::EventResult(assetId, m_executionUniqueId, &RuntimeRequests::GetAssetId);
        return assetId;
    }

    GraphIdentifier Node::GetGraphIdentifier() const
    {
        GraphIdentifier graphIdentifier;
        RuntimeRequestBus::EventResult(graphIdentifier, m_executionUniqueId, &RuntimeRequests::GetGraphIdentifier);
        return graphIdentifier;
    }

    void Node::OnDatumChanged(const Datum* datum)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::OnDatumChanged");

        auto varDatumIt = AZStd::find_if(m_varDatums.begin(), m_varDatums.end(), [datum](const VariableDatumBase& varDatum)
        {
            return &varDatum.GetData() == datum;
        });

        if (varDatumIt != m_varDatums.end())
        {
            SlotId slotId = GetSlotId(varDatumIt->GetId());
            if (slotId.IsValid())
            {
                NodeNotificationsBus::Event((GetEntity() != nullptr) ? GetEntityId() : AZ::EntityId(), &NodeNotifications::OnInputChanged, slotId);
            }
        }
    }

    const Datum* Node::GetDatumByIndex(size_t index) const
    {
        return index < m_varDatums.size() ? &AZStd::next(m_varDatums.begin(), index)->GetData() : nullptr;
    }

    const Datum* Node::GetInput(const SlotId& slotID) const
    {
        VariableDatumBase* varDatum = GetActiveVariableDatum(slotID);
        return varDatum ? &varDatum->GetData() : nullptr;
    }

    const Datum* Node::GetInput(const Node& node, const SlotId slotID)
    {
        return node.GetInput(slotID);
    }

    Datum* Node::ModDatumByIndex(size_t index)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::ModDatumByIndex");

        return index < m_varDatums.size() ? &AZStd::next(m_varDatums.begin(), index)->GetData() : nullptr;
    }

    Datum* Node::ModInput(Node& node, const SlotId slotID)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::ModInput(node, slotID)");

        return node.ModInput(slotID);
    }

    Datum* Node::ModInput(const SlotId& slotID)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::ModInput");

        return const_cast<Datum*>(GetInput(slotID));
    }

    SlotId Node::GetSlotId(const VariableId& varId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetSlotId");

        // Look up the variable id from the variables stored on the node
        auto slotIdVarInfoIter = AZStd::find_if(m_slotIdVarInfoMap.begin(), m_slotIdVarInfoMap.end(), [&varId](const AZStd::pair<SlotId, VariableInfo>& slotIdVarIdPair)
        {
            return varId == slotIdVarIdPair.second.m_ownedVariableId;
        });

        if (slotIdVarInfoIter != m_slotIdVarInfoMap.end())
        {
            return slotIdVarInfoIter->first;
        }

        // Look up the variable id from the variables stored on the GraphVariableManagerComponent
        slotIdVarInfoIter = AZStd::find_if(m_slotIdVarInfoMap.begin(), m_slotIdVarInfoMap.end(), [&varId](const AZStd::pair<SlotId, VariableInfo>& slotIdVarIdPair)
        {
            return varId == slotIdVarIdPair.second.m_currentVariableId;
        });

        return slotIdVarInfoIter != m_slotIdVarInfoMap.end() ? slotIdVarInfoIter->first : SlotId{};
    }

    VariableId Node::GetVariableId(const SlotId& slotId) const
    {
        auto slotIdVarInfoIter = m_slotIdVarInfoMap.find(slotId);
        return slotIdVarInfoIter != m_slotIdVarInfoMap.end() ? slotIdVarInfoIter->second.m_currentVariableId : VariableId{};
    }

    Slot* Node::GetSlot(const VariableId& varId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetSlot");

        SlotId slotId = GetSlotId(varId);
        if (!slotId.IsValid())
        {
            return nullptr;
        }

        auto slotIdIter = m_slotIdMap.find(slotId);
        return slotIdIter != m_slotIdMap.end() ? &*slotIdIter->second : nullptr;
    }

    VariableDatumBase* Node::GetActiveVariableDatum(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetActiveVariableDatum");

        VariableId varId = GetVariableId(slotId);
        if (!varId.IsValid())
        {
            return nullptr;
        }

        auto varIdIter = m_varIdMap.find(varId);
        if (varIdIter != m_varIdMap.end())
        {
            return &*varIdIter->second;
        }

        VariableDatum* varDatum{};
        VariableRequestBus::EventResult(varDatum, varId, &VariableRequests::GetVariableDatum);
        return varDatum;
    }

    auto Node::FindSlotIterator(const SlotId& slotId) const -> AZ::Outcome<SlotIterator, AZStd::string>
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::FindSlotIterator");

        auto slotIdIter = m_slotIdMap.find(slotId);
        if (slotIdIter != m_slotIdMap.end())
        {
            SlotIterator slotIterOut = slotIdIter->second;
            return AZ::Success(slotIterOut);
        }

        AZStd::string errorMsg;
        if (GetEntity())
        {
            errorMsg = AZStd::string::format("Node %s does not have the specified slot: %s", GetEntity()->GetName().data(), slotId.ToString().c_str());
        }
        return AZ::Failure(errorMsg);
    }

    auto Node::FindSlotIterator(const VariableId& varId) const -> AZ::Outcome<SlotIterator, AZStd::string>
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::FindSlotIterator");

        // Look up the variable id from the variables stored on the node
        auto slotId = GetSlotId(varId);
        if (slotId.IsValid())
        {
            return FindSlotIterator(slotId);
        }

        return AZ::Failure(AZStd::string::format("Unable to Find Slot Id associated with Variable Id %s", varId.ToString().data()));
    }

    auto Node::FindVariableIterator(const SlotId& slotId) const -> AZ::Outcome<VariableIterator, AZStd::string>
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::FindVariableIterator");

        auto slotIdVarInfoIter = m_slotIdVarInfoMap.find(slotId);

        if (slotIdVarInfoIter != m_slotIdVarInfoMap.end())
        {
            return FindVariableIterator(slotIdVarInfoIter->second.m_currentVariableId);
        }

        return AZ::Failure(AZStd::string::format("Unable to Find Variable Id associated with Slot Id %s", slotId.ToString().data()));
    }

    auto Node::FindVariableIterator(const VariableId& varId) const -> AZ::Outcome<VariableIterator, AZStd::string>
    {
        auto varIdIter = m_varIdMap.find(varId);
        if (varIdIter != m_varIdMap.end())
        {
            VariableIterator varIterOut = varIdIter->second;
            return AZ::Success(varIterOut);
        }

        AZStd::string errorMsg;
        if (GetEntity())
        {
            errorMsg = AZStd::string::format("Node %s does not have the variable datum: %s", GetEntity()->GetName().data(), varId.ToString().c_str());
        }
        return AZ::Failure(errorMsg);
    }

    AZ::Outcome<AZ::s64, AZStd::string> Node::FindSlotIndex(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::FindSlotIndex");

        auto slotIdIter = m_slotIdMap.find(slotId);
        if (slotIdIter != m_slotIdMap.end())
        {
            AZ::s64 slotIndex = AZStd::distance(m_slots.begin(), SlotList::const_iterator(slotIdIter->second));
            return AZ::Success(slotIndex);
        }

        return AZ::Failure(AZStd::string("Unable to find Slot Id in SlotIdMap"));
    }

    AZ::Outcome<AZ::s64, AZStd::string> Node::FindVariableIndex(const VariableId& varId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::FindVariableIndex");

        auto varIdIter = m_varIdMap.find(varId);
        if (varIdIter != m_varIdMap.end())
        {
            AZ::s64 varIndex = AZStd::distance(m_varDatums.begin(), VariableList::const_iterator(varIdIter->second));
            return AZ::Success(varIndex);
        }

        return AZ::Failure(AZStd::string("Unable to find Variable Id in VariableIdMap"));
    }

    bool Node::IsConnected(const Slot& slot) const
    {
        return IsConnected(slot.GetId());
    }

    bool Node::IsConnected(const SlotId& slotId) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::IsConnected");

        bool isConnected = false;
        RuntimeRequestBus::EventResult(isConnected, m_executionUniqueId, &RuntimeRequests::IsEndpointConnected, Endpoint{ GetEntityId(), slotId });

        return isConnected;
    }

    bool Node::IsEventHandler() const
    {
        return false;
    }
    
    bool Node::IsPureData() const
    {
        for (const auto& slot : m_slots)
        {
            if (SlotUtils::IsExecutionOut(slot.GetType()))
            {
                return false;
            }
        }

        return true;
    }
    
    AZStd::vector<AZStd::pair<const Node*, const SlotId>> Node::GetConnectedNodes(const Slot& slot) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::GetConnectedNodes");

        AZStd::vector<AZStd::pair<const Node*, const SlotId>> connectedNodes;
        AZStd::vector<Endpoint> connectedEndpoints;
        RuntimeRequestBus::EventResult(connectedEndpoints, m_executionUniqueId, &RuntimeRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slot.GetId() });

        for (const Endpoint& endpoint : connectedEndpoints)
        {
            const GraphData* graphData{};
            RuntimeRequestBus::EventResult(graphData, m_executionUniqueId, &RuntimeRequests::GetGraphDataConst);
            if (!graphData)
            {
                AZ_Error("Script Canvas", false, "Unable to find GraphData from the RuntimeRequestBus using id %s", m_executionUniqueId.ToString().data());
                continue;
            }
            auto nodeEntityIt = AZStd::find_if(graphData->m_nodes.begin(), graphData->m_nodes.end(), [&endpoint](const AZ::Entity* node) { return node->GetId() == endpoint.GetNodeId(); });
            if (nodeEntityIt == graphData->m_nodes.end())
            {
                AZStd::string assetName;
                RuntimeRequestBus::EventResult(assetName, m_executionUniqueId, &RuntimeRequests::GetAssetName);

                AZ_Error("Script Canvas", false, "Node Entity with id %s cannot be found on graph '%s'", endpoint.GetNodeId().ToString().data(), assetName.data());
                continue;
            }

            Node* connectedNode = AZ::EntityUtils::FindFirstDerivedComponent<Node>(*nodeEntityIt);
            if (connectedNode)
            {
                connectedNodes.emplace_back(connectedNode, endpoint.GetSlotId());
            }
            else
            {
                AZStd::string assetName;
                RuntimeRequestBus::EventResult(assetName, m_executionUniqueId, &RuntimeRequests::GetAssetName);

                AZ_Error("Script Canvas", false, "Unable to find node with name %s (id: %s) in the graph '%s'. Most likely the node was serialized with a type that is no longer reflected",
                    (*nodeEntityIt)->GetName().data(), (*nodeEntityIt)->GetId().ToString().data(), assetName.data());
            }
        }
        return connectedNodes;
    }

    AZStd::vector<AZStd::pair<Node*, const SlotId>> Node::ModConnectedNodes(const Slot& slot) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::ModConnectedNodes");
        AZStd::vector<AZStd::pair<Node*, const SlotId>> connectedNodes;
        ModConnectedNodes(slot, connectedNodes);
        return connectedNodes;
    }

    void Node::ModConnectedNodes(const Slot& slot, AZStd::vector<AZStd::pair<Node*, const SlotId>>& connectedNodes) const
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::ModConnectedNodes2");
        AZStd::vector<Endpoint> connectedEndpoints;
        RuntimeRequestBus::EventResult(connectedEndpoints, m_executionUniqueId, &RuntimeRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slot.GetId() });

        for (const Endpoint& endpoint : connectedEndpoints)
        {
            const GraphData* graphData{};
            RuntimeRequestBus::EventResult(graphData, m_executionUniqueId, &RuntimeRequests::GetGraphDataConst);
            if (!graphData)
            {
                AZ_Error("Script Canvas", false, "Unable to find GraphData from the RuntimeRequestBus using id %s", m_executionUniqueId.ToString().data());
                continue;
            }
            auto nodeEntityIt = AZStd::find_if(graphData->m_nodes.begin(), graphData->m_nodes.end(), [&endpoint](const AZ::Entity* node) { return node->GetId() == endpoint.GetNodeId(); });
            if (nodeEntityIt == graphData->m_nodes.end())
            {
                AZStd::string assetName;
                RuntimeRequestBus::EventResult(assetName, m_executionUniqueId, &RuntimeRequests::GetAssetName);
                
                AZ_Error("Script Canvas", false, "Node Entity with id %s cannot be found on graph '%s'", endpoint.GetNodeId().ToString().data(), assetName.data());
                continue;
            }

            Node* connectedNode = AZ::EntityUtils::FindFirstDerivedComponent<Node>(*nodeEntityIt);
            if (connectedNode)
            {
                connectedNodes.emplace_back(connectedNode, endpoint.GetSlotId());
            }
            else
            {
                AZStd::string assetName;
                RuntimeRequestBus::EventResult(assetName, m_executionUniqueId, &RuntimeRequests::GetAssetName);

                AZ_Error("Script Canvas", false, "Unable to find node with name %s (id: %s) in the graph '%s'. Most likely the node was serialized with a type that is no longer reflected",
                    (*nodeEntityIt)->GetName().data(), (*nodeEntityIt)->GetId().ToString().data(), assetName.data());
            }
        }
    }

    bool Node::HasConnectedNodes(const Slot& slot) const
    {
        bool isConnected = false;
        RuntimeRequestBus::EventResult(isConnected, m_executionUniqueId, &RuntimeRequests::IsEndpointConnected, Endpoint{ GetEntityId(), slot.GetId() });

        return isConnected;
    }


    void Node::OnInputChanged(Node& node, const Datum& input, const SlotId& slotID)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);
        node.OnInputChanged(input, slotID);
    }

    void Node::PushOutput(const Datum& output, const Slot& slot) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

        SC_EXECUTION_TRACE_SIGNAL_DATA_OUTPUT((*this), CreateNodeOutputDataSignal(slot.GetId(), output));

        ForEachConnectedNode
            ( slot
            , [&output](Node& node, const SlotId& slotID)
            {
                node.SetInput(output, slotID);
            });
    }

    void Node::ForEachConnectedNode(const Slot& slot, AZStd::function<void(Node&, const SlotId&)> callable) const
    {
        auto connectedNodes = ModConnectedNodes(slot);
        for (auto& nodeSlotPair : connectedNodes)
        {
            if (nodeSlotPair.first)
            {
                callable(*nodeSlotPair.first, nodeSlotPair.second);
            }
        }
    }

    void Node::SetInput(const Datum& newInput, const SlotId& slotId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);
        // Only the datum value stored within this node can be modified.
        // This will not modify variable that resides within the variable manager
        VariableDatumBase* varDatum{};
        auto foundSlotVarInfoIt = m_slotIdVarInfoMap.find(slotId);
        if (foundSlotVarInfoIt != m_slotIdVarInfoMap.end())
        {
            auto varIdIter = m_varIdMap.find(foundSlotVarInfoIt->second.m_ownedVariableId);
            if (varIdIter != m_varIdMap.end())
            {
                varDatum = &*varIdIter->second;
                Datum& setDatum = varDatum->GetData();
                WriteInput(setDatum, newInput);
                OnInputChanged(setDatum, slotId);
            }
        }
    }

    void Node::SetInput(Datum&& newInput, const SlotId& slotId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);
        // Only the datum value stored within this node can be modified.
        // This will not modify variable that resides within the variable manager
        VariableDatumBase* varDatum{};
        auto foundSlotVarInfoIt = m_slotIdVarInfoMap.find(slotId);
        if (foundSlotVarInfoIt != m_slotIdVarInfoMap.end())
        {
            auto varIdIter = m_varIdMap.find(foundSlotVarInfoIt->second.m_ownedVariableId);
            if (varIdIter != m_varIdMap.end())
            {
                varDatum = &*varIdIter->second;
                Datum& setDatum = varDatum->GetData();
                WriteInput(setDatum, AZStd::move(newInput));
                OnInputChanged(setDatum, slotId);
            }
        }
    }

    void Node::SetInput(Node& node, const SlotId& id, const Datum& input)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);
        node.SetInput(input, id);
    }

    void Node::SetInput(Node& node, const SlotId& id, Datum&& input)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::SetInput");

        node.SetInput(AZStd::move(input), id);
    }

    void Node::WriteInput(Datum& destination, const Datum& source)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::WriteInput");

        destination = source;
    }

    void Node::WriteInput(Datum& destination, Datum&& source)
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Node::WriteInput");

        destination = AZStd::move(source);
    }
}