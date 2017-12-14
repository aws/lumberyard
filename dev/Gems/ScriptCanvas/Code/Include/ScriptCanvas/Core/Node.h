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

#include "Core.h"
#include "Slot.h"
#include "Endpoint.h"
#include "SignalBus.h"
#include "NodeBus.h"
#include "Datum.h"
#include "DatumBus.h"

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/algorithm.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Grammar/NodeVisitor.h>
#include <ScriptCanvas/Core/Contracts/TypeContract.h>

#include <AzCore/std/tuple.h>

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


    //! SlotContainer structure where the SlotArray is responsible for the lifetime of the slots
    //! \member m_slots array structure which tracks the insertion order of slots
    //! \member m_slotIdSlotMap map structure which maps Slot Ids to Slot addressed
    //! \member m_slotNameSlotMap map structure which maps Slot Names to Slot addressed
    struct SlotContainer
    {
        AZ_TYPE_INFO(SlotContainer, "{269F9E86-018B-4B9E-96A5-4B15344BD3AD}");
        AZ_CLASS_ALLOCATOR(SlotContainer, AZ::SystemAllocator, 0);

        using SlotArray = AZStd::vector<Slot>;
        using SlotIdMap = AZStd::unordered_map<SlotId, int>;
        using SlotNameMap = AZStd::unordered_multimap<AZStd::string, int>;

        SlotContainer() = default;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SlotContainer>()
                    ->Version(1)
                    ->Field("m_slots", &SlotContainer::m_slots)
                    ->Field("m_slotIdSlotMap", &SlotContainer::m_slotIdSlotMap)
                    ->Field("m_slotNameSlotMap", &SlotContainer::m_slotNameSlotMap)
                    ;
            }
        }

        SlotArray m_slots;
        SlotIdMap m_slotIdSlotMap;
        SlotNameMap m_slotNameSlotMap;
    };

    class Node
        : public AZ::Component
        , public DatumNotificationBus::Handler
        , private SignalBus::Handler
        , NodeRequestBus::Handler
        , EditorNodeRequestBus::Handler
    {
        friend class Graph;
        friend struct BehaviorContextMethodHelper;

    public:
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
                SCRIPT_CANVAS_CALL_ON_INDEX_SEQUENCE(*node.ModInput(indices[Is])->template ModAs<AZStd::decay_t<t_Args>>() = AZStd::forward<t_Args>(args));
            }
        };

        AZ_COMPONENT(Node, "{52B454AE-FA7E-4FE9-87D3-A1CAB235C691}");

        Node();
        ~Node() override;
        Node(const Node&); // Needed just for DLL linkage. Does not perform a copy
        Node& operator=(const Node&); // Needed just for DLL linkage. Does not perform a copy

        static void Reflect(AZ::ReflectContext* reflection);

        static const Node* FindNodeConst(const AZ::EntityId& nodeID);

        static Node* FindNode(const AZ::EntityId& nodeID);

        Graph* GetGraph() const;

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

            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(RTTI_GetType());
            if (classData && classData->m_editData)
            {
                return classData->m_editData->m_name;
            }

            return classData->m_name;
        }

        AZStd::string GetSlotName(const SlotId& slotId) const;

        //! returns a list of all slots, regardless of type
        const AZStd::vector<Slot>& GetSlots() const { return m_slotContainer.m_slots; }

        //! returns a const list of nodes connected to slot(s) of the specified type
        NodePtrConstList GetConnectedNodesByType(SlotType slotType) const;
        AZStd::vector<AZStd::pair<const Node*, SlotId>> GetConnectedNodesAndSlotsByType(SlotType slotType) const;

        bool IsConnected(const Slot& slot) const;

        //////////////////////////////////////////////////////////////////////////
        // NodeRequestBus::Handler
        Slot* GetSlot(const SlotId& slotId) const override;
        AZStd::vector<const Slot*> GetAllSlots() const override;
        SlotId GetSlotId(AZStd::string_view slotName) const override;
        SlotId GetSlotIdByType(AZStd::string_view slotName, SlotType slotType) const override;
        AZStd::vector<SlotId> GetSlotIds(AZStd::string_view slotName) const override;
        const AZ::EntityId& GetGraphId() const override { return m_graphId; }
        bool SlotAcceptsType(const SlotId&, const Data::Type&) const override;
        Data::Type GetSlotDataType(const SlotId& slotId) const override;
        bool IsSlotValidStorage(const SlotId& slotId) const override;
        ////

        // DatumNotificationBus::Handler
        void OnDatumChanged(const Datum* datum) override;
        ////

        //! Implemented by CodeGen, custom nodes may specify default values for properties, these need to be applied during node creation to ensure they don't overwrite user modified values.
        virtual void ApplyPropertyValueDefaults() {}

        //! Some nodes may need to initialize entity references to the entity that owns the graph.
        virtual void InitializeDefaultEntityReferences() {}
        
        bool IsTargetInDataFlowPath(const Node* targetNode) const;
        bool IsInEventHandlingScope(const ID& possibleEventHandler) const;
        
        virtual void MarkDefaultableInput();
        
    protected:
        using SlotList = AZStd::vector<Slot>;

        static const Datum* GetInput(const Node& node, int index);
        static const Datum* GetInput(const Node& node, const SlotId slotID);
        static Datum* ModInput(Node& node, int index);
        static Datum* ModInput(Node& node, const SlotId slotID);
        static void OnInputChanged(Node& node, const Datum& input, const SlotId& slotID);
        static void SetInput(Node& node, const SlotId& id, const Datum& input);

        AZ::EntityId m_graphId;
        SlotContainer m_slotContainer;
        AZStd::vector<Datum> m_inputData;
        AZStd::unordered_map<int, int> m_inputIndexBySlotIndex;
        AZStd::vector<Data::Type> m_nonDatumTypes;
        AZStd::unordered_map<int, int> m_nonDatumTypeIndexBySlotIndex;
        AZStd::unordered_set<SlotId> m_possiblyStaleInput;

        // EditorNodeRequestsBus::Handler
        Datum* ModInput(const SlotId& slotID) override;
        const Datum* GetInput(const SlotId& slotId) const override;
        AZ::EntityId GetGraphEntityId() const override;
        ////

        Datum* ModInput(int index);
        const Datum* GetInput(int index) const;
        virtual void SetDefault(const SlotId& slotID);

        // add a typed input slot, and (generic) storage for the input
        struct SlotConfiguration
        {
            AZ_CLASS_ALLOCATOR(SlotConfiguration, AZ::SystemAllocator, 0);
            SlotConfiguration(AZStd::string_view name, AZStd::string_view toolTip, SlotType type, const AZStd::vector<ContractDescriptor>& contractDescs = AZStd::vector<ContractDescriptor>{}, bool addUniqueSlotByNameAndType = true)
                : m_name(name)
                , m_toolTip(toolTip)
                , m_type(type)
                , m_contractDescs(contractDescs)
                , m_addUniqueSlotByNameAndType(addUniqueSlotByNameAndType)
            {}

            AZStd::string_view m_name;
            AZStd::string_view m_toolTip;
            SlotType m_type = SlotType::None;
            const AZStd::vector<ContractDescriptor>& m_contractDescs;
            bool m_addUniqueSlotByNameAndType = true; // Only adds a new slot if a slot with the supplied name and SlotType does not exist on the node
        };

        SlotId AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, Datum::eOriginality originality, bool addUniqueSlotByNameAndType = true);
        SlotId AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, const void* source, Datum::eOriginality originality, bool addUniqueSlotByNameAndType = true);
        SlotId AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZ::BehaviorParameter& typeDesc, Datum::eOriginality originality, bool addUniqueSlotByNameAndType = true);
        template<typename DatumType>
        SlotId AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, Datum::eOriginality originality, DatumType&& defaultValue, bool addUniqueSlotByNameAndType = true);
        // add an input slot, with a type determined by the first connection
        SlotId AddInputDatumDynamicTypedSlot(AZStd::string_view name, AZStd::string_view toolTip = AZStd::string_view(), bool addUniqueSlotByNameAndType = true);
        // add an untyped input slot (for input overloaded methods on all types, like print)
        SlotId AddInputDatumUntypedSlot(AZStd::string_view name, const AZStd::vector<ContractDescriptor>* contracts = nullptr, AZStd::string_view toolTip = AZStd::string_view(), bool addUniqueSlotByNameAndType = true);
        // add a typed input slot, but do not add storage for the input
        enum class InputTypeContract { CustomType, DatumType, None };
        SlotId AddInputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, InputTypeContract contractType, bool addUniqueSlotByNameAndType = true);
        SlotId AddInputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZ::BehaviorParameter& typeDesc, InputTypeContract contractType, bool addUniqueSlotByNameAndType = true);
        // add a type not associated with a stored datum
        void AddNonDatumType(const Data::Type& type, int slotIndex);
        // add a typed output slot
        enum class OutputStorage { Optional, Required };
        SlotId AddOutputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, OutputStorage outputStorage, bool addUniqueSlotByNameAndType = true);
        // Adds or finds a slot with the corresponding name, type, contracts. This operation is idempotent only if addUniqueSlotByNameAndType is true
        SlotId AddSlot(const SlotConfiguration&);
        // Adds or finds a slot with the corresponding name, type, contracts. This operation is idempotent only if addUniqueSlotByNameAndType is true
        SlotId AddSlot(AZStd::string_view name, AZStd::string_view toolTip, SlotType type, const AZStd::vector<ContractDescriptor>& contractDescs = AZStd::vector<ContractDescriptor>{}, bool addUniqueSlotByNameAndType = true);
        // return success/failure and resulting slot index and slotID
        bool AddSlotInternal(const SlotConfiguration&, SlotId& slotIDOut, int& indexOut);
        // restored inputs to graph defaults, if necessary and possible
        void RefreshInput();
        // Removes the slot on this node that matches the supplied slotId
        bool RemoveSlot(const SlotId& slotId);

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

        template<typename t_Callable>
        void ForEachConnectedNode(const Slot& slot, t_Callable&& callable) const
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

        AZStd::vector<Endpoint> GetEndpointsByType(SlotType slotType) const;
        AZStd::vector<AZStd::pair<const Node*, const SlotId>> GetConnectedNodes(const Slot& slot) const;
        const Datum* GetInputBySlotIndex(int index) const;
        const Data::Type* GetNonDatumType(int index) const;
        const Data::Type* GetNonDatumType(const SlotId& slotID) const;
        const Data::Type* GetNonDatumTypeBySlotIndex(int index) const;
        AZStd::vector<const Slot*> GetSlotsByType(SlotType slotType) const;
        bool GetValidSlotIndex(const SlotId& slotID, const Slot*& slotOut, int& slotIndexOut) const;
        bool GetValidInputSlotId(int inputIndex, SlotId& slotIdOut) const;
        bool GetValidInputDataIndex(const int slotIndex, int& inputDatumIndexOut) const;
        bool GetValidNonDatumTypeIndex(const int slotIndex, int& outputDatumIndexOut) const;
        AZStd::vector<AZStd::pair<Node*, const SlotId>> ModConnectedNodes(const Slot& slot) const;
        SlotContainer& ModSlotContainer() { return m_slotContainer; }

        virtual void OnInputChanged(const Datum& input, const SlotId& slotID) {}

        //////////////////////////////////////////////////////////////////////////
        //! The body of the node's execution, implement the node's work here.
        void PushOutput(const Datum& output, const Slot& slot) const;
        void SetGraph(Graph* graph);
        virtual void OnGraphSet(Graph* graph) {}
        // on activate, simple expressions need to push this data
        virtual void SetInput(const Datum& input, const SlotId& id);

        //! Any node that is not pure data will perform its operation assuming that all input has been pushed and is reasonably valid
        // perform your derived nodes work in OnInputSignal(const SlotId&)
        void SignalInput(const SlotId& slot) override final;
        //! This must be called manually to send execution out of a specific slot
        void SignalOutput(const SlotId& slot) override final;

        bool SlotExists(AZStd::string_view name, SlotType type) const;
        bool SlotExists(AZStd::string_view name, SlotType type, SlotId& out) const;
        virtual void WriteInput(Datum& destination, const Datum& source);

        void SetDatumLabel(const SlotId& slotID, AZStd::string_view name);

        //! When EntityId data input into a slot store the graph's unique Id, they need to be updated
        //! to the EntityId of the entity that owns the graph (i.e. "Self")
        void ResolveSelfEntityReferences(const AZ::EntityId& graphOwnerId);

        bool IsTargetInDataFlowPath(const ID& targetNodeId, AZStd::unordered_set<ID>& path) const;
        bool IsInEventHandlingScope(const ID& eventHandler, const AZStd::vector<SlotId>& eventSlots, const SlotId& connectionSlot, AZStd::unordered_set<ID>& path) const;


        //////////////////////////////////////////////////////////////////////////
        /// VM 0.1 begin
    public:
        virtual void Visit(NodeVisitor& visitor) const { AZ_Assert(false, "This class isn't abstract (yet), but this function must be overridden in your node: %s", GetDebugName()); }
        const Node* GetNextExecutableNode() const;
        /// VM 0.1 end
        //////////////////////////////////////////////////////////////////////////

