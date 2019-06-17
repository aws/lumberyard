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
#include "ScriptCanvasTestNodes.h"

#include <AzCore/Component/EntityId.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/Libraries/Core/Core.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>
#include <ScriptCanvas/Libraries/Core/BehaviorContextObjectNode.h>
#include <ScriptCanvas/Libraries/Entity/EntityRef.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Logic/Logic.h>
#include <ScriptCanvas/Libraries/Math/Math.h>
#include <ScriptCanvas/Libraries/Comparison/Comparison.h>

#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    class RuntimeAsset;
}

namespace ScriptCanvasTests
{
    template<typename t_NodeType>
    t_NodeType* GetTestNode(const AZ::EntityId& graphUniqueId, const AZ::EntityId& nodeID)
    {
        using namespace ScriptCanvas;
        t_NodeType* node{};
        SystemRequestBus::BroadcastResult(node, &SystemRequests::GetNode<t_NodeType>, nodeID);
        EXPECT_TRUE(node != nullptr);
        return node;
    }

    template<typename t_NodeType>
    t_NodeType* CreateTestNode(const AZ::EntityId& graphUniqueId, AZ::EntityId& entityOut)
    {
        using namespace ScriptCanvas;

        AZ::Entity* entity{ aznew AZ::Entity };
        entity->Init();
        entityOut = entity->GetId();
        EXPECT_TRUE(entityOut.IsValid());
        SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, entityOut, graphUniqueId, azrtti_typeid<t_NodeType>());
        return GetTestNode<t_NodeType>(graphUniqueId, entityOut);
    }
    
    ScriptCanvas::Node* CreateDataNodeByType(const AZ::EntityId& graphUniqueId, const ScriptCanvas::Data::Type& type, AZ::EntityId& nodeIDout);

    template<typename t_Value>
    ScriptCanvas::Node* CreateDataNode(const AZ::EntityId& graphUniqueId, const t_Value& value, AZ::EntityId& nodeIDout)
    {
        using namespace ScriptCanvas;
        const Data::Type operandType = Data::FromAZType(azrtti_typeid<t_Value>());
        Node* node = CreateDataNodeByType(graphUniqueId, operandType, nodeIDout);

        EXPECT_NE(node, nullptr);
        if (node)
        {
            node->SetInput_UNIT_TEST("Set", value);
        }
        return node;
    }

    template<typename t_Value>
    ScriptCanvas::VariableId CreateVariable(AZ::EntityId graphUniqueId, const t_Value& value, AZStd::string_view variableName)
    {
        using namespace ScriptCanvas;
        AZ::Outcome<VariableId, AZStd::string> addVariableOutcome = AZ::Failure(AZStd::string());
        GraphVariableManagerRequestBus::EventResult(addVariableOutcome, graphUniqueId, &GraphVariableManagerRequests::AddVariable, variableName, Datum(value));
        if (!addVariableOutcome)
        {
            AZ_Warning("Script Canvas Test", false, "%s", addVariableOutcome.GetError().data());
            return {};
        }

        return addVariableOutcome.TakeValue();
    }

    ScriptCanvas::Nodes::Core::BehaviorContextObjectNode* CreateTestObjectNode(const AZ::EntityId& graphUniqueId, AZ::EntityId& entityOut, const AZ::Uuid& objectTypeID);
    AZ::EntityId CreateClassFunctionNode(const AZ::EntityId& graphUniqueId, const AZStd::string& className, const AZStd::string& methodName);
    const char* SlotTypeToString(ScriptCanvas::SlotType type);
    void DumpSlots(const ScriptCanvas::Node& node);
    bool Connect(ScriptCanvas::Graph& graph, const AZ::EntityId& fromNodeID, const char* fromSlotName, const AZ::EntityId& toNodeID, const char* toSlotName, bool dumpSlotsOnFailure = true);

    AZ::Data::Asset<ScriptCanvas::RuntimeAsset> CreateRuntimeAsset(ScriptCanvas::Graph* graph);

    class UnitTestEvents
        : public AZ::EBusTraits
    {
    public:
        virtual void Failed(const AZStd::string& description) = 0;
        virtual AZStd::string Result(const AZStd::string& description) = 0;
        virtual void SideEffect(const AZStd::string& description) = 0;
        virtual void Succeeded(const AZStd::string& description) = 0;
    };

    using UnitTestEventsBus = AZ::EBus<UnitTestEvents>;

    class UnitTestEventsHandler
        : public UnitTestEventsBus::Handler
    {
    public:
        ~UnitTestEventsHandler()
        {
            Clear();
        }

        void Clear()
        {
            for (auto iter : m_failures)
            {
                ADD_FAILURE() << iter.first.c_str();
            }

            for (auto iter : m_successes)
            {
                GTEST_SUCCEED();
            }

            m_failures.clear();
            m_results.clear();
            m_successes.clear();
            m_results.clear();
        }

        void Failed(const AZStd::string& description)
        {
            auto iter = m_failures.find(description);
            if (iter != m_failures.end())
            {
                ++iter->second;
            }
            else
            {
                m_failures.insert(AZStd::make_pair(description, 1));
            }
        }

        int FailureCount(const AZStd::string& description) const
        {
            auto iter = m_failures.find(description);
            return iter != m_failures.end() ? iter->second : 0;
        }

        bool IsFailed() const
        {
            return !m_failures.empty();
        }

        bool IsSucceeded() const
        {
            return !m_successes.empty();
        }

        void SideEffect(const AZStd::string& description)
        {
            auto iter = m_sideEffects.find(description);
            if (iter != m_sideEffects.end())
            {
                ++iter->second;
            }
            else
            {
                m_sideEffects.insert(AZStd::make_pair(description, 1));
            }
        }


        int SideEffectCount() const
        {
            int count(0);

            for (auto iter : m_sideEffects)
            {
                count += iter.second;
            }

            return count;
        }

        int SideEffectCount(const AZStd::string& description) const
        {
            auto iter = m_sideEffects.find(description);
            return iter != m_sideEffects.end() ? iter->second : 0;
        }

        void Succeeded(const AZStd::string& description)
        {
            auto iter = m_successes.find(description);

            if (iter != m_successes.end())
            {
                ++iter->second;
            }
            else
            {
                m_successes.insert(AZStd::make_pair(description, 1));
            }
        }

        int SuccessCount(const AZStd::string& description) const
        {
            auto iter = m_successes.find(description);
            return iter != m_successes.end() ? iter->second : 0;
        }

        AZStd::string Result(const AZStd::string& description)
        {
            auto iter = m_results.find(description);

            if (iter != m_results.end())
            {
                ++iter->second;
            }
            else
            {
                m_results.insert(AZStd::make_pair(description, 1));
            }

            return description;
        }

        int ResultCount(const AZStd::string& description) const
        {
            auto iter = m_results.find(description);
            return iter != m_results.end() ? iter->second : 0;
        }

    private:
        AZStd::unordered_map<AZStd::string, int> m_failures;
        AZStd::unordered_map<AZStd::string, int> m_sideEffects;
        AZStd::unordered_map<AZStd::string, int> m_successes;
        AZStd::unordered_map<AZStd::string, int> m_results;
    };

    struct TestBehaviorContextProperties
    {
        AZ_TYPE_INFO(TestBehaviorContextProperties, "{13ECCA93-9A62-4A46-846C-ABFEEA215C48}");

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<TestBehaviorContextProperties>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->Class<TestBehaviorContextProperties>("TestBehaviorContextProperties")
                    ->Property("boolean", BehaviorValueGetter(&TestBehaviorContextProperties::m_booleanProp), BehaviorValueSetter(&TestBehaviorContextProperties::m_booleanProp))
                    ->Property("number", BehaviorValueGetter(&TestBehaviorContextProperties::m_numberProp), BehaviorValueSetter(&TestBehaviorContextProperties::m_numberProp))
                    ->Property("string", BehaviorValueGetter(&TestBehaviorContextProperties::m_stringProp), BehaviorValueSetter(&TestBehaviorContextProperties::m_stringProp))
                    ->Property("getterOnlyNumber", BehaviorValueGetter(&TestBehaviorContextProperties::m_getterOnlyProp), nullptr)
                    ->Property("setterOnlyNumber", nullptr, BehaviorValueSetter(&TestBehaviorContextProperties::m_setterOnlyProp))
                    ;
            }
        }

        ScriptCanvas::Data::BooleanType m_booleanProp = false;
        ScriptCanvas::Data::NumberType m_numberProp = 0.0f;
        ScriptCanvas::Data::StringType m_stringProp;
        ScriptCanvas::Data::NumberType m_getterOnlyProp = 0.0f;
        ScriptCanvas::Data::NumberType m_setterOnlyProp = 0.0f;
    };

    class TestBehaviorContextObject
    {
    public:
        AZ_TYPE_INFO(TestBehaviorContextObject, "{52DA0875-8EF6-4D39-9CF6-9F83BAA59A1D}");

        static TestBehaviorContextObject MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs);

        static const TestBehaviorContextObject* MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs);

        static const TestBehaviorContextObject& MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs);

        static int MaxReturnByValueInteger(int lhs, int rhs);

        static const int* MaxReturnByPointerInteger(const int* lhs, const int* rhs);

        static const int& MaxReturnByReferenceInteger(const int& lhs, const int& rhs);

        static void Reflect(AZ::ReflectContext* reflectContext);

        static AZ::u32 GetCreatedCount() { return s_createdCount; }
        static AZ::u32 GetDestroyedCount() { return s_destroyedCount; }
        static void ResetCounts() { s_createdCount = s_destroyedCount = 0; }

        TestBehaviorContextObject()
        {
            ++s_createdCount;
        }

        TestBehaviorContextObject(const TestBehaviorContextObject& src)
            : m_value(src.m_value)
        {
            ++s_createdCount;
        }

        TestBehaviorContextObject(TestBehaviorContextObject&& src)
            : m_value(src.m_value)
        {
            ++s_createdCount;
        }

        TestBehaviorContextObject(int value)
            : m_value(value)
        {
            ++s_createdCount;
        }

        ~TestBehaviorContextObject()
        {
            AZ::u32 destroyedCount = ++s_destroyedCount;
            AZ::u32 createdCount = s_createdCount;
            AZ_Assert(destroyedCount <= createdCount, "ScriptCanvas execution error");
        }

        TestBehaviorContextObject& operator=(const TestBehaviorContextObject& source)
        {
            m_value = source.m_value;
            return *this;
        }

        // make a to string

        int GetValue() const { return m_value; }
        void SetValue(int value) { m_value = value; }

        bool IsNormalized() const { return m_isNormalized; }
        void Normalize() { m_isNormalized = true; }
        void Denormalize() { m_isNormalized = false; }

        bool operator>(const TestBehaviorContextObject& other) const
        {
            return m_value > other.m_value;
        }

        bool operator>=(const TestBehaviorContextObject& other) const
        {
            return m_value >= other.m_value;
        }

        bool operator==(const TestBehaviorContextObject& other) const
        {
            return m_value == other.m_value;
        }

        bool operator!=(const TestBehaviorContextObject& other) const
        {
            return m_value != other.m_value;
        }

        bool operator<=(const TestBehaviorContextObject& other) const
        {
            return m_value <= other.m_value;
        }

        bool operator<(const TestBehaviorContextObject& other) const
        {
            return m_value < other.m_value;
        }

    private:
        int m_value = -1;
        bool m_isNormalized = false;
        static AZ::u32 s_createdCount;
        static AZ::u32 s_destroyedCount;
    };

    TestBehaviorContextObject MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs);
    const TestBehaviorContextObject* MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs);
    const TestBehaviorContextObject& MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs);

    class EventTest
        : public AZ::ComponentBus
    {
    public:
        virtual void Event() = 0;
        virtual int Number(int number) = 0;
        virtual AZStd::string String(const AZStd::string& string) = 0;
        virtual TestBehaviorContextObject Object(const TestBehaviorContextObject& vector) = 0;
    };

    using EventTestBus = AZ::EBus<EventTest>;

    class EventTestHandler
        : public EventTestBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER
            ( EventTestHandler
            , "{29E7F7EE-0867-467A-8389-68B07C184109}"
            , AZ::SystemAllocator
            , Event
            , Number
            , String
            , Object);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EventTestHandler>()
                    ->Version(0)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EventTestBus>("EventTestHandler")
                    ->Handler<EventTestHandler>()
                    ;
            }
        }

        void Event() override
        {
            Call(FN_Event);
        }

        int Number(int input) override
        {
            int output;
            CallResult(output, FN_Number, input);
            return output;
        }

        AZStd::string String(const AZStd::string& input) override
        {
            AZStd::string output;
            CallResult(output, FN_String, input);
            return output;
        }

        TestBehaviorContextObject Object(const TestBehaviorContextObject& input) override
        {
            TestBehaviorContextObject output;
            CallResult(output, FN_Object, input);
            return output;
        }
    }; // EventTestHandler

    class ConvertibleToString
    {
    public:
        AZ_TYPE_INFO(StringView, "{DBF947E7-4097-4C5D-AF0D-E2DB311E8958}");

        static AZStd::string ConstCharPtrToString(const char* inputStr)
        {
            return AZStd::string_view(inputStr);
        }

        static AZStd::string StringViewToString(AZStd::string_view inputView)
        {
            return inputView;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ConvertibleToString>()
                    ->Version(0)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<ConvertibleToString>("ConvertibleToString")
                    ->Method("ConstCharPtrToString", &ConvertibleToString::ConstCharPtrToString)
                    ->Method("StringViewToString", &ConvertibleToString::StringViewToString)
                    ;
            }
        }
    };

    class TraceSuppression
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void SuppressPreAssert(bool suppress) = 0;
        virtual void SuppressAssert(bool suppress) = 0;
        virtual void SuppressException(bool suppress) = 0;
        virtual void SuppressPreError(bool suppress) = 0;
        virtual void SuppressError(bool suppress) = 0;
        virtual void SuppressPreWarning(bool suppress) = 0;
        virtual void SuppressWarning(bool suppress) = 0;
        virtual void SuppressPrintf(bool suppress) = 0;
        virtual void SuppressAllOutput(bool suppress) = 0;
    };

    using TraceSuppressionBus = AZ::EBus<TraceSuppression>;

    class TraceMessageComponent
        : public AZ::Component
        , protected AZ::Debug::TraceMessageBus::Handler
        , protected TraceSuppressionBus::Handler
    {
    public:
        AZ_COMPONENT(TraceMessageComponent, "{E12144CE-809D-4056-9735-4384D7DBCCDC}");

        TraceMessageComponent() = default;
        ~TraceMessageComponent() override = default;

        void Activate() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
            TraceSuppressionBus::Handler::BusConnect();
        }
        void Deactivate() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            TraceSuppressionBus::Handler::BusDisconnect();
        }

    private:

        /// \ref ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TraceMessageComponent, AZ::Component>()
                    ;
            }
        }

        bool OnPreAssert(const char*, int, const char*, const char*) { return suppressPreAssert; }
        bool OnAssert(const char*) { return suppressAssert; }
        bool OnException(const char*) { return suppressException; }
        bool OnPreError(const char*, const char*, int, const char*, const char*) { return suppressPreError; }
        bool OnError(const char*, const char*) { return suppressError; }
        bool OnPreWarning(const char*, const char*, int, const char*, const char*) { return suppressPreWarning; }
        bool OnWarning(const char*, const char*) { return suppressWarning; }
        bool OnPrintf(const char*, const char*) { return suppressPrintf; }
        bool OnOutput(const char*, const char*) { return suppressAllOutput; }

        void SuppressPreAssert(bool suppress) override { suppressPreAssert = suppress; }
        void SuppressAssert(bool suppress)override { suppressAssert = suppress; }
        void SuppressException(bool suppress) override { suppressException = suppress; }
        void SuppressPreError(bool suppress) override { suppressPreError = suppress; }
        void SuppressError(bool suppress) override { suppressPreError = suppress; }
        void SuppressPreWarning(bool suppress) override { suppressPreWarning = suppress; }
        void SuppressWarning(bool suppress) override { suppressWarning = suppress; }
        void SuppressPrintf(bool suppress) override { suppressPrintf = suppress; }
        void SuppressAllOutput(bool suppress) override { suppressAllOutput = suppress; }

        bool suppressPreAssert = false;
        bool suppressAssert = false;
        bool suppressException = false;
        bool suppressPreError = false;
        bool suppressError = false;
        bool suppressPreWarning = false;
        bool suppressWarning = false;
        bool suppressPrintf = false;
        bool suppressAllOutput = false;
    };

    struct ScopedOutputSuppression
    {
        ScopedOutputSuppression(bool suppressState = true)
        {
            AZ::Debug::TraceMessageBus::BroadcastResult(m_oldSuppression, &AZ::Debug::TraceMessageEvents::OnOutput, "", "");
            TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressAllOutput, suppressState);
        }

        ~ScopedOutputSuppression()
        {
            TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressAllOutput, m_oldSuppression);
        }
    private:
        bool m_oldSuppression = false;
    };

    class UnitTestEntityContext
        : public AzFramework::EntityIdContextQueryBus::MultiHandler
        , public AzFramework::EntityContextRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(UnitTestEntityContext, AZ::SystemAllocator, 0);

        UnitTestEntityContext() { AzFramework::EntityContextRequestBus::Handler::BusConnect(GetOwningContextId()); }

        //// EntityIdContextQueryBus::MultiHandler
        // Returns a generate Uuid which represents the UnitTest EntityContext
        AzFramework::EntityContextId GetOwningContextId() override { return AzFramework::EntityContextId("{36591268-5CE9-4BC1-8277-0AB9AD528447}"); }
        AZ::SliceComponent::SliceInstanceAddress GetOwningSlice() override { return {}; }
        ////

        //// EntityContextRequestBus::Handler
        AZ::SliceComponent* GetRootSlice() override { return {}; }
        AZ::Data::AssetId CurrentlyInstantiatingSlice() override { return AZ::Data::AssetId(); }
        AZ::Entity* CreateEntity(const char* name) override;

        void AddEntity(AZ::Entity* entity) override;

        void ActivateEntity(AZ::EntityId entityId) override;
        void DeactivateEntity(AZ::EntityId entityId) override;
        bool DestroyEntity(AZ::Entity* entity) override;
        bool DestroyEntityById(AZ::EntityId entityId) override;
        AZ::Entity* CloneEntity(const AZ::Entity& sourceEntity) override;
        const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& GetLoadedEntityIdMap() override { return m_unitTestEntityIdMap; }
        void ResetContext() override;
        ////

        void AddEntity(AZ::EntityId entityId);
        void RemoveEntity(AZ::EntityId entityId);
        bool IsOwnedByThisContext(AZ::EntityId entityId) { return m_unitTestEntityIdMap.count(entityId) != 0; }

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_unitTestEntityIdMap;
    };

    template<typename t_Operator, typename t_Operand, typename t_Value>
    void BinaryOpTest(const t_Operand& lhs, const t_Operand& rhs, const t_Value& expectedResult, bool forceGraphError = false)
    {
        using namespace ScriptCanvas;
        using namespace Nodes;
        AZ::Entity graphEntity("Graph");
        graphEntity.Init();
        SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, &graphEntity);
        auto graph = graphEntity.FindComponent<Graph>();
        EXPECT_NE(nullptr, graph);

        const AZ::EntityId& graphEntityId = graph->GetEntityId();
        const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

        AZ::EntityId startID;
        CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
        AZ::EntityId operatorID;
        CreateTestNode<t_Operator>(graphUniqueId, operatorID);
        AZ::EntityId lhsID;
        CreateDataNode(graphUniqueId, lhs, lhsID);
        AZ::EntityId rhsID;
        CreateDataNode(graphUniqueId, rhs, rhsID);
        AZ::EntityId printID;
        auto printNode = CreateTestNode<TestNodes::TestResult>(graphUniqueId, printID);

        EXPECT_TRUE(Connect(*graph, lhsID, "Get", operatorID, BinaryOperator::k_lhsName));

        if (!forceGraphError)
        {
            EXPECT_TRUE(Connect(*graph, rhsID, "Get", operatorID, BinaryOperator::k_rhsName));
        }

        EXPECT_TRUE(Connect(*graph, operatorID, BinaryOperator::k_resultName, printID, "Value"));
        EXPECT_TRUE(Connect(*graph, startID, "Out", operatorID, BinaryOperator::k_evaluateName));

        bool isArithmetic = AZStd::is_base_of<ScriptCanvas::Nodes::ArithmeticExpression, t_Operator>::value;
        if (isArithmetic)
        {
            EXPECT_TRUE(Connect(*graph, operatorID, BinaryOperator::k_outName, printID, "In"));
        }
        else
        {
            EXPECT_TRUE(Connect(*graph, operatorID, BinaryOperator::k_onTrue, printID, "In"));
            EXPECT_TRUE(Connect(*graph, operatorID, BinaryOperator::k_onFalse, printID, "In"));
        }

        {
            ScopedOutputSuppression suppressOutput;
            graph->GetEntity()->Activate();
            EXPECT_EQ(graph->IsInErrorState(), forceGraphError);
        }

        if (!forceGraphError)
        {
            if (auto result = printNode->GetInput_UNIT_TEST<t_Value>("Value"))
            {
                EXPECT_EQ(*result, expectedResult);
            }
            else
            {
                ADD_FAILURE();
            }
        }

        graph->GetEntity()->Deactivate();
        delete graph;
    }

    template<typename t_Value>
    void AssignTest(const t_Value& lhs, const t_Value& rhs, UnitTestEntityContext* entityContext = {})
    {
        using namespace ScriptCanvas;
        using namespace Nodes;
        AZ::Entity graphEntity("Graph");
        if (entityContext)
        {
            entityContext->AddEntity(graphEntity.GetId());
        }
        graphEntity.Init();
        Graph* graph{};
        SystemRequestBus::BroadcastResult(graph, &SystemRequests::CreateGraphOnEntity, &graphEntity);
        EXPECT_NE(nullptr, graph);
        EXPECT_NE(lhs, rhs);

        const AZ::EntityId& graphEntityId = graph->GetEntityId();
        const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

        AZ::EntityId startID;
        CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
        AZ::EntityId operatorID;
        CreateTestNode<Nodes::Core::Assign>(graphUniqueId, operatorID);
        AZ::EntityId lhsID;
        auto sourceNode = CreateDataNode(graphUniqueId, lhs, lhsID);
        AZ::EntityId rhsID;
        auto targetNode = CreateDataNode(graphUniqueId, rhs, rhsID);

        EXPECT_TRUE(Connect(*graph, lhsID, "Get", operatorID, "Source"));
        EXPECT_TRUE(Connect(*graph, rhsID, "Set", operatorID, "Target"));

        EXPECT_TRUE(Connect(*graph, startID, "Out", operatorID, "In"));

        graph->GetEntity()->Activate();
        EXPECT_FALSE(graph->IsInErrorState());

        if (auto result = targetNode->template GetInput_UNIT_TEST<t_Value>("Set"))
        {
            EXPECT_EQ(*result, lhs);
        }
        else
        {
            ADD_FAILURE();
        }

        graph->GetEntity()->Deactivate();
        if (entityContext)
        {
            entityContext->RemoveEntity(graphEntity.GetId());
        }
    }

    template<typename TBusIdType>
    class TemplateEventTest
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = TBusIdType;

        virtual void GenericEvent() = 0;
        virtual AZStd::string UpdateNameEvent(AZStd::string_view) = 0;
        virtual AZ::Vector3 VectorCreatedEvent(const AZ::Vector3&) = 0;
    };

    template<typename TBusIdType>
    using TemplateEventTestBus = AZ::EBus<TemplateEventTest<TBusIdType>>;

    template<typename TBusIdType>
    class TemplateEventTestHandler
        : public TemplateEventTestBus<TBusIdType>::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER_TEMPLATE(TemplateEventTestHandler, (TemplateEventTestHandler, "{AADA0906-C216-4D98-BD51-76BB0C2F7FAF}", TBusIdType), AZ::SystemAllocator
            , GenericEvent
            , UpdateNameEvent
            , VectorCreatedEvent
        );

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                const AZStd::string ebusName(AZ::AzTypeInfo<TemplateEventTestHandler>::Name());
                behaviorContext->EBus<TemplateEventTestBus<TBusIdType>>(ebusName.data())
                    ->template Handler<TemplateEventTestHandler>()
                    ->Event("Update Name", &TemplateEventTestBus<TBusIdType>::Events::UpdateNameEvent)
                    ;
            }
        }

        void GenericEvent() override
        {
            Call(FN_GenericEvent);
        }

        AZStd::string UpdateNameEvent(AZStd::string_view name) override
        {
            AZStd::string result;
            CallResult(result, FN_UpdateNameEvent, name);
            return result;
        }

        AZ::Vector3 VectorCreatedEvent(const AZ::Vector3& newVector) override
        {
            AZ::Vector3 result;
            CallResult(result, FN_VectorCreatedEvent, newVector);
            return result;
        }
    };


    class ScriptUnitTestEventHandler
        : public UnitTestEventsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER
        (ScriptUnitTestEventHandler
            , "{3FC89358-837D-43D5-8C4F-61B672A751B2}"
            , AZ::SystemAllocator
            , Failed
            , Result
            , SideEffect
            , Succeeded);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptUnitTestEventHandler>()
                    ->Version(0)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<UnitTestEventsBus>("UnitTestEventsBus")
                    ->Handler<ScriptUnitTestEventHandler>()
                    ->Event("Failed", &UnitTestEventsBus::Events::Failed)
                    ->Event("Result", &UnitTestEventsBus::Events::Result)
                    ->Event("SideEffect", &UnitTestEventsBus::Events::SideEffect)
                    ->Event("Succeeded", &UnitTestEventsBus::Events::Succeeded)
                    ;
            }
        }

        void Failed(const AZStd::string& description) override
        {
            Call(FN_Failed, description);
        }

        AZStd::string Result(const AZStd::string& description) override
        {
            AZStd::string output;
            CallResult(output, FN_Result, description);
            return output;
        }

        void SideEffect(const AZStd::string& description) override
        {
            Call(FN_SideEffect, description);
        }

        void Succeeded(const AZStd::string& description) override
        {
            Call(FN_Succeeded, description);
        }
    }; // ScriptUnitTestEventHandler

    class ScriptUnitTestNodeNotificationHandler
        : private ScriptCanvas::NodeNotificationsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptUnitTestNodeNotificationHandler, AZ::SystemAllocator, 0);

        ScriptUnitTestNodeNotificationHandler(AZ::EntityId nodeId)
        {
            ScriptCanvas::NodeNotificationsBus::Handler::BusConnect(nodeId);
        }

        ~ScriptUnitTestNodeNotificationHandler()
        {
            ScriptCanvas::NodeNotificationsBus::Handler::BusDisconnect();
        }

        AZ::u32 GetSlotsAdded() const
        {
            return m_slotsAdded;
        }

        AZ::u32 GetSlotsRemoved() const
        {
            return m_slotsRemoved;
        }

    private:
        void OnSlotAdded(const ScriptCanvas::SlotId& slotId) override
        {
            ++m_slotsAdded;
        }

        void OnSlotRemoved(const ScriptCanvas::SlotId& slotId) override
        {
            ++m_slotsRemoved;
        }

        AZ::u32 m_slotsAdded = 0;
        AZ::u32 m_slotsRemoved = 0;
    };

} // ScriptCanvasTests
