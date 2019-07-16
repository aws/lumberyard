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

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/tuple.h>

#include <ScriptCanvas/Core/Contracts/TypeContract.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/DatumBus.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/SignalBus.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/NodeVisitor.h>
#include <ScriptCanvas/Variable/VariableDatumBase.h>

#define SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(lambdaInterior)\
    int dummy[]{ 0, ( lambdaInterior , 0)... };\
    static_cast<void>(dummy); /* avoid warning for unused variable */

namespace ScriptCanvas
{
    namespace Internal
    {
        template<typename t_Func, t_Func function, typename t_Traits, size_t NumOutputs>
        struct MultipleOutputHelper;

        template<typename ResultType, typename t_Traits, typename>
        struct OutputSlotHelper;

        template<typename ResultType, typename t_Traits, typename>
        struct PushOutputHelper;

        template<typename ResultType, typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        struct CallHelper;
    }
    class Graph;

    struct BehaviorContextMethodHelper;

    template<typename t_Class>
    class SerializeContextEventHandlerDefault : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called after we are done writing to the instance pointed by classPtr.
        void OnWriteEnd(void* objectPtr) override
        {
            t_Class* deserializedObject = reinterpret_cast<t_Class*>(objectPtr);
            deserializedObject->OnWriteEnd();
        }
    };

    //! Maintains the data type and variableId associated with a data slot
    //! This structure is used as value in a map where the key is the slot being associated with a Data::Type
    struct VariableInfo
    {
        AZ_TYPE_INFO(VariableInfo, "{57DEBC6B-8708-454B-96DC-0A34D1835709}");
        AZ_CLASS_ALLOCATOR(VariableInfo, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        VariableInfo() = default;
        VariableInfo(const VariableId& nodeOwnedVarId);
        VariableInfo(const Data::Type& dataType);

        VariableId m_currentVariableId; // Variable Id of VariableDatum to use when accessing the associated slot data input
        VariableId m_ownedVariableId; // Variable Id of VariableDatum which is managed by this node and associated with the slot
        Data::Type m_dataType;
    };

    class Node
        : public AZ::Component
        , public DatumNotificationBus::Handler
        , private SignalBus::Handler
        , public NodeRequestBus::Handler
        , private EditorNodeRequestBus::Handler
    {
        friend class Graph;
        friend class RuntimeComponent;
        friend struct BehaviorContextMethodHelper;
        friend class NodeEventHandler;

    public:
        using SlotList = AZStd::list<Slot>;
        using SlotIterator = SlotList::iterator;
        using VariableList = AZStd::list<VariableDatumBase>;
        using VariableIterator = VariableList::iterator;
        enum class OutputStorage { Optional, Required };

        template<size_t... inputDatumIndices>
        struct SetDefaultValuesByIndex
        {
            template<typename... t_Args>
            AZ_INLINE static void _(Node& node, t_Args&&... args)
            {
                Help(node, AZStd::make_index_sequence<sizeof...(t_Args)>(), AZStd::forward<t_Args>(args)...);
            }

        private:
            template<AZStd::size_t... Is, typename... t_Args>
            AZ_INLINE static void Help(Node& node, AZStd::index_sequence<Is...>, t_Args&&... args)
            {
                static int indices[] = { inputDatumIndices... };
                AZ_STATIC_ASSERT(sizeof...(Is) == AZ_ARRAY_SIZE(indices), "size of default values doesn't match input datum indices for them");
                SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(*node.ModDatumByIndex(indices[Is])->template ModAs<AZStd::decay_t<t_Args>>() = AZStd::forward<t_Args>(args));
            }
        };

        AZ_COMPONENT(Node, "{52B454AE-FA7E-4FE9-87D3-A1CAB235C691}");

        Node();
        ~Node() override;
        Node(const Node&); // Needed just for DLL linkage. Does not perform a copy
        Node& operator=(const Node&); // Needed just for DLL linkage. Does not perform a copy

        static void Reflect(AZ::ReflectContext* reflection);

        Graph* GetGraph() const;
        AZ::EntityId GetGraphEntityId() const override;
        AZ::Data::AssetId GetGraphAssetId() const;
        GraphIdentifier GetGraphIdentifier() const;

        virtual bool IsEntryPoint() const
        {
            return false;
        }

        //! Node internal initialization, for custom init, use OnInit
        void Init() override final;

        //! Node internal activation and housekeeping, for custom activation configuration use OnActivate
        void Activate() override final;

        //! Node internal deactivation and housekeeping, for custom deactivation configuration use OnDeactivate
        void Deactivate() override final;

        virtual AZStd::string GetDebugName() const
        {
            if (GetEntityId().IsValid())
            {
                return AZStd::string::format("%s (%s)", GetEntity()->GetName().c_str(), TYPEINFO_Name());
            }
            return TYPEINFO_Name();
        }

        virtual AZStd::string GetNodeName() const
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (serializeContext)
            {
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(RTTI_GetType());

                if (classData)
                {
                    if (classData->m_editData)
                    {
                        return classData->m_editData->m_name;
                    }
                    else
                    {
                        return classData->m_name;
                    }
                }
            }

            return "<unknown>";
        }

        AZStd::string GetSlotName(const SlotId& slotId) const;

        //! returns a list of all slots, regardless of type
        const SlotList& GetSlots() const { return m_slots; }

        //! returns a const list of nodes connected to slot(s) of the specified type
        NodePtrConstList GetConnectedNodesByType(SlotType slotType) const;
        AZStd::vector<AZStd::pair<const Node*, SlotId>> GetConnectedNodesAndSlotsByType(SlotType slotType) const;

        bool IsConnected(const Slot& slot) const;
        bool IsConnected(const SlotId& slotId) const;

        bool IsPureData() const;

        virtual bool IsEventHandler() const;

        //////////////////////////////////////////////////////////////////////////
        // NodeRequestBus::Handler
        Slot* GetSlot(const SlotId& slotId) const override;
        size_t GetSlotIndex(const SlotId& slotId) const override;
        AZStd::vector<const Slot*> GetAllSlots() const override;
        SlotId GetSlotId(AZStd::string_view slotName) const override;
        SlotId GetSlotIdByType(AZStd::string_view slotName, SlotType slotType) const override;
        AZStd::vector<SlotId> GetSlotIds(AZStd::string_view slotName) const override;
        const AZ::EntityId& GetGraphId() const override { return m_executionUniqueId; }
        bool SlotAcceptsType(const SlotId&, const Data::Type&) const override;
        Data::Type GetSlotDataType(const SlotId& slotId) const override;
        VariableId GetSlotVariableId(const SlotId& slotId) const override;
        void SetSlotVariableId(const SlotId& slotId, const VariableId& variableId) override;
        void ResetSlotVariableId(const SlotId& slotId) override;
        AZ::Outcome<AZ::s64, AZStd::string> FindSlotIndex(const SlotId& slotId) const override;
        bool IsOnPureDataThread(const SlotId& slotId) const override;
        ////

        // DatumNotificationBus::Handler
        void OnDatumChanged(const Datum* datum) override;
        ////
        
        bool IsTargetInDataFlowPath(const Node* targetNode) const;
        bool IsInEventHandlingScope(const ID& possibleEventHandler) const;
        
        virtual void MarkDefaultableInput();
        
        AZStd::string CreateInputMapString(const SlotDataMap& map) const;

        bool IsNodeType(const NodeTypeIdentifier& nodeIdentifier) const;
        NodeTypeIdentifier GetNodeType() const;

        // Mainly here for EBus handlers which contain multiple 'events' which are differentiated by
        // Endpoint.
        virtual NodeTypeIdentifier GetOutputNodeType(const SlotId& slotId) const;
        virtual NodeTypeIdentifier GetInputNodeType(const SlotId& slotId) const;

    protected:
        static const Datum* GetInput(const Node& node, const SlotId slotID);
        static Datum* ModInput(Node& node, const SlotId slotID);
        static void OnInputChanged(Node& node, const Datum& input, const SlotId& slotID);
        static void SetInput(Node& node, const SlotId& id, const Datum& input);
        static void SetInput(Node& node, const SlotId& id, Datum&& input);
        
        // \todo make fast query to the system debugger
        AZ_INLINE static bool IsNodeObserved(const Node& node)
        {
            bool isObserved{};
            ExecutionNotificationsBus::BroadcastResult(isObserved, &ExecutionNotifications::IsNodeObserved, node);
            return isObserved;
        }

        AZ_INLINE static bool IsVariableObserved(const Node& node, VariableId variableId)
        {
            bool isObserved{};
            ExecutionNotificationsBus::BroadcastResult(isObserved, &ExecutionNotifications::IsVariableObserved, node, variableId);
            return isObserved;
        }
        
        AZ::EntityId m_executionUniqueId;
        NodeTypeIdentifier m_nodeType;

        SlotList m_slots;
        VariableList m_varDatums;

        AZStd::unordered_set< SlotId > m_removingSlots;

        AZStd::unordered_map<SlotId, VariableInfo> m_slotIdVarInfoMap;

        AZStd::unordered_map<SlotId, SlotIterator> m_slotIdMap;
        AZStd::unordered_multimap<AZStd::string, SlotIterator> m_slotNameMap;

        AZStd::unordered_map<VariableId, VariableIterator> m_varIdMap;
        AZStd::unordered_set<SlotId> m_possiblyStaleInput;

    protected:
        SlotId GetSlotId(const VariableId& varId) const;
        VariableId GetVariableId(const SlotId& slotId) const;
        Slot* GetSlot(const VariableId& varId) const;
        VariableDatumBase* GetActiveVariableDatum(const SlotId& slotId) const;

        // EditorNodeRequestsBus::Handler
        Datum* ModInput(const SlotId& slotID) override;
        const Datum* GetInput(const SlotId& slotId) const override;
        ////

        Datum* ModDatumByIndex(size_t index);
        const Datum* GetDatumByIndex(size_t index) const;
        const Slot* GetSlotByIndex(size_t index) const;

        // Inserts a data input slot before the element found at @index. If the index < 0 or >= slots.size(), the slot will be will be added at the end
        SlotId InsertInputDatumSlot(AZ::s64 index, const DataSlotConfiguration& slotConfig, Datum&& initialDatum);

        // Always adds if the SlotConfiguration addUniqueSlotByName field is false.
        // If the addUniqueSlotByName is true, it will only add the slot if it doesn't exist in the SlotList.
        // If the slot does exist in the SlotList the slot id of that slot is returned 
        SlotId AddInputDatumSlot(const DataSlotConfiguration& slotConfig, Datum&& initialDatum);
        SlotId AddInputDatumSlot(const DataSlotConfiguration& slotConfig);

        SlotId AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, Datum::eOriginality originality, bool addUniqueSlotByNameAndType = true);
        SlotId AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, const void* source, Datum::eOriginality originality, bool addUniqueSlotByNameAndType = true);
        SlotId AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZ::BehaviorParameter& typeDesc, Datum::eOriginality originality, bool addUniqueSlotByNameAndType = true);

        template<typename DatumType>
        SlotId AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, Datum::eOriginality originality, DatumType&& defaultValue, bool addUniqueSlotByNameAndType = true);
        
        // add an input slot, with a type determined by the first connection
        SlotId AddInputDatumDynamicTypedSlot(AZStd::string_view name, AZStd::string_view toolTip = {}, bool addUniqueSlotByNameAndType = true);

        // add an container input slot, with a type determined by the first connection
        SlotId AddInputDatumDynamicContainerTypedSlot(AZStd::string_view name, AZStd::string_view toolTip = {}, const AZStd::vector<ContractDescriptor>& contractsIn = AZStd::vector<ContractDescriptor>{}, bool addUniqueSlotByNameAndType = true);
        
        // add an untyped input slot (for input overloaded methods on all types, like print)
        SlotId AddInputDatumOverloadedSlot(AZStd::string_view name, AZStd::string_view toolTip = {}, const AZStd::vector<ContractDescriptor>& contracts = AZStd::vector<ContractDescriptor>{}, bool addUniqueSlotByNameAndType = true);
        
        enum class InputTypeContract { CustomType, DatumType, None };
        
        // add a typed input slot, but do not add storage for the input
        SlotId AddInputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, InputTypeContract contractType, bool addUniqueSlotByNameAndType = true);
        SlotId AddInputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZ::BehaviorParameter& typeDesc, InputTypeContract contractType, bool addUniqueSlotByNameAndType = true);

        SlotId AddInputTypeSlot(const SlotConfiguration& slotConfiguration);
        
        // add a typed output slot
        SlotId AddOutputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, OutputStorage outputStorage, bool addUniqueSlotByNameAndType = true);
        
        // Inserts a slot before the element found at @index. If the index < 0 or >= slots.size(), the slot will be will be added at the end
        SlotId InsertSlot(AZ::s64 index, const SlotConfiguration&);

        // Adds or finds a slot with the corresponding name, type, contracts. This operation is idempotent only if addUniqueSlotByNameAndType is true
        // Always adds if the SlotConfiguration addUniqueSlotByName field is false.
        // If the addUniqueSlotByName is true, it will only add the slot if it doesn't exist in the SlotList.
        // If the slot does exist in the SlotList the slot id of that slot is returned 
        SlotId AddSlot(const SlotConfiguration&);
        SlotId AddDataSlot(const DataSlotConfiguration& dataSlotConfiguration);
        
        // Adds or finds a slot with the corresponding name, type, contracts. This operation is idempotent only if addUniqueSlotByNameAndType is true
        ///< \deprecated use AddSlot(const SlotConfiguration&) instead
        SlotId AddSlot(AZStd::string_view name, AZStd::string_view toolTip, SlotType type, const AZStd::vector<ContractDescriptor>& contractDescs = AZStd::vector<ContractDescriptor>{}, bool addUniqueSlotByNameAndType = true)
        {
            SlotConfiguration slotConfiguration;

            slotConfiguration.m_name = name;
            slotConfiguration.m_toolTip = toolTip;
            slotConfiguration.m_slotType = type;
            slotConfiguration.m_contractDescs = contractDescs;
            slotConfiguration.m_addUniqueSlotByNameAndType = addUniqueSlotByNameAndType;

            return AddSlot(slotConfiguration);
        }
        
    private:
        // insert or find an input slot with a initialized using a datum and returns Success if a new slot was inserted.
        // The SlotIterator& parameter is populated with an iterator to the inserted or found Slot within the slot list 
        // The VariableIterator& parameter is populated with an iterator to the inserted VariableDatum with the variable datum list
        AZ::Outcome<void, AZStd::string> InsertInputDatumSlotInternal(AZ::s64 index, const DataSlotConfiguration& slotConfig, Datum&& datum, SlotIterator& slotIterOut, VariableIterator& varIterOut);
        
        // insert a slot within the in the slot list and set the slot data type to be the supplied data type
        // The SlotIterator& parameter is populated with an iterator to the inserted or found Slot within the slot list 
        AZ::Outcome<void, AZStd::string> InsertDataTypeSlotInternal(AZ::s64 index, const DataSlotConfiguration& slotConfig, SlotIterator& slotIterOut);

        // insert or find a slot in the slot list and returns Success if a new slot was inserted.
        // The SlotIterator& parameter is populated with an iterator to the inserted or found slot within the slot list 
        AZ::Outcome<void, AZStd::string> InsertSlotInternal(AZ::s64 index, const SlotConfiguration& slotConfig, SlotIterator& iterOut);

        // This function is only called once, when the node is added to a graph, as opposed to Init(), which will be called 
        // soon after construction, or after deserialization. So the functionality in configure does not need to be idempotent.
        void Configure();

    protected:
        SlotDataMap CreateInputMap() const;
        SlotDataMap CreateOutputMap() const;

        NamedEndpoint CreateNamedEndpoint(AZ::EntityId editorNodeId, SlotId slotId) const;

        Signal CreateNodeInputSignal(const SlotId& slotId) const;
        Signal CreateNodeOutputSignal(const SlotId& slotId) const;
        OutputDataSignal CreateNodeOutputDataSignal(const SlotId& slotId, const Datum& datum) const;
        
        NodeStateChange CreateNodeStateUpdate() const;
        VariableChange CreateVariableChange(const VariableDatumBase& variable) const;
        
        // restored inputs to graph defaults, if necessary and possible
        void RefreshInput();

        // Removes the slot on this node that matches the supplied slotId
        bool RemoveSlot(const SlotId& slotId, bool removeConnections = true);
        void RemoveConnectionsForSlot(const SlotId& slotId);
        void SignalSlotRemoved(const SlotId& slotId);

        // some slots change based on the input connected to them
        enum class DynamicTypeArity { Single, Multiple };
        bool DynamicSlotAcceptsType(const SlotId& slotID, const Data::Type& type, DynamicTypeArity arity, const Slot& outputSlot, const AZStd::vector<Slot*>& inputSlots) const;
        bool DynamicSlotInputAcceptsType(const SlotId& slotID, const Data::Type& type, DynamicTypeArity arity, const Slot& inputSlot) const;

        //! This is a used by CodeGen to configure slots just prior to OnInit.
        virtual void ConfigureSlots() {}
        //! Entity level initialization, perform any resource allocation here that should be available throughout the node's existence.
        virtual void OnInit() {}
        //! Entity level activation, perform entity lifetime setup here, i.e. connect to EBuses
        virtual void OnActivate() {}
        //! Entity level deactivation, perform any entity lifetime release here, i.e disconnect from EBuses
        virtual void OnDeactivate() {}
        //! Any node that is not pure data will perform its operation assuming that all input has been pushed and is reasonably valid
        // perform your derived nodes work in OnInputSignal(const SlotId&)
        virtual void OnInputSignal(const SlotId& slot) {}

        //! Signal sent once the UniqueExecutionId is set.
        virtual void OnGraphSet() {};

        void ForEachConnectedNode(const Slot& slot, AZStd::function<void(Node&, const SlotId&)> callable) const;

        AZStd::vector<Endpoint> GetEndpointsByType(SlotType slotType) const;
        AZStd::vector<AZStd::pair<const Node*, const SlotId>> GetConnectedNodes(const Slot& slot) const;
        AZStd::vector<AZStd::pair<Node*, const SlotId>> ModConnectedNodes(const Slot& slot) const;
        void ModConnectedNodes(const Slot& slot, AZStd::vector<AZStd::pair<Node*, const SlotId>>&) const;

        bool HasConnectedNodes(const Slot& slot) const;

        AZStd::vector<const Slot*> GetSlotsByType(SlotType slotType) const;

        AZ::Outcome<AZ::s64, AZStd::string> FindVariableIndex(const VariableId& varId) const;

        virtual void OnInputChanged(const Datum& input, const SlotId& slotID) {}

        //////////////////////////////////////////////////////////////////////////
        //! The body of the node's execution, implement the node's work here.
        void PushOutput(const Datum& output, const Slot& slot) const;
        void SetGraphUniqueId(AZ::EntityId uniqueId);
        // on activate, simple expressions need to push this data
        virtual void SetInput(const Datum& input, const SlotId& id);
        virtual void SetInput(Datum&& input, const SlotId& id);

        //! Any node that is not pure data will perform its operation assuming that all input has been pushed and is reasonably valid
        // perform your derived nodes work in OnInputSignal(const SlotId&)
        void SignalInput(const SlotId& slot) override final;
        //! This must be called manually to send execution out of a specific slot
        void SignalOutput(const SlotId& slot, ExecuteMode mode = ExecuteMode::Normal) override final;

        bool SlotExists(AZStd::string_view name, SlotType type) const;
        bool SlotExists(AZStd::string_view name, SlotType type, SlotId& out) const;
        virtual void WriteInput(Datum& destination, const Datum& source);
        virtual void WriteInput(Datum& destination, Datum&& source);

        bool IsTargetInDataFlowPath(const ID& targetNodeId, AZStd::unordered_set<ID>& path) const;
        bool IsInEventHandlingScope(const ID& eventHandler, const AZStd::vector<SlotId>& eventSlots, const SlotId& connectionSlot, AZStd::unordered_set<ID>& path) const;
        bool IsOnPureDataThreadHelper(AZStd::unordered_set<ID>& path) const;

        void PopulateNodeType();

    private:
        void SetToDefaultValueOfType(const SlotId& slotID);
        void RebuildSlotAndVariableIterators();

        // Looks up a slot iterator using the supplied slot Id
        // returns true if the slot iterator was found 
        AZ::Outcome<SlotIterator, AZStd::string> FindSlotIterator(const SlotId& slotId) const;
        // Looks up a slot iterator using the supplied variable id
        // The variable id is first searched among the list of the nodes VariableDatums
        // If it is not found, it is then searched among the active VariableDatums
        // returns true if the slot iterator was found
        AZ::Outcome<SlotIterator, AZStd::string> FindSlotIterator(const VariableId& varId) const;
        // Looks up a variable iterator using the supplied slot id in the slot id -> variable info map
        // returns true if the variable iterator was found
        AZ::Outcome<VariableIterator, AZStd::string> FindVariableIterator(const SlotId& slotId) const;
        // Looks up a variable iterator using the supplied variable id in the variable id map
        // returns true if the variable iterator was found
        AZ::Outcome<VariableIterator, AZStd::string> FindVariableIterator(const VariableId& varId) const;

        //////////////////////////////////////////////////////////////////////////
        /// VM 0.1 begin
    public:
        virtual void Visit(NodeVisitor& visitor) const { AZ_Assert(false, "This class isn't abstract (yet), but this function must be overridden in your node: %s", GetDebugName().c_str()); }
        const Node* GetNextExecutableNode() const;
        /// VM 0.1 end
        //////////////////////////////////////////////////////////////////////////

