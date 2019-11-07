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
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/SignalBus.h>
#include <ScriptCanvas/Core/Slot.h>
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

    struct ExtendableSlotConfiguration
    {
    public:
        AZ_TYPE_INFO(ExtendableSlotConfiguration, "{3EA2D6DB-1B8F-451B-A6CE-D5779E56F4A8}");
        AZ_CLASS_ALLOCATOR(ExtendableSlotConfiguration, AZ::SystemAllocator, 0);

        ExtendableSlotConfiguration() = default;
        ~ExtendableSlotConfiguration() = default;

        AZStd::string m_name;
        AZStd::string m_tooltip;

        AZStd::string m_displayGroup;

        AZ::Crc32     m_identifier;
        ConnectionType m_connectionType = ConnectionType::Unknown;
    };

    class Node
        : public AZ::Component
        , public DatumNotificationBus::Handler
        , private SignalBus::Handler
        , public NodeRequestBus::Handler
        , private EditorNodeRequestBus::Handler
        , public EndpointNotificationBus::MultiHandler
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

        using ExploredDynamicGroupCache = AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set< AZ::Crc32 >>;

        AZ_COMPONENT(Node, "{52B454AE-FA7E-4FE9-87D3-A1CAB235C691}");
        static void Reflect(AZ::ReflectContext* reflection);

        Node();
        ~Node() override;
        Node(const Node&); // Needed just for DLL linkage. Does not perform a copy
        Node& operator=(const Node&); // Needed just for DLL linkage. Does not perform a copy        

        Graph* GetGraph() const;
        AZ::EntityId GetGraphEntityId() const override;
        AZ::Data::AssetId GetGraphAssetId() const;
        GraphIdentifier GetGraphIdentifier() const;

        void SanityCheckDynamicDisplay(ExploredDynamicGroupCache& exploredGroupCache);

        virtual bool IsEntryPoint() const;

        //! Node internal initialization, for custom init, use OnInit
        void Init() override final;

        //! Node internal activation and housekeeping, for custom activation configuration use OnActivate
        void Activate() override final;

        //! Node internal deactivation and housekeeping, for custom deactivation configuration use OnDeactivate
        void Deactivate() override final;

        virtual AZStd::string GetDebugName() const;
        virtual AZStd::string GetNodeName() const;

        AZStd::string GetSlotName(const SlotId& slotId) const;

        //! returns a list of all slots, regardless of type
        const SlotList& GetSlots() const { return m_slots; }        

        AZStd::vector< Slot* > GetSlotsWithDisplayGroup(AZStd::string_view displayGroup) const;
        AZStd::vector< Slot* > GetSlotsWithDynamicGroup(const AZ::Crc32& dynamicGroup) const;
        AZStd::vector<const Slot*> GetAllSlotsByDescriptor(const SlotDescriptor& slotDescriptor, bool allowLatentSlots = false) const;

        //! returns a const list of nodes connected to slot(s) of the specified type
        NodePtrConstList FindConnectedNodesByDescriptor(const SlotDescriptor& slotType, bool followLatentConnections = false) const;
        AZStd::vector<AZStd::pair<const Node*, SlotId>> FindConnectedNodesAndSlotsByDescriptor(const SlotDescriptor& slotType, bool followLatentConnections = false) const;

        bool IsConnected(const Slot& slot) const;
        bool IsConnected(const SlotId& slotId) const;

        bool IsPureData() const;

        //////////////////////////////////////////////////////////////////////////
        // NodeRequestBus::Handler
        Slot* GetSlot(const SlotId& slotId) const override;
        size_t GetSlotIndex(const SlotId& slotId) const override;
        AZStd::vector<const Slot*> GetAllSlots() const override;
        SlotId GetSlotId(AZStd::string_view slotName) const override;
        SlotId FindSlotIdForDescriptor(AZStd::string_view slotName, const SlotDescriptor& descriptor) const override;

        AZStd::vector<SlotId> GetSlotIds(AZStd::string_view slotName) const override;
        const AZ::EntityId& GetGraphId() const override { return m_executionUniqueId; }
        bool SlotAcceptsType(const SlotId&, const Data::Type&) const override;
        Data::Type GetSlotDataType(const SlotId& slotId) const override;
        VariableId GetSlotVariableId(const SlotId& slotId) const override;
        void SetSlotVariableId(const SlotId& slotId, const VariableId& variableId) override;
        void ResetSlotVariableId(const SlotId& slotId) override;
        AZ::Outcome<AZ::s64, AZStd::string> FindSlotIndex(const SlotId& slotId) const override;
        bool IsOnPureDataThread(const SlotId& slotId) const override;

        AZ::Outcome<void, AZStd::string> IsValidTypeForGroup(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) const override;

        void SignalBatchedConnectionManipulationBegin() override;
        void SignalBatchedConnectionManipulationEnd() override;

        void SetNodeEnabled(bool enabled) override;
        bool IsNodeEnabled() const override;
        ////

        Slot* GetSlotByName(AZStd::string_view slotName) const;

        // DatumNotificationBus::Handler
        void OnDatumChanged(const Datum* datum) override;
        ////

        ////
        // EndpointNotificationBus
        void OnEndpointConnected(const Endpoint& endpoint) override;
        void OnEndpointDisconnected(const Endpoint& endpoint) override;
        ////
        
        bool IsTargetInDataFlowPath(const Node* targetNode) const;
        bool IsInEventHandlingScope(const ID& possibleEventHandler) const;
        
        virtual void MarkDefaultableInput();
        
        AZStd::string CreateInputMapString(const SlotDataMap& map) const;

        bool IsNodeType(const NodeTypeIdentifier& nodeIdentifier) const;
        NodeTypeIdentifier GetNodeType() const;

        void ResetSlotToDefaultValue(const SlotId& slotId);

        void DeleteSlot(const SlotId& slotId)
        {
            if (CanDeleteSlot(slotId))
            {
                RemoveSlot(slotId);
            }
        }
        
        virtual bool CanDeleteSlot(const SlotId& slotId) const;

        virtual bool IsNodeExtendable() const;
        virtual int GetNumberOfExtensions() const;
        virtual ExtendableSlotConfiguration GetExtensionConfiguration(int extensionCount) const;

        virtual SlotId HandleExtension(AZ::Crc32 extensionId);        

        // Mainly here for EBus handlers which contain multiple 'events' which are differentiated by
        // Endpoint.
        virtual NodeTypeIdentifier GetOutputNodeType(const SlotId& slotId) const;
        virtual NodeTypeIdentifier GetInputNodeType(const SlotId& slotId) const;

        // Hook here to allow CodeGen to override this
        virtual bool IsDeprecated() const { return false; };

    protected:
        static const Datum* GetInput(const Node& node, const SlotId slotID);
        static Datum* ModInput(Node& node, const SlotId slotID);
        static void OnInputChanged(Node& node, const Datum& input, const SlotId& slotID);
        static void SetInput(Node& node, const SlotId& id, const Datum& input);
        static void SetInput(Node& node, const SlotId& id, Datum&& input);

        bool HasSlots() const;

        //! returns a list of all slots, regardless of type
        SlotList& ModSlots() { return m_slots; }
        
        // \todo make fast query to the system debugger
        AZ_INLINE static bool IsNodeObserved(const Node& node);
        AZ_INLINE static bool IsVariableObserved(const Node& node, VariableId variableId);
    
        SlotId GetSlotId(const VariableId& varId) const;
        VariableId GetVariableId(const SlotId& slotId) const;
        Slot* GetSlot(const VariableId& varId) const;
        VariableDatumBase* GetActiveVariableDatum(const SlotId& slotId) const;

        const VariableList& GetVarDatums() const;
        const VariableDatumBase& GetVarDatum(int index) const;
        VariableDatumBase& ModVarDatum(int index);

        // EditorNodeRequestsBus::Handler
        Datum* ModInput(const SlotId& slotID) override;
        const Datum* GetInput(const SlotId& slotId) const override;
        ////

        Datum* ModDatumByIndex(size_t index);
        const Datum* GetDatumByIndex(size_t index) const;
        const Slot* GetSlotByIndex(size_t index) const;
        
        SlotId AddSlot(const SlotConfiguration& slotConfiguration);

        // Inserts a slot before the element found at @index. If the index < 0 or >= slots.size(), the slot will be will be added at the end
        SlotId InsertSlot(AZ::s64 index, const SlotConfiguration& slotConfiguration);

        // Removes the slot on this node that matches the supplied slotId
        bool RemoveSlot(const SlotId& slotId, bool removeConnections = true);
        void RemoveConnectionsForSlot(const SlotId& slotId);
        void SignalSlotRemoved(const SlotId& slotId);

        virtual void OnResetDatumToDefaultValue(Datum* datum);

    private:
        // insert or find a slot in the slot list and returns Success if a new slot was inserted.
        // The SlotIterator& parameter is populated with an iterator to the inserted or found slot within the slot list 
        AZ::Outcome<void, AZStd::string> FindOrInsertSlot(AZ::s64 index, const SlotConfiguration& slotConfig, SlotIterator& iterOut);

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

        void ClearDisplayType(const AZ::Crc32& dynamicGroup)
        {
            ExploredDynamicGroupCache cache;
            ClearDisplayType(dynamicGroup, cache);
        }
        void ClearDisplayType(const AZ::Crc32& dynamicGroup, ExploredDynamicGroupCache& exploredCache);

        void SetDisplayType(const AZ::Crc32& dynamicGroup, const Data::Type& dataType)
        {
            ExploredDynamicGroupCache cache;
            SetDisplayType(dynamicGroup, dataType, cache);
        }
        void SetDisplayType(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, ExploredDynamicGroupCache& exploredCache);

        Data::Type GetDisplayType(const AZ::Crc32& dynamicGroup) const;

        bool HasConcreteDisplayType(const AZ::Crc32& dynamicGroup) const
        {
            ExploredDynamicGroupCache cache;
            return HasConcreteDisplayType(dynamicGroup, cache);
        }
        bool HasConcreteDisplayType(const AZ::Crc32& dynamicGroup, ExploredDynamicGroupCache& exploredCache) const;

        bool HasDynamicGroup(const AZ::Crc32& dynamicGroup) const;

        void SetDynamicGroup(const SlotId& slotId, const AZ::Crc32& dynamicGroup);

        bool IsSlotConnectedToConcreteDisplayType(const Slot& slot)
        {
            ExploredDynamicGroupCache cache;
            return IsSlotConnectedToConcreteDisplayType(slot, cache);
        }

        bool IsSlotConnectedToConcreteDisplayType(const Slot& slot, ExploredDynamicGroupCache& exploredGroupCache) const;

        AZ::Outcome<void, AZStd::string> IsValidTypeForGroupInternal(const AZ::Crc32& dynamicGroup, const Data::Type& dataType, ExploredDynamicGroupCache& exploredCache) const;
        
        // restored inputs to graph defaults, if necessary and possible
        void RefreshInput();        

        ////
        // Child Node Interfaces
        virtual bool IsEventHandler() const { return false; }

        //! This is a used by CodeGen to configure slots just prior to OnInit.
        virtual void ConfigureSlots() {}
        //! Entity level initialization, perform any resource allocation here that should be available throughout the node's existence.
        virtual void OnInit() {}

        //! Entity level configuration, perform any post configuration actions on slots here.
        virtual void OnConfigured() {}

        //! Entity level activation, perform entity lifetime setup here, i.e. connect to EBuses
        virtual void OnActivate() {}
        //! Entity level deactivation, perform any entity lifetime release here, i.e disconnect from EBuses
        virtual void OnDeactivate() {}
        //! Any node that is not pure data will perform its operation assuming that all input has been pushed and is reasonably valid
        // perform your derived nodes work in OnInputSignal(const SlotId&)
        virtual void OnInputSignal(const SlotId& slot) {}

        //! Signal sent once the UniqueExecutionId is set.
        virtual void OnGraphSet() {};

        //! Signal sent when a Dynamic Group Display type is changed
        virtual void OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) {};
        ////

        //! Signal when 
        virtual void OnSlotRemoved(const SlotId& slotId) {};

        void ForEachConnectedNode(const Slot& slot, AZStd::function<void(Node&, const SlotId&)> callable) const;
        
        AZStd::vector<Endpoint> GetAllEndpointsByDescriptor(const SlotDescriptor& slotDescriptor, bool allowLatentEndpoints = false) const;

        AZStd::vector<AZStd::pair<const Node*, const SlotId>> GetConnectedNodes(const Slot& slot) const;
        AZStd::vector<AZStd::pair<Node*, const SlotId>> ModConnectedNodes(const Slot& slot) const;

        void ModConnectedNodes(const Slot& slot, AZStd::vector<AZStd::pair<Node*, const SlotId>>&) const;
        bool HasConnectedNodes(const Slot& slot) const;

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

        bool SlotExists(AZStd::string_view name, const SlotDescriptor& slotDescriptor) const;

        virtual void WriteInput(Datum& destination, const Datum& source);
        virtual void WriteInput(Datum& destination, Datum&& source);

        bool IsTargetInDataFlowPath(const ID& targetNodeId, AZStd::unordered_set<ID>& path) const;
        bool IsInEventHandlingScope(const ID& eventHandler, const AZStd::vector<SlotId>& eventSlots, const SlotId& connectionSlot, AZStd::unordered_set<ID>& path) const;
        bool IsOnPureDataThreadHelper(AZStd::unordered_set<ID>& path) const;

        void PopulateNodeType();

    private:
        void SetToDefaultValueOfType(const SlotId& slotID);
        void RebuildInternalState();

        void ProcessDataSlot(Slot& slot);

        void OnNodeStateChanged();

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

        AZ::EntityId m_executionUniqueId;
        NodeTypeIdentifier m_nodeType;

        bool m_enabled = true;

        bool m_queueDisplayUpdates = false;
        AZStd::unordered_map<AZ::Crc32, Data::Type> m_queuedDisplayUpdates;

        SlotList m_slots;
        VariableList m_varDatums;

        AZStd::unordered_set< SlotId > m_removingSlots;

        AZStd::unordered_map<SlotId, VariableInfo> m_slotIdVarInfoMap;

        AZStd::unordered_map<SlotId, SlotIterator> m_slotIdMap;
        AZStd::unordered_multimap<AZStd::string, SlotIterator> m_slotNameMap;

        AZStd::unordered_map<VariableId, VariableIterator> m_varIdMap;
        AZStd::unordered_set<SlotId> m_possiblyStaleInput;

        AZStd::unordered_multimap<AZ::Crc32, SlotId> m_dynamicGroups;
        AZStd::unordered_map<AZ::Crc32, Data::Type> m_dynamicGroupDisplayTypes;

