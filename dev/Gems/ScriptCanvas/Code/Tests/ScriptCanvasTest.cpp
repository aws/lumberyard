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

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Memory/MemoryDrillerBus.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/typetraits/function_traits.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>

#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/Contracts/DisallowReentrantExecutionContract.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Core/GraphAssetHandler.h>

#include <ScriptCanvas/Libraries/Core/Core.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>
#include <ScriptCanvas/Libraries/Core/BehaviorContextObjectNode.h>
#include <ScriptCanvas/Libraries/Entity/EntityRef.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Logic/Logic.h>
#include <ScriptCanvas/Libraries/Math/Math.h>
#include <ScriptCanvas/Libraries/Comparison/Comparison.h>

#include <ScriptCanvas/SystemComponent.h>
#include <ScriptCanvas/ScriptCanvasGem.h>

#include "EntityRefTests.h"

// disable test bodies to see if there's anything wrong with the system or test framework not related to ScriptCanvas testing
#define TEST_BODIES_ENABLED 1 // 1 = enabled by default, 0 = disabled by default

#define TEST_BODY_DEFAULT (TEST_BODIES_ENABLED)  
#define TEST_BODY_EXCEPTION (!TEST_BODY_DEFAULT)

#if (TEST_BODIES_ENABLED)
#define RETURN_IF_TEST_BODIES_ARE_DISABLED(isException) if (!isException) { ADD_FAILURE(); return; }
#else
#define RETURN_IF_TEST_BODIES_ARE_DISABLED(isException) if (isException) { return; }
#endif

namespace ScriptCanvasTests
{
    class Application : public AZ::ComponentApplication
    {
    public:

        // We require an EditContext to be created as early as possible
        // as we rely on EditContext attributes for detecting Graph entry points.
        void CreateSerializeContext() override
        {
            AZ::ComponentApplication::CreateSerializeContext();
            GetSerializeContext()->CreateEditContext();
        }
    };
}

class Print
    : public ScriptCanvas::Node
{
public:
    AZ_COMPONENT(Print, "{085CBDD3-D4E0-44D4-BF68-8732E35B9DF1}", ScriptCanvas::Node);

    // really only used for unit testing
    AZ_INLINE void SetText(const ScriptCanvas::Data::StringType& text) { m_string = text; }
    AZ_INLINE const ScriptCanvas::Data::StringType& GetText() const { return m_string; }

    void Visit(ScriptCanvas::NodeVisitor& visitor) const override { visitor.Visit(*this); }

protected:

    void OnInit() override
    {
        AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
        AddSlot("Out", "", ScriptCanvas::SlotType::ExecutionOut);
        AddInputDatumUntypedSlot("Value");
    }

    void OnInputSignal(const ScriptCanvas::SlotId&) override
    {
        const int onlyDatumIndex(0);
        const ScriptCanvas::Datum* input = GetInput(onlyDatumIndex);

        if (input && !input->Empty())
        {
            input->ToString(m_string);
        }

        // technically, I should remove this, make it an object that holds a string, with an untyped slot, and this could be a local value
        if (!m_string.empty())
        {
            AZ_TracePrintf("Script Canvas", "%s\n", m_string.c_str());
        }

        SignalOutput(GetSlotId("Out"));
    }

public:

    static void Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Print, Node>()
                ->Version(5)
                ->Field("m_string", &Print::m_string)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Print>("Print", "Development node, will be replaced by a Log node")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Print.png")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Print::m_string, "String", "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

private:
    ScriptCanvas::Data::StringType m_string;
};

AZ::Debug::DrillerManager* s_drillerManager = nullptr;

const bool s_enableMemoryLeakChecking = false;

class ScriptCanvasTestFixture
    : public ::testing::Test
{
    static ScriptCanvasTests::Application* GetApplication();

protected:

    static ScriptCanvasTests::Application s_application;

    static void SetUpTestCase()
    {
        if (!s_drillerManager)
        {
            s_drillerManager = AZ::Debug::DrillerManager::Create();
            // Memory driller is responsible for tracking allocations. 
            // Tracking type and overhead is determined by app configuration.
            s_drillerManager->Register(aznew AZ::Debug::MemoryDriller);
        }

        AZ::SystemAllocator::Descriptor systemAllocatorDesc;
        systemAllocatorDesc.m_allocationRecords = s_enableMemoryLeakChecking;
        systemAllocatorDesc.m_stackRecordLevels = 12;
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create(systemAllocatorDesc);

        AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords();
        if (records && s_enableMemoryLeakChecking)
        {
            records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
        }

        AZ::ComponentApplication::StartupParameters appStartup;

        appStartup.m_createStaticModulesCallback =
            [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(new ScriptCanvas::ScriptCanvasModule);
        };

        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::RegisterComponentDescriptor, ScriptCanvasTests::TestComponent::CreateDescriptor());

        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_enableDrilling = false; // We'll manage our own driller in these tests
        appDesc.m_useExistingAllocator = true; // Use the SystemAllocator we own in this test.

        AZ::Entity* systemEntity = s_application.Create(appDesc, appStartup);
        AZ::TickBus::AllowFunctionQueuing(true);

        systemEntity->CreateComponent<AZ::MemoryComponent>();
        systemEntity->CreateComponent<AZ::AssetManagerComponent>();
        systemEntity->CreateComponent<ScriptCanvas::SystemComponent>();

        systemEntity->Init();
        systemEntity->Activate();
    }

    static void TearDownTestCase()
    {
        s_application.Destroy();

        // This allows us to print the raw dump of allocations that have not been freed, but is not required. 
        // Leaks will be reported when we destroy the allocator.
        const bool printRecords = false;
        AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords();
        if (records && printRecords)
        {
            records->EnumerateAllocations(AZ::Debug::PrintAllocationsCB(false));
        }

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

        if (s_drillerManager)
        {
            AZ::Debug::DrillerManager::Destroy(s_drillerManager);
            s_drillerManager = nullptr;
        }
    }

    void SetUp() override
    {
        m_serializeContext = s_application.GetSerializeContext();
        m_behaviorContext = s_application.GetBehaviorContext();

        if (!AZ::IO::FileIOBase::GetInstance())
        {
            m_fileIO.reset(aznew AZ::IO::LocalFileIO());
            AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
        }
        AZ_Assert(AZ::IO::FileIOBase::GetInstance(), "File IO was not properly installed");

        AzFramework::EntityReference::Reflect(m_serializeContext);
        Print::Reflect(m_serializeContext);
        Print::Reflect(m_behaviorContext);
    }

    void TearDown() override
    {
        m_serializeContext->EnableRemoveReflection();
        AzFramework::EntityReference::Reflect(m_serializeContext);
        Print::Reflect(m_serializeContext);
        m_serializeContext->DisableRemoveReflection();
        m_behaviorContext->EnableRemoveReflection();
        Print::Reflect(m_behaviorContext);
        m_behaviorContext->DisableRemoveReflection();
        AZ::IO::FileIOBase::SetInstance(nullptr);
        m_fileIO = nullptr;
    }

    AZStd::unique_ptr<AZ::IO::FileIOBase> m_fileIO;
    AZ::SerializeContext* m_serializeContext;
    AZ::BehaviorContext* m_behaviorContext;
    
};

ScriptCanvasTests::Application ScriptCanvasTestFixture::s_application;

ScriptCanvasTests::Application* ScriptCanvasTestFixture::GetApplication()
{
    return &s_application;
}

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

ScriptCanvas::Nodes::Core::BehaviorContextObjectNode* CreateTestObjectNode(const AZ::EntityId& graphUniqueId, AZ::EntityId& entityOut, const AZ::Uuid& objectTypeID)
{
    ScriptCanvas::Nodes::Core::BehaviorContextObjectNode* node = CreateTestNode<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, entityOut);
    EXPECT_TRUE(node != nullptr);
    node->InitializeObject(objectTypeID);
    return node;
}

AZ::EntityId CreateClassFunctionNode(const AZ::EntityId& graphUniqueId, const AZStd::string& className, const AZStd::string& methodName)
{
    using namespace ScriptCanvas;

    ScriptCanvas::Namespaces emptyNamespaces;

    AZ::Entity* splitEntity{ aznew AZ::Entity };
    splitEntity->Init();
    AZ::EntityId methodNodeID{ splitEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, methodNodeID, graphUniqueId, Nodes::Core::Method::RTTI_Type());
    Nodes::Core::Method* methodNode(nullptr);
    SystemRequestBus::BroadcastResult(methodNode, &SystemRequests::GetNode<Nodes::Core::Method>, methodNodeID);
    EXPECT_TRUE(methodNode != nullptr);
    methodNode->InitializeClassOrBus(emptyNamespaces, className, methodName);
    return methodNodeID;
}

template<typename t_Value>
ScriptCanvas::Node* CreateDataNode(const AZ::EntityId& graphUniqueId, const t_Value& value, AZ::EntityId& nodeIDout)
{
    using namespace ScriptCanvas;
    const Data::Type operandType = Data::FromAZType(azrtti_typeid<t_Value>());
    Node* node{};

    switch (operandType.GetType())
    {
    case Data::eType::AABB:
        node = CreateTestNode<Nodes::Math::AABB>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::BehaviorContextObject:
    {
        auto objectNode = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, nodeIDout);
        objectNode->InitializeObject(operandType);
        node = objectNode;
    }
    break;
    
    case Data::eType::Boolean:
        node = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::Color:
        node = CreateTestNode<Nodes::Math::Color>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::CRC:
        node = CreateTestNode<Nodes::Math::CRC>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::EntityID:
        node = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::Matrix3x3:
        node = CreateTestNode<Nodes::Math::Matrix3x3>(graphUniqueId, nodeIDout);
        break;
    
    case Data::eType::Matrix4x4:
        node = CreateTestNode<Nodes::Math::Matrix4x4>(graphUniqueId, nodeIDout);
        break;
    
    case Data::eType::Number:
        node = CreateTestNode<Nodes::Math::Number>(graphUniqueId, nodeIDout);
        break;
    
    case Data::eType::OBB:
        node = CreateTestNode<Nodes::Math::OBB>(graphUniqueId, nodeIDout);
        break;
    
    case Data::eType::Plane:
        node = CreateTestNode<Nodes::Math::Plane>(graphUniqueId, nodeIDout);
        break;
    
    case Data::eType::Rotation:
        node = CreateTestNode<Nodes::Math::Rotation>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::String:
        node = CreateTestNode<Nodes::Core::String>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::Transform:
        node = CreateTestNode<Nodes::Math::Transform>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::Vector2:
        node = CreateTestNode<Nodes::Math::Vector2>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::Vector3:
        node = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, nodeIDout);
        break;

    case Data::eType::Vector4:
        node = CreateTestNode<Nodes::Math::Vector4>(graphUniqueId, nodeIDout);
        break;

    default:
        AZ_Error("ScriptCanvas", false, "unsupported data type");
    }

    EXPECT_NE(node, nullptr);
    if (node)
    {
        node->SetInput_UNIT_TEST("Set", value);
    }
    return node;
}

const char* SlotTypeToString(ScriptCanvas::SlotType type)
{
    switch (type)
    {
    case ScriptCanvas::SlotType::ExecutionIn:
        return "ExecutionIn";
    case ScriptCanvas::SlotType::ExecutionOut:
        return "ExecutionOut";
    case ScriptCanvas::SlotType::DataIn:
        return "DataIn";
    case ScriptCanvas::SlotType::DataOut:
        return "DataOut";
    default:
        return "invalid";
    }
}

void DumpSlots(const ScriptCanvas::Node& node)
{
    const auto& slots = node.GetSlots();
    
    for (const auto& slot : slots)
    {
        AZ_TracePrintf("ScriptCanvasTest", "'%s':%s\n", slot.GetName().data(), SlotTypeToString(slot.GetType()));
    }
}

bool Connect(ScriptCanvas::Graph& graph, const AZ::EntityId& fromNodeID, const char* fromSlotName, const AZ::EntityId& toNodeID, const char* toSlotName, bool dumpSlotsOnFailure = true)
{
    using namespace ScriptCanvas;
    AZ::Entity* fromNode{};
    AZ::ComponentApplicationBus::BroadcastResult(fromNode, &AZ::ComponentApplicationBus::Events::FindEntity, fromNodeID);
    AZ::Entity* toNode{};
    AZ::ComponentApplicationBus::BroadcastResult(toNode, &AZ::ComponentApplicationBus::Events::FindEntity, toNodeID);
    if (fromNode && toNode)
    {
        Node* from  = AZ::EntityUtils::FindFirstDerivedComponent<Node>(fromNode);
        Node* to = AZ::EntityUtils::FindFirstDerivedComponent<Node>(toNode);

        auto fromSlotId = from->GetSlotId(fromSlotName);
        auto toSlotId = to->GetSlotId(toSlotName);

        if (graph.Connect(fromNodeID, fromSlotId, toNodeID, toSlotId))
        {
            return true;
        }
        else if (dumpSlotsOnFailure)
        {
            AZ_TracePrintf("ScriptCanvasTest", "Slots from:\n");
            DumpSlots(*from);
            AZ_TracePrintf("ScriptCanvasTest", "\nSlots to:\n");
            DumpSlots(*to);
        }
    }
    
    return false;
}

class ContractNode : public ScriptCanvas::Node
{
public:
    AZ_CLASS_ALLOCATOR(ContractNode, AZ::SystemAllocator, 0);
    AZ_RTTI(ContractNode, "{76A17F4F-F508-4C20-83A0-0125468946C7}", ScriptCanvas::Node);

    static void Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ContractNode, Node>()
                ->Version(1)
                ;
        }
    }

    void OnInit() override
    {
        using namespace ScriptCanvas;

        const SlotId& inSlot = AddSlot("In", "", SlotType::ExecutionIn);

        auto func = []() { return aznew DisallowReentrantExecutionContract{}; };
        ContractDescriptor descriptor{ AZStd::move(func) };
        GetSlot(inSlot)->AddContract(descriptor);

        AddSlot("Out", "", SlotType::ExecutionOut);
        AddInputTypeSlot("Set String", "", Data::Type::String(), InputTypeContract::CustomType);
        AddInputTypeSlot("Set Number", "", Data::Type::Number(), InputTypeContract::CustomType);
        AddOutputTypeSlot("Get String", "", Data::Type::String(), OutputStorage::Optional);
        AddOutputTypeSlot("Get Number", "", Data::Type::Number(), OutputStorage::Optional);
    }

protected:
    void OnInputSignal(const ScriptCanvas::SlotId&) override
    {
        SignalOutput(GetSlotId("Out"));
    }
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
    auto printNode = CreateTestNode<Print>(graphUniqueId, printID);

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

    graph->SetStartNode(startID);
    graph->GetEntity()->Activate();
    EXPECT_EQ(graph->IsInErrorState(), forceGraphError);

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
void AssignTest(const t_Value& lhs, const t_Value& rhs)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    AZ::Entity graphEntity("Graph");
    graphEntity.Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, &graphEntity);
    auto graph = graphEntity.FindComponent<Graph>();
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

    graph->SetStartNode(startID);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    if (auto result = targetNode->GetInput_UNIT_TEST<t_Value>("Set"))
    {
        EXPECT_EQ(*result, lhs);
    }
    else
    {
        ADD_FAILURE();
    }

    graph->GetEntity()->Deactivate();
    delete graph;
}

namespace
{
    using namespace ScriptCanvas;  