#if defined(AZ_TESTS_ENABLED)
    public:
        template<typename t_Value>
        const t_Value* GetInput_UNIT_TEST(AZStd::string_view slotName)
        {
            return GetInput_UNIT_TEST<t_Value>(GetSlotId(slotName));
        }

        template<typename t_Value>
        t_Value* ModInput_UNIT_TEST(AZStd::string_view slotName)
        {
            return ModInput_UNIT_TEST<t_Value>(GetSlotId(slotName));
        }

        template<typename t_Value>
        const t_Value* GetInput_UNIT_TEST(const SlotId& slotId)
        {
            const Datum* input = GetInput(slotId);
            return input ? input->GetAs<t_Value>() : nullptr;
        }

        template<typename t_Value>
        t_Value* ModInput_UNIT_TEST(const SlotId& slotId)
        {
            Datum* input = ModInput(slotId);
            return input ? input->ModAs<t_Value>() : nullptr;
        }

        const Datum* GetInput_UNIT_TEST(AZStd::string_view slotName)
        {
            return GetInput_UNIT_TEST(GetSlotId(slotName));
        }

        const Datum* GetInput_UNIT_TEST(const SlotId& slotId)
        {
            return GetInput(slotId);
        }

        // initializes the node input to the value passed in, not a pointer to it
        template<typename t_Value>
        void SetInput_UNIT_TEST(AZStd::string_view slotName, const t_Value& value)
        {
            SetInput_UNIT_TEST<t_Value>(GetSlotId(slotName), value);
        }

        template<typename t_Value>
        void SetInput_UNIT_TEST(const SlotId& slotId, const t_Value& value)
        {
            SetInput(Datum(value), slotId);
        }

        void SetInput_UNIT_TEST(AZStd::string_view slotName, const Datum& value)
        {
            SetInput_UNIT_TEST(GetSlotId(slotName), value);
        }

        void SetInput_UNIT_TEST(AZStd::string_view slotName, Datum&& value)
        {
            SetInput_UNIT_TEST(GetSlotId(slotName), AZStd::move(value));
        }

        void SetInput_UNIT_TEST(const SlotId& slotId, Datum&& value)
        {
            SetInput(AZStd::move(value), slotId);
        }