#if defined(AZ_TESTS_ENABLED)
    public:
        template<typename t_Value>
        const t_Value* GetInput_UNIT_TEST(const char* slotName)
        {
            const Datum* input = GetInput(GetSlotId(slotName));
            return input ? input->GetAs<t_Value>() : nullptr;
        }

        template<typename t_Value>
        t_Value* ModInput_UNIT_TEST(const char* slotName)
        {
            Datum* input = ModInput(GetSlotId(slotName));
            return input ? input->ModAs<t_Value>() : nullptr;
        }

        // initializes the node input to the value passed in, not a pointer to it
        // this distinction really only applied to vectors, for now (soon for all native SC math types)
        template<typename t_Value>
        void SetInput_UNIT_TEST(const char* slotName, const t_Value& value)
        {
            Datum input = Datum::CreateInitializedCopy(value);
            SetInput(input, GetSlotId(slotName));
        }

        AZ_INLINE const Datum* GetInput_UNIT_TEST(int index) const
        {
            return GetInput(index);
        }

        AZ_INLINE void SetInputDatum_UNIT_TEST(int index, const Datum& input)
        {
            *ModInput(index) = input;
        }

        // initializes the node input to the ADDRESS of the value passed in, so you get a behavior context object with a pointer to the value
        // this distinction really only applied to vectors, for now (soon for all native SC math types)
        template<typename t_Value>
        void SetInputFromBehaviorContext_UNIT_TEST(const char* slotName, const t_Value& value)
        {
            Datum input = Datum::CreateInitializedCopyFromBehaviorContext(value);
            SetInput(input, GetSlotId(slotName));
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
        SlotId slotIDOut;
        if (SlotExists(name, SlotType::DataIn, slotIDOut))
        {
            // Re-apply the label when the node is created from pasting.
            SetDatumLabel(slotIDOut, name.data());
        }
        else
        {
            AZStd::vector<ContractDescriptor> contracts;
            contracts.push_back({ []() { return aznew TypeContract(); } });

            int slotIndex;
            if (AddSlotInternal({ name, toolTip, SlotType::DataIn, contracts }, slotIDOut, slotIndex))
            {
                const int inputIndex = azlossy_cast<int>(m_inputData.size());
                m_inputData.emplace_back(Datum::CreateInitializedCopy(AZStd::forward<DatumType>(defaultValue)));

                Datum& datum = m_inputData[inputIndex];
                datum.SetNotificationsTarget(GetEntityId());
                datum.SetLabel(name.data());

                m_inputIndexBySlotIndex.insert(AZStd::make_pair(slotIndex, inputIndex));
            }
        }

        return slotIDOut;
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
                node.AddOutputTypeSlot(AZStd::string::format("%s: %s", t_Traits::GetResultName(0), Data::GetName(Data::FromAZType<AZStd::decay_t<ResultType>>())),
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
                    node.AddOutputTypeSlot(AZStd::string::format("%s: %s", t_Traits::GetResultName(Is), Data::GetName(Data::FromAZType<AZStd::decay_t<AZStd::tuple_element_t<Is, ResultType>>>())),
                        AZStd::string_view(), Data::FromAZType<AZStd::decay_t<AZStd::tuple_element_t<Is, ResultType>>>(), Node::OutputStorage::Optional));
            }
        };


        template<typename ResultType, typename t_Traits, typename = AZStd::void_t<>>
        struct PushOutputHelper
        {
            template<size_t... Is>
            static void PushOutput(Node& node, ResultType&& result, AZStd::index_sequence<Is...>)
            {
                node.PushOutput(Datum::CreateInitializedCopy(AZStd::forward<ResultType>(result)), node.GetSlots()[t_Traits::s_resultsSlotIndicesStart]);
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
                    Datum::CreateInitializedCopy(AZStd::get<Is>(AZStd::forward<ResultType>(tupleResult))),
                    node.GetSlots()[t_Traits::s_resultsSlotIndicesStart + Is]));
            }
        };

        template<typename ResultType, typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        struct CallHelper
        {
            template<typename... t_Args, size_t... ArgIndices>
            static void Call(Node& node, AZStd::Internal::pack_traits_arg_sequence<t_Args...>, AZStd::index_sequence<ArgIndices...>)
            {
                PushOutputHelper<ResultType, t_Traits>::PushOutput(node, AZStd::invoke(function, *node.GetInput(ArgIndices)->GetAs<AZStd::decay_t<t_Args>>()...), ResultIndexSequence());
            }
        };

        template<typename ResultIndexSequence, typename t_Func, t_Func function, typename t_Traits>
        struct CallHelper<void, ResultIndexSequence, t_Func, function, t_Traits>
        {
            template<typename... t_Args, size_t... ArgIndices>
            static void Call(Node& node, AZStd::Internal::pack_traits_arg_sequence<t_Args...>, AZStd::index_sequence<ArgIndices...>)
            {
                AZStd::invoke(function, *node.GetInput(ArgIndices)->GetAs<AZStd::decay_t<t_Args>>()...);
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