    class InfiniteLoopNode
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(InfiniteLoopNode, "{709A78D5-3449-4E94-B751-C68AC6385749}", Node);

        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<InfiniteLoopNode, Node>()
                    ->Version(0)
                    ;

            }
        }

    protected:

        void OnInputSignal(const SlotId&) override
        {
            SignalOutput(GetSlotId("Before Infinity"));
        }

        void OnInit() override
        {
            AddSlot("In", "", SlotType::ExecutionIn);
            AddSlot("Before Infinity", "", SlotType::ExecutionOut);
            AddSlot("After Infinity", "", SlotType::ExecutionOut);
        }
    };

    class UnitTestErrorNode
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(UnitTestErrorNode, "{6A3E9EAD-F84B-4474-90B6-C3334DA669C2}", Node);

        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<UnitTestErrorNode, Node>()
                    ->Version(0)
                    ;

            }
        }

    protected:

        void OnInit() override
        {
            AddSlot("In", "", SlotType::ExecutionIn);
            AddSlot("Out", "", SlotType::ExecutionOut);
            AddSlot("This", "", SlotType::DataOut);
        }

        void OnInputSignal(const SlotId&) override
        {
            SCRIPTCANVAS_REPORT_ERROR((*this), "Unit test error!");

            SignalOutput(GetSlotId("Out"));
        }

    private:

    };

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
    }; // class UnitTestEventsHandler

    class ScriptUnitTestEventHandler
        : public UnitTestEventsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER
            ( ScriptUnitTestEventHandler
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

    class EventTest
        : public AZ::ComponentBus
    {
    public:
        virtual void Event() = 0;
        virtual int Number(int number) = 0;
        virtual AZStd::string String(const AZStd::string& string) = 0;
        virtual AZ::Vector3 Vector(const AZ::Vector3& vector) = 0;
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
            , Vector);

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

        AZ::Vector3 Vector(const AZ::Vector3& input) override
        {
            AZ::Vector3 output;
            CallResult(output, FN_Vector, input);
            return output;
        }
    }; // EventTestHandler

    class StringView
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(StringView, "{F47ACD24-79EB-4DBE-B325-8B9DB0839A75}", ScriptCanvas::Node);
         
        void Visit(ScriptCanvas::NodeVisitor& visitor) const override { visitor.Visit(*this); }

    protected:

        void OnInit() override
        {
            AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
            AddSlot("Out", "", ScriptCanvas::SlotType::ExecutionOut);
            AddInputDatumSlot("View", "Input string_view object", Data::Type::String(), ScriptCanvas::Datum::eOriginality::Copy);
            m_resultSlotId = AddOutputTypeSlot("Result", "Output string object", Data::FromAZType(azrtti_typeid<AZStd::string>()), Node::OutputStorage::Optional);
        }

        void OnInputSignal(const ScriptCanvas::SlotId&) override
        {
            const int onlyInputDatumIndex(0);
            const int onlyOutputSlotIndex(0);
            const ScriptCanvas::Datum* input = GetInput(onlyInputDatumIndex);
            ScriptCanvas::Data::StringType result;
            if (input && !input->Empty())
            {
                input->ToString(result);
            }

            ScriptCanvas::Datum output(Datum::CreateInitializedCopy(result));
            auto resultSlot = GetSlot(m_resultSlotId);
            if (resultSlot)
            {
                PushOutput(output, *resultSlot);
            }
            SignalOutput(GetSlotId("Out"));
        }

    public:

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<StringView, Node>()
                    ->Version(0)
                    ;
            }
        }

    private:
        SlotId m_resultSlotId;
    };

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

        AZ_CLASS_ALLOCATOR(TemplateEventTestHandler, AZ::SystemAllocator, 0);
        AZ_RTTI((TemplateEventTestHandler, "{AADA0906-C216-4D98-BD51-76BB0C2F7FAF}", TBusIdType), AZ::BehaviorEBusHandler);
        using ThisType = TemplateEventTestHandler;
        enum
        {
            AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_ENUM, AZ_EBUS_SEQ(GenericEvent, UpdateNameEvent, VectorCreatedEvent))
            FN_MAX
        };

        int GetFunctionIndex(const char* functionName) const override {
            AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_INDEX, AZ_EBUS_SEQ(GenericEvent, UpdateNameEvent, VectorCreatedEvent))
                return -1;
        }

        void Disconnect() override {
            BusDisconnect();
        }

        TemplateEventTestHandler() {
            m_events.resize(FN_MAX);
            AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_REG_EVENT, AZ_EBUS_SEQ(GenericEvent, UpdateNameEvent, VectorCreatedEvent))
        }

        bool Connect(AZ::BehaviorValueParameter* id = nullptr) override {
            return AZ::Internal::EBusConnector<TemplateEventTestHandler>::Connect(this, id);
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                const AZStd::string ebusName(AZ::AzTypeInfo<TemplateEventTestHandler>::Name());
                behaviorContext->EBus<TemplateEventTestBus<TBusIdType>>(ebusName.data())
                    ->Handler<TemplateEventTestHandler>()
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

    void ReflectSignCorrectly()
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "A behavior context is required!");
        behaviorContext->Method("Sign", AZ::GetSign);
    }
    
    AZ_INLINE void ArgsNoReturn(float)
    {
        UnitTestEventsBus::Broadcast(&UnitTestEvents::SideEffect, "ArgsNoReturn SideFX");
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ArgsNoReturn, "{980E4400-288B-4DA2-8C5C-BBC5164CA2AB}", "One Arg");

    AZ_INLINE std::tuple<AZStd::string, bool> ArgsReturnMulti(double input)
    {
        // compile test
        return input >= 0.0
            ? std::make_tuple("positive", true)
            : std::make_tuple("negative", false);
    } 
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(ArgsReturnMulti, "{D7475558-BD14-4588-BC3A-6B4BD1ACF3B4}", "input:One Arg", "output:string", "output:bool");

    AZ_INLINE void NoArgsNoReturn()
    { 
        UnitTestEventsBus::Broadcast(&UnitTestEvents::SideEffect, "NoArgsNoReturn SideFX");
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(NoArgsNoReturn, "{18BC4E04-7D97-4379-8A36-877881633AA9}");

    AZ_INLINE float NoArgsReturn() 
    { 
        UnitTestEventsBus::Broadcast(&UnitTestEvents::SideEffect, "NoArgsReturn SideFX");
        return 0.0f;
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(NoArgsReturn, "{08E6535A-FCE0-4953-BA3E-59CF5A10073B}");

    AZ_INLINE std::tuple<AZStd::string, bool> NoArgsReturnMulti()
    {
        return std::make_tuple("no-args", false);
    }
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(NoArgsReturnMulti, "{A73262FA-2756-40D6-A25C-8B98A64348F2}", "output:string", "output:bool");

    class TestBehaviorContextObject
    {
    public:
        AZ_TYPE_INFO(TestBehaviorContextObject, "{52DA0875-8EF6-4D39-9CF6-9F83BAA59A1D}");

        static TestBehaviorContextObject MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs)
        {
            return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
        }

        static const TestBehaviorContextObject* MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs)
        {
            return (lhs && rhs && lhs->GetValue() >= rhs->GetValue()) ? lhs : rhs;
        }
        
        static const TestBehaviorContextObject& MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs)
        {
            return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
        }

        static int MaxReturnByValueInteger(int lhs, int rhs)
        {
            return lhs >= rhs ? lhs : rhs;
        }

        static const int* MaxReturnByPointerInteger(const int* lhs, const int* rhs)
        {
            return (lhs && rhs && (*lhs) >= (*rhs)) ? lhs : rhs;
        }

        static const int& MaxReturnByReferenceInteger(const int& lhs, const int& rhs)
        {
            return lhs >= rhs ? lhs : rhs;
        }

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<TestBehaviorContextObject>()
                    ->Version(0)
                    ->Field("m_value", &TestBehaviorContextObject::m_value)
                    ;
            }
            
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->Class<TestBehaviorContextObject>("TestBehaviorContextObject")
                    ->Method("In", &TestBehaviorContextObject::GetValue)
                    ->Method("Out", &TestBehaviorContextObject::SetValue)
                    ->Method("MaxReturnByValue", &TestBehaviorContextObject::MaxReturnByValue)
                    ->Method("MaxReturnByPointer", &TestBehaviorContextObject::MaxReturnByPointer)
                    ->Method("MaxReturnByReference", &TestBehaviorContextObject::MaxReturnByReference)
                    ->Method("MaxReturnByValueInteger", &TestBehaviorContextObject::MaxReturnByValueInteger)
                    ->Method("MaxReturnByPointerInteger", &TestBehaviorContextObject::MaxReturnByPointerInteger)
                    ->Method("MaxReturnByReferenceInteger", &TestBehaviorContextObject::MaxReturnByReferenceInteger)
                    ->Method<bool (TestBehaviorContextObject::*)(const TestBehaviorContextObject&) const>("LessThan", &TestBehaviorContextObject::operator<)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)
                    ->Method<bool (TestBehaviorContextObject::*)(const TestBehaviorContextObject&) const>("LessEqualThan", &TestBehaviorContextObject::operator<=)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessEqualThan)
                    ->Method<bool (TestBehaviorContextObject::*)(const TestBehaviorContextObject&) const>("Equal", &TestBehaviorContextObject::operator==)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ;
            }
        }

        static AZ::u32 GetCreatedCount() { return s_createdCount; }
        static AZ::u32 GetDestroyedCount() { return s_destroyedCount; }
        static void ResetCounts() { s_createdCount = s_destroyedCount = 0; }

        TestBehaviorContextObject()
            : m_value(0)
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

        int GetValue() const { return m_value; }
        void SetValue(int value) { m_value = value; }

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
        int m_value;
        static AZ::u32 s_createdCount;
        static AZ::u32 s_destroyedCount;
    };

    AZ::u32 TestBehaviorContextObject::s_createdCount = 0;
    AZ::u32 TestBehaviorContextObject::s_destroyedCount = 0;
    
    TestBehaviorContextObject MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs)
    {
        return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
    }

    const TestBehaviorContextObject* MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs)
    {
        return (lhs && rhs && (lhs->GetValue() >= rhs->GetValue())) ? lhs : rhs;
    }

    const TestBehaviorContextObject& MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs)
    {
        return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
    }

    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByValue, "{60C054C6-8A07-4D41-A9E4-E3BB0D20F098}", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByPointer, "{16AFDE59-31B5-4B49-999F-8B486FC91371}", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByReference, "{0A1FD51A-1D53-46FC-9A2F-DF711F62FDE9}", "0", "1");

    // generic types that passed around by reference/pointer should behavior just like the value types
    int MaxReturnByValueInteger(int lhs, int rhs)
    {
        return lhs >= rhs ? lhs : rhs;
    }

    const int* MaxReturnByPointerInteger(const int* lhs, const int* rhs)
    {
        return (lhs && rhs && (*lhs) >= (*rhs)) ? lhs : rhs;
    }

    const int& MaxReturnByReferenceInteger(const int& lhs, const int& rhs)
    {
        return lhs >= rhs ? lhs : rhs;
    }

    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByValueInteger, "{5165F1BA-248F-434F-9227-B6AC2102D4B5}", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByPointerInteger, "{BE658D24-8AB0-463B-979D-C829985E96EF}", "0", "1");
    SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MaxReturnByReferenceInteger, "{8DE10FF6-9628-4015-A149-4276BF98D2AB}", "0", "1");

    std::tuple<TestBehaviorContextObject, int> MaxReturnByValueMulti(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs, int lhsInt, int rhsInt)
    {
        return std::make_tuple
            ( lhs.GetValue() >= rhs.GetValue() ? lhs : rhs
            , lhsInt >= rhsInt ? lhsInt : rhsInt);
    }

    std::tuple<const TestBehaviorContextObject*, const int*> MaxReturnByPointerMulti(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs, const int* lhsInt, const int* rhsInt)
    {
        return std::make_tuple
            ( (lhs && rhs && (lhs->GetValue() >= rhs->GetValue())) ? lhs : rhs
            , lhsInt >= rhsInt ? lhsInt : rhsInt);
    }

    std::tuple<const TestBehaviorContextObject&, const int&> MaxReturnByReferenceMulti(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs, const int& lhsInt, const int& rhsInt)
    {
        return std::forward_as_tuple
            ( lhs.GetValue() >= rhs.GetValue() ? lhs : rhs
            , lhsInt >= rhsInt ? lhsInt : rhsInt);
    }

    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(MaxReturnByValueMulti, "{5BE8F2C8-C036-4C82-A7C1-4DCBAC2FA6FC}", "0", "1", "0", "1", "Result", "Result");
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(MaxReturnByPointerMulti, "{339BDAB0-BB80-4BFE-B377-12FD08278A8E}", "0", "1", "0", "1", "Result", "Result");
    SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(MaxReturnByReferenceMulti, "{7FECD272-4348-463C-80CC-45D0C77378A6}", "0", "1", "0", "1", "Result", "Result");
    
} // namespace

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Tests

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST_F(ScriptCanvasTestFixture, NativeProperties)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId addId = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");

    AZ::EntityId vector3IdA, vector3IdB, vector3IdC;
    auto vector3NodeA = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdA);
    auto vector3NodeB = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdB);
    auto vector3NodeC = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdC);

    AZ::EntityId number1Id, number2Id, number3Id, number4Id, number5Id, number6Id, number7Id, number8Id, number9Id;
    Node* number1Node = CreateDataNode<Data::NumberType>(graphUniqueId, 1, number1Id);
    Node* number2Node = CreateDataNode<Data::NumberType>(graphUniqueId, 2, number2Id);
    Node* number3Node = CreateDataNode<Data::NumberType>(graphUniqueId, 3, number3Id);
    Node* number4Node = CreateDataNode<Data::NumberType>(graphUniqueId, 4, number4Id);
    Node* number5Node = CreateDataNode<Data::NumberType>(graphUniqueId, 5, number5Id);
    Node* number6Node = CreateDataNode<Data::NumberType>(graphUniqueId, 6, number6Id);
    Node* number7Node = CreateDataNode<Data::NumberType>(graphUniqueId, 7, number7Id);
    Node* number8Node = CreateDataNode<Data::NumberType>(graphUniqueId, 8, number8Id);
    Node* number9Node = CreateDataNode<Data::NumberType>(graphUniqueId, 0, number9Id);

    // data
    EXPECT_TRUE(Connect(*graph, number1Id, "Get", vector3IdA, "Number: x"));
    EXPECT_TRUE(Connect(*graph, number2Id, "Get", vector3IdA, "Number: y"));
    EXPECT_TRUE(Connect(*graph, number3Id, "Get", vector3IdA, "Number: z"));

    EXPECT_TRUE(Connect(*graph, number4Id, "Get", vector3IdB, "Number: x"));
    EXPECT_TRUE(Connect(*graph, number5Id, "Get", vector3IdB, "Number: y"));
    EXPECT_TRUE(Connect(*graph, number6Id, "Get", vector3IdB, "Number: z"));

    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", addId, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", addId, "Vector3: 1"));

    EXPECT_TRUE(Connect(*graph, addId, "Result: Vector3", vector3IdC, "Set"));

    EXPECT_TRUE(Connect(*graph, vector3IdC, "x: Number", number7Id, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "y: Number", number8Id, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "z: Number", number9Id, "Set"));

    // code
    EXPECT_TRUE(Connect(*graph, startID, "Out", addId, "In"));

    graph->SetStartNode(startID);
    graphEntity->Activate();

    EXPECT_FALSE(graph->IsInErrorState());

    if (auto result = vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        EXPECT_EQ(AZ::Vector3(5, 7, 9), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number7Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(5, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number8Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(7, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number9Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(9, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;
}

TEST_F(ScriptCanvasTestFixture, BehaviorContextProperties)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId addId = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");

    AZ::EntityId vector3IdA, vector3IdB, vector3IdC;
    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<AZ::Vector3>());
    Core::BehaviorContextObjectNode* vector3NodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdB);
    vector3NodeB->InitializeObject(azrtti_typeid<AZ::Vector3>());
    Core::BehaviorContextObjectNode* vector3NodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdC);
    vector3NodeC->InitializeObject(azrtti_typeid<AZ::Vector3>());
    
    AZ::EntityId number1Id, number2Id, number3Id, number4Id, number5Id, number6Id, number7Id, number8Id, number9Id;
    Node* number1Node = CreateDataNode<Data::NumberType>(graphUniqueId, 1, number1Id);
    Node* number2Node = CreateDataNode<Data::NumberType>(graphUniqueId, 2, number2Id);
    Node* number3Node = CreateDataNode<Data::NumberType>(graphUniqueId, 3, number3Id);
    Node* number4Node = CreateDataNode<Data::NumberType>(graphUniqueId, 4, number4Id);
    Node* number5Node = CreateDataNode<Data::NumberType>(graphUniqueId, 5, number5Id);
    Node* number6Node = CreateDataNode<Data::NumberType>(graphUniqueId, 6, number6Id);
    Node* number7Node = CreateDataNode<Data::NumberType>(graphUniqueId, 7, number7Id);
    Node* number8Node = CreateDataNode<Data::NumberType>(graphUniqueId, 8, number8Id);
    Node* number9Node = CreateDataNode<Data::NumberType>(graphUniqueId, 0, number9Id);
    
    // data
    EXPECT_TRUE(Connect(*graph, number1Id, "Get", vector3IdA, "Number: x"));
    EXPECT_TRUE(Connect(*graph, number2Id, "Get", vector3IdA, "Number: y"));
    EXPECT_TRUE(Connect(*graph, number3Id, "Get", vector3IdA, "Number: z"));
    
    EXPECT_TRUE(Connect(*graph, number4Id, "Get", vector3IdB, "Number: x"));
    EXPECT_TRUE(Connect(*graph, number5Id, "Get", vector3IdB, "Number: y"));
    EXPECT_TRUE(Connect(*graph, number6Id, "Get", vector3IdB, "Number: z"));

    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", addId, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", addId, "Vector3: 1"));

    EXPECT_TRUE(Connect(*graph, addId, "Result: Vector3", vector3IdC, "Set"));

    EXPECT_TRUE(Connect(*graph, vector3IdC, "x: Number", number7Id, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "y: Number", number8Id, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "z: Number", number9Id, "Set"));
    
    // code
    EXPECT_TRUE(Connect(*graph, startID, "Out", addId, "In"));

    graph->SetStartNode(startID);
    graphEntity->Activate();

    EXPECT_FALSE(graph->IsInErrorState());
 
    if (auto result = vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        EXPECT_EQ(AZ::Vector3(5, 7, 9), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number7Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(5, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number8Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(7, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number9Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(9, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;
}

TEST_F(ScriptCanvasTestFixture, BehaviorContextObjectGenericConstructor)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();


    //Validates the GenericConstructorOverride attribute is being used to construct types that are normally not initialized in C++
    AZ::EntityId vector2IdA;
    AZ::EntityId vector3IdA;
    AZ::EntityId vector4IdA;
    AZ::EntityId colorIdA;
    AZ::EntityId quaternionNodeIdA;
    AZ::EntityId matrix3x3IdA;
    AZ::EntityId matrix4x4IdA;
    AZ::EntityId transformIdA;
    AZ::EntityId uuidIdA;
    AZ::EntityId aabbIdA;
    AZ::EntityId obbIdA;
    AZ::EntityId planeIdA;
    Core::BehaviorContextObjectNode* vector2NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector2IdA);
    vector2NodeA->InitializeObject(azrtti_typeid<AZ::Vector2>());
    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<AZ::Vector3>());
    Core::BehaviorContextObjectNode* vector4NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector4IdA);
    vector4NodeA->InitializeObject(azrtti_typeid<AZ::Vector4>());
    Core::BehaviorContextObjectNode* colorNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, colorIdA);
    colorNodeA->InitializeObject(azrtti_typeid<AZ::Color>());
    Core::BehaviorContextObjectNode* quaternionNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, quaternionNodeIdA);
    quaternionNodeA->InitializeObject(azrtti_typeid<AZ::Quaternion>());
    Core::BehaviorContextObjectNode* matrix3x3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, matrix3x3IdA);
    matrix3x3NodeA->InitializeObject(azrtti_typeid<AZ::Matrix3x3>());
    Core::BehaviorContextObjectNode* matrix4x4NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, matrix4x4IdA);
    matrix4x4NodeA->InitializeObject(azrtti_typeid<AZ::Matrix4x4>());
    Core::BehaviorContextObjectNode* transformNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, transformIdA);
    transformNodeA->InitializeObject(azrtti_typeid<AZ::Transform>());
    Core::BehaviorContextObjectNode* uuidNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, uuidIdA);
    uuidNodeA->InitializeObject(azrtti_typeid<AZ::Uuid>());
    Core::BehaviorContextObjectNode* aabbNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, aabbIdA);
    aabbNodeA->InitializeObject(azrtti_typeid<AZ::Aabb>());
    Core::BehaviorContextObjectNode* obbNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, obbIdA);
    obbNodeA->InitializeObject(azrtti_typeid<AZ::Obb>());
    Core::BehaviorContextObjectNode* planeNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, planeIdA);
    planeNodeA->InitializeObject(azrtti_typeid<AZ::Plane>());
    
    EXPECT_FALSE(graph->IsInErrorState());

    if (auto result = vector2NodeA->GetInput_UNIT_TEST<AZ::Vector2>("Set"))
    {
        EXPECT_EQ(AZ::Vector2::CreateZero(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = vector3NodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        EXPECT_EQ(AZ::Vector3::CreateZero(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = vector4NodeA->GetInput_UNIT_TEST<AZ::Vector4>("Set"))
    {
        EXPECT_EQ(AZ::Vector4::CreateZero(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = colorNodeA->GetInput_UNIT_TEST<AZ::Color>("Set"))
    {
        EXPECT_EQ(AZ::Color::CreateZero(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = quaternionNodeA->GetInput_UNIT_TEST<AZ::Quaternion>("Set"))
    {
        EXPECT_EQ(AZ::Quaternion::CreateIdentity(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = matrix3x3NodeA->GetInput_UNIT_TEST<AZ::Matrix3x3>("Set"))
    {
        EXPECT_EQ(AZ::Matrix3x3::CreateIdentity(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = matrix4x4NodeA->GetInput_UNIT_TEST<AZ::Matrix4x4>("Set"))
    {
        EXPECT_EQ(AZ::Matrix4x4::CreateIdentity(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = transformNodeA->GetInput_UNIT_TEST<AZ::Transform>("Set"))
    {
        EXPECT_EQ(AZ::Transform::CreateIdentity(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = uuidNodeA->GetInput_UNIT_TEST<AZ::Uuid>("Set"))
    {
        EXPECT_EQ(AZ::Uuid::CreateNull(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = aabbNodeA->GetInput_UNIT_TEST<AZ::Aabb>("Set"))
    {
        EXPECT_EQ(AZ::Aabb::CreateNull(), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    delete graphEntity;
}


TEST_F(ScriptCanvasTestFixture, IsNullCheck)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId isNullFalseID;
    CreateTestNode<Nodes::Logic::IsNull>(graphUniqueId, isNullFalseID);

    AZ::EntityId isNullTrueID;
    CreateTestNode<Nodes::Logic::IsNull>(graphUniqueId, isNullTrueID);

    AZ::EntityId isNullResultFalseID;
    auto isNullResultFalse = CreateDataNode(graphUniqueId, true, isNullResultFalseID);
    AZ::EntityId isNullResultTrueID;
    auto isNullResultTrue = CreateDataNode(graphUniqueId, false, isNullResultTrueID);

    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "Vector3", "Normalize");

    AZ::EntityId vectorID;
    Nodes::Core::BehaviorContextObjectNode* vector = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, vectorID);
    const AZStd::string vector3("Vector3");
    vector->InitializeObject(vector3);
    if (auto vector3 = vector->ModInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        *vector3 = AZ::Vector3(1,1,1);
    }
    else
    {
        ADD_FAILURE();
    }

    // test the reference type contract
    EXPECT_FALSE(Connect(*graph, isNullResultFalseID, "Get", isNullTrueID, "Reference"));
    // data
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", normalizeID, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", isNullFalseID, "Reference"));
    EXPECT_TRUE(Connect(*graph, isNullTrueID, "Is Null", isNullResultTrueID, "Set"));
    EXPECT_TRUE(Connect(*graph, isNullFalseID, "Is Null", isNullResultFalseID, "Set"));
    // execution 
    EXPECT_TRUE(Connect(*graph, startID, "Out", isNullTrueID, "In"));
    EXPECT_TRUE(Connect(*graph, isNullTrueID, "True", isNullFalseID, "In"));
    EXPECT_TRUE(Connect(*graph, isNullFalseID, "False", normalizeID, "In"));
        
    
    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    if (auto vector3 = vector->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        EXPECT_TRUE(vector3->IsNormalized());
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto shouldBeFalse = isNullResultFalse->GetInput_UNIT_TEST<bool>("Set"))
    {
        EXPECT_FALSE(*shouldBeFalse);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto shouldBeTrue = isNullResultTrue->GetInput_UNIT_TEST<bool>("Set"))
    {
        EXPECT_TRUE(*shouldBeTrue);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NullThisPointerDoesNotCrash)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "Vector3", "Normalize");
       
    EXPECT_TRUE(Connect(*graph, startID, "Out", normalizeID, "In"));
    
    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Activate();
    // just don't crash, but be in error
    EXPECT_TRUE(graph->IsInErrorState());

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, Assignment)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Data;
    using namespace Nodes;

    AZ::Vector3 min(1, 1, 1);
    AZ::Vector3 max(2, 2, 2);

    AssignTest(AABBType::CreateFromMinMax(min, max), AABBType::CreateFromMinMax(max, min + max));
    AssignTest(AZ::Color(255.f, 127.f, 0.f, 127.f), AZ::Color(0.f, 127.f, 255.f, 127.f));
    AssignTest(CRCType("lhs"), CRCType("rhs"));
    AssignTest(true, false);
    AssignTest(AZ::EntityId(1), AZ::EntityId(2));
    AssignTest(3.0, 4.0);
    AssignTest(Matrix3x3Type::CreateIdentity(), Matrix3x3Type::CreateZero());
    AssignTest(Matrix4x4Type::CreateIdentity(), Matrix4x4Type::CreateZero());
    AssignTest(PlaneType::CreateFromNormalAndDistance(min, 10), PlaneType::CreateFromNormalAndDistance(max, -10));
    AssignTest(OBBType::CreateFromAabb(AABBType::CreateFromMinMax(min, max)), OBBType::CreateFromAabb(AABBType::CreateFromMinMax(max, min + max)));
    AssignTest(StringType("hello"), StringType("good-bye"));
    AssignTest(RotationType::CreateIdentity(), RotationType::CreateZero());
    AssignTest(TransformType::CreateIdentity(), TransformType::CreateZero());
    AssignTest(AZ::Vector2(2, 2), AZ::Vector2(1, 1));
    AssignTest(AZ::Vector3(2, 2, 2), AZ::Vector3(1, 1, 1));
    AssignTest(AZ::Vector4(2, 2, 2, 2), AZ::Vector4(1, 1, 1, 1));
    
    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId operatorID;
    CreateTestNode<Nodes::Core::Assign>(graphUniqueId, operatorID);
    
    AZ::EntityId sourceID;
    Core::BehaviorContextObjectNode* sourceNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, sourceID);
    sourceNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *sourceNode->ModInput_UNIT_TEST<AZ::Vector3>("Set") = AZ::Vector3(1,1,1);
    
    AZ::EntityId targetID;
    Core::BehaviorContextObjectNode* targetNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, targetID);
    targetNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *targetNode->ModInput_UNIT_TEST<AZ::Vector3>("Set") = AZ::Vector3(2,2,2);
    
    EXPECT_TRUE(Connect(*graph, sourceID, "Get", operatorID, "Source"));
    EXPECT_TRUE(Connect(*graph, targetID, "Set", operatorID, "Target"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", operatorID, "In"));

    graph->SetStartNode(startID);
    graphEntity->Activate();

    EXPECT_FALSE(graph->IsInErrorState());

    if (auto result = targetNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        EXPECT_EQ(AZ::Vector3(1, 1, 1), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;
}

TEST_F(ScriptCanvasTestFixture, NodeGenerics)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    ArgsNoReturnNode::Reflect(m_behaviorContext);
    ArgsNoReturnNode::Reflect(m_serializeContext);
    ArgsReturnMultiNode::Reflect(m_behaviorContext);
    ArgsReturnMultiNode::Reflect(m_serializeContext);
    NoArgsNoReturnNode::Reflect(m_behaviorContext);
    NoArgsNoReturnNode::Reflect(m_serializeContext);
    NoArgsReturnNode::Reflect(m_behaviorContext);
    NoArgsReturnNode::Reflect(m_serializeContext);
    NoArgsReturnMultiNode::Reflect(m_behaviorContext);
    NoArgsReturnMultiNode::Reflect(m_serializeContext);
    
    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId noArgsNoReturnNodeID;
    CreateTestNode<NoArgsNoReturnNode>(graphUniqueId, noArgsNoReturnNodeID);
    AZ::EntityId argsNoReturnNodeID;
    CreateTestNode<ArgsNoReturnNode>(graphUniqueId, argsNoReturnNodeID);
    AZ::EntityId noArgsReturnNodeID;
    CreateTestNode<NoArgsReturnNode>(graphUniqueId, noArgsReturnNodeID);
    
    AZ::EntityId unused0, unused1;
    CreateTestNode<ArgsReturnMultiNode>(graphUniqueId, unused0);
    CreateTestNode<NoArgsReturnMultiNode>(graphUniqueId, unused1);
    
    EXPECT_TRUE(Connect(*graph, startID, "Out", noArgsNoReturnNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, noArgsNoReturnNodeID, "Out", argsNoReturnNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, argsNoReturnNodeID, "Out", noArgsReturnNodeID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    delete graph->GetEntity();

    EXPECT_EQ(unitTestHandler.SideEffectCount(), 3);

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    ArgsNoReturnNode::Reflect(m_behaviorContext);
    ArgsNoReturnNode::Reflect(m_serializeContext);
    ArgsReturnMultiNode::Reflect(m_behaviorContext);
    ArgsReturnMultiNode::Reflect(m_serializeContext);
    NoArgsNoReturnNode::Reflect(m_behaviorContext);
    NoArgsNoReturnNode::Reflect(m_serializeContext);
    NoArgsReturnNode::Reflect(m_behaviorContext);
    NoArgsReturnNode::Reflect(m_serializeContext);
    NoArgsReturnMultiNode::Reflect(m_behaviorContext);
    NoArgsReturnMultiNode::Reflect(m_serializeContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByValue)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
        
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByValueNode::Reflect(m_serializeContext);
    MaxReturnByValueNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByValueNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    
    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_NE(value3, value1);
        EXPECT_NE(value3, value2);
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByValueNode::Reflect(m_serializeContext);
    MaxReturnByValueNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByPointer)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByPointerNode::Reflect(m_serializeContext);
    MaxReturnByPointerNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByPointerNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_NE(value3, value1);
        EXPECT_EQ(value3, value2);
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByPointerNode::Reflect(m_serializeContext);
    MaxReturnByPointerNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByReference)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByReferenceNode::Reflect(m_serializeContext);
    MaxReturnByReferenceNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByReferenceNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_NE(value3, value1);
        EXPECT_EQ(value3, value2);
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByReferenceNode::Reflect(m_serializeContext);
    MaxReturnByReferenceNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByValueInteger)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByValueIntegerNode::Reflect(m_serializeContext);
    MaxReturnByValueIntegerNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId graphEntityId = graph->GetEntityId();
    const AZ::EntityId graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByValueIntegerNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Node* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto value1 = valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value1);
        EXPECT_NE(value3, value2);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByValueIntegerNode::Reflect(m_serializeContext);
    MaxReturnByValueIntegerNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByPointerInteger)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByPointerIntegerNode::Reflect(m_serializeContext);
    MaxReturnByPointerIntegerNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByPointerIntegerNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Node* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto value1 = valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value1);
        EXPECT_NE(value3, value2);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByPointerIntegerNode::Reflect(m_serializeContext);
    MaxReturnByPointerIntegerNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByReferenceInteger)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByReferenceIntegerNode::Reflect(m_serializeContext);
    MaxReturnByReferenceIntegerNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByReferenceIntegerNode>(graphUniqueId, maxByValueID);

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Node* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto value1 = valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value1);
        EXPECT_NE(value3, value2);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByReferenceIntegerNode::Reflect(m_serializeContext);
    MaxReturnByReferenceIntegerNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByValueMulti)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByValueMultiNode::Reflect(m_serializeContext);
    MaxReturnByValueMultiNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID;
    CreateTestNode<MaxReturnByValueMultiNode>(graphUniqueId, maxByValueID);
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    AZ::EntityId valueIntegerID1, valueIntegerID2, valueIntegerID3, valueIntegerID4, valueIntegerID5;

    Node* valueIntegerNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueIntegerID1);
    EXPECT_EQ(1, *valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueIntegerID2);
    EXPECT_EQ(2, *valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueIntegerID3);
    EXPECT_EQ(3, *valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueIntegerID4);
    EXPECT_EQ(4, *valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueIntegerID5);
    EXPECT_EQ(5, *valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto valueInteger1 = valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger2 = valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(valueInteger1, valueInteger2);
    EXPECT_NE(valueInteger1, valueInteger3);
    EXPECT_NE(valueInteger1, valueInteger4);
    EXPECT_NE(valueInteger1, valueInteger5);
    EXPECT_NE(valueInteger2, valueInteger3);
    EXPECT_NE(valueInteger2, valueInteger4);
    EXPECT_NE(valueInteger2, valueInteger5);
    EXPECT_NE(valueInteger3, valueInteger4);
    EXPECT_NE(valueInteger3, valueInteger5);
    EXPECT_NE(valueInteger4, valueInteger5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    EXPECT_TRUE(Connect(*graph, valueIntegerID1, "Get", maxByValueID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID2, "Get", maxByValueID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueIntegerID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID3, "Get", valueIntegerID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: Number", valueIntegerID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (valueInteger3 && valueInteger4 && valueInteger5)
    {
        EXPECT_EQ(2, *valueInteger3);
        EXPECT_EQ(2, *valueInteger4);
        EXPECT_EQ(2, *valueInteger5);
        EXPECT_NE(valueInteger3, valueInteger1);
        EXPECT_NE(valueInteger3, valueInteger2);
        EXPECT_NE(valueInteger3, valueInteger4);
        EXPECT_NE(valueInteger3, valueInteger5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_NE(value3, value1);
        EXPECT_NE(value3, value2);
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByValueMultiNode::Reflect(m_serializeContext);
    MaxReturnByValueMultiNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByReferenceMulti)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByReferenceMultiNode::Reflect(m_serializeContext);
    MaxReturnByReferenceMultiNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByReferenceID;
    CreateTestNode<MaxReturnByReferenceMultiNode>(graphUniqueId, maxByReferenceID);
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    AZ::EntityId valueIntegerID1, valueIntegerID2, valueIntegerID3, valueIntegerID4, valueIntegerID5;

    Node* valueIntegerNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueIntegerID1);
    EXPECT_EQ(1, *valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueIntegerID2);
    EXPECT_EQ(2, *valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueIntegerID3);
    EXPECT_EQ(3, *valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueIntegerID4);
    EXPECT_EQ(4, *valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueIntegerID5);
    EXPECT_EQ(5, *valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto valueInteger1 = valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger2 = valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(valueInteger1, valueInteger2);
    EXPECT_NE(valueInteger1, valueInteger3);
    EXPECT_NE(valueInteger1, valueInteger4);
    EXPECT_NE(valueInteger1, valueInteger5);
    EXPECT_NE(valueInteger2, valueInteger3);
    EXPECT_NE(valueInteger2, valueInteger4);
    EXPECT_NE(valueInteger2, valueInteger5);
    EXPECT_NE(valueInteger3, valueInteger4);
    EXPECT_NE(valueInteger3, valueInteger5);
    EXPECT_NE(valueInteger4, valueInteger5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByReferenceID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByReferenceID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    EXPECT_TRUE(Connect(*graph, valueIntegerID1, "Get", maxByReferenceID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID2, "Get", maxByReferenceID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: Number", valueIntegerID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID3, "Get", valueIntegerID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: Number", valueIntegerID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByReferenceID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (valueInteger3 && valueInteger4 && valueInteger5)
    {
        EXPECT_EQ(2, *valueInteger3);
        EXPECT_EQ(2, *valueInteger4);
        EXPECT_EQ(2, *valueInteger5);
        EXPECT_NE(valueInteger3, valueInteger1);
        EXPECT_NE(valueInteger3, valueInteger2);
        EXPECT_NE(valueInteger3, valueInteger4);
        EXPECT_NE(valueInteger3, valueInteger5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_NE(value3, value1);
        EXPECT_EQ(value3, value2);
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByReferenceMultiNode::Reflect(m_serializeContext);
    MaxReturnByReferenceMultiNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NodeGenericsByPointerMulti)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByPointerMultiNode::Reflect(m_serializeContext);
    MaxReturnByPointerMultiNode::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByPointerID;
    CreateTestNode<MaxReturnByPointerMultiNode>(graphUniqueId, maxByPointerID);
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    auto value1 = valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value2 = valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1, value3);
    EXPECT_NE(value1, value4);
    EXPECT_NE(value1, value5);
    EXPECT_NE(value2, value3);
    EXPECT_NE(value2, value4);
    EXPECT_NE(value2, value5);
    EXPECT_NE(value3, value4);
    EXPECT_NE(value3, value5);
    EXPECT_NE(value4, value5);

    AZ::EntityId valueIntegerID1, valueIntegerID2, valueIntegerID3, valueIntegerID4, valueIntegerID5;

    Node* valueIntegerNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueIntegerID1);
    EXPECT_EQ(1, *valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueIntegerID2);
    EXPECT_EQ(2, *valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueIntegerID3);
    EXPECT_EQ(3, *valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueIntegerID4);
    EXPECT_EQ(4, *valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    Node* valueIntegerNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueIntegerID5);
    EXPECT_EQ(5, *valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto valueInteger1 = valueIntegerNode1->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger2 = valueIntegerNode2->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    EXPECT_NE(valueInteger1, valueInteger2);
    EXPECT_NE(valueInteger1, valueInteger3);
    EXPECT_NE(valueInteger1, valueInteger4);
    EXPECT_NE(valueInteger1, valueInteger5);
    EXPECT_NE(valueInteger2, valueInteger3);
    EXPECT_NE(valueInteger2, valueInteger4);
    EXPECT_NE(valueInteger2, valueInteger5);
    EXPECT_NE(valueInteger3, valueInteger4);
    EXPECT_NE(valueInteger3, valueInteger5);
    EXPECT_NE(valueInteger4, valueInteger5);

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByPointerID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByPointerID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    EXPECT_TRUE(Connect(*graph, valueIntegerID1, "Get", maxByPointerID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID2, "Get", maxByPointerID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueIntegerID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueIntegerID3, "Get", valueIntegerID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueIntegerID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByPointerID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    valueInteger3 = valueIntegerNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    valueInteger4 = valueIntegerNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    valueInteger5 = valueIntegerNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (valueInteger3 && valueInteger4 && valueInteger5)
    {
        EXPECT_EQ(2, *valueInteger3);
        EXPECT_EQ(2, *valueInteger4);
        EXPECT_EQ(2, *valueInteger5);
        EXPECT_NE(valueInteger3, valueInteger1);
        EXPECT_NE(valueInteger3, valueInteger2);
        EXPECT_NE(valueInteger3, valueInteger4);
        EXPECT_NE(valueInteger3, valueInteger5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_NE(value3, value1);
        EXPECT_EQ(value3, value2);
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    MaxReturnByPointerMultiNode::Reflect(m_serializeContext);
    MaxReturnByPointerMultiNode::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, MathCustom)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId graphEntityId = graph->GetEntityId();
    const AZ::EntityId graphUniqueId = graph->GetUniqueId();
    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::Vector3 allOne(1, 1, 1);

    AZ::EntityId justCompile;
    CreateTestNode<Nodes::Math::AABB>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Color>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::CRC>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Matrix3x3>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Matrix4x4>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::OBB>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Plane>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Vector2>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, justCompile);
    CreateTestNode<Nodes::Math::Vector4>(graphUniqueId, justCompile);
    
    AZ::EntityId addVectorId;
    Node* addVector = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne, addVectorId);
    AZ::EntityId subtractVectorId;
    Node* subtractVector = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne, subtractVectorId);
    AZ::EntityId normalizeVectorId;
    Node* normalizeVector = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne, normalizeVectorId);

    AZ::EntityId addId;
    CreateTestNode<VectorAddNode>(graphUniqueId, addId);
    AZ::EntityId subtractId;
    CreateTestNode<VectorSubtractNode>(graphUniqueId, subtractId);
    AZ::EntityId normalizeId;
    CreateTestNode<VectorNormalizeNode>(graphUniqueId, normalizeId);

    EXPECT_TRUE(Connect(*graph, addVectorId, "Get", addId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, addVectorId, "Get", addId, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, addVectorId, "Set", addId, "Result: Vector3"));

    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Get", subtractId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Get", subtractId, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Set", subtractId, "Result: Vector3"));

    EXPECT_TRUE(Connect(*graph, normalizeVectorId, "Get", normalizeId, "Vector3: Vector"));
    EXPECT_TRUE(Connect(*graph, normalizeVectorId, "Set", normalizeId, "Result: Vector3"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", addId, "In"));
    EXPECT_TRUE(Connect(*graph, addId, "Out", subtractId, "In"));
    EXPECT_TRUE(Connect(*graph, subtractId, "Out", normalizeId, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    EXPECT_EQ(*addVector->GetInput_UNIT_TEST<AZ::Vector3>("Set"), AZ::Vector3(2, 2, 2));
    EXPECT_EQ(*subtractVector->GetInput_UNIT_TEST<AZ::Vector3>("Set"), AZ::Vector3::CreateZero());
    EXPECT_TRUE(normalizeVector->GetInput_UNIT_TEST<AZ::Vector3>("Set")->IsNormalized());

    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, MathMixed1)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();
    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    const AZ::Vector3 allOne4(1, 1, 1);
    const AZ::Vector3 allTwo4(2, 2, 2);
    const AZ::Vector3 allFour4(4, 4, 4);

    const AZ::Vector3 allOne3(allOne4);
    const AZ::Vector3 allTwo3(allTwo4);
    const AZ::Vector3 allFour3(allFour4);

    AZ::EntityId vectorAId, vectorBId, vectorCId, vectorDId;
    Node* vectorANode = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorAId);
    Node* vectorBNode = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorBId);
    Node* vectorCNode = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorCId);
    Node* vectorDNode = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorDId);

    AZ::EntityId vector3AId, vector3BId, vector3CId, vector3DId;

    Core::BehaviorContextObjectNode* vector3ANode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3AId);
    vector3ANode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3ANode->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne3;

    Core::BehaviorContextObjectNode* vector3BNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3BId);
    vector3BNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3BNode->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne3;

    Core::BehaviorContextObjectNode* vector3CNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3CId);
    vector3CNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3CNode->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne3;

    Core::BehaviorContextObjectNode* vector3DNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3DId);
    vector3DNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3DNode->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne3;

    AZ::EntityId addId;
    CreateTestNode<VectorAddNode>(graphUniqueId, addId);
    AZ::EntityId add3Id = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");

    // data connections
    EXPECT_TRUE(Connect(*graph, vectorAId, "Get", addId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, vector3AId, "Get", addId, "Vector3: B"));

    EXPECT_TRUE(Connect(*graph, addId, "Result: Vector3", vector3BId, "Set"));
    EXPECT_TRUE(Connect(*graph, addId, "Result: Vector3", vectorBId, "Set"));

    EXPECT_TRUE(Connect(*graph, vectorBId, "Get", vector3CId, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3CId, "Get", vectorCId, "Set"));

    EXPECT_TRUE(Connect(*graph, vector3BId, "Get", add3Id, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vector3CId, "Get", add3Id, "Vector3: 1"));

    EXPECT_TRUE(Connect(*graph, add3Id, "Result: Vector3", vectorDId, "Set"));
    EXPECT_TRUE(Connect(*graph, add3Id, "Result: Vector3", vector3DId, "Set"));

    // execution connections
    EXPECT_TRUE(Connect(*graph, startID, "Out", addId, "In"));
    EXPECT_TRUE(Connect(*graph, addId, "Out", add3Id, "In"));

    EXPECT_EQ(*vectorANode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorBNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorCNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorDNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vector3ANode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne3);
    EXPECT_EQ(*vector3BNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne3);
    EXPECT_EQ(*vector3CNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne3);
    EXPECT_EQ(*vector3DNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne3);

    auto vector3Aptr = vector3ANode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3Bptr = vector3BNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3Cptr = vector3CNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3Dptr = vector3DNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    auto vectorA = *vectorANode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vectorB = *vectorBNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vectorC = *vectorCNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vectorD = *vectorDNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");

    auto vector3A = *vector3ANode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3B = *vector3BNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3C = *vector3CNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3D = *vector3DNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");

    EXPECT_EQ(vectorA, allOne4);
    EXPECT_EQ(vectorB, allTwo4);
    EXPECT_EQ(vectorC, allTwo4);
    EXPECT_EQ(vectorD, allFour4);

    EXPECT_EQ(vector3A, allOne3);
    EXPECT_EQ(vector3B, allTwo3);
    EXPECT_EQ(vector3C, allTwo3);
    EXPECT_EQ(vector3D, allFour3);

    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, MathMixed2)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();
    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    const AZ::Vector3 allOne4(1, 1, 1);
    const AZ::Vector3 allEight4(8, 8, 8);

    const AZ::Vector3 allOne3(allOne4);
    const AZ::Vector3 allEight3(allEight4);

    AZ::EntityId vectorIdA, vectorIdB, vectorIdC, vectorIdD;
    Node* vectorNodeA = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorIdA);
    Node* vectorNodeB = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorIdB);
    Node* vectorNodeC = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorIdC);
    Node* vectorNodeD = CreateDataNode<AZ::Vector3>(graphUniqueId, allOne4, vectorIdD);

    AZ::EntityId vector3IdA, vector3IdB, vector3IdC, vector3IdD;
    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeA->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne3;
    Core::BehaviorContextObjectNode* vector3NodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdB);
    vector3NodeB->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeB->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne3;
    Core::BehaviorContextObjectNode* vector3NodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdC);
    vector3NodeC->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeC->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne3;
    Core::BehaviorContextObjectNode* vector3NodeD = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdD);
    vector3NodeD->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeD->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne3;

    AZ::EntityId addIdA;
    CreateTestNode<VectorAddNode>(graphUniqueId, addIdA);
    AZ::EntityId addIdB;
    CreateTestNode<VectorAddNode>(graphUniqueId, addIdB);
    AZ::EntityId addIdC;
    CreateTestNode<VectorAddNode>(graphUniqueId, addIdC);

    AZ::EntityId add3IdA = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");
    AZ::EntityId add3IdB = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");
    AZ::EntityId add3IdC = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");

    AZ::EntityId vectorNormalizeWithLengthId;
    CreateTestNode<VectorNormalizeWithLengthNode>(graphUniqueId, vectorNormalizeWithLengthId);

    const AZ::Vector3 x10(10, 0, 0);
    AZ::EntityId vectorNormalizedInId, vectorNormalizedOutId, numberLengthId;
    Node* vectorNormalizedInNode = CreateDataNode<AZ::Vector3>(graphUniqueId, x10, vectorNormalizedInId);
    Node* vectorNormalizedOutNode = CreateDataNode<AZ::Vector3>(graphUniqueId, x10, vectorNormalizedOutId);
    Node* numberLengthNode = CreateDataNode<Data::NumberType>(graphUniqueId, -1, numberLengthId);

    // data connections
    
    EXPECT_TRUE(Connect(*graph, vectorIdA, "Get", addIdA, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, vectorIdA, "Get", add3IdA, "Vector3: 0"));

    EXPECT_TRUE(Connect(*graph, addIdA, "Result: Vector3", addIdB, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, addIdA, "Result: Vector3", add3IdB, "Vector3: 0"));

    EXPECT_TRUE(Connect(*graph, addIdB, "Result: Vector3", addIdC, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, addIdB, "Result: Vector3", add3IdC, "Vector3: 0"));

    EXPECT_TRUE(Connect(*graph, addIdC, "Result: Vector3", vectorIdB, "Set"));
    EXPECT_TRUE(Connect(*graph, addIdC, "Result: Vector3", vector3IdB, "Set"));

    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", vectorIdC, "Set"));
    
    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", addIdA, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", add3IdA, "Vector3: 1"));

    EXPECT_TRUE(Connect(*graph, add3IdA, "Result: Vector3", addIdB, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, add3IdA, "Result: Vector3", add3IdB, "Vector3: 1"));

    EXPECT_TRUE(Connect(*graph, add3IdB, "Result: Vector3", addIdC, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, add3IdB, "Result: Vector3", add3IdC, "Vector3: 1"));

    EXPECT_TRUE(Connect(*graph, add3IdC, "Result: Vector3", vectorIdD, "Set"));
    EXPECT_TRUE(Connect(*graph, add3IdC, "Result: Vector3", vector3IdD, "Set"));

    EXPECT_TRUE(Connect(*graph, vectorIdD, "Get", vector3IdC, "Set"));

    EXPECT_TRUE(Connect(*graph, vectorNormalizedInId, "Get", vectorNormalizeWithLengthId, "Vector3: Vector"));
    EXPECT_TRUE(Connect(*graph, vectorNormalizeWithLengthId, "Normalized: Vector3", vectorNormalizedOutId, "Set"));
    EXPECT_TRUE(Connect(*graph, vectorNormalizeWithLengthId, "Length: Number", numberLengthId, "Set"));

    // execution connections
    EXPECT_TRUE(Connect(*graph, startID, "Out", addIdA, "In"));
    EXPECT_TRUE(Connect(*graph, addIdA, "Out", add3IdA, "In"));
    EXPECT_TRUE(Connect(*graph, add3IdA, "Out", addIdB, "In"));
    EXPECT_TRUE(Connect(*graph, addIdB, "Out", add3IdB, "In"));
    EXPECT_TRUE(Connect(*graph, add3IdB, "Out", addIdC, "In"));
    EXPECT_TRUE(Connect(*graph, addIdC, "Out", add3IdC, "In"));

    EXPECT_TRUE(Connect(*graph, add3IdC, "Out", vectorNormalizeWithLengthId, "In"));

    EXPECT_EQ(*vectorNodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorNodeB->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorNodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorNodeD->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);

    EXPECT_EQ(*vectorNormalizedInNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), x10);
    EXPECT_EQ(*vectorNormalizedOutNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), x10);
    EXPECT_EQ(*numberLengthNode->GetInput_UNIT_TEST<Data::NumberType>("Set"), -1);

    EXPECT_EQ(*vector3NodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne3);
    EXPECT_EQ(*vector3NodeB->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne3);
    EXPECT_EQ(*vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne3);
    EXPECT_EQ(*vector3NodeD->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne3);

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    auto vectorA = *vectorNodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vectorB = *vectorNodeB->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vectorC = *vectorNodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vectorD = *vectorNodeD->GetInput_UNIT_TEST<AZ::Vector3>("Set");

    auto vector3A = *vector3NodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3Aptr = vector3NodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    
    auto vector3B = *vector3NodeB->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3C = *vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto vector3D = *vector3NodeD->GetInput_UNIT_TEST<AZ::Vector3>("Set");

    EXPECT_EQ(vectorA, allOne4);
    EXPECT_EQ(vectorB, allEight4);
    EXPECT_EQ(vectorC, allEight4);
    EXPECT_EQ(vectorD, allEight4);

    EXPECT_EQ(vector3A, allOne3);
    EXPECT_EQ(vector3B, allEight3);
    EXPECT_EQ(vector3C, allEight3);
    EXPECT_EQ(vector3D, allEight3);

    auto normalizedIn = *vectorNormalizedInNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto normalizedOut = *vectorNormalizedOutNode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    auto numberLength = *numberLengthNode->GetInput_UNIT_TEST<Data::NumberType>("Set");
    EXPECT_EQ(x10, normalizedIn);
    EXPECT_EQ(AZ::Vector3(1, 0, 0), normalizedOut);
    EXPECT_EQ(10, numberLength);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, MathMixed3)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();
    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    auto identity = Data::TransformType::CreateIdentity();
    auto zero = Data::TransformType::CreateZero();
    auto initial = zero;

    AZ::EntityId transformIdA, transformIdB, transformIdC, transformIdD;
    Node* transformNodeA = CreateDataNode<Data::TransformType>(graphUniqueId, identity, transformIdA);
    Node* transformNodeB = CreateDataNode<Data::TransformType>(graphUniqueId, zero, transformIdB);
    Node* transformNodeC = CreateDataNode<Data::TransformType>(graphUniqueId, zero, transformIdC);
    Node* transformNodeD = CreateDataNode<Data::TransformType>(graphUniqueId, zero, transformIdD);

    AZ::EntityId bcTransformIdA, bcTransformIdB, bcTransformIdC, bcTransformIdD;
    Core::BehaviorContextObjectNode* bcTransformNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, bcTransformIdA);
    bcTransformNodeA->InitializeObject(azrtti_typeid<AZ::Transform>());
    *bcTransformNodeA->ModInput_UNIT_TEST<AZ::Transform>("Set") = initial;
    Core::BehaviorContextObjectNode* bcTransformNodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, bcTransformIdB);
    bcTransformNodeB->InitializeObject(azrtti_typeid<AZ::Transform>());
    *bcTransformNodeB->ModInput_UNIT_TEST<AZ::Transform>("Set") = initial;
    Core::BehaviorContextObjectNode* bcTransformNodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, bcTransformIdC);
    bcTransformNodeC->InitializeObject(azrtti_typeid<AZ::Transform>());
    *bcTransformNodeC->ModInput_UNIT_TEST<AZ::Transform>("Set") = initial;
    Core::BehaviorContextObjectNode* bcTransformNodeD = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, bcTransformIdD);
    bcTransformNodeD->InitializeObject(azrtti_typeid<AZ::Transform>());
    *bcTransformNodeD->ModInput_UNIT_TEST<AZ::Transform>("Set") = initial;

    AZ::EntityId operatorID;
    CreateTestNode<Nodes::Core::Assign>(graphUniqueId, operatorID);
   
    EXPECT_TRUE(Connect(*graph, transformIdA, "Get", operatorID, "Source"));
    EXPECT_TRUE(Connect(*graph, operatorID, "Target", bcTransformIdA, "Set"));
    EXPECT_TRUE(Connect(*graph, bcTransformIdA, "Get", transformIdB, "Set"));
    
    EXPECT_TRUE(Connect(*graph, transformIdB, "Get", bcTransformIdB, "Set"));
    EXPECT_TRUE(Connect(*graph, bcTransformIdB, "Get", transformIdC, "Set"));

    EXPECT_TRUE(Connect(*graph, transformIdC, "Get", bcTransformIdC, "Set"));
    EXPECT_TRUE(Connect(*graph, bcTransformIdC, "Get", transformIdD, "Set"));

    EXPECT_TRUE(Connect(*graph, transformIdD, "Get", bcTransformIdD, "Set"));
    
    EXPECT_TRUE(Connect(*graph, startID, "Out", operatorID, "In"));
    
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    auto resultA = transformNodeA->GetInput_UNIT_TEST<Data::TransformType>("Set");
    if (resultA)
    {
        EXPECT_EQ(identity, *resultA);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultB = transformNodeB->GetInput_UNIT_TEST<Data::TransformType>("Set");
    if (resultB)
    {
        EXPECT_EQ(identity, *resultB);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultC = transformNodeC->GetInput_UNIT_TEST<Data::TransformType>("Set");
    if (resultC)
    {
        EXPECT_EQ(identity, *resultC);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultD = transformNodeD->GetInput_UNIT_TEST<Data::TransformType>("Set");
    if (resultD)
    {
        EXPECT_EQ(identity, *resultD);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultAbc = bcTransformNodeA->GetInput_UNIT_TEST<Data::TransformType>("Set");
    if (resultAbc)
    {
        EXPECT_EQ(identity, *resultAbc);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultBbc = bcTransformNodeB->GetInput_UNIT_TEST<Data::TransformType>("Set");
    if (resultBbc)
    {
        EXPECT_EQ(identity, *resultBbc);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultCbc = bcTransformNodeC->GetInput_UNIT_TEST<Data::TransformType>("Set");
    if (resultCbc)
    {
        EXPECT_EQ(identity, *resultCbc);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultDbc = bcTransformNodeD->GetInput_UNIT_TEST<Data::TransformType>("Set");
    if (resultDbc)
    {
        EXPECT_EQ(identity, *resultDbc);
    }
    else
    {
        ADD_FAILURE();
    }

    EXPECT_NE(resultA, resultB);
    EXPECT_NE(resultA, resultC);
    EXPECT_NE(resultA, resultD);
    EXPECT_NE(resultA, resultAbc);
    EXPECT_NE(resultA, resultBbc);
    EXPECT_NE(resultA, resultCbc);
    EXPECT_NE(resultA, resultDbc);

    EXPECT_NE(resultB, resultC);
    EXPECT_NE(resultB, resultD);
    EXPECT_NE(resultB, resultAbc);
    EXPECT_NE(resultB, resultBbc);
    EXPECT_NE(resultB, resultCbc);
    EXPECT_NE(resultB, resultDbc);

    EXPECT_NE(resultC, resultD);
    EXPECT_NE(resultC, resultAbc);
    EXPECT_NE(resultC, resultBbc);
    EXPECT_NE(resultC, resultCbc);
    EXPECT_NE(resultC, resultDbc);

    EXPECT_NE(resultD, resultAbc);
    EXPECT_NE(resultD, resultBbc);
    EXPECT_NE(resultD, resultCbc);
    EXPECT_NE(resultD, resultDbc);

    EXPECT_NE(resultAbc, resultBbc);
    EXPECT_NE(resultAbc, resultCbc);
    EXPECT_NE(resultAbc, resultDbc);

    EXPECT_NE(resultBbc, resultCbc);
    EXPECT_NE(resultBbc, resultDbc);

    EXPECT_NE(resultCbc, resultDbc);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, MathMixed4)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();
    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    auto identity = Data::RotationType::CreateIdentity();
    auto zero = Data::RotationType::CreateZero();
    auto initial = zero;

    AZ::EntityId transformIdA, transformIdB, transformIdC, transformIdD;
    Node* transformNodeA = CreateDataNode<Data::RotationType>(graphUniqueId, identity, transformIdA);
    Node* transformNodeB = CreateDataNode<Data::RotationType>(graphUniqueId, zero, transformIdB);
    Node* transformNodeC = CreateDataNode<Data::RotationType>(graphUniqueId, zero, transformIdC);
    Node* transformNodeD = CreateDataNode<Data::RotationType>(graphUniqueId, zero, transformIdD);

    AZ::EntityId bcRotationIdA, bcRotationIdB, bcRotationIdC, bcRotationIdD;
    Core::BehaviorContextObjectNode* bcRotationNodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, bcRotationIdA);
    bcRotationNodeA->InitializeObject(azrtti_typeid<AZ::Quaternion>());
    *bcRotationNodeA->ModInput_UNIT_TEST<AZ::Quaternion>("Set") = initial;
    Core::BehaviorContextObjectNode* bcRotationNodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, bcRotationIdB);
    bcRotationNodeB->InitializeObject(azrtti_typeid<AZ::Quaternion>());
    *bcRotationNodeB->ModInput_UNIT_TEST<AZ::Quaternion>("Set") = initial;
    Core::BehaviorContextObjectNode* bcRotationNodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, bcRotationIdC);
    bcRotationNodeC->InitializeObject(azrtti_typeid<AZ::Quaternion>());
    *bcRotationNodeC->ModInput_UNIT_TEST<AZ::Quaternion>("Set") = initial;
    Core::BehaviorContextObjectNode* bcRotationNodeD = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, bcRotationIdD);
    bcRotationNodeD->InitializeObject(azrtti_typeid<AZ::Quaternion>());
    *bcRotationNodeD->ModInput_UNIT_TEST<AZ::Quaternion>("Set") = initial;

    AZ::EntityId operatorID;
    CreateTestNode<Nodes::Core::Assign>(graphUniqueId, operatorID);

    EXPECT_TRUE(Connect(*graph, transformIdA, "Get", operatorID, "Source"));
    EXPECT_TRUE(Connect(*graph, operatorID, "Target", bcRotationIdA, "Set"));
    EXPECT_TRUE(Connect(*graph, bcRotationIdA, "Get", transformIdB, "Set"));

    EXPECT_TRUE(Connect(*graph, transformIdB, "Get", bcRotationIdB, "Set"));
    EXPECT_TRUE(Connect(*graph, bcRotationIdB, "Get", transformIdC, "Set"));

    EXPECT_TRUE(Connect(*graph, transformIdC, "Get", bcRotationIdC, "Set"));
    EXPECT_TRUE(Connect(*graph, bcRotationIdC, "Get", transformIdD, "Set"));

    EXPECT_TRUE(Connect(*graph, transformIdD, "Get", bcRotationIdD, "Set"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", operatorID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    auto resultA = transformNodeA->GetInput_UNIT_TEST<Data::RotationType>("Set");
    if (resultA)
    {
        EXPECT_EQ(identity, *resultA);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultB = transformNodeB->GetInput_UNIT_TEST<Data::RotationType>("Set");
    if (resultB)
    {
        EXPECT_EQ(identity, *resultB);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultC = transformNodeC->GetInput_UNIT_TEST<Data::RotationType>("Set");
    if (resultC)
    {
        EXPECT_EQ(identity, *resultC);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultD = transformNodeD->GetInput_UNIT_TEST<Data::RotationType>("Set");
    if (resultD)
    {
        EXPECT_EQ(identity, *resultD);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultAbc = bcRotationNodeA->GetInput_UNIT_TEST<Data::RotationType>("Set");
    if (resultAbc)
    {
        EXPECT_EQ(identity, *resultAbc);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultBbc = bcRotationNodeB->GetInput_UNIT_TEST<Data::RotationType>("Set");
    if (resultBbc)
    {
        EXPECT_EQ(identity, *resultBbc);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultCbc = bcRotationNodeC->GetInput_UNIT_TEST<Data::RotationType>("Set");
    if (resultCbc)
    {
        EXPECT_EQ(identity, *resultCbc);
    }
    else
    {
        ADD_FAILURE();
    }

    auto resultDbc = bcRotationNodeD->GetInput_UNIT_TEST<Data::RotationType>("Set");
    if (resultDbc)
    {
        EXPECT_EQ(identity, *resultDbc);
    }
    else
    {
        ADD_FAILURE();
    }

    EXPECT_NE(resultA, resultB);
    EXPECT_NE(resultA, resultC);
    EXPECT_NE(resultA, resultD);
    EXPECT_NE(resultA, resultAbc);
    EXPECT_NE(resultA, resultBbc);
    EXPECT_NE(resultA, resultCbc);
    EXPECT_NE(resultA, resultDbc);

    EXPECT_NE(resultB, resultC);
    EXPECT_NE(resultB, resultD);
    EXPECT_NE(resultB, resultAbc);
    EXPECT_NE(resultB, resultBbc);
    EXPECT_NE(resultB, resultCbc);
    EXPECT_NE(resultB, resultDbc);

    EXPECT_NE(resultC, resultD);
    EXPECT_NE(resultC, resultAbc);
    EXPECT_NE(resultC, resultBbc);
    EXPECT_NE(resultC, resultCbc);
    EXPECT_NE(resultC, resultDbc);

    EXPECT_NE(resultD, resultAbc);
    EXPECT_NE(resultD, resultBbc);
    EXPECT_NE(resultD, resultCbc);
    EXPECT_NE(resultD, resultDbc);

    EXPECT_NE(resultAbc, resultBbc);
    EXPECT_NE(resultAbc, resultCbc);
    EXPECT_NE(resultAbc, resultDbc);

    EXPECT_NE(resultBbc, resultCbc);
    EXPECT_NE(resultBbc, resultDbc);

    EXPECT_NE(resultCbc, resultDbc);

    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, ValueTypes)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Datum number0(Datum::CreateInitializedCopy(0));
    int number0Value = *number0.GetAs<int>();

    Datum number1(Datum::CreateInitializedCopy(1));
    int number1Value = *number1.GetAs<int>();

    Datum numberFloat(Datum::CreateInitializedCopy(2.f));
    float numberFloatValue = *numberFloat.GetAs<float>();

    Datum numberDouble(Datum::CreateInitializedCopy(3.0));
    double numberDoubleValue = *numberDouble.GetAs<double>();

    Datum numberHex(Datum::CreateInitializedCopy(0xff));
    int numberHexValue = *numberHex.GetAs<int>();

    Datum numberPi(Datum::CreateInitializedCopy(3.14f));
    float numberPiValue = *numberPi.GetAs<float>();

    Datum numberSigned(Datum::CreateInitializedCopy(-100));
    int numberSignedValue = *numberSigned.GetAs<int>();

    Datum numberUnsigned(Datum::CreateInitializedCopy(100u));
    unsigned int numberUnsignedValue = *numberUnsigned.GetAs<unsigned int>();

    Datum numberDoublePi(Datum::CreateInitializedCopy(6.28));
    double numberDoublePiValue = *numberDoublePi.GetAs<double>();

    EXPECT_EQ(number0Value, 0);
    EXPECT_EQ(number1Value, 1);

    EXPECT_TRUE(AZ::IsClose(numberFloatValue, 2.f, std::numeric_limits<float>::epsilon()));
    EXPECT_TRUE(AZ::IsClose(numberDoubleValue, 3.0, std::numeric_limits<double>::epsilon()));

    EXPECT_NE(number0Value, number1Value);
    EXPECT_FLOAT_EQ(numberPiValue, 3.14f);

    EXPECT_NE(number0Value, numberPiValue);
    EXPECT_NE(numberPiValue, numberDoublePiValue);

    Datum boolTrue(Datum::CreateInitializedCopy(true));
    EXPECT_TRUE(*boolTrue.GetAs<bool>());
    Datum boolFalse(Datum::CreateInitializedCopy(false));
    EXPECT_FALSE(*boolFalse.GetAs<bool>());
    boolFalse = boolTrue;
    EXPECT_TRUE(*boolFalse.GetAs<bool>());
    Datum boolFalse2(Datum::CreateInitializedCopy(false));
    boolTrue = boolFalse2;
    EXPECT_FALSE(*boolTrue.GetAs<bool>());
    
    {
        const int k_count(10000);
        AZStd::vector<Datum> objects;
        for (int i = 0; i < k_count; ++i)
        {
            objects.push_back(Datum("Vector3", Datum::eOriginality::Original));
        }
    }

    GTEST_SUCCEED();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST_F(ScriptCanvasTestFixture, MathOperations)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes::Math;

    BinaryOpTest<Multiply>(3, 4, 3 * 4);
    BinaryOpTest<Multiply>(3.5f, 2.0f, 3.5f*2.0f);
    BinaryOpTest<Sum>(3, 4, 3 + 4);
    BinaryOpTest<Sum>(3.f, 4.f, 3.0f + 4.0f);
    BinaryOpTest<Subtract>(3, 4, 3 - 4);
    BinaryOpTest<Subtract>(3.f, 4.f, 3.f - 4.f);
    BinaryOpTest<Divide>(3, 4, 3 / 4);
    BinaryOpTest<Divide>(3.f, 4.f, 3.f / 4.f);
}

/*
template <typename T>
void TestGetRandomIntegral(T lhs, T rhs)
{
const T randValue = ScriptCanvas::MathRandom::GetRandomIntegral<T>(lhs, rhs);
const T min = AZ::GetMin(lhs, rhs);
const T max = AZ::GetMax(lhs, rhs);
EXPECT_TRUE(randValue >= min && randValue <= max);
}

template <typename T>
void TestGetRandomReal(T lhs, T rhs)
{
const T randValue = ScriptCanvas::MathRandom::GetRandomReal<T>(lhs, rhs);
const T min = AZ::GetMin(lhs, rhs);
const T max = AZ::GetMax(lhs, rhs);
EXPECT_TRUE(randValue >= min && randValue <= max);
}

TEST_F(ScriptCanvasTestFixture, MathOperationsRandom)
{
TestGetRandomIntegral<char>('a', 'z');
TestGetRandomIntegral<AZ::s8>(-127, 127);
TestGetRandomIntegral<AZ::u8>(1, 255);
TestGetRandomIntegral<AZ::s16>(-200, 200);
TestGetRandomIntegral<AZ::u16>(200, 2000);
TestGetRandomIntegral<AZ::s32>(-400, 400);
TestGetRandomIntegral<AZ::u32>(400, 4000);
TestGetRandomIntegral<AZ::s64>(-800, 800);
TestGetRandomIntegral<AZ::u64>(800, 8000);

TestGetRandomReal<float>(0.0f, 1.0f);
TestGetRandomReal<double>(0.0, 1.0);

for (AZ::s8 i = 0; i < 127; ++i)
{
TestGetRandomIntegral<AZ::s8>(-i, i);
TestGetRandomReal<double>(-1.0 * i, 1.0 * i);
}
}
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST_F(ScriptCanvasTestFixture, StringOperations)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas; 

    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();
    AZ::EntityId startNodeID{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNodeID, graphUniqueId, Nodes::Core::Start::TYPEINFO_Uuid());

    AZ::Uuid typeID = azrtti_typeid<AZStd::string>();
    const AZStd::string string0("abcd");
    AZ::EntityId string0NodeID;
    auto string0NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string0NodeID);
    string0NodePtr->SetInput_UNIT_TEST("Set", string0);

    // String 1
    const AZStd::string string1("ef");
    AZ::EntityId string1NodeID;
    auto string1NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string1NodeID);
    string1NodePtr->SetInput_UNIT_TEST("Set", string1);

    // String 2
    const AZStd::string string2("   abcd");
    AZ::EntityId string2NodeID;
    auto string2NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string2NodeID);
    string2NodePtr->SetInput_UNIT_TEST("Set", string2);

    // String 3
    const AZStd::string string3("abcd   ");
    AZ::EntityId string3NodeID;
    auto string3NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string3NodeID);
    string3NodePtr->SetInput_UNIT_TEST("Set", string3);

    // String 4
    const AZStd::string string4("abcd/ef/ghi");
    AZ::EntityId string4NodeID;
    auto string4NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string4NodeID);
    string4NodePtr->SetInput_UNIT_TEST("Set", string4);

    // String 5
    const AZStd::string string5("ab");
    AZ::EntityId string5NodeID;
    auto string5NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string5NodeID);
    string5NodePtr->SetInput_UNIT_TEST("Set", string5);

    // String 6
    const AZStd::string string6("ABCD");
    AZ::EntityId string6NodeID;
    auto string6NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string6NodeID);
    string6NodePtr->SetInput_UNIT_TEST("Set", string6);

    // String 7
    const AZStd::string string7("/");
    AZ::EntityId string7NodeID;
    auto string7NodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, string7NodeID);
    string7NodePtr->SetInput_UNIT_TEST("Set", string7);

    //Int 0
    AZ::EntityId int0NodeID;
    auto int0NodePtr = CreateTestNode<ScriptCanvas::Nodes::Math::Number>(graphUniqueId, int0NodeID);
    int0NodePtr->SetInput_UNIT_TEST("Set", 0);

    // Int 1
    AZ::EntityId int1NodeID;
    auto int1NodePtr = CreateTestNode<ScriptCanvas::Nodes::Math::Number>(graphUniqueId, int1NodeID);
    int1NodePtr->SetInput_UNIT_TEST("Set", 2);

    // Vector 0
    const AZStd::vector<AZStd::string> vector0({ AZStd::string("abcd"), AZStd::string("ef") });
    AZ::EntityId vector0NodeID;
    auto vector0NodePtr = CreateTestObjectNode(graphUniqueId, vector0NodeID, azrtti_typeid<AZStd::vector<AZStd::string>>());
    vector0NodePtr->SetInput_UNIT_TEST("Set", vector0);

    // LengthResult
    AZ::EntityId lengthResultNodeID;
    auto lengthResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Math::Number>(graphUniqueId, lengthResultNodeID);

    // FindResult
    AZ::EntityId findResultNodeID;
    auto findResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Math::Number>(graphUniqueId, findResultNodeID);

    // SubstringResult
    AZ::EntityId substringResultNodeID;
    auto* substringResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, substringResultNodeID);

    // ReplaceResult
    AZ::EntityId replaceResultNodeID;
    auto* replaceResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, replaceResultNodeID);

    // ReplaceByIndexResult
    AZ::EntityId replaceByIndexResultNodeID;
    auto* replaceByIndexResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, replaceByIndexResultNodeID);

    // AddResult
    AZ::EntityId addResultNodeID;
    auto* addResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, addResultNodeID);

    // TrimLeftResult
    AZ::EntityId trimLeftResultNodeID;
    auto* trimLeftResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, trimLeftResultNodeID);

    // TrimRightResult
    AZ::EntityId trimRightResultNodeID;
    auto* trimRightResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, trimRightResultNodeID);

    // ToLowerResult
    AZ::EntityId toLowerResultNodeID;
    auto* toLowerResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, toLowerResultNodeID);

    // ToUpperResult
    AZ::EntityId toUpperResultNodeID;
    auto* toUpperResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, toUpperResultNodeID);

    // JoinResult
    AZ::EntityId joinResultNodeID;
    auto* joinResultNodePtr = CreateTestNode<ScriptCanvas::Nodes::Core::String>(graphUniqueId, joinResultNodeID);

    // SplitResult
    AZ::EntityId splitResultNodeID;
    auto* splitResultNodePtr = CreateTestObjectNode(graphUniqueId, splitResultNodeID, azrtti_typeid<AZStd::vector<AZStd::string>>());

    const AZStd::string stringClassName("AZStd::basic_string<char, AZStd::char_traits<char>, allocator>");

    // Length
    AZ::EntityId lengthNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Length"));

    //Find
    AZ::EntityId findNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Find"));

    //Substring
    AZ::EntityId substringNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Substring"));

    //Replace
    AZ::EntityId replaceNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Replace"));

    //ReplaceByIndex
    AZ::EntityId replaceByIndexNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("ReplaceByIndex"));

    //Add
    AZ::EntityId addNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Add"));

    //TrimLeft
    AZ::EntityId trimLeftNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("TrimLeft"));

    //TrimRight
    AZ::EntityId trimRightNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("TrimRight"));

    //ToLower
    AZ::EntityId toLowerNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("ToLower"));

    //ToUpper
    AZ::EntityId toUpperNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("ToUpper"));

    //Join
    AZ::EntityId joinNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Join"));

    //Split
    AZ::EntityId splitNodeID = CreateClassFunctionNode(graphUniqueId, stringClassName, AZStd::string("Split"));

    // Flow
    ScriptCanvas::SlotId firstSlotId;
    ScriptCanvas::SlotId secondSlotId;

    NodeRequestBus::EventResult(firstSlotId, startNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, lengthNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(startNodeID, firstSlotId, lengthNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, lengthNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, findNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(lengthNodeID, firstSlotId, findNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, substringNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, substringNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, addNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, addNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, addNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, trimLeftNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(addNodeID, firstSlotId, trimLeftNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, trimLeftNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, trimRightNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimLeftNodeID, firstSlotId, trimRightNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, trimRightNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, toLowerNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimRightNodeID, firstSlotId, toLowerNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, toLowerNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, toUpperNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toLowerNodeID, firstSlotId, toUpperNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, toUpperNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, joinNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toUpperNodeID, firstSlotId, joinNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, joinNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, splitNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(joinNodeID, firstSlotId, splitNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, splitNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, replaceNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(splitNodeID, firstSlotId, replaceNodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "Out");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "In");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, replaceByIndexNodeID, secondSlotId));

    // Length
    NodeRequestBus::EventResult(firstSlotId, lengthNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(lengthNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, lengthNodeID, &NodeRequests::GetSlotId, "Result: Number");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, lengthResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(lengthNodeID, firstSlotId, lengthResultNodeID, secondSlotId));

    // Find
    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string5NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, string5NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "Number: 2");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, int0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, findNodeID, &NodeRequests::GetSlotId, "Result: Number");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, findResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(findNodeID, firstSlotId, findResultNodeID, secondSlotId));

    // Substring
    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "Number: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, int0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "Number: 2");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int1NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, int1NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, substringNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, substringResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(substringNodeID, firstSlotId, substringResultNodeID, secondSlotId));

    // Replace
    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string5NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, string5NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "String: 2");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string1NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, string1NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, replaceResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceNodeID, firstSlotId, replaceResultNodeID, secondSlotId));

    //ReplaceByIndex
    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "Number: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, int0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "Number: 2");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, int1NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, int1NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "String: 3");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string5NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, string5NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, replaceByIndexNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, replaceByIndexResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(replaceByIndexNodeID, firstSlotId, replaceByIndexResultNodeID, secondSlotId));

    // Add
    NodeRequestBus::EventResult(firstSlotId, addNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(addNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, addNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string1NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(addNodeID, firstSlotId, string1NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, addNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, addResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(addNodeID, firstSlotId, addResultNodeID, secondSlotId));

    //TrimLeft
    NodeRequestBus::EventResult(firstSlotId, trimLeftNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string2NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimLeftNodeID, firstSlotId, string2NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, trimLeftNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, trimLeftResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimLeftNodeID, firstSlotId, trimLeftResultNodeID, secondSlotId));

    // TrimRight
    NodeRequestBus::EventResult(firstSlotId, trimRightNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string3NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimRightNodeID, firstSlotId, string3NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, trimRightNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, trimRightResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(trimRightNodeID, firstSlotId, trimRightResultNodeID, secondSlotId));

    // ToLower
    NodeRequestBus::EventResult(firstSlotId, toLowerNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string6NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toLowerNodeID, firstSlotId, string6NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, toLowerNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, toLowerResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toLowerNodeID, firstSlotId, toLowerResultNodeID, secondSlotId));

    // ToUpper
    NodeRequestBus::EventResult(firstSlotId, toUpperNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toUpperNodeID, firstSlotId, string0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, toUpperNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, toUpperResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(toUpperNodeID, firstSlotId, toUpperResultNodeID, secondSlotId));

    // Join
    NodeRequestBus::EventResult(firstSlotId, joinNodeID, &NodeRequests::GetSlotId, "AZStd::vector<AZStd::basic_string<char, AZStd::char_traits<char>, allocator>, allocator>: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, vector0NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(joinNodeID, firstSlotId, vector0NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, joinNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string7NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(joinNodeID, firstSlotId, string7NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, joinNodeID, &NodeRequests::GetSlotId, "Result: String");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, joinResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(joinNodeID, firstSlotId, joinResultNodeID, secondSlotId));

    // Split
    NodeRequestBus::EventResult(firstSlotId, splitNodeID, &NodeRequests::GetSlotId, "String: 0");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string4NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(splitNodeID, firstSlotId, string4NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, splitNodeID, &NodeRequests::GetSlotId, "String: 1");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, string7NodeID, &NodeRequests::GetSlotId, "Get");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(splitNodeID, firstSlotId, string7NodeID, secondSlotId));

    NodeRequestBus::EventResult(firstSlotId, splitNodeID, &NodeRequests::GetSlotId, "Result: AZStd::vector<AZStd::basic_string<char, AZStd::char_traits<char>, allocator>, allocator>");
    EXPECT_TRUE(firstSlotId.IsValid());
    NodeRequestBus::EventResult(secondSlotId, splitResultNodeID, &NodeRequests::GetSlotId, "Set");
    EXPECT_TRUE(secondSlotId.IsValid());
    EXPECT_TRUE(graph->Connect(splitNodeID, firstSlotId, splitResultNodeID, secondSlotId));

    graph->SetStartNode(startNodeID);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    if (lengthResultNodePtr)
    {
        if (auto result = lengthResultNodePtr->GetInput_UNIT_TEST<int>("Set"))
        {
            EXPECT_EQ(4, *result);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (findResultNodePtr)
    {
        if (auto result = findResultNodePtr->GetInput_UNIT_TEST<int>("Set"))
        {
            EXPECT_EQ(0, *result);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (substringResultNodePtr)
    {
        auto resultValue = substringResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("ab", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (replaceResultNodePtr)
    {
        auto resultValue = replaceResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("efcd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (replaceByIndexResultNodePtr)
    {
        auto resultValue = replaceByIndexResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (addResultNodePtr)
    {
        auto resultValue = addResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcdef", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (trimLeftResultNodePtr)
    {
        auto resultValue = trimLeftResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (trimRightResultNodePtr)
    {
        auto resultValue = trimRightResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (toLowerResultNodePtr)
    {
        auto resultValue = toLowerResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcd", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (toUpperResultNodePtr)
    {
        auto resultValue = toUpperResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("ABCD", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (joinResultNodePtr)
    {
        auto resultValue = joinResultNodePtr->GetInput_UNIT_TEST<Data::StringType>("Set");
        if (resultValue)
        {
            EXPECT_EQ("abcd/ef", *resultValue);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (splitResultNodePtr)
    {
        auto result = splitResultNodePtr->GetInput_UNIT_TEST<AZStd::vector<AZStd::string>>("Set");
        if (result)
        {
            AZStd::vector<AZStd::string> ans = { "abcd", "ef", "ghi" };
            EXPECT_EQ(ans, *result);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    graph->GetEntity()->Deactivate();
    delete graph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST_F(ScriptCanvasTestFixture, SimplestNotTrue)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;

    const bool s_value = true;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Nodes
    AZ::EntityId startNodeId;
    Nodes::Core::Start* startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);
    ASSERT_TRUE(startNode != nullptr);

    AZ::EntityId booleanNodeId;
    Nodes::Logic::Boolean* booleanNode = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, booleanNodeId);
    ASSERT_TRUE(booleanNode != nullptr);
    booleanNode->SetInput_UNIT_TEST("Set", s_value);

    AZ::EntityId printNodeId;
    Print* printNode = CreateTestNode<Print>(graphUniqueId, printNodeId);
    ASSERT_TRUE(printNode != nullptr);

    AZ::EntityId notNodeId;
    Nodes::Logic::Not* notNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNodeId);
    ASSERT_TRUE(notNode != nullptr);

    AZ::EntityId notNotNodeId;
    Nodes::Logic::Not* notNotNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNotNodeId);
    ASSERT_TRUE(notNotNode != nullptr);

    AZ::EntityId printNode2Id;
    Print* printNode2 = CreateTestNode<Print>(graphUniqueId, printNode2Id);
    ASSERT_TRUE(printNode2 != nullptr);

    // Start  -> Not (false) => Print (true) -> Not (true) => Print
    //          /    \_________________________/
    // Boolean /
    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", notNodeId, Nodes::UnaryOperator::k_evaluateName));

    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_valueName, booleanNodeId, PureData::k_getThis));

    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onTrue, printNodeId, "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onFalse, printNodeId, "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_resultName, printNodeId, "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeId, "Out", notNotNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_valueName, notNodeId, Nodes::UnaryOperator::k_resultName));

    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onTrue, printNode2Id, "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onFalse, printNode2Id, "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_resultName, printNode2Id, "Value"));

    graph->SetStartNode(startNodeId);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // Validate results
    {
        const bool* result = printNode->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, !s_value);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    {
        const bool* result = printNode2->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, s_value);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    graph->GetEntity()->Deactivate();

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, SimplestNotFalse)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;

    const bool s_value = false;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Nodes
    AZ::EntityId startNodeId;
    Nodes::Core::Start* startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);
    ASSERT_TRUE(startNode != nullptr);

    AZ::EntityId booleanNodeId;
    Nodes::Logic::Boolean* booleanNode = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, booleanNodeId);
    ASSERT_TRUE(booleanNode != nullptr);
    booleanNode->SetInput_UNIT_TEST("Set", s_value);

    AZ::EntityId printNodeId;
    Print* printNode = CreateTestNode<Print>(graphUniqueId, printNodeId);
    ASSERT_TRUE(printNode != nullptr);

    AZ::EntityId notNodeId;
    Nodes::Logic::Not* notNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNodeId);
    ASSERT_TRUE(notNode != nullptr);

    AZ::EntityId notNotNodeId;
    Nodes::Logic::Not* notNotNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNotNodeId);
    ASSERT_TRUE(notNotNode != nullptr);

    AZ::EntityId printNode2Id;
    Print* printNode2 = CreateTestNode<Print>(graphUniqueId, printNode2Id);
    ASSERT_TRUE(printNode2 != nullptr);

    // Start  -> Not (false) => Print (true) -> Not (true) => Print
    //          /    \_________________________/
    // Boolean /
    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", notNodeId, Nodes::UnaryOperator::k_evaluateName));

    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_valueName, booleanNodeId, PureData::k_getThis));

    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onTrue, printNodeId, "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onFalse, printNodeId, "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_resultName, printNodeId, "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeId, "Out", notNotNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_valueName, notNodeId, Nodes::UnaryOperator::k_resultName));

    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onTrue, printNode2Id, "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onFalse, printNode2Id, "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_resultName, printNode2Id, "Value"));

    graph->SetStartNode(startNodeId);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // Validate results
    {
        const bool* result = printNode->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, !s_value);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    {
        const bool* result = printNode2->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, s_value);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    graph->GetEntity()->Deactivate();

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, LogicTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Nodes
    AZ::EntityId startNodeId;
    Nodes::Core::Start* startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);

    enum BooleanNodes
    {
        False,
        True
    };

    AZ::EntityId booleanNodeIds[2];
    Nodes::Logic::Boolean* booleanNodes[2];

    booleanNodes[BooleanNodes::False] = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, booleanNodeIds[BooleanNodes::False]);
    ASSERT_TRUE(booleanNodes[BooleanNodes::False] != nullptr);
    booleanNodes[BooleanNodes::False]->SetInput_UNIT_TEST("Set", false);
    ASSERT_FALSE(*booleanNodes[BooleanNodes::False]->GetInput_UNIT_TEST<bool>("Set"));

    booleanNodes[BooleanNodes::True] = CreateTestNode<Nodes::Logic::Boolean>(graphUniqueId, booleanNodeIds[BooleanNodes::True]);
    ASSERT_TRUE(booleanNodes[BooleanNodes::True] != nullptr);
    booleanNodes[BooleanNodes::True]->SetInput_UNIT_TEST("Set", true);
    ASSERT_TRUE(*booleanNodes[BooleanNodes::True]->GetInput_UNIT_TEST<bool>("Set"));
    
    enum PrintNodes
    {
        Or,
        And,
        Not,
        NotNot,
        Count
    };

    AZ::EntityId printNodeIds[PrintNodes::Count];
    Print* printNodes[PrintNodes::Count];

    printNodes[PrintNodes::Or] = CreateTestNode<Print>(graphUniqueId, printNodeIds[Or]);
    ASSERT_TRUE(printNodes[PrintNodes::Or] != nullptr);

    printNodes[PrintNodes::And] = CreateTestNode<Print>(graphUniqueId, printNodeIds[And]);
    ASSERT_TRUE(printNodes[PrintNodes::And] != nullptr);

    printNodes[PrintNodes::Not] = CreateTestNode<Print>(graphUniqueId, printNodeIds[Not]);
    ASSERT_TRUE(printNodes[PrintNodes::Not] != nullptr);

    printNodes[PrintNodes::NotNot] = CreateTestNode<Print>(graphUniqueId, printNodeIds[NotNot]);
    ASSERT_TRUE(printNodes[PrintNodes::NotNot] != nullptr);

    AZ::EntityId orNodeId;
    Nodes::Logic::Or* orNode = CreateTestNode<Nodes::Logic::Or>(graphUniqueId, orNodeId);

    AZ::EntityId andNodeId;
    Nodes::Logic::And* andNode = CreateTestNode<Nodes::Logic::And>(graphUniqueId, andNodeId);

    AZ::EntityId notNodeId;
    Nodes::Logic::Not* notNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNodeId);

    AZ::EntityId notNotNodeId;
    Nodes::Logic::Not* notNotNode = CreateTestNode<Nodes::Logic::Not>(graphUniqueId, notNotNodeId);
    
    // Start ---------------->  And --> Print -->  Or -> Print -> Not -> Print -> NotNot -> Print
    // Boolean (true) _________/_/_____true______/_ /___ true ____/  \__ false ___/    \____true
    // Boolean (false) ____________________________/
    
    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", andNodeId, "In"));
    
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_onTrue, printNodeIds[And], "In"));
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_onFalse, printNodeIds[And], "In"));
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_lhsName, booleanNodeIds[True], PureData::k_getThis));
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_rhsName, booleanNodeIds[True], PureData::k_getThis));
    EXPECT_TRUE(Connect(*graph, andNodeId, Nodes::BinaryOperator::k_resultName, printNodeIds[And], "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeIds[And], "Out", orNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_onTrue, printNodeIds[Or], "In"));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_onFalse, printNodeIds[Or], "In"));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_lhsName, booleanNodeIds[True], PureData::k_getThis));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_rhsName, booleanNodeIds[False], PureData::k_getThis));
    EXPECT_TRUE(Connect(*graph, orNodeId, Nodes::BinaryOperator::k_resultName, printNodeIds[Or], "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeIds[Or], "Out", notNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onTrue, printNodeIds[Not], "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_onFalse, printNodeIds[Not], "In"));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_valueName, orNodeId, Nodes::BinaryOperator::k_resultName));
    EXPECT_TRUE(Connect(*graph, notNodeId, Nodes::UnaryOperator::k_resultName, printNodeIds[Not], "Value"));

    EXPECT_TRUE(Connect(*graph, printNodeIds[Not], "Out", notNotNodeId, Nodes::UnaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onTrue, printNodeIds[NotNot], "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_onFalse, printNodeIds[NotNot], "In"));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_valueName, notNotNodeId, Nodes::BinaryOperator::k_resultName));
    EXPECT_TRUE(Connect(*graph, notNotNodeId, Nodes::UnaryOperator::k_resultName, printNodeIds[NotNot], "Value"));
    
    graph->SetStartNode(startNodeId);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // Validate results

    if (printNodes[And])
    {
        const bool* result = printNodes[And]->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, true);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (printNodes[Or])
    {
        const bool* result = printNodes[And]->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, true);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (printNodes[Not])
    {
        const bool* result = printNodes[Not]->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, false);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    if (printNodes[NotNot])
    {
        const bool* result = printNodes[NotNot]->GetInput_UNIT_TEST<bool>("Value");
        if (result)
        {
            EXPECT_EQ(*result, true);
        }
        else
        {
            ADD_FAILURE();
        }
    }

    graph->GetEntity()->Deactivate();

    delete graph->GetEntity();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    bool GraphHasVectorWithValue(const Graph& graph, const AZ::Vector3& vector)
    {
         for (auto nodeId : graph.GetNodes())
         {
             AZ::Entity* entity{};
             AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
 
             if (entity)
             {
                 if (auto node = AZ::EntityUtils::FindFirstDerivedComponent<Core::BehaviorContextObjectNode>(entity))
                 {
                     if (auto candidate = node->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
                     {
                         if (*candidate == vector)
                         {
                             return true;
                         }
                     }
                 }
             }
         }
 
         return false;
    }
}

TEST_F(ScriptCanvasTestFixture, SerializationSaveTest_Binary)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
    //////////////////////////////////////////////////////////////////////////

    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();
    AZ::EntityId startNode{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNode, graphUniqueId, Nodes::Core::Start::TYPEINFO_Uuid());

    // Constant 0
    AZ::Entity* constant0Entity{ aznew AZ::Entity };
    constant0Entity->Init();
    AZ::EntityId constant0Node{ constant0Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, constant0Node, graphUniqueId, Nodes::Math::Number::TYPEINFO_Uuid());

    // Get the node and set the value
    Nodes::Math::Number* constant0NodePtr = nullptr;
    SystemRequestBus::BroadcastResult(constant0NodePtr, &SystemRequests::GetNode<Nodes::Math::Number>, constant0Node);
    EXPECT_TRUE(constant0NodePtr != nullptr);
    if (constant0NodePtr)
    {
        constant0NodePtr->SetInput_UNIT_TEST("Set", 4);
    }

    // Constant 1
    AZ::Entity* constant1Entity{ aznew AZ::Entity };
    constant1Entity->Init();
    AZ::EntityId constant1Node{ constant1Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, constant1Node, graphUniqueId, Nodes::Math::Number::TYPEINFO_Uuid());
    // Get the node and set the value
    Nodes::Math::Number* constant1NodePtr = nullptr;
    SystemRequestBus::BroadcastResult(constant1NodePtr, &SystemRequests::GetNode<Nodes::Math::Number>, constant1Node);
    EXPECT_TRUE(constant1NodePtr != nullptr);
    if (constant1NodePtr)
    {
        constant1NodePtr->SetInput_UNIT_TEST("Set", 12);
    }

    // Sum
    AZ::Entity* sumEntity{ aznew AZ::Entity };
    sumEntity->Init();
    AZ::EntityId sumNode{ sumEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, sumNode, graphUniqueId, Nodes::Math::Sum::TYPEINFO_Uuid());

    AZ::EntityId printID;
    auto printNode = CreateTestNode<Print>(graphUniqueId, printID);

    // Connect, disconnect and reconnect to validate disconnection
    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId("Arg1"), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    graph->Disconnect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId("Arg1"), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId("Arg1"), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    {
        auto* sumNodePtr = AZ::EntityUtils::FindFirstDerivedComponent<Nodes::Math::Sum>(sumEntity);
        EXPECT_NE(nullptr, sumNodePtr);
        EXPECT_EQ(1, graph->GetConnectedEndpoints({ sumNode, sumNodePtr->GetSlotId("Arg1") }).size());
    }

    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId("Arg2"), constant1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant1Entity)->GetSlotId("Get"));

    EXPECT_TRUE(Connect(*graph, sumNode, "Result", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, startNode, "Out", sumNode, Nodes::BinaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, sumNode, Nodes::BinaryOperator::k_outName, printID, "In"));

    AZ::EntityId vector3IdA, vector3IdB, vector3IdC;
    const AZ::Vector3 allOne(1,1,1);
    const AZ::Vector3 allTwo(2,2,2);
    const AZ::Vector3 allThree(3,3,3);
    const AZ::Vector3 allFour(4,4,4);

    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeA->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne;
    Core::BehaviorContextObjectNode* vector3NodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdB);
    vector3NodeB->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeB->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allTwo;
    Core::BehaviorContextObjectNode* vector3NodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdC);
    vector3NodeC->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeC->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allFour;
    
    AZ::EntityId add3IdA = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");
    
    EXPECT_TRUE(Connect(*graph, printID, "Out", add3IdA, "In"));
    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", add3IdA, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", add3IdA, "Vector3: 1"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "Set", add3IdA, "Result: Vector3"));

    graph->SetStartNode(startNode);

    AZStd::unique_ptr<AZ::Entity> graphEntity{ graph->GetEntity() };

    EXPECT_EQ(allOne, *vector3NodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set"));
    EXPECT_EQ(allTwo, *vector3NodeB->GetInput_UNIT_TEST<AZ::Vector3>("Set"));
    EXPECT_EQ(allFour, *vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set"));
    
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
    EXPECT_FALSE(GraphHasVectorWithValue(*graph, allThree));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allFour));
    
    auto nodeIDs = graph->GetNodes();
    EXPECT_EQ(9, nodeIDs.size());

    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    auto vector3C = *vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    EXPECT_EQ(vector3C, allThree);
    
    if (auto result = printNode->GetInput_UNIT_TEST<float>("Value"))
    {
        EXPECT_FLOAT_EQ(*result, 16.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allThree));
    EXPECT_FALSE(GraphHasVectorWithValue(*graph, allFour));

    
    graphEntity->Deactivate();

    auto* fileIO = AZ::IO::LocalFileIO::GetInstance();
    EXPECT_TRUE(fileIO != nullptr);

    // Save it
    bool result = false;
    AZ::IO::FileIOStream outFileStream("serializationTest.scriptCanvas", AZ::IO::OpenMode::ModeWrite);
    if (outFileStream.IsOpen())
    {
        GraphAssetHandler graphAssetHandler;
        AZ::Data::Asset<GraphAsset> graphAsset;
        graphAsset.Create(AZ::Uuid::CreateRandom());
        graphAsset.Get()->SetGraph(graphEntity.get());
        EXPECT_TRUE(graphAssetHandler.SaveAssetData(graphAsset, &outFileStream));
    }

    outFileStream.Close();
}

TEST_F(ScriptCanvasTestFixture, SerializationLoadTest_Binary)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    //////////////////////////////////////////////////////////////////////////

    // Make the graph.
    Graph* graph = nullptr;

    AZStd::unique_ptr<AZ::Entity> graphEntity;

    auto* fileIO = AZ::IO::LocalFileIO::GetInstance();
    EXPECT_NE(nullptr, fileIO);
    EXPECT_TRUE(fileIO->Exists("serializationTest.ScriptCanvas"));

    // Load it
    AZ::IO::FileIOStream inFileStream("serializationTest.scriptCanvas", AZ::IO::OpenMode::ModeRead);
    if (inFileStream.IsOpen())
    {
        // Serialize the graph entity back in.
        GraphAssetHandler graphAssetHandler;
        AZ::Data::Asset<GraphAsset> graphAsset;
        graphAsset.Create(AZ::Uuid::CreateRandom());
        EXPECT_TRUE(graphAssetHandler.LoadAssetData(graphAsset, &inFileStream, {}));
        graphEntity.reset(graphAsset.Get()->GetGraph());

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> idRemap;
        AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(graphEntity.get(), idRemap, m_serializeContext);
        EXPECT_TRUE(graphEntity != nullptr);

        // Close the file
        inFileStream.Close();

        // Initialize the entity
        graphEntity->Init();
        graphEntity->Activate();
        
        // Run the graph component
        graph = graphEntity->FindComponent<Graph>();
        EXPECT_TRUE(graph != nullptr); // "Graph entity did not have the graph component.");
        EXPECT_FALSE(graph->IsInErrorState());

        const AZ::Vector3 allOne(1, 1, 1);
        const AZ::Vector3 allTwo(2, 2, 2);
        const AZ::Vector3 allThree(3, 3, 3);
        const AZ::Vector3 allFour(4, 4, 4);
        
        auto nodeIDs = graph->GetNodes();
        EXPECT_EQ(9, nodeIDs.size());

        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allThree));
        EXPECT_FALSE(GraphHasVectorWithValue(*graph, allFour));

        // Graph result should be still 16
        // Serialized graph has new remapped entity ids
        auto nodes = graph->GetNodes();
        auto sumNodeIt = AZStd::find_if(nodes.begin(), nodes.end(), [](const AZ::EntityId& nodeId) -> bool
        {
            AZ::Entity* entity{};
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
            return entity ? AZ::EntityUtils::FindFirstDerivedComponent<Print>(entity) ? true : false : false;
        });
        EXPECT_NE(nodes.end(), sumNodeIt);
                
        Print* printNode = nullptr;
        SystemRequestBus::BroadcastResult(printNode, &SystemRequests::GetNode<Print>, *sumNodeIt);
        EXPECT_TRUE(printNode != nullptr);

        if (auto result = printNode->GetInput_UNIT_TEST<float>("Value"))
        {
            EXPECT_FLOAT_EQ(*result, 16.0f);
        }
        else
        {
            ADD_FAILURE();
        }

        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allThree));
        EXPECT_FALSE(GraphHasVectorWithValue(*graph, allFour));
    }
}

TEST_F(ScriptCanvasTestFixture, Contracts)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    ContractNode::Reflect(m_serializeContext);
    ContractNode::Reflect(m_behaviorContext);
    
    //////////////////////////////////////////////////////////////////////////
    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity("Start") };
    startEntity->Init();
    AZ::EntityId startNode{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNode, graphUniqueId, Nodes::Core::Start::TYPEINFO_Uuid());

    // ContractNode 0
    AZ::Entity* contract0Entity{ aznew AZ::Entity("Contract 0") };
    contract0Entity->Init();
    AZ::EntityId contract0Node{ contract0Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, contract0Node, graphUniqueId, ContractNode::TYPEINFO_Uuid());

    // ContractNode 1
    AZ::Entity* contract1Entity{ aznew AZ::Entity("Contract 1") };
    contract1Entity->Init();
    AZ::EntityId contract1Node{ contract1Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, contract1Node, graphUniqueId, ContractNode::TYPEINFO_Uuid());

    // invalid
    EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Out")));
    EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));
    EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Get String")));
    EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Get String")));
    EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String")));
    EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String")));
    EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set String")));
    EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set Number")));
    EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get Number")));
    EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set String")));

    EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));

    // valid
    EXPECT_TRUE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get String")));
    EXPECT_TRUE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set Number"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get Number")));

    // Execute it
    graph->SetStartNode(startNode);
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

#if defined (SCRIPTCANVAS_ERRORS_ENABLED)

TEST_F(ScriptCanvasTestFixture, Error)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    ScriptUnitTestEventHandler::Reflect(m_serializeContext);
    ScriptUnitTestEventHandler::Reflect(m_behaviorContext);
    UnitTestErrorNode::Reflect(m_serializeContext);
    UnitTestErrorNode::Reflect(m_behaviorContext);
    InfiniteLoopNode::Reflect(m_serializeContext);
    InfiniteLoopNode::Reflect(m_behaviorContext);

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZStd::string description = "Unit test error!";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZ::EntityId errorNodeID;
    CreateTestNode<UnitTestErrorNode>(graphUniqueId, errorNodeID);

    AZ::EntityId sideEffectID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");
    graph->SetStartNode(startID);

    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, startID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, errorNodeID, "Out", sideEffectID, "In"));

    // test to make sure the graph received an error
    graph->GetEntity()->Activate();
    EXPECT_TRUE(graph->IsInErrorState());
    EXPECT_TRUE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(description, graph->GetLastErrorDescription());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 0);

    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, ErrorHandled)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZStd::string description = "Unit test error!";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZStd::string sideFX2 = "Side FX 2";
    AZ::EntityId sideFX2NodeID;
    CreateDataNode(graphUniqueId, sideFX2, sideFX2NodeID);

    AZ::EntityId errorNodeID;
    CreateTestNode<UnitTestErrorNode>(graphUniqueId, errorNodeID);

    AZ::EntityId sideEffectID1 = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");
    AZ::EntityId sideEffectID2 = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    AZ::EntityId errorHandlerID;
    CreateTestNode<Nodes::Core::ErrorHandler>(graphUniqueId, errorHandlerID);

    graph->SetStartNode(startID);

    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID1, "String: 0"));
    EXPECT_TRUE(Connect(*graph, sideFX2NodeID, "Get", sideEffectID2, "String: 0"));
    EXPECT_TRUE(Connect(*graph, startID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, errorNodeID, "Out", sideEffectID1, "In"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Source", errorNodeID, "This"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Out", sideEffectID2, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    EXPECT_FALSE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 0);
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX2), 1);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, ErrorNode)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    // Start
    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZStd::string description = "Test Error Report";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZ::EntityId errorNodeID;
    CreateTestNode<Nodes::Core::Error>(graphUniqueId, errorNodeID);

    AZ::EntityId sideEffectID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, startID, "Out", sideEffectID, "In"));
    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, sideEffectID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, stringNodeID, "Get", errorNodeID, "Description"));

    // Execute it
    graph->SetStartNode(startID);

    // test to make sure the graph received an error
    graph->GetEntity()->Activate();
    EXPECT_TRUE(graph->IsInErrorState());
    EXPECT_TRUE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(description, graph->GetLastErrorDescription());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 1);

    graph->GetEntity()->Deactivate();

    // test to make sure the graph did to retry execution
    graph->GetEntity()->Activate();
    EXPECT_TRUE(graph->IsInErrorState());
    EXPECT_TRUE(graph->IsInIrrecoverableErrorState());
    // if the graph was allowed to execute, the side effect counter should be higher
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 1);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, ErrorNodeHandled)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZStd::string description = "Test Error Report";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId errorNodeID;
    CreateTestNode<Nodes::Core::Error>(graphUniqueId, errorNodeID);
    AZ::EntityId errorHandlerID;
    CreateTestNode<Nodes::Core::ErrorHandler>(graphUniqueId, errorHandlerID);
    AZ::EntityId sideEffectID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, startID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Source", errorNodeID, "This"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Out", sideEffectID, "In"));
    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID, "String: 0"));

    graph->SetStartNode(startID);
    graph->GetEntity()->Activate();

    EXPECT_FALSE(graph->IsInErrorState());
    EXPECT_FALSE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 1);

    graph->GetEntity()->Deactivate();
    graph->GetEntity()->Activate();

    EXPECT_FALSE(graph->IsInErrorState());
    EXPECT_FALSE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 2);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, InfiniteLoopDetected)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZStd::string sideFXpass = "Side FX Infinite Loop Error Handled";
    AZ::EntityId sideFXpassNodeID;
    CreateDataNode(graphUniqueId, sideFXpass, sideFXpassNodeID);

    AZStd::string sideFXfail = "Side FX Infinite Loop Node Failed";
    AZ::EntityId sideFXfailNodeID;
    CreateDataNode(graphUniqueId, sideFXfail, sideFXfailNodeID);

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId infiniteLoopNodeID;
    CreateTestNode<InfiniteLoopNode>(graphUniqueId, infiniteLoopNodeID);
    AZ::EntityId errorHandlerID;
    CreateTestNode<Nodes::Core::ErrorHandler>(graphUniqueId, errorHandlerID);
    
    AZ::EntityId sideEffectPassID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");
    AZ::EntityId sideEffectFailID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, sideFXpassNodeID, "Get", sideEffectPassID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, sideFXfailNodeID, "Get", sideEffectFailID, "String: 0"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", infiniteLoopNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, infiniteLoopNodeID, "In", infiniteLoopNodeID, "Before Infinity"));
       
    EXPECT_TRUE(Connect(*graph, infiniteLoopNodeID, "After Infinity", sideEffectFailID, "In"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Out", sideEffectPassID, "In"));
    
    graph->SetStartNode(startID);
    graph->GetEntity()->Activate();

    EXPECT_FALSE(graph->IsInErrorState());
    EXPECT_FALSE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(1, unitTestHandler.SideEffectCount(sideFXpass));
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXfail));

    graph->GetEntity()->Deactivate();
    graph->GetEntity()->Activate();

    EXPECT_FALSE(graph->IsInErrorState());
    EXPECT_FALSE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(2, unitTestHandler.SideEffectCount(sideFXpass));
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXfail));

    delete graph->GetEntity();
}

#endif // SCRIPTCANAS_ERRORS_ENABLED

TEST_F(ScriptCanvasTestFixture, BehaviorContext_BusHandler)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId eventHandlerID;
    Nodes::Core::EBusEventHandler* eventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, eventHandlerID);
    eventHandler->InitializeBus("EventTestHandler");

    // number event
    AZ::EntityId numberHandlerID;
    Nodes::Core::EBusEventHandler* numberHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, numberHandlerID);
    numberHandler->InitializeBus("EventTestHandler");

    int threeInt(3);
    AZ::EntityId threeID;
    CreateDataNode(graphUniqueId, threeInt, threeID);
    AZ::EntityId sumID;
    CreateTestNode<Nodes::Math::Sum>(graphUniqueId, sumID);

    // string event
    AZ::EntityId stringHandlerID;
    Nodes::Core::EBusEventHandler* stringHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, stringHandlerID);
    stringHandler->InitializeBus("EventTestHandler");

    AZ::EntityId stringID;
    Nodes::Core::String* string = CreateTestNode<Nodes::Core::String>(graphUniqueId, stringID);
    AZStd::string goodBye("good bye!");
    string->SetInput_UNIT_TEST("Set", goodBye);

    // vector event (objects)
    AZ::EntityId vectorHandlerID;
    Nodes::Core::EBusEventHandler* vectorHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, vectorHandlerID);
    vectorHandler->InitializeBus("EventTestHandler");

    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "Vector3", "Normalize");
    AZ::EntityId vectorID;
    Nodes::Core::BehaviorContextObjectNode* vector = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, vectorID);
    const AZStd::string vector3("Vector3");
    vector->InitializeObject(vector3);

    // print (for strings, and generic event)
    AZ::EntityId printID;
    Print* print = CreateTestNode<Print>(graphUniqueId, printID);
    print->SetText("print was un-initialized");

    AZ::EntityId print2ID;
    Print* print2 = CreateTestNode<Print>(graphUniqueId, print2ID);
    print2->SetText("print was un-initialized");

    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Handle:Event", printID, "In"));

    EXPECT_TRUE(Connect(*graph, stringHandlerID, "Handle:String", printID, "In"));
    EXPECT_TRUE(Connect(*graph, stringHandlerID, "String", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, stringHandlerID, "String", stringID, "Set"));
    EXPECT_TRUE(Connect(*graph, stringHandlerID, "Result: String", stringID, "Get"));

    EXPECT_TRUE(Connect(*graph, numberHandlerID, "Handle:Number", sumID, Nodes::BinaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, sumID, Nodes::BinaryOperator::k_outName, printID, "In"));
    EXPECT_TRUE(Connect(*graph, numberHandlerID, "Number", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, numberHandlerID, "Number", sumID, Nodes::BinaryOperator::k_lhsName));
    EXPECT_TRUE(Connect(*graph, numberHandlerID, "Result: Number", sumID, Nodes::BinaryOperator::k_resultName));
    EXPECT_TRUE(Connect(*graph, threeID, "Get", sumID, Nodes::BinaryOperator::k_rhsName));

    EXPECT_TRUE(Connect(*graph, vectorHandlerID, "Handle:Vector", print2ID, "In"));
    EXPECT_TRUE(Connect(*graph, print2ID, "Out", normalizeID, "In"));

    EXPECT_TRUE(Connect(*graph, vectorHandlerID, "Vector3", vectorID, "Set"));
    EXPECT_TRUE(Connect(*graph, vectorHandlerID, "Result: Vector3", vectorID, "Get"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", normalizeID, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", print2ID, "Value"));

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // just don't crash
    EventTestBus::Event(graphEntityID, &EventTest::Event);
    EXPECT_FALSE(graph->IsInErrorState());

    // string
    AZ::EBusAggregateResults<AZStd::string> stringResults;
    AZStd::string hello("Hello!");
    EventTestBus::EventResult(stringResults, graphEntityID, &EventTest::String, hello);
    auto iterString = AZStd::find(stringResults.values.begin(), stringResults.values.end(), hello);
    EXPECT_NE(iterString, stringResults.values.end());
    EXPECT_EQ(print->GetText(), hello);
    EXPECT_FALSE(graph->IsInErrorState());

    // number
    AZ::EBusAggregateResults<int> numberResults;
    int four(4);
    EventTestBus::EventResult(numberResults, graphEntityID, &EventTest::Number, four);
    int seven(7);
    auto interNumber = AZStd::find(numberResults.values.begin(), numberResults.values.end(), seven);
    EXPECT_NE(interNumber, numberResults.values.end());
    AZStd::string sevenString = AZStd::string("4.000000");
    EXPECT_EQ(print->GetText(), sevenString);
    EXPECT_FALSE(graph->IsInErrorState());

    // vector
    AZ::EBusAggregateResults<AZ::Vector3> vectorResults;
    AZ::Vector3 allSeven(7.0f, 7.0f, 7.0f);
    void* allSevenVoidPtr = reinterpret_cast<void*>(&allSeven);
    EventTestBus::EventResult(vectorResults, graphEntityID, &EventTest::Vector, allSeven);
    allSeven.NormalizeSafe();
    auto iterVector = AZStd::find(vectorResults.values.begin(), vectorResults.values.end(), allSeven);
    EXPECT_NE(iterVector, vectorResults.values.end());
    // \todo restore a similar test with a by reference/pointer version
    // EXPECT_EQ(iterVector, allSevenVoidPtr);
    AZStd::string vectorString = AZStd::string("(x=7.0000000,y=7.0000000,z=7.0000000)");
    EXPECT_EQ(print2->GetText(), vectorString);
    EXPECT_FALSE(graph->IsInErrorState());

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_BusHandler2)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId eventHandlerID;
    Nodes::Core::EBusEventHandler* eventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, eventHandlerID);
    eventHandler->InitializeBus("EventTestHandler");

    // number event
    AZ::EntityId numberHandlerID;
    Nodes::Core::EBusEventHandler* numberHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, numberHandlerID);
    numberHandler->InitializeBus("EventTestHandler");
    int threeInt(3);
    AZ::EntityId threeID;
    CreateDataNode(graphUniqueId, threeInt, threeID);
    AZ::EntityId sumID;
    CreateTestNode<Nodes::Math::Sum>(graphUniqueId, sumID);

    // string event
    AZ::EntityId stringHandlerID;
    Nodes::Core::EBusEventHandler* stringHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, stringHandlerID);
    stringHandler->InitializeBus("EventTestHandler");
    AZ::EntityId stringID;
    Nodes::Core::String* string = CreateTestNode<Nodes::Core::String>(graphUniqueId, stringID);
    AZStd::string goodBye("good bye!");
    string->SetInput_UNIT_TEST("Set", goodBye);

    // vector event (objects)
    AZ::EntityId vectorHandlerID;
    Nodes::Core::EBusEventHandler* vectorHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, vectorHandlerID);
    vectorHandler->InitializeBus("EventTestHandler");
    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "Vector3", "Normalize");
     AZ::EntityId vectorID;
     Nodes::Core::BehaviorContextObjectNode* vector = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, vectorID);
     const AZStd::string vector3("Vector3");
     vector->InitializeObject(vector3);

    // print (for strings, and generic event)
    AZ::EntityId printID;
    Print* print = CreateTestNode<Print>(graphUniqueId, printID);
    print->SetText("print was un-initialized");

    AZ::EntityId print2ID;
    Print* print2 = CreateTestNode<Print>(graphUniqueId, print2ID);
    print2->SetText("print was un-initialized");

    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Handle:Event", printID, "In"));

    EXPECT_TRUE(Connect(*graph, stringHandlerID, "Handle:String", printID, "In"));
    EXPECT_TRUE(Connect(*graph, stringHandlerID, "String", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, stringHandlerID, "String", stringID, "Set"));
    EXPECT_TRUE(Connect(*graph, stringHandlerID, "Result: String", stringID, "Get"));

    EXPECT_TRUE(Connect(*graph, numberHandlerID, "Handle:Number", sumID, Nodes::BinaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, sumID, Nodes::BinaryOperator::k_outName, printID, "In"));
    EXPECT_TRUE(Connect(*graph, numberHandlerID, "Number", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, numberHandlerID, "Number", sumID, Nodes::BinaryOperator::k_lhsName));
    EXPECT_TRUE(Connect(*graph, numberHandlerID, "Result: Number", sumID, Nodes::BinaryOperator::k_resultName));
    EXPECT_TRUE(Connect(*graph, threeID, "Get", sumID, Nodes::BinaryOperator::k_rhsName));

    EXPECT_TRUE(Connect(*graph, vectorHandlerID, "Handle:Vector", print2ID, "In"));
    EXPECT_TRUE(Connect(*graph, print2ID, "Out", normalizeID, "In"));
    
    EXPECT_TRUE(Connect(*graph, vectorHandlerID, "Vector3", vectorID, "Set"));
    EXPECT_TRUE(Connect(*graph, vectorHandlerID, "Result: Vector3", vectorID, "Get"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", normalizeID, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", print2ID, "Value"));

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // just don't crash
    EventTestBus::Event(graphEntityID, &EventTest::Event);
    EXPECT_FALSE(graph->IsInErrorState());

    // string
    AZ::EBusAggregateResults<AZStd::string> stringResults;
    AZStd::string hello("Hello!");
    EventTestBus::EventResult(stringResults, graphEntityID, &EventTest::String, hello);
    auto iterString = AZStd::find(stringResults.values.begin(), stringResults.values.end(), hello);
    EXPECT_NE(iterString, stringResults.values.end());
    EXPECT_EQ(print->GetText(), hello);
    EXPECT_FALSE(graph->IsInErrorState());

    // number
    AZ::EBusAggregateResults<int> numberResults;
    int four(4);
    EventTestBus::EventResult(numberResults, graphEntityID, &EventTest::Number, four);
    EXPECT_FALSE(graph->IsInErrorState());
    int seven(7);
    auto interNumber = AZStd::find(numberResults.values.begin(), numberResults.values.end(), seven);
    EXPECT_NE(interNumber, numberResults.values.end());
    AZStd::string sevenString = AZStd::string("4.000000");
    EXPECT_EQ(print->GetText(), sevenString);

    // vector
    AZ::EBusAggregateResults<AZ::Vector3> vectorResults;
    AZ::Vector3 allSeven(7.0f, 7.0f, 7.0f);
    void* allSevenVoidPtr = reinterpret_cast<void*>(&allSeven);
    EventTestBus::EventResult(vectorResults, graphEntityID, &EventTest::Vector, allSeven);
    EXPECT_FALSE(graph->IsInErrorState());
    allSeven.NormalizeSafe();
    auto iterVector = AZStd::find(vectorResults.values.begin(), vectorResults.values.end(), allSeven);
    EXPECT_NE(iterVector, vectorResults.values.end());
    // \todo restore a similar test with a by reference/pointer version
    // EXPECT_EQ(iterVector, allSevenVoidPtr);
    AZStd::string vectorString = AZStd::string("(x=7.0000000,y=7.0000000,z=7.0000000)");
    EXPECT_EQ(print2->GetText(), vectorString);

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
};

TEST_F(ScriptCanvasTestFixture, BehaviorContext_BusHandler3)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId eventHandlerID;
    Nodes::Core::EBusEventHandler* eventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, eventHandlerID);
    eventHandler->InitializeBus("EventTestHandler");

    // number event
    int threeInt(3);
    AZ::EntityId threeID;
    CreateDataNode(graphUniqueId, threeInt, threeID);
    AZ::EntityId sumID;
    CreateTestNode<Nodes::Math::Sum>(graphUniqueId, sumID);

    // string event
    AZ::EntityId stringID;
    Nodes::Core::String* string = CreateTestNode<Nodes::Core::String>(graphUniqueId, stringID);
    AZStd::string goodBye("good bye!");
    string->SetInput_UNIT_TEST("Set", goodBye);

    // vector event (objects)
    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "Vector3", "Normalize");
    AZ::EntityId vectorID;
    Nodes::Core::BehaviorContextObjectNode* vector = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, vectorID);
    const AZStd::string vector3("Vector3");
    vector->InitializeObject(vector3);

    // print (for strings, and generic event)
    AZ::EntityId printID;
    Print* print = CreateTestNode<Print>(graphUniqueId, printID);
    print->SetText("print was un-initialized");

    AZ::EntityId print2ID;
    Print* print2 = CreateTestNode<Print>(graphUniqueId, print2ID);
    print2->SetText("print was un-initialized");

    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Handle:Event", printID, "In"));

    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Handle:String", printID, "In"));
    EXPECT_TRUE(Connect(*graph, eventHandlerID, "String", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, eventHandlerID, "String", stringID, "Set"));
    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Result: String", stringID, "Get"));

    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Handle:Number", sumID, Nodes::BinaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, sumID, Nodes::BinaryOperator::k_outName, printID, "In"));
    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Number", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Number", sumID, Nodes::BinaryOperator::k_lhsName));
    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Result: Number", sumID, Nodes::BinaryOperator::k_resultName));
    EXPECT_TRUE(Connect(*graph, threeID, "Get", sumID, Nodes::BinaryOperator::k_rhsName));

    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Handle:Vector", print2ID, "In"));
    EXPECT_TRUE(Connect(*graph, print2ID, "Out", normalizeID, "In"));

    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Vector3", vectorID, "Set"));
    EXPECT_TRUE(Connect(*graph, eventHandlerID, "Result: Vector3", vectorID, "Get"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", normalizeID, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", print2ID, "Value"));

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    // just don't crash
    EventTestBus::Event(graphEntityID, &EventTest::Event);
    EXPECT_FALSE(graph->IsInErrorState());

    // string
    AZ::EBusAggregateResults<AZStd::string> stringResults;
    AZStd::string hello("Hello!");
    EventTestBus::EventResult(stringResults, graphEntityID, &EventTest::String, hello);
    EXPECT_FALSE(graph->IsInErrorState());
    auto iterString = AZStd::find(stringResults.values.begin(), stringResults.values.end(), hello);
    EXPECT_NE(iterString, stringResults.values.end());
    EXPECT_EQ(print->GetText(), hello);

    // number
    AZ::EBusAggregateResults<int> numberResults;
    int four(4);
    EventTestBus::EventResult(numberResults, graphEntityID, &EventTest::Number, four);
    EXPECT_FALSE(graph->IsInErrorState());
    int seven(7);
    auto interNumber = AZStd::find(numberResults.values.begin(), numberResults.values.end(), seven);
    EXPECT_NE(interNumber, numberResults.values.end());
    AZStd::string sevenString = AZStd::string("4.000000");
    EXPECT_EQ(print->GetText(), sevenString);

    // vector
    AZ::EBusAggregateResults<AZ::Vector3> vectorResults;
    AZ::Vector3 allSeven(7.0f, 7.0f, 7.0f);
    void* allSevenVoidPtr = reinterpret_cast<void*>(&allSeven);
    EventTestBus::EventResult(vectorResults, graphEntityID, &EventTest::Vector, allSeven);
    EXPECT_FALSE(graph->IsInErrorState());
    allSeven.NormalizeSafe();
    auto iterVector = AZStd::find(vectorResults.values.begin(), vectorResults.values.end(), allSeven);
    EXPECT_NE(iterVector, vectorResults.values.end());
    // \todo restore a similar test with a by reference/pointer version
    // EXPECT_EQ(iterVector, allSevenVoidPtr);
    AZStd::string vectorString = AZStd::string("(x=7.0000000,y=7.0000000,z=7.0000000)");
    EXPECT_EQ(print2->GetText(), vectorString);

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
};

TEST_F(ScriptCanvasTestFixture, BehaviorContext_BusHandlerNonEntityIdBusId)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    StringView::Reflect(m_serializeContext);
    StringView::Reflect(m_behaviorContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_behaviorContext);
    TemplateEventTestHandler<AZStd::string>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZStd::string>::Reflect(m_behaviorContext);

    auto graphEntity = aznew AZ::Entity;
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    Graph* graph = AZ::EntityUtils::FindFirstDerivedComponent<Graph>(graphEntity);
    EXPECT_NE(nullptr, graph);
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId uuidEventHandlerId;
    auto uuidEventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, uuidEventHandlerId);
    uuidEventHandler->InitializeBus("TemplateEventTestHandler<AZ::Uuid >");
    AZ::Uuid uuidBusId = AZ::Uuid::CreateName("TemplateEventBus");
    uuidEventHandler->SetInput_UNIT_TEST(Nodes::Core::EBusEventHandler::c_busIdName, uuidBusId); //Set Uuid bus id

    AZ::EntityId stringEventHandlerId;
    auto stringEventHandler = CreateTestNode<Nodes::Core::EBusEventHandler>(graphUniqueId, stringEventHandlerId);
    stringEventHandler->InitializeBus("TemplateEventTestHandler<AZStd::basic_string<char, AZStd::char_traits<char>, allocator> >");
    AZStd::string stringBusId = "TemplateEventBus";
    stringEventHandler->SetInput_UNIT_TEST(Nodes::Core::EBusEventHandler::c_busIdName, stringBusId); // Set String bus id

    // print (for generic event)
    AZ::EntityId printId;
    auto print = CreateTestNode<Print>(graphUniqueId, printId);
    print->SetText("Generic Event executed");

    // print (for string event)
    AZ::EntityId print2Id;
    auto print2 = CreateTestNode<Print>(graphUniqueId, print2Id);
    print2->SetText("print2 was un-initialized");

    // print (for Vector3 event)
    AZ::EntityId print3Id;
    auto print3 = CreateTestNode<Print>(graphUniqueId, print3Id);
    print3->SetText("print3 was un-initialized");

    // result string
    AZ::EntityId stringId;
    auto stringNode = CreateTestNode<StringView>(graphUniqueId, stringId);

    // result vector
    AZ::EntityId vector3Id;
    auto vector3Node = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3Id);
    vector3Node->SetInput_UNIT_TEST("Set", AZ::Vector3(0.0f, 0.0f, 0.0f));

    // Ebus Handlers connected by Uuid
    {
        // Generic Event
        EXPECT_TRUE(Connect(*graph, uuidEventHandlerId, "Handle:GenericEvent", printId, "In"));

        EXPECT_TRUE(Connect(*graph, uuidEventHandlerId, "Handle:UpdateNameEvent", print2Id, "In"));
        EXPECT_TRUE(Connect(*graph, uuidEventHandlerId, Data::GetName(Data::Type::String()), print2Id, "Value"));
        EXPECT_TRUE(Connect(*graph, uuidEventHandlerId, "Handle:UpdateNameEvent", stringId, "In"));
        EXPECT_TRUE(Connect(*graph, uuidEventHandlerId, Data::GetName(Data::Type::String()), stringId, "View"));
        EXPECT_TRUE(Connect(*graph, stringId, "Result", uuidEventHandlerId, "Result: String"));

        EXPECT_TRUE(Connect(*graph, uuidEventHandlerId, "Handle:VectorCreatedEvent", print3Id, "In"));
        EXPECT_TRUE(Connect(*graph, uuidEventHandlerId, "Vector3", print3Id, "Value"));
        EXPECT_TRUE(Connect(*graph, uuidEventHandlerId, "Vector3", vector3Id, "Set"));
        EXPECT_TRUE(Connect(*graph, vector3Id, "Get", uuidEventHandlerId, "Result: Vector3"));
    }

    // EBus Handlers connected by String
    {
        // Generic Event
        EXPECT_TRUE(Connect(*graph, stringEventHandlerId, "Handle:GenericEvent", printId, "In"));

        EXPECT_TRUE(Connect(*graph, stringEventHandlerId, "Handle:UpdateNameEvent", print2Id, "In"));
        EXPECT_TRUE(Connect(*graph, stringEventHandlerId, Data::GetName(Data::Type::String()), print2Id, "Value"));
        EXPECT_TRUE(Connect(*graph, stringEventHandlerId, Data::GetName(Data::Type::String()), stringId, "View"));
        EXPECT_TRUE(Connect(*graph, stringId, "Result", stringEventHandlerId, "Result: String"));

        EXPECT_TRUE(Connect(*graph, stringEventHandlerId, "Handle:VectorCreatedEvent", print3Id, "In"));
        EXPECT_TRUE(Connect(*graph, stringEventHandlerId, "Vector3", print3Id, "Value"));
        EXPECT_TRUE(Connect(*graph, stringEventHandlerId, "Vector3", vector3Id, "Set"));
        EXPECT_TRUE(Connect(*graph, vector3Id, "Get", stringEventHandlerId, "Result: Vector3"));
    }

    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    TemplateEventTestBus<AZ::Uuid>::Event(uuidBusId, &TemplateEventTest<AZ::Uuid>::GenericEvent);
    EXPECT_FALSE(graph->IsInErrorState());

    TemplateEventTestBus<AZStd::string>::Event(stringBusId, &TemplateEventTest<AZStd::string>::GenericEvent);
    EXPECT_FALSE(graph->IsInErrorState());

    // string
    AZStd::string stringResultByUuid;
    AZStd::string stringResultByString;
    AZStd::string hello("Hello!");
    TemplateEventTestBus<AZ::Uuid>::EventResult(stringResultByUuid, uuidBusId, &TemplateEventTest<AZ::Uuid>::UpdateNameEvent, hello);
    TemplateEventTestBus<AZStd::string>::EventResult(stringResultByString, stringBusId, &TemplateEventTest<AZStd::string>::UpdateNameEvent, hello);
    EXPECT_FALSE(graph->IsInErrorState());

    EXPECT_EQ(hello, stringResultByUuid);
    EXPECT_EQ(hello, stringResultByString);
    EXPECT_EQ(hello, print2->GetText());


    // vector
    AZ::Vector3 vectorResultByUuid = AZ::Vector3::CreateZero();
    AZ::Vector3 vectorResultByString = AZ::Vector3::CreateZero();
    AZ::Vector3 sevenAteNine(7.0f, 8.0f, 9.0f);
    TemplateEventTestBus<AZ::Uuid>::EventResult(vectorResultByUuid, uuidBusId, &TemplateEventTest<AZ::Uuid>::VectorCreatedEvent, sevenAteNine);
    TemplateEventTestBus<AZStd::string>::EventResult(vectorResultByString, stringBusId, &TemplateEventTest<AZStd::string>::VectorCreatedEvent, sevenAteNine);
    EXPECT_FALSE(graph->IsInErrorState());
    
    AZStd::string vectorString = AZStd::string("(x=7.0000000,y=8.0000000,z=9.0000000)");
    EXPECT_EQ(sevenAteNine, vectorResultByUuid);
    EXPECT_EQ(sevenAteNine, vectorResultByString);
    EXPECT_EQ(vectorString, print3->GetText());

    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    StringView::Reflect(m_serializeContext);
    StringView::Reflect(m_behaviorContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZ::Uuid>::Reflect(m_behaviorContext);
    TemplateEventTestHandler<AZStd::string>::Reflect(m_serializeContext);
    TemplateEventTestHandler<AZStd::string>::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
};

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ClassExposition)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    ReflectSignCorrectly();

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();
    AZ::EntityId startNodeID{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNodeID, graphUniqueId, azrtti_typeid<Nodes::Core::Start>());
    Nodes::Core::Start* startNode(nullptr);
    SystemRequestBus::BroadcastResult(startNode, &SystemRequests::GetNode<Nodes::Core::Start>, startNodeID);
    EXPECT_TRUE(startNode != nullptr);

    const AZ::Vector3 allOne(1.0f, 1.0f, 1.0f);
    const AZ::Vector3* allOnePtr = &allOne;

    AZ::Entity* vectorAEntity{ aznew AZ::Entity };
    vectorAEntity->Init();
    AZ::EntityId vectorANodeID{ vectorAEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, vectorANodeID, graphUniqueId, azrtti_typeid<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode>());
    ScriptCanvas::Nodes::Core::BehaviorContextObjectNode* vectorANode(nullptr);
    SystemRequestBus::BroadcastResult(vectorANode, &SystemRequests::GetNode<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode>, vectorANodeID);
    EXPECT_TRUE(vectorANode != nullptr);
    vectorANode->InitializeObject(AZStd::string("Vector3"));
    // if I make way to create/initialize the object as a behavior object reference, I don't need this function anymore
    vectorANode->SetInputFromBehaviorContext_UNIT_TEST("Set", allOne);

    const AZ::Vector3* vectorANodeVector = vectorANode->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    
    EXPECT_EQ(vectorANodeVector, allOnePtr);

    ScriptCanvas::Namespaces emptyNamespaces;

    AZ::Entity* vectorBEntity{ aznew AZ::Entity };
    vectorBEntity->Init();
    AZ::EntityId vectorBNodeID{ vectorBEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, vectorBNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* vectorBNode(nullptr);
    SystemRequestBus::BroadcastResult(vectorBNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, vectorBNodeID);
    EXPECT_TRUE(vectorBNode != nullptr);
    vectorBNode->InitializeObject(AZStd::string("Vector3"));

    AZ::Entity* vectorCEntity{ aznew AZ::Entity };
    vectorCEntity->Init();
    AZ::EntityId vectorCNodeID{ vectorCEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, vectorCNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* vectorCNode(nullptr);
    SystemRequestBus::BroadcastResult(vectorCNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, vectorCNodeID);
    EXPECT_TRUE(vectorCNode != nullptr);
    vectorCNode->InitializeObject(AZStd::string("Vector3"));

    AZ::Entity* normalizeEntity{ aznew AZ::Entity };
    normalizeEntity->Init();
    AZ::EntityId normalizeNodeID{ normalizeEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, normalizeNodeID, graphUniqueId, Nodes::Core::Method::RTTI_Type());
    Nodes::Core::Method* normalizeNode(nullptr);
    SystemRequestBus::BroadcastResult(normalizeNode, &SystemRequests::GetNode<Nodes::Core::Method>, normalizeNodeID);
    EXPECT_TRUE(normalizeNode != nullptr);
    normalizeNode->InitializeClass(emptyNamespaces, AZStd::string("Vector3"), AZStd::string("Normalize"));

    AZ::Entity* getSignEntity{ aznew AZ::Entity };
    getSignEntity->Init();
    AZ::EntityId getSignNodeID{ getSignEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, getSignNodeID, graphUniqueId, Nodes::Core::Method::RTTI_Type());
    Nodes::Core::Method* getSignNode(nullptr);
    SystemRequestBus::BroadcastResult(getSignNode, &SystemRequests::GetNode<Nodes::Core::Method>, getSignNodeID);
    EXPECT_TRUE(getSignNode != nullptr);
    getSignNode->InitializeFree(emptyNamespaces, AZStd::string("Sign"));

    AZ::Entity* getSignEntity2{ aznew AZ::Entity };
    getSignEntity2->Init();
    AZ::EntityId getSignNodeID2{ getSignEntity2->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, getSignNodeID2, graphUniqueId, Nodes::Core::Method::RTTI_Type());
    Nodes::Core::Method* getSignNode2(nullptr);
    SystemRequestBus::BroadcastResult(getSignNode2, &SystemRequests::GetNode<Nodes::Core::Method>, getSignNodeID2);
    EXPECT_TRUE(getSignNode2 != nullptr);
    getSignNode2->InitializeFree(emptyNamespaces, AZStd::string("Sign"));

    AZ::Entity* floatPosEntity{ aznew AZ::Entity };
    floatPosEntity->Init();
    AZ::EntityId floatPosNodeID{ floatPosEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, floatPosNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* floatPosNode(nullptr);
    SystemRequestBus::BroadcastResult(floatPosNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, floatPosNodeID);
    EXPECT_TRUE(floatPosNode != nullptr);
    floatPosNode->InitializeObject(azrtti_typeid<float>());
    floatPosNode->SetInput_UNIT_TEST("Set", 333.0f);

    AZ::Entity* signPosEntity{ aznew AZ::Entity };
    signPosEntity->Init();
    AZ::EntityId signPosNodeID{ signPosEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, signPosNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* signPosNode(nullptr);
    SystemRequestBus::BroadcastResult(signPosNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, signPosNodeID);
    EXPECT_TRUE(signPosNode != nullptr);
    signPosNode->InitializeObject(azrtti_typeid<float>());
    signPosNode->SetInput_UNIT_TEST("Set", -333.0f);

    AZ::Entity* floatNegEntity{ aznew AZ::Entity };
    floatNegEntity->Init();
    AZ::EntityId floatNegNodeID{ floatNegEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, floatNegNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* floatNegNode(nullptr);
    SystemRequestBus::BroadcastResult(floatNegNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, floatNegNodeID);
    EXPECT_TRUE(floatNegNode != nullptr);
    floatNegNode->InitializeObject(azrtti_typeid<float>());
    floatNegNode->SetInput_UNIT_TEST("Set", -333.0f);

    AZ::Entity* signNegEntity{ aznew AZ::Entity };
    signNegEntity->Init();
    AZ::EntityId signNegNodeID{ signNegEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, signNegNodeID, graphUniqueId, Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
    Nodes::Core::BehaviorContextObjectNode* signNegNode(nullptr);
    SystemRequestBus::BroadcastResult(signNegNode, &SystemRequests::GetNode<Nodes::Core::BehaviorContextObjectNode>, signNegNodeID);
    EXPECT_TRUE(signNegNode != nullptr);
    signNegNode->InitializeObject(azrtti_typeid<float>());
    signNegNode->SetInput_UNIT_TEST("Set", 333.0f);


    EXPECT_TRUE(normalizeNode->GetSlotId("Vector3: 0").IsValid());
    EXPECT_TRUE(vectorANode->GetSlotId("Get").IsValid());

    auto vector30 = normalizeNode->GetSlotId("Vector3: 0");
    auto getID = vectorANode->GetSlotId("Get");

    EXPECT_TRUE(graph->Connect(startNodeID, startNode->GetSlotId("Out"), normalizeNodeID, normalizeNode->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(normalizeNodeID, normalizeNode->GetSlotId("Vector3: 0"), vectorANodeID, vectorANode->GetSlotId("Get")));

    EXPECT_TRUE(graph->Connect(normalizeNodeID, normalizeNode->GetSlotId("Out"), getSignNodeID, getSignNode->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(getSignNodeID, getSignNode->GetSlotId("Number: 0"), floatPosNodeID, floatPosNode->GetSlotId("Get")));
    EXPECT_TRUE(graph->Connect(getSignNodeID, getSignNode->GetSlotId("Result: Number"), signPosNodeID, signPosNode->GetSlotId("Set")));

    EXPECT_TRUE(graph->Connect(getSignNodeID, getSignNode->GetSlotId("Out"), getSignNodeID2, getSignNode2->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(getSignNodeID2, getSignNode2->GetSlotId("Number: 0"), floatNegNodeID, floatNegNode->GetSlotId("Get")));
    EXPECT_TRUE(graph->Connect(getSignNodeID2, getSignNode2->GetSlotId("Result: Number"), signNegNodeID, signNegNode->GetSlotId("Set")));

    AZ::Entity entity("ScriptCanvasHandlerTest");
    entity.Init();
    entity.Activate();
    AZ::EntityId entityID(entity.GetId());

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    if (vectorANodeVector)
    {
        EXPECT_TRUE(vectorANodeVector->IsNormalized(0.1f)) << "Vector 3 not normalized";
    }
    else
    {
        ADD_FAILURE() << "Vector3 type not found!";
    }

    if (auto positive = signPosNode->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(*positive, 1.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto negative = signNegNode->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(*negative, -1.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;
};

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ObjectTrackingByValue)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    
    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByValue");
    
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());
    
    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());
    
    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));
    
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }
    
    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ObjectTrackingByPointer)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    
    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByPointerID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByPointer");
    
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());
    
    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByPointerID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByPointerID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByPointerID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ObjectTrackingByReference)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    
    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByReferenceID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByReference");
    
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());
        
    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByReferenceID, "TestBehaviorContextObject: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByReferenceID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByReferenceID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, value3->GetValue());
        EXPECT_EQ(value3, value4);
        EXPECT_EQ(value3, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_InvalidInputByReference)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    
    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByReferenceID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByReference");
    
    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());
        
    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByReferenceID, "TestBehaviorContextObject: 0"));
    // EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByReferenceID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByReferenceID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_TRUE(graph->IsInErrorState());
    
    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_InvalidInputByValue)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByValueID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByValue");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    Core::BehaviorContextObjectNode* valueNode1 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID1);
    valueNode1->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode1->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(1);
    EXPECT_EQ(1, valueNode1->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode2 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID2);
    valueNode2->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode2->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(2);
    EXPECT_EQ(2, valueNode2->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode3 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID3);
    valueNode3->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode3->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(3);
    EXPECT_EQ(3, valueNode3->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode4 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID4);
    valueNode4->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode4->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(4);
    EXPECT_EQ(4, valueNode4->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    Core::BehaviorContextObjectNode* valueNode5 = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, valueID5);
    valueNode5->InitializeObject(azrtti_typeid<TestBehaviorContextObject>());
    valueNode5->ModInput_UNIT_TEST<TestBehaviorContextObject>("Set")->SetValue(5);
    EXPECT_EQ(5, valueNode5->GetInput_UNIT_TEST<TestBehaviorContextObject>("Set")->GetValue());

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByValueID, "TestBehaviorContextObject: 0"));
    // EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByValueID, "TestBehaviorContextObject: 1"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByValueID, "Result: TestBehaviorContextObject", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByValueID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_TRUE(graph->IsInErrorState());
    
    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ScriptCanvasValueDataTypesByValue)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByPointerID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByValueInteger");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    auto* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByPointerID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByPointerID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByPointerID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
        EXPECT_NE(value4, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ScriptCanvasValueDataTypesByPointer)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByPointerID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByPointerInteger");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    auto* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByPointerID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByPointerID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByPointerID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByPointerID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
        EXPECT_NE(value4, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ScriptCanvasValueDataTypesByReference)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    TestBehaviorContextObject::ResetCounts();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    graph->SetStartNode(startID);

    AZ::EntityId maxByReferenceID = CreateClassFunctionNode(graphUniqueId, "TestBehaviorContextObject", "MaxReturnByReferenceInteger");

    AZ::EntityId valueID1, valueID2, valueID3, valueID4, valueID5;

    auto* valueNode1 = CreateDataNode<Data::NumberType>(graphUniqueId, 1, valueID1);
    EXPECT_EQ(1, *valueNode1->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode2 = CreateDataNode<Data::NumberType>(graphUniqueId, 2, valueID2);
    EXPECT_EQ(2, *valueNode2->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode3 = CreateDataNode<Data::NumberType>(graphUniqueId, 3, valueID3);
    EXPECT_EQ(3, *valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode4 = CreateDataNode<Data::NumberType>(graphUniqueId, 4, valueID4);
    EXPECT_EQ(4, *valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    auto* valueNode5 = CreateDataNode<Data::NumberType>(graphUniqueId, 5, valueID5);
    EXPECT_EQ(5, *valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set"));

    // data
    EXPECT_TRUE(Connect(*graph, valueID1, "Get", maxByReferenceID, "Number: 0"));
    EXPECT_TRUE(Connect(*graph, valueID2, "Get", maxByReferenceID, "Number: 1"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: Number", valueID3, "Set"));
    EXPECT_TRUE(Connect(*graph, valueID3, "Get", valueID4, "Set"));
    EXPECT_TRUE(Connect(*graph, maxByReferenceID, "Result: Number", valueID5, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startID, "Out", maxByReferenceID, "In"));

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    const AZ::u32 destroyedCount = TestBehaviorContextObject::GetDestroyedCount();
    const AZ::u32 createdCount = TestBehaviorContextObject::GetCreatedCount();

    auto value3 = valueNode3->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value4 = valueNode4->GetInput_UNIT_TEST<Data::NumberType>("Set");
    auto value5 = valueNode5->GetInput_UNIT_TEST<Data::NumberType>("Set");

    if (value3 && value4 && value5)
    {
        EXPECT_EQ(2, *value3);
        EXPECT_EQ(2, *value4);
        EXPECT_EQ(2, *value5);
        EXPECT_NE(value3, value4);
        EXPECT_NE(value3, value5);
        EXPECT_NE(value4, value5);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graph->GetEntity();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BehaviorContext_ScriptCanvasStringToNonAZStdString)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    ConvertibleToString::Reflect(m_serializeContext);
    ConvertibleToString::Reflect(m_behaviorContext);

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_NE(nullptr, graph);
    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startId;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startId);

    AZ::EntityId constCharPtrToStringId = CreateClassFunctionNode(graphUniqueId, "ConvertibleToString", "ConstCharPtrToString");
    AZ::EntityId stringViewToStringId = CreateClassFunctionNode(graphUniqueId, "ConvertibleToString", "StringViewToString");

    AZ::EntityId stringId1, stringId2;
    AZ::EntityId resultValueId1, resultValueId2;

    auto* stringNode1 = CreateDataNode<Data::StringType>(graphUniqueId, "Hello", stringId1);
    EXPECT_EQ("Hello", *stringNode1->GetInput_UNIT_TEST<Data::StringType>("Set"));

    auto* stringNode2 = CreateDataNode<Data::StringType>(graphUniqueId, "World", stringId2);
    EXPECT_EQ("World", *stringNode2->GetInput_UNIT_TEST<Data::StringType>("Set"));

    auto* resultValueNode1 = CreateDataNode<Data::StringType>(graphUniqueId, "", resultValueId1);
    auto* resultValueNode2 = CreateDataNode<Data::StringType>(graphUniqueId, "", resultValueId2);

    // data
    EXPECT_TRUE(Connect(*graph, stringId1, "Get", constCharPtrToStringId, "String: 0"));
    EXPECT_TRUE(Connect(*graph, stringId2, "Get", stringViewToStringId, "String: 0"));
    EXPECT_TRUE(Connect(*graph, constCharPtrToStringId, "Result: String", resultValueId1, "Set"));
    EXPECT_TRUE(Connect(*graph, stringViewToStringId, "Result: String", resultValueId2, "Set"));

    // execution
    EXPECT_TRUE(Connect(*graph, startId, "Out", constCharPtrToStringId, "In"));
    EXPECT_TRUE(Connect(*graph, startId, "Out", stringViewToStringId, "In"));

    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());


    auto constCharToStringValue = resultValueNode1->GetInput_UNIT_TEST<Data::StringType>("Set");
    auto stringViewToStringValue = resultValueNode2->GetInput_UNIT_TEST<Data::StringType>("Set");

    EXPECT_NE(nullptr, constCharToStringValue);
    EXPECT_NE(nullptr, stringViewToStringValue);
    if (constCharToStringValue && stringViewToStringValue)
    {
        EXPECT_EQ("Hello", *constCharToStringValue);
        EXPECT_EQ("World", *stringViewToStringValue);
    }
    else
    {
        ADD_FAILURE() << "Values were nullptr";
    }

    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    ConvertibleToString::Reflect(m_serializeContext);
    ConvertibleToString::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, BinaryOperationTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas::Nodes;
    using namespace ScriptCanvas::Nodes::Comparison;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    BinaryOpTest<EqualTo>(1, 1, true);
    BinaryOpTest<EqualTo>(1, 0, false);
    BinaryOpTest<EqualTo>(0, 1, false);
    BinaryOpTest<EqualTo>(0, 0, true);

    BinaryOpTest<NotEqualTo>(1, 1, false);
    BinaryOpTest<NotEqualTo>(1, 0, true);
    BinaryOpTest<NotEqualTo>(0, 1, true);
    BinaryOpTest<NotEqualTo>(0, 0, false);

    BinaryOpTest<Greater>(1, 1, false);
    BinaryOpTest<Greater>(1, 0, true);
    BinaryOpTest<Greater>(0, 1, false);
    BinaryOpTest<Greater>(0, 0, false);

    BinaryOpTest<GreaterEqual>(1, 1, true);
    BinaryOpTest<GreaterEqual>(1, 0, true);
    BinaryOpTest<GreaterEqual>(0, 1, false);
    BinaryOpTest<GreaterEqual>(0, 0, true);

    BinaryOpTest<Less>(1, 1, false);
    BinaryOpTest<Less>(1, 0, false);
    BinaryOpTest<Less>(0, 1, true);
    BinaryOpTest<Less>(0, 0, false);

    BinaryOpTest<LessEqual>(1, 1, true);
    BinaryOpTest<LessEqual>(1, 0, false);
    BinaryOpTest<LessEqual>(0, 1, true);
    BinaryOpTest<LessEqual>(0, 0, true);

    BinaryOpTest<EqualTo>(true, true, true);
    BinaryOpTest<EqualTo>(true, false, false);
    BinaryOpTest<EqualTo>(false, true, false);
    BinaryOpTest<EqualTo>(false, false, true);

    BinaryOpTest<NotEqualTo>(true, true, false);
    BinaryOpTest<NotEqualTo>(true, false, true);
    BinaryOpTest<NotEqualTo>(false, true, true);
    BinaryOpTest<NotEqualTo>(false, false, false);

    const AZ::Vector3 vectorOne(1.0f, 1.0f, 1.0f);
    const AZ::Vector3 vectorZero(0.0f, 0.0f, 0.0f);

    BinaryOpTest<EqualTo>(vectorOne, vectorOne, true);
    BinaryOpTest<EqualTo>(vectorOne, vectorZero, false);
    BinaryOpTest<EqualTo>(vectorZero, vectorOne, false);
    BinaryOpTest<EqualTo>(vectorZero, vectorZero, true);

    BinaryOpTest<NotEqualTo>(vectorOne, vectorOne, false);
    BinaryOpTest<NotEqualTo>(vectorOne, vectorZero, true);
    BinaryOpTest<NotEqualTo>(vectorZero, vectorOne, true);
    BinaryOpTest<NotEqualTo>(vectorZero, vectorZero, false);

    const TestBehaviorContextObject zero(0);
    const TestBehaviorContextObject one(1);
    const TestBehaviorContextObject otherOne(1);

    BinaryOpTest<EqualTo>(one, one, true);
    BinaryOpTest<EqualTo>(one, zero, false);
    BinaryOpTest<EqualTo>(zero, one, false);
    BinaryOpTest<EqualTo>(zero, zero, true);

    BinaryOpTest<NotEqualTo>(one, one, false);
    BinaryOpTest<NotEqualTo>(one, zero, true);
    BinaryOpTest<NotEqualTo>(zero, one, true);
    BinaryOpTest<NotEqualTo>(zero, zero, false);

    BinaryOpTest<Greater>(one, one, false);
    BinaryOpTest<Greater>(one, zero, true);
    BinaryOpTest<Greater>(zero, one, false);
    BinaryOpTest<Greater>(zero, zero, false);

    BinaryOpTest<GreaterEqual>(one, one, true);
    BinaryOpTest<GreaterEqual>(one, zero, true);
    BinaryOpTest<GreaterEqual>(zero, one, false);
    BinaryOpTest<GreaterEqual>(zero, zero, true);

    BinaryOpTest<Less>(one, one, false);
    BinaryOpTest<Less>(one, zero, false);
    BinaryOpTest<Less>(zero, one, true);
    BinaryOpTest<Less>(zero, zero, false);

    BinaryOpTest<LessEqual>(one, one, true);
    BinaryOpTest<LessEqual>(one, zero, false);
    BinaryOpTest<LessEqual>(zero, one, true);
    BinaryOpTest<LessEqual>(zero, zero, true);
   
    BinaryOpTest<EqualTo>(one, otherOne, true);
    BinaryOpTest<EqualTo>(otherOne, one, true);

    BinaryOpTest<NotEqualTo>(one, otherOne, false);
    BinaryOpTest<NotEqualTo>(otherOne, one, false);

    BinaryOpTest<Greater>(one, otherOne, false);
    BinaryOpTest<Greater>(otherOne, one, false);

    BinaryOpTest<GreaterEqual>(one, otherOne, true);
    BinaryOpTest<GreaterEqual>(otherOne, one, true);

    BinaryOpTest<Less>(one, otherOne, false);
    BinaryOpTest<Less>(otherOne, one, false);

    BinaryOpTest<LessEqual>(one, otherOne, true);
    BinaryOpTest<LessEqual>(otherOne, one, true);

    // force failures, to verify crash proofiness
    BinaryOpTest<EqualTo>(one, zero, false, true);
    BinaryOpTest<NotEqualTo>(one, zero, true, true);
    BinaryOpTest<Greater>(one, zero, true, true);
    BinaryOpTest<GreaterEqual>(one, zero, true, true);
    BinaryOpTest<Less>(one, zero, false, true);
    BinaryOpTest<LessEqual>(one, zero, false, true);

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, EntityRefTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    // Fake "world" entities
    AZ::Entity first("First");
    first.CreateComponent<ScriptCanvasTests::TestComponent>();
    first.Init();
    first.Activate();

    AZ::Entity second("Second");
    second.CreateComponent<ScriptCanvasTests::TestComponent>();
    second.Init();
    second.Activate();

    // Script Canvas Graph
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    graph->GetEntity()->SetName("Graph Owner Entity");
    
    graph->GetEntity()->CreateComponent<ScriptCanvasTests::TestComponent>();
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    // EntityRef node to some specific entity #1
    AZ::EntityId selfEntityIdNode;
    auto selfEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, selfEntityIdNode);
    selfEntityRef->SetEntityRef(first.GetId());

    // EntityRef node to some specific entity #2
    AZ::EntityId otherEntityIdNode;
    auto otherEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, otherEntityIdNode);
    otherEntityRef->SetEntityRef(second.GetId());

    // Explicitly set an EntityRef node with this graph's entity Id.
    AZ::EntityId graphEntityIdNode;
    auto graphEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, graphEntityIdNode);
    graphEntityRef->SetEntityRef(graph->GetEntity()->GetId());

    // First test: directly set an entity Id on the EntityID: 0 slot
    AZ::EntityId eventAid;
    Nodes::Core::Method* nodeA = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventAid);
    nodeA->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");
    if (auto entityId = nodeA->ModInput_UNIT_TEST<AZ::EntityId>("EntityID: 0"))
    {
        *entityId = first.GetId();
    }
    
    // Second test: Will connect the slot to an EntityRef node
    AZ::EntityId eventBid;
    Nodes::Core::Method* nodeB = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventBid);
    nodeB->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");

    // Third test: Set the slot's EntityId: 0 to SelfReferenceId, this should result in the same Id as graph->GetEntityId()
    AZ::EntityId eventCid;
    Nodes::Core::Method* nodeC = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventCid);
    nodeC->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");
    if (auto entityId = nodeC->ModInput_UNIT_TEST<AZ::EntityId>("EntityID: 0"))
    {
        *entityId = ScriptCanvas::SelfReferenceId;
    }

    // True 
    AZ::EntityId trueNodeId;
    CreateDataNode<Data::BooleanType>(graphUniqueId, true, trueNodeId);

    // False
    AZ::EntityId falseNodeId;
    CreateDataNode<Data::BooleanType>(graphUniqueId, false, falseNodeId);

    // Start            -> TestEvent
    //                   O EntityId: 0    (not connected, it was set directly on the slot)
    // EntityRef: first -O EntityId: 1
    // False            -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventAid, "In"));
    EXPECT_TRUE(Connect(*graph, eventAid, "EntityID: 1", selfEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventAid, "Boolean: 2", falseNodeId, "Get"));

    // Start            -> TestEvent
    // EntityRef: second -O EntityId: 0 
    // EntityRef: second -O EntityId: 1
    // False             -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventBid, "In"));
    EXPECT_TRUE(Connect(*graph, eventBid, "EntityID: 0", otherEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventBid, "EntityID: 1", otherEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventBid, "Boolean: 2", falseNodeId, "Get"));

    // Start            -> TestEvent
    //                   -O EntityId: 0  (not connected, slot is set to SelfReferenceId)
    // graphEntityIdNode -O EntityId: 1
    // True              -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventCid, "In"));
    EXPECT_TRUE(Connect(*graph, eventCid, "EntityID: 1", graphEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventCid, "Boolean: 2", trueNodeId, "Get"));

    // Execute the graph
    graph->ResolveSelfReferences(graphEntityId);

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    delete graph->GetEntity();

}

// Asynchronous ScriptCanvas Behaviors

// TODO #lsempe: Async tests are not available in VS2013 due to some errors with std::promise, enable after problem is solved.
#if AZ_COMPILER_MSVC && AZ_COMPILER_MSVC >= 1900

#pragma warning(disable: 4355) // The this pointer is valid only within nonstatic member functions. It cannot be used in the initializer list for a base class.
#include <future>

class AsyncEvent : public AZ::EBusTraits
{
public:

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = AZ::EntityId;
    //////////////////////////////////////////////////////////////////////////

    virtual void OnAsyncEvent() = 0;
};

using AsyncEventNotificationBus = AZ::EBus<AsyncEvent>;

class LongRunningProcessSimulator3000
{
public:

    static void Run(const AZ::EntityId& listener)
    {
        int duration = 40;
        while (--duration > 0)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        }

        AsyncEventNotificationBus::Event(listener, &AsyncEvent::OnAsyncEvent);
    }
};

class AsyncNode
    : public ScriptCanvas::Node
    , protected AsyncEventNotificationBus::Handler
    , protected AZ::TickBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(AsyncNode, AZ::SystemAllocator, 0);
    AZ_RTTI(AsyncNode, "{0A7FF6C6-878B-42EC-A8BB-4D29C4039853}", ScriptCanvas::Node);

    bool IsEntryPoint() const { return true; }

    AsyncNode()
        : Node()
    {}

    static void Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<AsyncNode, Node>()
                ->Version(1)
            ;
        }
    }

    void ConfigureSlots() override
    {
        AddSlot("In", "", ScriptCanvas::SlotType::ExecutionIn);
        AddSlot("Out", "", ScriptCanvas::SlotType::ExecutionOut);
    }

    void OnActivate() override
    {
        AZ::TickBus::Handler::BusConnect();
        AsyncEventNotificationBus::Handler::BusConnect(GetEntityId());

        std::packaged_task<void()> task([this]() { LongRunningProcessSimulator3000::Run(GetEntityId()); }); // wrap the function
        std::thread(std::move(task)).detach(); // launch on a thread
    }

    void OnDeactivate() override
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    virtual void HandleAsyncEvent()
    {
        EXPECT_GT(m_duration, 0.f);

        Shutdown();
    }

    void OnAsyncEvent() override
    {
        HandleAsyncEvent();
    }

    void Shutdown()
    {
        // We've received the event, no longer need the bus connection
        AsyncEventNotificationBus::Handler::BusDisconnect();

        // We're done, kick it out.
        SignalOutput(GetSlotId("Out"));

        // Disconnect from tick bus as well
        AZ::TickBus::Handler::BusDisconnect();

        // Deactivate the graph's entity to shut everything down.
        GetGraph()->GetEntity()->Deactivate();
    }

    void OnTick(float deltaTime, AZ::ScriptTimePoint) override
    {
        AZ_TracePrintf("Debug", "Awaiting async operation: %.2f\n", m_duration);
        m_duration += deltaTime;
    }

private:

    double m_duration = 0.f;
};

TEST_F(ScriptCanvasTestFixture, Asynchronous_Behaviors)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;

    AsyncNode::Reflect(m_serializeContext);
    AsyncNode::Reflect(m_behaviorContext);

    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();

    AZ::EntityId startNodeId;
    Nodes::Core::Start* startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);

    AZ::EntityId asyncNodeId;
    AsyncNode* asyncNode = CreateTestNode<AsyncNode>(graphUniqueId, asyncNodeId);

    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", asyncNodeId, "In"));

    graphEntity->Activate();

    // Tick the TickBus while the graph entity is active
    while (graph->GetEntity()->GetState() == AZ::Entity::ES_ACTIVE)
    {
        AZ::TickBus::ExecuteQueuedEvents();
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.01f, AZ::ScriptTimePoint(AZStd::chrono::system_clock::now()));
    }

    delete graphEntity;
}

///////////////////////////////////////////////////////////////////////////////

namespace
{
    // Fibonacci solver, used to compare against the graph version.
    long ComputeFibonacci(int digits)
    {
        int a = 0;
        int b = 1;
        long sum = 0;
        for (int i = 0; i < digits - 2; ++i)
        {
            sum = a + b;
            a = b;
            b = sum;
        }
        return sum;
    }
}

class AsyncFibonacciComputeNode
    : public AsyncNode
{
public:
    AZ_CLASS_ALLOCATOR(AsyncFibonacciComputeNode, AZ::SystemAllocator, 0);
    AZ_RTTI(AsyncFibonacciComputeNode, "{B198F52D-708C-414B-BB90-DFF0462D7F03}", AsyncNode);

    AsyncFibonacciComputeNode()
    : AsyncNode()
    {}

    bool IsEntryPoint() const { return true; }

    static const int k_numberOfFibonacciDigits = 64;

    static void Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<AsyncFibonacciComputeNode, AsyncNode>()
                ->Version(1)
            ;
        }
    }

    void OnActivate() override
    {
        AZ::TickBus::Handler::BusConnect();
        AsyncEventNotificationBus::Handler::BusConnect(GetEntityId());

        int digits = k_numberOfFibonacciDigits;

        std::promise<long> p;
        m_computeFuture = p.get_future();
        std::thread([this, digits](std::promise<long> p)
        {
            p.set_value(ComputeFibonacci(digits));
            AsyncEventNotificationBus::Event(GetEntityId(), &AsyncEvent::OnAsyncEvent);
        }, AZStd::move(p)).detach();
    }

    void OnDeactivate() override
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void HandleAsyncEvent() override
    {
        m_result = m_computeFuture.get();

        EXPECT_EQ(m_result, ComputeFibonacci(k_numberOfFibonacciDigits));
    }

    void OnTick(float deltaTime, AZ::ScriptTimePoint) override
    {
        AZ_TracePrintf("Debug", "Awaiting async fib operation: %.2f\n", m_duration);
        m_duration += deltaTime;

        if (m_result != 0)
        {
            Shutdown();
        }
    }

private:

    std::future<long> m_computeFuture;
    long m_result = 0;
    double m_duration = 0.f;
};

TEST_F(ScriptCanvasTestFixture, ComputeFibonacciAsyncGraphTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;

    AsyncFibonacciComputeNode::Reflect(m_serializeContext);
    AsyncFibonacciComputeNode::Reflect(m_behaviorContext);

    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    AZ::Entity* graphEntity = graph->GetEntity();
    graphEntity->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startNodeId;
    Nodes::Core::Start* startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, startNodeId);

    AZ::EntityId asyncNodeId;
    AsyncFibonacciComputeNode* asyncNode = CreateTestNode<AsyncFibonacciComputeNode>(graphUniqueId, asyncNodeId);

    EXPECT_TRUE(Connect(*graph, startNodeId, "Out", asyncNodeId, "In"));

    graphEntity->Activate();

    // Tick the TickBus while the graph entity is active
    while (graph->GetEntity()->GetState() == AZ::Entity::ES_ACTIVE)
    {
        AZ::TickBus::ExecuteQueuedEvents();
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.01f, AZ::ScriptTimePoint(AZStd::chrono::system_clock::now()));
    }

    delete graphEntity;
}

#endif // AZ_COMPILER_MSVC >= 1900

AZ_UNIT_TEST_HOOK();