#endif//defined(AZ_TESTS_ENABLED)

        template<typename t_Func, t_Func, typename, size_t>
        friend struct Internal::MultipleOutputHelper;

        template<typename ResultType, typename t_Traits, typename>
        friend struct Internal::OutputSlotHelper;

        template<typename ResultType, typename t_Traits, typename>
        friend struct Internal::PushOutputHelper;

        template<typename ResultType, typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        friend struct Internal::CallHelper;
    };

    template<typename DatumType>
    SlotId Node::AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, Datum::eOriginality originality, DatumType&& defaultValue, bool addUniqueSlotByNameAndType)
    {
        DataSlotConfiguration slotConfiguration;

        slotConfiguration.m_name = name;
        slotConfiguration.m_toolTip = toolTip;
        slotConfiguration.m_slotType = SlotType::DataIn;
        slotConfiguration.m_addUniqueSlotByNameAndType = addUniqueSlotByNameAndType;

        return AddInputDatumSlot(slotConfiguration, Datum(AZStd::forward<DatumType>(defaultValue)));
    }

    namespace Internal
    {
        template<typename, typename = AZStd::void_t<>>
        struct IsTupleLike : AZStd::false_type {};

        /* 
        https://blogs.msdn.microsoft.com/vcblog/2015/12/02/partial-support-for-expression-sfinae-in-vs-2015-update-1/
        VS2013 is unable to deal with the decltype expressions in template arguments(but only in some cases)
        // This signature would have allowed any type that specializes the std::get template function to produce multiple output slots
        // UNCOMMENT when VS2013 is no longer supported
        template<typename ResultType>
        struct IsTupleLike<ResultType, AZStd::void_t<decltype(AZStd::get<0>(AZStd::declval<ResultType>()))> : AZStd::true_type {};
        */
        // TODO: More constrained template that should be removed when VS2013 is no longer supported
        template<typename... Args>
        struct IsTupleLike<AZStd::tuple<Args...>, AZStd::void_t<>> : AZStd::true_type {};

        template<typename T, size_t N>
        struct IsTupleLike<AZStd::array<T, N>, AZStd::void_t<>> : AZStd::true_type {};

        template<typename T1, typename T2>
        struct IsTupleLike<AZStd::pair<T1, T2>, AZStd::void_t<>> : AZStd::true_type {};

        template<typename ResultType, typename t_Traits, typename = AZStd::void_t<>>
        struct OutputSlotHelper
        {
            template<AZStd::size_t... Is>
            static void AddOutputSlot(Node& node, AZStd::index_sequence<Is...>)
            {
                node.AddOutputTypeSlot(AZStd::string::format("%s: %s", t_Traits::GetResultName(0), Data::GetName(Data::FromAZType<AZStd::decay_t<ResultType>>()).data()),
                    AZStd::string_view(), Data::FromAZType<AZStd::decay_t<ResultType>>(), Node::OutputStorage::Optional);
            }
        };

        template<typename t_Traits>
        struct OutputSlotHelper<void, t_Traits, void>
        {
            template<size_t... Is> static void AddOutputSlot(Node& node, AZStd::index_sequence<Is...>) {}
        };

        template<typename ResultType, typename t_Traits>
        struct OutputSlotHelper<ResultType, t_Traits, AZStd::enable_if_t<IsTupleLike<ResultType>::value>>
        {
            template<AZStd::size_t... Is>
            static void AddOutputSlot(Node& node, AZStd::index_sequence<Is...>)
            {
                SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(
                    node.AddOutputTypeSlot(AZStd::string::format("%s: %s", t_Traits::GetResultName(Is), Data::GetName(Data::FromAZType<AZStd::decay_t<AZStd::tuple_element_t<Is, ResultType>>>()).data()),
                        AZStd::string_view(), Data::FromAZType<AZStd::decay_t<AZStd::tuple_element_t<Is, ResultType>>>(), Node::OutputStorage::Optional));
            }
        };


        template<typename ResultType, typename t_Traits, typename = AZStd::void_t<>>
        struct PushOutputHelper
        {
            template<size_t... Is>
            static void PushOutput(Node& node, ResultType&& result, AZStd::index_sequence<Is...>)
            {
                node.PushOutput(Datum(AZStd::forward<ResultType>(result)), *node.GetSlotByIndex(t_Traits::s_resultsSlotIndicesStart));
            }
        };

        template<typename t_Traits>
        struct PushOutputHelper<void, t_Traits, void>
        {
            template<size_t... Is> static void PushOutput(Node& node, AZStd::ignore_t result, AZStd::index_sequence<Is...>) {}
        };

        template<typename ResultType, typename t_Traits>
        struct PushOutputHelper<ResultType, t_Traits, AZStd::enable_if_t<IsTupleLike<ResultType>::value>>
        {
            template<size_t... Is>
            static void PushOutput(Node& node, ResultType&& tupleResult, AZStd::index_sequence<Is...>)
            {
                SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(node.PushOutput(
                    Datum(AZStd::get<Is>(AZStd::forward<ResultType>(tupleResult))),
                    *node.GetSlotByIndex(t_Traits::s_resultsSlotIndicesStart + Is)));
            }
        };

        template<typename ResultType, typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        struct CallHelper
        {
            template<typename... t_Args, size_t... ArgIndices>
            static void Call(Node& node, AZStd::Internal::pack_traits_arg_sequence<t_Args...>, AZStd::index_sequence<ArgIndices...>)
            {
                PushOutputHelper<ResultType, t_Traits>::PushOutput(node, AZStd::invoke(function, *node.GetDatumByIndex(ArgIndices)->GetAs<AZStd::decay_t<t_Args>>()...), ResultIndexSequence());
            }
        };

        template<typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        struct CallHelper<void, ResultIndexSequence, t_Func, function, t_Traits>
        {
            template<typename... t_Args, size_t... ArgIndices>
            static void Call(Node& node, AZStd::Internal::pack_traits_arg_sequence<t_Args...>, AZStd::index_sequence<ArgIndices...>)
            {
                AZStd::invoke(function, *node.GetDatumByIndex(ArgIndices)->GetAs<AZStd::decay_t<t_Args>>()...);
            }
        };

        template<typename t_Func, t_Func function, typename t_Traits, size_t NumOutputs>
        struct MultipleOutputHelper
        {
            using ResultIndexSequence = AZStd::make_index_sequence<NumOutputs>;
            using ResultType = AZStd::function_traits_get_result_t<t_Func>;

            static void Add(Node& node)
            {
                OutputSlotHelper<ResultType, t_Traits>::AddOutputSlot(node, ResultIndexSequence());
            }

            static void Call(Node& node)
            {
                CallHelper<ResultType, ResultIndexSequence, t_Func, function, t_Traits>::Call(node, typename AZStd::function_traits<t_Func>::arg_sequence{}, AZStd::make_index_sequence<AZStd::function_traits<t_Func>::arity>{});
            }
        };

        template<typename t_Func, t_Func function, typename t_Traits, typename t_Result, typename = AZStd::void_t<>>
        struct MultipleOutputInvokeHelper
            : MultipleOutputHelper<t_Func, function, t_Traits, 1> {};

        template<typename t_Func, t_Func function, typename t_Traits>
        struct MultipleOutputInvokeHelper<t_Func, function, t_Traits, void, void>
            : MultipleOutputHelper<t_Func, function, t_Traits, 0> {};

        template<typename t_Func, t_Func function, typename t_Traits, typename t_Result>
        struct MultipleOutputInvokeHelper< t_Func, function, t_Traits, t_Result, AZStd::enable_if_t<IsTupleLike<t_Result>::value>>
            : MultipleOutputHelper<t_Func, function, t_Traits, AZStd::tuple_size<t_Result>::value> {};
    }

    template<typename t_Func, t_Func function, typename t_Traits>
    struct MultipleOutputInvoker : Internal::MultipleOutputInvokeHelper<t_Func, function, t_Traits, AZStd::decay_t<AZStd::function_traits_get_result_t<t_Func>>> {};
}