#if !defined(_RELEASE) && defined(AZ_TESTS_ENABLED)
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

        template<size_t... inputDatumIndices>
        friend struct SetDefaultValuesByIndex;
    };

    bool Node::IsNodeObserved(const Node& node)
    {
        bool isObserved{};
        ExecutionNotificationsBus::BroadcastResult(isObserved, &ExecutionNotifications::IsNodeObserved, node);
        return isObserved;
    }

    bool Node::IsVariableObserved(const Node& node, VariableId variableId)
    {
        bool isObserved{};
        ExecutionNotificationsBus::BroadcastResult(isObserved, &ExecutionNotifications::IsVariableObserved, node, variableId);
        return isObserved;
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
                DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = AZStd::string::format("%s: %s", t_Traits::GetResultName(0), Data::GetName(Data::FromAZType<AZStd::decay_t<ResultType>>()).data());
                slotConfiguration.SetType(Data::FromAZType<AZStd::decay_t<ResultType>>());
                slotConfiguration.SetConnectionType(ConnectionType::Output);

                node.AddSlot(slotConfiguration);
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
            template<size_t Index>
            static void CreateDataSlot(Node& node, ConnectionType connectionType)
            {
                DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = AZStd::string::format("%s: %s", t_Traits::GetResultName(Index), Data::GetName(Data::FromAZType<AZStd::decay_t<AZStd::tuple_element_t<Index, ResultType>>>()).data());
                slotConfiguration.SetType(Data::FromAZType<AZStd::decay_t<AZStd::tuple_element_t<Index, ResultType>>>());

                slotConfiguration.SetConnectionType(connectionType);                
                node.AddSlot(slotConfiguration);
            }

            template<AZStd::size_t... Is>
            static void AddOutputSlot(Node& node, AZStd::index_sequence<Is...>)                
            {
                SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(
                    (CreateDataSlot<Is>(node, ConnectionType::Output))
                );
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
