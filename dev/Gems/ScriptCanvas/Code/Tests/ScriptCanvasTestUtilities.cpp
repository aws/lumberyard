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
#include "ScriptCanvasTestUtilities.h"

namespace ScriptCanvasTests
{

    TestBehaviorContextObject TestBehaviorContextObject::MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs)
    {
        return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
    }

    const TestBehaviorContextObject* TestBehaviorContextObject::MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs)
    {
        return (lhs && rhs && lhs->GetValue() >= rhs->GetValue()) ? lhs : rhs;
    }

    const TestBehaviorContextObject& TestBehaviorContextObject::MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs)
    {
        return lhs.GetValue() >= rhs.GetValue() ? lhs : rhs;
    }

    int TestBehaviorContextObject::MaxReturnByValueInteger(int lhs, int rhs)
    {
        return lhs >= rhs ? lhs : rhs;
    }

    const int* TestBehaviorContextObject::MaxReturnByPointerInteger(const int* lhs, const int* rhs)
    {
        return (lhs && rhs && (*lhs) >= (*rhs)) ? lhs : rhs;
    }

    const int& TestBehaviorContextObject::MaxReturnByReferenceInteger(const int& lhs, const int& rhs)
    {
        return lhs >= rhs ? lhs : rhs;
    }

    void TestBehaviorContextObject::Reflect(AZ::ReflectContext* reflectContext)
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

    AZ::u32 TestBehaviorContextObject::s_createdCount = 0;
    AZ::u32 TestBehaviorContextObject::s_destroyedCount = 0;

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

    ScriptCanvas::Node* CreateDataNodeByType(const AZ::EntityId& graphUniqueId, const ScriptCanvas::Data::Type& type, AZ::EntityId& nodeIDout)
    {
        using namespace ScriptCanvas;

        Node* node(nullptr);

        switch (type.GetType())
        {
        case Data::eType::AABB:
            node = CreateTestNode<Nodes::Math::AABB>(graphUniqueId, nodeIDout);
            break;

        case Data::eType::BehaviorContextObject:
        {
            auto objectNode = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, nodeIDout);
            objectNode->InitializeObject(type);
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

    bool Connect(ScriptCanvas::Graph& graph, const AZ::EntityId& fromNodeID, const char* fromSlotName, const AZ::EntityId& toNodeID, const char* toSlotName, bool dumpSlotsOnFailure /*= true*/)
    {
        using namespace ScriptCanvas;
        AZ::Entity* fromNode{};
        AZ::ComponentApplicationBus::BroadcastResult(fromNode, &AZ::ComponentApplicationBus::Events::FindEntity, fromNodeID);
        AZ::Entity* toNode{};
        AZ::ComponentApplicationBus::BroadcastResult(toNode, &AZ::ComponentApplicationBus::Events::FindEntity, toNodeID);
        if (fromNode && toNode)
        {
            Node* from = AZ::EntityUtils::FindFirstDerivedComponent<Node>(fromNode);
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

} // ScriptCanvasTests