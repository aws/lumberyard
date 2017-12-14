/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"

#include <Tests/ScriptCanvasTestFixture.h>
#include <Tests/ScriptCanvasTestUtilities.h>

#include <AzCore/Math/Vector3.h>
#include <ScriptCanvas/Libraries/Math/Math.h>
#include <ScriptCanvas/Data/Data.h>

using namespace ScriptCanvasTests;

template<typename t_Node>
AZStd::vector<ScriptCanvas::Datum> TestMathFunction(std::initializer_list<AZStd::string_view> inputNamesList, std::initializer_list<ScriptCanvas::Datum> inputList, std::initializer_list<AZStd::string_view> outputNamesList, std::initializer_list<ScriptCanvas::Datum> outputList)
{
    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::vector<AZStd::string_view> inputNames(inputNamesList);
    AZStd::vector<Datum> input(inputList);
    AZStd::vector<AZStd::string_view> outputNames(outputNamesList);
    AZStd::vector<Datum> output(outputList);

    AZ_Assert(inputNames.size() == input.size(), "Size mismatch");
    AZ_Assert(outputNames.size() == output.size(), "Size mismatch");

    AZ::Entity graphEntity("Graph");
    graphEntity.Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, &graphEntity);
    auto graph = graphEntity.FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId functionID;
    CreateTestNode<t_Node>(graphUniqueId, functionID);
    EXPECT_TRUE(Connect(*graph, startID, "Out", functionID, "In"));

    AZStd::vector<Node*> inputNodes;
    AZStd::vector<AZ::EntityId> inputNodeIDs;
    AZStd::vector<Node*> outputNodes;
    AZStd::vector<AZ::EntityId> outputNodeIDs;

    for (int i = 0; i < input.size(); ++i)
    {
        AZ::EntityId inputNodeID;
        auto node = CreateDataNodeByType(graphUniqueId, input[i].GetType(), inputNodeID);
        inputNodeIDs.push_back(inputNodeID);
        inputNodes.push_back(node);
        node->SetInputDatum_UNIT_TEST(0, input[i]);
    }

    for (int i = 0; i < output.size(); ++i)
    {
        AZ::EntityId outputNodeID;
        auto node = CreateDataNodeByType(graphUniqueId, output[i].GetType(), outputNodeID);
        outputNodeIDs.push_back(outputNodeID);
        outputNodes.push_back(node);
    }

    for (int i = 0; i < inputNames.size(); ++i)
    {
        EXPECT_TRUE(Connect(*graph, inputNodeIDs[i], "Get", functionID, inputNames[i].data()));
    }

    for (int i = 0; i < output.size(); ++i)
    {
        EXPECT_TRUE(Connect(*graph, functionID, outputNames[i].data(), outputNodeIDs[i], "Set"));
    }

    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());

    for (int i = 0; i < output.size(); ++i)
    {
        output[i] = *outputNodes[i]->GetInput_UNIT_TEST(0);
    }

    graph->GetEntity()->Deactivate();
    delete graph;
    return output;
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
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addId);
    AZ::EntityId subtractId;
    CreateTestNode<Vector3Nodes::SubtractNode>(graphUniqueId, subtractId);
    AZ::EntityId normalizeId;
    CreateTestNode<Vector3Nodes::NormalizeNode>(graphUniqueId, normalizeId);

    EXPECT_TRUE(Connect(*graph, addVectorId, "Get", addId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, addVectorId, "Get", addId, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, addVectorId, "Set", addId, "Result: Vector3"));

    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Get", subtractId, "Vector3: A"));
    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Get", subtractId, "Vector3: B"));
    EXPECT_TRUE(Connect(*graph, subtractVectorId, "Set", subtractId, "Result: Vector3"));

    EXPECT_TRUE(Connect(*graph, normalizeVectorId, "Get", normalizeId, "Vector3: Source"));
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
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addId);
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
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addIdA);
    AZ::EntityId addIdB;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addIdB);
    AZ::EntityId addIdC;
    CreateTestNode<Vector3Nodes::AddNode>(graphUniqueId, addIdC);

    AZ::EntityId add3IdA = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");
    AZ::EntityId add3IdB = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");
    AZ::EntityId add3IdC = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");

    AZ::EntityId vectorNormalizeId;
    CreateTestNode<Vector3Nodes::NormalizeNode>(graphUniqueId, vectorNormalizeId);

    const AZ::Vector3 x10(10, 0, 0);
    AZ::EntityId vectorNormalizedInId, vectorNormalizedOutId, numberLengthId;
    Node* vectorNormalizedInNode = CreateDataNode<AZ::Vector3>(graphUniqueId, x10, vectorNormalizedInId);
    Node* vectorNormalizedOutNode = CreateDataNode<AZ::Vector3>(graphUniqueId, x10, vectorNormalizedOutId);

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

    EXPECT_TRUE(Connect(*graph, vectorNormalizedInId, "Get", vectorNormalizeId, "Vector3: Source"));
    EXPECT_TRUE(Connect(*graph, vectorNormalizeId, "Result: Vector3", vectorNormalizedOutId, "Set"));

    // execution connections
    EXPECT_TRUE(Connect(*graph, startID, "Out", addIdA, "In"));
    EXPECT_TRUE(Connect(*graph, addIdA, "Out", add3IdA, "In"));
    EXPECT_TRUE(Connect(*graph, add3IdA, "Out", addIdB, "In"));
    EXPECT_TRUE(Connect(*graph, addIdB, "Out", add3IdB, "In"));
    EXPECT_TRUE(Connect(*graph, add3IdB, "Out", addIdC, "In"));
    EXPECT_TRUE(Connect(*graph, addIdC, "Out", add3IdC, "In"));

    EXPECT_TRUE(Connect(*graph, add3IdC, "Out", vectorNormalizeId, "In"));

    EXPECT_EQ(*vectorNodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorNodeB->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorNodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);
    EXPECT_EQ(*vectorNodeD->GetInput_UNIT_TEST<AZ::Vector3>("Set"), allOne4);

    EXPECT_EQ(*vectorNormalizedInNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), x10);
    EXPECT_EQ(*vectorNormalizedOutNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"), x10);

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
    EXPECT_EQ(x10, normalizedIn);
    EXPECT_EQ(AZ::Vector3(1, 0, 0), normalizedOut);

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

TEST_F(ScriptCanvasTestFixture, ColorNodes)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::ColorNodes;
    using namespace ScriptCanvas::Data;
    using namespace ScriptCanvas::Nodes;

    const ColorType a(Vector4Type(0.5f, 0.25f, 0.75f, 1.0f));
    const ColorType b(Vector4Type(0.66f, 0.33f, 0.5f, 0.5f));
    const NumberType number(0.314);

    { // Add
        auto result = a + b;
        auto output = TestMathFunction<AddNode>({ "Color: A", "Color: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // DivideByColor
        auto result = a / b;
        auto output = TestMathFunction<DivideByColorNode>({ "Color: A", "Color: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }
#endif

    { // DivideByNumber
        auto result = a / ToVectorFloat(number);
        auto output = TestMathFunction<DivideByNumberNode>({ "Color: Source", "Number: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(number) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // Dot
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Color: A", "Color: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // Dot3
        auto result = a.Dot3(b);
        auto output = TestMathFunction<Dot3Node>({ "Color: A", "Color: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // FromValues
        auto result = ColorType(0.2f, 0.4f, 0.6f, 0.8f);
        auto output = TestMathFunction<FromValuesNode>({ "Number: R", "Number: G", "Number: B", "Number: A" }, { Datum::CreateInitializedCopy(0.2f), Datum::CreateInitializedCopy(0.4f), Datum::CreateInitializedCopy(0.6f), Datum::CreateInitializedCopy(0.8f) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // FromVector3
        auto source = Vector3Type(0.2f, 0.4f, 0.6f);
        auto result = ColorType::CreateFromVector3(source);
        auto output = TestMathFunction<FromVector3Node>({ "Vector3: RGB" }, { Datum::CreateInitializedCopy(source) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // FromVector3AndNumber
        auto vector3 = Vector3Type(0.1, 0.2, 0.3);
        auto number = 0.4f;
        auto result = ColorType::CreateFromVector3AndFloat(vector3, number);
        auto output = TestMathFunction<FromVector3AndNumberNode>({ "Vector3: RGB", "Number: A" }, { Datum::CreateInitializedCopy(vector3), Datum::CreateInitializedCopy(number) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // FromVector4
        auto vector4 = Vector4Type(0.1, 0.2, 0.3, 0.4);
        auto result = ColorType(vector4);
        auto output = TestMathFunction<FromVector4Node>({ "Vector4: RGBA" }, { Datum::CreateInitializedCopy(vector4) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }
#endif

    { // GammaToLinear
        auto result = a.GammaToLinear();
        auto output = TestMathFunction<GammaToLinearNode>({ "Color: Source" }, { Datum::CreateInitializedCopy(a) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // IsClose
        {
            auto result = a.IsClose(a);
            auto output = TestMathFunction<IsCloseNode>({ "Color: A", "Color: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(0.1f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_TRUE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }

        {
            auto result = b.IsClose(a);
            auto output = TestMathFunction<IsCloseNode>({ "Color: A", "Color: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(0.1f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            EXPECT_FALSE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }
    }

    { // IsZero
        {
            auto zero = ColorType::CreateZero();
            auto result = zero.IsZero();
            auto output = TestMathFunction<IsZeroNode>({ "Color: Source",  "Number: Tolerance" }, { Datum::CreateInitializedCopy(zero), Datum::CreateInitializedCopy(0.1f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_TRUE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }

        {
            auto result = a.IsZero();
            auto output = TestMathFunction<IsZeroNode>({ "Color: Source", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(0.1f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            EXPECT_FALSE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }
    }

    { // LinearToGamma
        auto result = a.LinearToGamma();
        auto output = TestMathFunction<LinearToGammaNode>({ "Color: Source" }, { Datum::CreateInitializedCopy(a) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // MultiplyByColor
        auto result = a * b;
        auto output = TestMathFunction<MultiplyByColorNode>({ "Color: A", "Color: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // MultiplyByNumber
        auto result = a * ToVectorFloat(number);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Color: Source", "Number: Multiplier" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(number) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // Negate
        auto result = -a;
        auto output = TestMathFunction<NegateNode>({ "Color: Source" }, { Datum::CreateInitializedCopy(a) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // One
        auto source = ColorType();
        auto result = ColorType::CreateOne();
        auto output = TestMathFunction<OneNode>({}, {}, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    { // Subtract
        auto result = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Color: A", "Color: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Color" }, { Datum::CreateInitializedCopy(ColorType()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<ColorType>()));
    }

    // Missing:
    // ModRNode
    // ModGNode
    // ModBNode
    // ModANode
}

TEST_F(ScriptCanvasTestFixture, CrcNodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::CrcNodes;

    { // FromString
        Data::StringType value = "Test";
        auto result = Data::CRCType(value.data());
        auto output = TestMathFunction<FromStringNode>({ "String: Value" }, { Datum::CreateInitializedCopy(value) },
        { "Result: CRC" }, { Datum::CreateInitializedCopy(Data::CRCType()) });
        EXPECT_EQ(result, *output[0].GetAs<Data::CRCType>());
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // FromNumber
        AZ::u32 value = 0x784dd132;
        auto result = Data::CRCType(value);
        auto output = TestMathFunction<FromNumberNode>({ "Number: Value" }, { Datum::CreateInitializedCopy(static_cast<AZ::u64>(value)) },
        { "Result: CRC" }, { Datum::CreateInitializedCopy(Data::CRCType()) });
        EXPECT_EQ(result, *output[0].GetAs<Data::CRCType>());
    }

    { // GetNumber
        auto source = Data::CRCType("Test");
        auto result = source.operator AZ::u32();
        auto output = TestMathFunction<GetNumberNode>({ "CRC: Value", }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(Data::NumberType()) });
        EXPECT_EQ(result, *output[0].GetAs<Data::NumberType>());
    }
#endif
}

TEST_F(ScriptCanvasTestFixture, AABBNodes)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
    using namespace ScriptCanvas::AABBNodes;
    using namespace ScriptCanvas::Data;

    const Vector3Type outsideMax(222);
    const Vector3Type min3(-111, -111, -111);
    const Vector3Type max3(111, 111, 111);

    const Vector3Type minHalf3(min3 * 0.5f);
    const Vector3Type maxHalf3(max3 * 0.5f);

    auto IsClose = [](const AABBType& lhs, const AABBType& rhs)->bool { return lhs.GetMin().IsClose(rhs.GetMin()) && lhs.GetMax().IsClose(rhs.GetMax()); };

    { // AddAabb
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<AddAABBNode>({ "AABB: A", "AABB: B" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(source) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(Data::AABBType()) });
        source.AddAabb(source);
        EXPECT_TRUE(IsClose(source, *output[0].GetAs<AABBType>()));
    }

    { // AddPoint
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<AddPointNode>({ "AABB: Source", "Vector3: Point" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(outsideMax) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(Data::AABBType()) });
        source.AddPoint(outsideMax);
        EXPECT_TRUE(IsClose(source, *output[0].GetAs<AABBType>()));
    }

    { // ApplyTransform
        auto transform = TransformType::CreateFromColumns(Vector3Type(1, 2, 3), Vector3Type(1, 2, 3), Vector3Type(1, 2, 3), Vector3Type(4, 4, 4));
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<ApplyTransformNode>({ "AABB: Source", "Transform: Transform" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(transform) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(Data::AABBType()) });
        source.ApplyTransform(transform);
        EXPECT_TRUE(IsClose(source, *output[0].GetAs<AABBType>()));
    }

    { // Center
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<CenterNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        auto result = source.GetCenter();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // Clamp
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto sourceHalf = AABBType::CreateFromMinMax(minHalf3, maxHalf3);
        auto output = TestMathFunction<ClampNode>({ "AABB: Source", "AABB: Clamp" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(sourceHalf) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        auto result = source.GetClamped(sourceHalf);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // ContainsAABB
        auto bigger = AABBType::CreateFromMinMax(min3, max3);
        auto smaller = AABBType::CreateFromMinMax(minHalf3, maxHalf3);
        auto output = TestMathFunction<ContainsAABBNode>({ "AABB: Source", "AABB: Candidate" }, { Datum::CreateInitializedCopy(bigger), Datum::CreateInitializedCopy(smaller) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(BooleanType()) });
        auto result = bigger.Contains(smaller);
        EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        EXPECT_TRUE(result);
    }

    { // ContainsVector3Node
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto contained = Vector3Type::CreateZero();
        auto NOTcontained = Vector3Type(-10000, -10000, -10000);

        {
            auto output = TestMathFunction<ContainsVector3Node>({ "AABB: Source", "Vector3: Candidate" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(contained) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            auto result = source.Contains(contained);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
            EXPECT_TRUE(result);
        }
        {
            auto output = TestMathFunction<ContainsVector3Node>({ "AABB: Source", "Vector3: Candidate" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(NOTcontained) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            auto result = source.Contains(NOTcontained);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
            EXPECT_FALSE(result);
        }
    }

    { // DepthNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<DepthNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        float result = source.GetDepth();
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // DistanceNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto point = Vector3Type(100, 3000, 15879);
        auto output = TestMathFunction<DistanceNode>({ "AABB: Source", "Vector3: Point" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(point) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        float result = source.GetDistance(point);
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // ExpandNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto delta = Vector3Type(10, 20, 30);
        auto output = TestMathFunction<ExpandNode>({ "AABB: Source", "Vector3: Delta" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(delta) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        source.Expand(delta);
        EXPECT_TRUE(IsClose(source, *output[0].GetAs<AABBType>()));
    }

    { // ExtentsNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<ExtentsNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        auto result = source.GetExtents();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // FromCenterHalfExtentsNode
        auto center = Vector3Type(1, 2, 3);
        auto extents = Vector3Type(9, 8, 7);
        auto output = TestMathFunction<FromCenterHalfExtentsNode>({ "Vector3: Center", "Vector3: HalfExtents" }, { Datum::CreateInitializedCopy(center), Datum::CreateInitializedCopy(extents) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        auto result = AABBType::CreateCenterHalfExtents(center, extents);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // FromCenterRadiusNode
        auto center = Vector3Type(1, 2, 3);
        auto radius = 33.0f;
        auto output = TestMathFunction<FromCenterRadiusNode>({ "Vector3: Center", "Number: Radius" }, { Datum::CreateInitializedCopy(center), Datum::CreateInitializedCopy(radius) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        auto result = AABBType::CreateCenterRadius(center, radius);
        auto output0 = *output[0].GetAs<AABBType>();
        EXPECT_TRUE(IsClose(result, output0));
    }

    { // FromMinMaxNode
        auto result = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<FromMinMaxNode>({ "Vector3: Min", "Vector3: Max" }, { Datum::CreateInitializedCopy(min3), Datum::CreateInitializedCopy(max3) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // FromOBBNode
        auto source = OBBType::CreateFromAabb(AABBType::CreateFromMinMax(min3, max3));
        auto output = TestMathFunction<FromOBBNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        auto result = AABBType::CreateFromObb(source);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // FromPointNode
        auto source = Vector3Type(3, 2, 1);
        auto output = TestMathFunction<FromPointNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        auto result = AABBType::CreateFromPoint(source);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // IsFiniteNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<IsFiniteNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(BooleanType()) });
        auto result = source.IsFinite();
        EXPECT_TRUE(result);
        EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
    }

    { // IsValidNode
        {
            auto source = AABBType::CreateFromMinMax(min3, max3);
            auto output = TestMathFunction<IsValidNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(BooleanType()) });
            auto result = source.IsValid();
            EXPECT_TRUE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }

        {
            auto source = AABBType::CreateFromMinMax(min3, max3);
            source.SetMin(max3);
            source.SetMax(min3);
            auto output = TestMathFunction<IsValidNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(BooleanType()) });
            auto result = source.IsValid();
            EXPECT_FALSE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }
    }

    { // LengthNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<LengthNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        float result = source.GetHeight();
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // NullNode
        auto output = TestMathFunction<NullNode>({}, {}, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        auto result = AABBType::CreateNull();
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // OverlapsNode
        {
            auto a = AABBType::CreateFromMinMax(min3, max3);
            auto b = AABBType::CreateFromMinMax(min3 + Vector3Type(5, 5, 5), max3 + Vector3Type(5, 5, 5));
            auto output = TestMathFunction<OverlapsNode>({ "AABB: A", "AABB: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            auto result = a.Overlaps(b);
            EXPECT_TRUE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }

        {
            auto a = AABBType::CreateFromMinMax(min3, max3);
            auto b = AABBType::CreateFromMinMax(min3 + Vector3Type(300, 300, 300), max3 + Vector3Type(300, 300, 300));
            auto output = TestMathFunction<OverlapsNode>({ "AABB: A", "AABB: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            auto result = a.Overlaps(b);
            EXPECT_FALSE(result);
            EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
        }
    }

    { // SurfaceAreaNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<SurfaceAreaNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        float result = source.GetSurfaceArea();
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // GetMaxNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<GetMaxNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        auto result = source.GetMax();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // GetMinNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<GetMinNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        auto result = source.GetMin();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // ToSphereNode

        auto source = AABBType::CreateFromMinMax(min3 + Vector3Type(5, 5, 5), max3 + Vector3Type(5, 5, 5));
        auto output = TestMathFunction<ToSphereNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Center: Vector3", "Radius: Number" }, { Datum::CreateInitializedCopy(Vector3Type()), Datum::CreateInitializedCopy(NumberType()) });
        Vector3Type center;
        AZ::VectorFloat radiusVF;
        source.GetAsSphere(center, radiusVF);
        EXPECT_TRUE(center.IsClose(*output[0].GetAs<Vector3Type>()));
        float radius = radiusVF;
        EXPECT_FLOAT_EQ(radius, aznumeric_caster(*output[1].GetAs<NumberType>()));
    }

    { // TranslateNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto translation = Vector3Type(25, 30, -40);
        auto output = TestMathFunction<TranslateNode>({ "AABB: Source", "Vector3: Translation" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(translation) }, { "Result: AABB" }, { Datum::CreateInitializedCopy(AABBType()) });
        auto result = source.GetTranslated(translation);
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<AABBType>()));
    }

    { // WidthNode
        auto source = AABBType::CreateFromMinMax(min3, max3);
        auto output = TestMathFunction<WidthNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        float result = source.GetWidth();
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }
}

TEST_F(ScriptCanvasTestFixture, Matrix3x3Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Matrix3x3Nodes;

    {   // Add
        auto a(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne()));
        auto b(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne()));
        auto c = a + b;
        auto output = TestMathFunction<AddNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    {   // DivideByNumber
        auto a(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type(3.0, 3.0, 3.0), Data::Vector3Type(3.0, 3.0, 3.0), Data::Vector3Type(3.0, 3.0, 3.0)));
        Data::NumberType b(3.0);
        auto result = a / Data::ToVectorFloat(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Matrix3x3: Source", "Number: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromColumns
        auto col1 = Data::Vector3Type(1.0, 2.0, 3.0);
        auto col2 = Data::Vector3Type(4.0, 5.0, 6.0);
        auto col3 = Data::Vector3Type(7.0, 8.0, 9.0);
        auto result(Data::Matrix3x3Type::CreateFromColumns(col1, col2, col3));
        auto output = TestMathFunction<FromColumnsNode>({ "Vector3: Column1", "Vector3: Column2", "Vector3: Column3" }, { Datum::CreateInitializedCopy(col1), Datum::CreateInitializedCopy(col2), Datum::CreateInitializedCopy(col3) },
        { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromCrossProduct
        auto source = Data::Vector3Type(1.0, -1.0, 0.0);
        auto result(Data::Matrix3x3Type::CreateCrossProduct(source));
        auto output = TestMathFunction<FromCrossProductNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromDiagonal
        auto source = Data::Vector3Type(1.0, 1.0, 1.0);
        auto result(Data::Matrix3x3Type::CreateDiagonal(source));
        auto output = TestMathFunction<FromDiagonalNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromMatrix4x4
        auto row1 = Data::Vector4Type(1.0, 2.0, 3.0, 10.0);
        auto row2 = Data::Vector4Type(4.0, 5.0, 6.0, 20.0);
        auto row3 = Data::Vector4Type(7.0, 8.0, 9.0, -30.0);
        auto row4 = Data::Vector4Type(-75.454, 2.5419, -102343435.72, 5587981.54);
        auto source = Data::Matrix4x4Type::CreateFromRows(row1, row2, row3, row4);
        auto result(Data::Matrix3x3Type::CreateFromMatrix4x4(source));
        auto output = TestMathFunction<FromMatrix4x4Node>({ "Matrix4x4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromQuaternion
        auto rotation = Data::RotationType(1.0, 2.0, 3.0, 4.0);
        auto result(Data::Matrix3x3Type::CreateFromQuaternion(rotation));
        auto output = TestMathFunction<FromQuaternionNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(rotation) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromRotationXDegrees
      // FromRotationXRadians
        auto degrees = 45.0f;
        auto resultDegrees(Data::Matrix3x3Type::CreateRotationX(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationXDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(degrees) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromRotationYDegrees
        auto degrees = 30.0f;
        auto resultDegrees(Data::Matrix3x3Type::CreateRotationY(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationYDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(degrees) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromRotationZDegrees
        auto degrees = 60.0f;
        auto resultDegrees(Data::Matrix3x3Type::CreateRotationZ(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationZDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(degrees) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromRows
        auto row1 = Data::Vector3Type(1.0, 2.0, 3.0);
        auto row2 = Data::Vector3Type(4.0, 5.0, 6.0);
        auto row3 = Data::Vector3Type(7.0, 8.0, 9.0);
        auto result(Data::Matrix3x3Type::CreateFromRows(row1, row2, row3));
        auto output = TestMathFunction<FromRowsNode>({ "Vector3: Row1", "Vector3: Row2", "Vector3: Row3" }, { Datum::CreateInitializedCopy(row1), Datum::CreateInitializedCopy(row2), Datum::CreateInitializedCopy(row3) },
        { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromScale
        auto scale = Data::Vector3Type(2.0, 2.0, 2.0);
        auto result(Data::Matrix3x3Type::CreateScale(scale));
        auto output = TestMathFunction<FromScaleNode>({ "Vector3: Scale" }, { Datum::CreateInitializedCopy(scale) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // FromTransform
        auto transform = Data::TransformType::CreateFromRows(
            Data::Vector4Type(1.0, 0.0, 0.0, 0.0),
            Data::Vector4Type(0.0, 1.0, 0.0, 0.0),
            Data::Vector4Type(0.0, 0.0, 1.0, 0.0));
        auto result(Data::Matrix3x3Type::CreateFromTransform(transform));
        auto output = TestMathFunction<FromTransformNode>({ "Transform: Transform" }, { Datum::CreateInitializedCopy(transform) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // Invert
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 1.0, 26.0),
            Data::Vector3Type(7.0, -8.0, 1.0));
        auto result(source.GetInverseFull());
        auto output = TestMathFunction<InvertNode>({ "Matrix3x3: Source", }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // IsCloseNode
        auto a(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne()));
        auto b(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type(2, 2, 2), Data::Vector3Type(2, 2, 2), Data::Vector3Type(2, 2, 2)));
        auto c(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type(1.001, 1.001, 1.001), Data::Vector3Type(1.001, 1.001, 1.001), Data::Vector3Type(1.001, 1.001, 1.001)));

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, Data::ToVectorFloat(2.1));
        output = TestMathFunction<IsCloseNode>({ "Matrix3x3: A", "Matrix3x3: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(2.1) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(c) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, Data::ToVectorFloat(0.9));
        output = TestMathFunction<IsCloseNode>({ "Matrix3x3: A", "Matrix3x3: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(0.9) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, std::numeric_limits<float>::infinity(), 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        auto result = source.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Boolean", }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsOrthogonal
        auto source(Data::Matrix3x3Type::CreateDiagonal(Data::Vector3Type(2.0, 2.0, 2.0)));
        auto result = source.IsOrthogonal();
        auto output = TestMathFunction<IsOrthogonalNode>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    {   // MultiplyByNumber
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        Data::NumberType scalar(3.0);
        auto result = source * Data::ToVectorFloat(scalar);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Matrix3x3: Source", "Number: Multiplier" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(scalar) }, { "Result: Matrix3x3" },
        { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    {  // MultiplyByMatrix
        auto a(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        auto b(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(-1.0, -2.0, 13.0),
            Data::Vector3Type(14.0, 15.0, -6.0),
            Data::Vector3Type(17.0, -8.0, 19.0)));
        auto result = a * b;
        auto output = TestMathFunction<MultiplyByMatrixNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) },
        { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    {  // MultiplyByVector
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        Data::Vector3Type vector3(2.0);
        auto result = source * vector3;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Matrix3x3: Source", "Vector3: Vector" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(vector3) },
        { "Result: Vector3" }, { Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Orthogonalize
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        auto result = source.GetOrthogonalized();
        auto output = TestMathFunction<OrthogonalizeNode>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    {   // Subtract
        auto a(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type(2.0, -8.5, -6.12), Data::Vector3Type(0.0, 0.0, 2.3), Data::Vector3Type(17.2, 4.533, 16.33492)));
        auto b(Data::Matrix3x3Type::CreateFromRows(Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne(), Data::Vector3Type::CreateOne()));
        auto c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Matrix3x3: A", "Matrix3x3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // ToAdjugate
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 0.0, 0.0),
            Data::Vector3Type(0.0, 1.0, 0.0),
            Data::Vector3Type(0.0, 0.0, 1.0));
        auto result(source.GetAdjugate());
        auto output = TestMathFunction<ToAdjugateNode>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // GetColumnNode
        auto source = Data::Matrix3x3Type::CreateFromColumns(
            Data::Vector3Type(1.0, 2.0, 4.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0));
        auto result(source.GetColumn(1));
        auto output = TestMathFunction<GetColumnNode>({ "Matrix3x3: Source", "Number: Column" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(1) },
        { "Result: Vector3" }, { Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // GetColumns
        const Data::Vector3Type col1(1.0, 2.0, 3.0);
        const Data::Vector3Type col2(5.0, 16.5, 21.2);
        const Data::Vector3Type col3(-44.4, -72.1, 72.4);
        auto source = Data::Matrix3x3Type::CreateFromColumns(col1, col2, col3);
        auto output = TestMathFunction<GetColumnsNode>(
        { "Matrix3x3: Source" },
        { Datum::CreateInitializedCopy(source) },
        { "Column1: Vector3", "Column2: Vector3", "Column3: Vector3" },
        { Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(col1.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(col2.IsClose(*output[1].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(col3.IsClose(*output[2].GetAs<Data::Vector3Type>()));
    }

    { // ToDeterminant
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 0.0, 0.0),
            Data::Vector3Type(0.0, 1.0, 0.0),
            Data::Vector3Type(0.0, 0.0, 1.0));
        auto result(source.GetDeterminant());
        // Test naming single output slots: The GetDeterminant function names it's result slot "Determinant"
        auto output = TestMathFunction<ToDeterminantNode>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Determinant: Number" }, { Datum::CreateInitializedCopy(Data::NumberType()) });
        EXPECT_TRUE(result.IsClose(Data::ToVectorFloat(*output[0].GetAs<Data::NumberType>())));
    }

    { // GetDiagonal
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0));
        auto result(source.GetDiagonal());
        auto output = TestMathFunction<GetDiagonalNode>({ "Matrix3x3: Source", }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // GetElement
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 4.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0));
        auto result(source.GetElement(2, 1));
        auto output = TestMathFunction<GetElementNode>({ "Matrix3x3: Source", "Number: Row", "Number: Column" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(2), Datum::CreateInitializedCopy(1) },
        { "Result: Number" }, { Datum::CreateInitializedCopy(Data::NumberType()) });
        EXPECT_TRUE(result.IsClose(Data::ToVectorFloat(*output[0].GetAs<Data::NumberType>())));
    }

    { // GetRow
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 4.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0));
        auto result(source.GetRow(0));
        auto output = TestMathFunction<GetRowNode>({ "Matrix3x3: Source", "Number: Row" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(0) },
        { "Result: Vector3" }, { Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // GetRows
        const Data::Vector3Type row1(1.0, 2.0, 3.0);
        const Data::Vector3Type row2(5.0, 16.5, 21.2);
        const Data::Vector3Type row3(-44.4, -72.1, 72.4);
        auto source = Data::Matrix3x3Type::CreateFromRows(row1, row2, row3);
        auto output = TestMathFunction<GetRowsNode>(
        { "Matrix3x3: Source" },
        { Datum::CreateInitializedCopy(source) },
        { "Row1: Vector3", "Row2: Vector3", "Row3: Vector3" },
        { Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(row1.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(row2.IsClose(*output[1].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(row3.IsClose(*output[2].GetAs<Data::Vector3Type>()));
    }

    { // ToScale
        auto source = Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 1.0, -6.0),
            Data::Vector3Type(7.0, -8.0, 1.0));
        auto result(source.RetrieveScale());
        auto output = TestMathFunction<ToScaleNode>({ "Matrix3x3: Source", }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Transpose
        auto source(Data::Matrix3x3Type::CreateFromRows(
            Data::Vector3Type(1.0, 2.0, 3.0),
            Data::Vector3Type(4.0, 5.0, 6.0),
            Data::Vector3Type(7.0, 8.0, 9.0)));
        auto result = source.GetTranspose();
        auto output = TestMathFunction<TransposeNode>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }

    { // Zero
        auto result = Data::Matrix3x3Type::CreateZero();
        auto output = TestMathFunction<ZeroNode>({}, {}, { "Result: Matrix3x3" }, { Datum::CreateInitializedCopy(Data::Matrix3x3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix3x3Type>()));
    }
}

TEST_F(ScriptCanvasTestFixture, Matrix4x4Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Matrix4x4Nodes;

    { // FromColumns
        auto col1 = Data::Vector4Type(1.0, 2.0, 3.0, 4.0);
        auto col2 = Data::Vector4Type(5.0, 6.0, 7.0, 8.0);
        auto col3 = Data::Vector4Type(9.0, 10.0, 11.0, 12.0);
        auto col4 = Data::Vector4Type(13.0, 15.0, 15.0, 16.0);
        auto result(Data::Matrix4x4Type::CreateFromColumns(col1, col2, col3, col4));
        auto output = TestMathFunction<FromColumnsNode>({ "Vector4: Column1", "Vector4: Column2", "Vector4: Column3", "Vector4: Column4" },
        { Datum::CreateInitializedCopy(col1), Datum::CreateInitializedCopy(col2), Datum::CreateInitializedCopy(col3), Datum::CreateInitializedCopy(col4) },
        { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromDiagonal
        auto source = Data::Vector4Type(1.0, 1.0, 1.0, 1.0);
        auto result(Data::Matrix4x4Type::CreateDiagonal(source));
        auto output = TestMathFunction<FromDiagonalNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromMatrix3x3
        auto row1 = Data::Vector3Type(1.0, 2.0, 3.0);
        auto row2 = Data::Vector3Type(4.0, 5.0, 6.0);
        auto row3 = Data::Vector3Type(7.0, 8.0, 9.0);
        auto source = Data::Matrix3x3Type::CreateFromRows(row1, row2, row3);
        auto result(Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type::CreateFromVector3(source.GetRow(0)),
            Data::Vector4Type::CreateFromVector3(source.GetRow(1)),
            Data::Vector4Type::CreateFromVector3(source.GetRow(2)),
            Data::Vector4Type(0.0, 0.0, 0.0, 1.0)));
        auto output = TestMathFunction<FromMatrix3x3Node>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromQuaternion
        auto rotation = Data::RotationType(1.0, 2.0, 3.0, 4.0);
        auto result(Data::Matrix4x4Type::CreateFromQuaternion(rotation));
        auto output = TestMathFunction<FromQuaternionNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(rotation) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromQuaternionAndTranslation
        auto rotation = Data::RotationType(1.0, 2.0, 3.0, 4.0);
        auto translation = Data::Vector3Type(10.0, -4.5, -16.10);
        auto result = Data::Matrix4x4Type::CreateFromQuaternionAndTranslation(rotation, translation);
        auto output = TestMathFunction<FromQuaternionAndTranslationNode>({ "Rotation: Rotation", "Vector3: Translation" }, { Datum::CreateInitializedCopy(rotation), Datum::CreateInitializedCopy(translation) },
        { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // FromProjection
        const auto verticalFov = Data::NumberType(4.0 / 3.0 * AZ::Constants::HalfPi);
        const auto aspectRatio = Data::NumberType(16.0 / 9.0);
        const auto nearDist = Data::NumberType(0.1);
        const auto farDist = Data::NumberType(10.0);
        auto result = Data::Matrix4x4Type::CreateProjection(Data::ToVectorFloat(verticalFov), Data::ToVectorFloat(aspectRatio), Data::ToVectorFloat(nearDist), Data::ToVectorFloat(farDist));
        auto output = TestMathFunction<FromProjectionNode>({ "Number: Vertical FOV", "Number: Aspect Ratio", "Number: Near", "Number: Far" },
        { Datum::CreateInitializedCopy(verticalFov), Datum::CreateInitializedCopy(aspectRatio), Datum::CreateInitializedCopy(nearDist), Datum::CreateInitializedCopy(farDist) },
        { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromProjectionFov
        const auto verticalFov = Data::NumberType(4.0 / 3.0 * AZ::Constants::HalfPi);
        const auto horizontalFov = Data::NumberType(5.0 / 6.0 * AZ::Constants::HalfPi);
        const auto nearDist = Data::NumberType(0.1);
        const auto farDist = Data::NumberType(10.0);
        auto result = Data::Matrix4x4Type::CreateProjectionFov(Data::ToVectorFloat(verticalFov), Data::ToVectorFloat(horizontalFov), Data::ToVectorFloat(nearDist), Data::ToVectorFloat(farDist));
        auto output = TestMathFunction<FromProjectionFovNode>({ "Number: Vertical FOV", "Number: Horizontal FOV", "Number: Near", "Number: Far" },
        { Datum::CreateInitializedCopy(verticalFov), Datum::CreateInitializedCopy(horizontalFov), Datum::CreateInitializedCopy(nearDist), Datum::CreateInitializedCopy(farDist) },
        { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromProjectionVolume
        const auto left = Data::NumberType(-10.0);
        const auto right = Data::NumberType(10.0);
        const auto bottom = Data::NumberType(0.0);
        const auto top = Data::NumberType(12.0);
        const auto nearDist = Data::NumberType(0.1);
        const auto farDist = Data::NumberType(10.0);
        auto result = Data::Matrix4x4Type::CreateProjectionOffset(Data::ToVectorFloat(left), Data::ToVectorFloat(right), Data::ToVectorFloat(bottom), Data::ToVectorFloat(top), Data::ToVectorFloat(nearDist), Data::ToVectorFloat(farDist));
        auto output = TestMathFunction<FromProjectionVolumeNode>({ "Number: Left", "Number: Right", "Number: Bottom", "Number: Top", "Number: Near", "Number: Far" },
        { Datum::CreateInitializedCopy(left), Datum::CreateInitializedCopy(right), Datum::CreateInitializedCopy(bottom), Datum::CreateInitializedCopy(top),  Datum::CreateInitializedCopy(nearDist), Datum::CreateInitializedCopy(farDist) },
        { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }
#endif

    { // FromRotationXDegrees
        auto degrees = 45.0f;
        auto resultDegrees(Data::Matrix4x4Type::CreateRotationX(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationXDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(degrees) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromRotationYDegrees
        auto degrees = 30.0f;
        auto resultDegrees(Data::Matrix4x4Type::CreateRotationY(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationYDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(degrees) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromRotationZDegrees
        auto degrees = 60.0f;
        auto resultDegrees(Data::Matrix4x4Type::CreateRotationZ(AZ::DegToRad(degrees)));
        auto outputDegrees = TestMathFunction<FromRotationZDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(degrees) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(resultDegrees.IsClose(*outputDegrees[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromRows
        auto row1 = Data::Vector4Type(1.0, 2.0, 3.0, 4.0);
        auto row2 = Data::Vector4Type(5.0, 6.0, 7.0, 8.0);
        auto row3 = Data::Vector4Type(9.0, 10.0, 11.0, 12.0);
        auto row4 = Data::Vector4Type(13.0, 15.0, 15.0, 16.0);
        auto result(Data::Matrix4x4Type::CreateFromRows(row1, row2, row3, row4));
        auto output = TestMathFunction<FromRowsNode>({ "Vector4: Row1", "Vector4: Row2", "Vector4: Row3", "Vector4: Row4" },
        { Datum::CreateInitializedCopy(row1), Datum::CreateInitializedCopy(row2), Datum::CreateInitializedCopy(row3), Datum::CreateInitializedCopy(row4) },
        { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromScale
        auto scale = Data::Vector3Type(2.0, 2.0, 2.0);
        auto result(Data::Matrix4x4Type::CreateScale(scale));
        auto output = TestMathFunction<FromScaleNode>({ "Vector3: Scale" }, { Datum::CreateInitializedCopy(scale) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromTransform
        auto transform = Data::TransformType::CreateFromRows(
            Data::Vector4Type(1.0, 0.0, 0.0, 0.0),
            Data::Vector4Type(0.0, 1.0, 0.0, 0.0),
            Data::Vector4Type(0.0, 0.0, 1.0, 0.0));
        auto result = Data::Matrix4x4Type::CreateFromRows(transform.GetRow(0), transform.GetRow(1), transform.GetRow(2), Data::Vector4Type::CreateAxisW());
        auto output = TestMathFunction<FromTransformNode>({ "Transform: Transform" }, { Datum::CreateInitializedCopy(transform) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // FromTranslation
        auto source = Data::Vector3Type(1.0, -1.0, 15.0);
        auto result(Data::Matrix4x4Type::CreateTranslation(source));
        auto output = TestMathFunction<FromTranslationNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // Invert
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 21.2),
            Data::Vector4Type(4.0, 1.0, 26.0, -45.8),
            Data::Vector4Type(7.0, -8.0, 1.0, 73.3),
            Data::Vector4Type(27.5, 36.8, 0.8, 14.0));
        auto result(source.GetInverseFull());
        auto output = TestMathFunction<InvertNode>({ "Matrix4x4: Source", }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // IsCloseNode
        auto a(Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type::CreateOne(), Data::Vector4Type::CreateOne(), Data::Vector4Type::CreateOne(), Data::Vector4Type::CreateOne()));
        auto b(Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type(2.0, 2.0, 2.0, 2.0), Data::Vector4Type(2.0, 2.0, 2.0, 2.0), Data::Vector4Type(2.0, 2.0, 2.0, 2.0), Data::Vector4Type(2.0, 2.0, 2.0, 2.0)));
        auto c(Data::Matrix4x4Type::CreateFromRows(Data::Vector4Type(1.001, 1.001, 1.001, 1.001), Data::Vector4Type(1.001, 1.001, 1.001, 1.001), Data::Vector4Type(1.001, 1.001, 1.001, 1.001), Data::Vector4Type(1.001, 1.001, 1.001, 1.001)));

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Matrix4x4: A", "Matrix4x4: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, Data::ToVectorFloat(2.1));
        output = TestMathFunction<IsCloseNode>({ "Matrix4x4: A", "Matrix4x4: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(2.1) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Matrix4x4: A", "Matrix4x4: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(c) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, Data::ToVectorFloat(0.9));
        output = TestMathFunction<IsCloseNode>({ "Matrix4x4: A", "Matrix4x4: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(0.9) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 21.2),
            Data::Vector4Type(4.0, 1.0, 26.0, -45.8),
            Data::Vector4Type(7.0, -8.0, std::numeric_limits<float>::infinity(), 73.3),
            Data::Vector4Type(27.5, 36.8, 0.8, 14.0));
        auto result = source.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Matrix4x4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Boolean", }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    {  // MultiplyByMatrix
        auto a(Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 17.98)));
        auto b(Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(-1.0, -2.0, 13.0, -0.0),
            Data::Vector4Type(14.0, 15.0, -6.0, +0.0),
            Data::Vector4Type(17.0, -8.0, 19.0, 1.0),
            Data::Vector4Type(4.1, -7.6, -11.3, 1.0)));
        auto result = a * b;
        auto output = TestMathFunction<MultiplyByMatrixNode>({ "Matrix4x4: A", "Matrix4x4: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) },
        { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    {  // MultiplyByVector
        auto source(Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 17.98)));
        Data::Vector4Type vector4(3.0);
        auto result = source * vector4;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Matrix4x4: Source", "Vector4: Vector" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(vector4) },
        { "Result: Vector4" }, { Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetColumn
        auto source = Data::Matrix4x4Type::CreateFromColumns(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 40.0));
        auto result(source.GetColumn(3));
        auto output = TestMathFunction<GetColumnNode>({ "Matrix4x4: Source", "Number: Column" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(3) },
        { "Result: Vector4" }, { Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetColumns
        const Data::Vector4Type col1(1.0, 2.0, 3.0, 5.0);
        const Data::Vector4Type col2(5.0, 16.5, 21.2, -1.2);
        const Data::Vector4Type col3(-44.4, -72.1, 72.4, 6.5);
        const Data::Vector4Type col4(2.7, 17.65, 2.3, 13.3);
        auto source = Data::Matrix4x4Type::CreateFromColumns(col1, col2, col3, col4);
        auto output = TestMathFunction<GetColumnsNode>(
        { "Matrix4x4: Source" },
        { Datum::CreateInitializedCopy(source) },
        { "Column1: Vector4", "Column2: Vector4", "Column3: Vector4", "Column4: Vector4" },
        { Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(col1.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(col2.IsClose(*output[1].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(col3.IsClose(*output[2].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(col4.IsClose(*output[3].GetAs<Data::Vector4Type>()));
    }

    { // GetDiagonal
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(10.0, 1.0, 2.0, 3.0),
            Data::Vector4Type(24.0, 4.0, 5.0, 6.0),
            Data::Vector4Type(40.0, 7.0, 8.0, 9.0),
            Data::Vector4Type(30.0, 10.0, 11.0, 12.0));
        auto result(source.GetDiagonal());
        auto output = TestMathFunction<GetDiagonalNode>({ "Matrix4x4: Source", }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetElement
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 40.0));
        auto result(source.GetElement(1, 3));
        auto outputSuccess = TestMathFunction<GetElementNode>({ "Matrix4x4: Source", "Number: Row", "Number: Column" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(1), Datum::CreateInitializedCopy(3) },
        { "Result: Number" }, { Datum::CreateInitializedCopy(Data::NumberType()) });
        auto outputFailure = TestMathFunction<GetElementNode>({ "Matrix4x4: Source", "Number: Row", "Number: Column" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(2), Datum::CreateInitializedCopy(2) },
        { "Result: Number" }, { Datum::CreateInitializedCopy(Data::NumberType()) });
        EXPECT_TRUE(result.IsClose(Data::ToVectorFloat(*outputSuccess[0].GetAs<Data::NumberType>())));
        EXPECT_FALSE(result.IsClose(Data::ToVectorFloat(*outputFailure[0].GetAs<Data::NumberType>())));
    }

    { // GetRow
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 10.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 20.0),
            Data::Vector4Type(7.0, 8.0, 9.0, 30.0),
            Data::Vector4Type(3.5, 9.1, 1.0, 40.0));
        auto result(source.GetRow(0));
        auto output = TestMathFunction<GetRowNode>({ "Matrix4x4: Source", "Number: Row" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(0) },
        { "Result: Vector4" }, { Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetRows
        const Data::Vector4Type row1(1.0, 2.0, 3.0, 5.0);
        const Data::Vector4Type row2(5.0, 16.5, 21.2, -1.2);
        const Data::Vector4Type row3(-44.4, -72.1, 72.4, 6.5);
        const Data::Vector4Type row4(2.7, 17.65, 2.3, 13.3);
        auto source = Data::Matrix4x4Type::CreateFromRows(row1, row2, row3, row4);
        auto output = TestMathFunction<GetRowsNode>(
        { "Matrix4x4: Source" },
        { Datum::CreateInitializedCopy(source) },
        { "Row1: Vector4", "Row2: Vector4", "Row3: Vector4", "Row4: Vector4" },
        { Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()), Datum::CreateInitializedCopy(Data::Vector4Type::CreateZero()) });
        EXPECT_TRUE(row1.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(row2.IsClose(*output[1].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(row3.IsClose(*output[2].GetAs<Data::Vector4Type>()));
        EXPECT_TRUE(row4.IsClose(*output[3].GetAs<Data::Vector4Type>()));
    }

    { // ToScale
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 4.5),
            Data::Vector4Type(4.0, 1.0, -6.0, -1.0),
            Data::Vector4Type(7.0, -8.0, 1.0, 1.0),
            Data::Vector4Type(0.0, 0.0, 0.0, 1.0));
        auto result(source.RetrieveScale());
        auto output = TestMathFunction<ToScaleNode>({ "Matrix4x4: Source", }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Data::Vector3Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Transpose
        auto source = Data::Matrix4x4Type::CreateFromRows(
            Data::Vector4Type(1.0, 2.0, 3.0, 0.0),
            Data::Vector4Type(4.0, 5.0, 6.0, 6.0),
            Data::Vector4Type(7.0, 8.0, 9.0, -10.0),
            Data::Vector4Type(5.0, 8.0, 3.0, 1.0));
        auto result = source.GetTranspose();
        auto output = TestMathFunction<TransposeNode>({ "Matrix4x4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }

    { // Zero
        auto result = Data::Matrix4x4Type::CreateZero();
        auto output = TestMathFunction<ZeroNode>({}, {}, { "Result: Matrix4x4" }, { Datum::CreateInitializedCopy(Data::Matrix4x4Type::CreateZero()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Matrix4x4Type>()));
    }
}

TEST_F(ScriptCanvasTestFixture, OBBNodes)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
    using namespace ScriptCanvas::OBBNodes;
    using namespace ScriptCanvas::Data;

    const Vector3Type outsideMax(222);
    const Vector3Type min3(-111, -111, -111);
    const Vector3Type max3(111, 111, 111);

    const Vector3Type minHalf3(min3 * 0.5f);
    const Vector3Type maxHalf3(max3 * 0.5f);

    auto obbRotated = OBBType::CreateFromPositionAndAxes(Vector3Type(10, 20, 30), Vector3Type(1, 0, 0), 10, Vector3Type(0, 1, 0), 30, Vector3Type(0, 0, 1), 20);
    auto transform = TransformType::CreateFromQuaternion(RotationType::CreateFromAxisAngle(Vector3Type(1, 1, 1).GetNormalized(), ToVectorFloat(AZ::DegToRad(30))));
    obbRotated = transform * obbRotated;

    auto IsClose = [](const OBBType& lhs, const OBBType& rhs)->bool
    {
        return lhs.GetAxisX().IsClose(rhs.GetAxisX())
            && lhs.GetAxisY().IsClose(rhs.GetAxisY())
            && lhs.GetAxisZ().IsClose(rhs.GetAxisZ())
            && lhs.GetPosition().IsClose(rhs.GetPosition());
    };

    {// FromAabbNode
        auto source = AABBType::CreateFromMinMax(Vector3Type(-1, -2, -3), Vector3Type(1, 2, 3));
        auto output = TestMathFunction<FromAabbNode>({ "AABB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: OBB" }, { Datum::CreateInitializedCopy(OBBType()) });
        auto result = OBBType::CreateFromAabb(source);
        EXPECT_TRUE(IsClose(result, (*output[0].GetAs<OBBType>())));
    }

    {// FromPositionAndAxesNode
        auto output = TestMathFunction<FromPositionAndAxesNode>
            ({ "Vector3: Position", "Vector3: X-Axis", "Number: X Half-Length", "Vector3: Y-Axis", "Number: Y Half-Length", "Vector3: Z-Axis", "Number: Z Half-Length" }
                , { Datum::CreateInitializedCopy(obbRotated.GetPosition()), Datum::CreateInitializedCopy(obbRotated.GetAxisX()), Datum::CreateInitializedCopy(obbRotated.GetHalfLengthX()), Datum::CreateInitializedCopy(obbRotated.GetAxisY()), Datum::CreateInitializedCopy(obbRotated.GetHalfLengthY()), Datum::CreateInitializedCopy(obbRotated.GetAxisZ()), Datum::CreateInitializedCopy(obbRotated.GetHalfLengthZ()) }
                , { "Result: OBB" }
        , { Datum::CreateInitializedCopy(OBBType()) });
        auto result = obbRotated;
        EXPECT_TRUE(IsClose(result, (*output[0].GetAs<OBBType>())));
    }

    {// GetAxisXNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetAxisXNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        auto result = source.GetAxisX();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    {// GetAxisYNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetAxisYNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        auto result = source.GetAxisY();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    {// GetAxisZNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetAxisZNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        auto result = source.GetAxisZ();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {// GetHalfLengthXNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetHalfLengthXNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        float result = source.GetHalfLengthX();
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    {// GetHalfLengthYNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetHalfLengthYNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        float result = source.GetHalfLengthY();
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    {// GetHalfLengthZNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetHalfLengthZNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(NumberType()) });
        float result = source.GetHalfLengthZ();
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }
#endif

    {// GetPositionNode
        auto source = obbRotated;
        auto output = TestMathFunction<GetPositionNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        auto result = obbRotated.GetPosition();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    {// IsFiniteNode
        auto source = obbRotated;
        auto output = TestMathFunction<IsFiniteNode>({ "OBB: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(BooleanType()) });
        auto result = source.IsFinite();
        EXPECT_TRUE(result);
        EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
    }

}

TEST_F(ScriptCanvasTestFixture, PlaneNodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::PlaneNodes;
    using namespace ScriptCanvas::Data;

    const PlaneType source = PlaneType::CreateFromNormalAndDistance(Vector3Type::CreateOne().GetNormalized(), 12.0);
    const Vector4Type sourceVec4 = source.GetPlaneEquationCoefficients();

    const Vector3Type point(33, 66, 99);
    const Vector3Type normal(Vector3Type(1, -2, 3).GetNormalized());
    const float A = normal.GetX();
    const float B = normal.GetY();
    const float C = normal.GetZ();
    const float D = 13.0f;

    auto IsClose = [](PlaneType lhs, PlaneType rhs) { return lhs.GetPlaneEquationCoefficients().IsClose(rhs.GetPlaneEquationCoefficients()); };

    { // CreateFromCoefficientsNode
        auto result = PlaneType::CreateFromCoefficients(A, B, C, D);
        auto output = TestMathFunction<FromCoefficientsNode>({ "Number: A", "Number: B", "Number: C", "Number: D" }, { Datum::CreateInitializedCopy(A),Datum::CreateInitializedCopy(B),Datum::CreateInitializedCopy(C),Datum::CreateInitializedCopy(D) }, { "Result: Plane" }, { Datum::CreateInitializedCopy(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }

    { // CreateFromNormalAndDistanceNode
        auto result = PlaneType::CreateFromNormalAndDistance(normal, D);
        auto output = TestMathFunction<FromNormalAndDistanceNode>({ "Vector3: Normal", "Number: Distance" }, { Datum::CreateInitializedCopy(normal), Datum::CreateInitializedCopy(D) }, { "Result: Plane" }, { Datum::CreateInitializedCopy(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }

    { // CreateFromNormalAndPointNode
        auto result = PlaneType::CreateFromNormalAndPoint(normal, point);
        auto output = TestMathFunction<FromNormalAndPointNode>({ "Vector3: Normal", "Vector3: Point" }, { Datum::CreateInitializedCopy(normal), Datum::CreateInitializedCopy(point) }, { "Result: Plane" }, { Datum::CreateInitializedCopy(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }

    { // DistanceToPointNode
        auto result = source.GetPointDist(point);
        auto output = TestMathFunction<DistanceToPointNode>({ "Plane: Source", "Vector3: Point" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(point) }, { "Result: Number" }, { Datum::CreateInitializedCopy(-1.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // IsFiniteNode
        auto result = source.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Plane: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_TRUE(result);
        EXPECT_EQ(result, *output[0].GetAs<BooleanType>());
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // ModDistanceNode
        auto result = source;
        result.SetDistance(D);
        auto output = TestMathFunction<ModDistanceNode>({ "Plane: Source", "Number: Distance" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(D) }, { "Result: Plane" }, { Datum::CreateInitializedCopy(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }

    { // ModNormalNode
        auto result = source;
        result.SetNormal(normal);
        auto output = TestMathFunction<ModNormalNode>({ "Plane: Source", "Vector3: Normal" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(normal) }, { "Result: Plane" }, { Datum::CreateInitializedCopy(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }
#endif

    { // GetDistanceNode
        auto result = source.GetDistance();
        auto output = TestMathFunction<GetDistanceNode>({ "Plane: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(-1) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }

    { // GetNormalNode
        auto result = source.GetNormal();
        auto output = TestMathFunction<GetNormalNode>({ "Plane: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type(0, 0, 0)) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // GetPlaneEquationCoefficientsNode
        auto result = source.GetPlaneEquationCoefficients();
        auto output = TestMathFunction<GetPlaneEquationCoefficientsNode>({ "Plane: Source" }, { Datum::CreateInitializedCopy(source) }, { "A: Number", "B: Number", "C: Number", "D: Number" }, { Datum::CreateInitializedCopy(0),Datum::CreateInitializedCopy(0),Datum::CreateInitializedCopy(0),Datum::CreateInitializedCopy(0) });

        for (int i = 0; i < 4; ++i)
        {
            EXPECT_FLOAT_EQ(result.GetElement(i), aznumeric_caster(*output[i].GetAs<Data::NumberType>()));
        }
    }

    { // GetProjectedNode
        auto result = source.GetProjected(point);
        auto output = TestMathFunction<ProjectNode>({ "Plane: Source", "Vector3: Point" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(point) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(Vector3Type()) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // TransformNode
        TransformType transform = TransformType::CreateFromQuaternionAndTranslation(RotationType::CreateFromAxisAngle(Vector3Type(1, 1, 1).GetNormalized(), AZ::DegToRad(30)), Vector3Type(-100, 50, -25));
        auto result = source.GetTransform(transform);
        auto output = TestMathFunction<TransformNode>({ "Plane: Source", "Transform: Transform" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(transform) }, { "Result: Plane" }, { Datum::CreateInitializedCopy(PlaneType()) });
        EXPECT_TRUE(IsClose(result, *output[0].GetAs<PlaneType>()));
    }
}

TEST_F(ScriptCanvasTestFixture, TransformNodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::TransformNodes;
    using namespace ScriptCanvas::Data;

    const Vector3Type vector3Zero(0, 0, 0);
    const Vector3Type zero3(0, 0, 0);
    const Vector3Type vector3One(1, 1, 1);
    const Vector3Type xPos(1, 0, 0);
    const Vector3Type yPos(0, 1, 0);
    const Vector3Type zPos(0, 0, 1);
    const Vector3Type position(-10.1, 0.1, 10.1);
    const Vector3Type scale(-.66, .33, .66);

    const Vector4Type r0(1, 0, 0, -.5);
    const Vector4Type r1(0, 1, 0, 0.0);
    const Vector4Type r2(0, 0, 1, 0.5);
    const Vector4Type zero4(0, 0, 0, 0);

    const Vector3Type xPosish(1, .1, 0);
    const Vector3Type yPosish(0, 1, .1);
    const Vector3Type zPosish(.1, 0, 1);

    const Matrix3x3Type matrix3x3One(Matrix3x3Type::CreateFromValue(1));

    const RotationType rotationOne(RotationType::CreateRotationZ(1));

    const TransformType identity(TransformType::CreateIdentity());
    const TransformType zero(TransformType::CreateZero());
    const TransformType invertable(TransformType::CreateFromQuaternionAndTranslation(rotationOne, position));
    const TransformType notOrthogonal(TransformType::CreateFromValue(3));

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // ExtractScale
        auto source = TransformType::CreateScale(Vector3Type(-0.5f, .5f, 1.5f));
        auto extracted = source;
        auto scale = extracted.ExtractScale();
        auto output = TestMathFunction<ExtractScaleNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(source) }, { "Scale: Vector3", "Extracted: Transform" }, { Datum::CreateInitializedCopy(vector3Zero), Datum::CreateInitializedCopy(identity) });

        auto scaleOutput = *output[0].GetAs<Vector3Type>();
        auto extractedOutput = *output[1].GetAs<TransformType>();
        EXPECT_TRUE(scale.IsClose(scaleOutput));
        EXPECT_TRUE(extracted.IsClose(extractedOutput));
    }
#endif

    {
      // FromColumnsNode
        auto result = TransformType::CreateFromColumns(xPos, yPos, zPos, position);
        auto output0 = TestMathFunction<FromColumnsNode>({ "Vector3: Column 0", "Vector3: Column 1", "Vector3: Column 2", "Vector3: Column 3", }, { Datum::CreateInitializedCopy(xPos), Datum::CreateInitializedCopy(yPos), Datum::CreateInitializedCopy(zPos), Datum::CreateInitializedCopy(position) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output0[0].GetAs<TransformType>()));
    }

    { // FromDiagonalNode
        auto output = TestMathFunction<FromDiagonalNode>({ "Vector3: Diagonal" }, { Datum::CreateInitializedCopy(vector3One) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateDiagonal(vector3One);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromMatrix3x3Node
        auto output = TestMathFunction<FromMatrix3x3Node>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(matrix3x3One) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateFromMatrix3x3(matrix3x3One);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromMatrix3x3AndTranslationNode
        auto output = TestMathFunction<FromMatrix3x3AndTranslationNode>({ "Matrix3x3: Matrix", "Vector3: Translation" }, { Datum::CreateInitializedCopy(matrix3x3One), Datum::CreateInitializedCopy(position) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateFromMatrix3x3AndTranslation(matrix3x3One, position);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromRotationNode
        auto output = TestMathFunction<FromRotationNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(rotationOne) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateFromQuaternion(rotationOne);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromRotationAndTranslationNode
        auto output = TestMathFunction<FromRotationAndTranslationNode>({ "Rotation: Rotation", "Vector3: Translation" }, { Datum::CreateInitializedCopy(rotationOne), Datum::CreateInitializedCopy(position) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateFromQuaternionAndTranslation(rotationOne, position);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromRowsNode
        auto output = TestMathFunction<FromRowsNode>({ "Vector4: Row 0", "Vector4: Row 1", "Vector4: Row 2", }, { Datum::CreateInitializedCopy(r0), Datum::CreateInitializedCopy(r1), Datum::CreateInitializedCopy(r2) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateFromRows(r0, r1, r2);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromScaleNode
        auto output = TestMathFunction<FromScaleNode>({ "Vector3: Scale" }, { Datum::CreateInitializedCopy(scale) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateScale(scale);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromTranslationNode
        auto output = TestMathFunction<FromTranslationNode>({ "Vector3: Translation" }, { Datum::CreateInitializedCopy(position) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateTranslation(position);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // FromValueNode
        auto output = TestMathFunction<FromValueNode>({ "Number: Source" }, { Datum::CreateInitializedCopy(3) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType::CreateFromValue(ToVectorFloat(3));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // InvertOrthogonalNode
        auto output = TestMathFunction<InvertOrthogonalNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(invertable) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = invertable.GetInverseFast();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }
#endif

    { // InvertSlowNode
        auto output = TestMathFunction<InvertSlowNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(invertable) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = invertable.GetInverseFull();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // IsCloseNode
        auto outputFalse = TestMathFunction<IsCloseNode>({ "Transform: A", "Transform: B" }, { Datum::CreateInitializedCopy(invertable), Datum::CreateInitializedCopy(identity) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(invertable.IsClose(identity), *outputFalse[0].GetAs<BooleanType>());
        EXPECT_FALSE(invertable.IsClose(identity));
        auto outputTrue = TestMathFunction<IsCloseNode>({ "Transform: A", "Transform: B" }, { Datum::CreateInitializedCopy(invertable), Datum::CreateInitializedCopy(invertable) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(invertable.IsClose(invertable), *outputTrue[0].GetAs<BooleanType>());
        EXPECT_TRUE(invertable.IsClose(invertable));
    }

    { // IsFiniteNode
        auto output = TestMathFunction<IsFiniteNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(identity) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(identity.IsFinite(), *output[0].GetAs<BooleanType>());
        EXPECT_TRUE(identity.IsFinite());
    }

    { // IsOrthogonalNode
        auto outputTrue = TestMathFunction<IsOrthogonalNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(identity) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(identity.IsOrthogonal(), *outputTrue[0].GetAs<BooleanType>());
        EXPECT_TRUE(identity.IsOrthogonal());
        auto outputFalse = TestMathFunction<IsOrthogonalNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(notOrthogonal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(notOrthogonal.IsOrthogonal(), *outputFalse[0].GetAs<BooleanType>());
        EXPECT_FALSE(notOrthogonal.IsOrthogonal());
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // Multiply3x3ByVector3Node
        auto output = TestMathFunction<Multiply3x3ByVector3Node>({ "Transform: Source", "Vector3: Multiplier" }, { Datum::CreateInitializedCopy(notOrthogonal), Datum::CreateInitializedCopy(scale) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero3) });
        auto result = notOrthogonal.Multiply3x3(scale);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }
#endif

    { // MultiplyByScaleNode
        auto output = TestMathFunction<MultiplyByScaleNode>({ "Transform: Source", "Vector3: Scale" }, { Datum::CreateInitializedCopy(notOrthogonal), Datum::CreateInitializedCopy(scale) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = TransformType(notOrthogonal);
        result.MultiplyByScale(scale);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // MultiplyByTransformNode
        auto output = TestMathFunction<MultiplyByTransformNode>({ "Transform: A", "Transform: B" }, { Datum::CreateInitializedCopy(notOrthogonal), Datum::CreateInitializedCopy(notOrthogonal) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = notOrthogonal * notOrthogonal;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    { // MultiplyByVector3Node
        auto output = TestMathFunction<MultiplyByVector3Node>({ "Transform: Source", "Vector3: Multiplier" }, { Datum::CreateInitializedCopy(notOrthogonal), Datum::CreateInitializedCopy(scale) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero3) });
        auto result = notOrthogonal * scale;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // MultiplyByVector4Node
        const Vector4Type multiplier(4, 3, 2, 7);
        auto output = TestMathFunction<MultiplyByVector4Node>({ "Transform: Source", "Vector4: Multiplier" }, { Datum::CreateInitializedCopy(notOrthogonal), Datum::CreateInitializedCopy(multiplier) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero4) });
        auto result = notOrthogonal * multiplier;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector4Type>()));
    }

    { // OrthogonalizeNode
        TransformType nearlyOrthogonal = TransformType::CreateFromColumns(xPosish, yPosish, zPosish, position);
        TransformType notOrthogonal = TransformType::CreateFromColumns(xPos, xPos, xPos, position);

        auto orthogonalResult = nearlyOrthogonal.GetOrthogonalized();
        auto orthogonalOutput = *TestMathFunction<OrthogonalizeNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(nearlyOrthogonal) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(notOrthogonal) })[0].GetAs<TransformType>();
        EXPECT_TRUE(orthogonalResult.IsClose(orthogonalOutput));
        EXPECT_TRUE(orthogonalResult.IsOrthogonal());
        EXPECT_TRUE(orthogonalOutput.IsOrthogonal());

        auto notOrthogonalResult = notOrthogonal.GetOrthogonalized();
        auto notOrthogonalOutput = *TestMathFunction<OrthogonalizeNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(notOrthogonal) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(identity) })[0].GetAs<TransformType>();
        EXPECT_TRUE(notOrthogonalResult.IsClose(notOrthogonalOutput));
        EXPECT_FALSE(notOrthogonalResult.IsOrthogonal());
        EXPECT_FALSE(notOrthogonalOutput.IsOrthogonal());
    }

    { // RotationXDegreesNode
        auto output = TestMathFunction<RotationXDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(30) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(identity) });
        auto result = TransformType::CreateRotationX(AZ::DegToRad(30));
        auto outputTM = *output[0].GetAs<TransformType>();
        EXPECT_TRUE(result.IsClose(outputTM));
    }

    { // RotationYDegreesNode
        auto output = TestMathFunction<RotationYDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(30) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(identity) });
        auto result = TransformType::CreateRotationY(AZ::DegToRad(30));
        auto outputTM = *output[0].GetAs<TransformType>();
        EXPECT_TRUE(result.IsClose(outputTM));
    }

    { // RotationZDegreesNode
        auto output = TestMathFunction<RotationZDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(30) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(identity) });
        auto result = TransformType::CreateRotationZ(AZ::DegToRad(30));
        auto outputTM = *output[0].GetAs<TransformType>();
        EXPECT_TRUE(result.IsClose(outputTM));
    }

    { // GetColumnNode
        for (int i = 0; i < 4; ++i)
        {
            auto output = TestMathFunction<GetColumnNode>({ "Transform: Source", "Number: Column" }, { Datum::CreateInitializedCopy(invertable), Datum::CreateInitializedCopy(i) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero3) });
            auto result = invertable.GetColumn(i);
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
        }
    }

    { // GetColumnsNode
        auto output = TestMathFunction<GetColumnsNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(invertable) }, { "Column 0: Vector3", "Column 1: Vector3", "Column 2: Vector3", "Column 3: Vector3" }, { Datum::CreateInitializedCopy(zero3), Datum::CreateInitializedCopy(zero3), Datum::CreateInitializedCopy(zero3), Datum::CreateInitializedCopy(zero3) });
        Vector3Type x, y, z, position;
        invertable.GetColumns(&x, &y, &z, &position);
        EXPECT_TRUE(x.IsClose(*output[0].GetAs<Vector3Type>()));
        EXPECT_TRUE(y.IsClose(*output[1].GetAs<Vector3Type>()));
        EXPECT_TRUE(z.IsClose(*output[2].GetAs<Vector3Type>()));
        EXPECT_TRUE(position.IsClose(*output[3].GetAs<Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // ToDeterminant3x3Node
        auto output = TestMathFunction<ToDeterminant3x3Node>({ "Transform: Source" }, { Datum::CreateInitializedCopy(invertable) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        auto result = invertable.GetDeterminant3x3();
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<NumberType>()));
    }
#endif

    { // GetElementNode
        for (int row(0); row < 3; ++row)
        {
            for (int col(0); col < 4; ++col)
            {
                auto output = TestMathFunction<GetElementNode>({ "Transform: Source", "Number: Row", "Number: Column" }, { Datum::CreateInitializedCopy(invertable), Datum::CreateInitializedCopy(row), Datum::CreateInitializedCopy(col) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
                auto result = invertable.GetElement(row, col);
                float resultFloat = invertable.GetElement(row, col);
                float outputFloat = aznumeric_caster(*output[0].GetAs<NumberType>());
                EXPECT_FLOAT_EQ(resultFloat, outputFloat);
            }
        }
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // ToPolarDecompositionNode
        auto output = TestMathFunction<ToPolarDecompositionNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(notOrthogonal) }, { "Orthogonal: Transform", "Symmetric: Transform" }, { Datum::CreateInitializedCopy(zero), Datum::CreateInitializedCopy(zero) });
        TransformType orth, symm;
        notOrthogonal.GetPolarDecomposition(&orth, &symm);
        auto output0TM = *output[0].GetAs<TransformType>();
        EXPECT_TRUE(orth.IsClose(output0TM));
        auto output1TM = *output[1].GetAs<TransformType>();
        EXPECT_TRUE(symm.IsClose(output1TM));
    }

    { // ToPolarDecompositionOrthogonalOnlyNode
        auto output = TestMathFunction<ToPolarDecompositionOrthogonalOnlyNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(notOrthogonal) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = notOrthogonal.GetPolarDecomposition();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }
#endif

    { // GetRowNode
        for (int i = 0; i < 3; ++i)
        {
            auto output = TestMathFunction<GetRowNode>({ "Transform: Source", "Number: Row" }, { Datum::CreateInitializedCopy(invertable), Datum::CreateInitializedCopy(i) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero4) });
            auto result = invertable.GetRow(i);
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector4Type>()));
        }
    }

    { // GetRowsNode
        auto output = TestMathFunction<GetRowsNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(invertable) }, { "Row 0: Vector4", "Row 1: Vector4", "Row 2: Vector4" }, { Datum::CreateInitializedCopy(zero4), Datum::CreateInitializedCopy(zero4), Datum::CreateInitializedCopy(zero4) });
        Vector4Type r0, r1, r2;
        invertable.GetRows(&r0, &r1, &r2);
        EXPECT_TRUE(r0.IsClose(*output[0].GetAs<Vector4Type>()));
        EXPECT_TRUE(r1.IsClose(*output[1].GetAs<Vector4Type>()));
        EXPECT_TRUE(r2.IsClose(*output[2].GetAs<Vector4Type>()));
    }

    { // ToScaleNode
        auto output = TestMathFunction<ToScaleNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(notOrthogonal) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero3) });
        auto result = notOrthogonal.RetrieveScale();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // GetTranslationNode
        auto output = TestMathFunction<GetTranslationNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(notOrthogonal) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero3) });
        auto result = notOrthogonal.GetTranslation();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }

    { // TransposeNode
        auto output = TestMathFunction<TransposeNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(notOrthogonal) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = notOrthogonal.GetTranspose();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // Transpose3x3Node
        auto output = TestMathFunction<Transpose3x3Node>({ "Transform: Source" }, { Datum::CreateInitializedCopy(notOrthogonal) }, { "Result: Transform" }, { Datum::CreateInitializedCopy(zero) });
        auto result = notOrthogonal.GetTranspose3x3();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }

    
    { // TransposedMultiply3x3Node
        auto output = TestMathFunction<TransposedMultiply3x3Node>({ "Transform: Source", "Vector3: Transpose" }, { Datum::CreateInitializedCopy(notOrthogonal), Datum::CreateInitializedCopy(scale) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero3) });
        auto result = notOrthogonal.TransposedMultiply3x3(scale);
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Vector3Type>()));
    }    

    { // ZeroNode
        auto output = TestMathFunction<ZeroNode>({}, {}, { "Result: Transform" }, { Datum::CreateInitializedCopy(identity) });
        auto result = TransformType::CreateZero();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<TransformType>()));
    }
#endif

    // Missing Nodes:
    // Transpose3x3Node

} // Transform Test

TEST_F(ScriptCanvasTestFixture, Vector2Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Vector2Nodes;

    Data::Vector2Type zero(0, 0);
    Data::Vector2Type one(1.f, 1.f);
    Data::Vector2Type negativeOne(-1.f, -1.f);

    {   // Absolute
        Data::Vector2Type source(-1, -1);
        Data::Vector2Type absolute = source.GetAbs();
        auto output = TestMathFunction<AbsoluteNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(absolute.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // Add
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(1, 1);
        Data::Vector2Type c = a + b;
        auto output = TestMathFunction<AddNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // Clamp
        Data::Vector2Type source(-10, 20);
        Data::Vector2Type min(1, 1);
        Data::Vector2Type max(1, 1);
        auto result = source.GetClamp(min, max);
        auto output = TestMathFunction<ClampNode>({ "Vector2: Source", "Vector2: Min", "Vector2: Max" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(min), Datum::CreateInitializedCopy(max), }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // Distance
        Data::Vector2Type a(1, 2);
        Data::Vector2Type b(-3, -2);
        auto result = a.GetDistance(b);
        auto output = TestMathFunction<DistanceNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // DistanceSqr
        Data::Vector2Type a(1, 2);
        Data::Vector2Type b(-3, -2);
        auto result = a.GetDistanceSq(b);
        auto output = TestMathFunction<DistanceSquaredNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    {   // DivideByNumber
        Data::Vector2Type a(1, 2);
        Data::NumberType b(3.0);
        auto result = a / Data::ToVectorFloat(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Vector2: Source", "Number: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {  // DivideByVector
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(2, 2);
        Data::Vector2Type c = a / b;
        auto output = TestMathFunction<DivideByVectorNode>({ "Vector2: Numerator", "Vector2: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Dot
        Data::Vector2Type a(1, 2);
        Data::Vector2Type b(-3, -2);
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // From Element
        const Data::Vector2Type source(1, 2);

        for (int index = 0; index < 2; ++index)
        {
            auto result = source;
            result.SetElement(index, 4.0f);
            auto output = TestMathFunction<FromElementNode>({ "Vector2: Source", "Number: Index", "Number: Value" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(index), Datum::CreateInitializedCopy(4.0) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
        }
    }    

    { // FromLength
        Data::Vector2Type source(1, 2);
        Data::NumberType length(10);
        auto result = source;
        result.SetLength(Data::ToVectorFloat(length));
        auto output = TestMathFunction<FromLengthNode>({ "Vector2: Source", "Number: Length" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(length) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }
#endif

    { // FromValues
        Data::NumberType x(1);
        Data::NumberType y(2);
        auto result = Data::Vector2Type(Data::ToVectorFloat(x), Data::ToVectorFloat(y));
        auto output = TestMathFunction<FromValuesNode>({ "Number: X", "Number: Y" }, { Datum::CreateInitializedCopy(x), Datum::CreateInitializedCopy(y) }, { "Result: Vector2", }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // GetElements
        auto source = Vector2Type(1, 2);
        auto output = TestMathFunction<GetElementsNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "X: Number", "Y: Number" }, { Datum::CreateInitializedCopy(0), Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(source.GetX(), aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
        EXPECT_FLOAT_EQ(source.GetY(), aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // IsClose
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(2, 2);
        Data::Vector2Type c(1.09, 1.09);

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, Data::ToVectorFloat(2.1));
        output = TestMathFunction<IsCloseNode>({ "Vector2: A", "Vector2: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(2.1) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(c) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, Data::ToVectorFloat(0.9));
        output = TestMathFunction<IsCloseNode>({ "Vector2: A", "Vector2: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(0.9) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        Data::Vector2Type sourceFinite(1, 1);
        auto result = sourceFinite.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(sourceFinite) }, { "Result: Boolean", }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsNormal
        Data::Vector2Type normal(1.f, 0.f);
        Data::Vector2Type nearlyNormal(1.001f, 0.0f);
        Data::Vector2Type notNormal(10.f, 0.f);

        {
            auto result = normal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(normal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = nearlyNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(nearlyNormal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = nearlyNormal.IsNormalized(2.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source", "Number: Tolerance" }, { Datum::CreateInitializedCopy(nearlyNormal), Datum::CreateInitializedCopy(2.0f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = notNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(notNormal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = notNormal.IsNormalized(12.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector2: Source", "Number: Tolerance" }, { Datum::CreateInitializedCopy(notNormal), Datum::CreateInitializedCopy(12.0f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }
    }

    { // IsZero
        auto result = zero.IsZero();
        auto output = TestMathFunction<IsZeroNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(zero) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = one.IsZero();
        output = TestMathFunction<IsZeroNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(one) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // Length
        Data::Vector2Type source(1, 2);
        auto result = source.GetLength();
        auto output = TestMathFunction<LengthNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthSquared
        Data::Vector2Type source(1, 2);
        auto result = source.GetLengthSq();
        auto output = TestMathFunction<LengthSquaredNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // Lerp
        Data::Vector2Type from(0.f, 0.f);
        Data::Vector2Type to(1.f, 1.f);
        Data::NumberType t(0.5);

        auto result = from.Lerp(to, Data::ToVectorFloat(t));
        auto output = TestMathFunction<LerpNode>({ "Vector2: From", "Vector2: To", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(t) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Max
        Data::Vector2Type a(-1, -1);
        Data::Vector2Type b(1, 1);
        auto result = a.GetMax(b);
        auto output = TestMathFunction<MaxNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Min
        Data::Vector2Type a(-1, -1);
        Data::Vector2Type b(1, 1);
        auto result = a.GetMin(b);
        auto output = TestMathFunction<MinNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {   // ModXNode
        Data::Vector2Type a(1, 2);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModXNode>({ "Vector2: Source", "Number: X" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetX(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModYNode
        Data::Vector2Type a(1, 2);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModYNode>({ "Vector2: Source", "Number: Y" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetY(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    { // MultiplyAdd
        Data::Vector2Type a(2, 2);
        Data::Vector2Type b(3, 3);
        Data::Vector2Type c(4, 4);
        auto result = a.GetMadd(b, c);
        auto output = TestMathFunction<MultiplyAddNode>({ "Vector2: A", "Vector2: B", "Vector2: C" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(c) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }
#endif

    {   // MultiplyByNumber
        Data::Vector2Type a(1, 2);
        Data::NumberType b(3.0);
        auto result = a * Data::ToVectorFloat(b);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Vector2: Source", "Number: Multiplier" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {  // MultiplyByVector
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(2, 2);
        Data::Vector2Type c = a * b;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Vector2: Source", "Vector2: Multiplier" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    {   // Negate
        Data::Vector2Type source = one;
        auto output = TestMathFunction<NegateNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        auto result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));

        source = negativeOne;
        output = TestMathFunction<NegateNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Normalize
        Data::Vector2Type source = one;
        auto output = TestMathFunction<NormalizeNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        auto result = source.GetNormalizedSafe();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // NormalizeWithLength
        Data::Vector2Type source = one;
        auto output = TestMathFunction<NormalizeWithLengthNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "Normalized: Vector2", "Length: Number" }, { Datum::CreateInitializedCopy(zero), Datum::CreateInitializedCopy(0) });
        auto result = source.NormalizeSafeWithLength();
        EXPECT_TRUE(source.IsClose(*output[0].GetAs<Data::Vector2Type>()));
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // Project
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(-2, -2);
        auto output = TestMathFunction<ProjectNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(a) });
        a.Project(b);
        EXPECT_TRUE(a.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // Slerp
        Data::Vector2Type from(0.f, 0.f);
        Data::Vector2Type to(1.f, 1.f);
        Data::NumberType t(0.5);

        auto slerp = from.Slerp(to, Data::ToVectorFloat(t));
        auto outputSlerp = TestMathFunction<SlerpNode>({ "Vector2: From", "Vector2: To", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(t) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(slerp.IsClose(*outputSlerp[0].GetAs<Data::Vector2Type>()));

        auto lerp = from.Lerp(to, Data::ToVectorFloat(t));
        auto outputLerp = TestMathFunction<LerpNode>({ "Vector2: From", "Vector2: To", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(t) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(lerp.IsClose(*outputLerp[0].GetAs<Data::Vector2Type>()));

        EXPECT_NE(lerp, slerp);
        EXPECT_NE(*outputLerp[0].GetAs<Data::Vector2Type>(), *outputSlerp[0].GetAs<Data::Vector2Type>());
    }

    {   // Subtract
        Data::Vector2Type a(1, 1);
        Data::Vector2Type b(2, 2);
        Data::Vector2Type c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Vector2: A", "Vector2: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

    { // GetElement
        const Data::Vector2Type source(1, 2);

        for (int index = 0; index < 2; ++index)
        {
            float result = aznumeric_caster(Data::FromVectorFloat(source.GetElement(index)));
            auto output = TestMathFunction<GetElementNode>({ "Vector2: Source", "Number: Index" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(index) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
            float outputNumber = aznumeric_caster(*output[0].GetAs<Data::NumberType>());
            EXPECT_FLOAT_EQ(result, outputNumber);
        }
    }

    {   // ToPerpendicular
        Data::Vector2Type source(3, -1);
        auto output = TestMathFunction<ToPerpendicularNode>({ "Vector2: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector2" }, { Datum::CreateInitializedCopy(zero) });
        auto result = source.GetPerpendicular();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector2Type>()));
    }

} // Test Vector2Node

TEST_F(ScriptCanvasTestFixture, Vector3Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Vector3Nodes;
    Data::Vector3Type zero(0, 0, 0);
    Data::Vector3Type one(1.f, 1.f, 1.f);
    Data::Vector3Type negativeOne(-1.f, -1.f, -1.f);

    {   // Absolute
        Data::Vector3Type source(-1, -1, -1);
        Data::Vector3Type absolute = source.GetAbs();
        auto output = TestMathFunction<AbsoluteNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(absolute.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // Add
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(1, 1, 1);
        Data::Vector3Type c = a + b;
        auto output = TestMathFunction<AddNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {   // AngleMod
        Data::Vector3Type source(1, 1, 1);
        Data::Vector3Type result = source.GetAngleMod();
        auto output = TestMathFunction<AngleModNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

    {   // BuildTangentBasis
        Data::Vector3Type source, tangent, bitangent;
        source = Data::Vector3Type(1, 1, 1);
        source.NormalizeSafe();
        source.BuildTangentBasis(tangent, bitangent);
        auto output = TestMathFunction<BuildTangentBasisNode>({ "Vector3: Normal" }, { Datum::CreateInitializedCopy(source) }, { "Tangent: Vector3", "Bitangent: Vector3" }, { Datum::CreateInitializedCopy(zero), Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(tangent.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(bitangent.IsClose(*output[1].GetAs<Data::Vector3Type>()));
    }

    {   // Clamp
        Data::Vector3Type source(-10, 20, 1);
        Data::Vector3Type min(1, 1, 1);
        Data::Vector3Type max(1, 1, 1);
        auto result = source.GetClamp(min, max);
        auto output = TestMathFunction<ClampNode>({ "Vector3: Source", "Vector3: Min", "Vector3: Max" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(min), Datum::CreateInitializedCopy(max), }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {   // Cosine
        Data::Vector3Type source(1.5, 1.5, 1.5);
        auto result = source.GetCos();
        auto output = TestMathFunction<CosineNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // CrossXAxis
        Data::Vector3Type source(1, 1, 1);
        auto result = source.CrossXAxis();
        auto output = TestMathFunction<CrossXAxisNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // CrossYAxis
        Data::Vector3Type source(1, 1, 1);
        auto result = source.CrossYAxis();
        auto output = TestMathFunction<CrossYAxisNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // CrossZAxis
        Data::Vector3Type source(1, 1, 1);
        auto result = source.CrossZAxis();
        auto output = TestMathFunction<CrossZAxisNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

    {   // Distance
        Data::Vector3Type a(1, 2, 3);
        Data::Vector3Type b(-3, -2, -1);
        auto result = a.GetDistance(b);
        auto output = TestMathFunction<DistanceNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // DistanceSqr
        Data::Vector3Type a(1, 2, 3);
        Data::Vector3Type b(-3, -2, -1);
        auto result = a.GetDistanceSq(b);
        auto output = TestMathFunction<DistanceSquaredNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    {   // DivideByNumber
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(3.0);
        auto result = a / Data::ToVectorFloat(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Vector3: Source", "Number: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {  // DivideByVector
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(2, 2, 2);
        Data::Vector3Type c = a / b;
        auto output = TestMathFunction<DivideByVectorNode>({ "Vector3: Numerator", "Vector3: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Dot
        Data::Vector3Type a(1, 2, 3);
        Data::Vector3Type b(-3, -2, -1);
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // From Element
        const Data::Vector3Type source(1, 2, 3);

        for (int index = 0; index < 3; ++index)
        {
            auto result = source;
            result.SetElement(index, 4.0f);
            auto output = TestMathFunction<FromElementNode>({ "Vector3: Source", "Number: Index", "Number: Value" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(index), Datum::CreateInitializedCopy(4.0) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        }
    }

    { // FromLength
        Data::Vector3Type source(1, 2, 3);
        Data::NumberType length(10);
        auto result = source;
        result.SetLength(Data::ToVectorFloat(length));
        auto output = TestMathFunction<FromLengthNode>({ "Vector3: Source", "Number: Length" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(length) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

    { // FromValues
        Data::NumberType x(1);
        Data::NumberType y(2);
        Data::NumberType z(3);
        auto result = Data::Vector3Type(Data::ToVectorFloat(x), Data::ToVectorFloat(y), Data::ToVectorFloat(z));
        auto output = TestMathFunction<FromValuesNode>({ "Number: X", "Number: Y", "Number: Z" }, { Datum::CreateInitializedCopy(x), Datum::CreateInitializedCopy(y), Datum::CreateInitializedCopy(z) }, { "Result: Vector3", }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // IsCloseNode
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(2, 2, 2);
        Data::Vector3Type c(1.001, 1.001, 1.001);

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, Data::ToVectorFloat(2.1));
        output = TestMathFunction<IsCloseNode>({ "Vector3: A", "Vector3: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(2.1) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(c) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, Data::ToVectorFloat(0.9));
        output = TestMathFunction<IsCloseNode>({ "Vector3: A", "Vector3: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(0.9) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        Data::Vector3Type sourceFinite(1, 1, 1);
        auto result = sourceFinite.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(sourceFinite) }, { "Result: Boolean", }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsNormal
        Data::Vector3Type normal(1.f, 0.f, 0.f);
        Data::Vector3Type nearlyNormal(1.001f, 0.0f, 0.0f);
        Data::Vector3Type notNormal(10.f, 0.f, 0.f);

        {
            auto result = normal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(normal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = nearlyNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(nearlyNormal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = nearlyNormal.IsNormalized(2.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source", "Number: Tolerance" }, { Datum::CreateInitializedCopy(nearlyNormal), Datum::CreateInitializedCopy(2.0f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = notNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(notNormal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = notNormal.IsNormalized(12.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector3: Source", "Number: Tolerance" }, { Datum::CreateInitializedCopy(notNormal), Datum::CreateInitializedCopy(12.0f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }
    }

    { // IsPerpendicular
        Data::Vector3Type x(1.f, 0.f, 0.f);
        Data::Vector3Type y(0.f, 1.f, 0.f);

        auto result = x.IsPerpendicular(x);
        auto output = TestMathFunction<IsPerpendicularNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(x), Datum::CreateInitializedCopy(x) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = x.IsPerpendicular(y);
        output = TestMathFunction<IsPerpendicularNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(x), Datum::CreateInitializedCopy(y) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsZero
        auto result = zero.IsZero();
        auto output = TestMathFunction<IsZeroNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(zero) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = one.IsZero();
        output = TestMathFunction<IsZeroNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(one) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // Length
        Data::Vector3Type source(1, 2, 3);
        auto result = source.GetLength();
        auto output = TestMathFunction<LengthNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthReciprocal
        Data::Vector3Type source(1, 2, 3);
        auto result = source.GetLengthReciprocal();
        auto output = TestMathFunction<LengthReciprocalNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthSquared
        Data::Vector3Type source(1, 2, 3);
        auto result = source.GetLengthSq();
        auto output = TestMathFunction<LengthSquaredNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // Lerp
        Data::Vector3Type from(0.f, 0.f, 0.f);
        Data::Vector3Type to(1.f, 1.f, 1.f);
        Data::NumberType t(0.5);

        auto result = from.Lerp(to, Data::ToVectorFloat(t));
        auto output = TestMathFunction<LerpNode>({ "Vector3: From", "Vector3: To", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(t) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Max
        Data::Vector3Type a(-1, -1, -1);
        Data::Vector3Type b(1, 1, 1);
        auto result = a.GetMax(b);
        auto output = TestMathFunction<MaxNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Min
        Data::Vector3Type a(-1, -1, -1);
        Data::Vector3Type b(1, 1, 1);
        auto result = a.GetMin(b);
        auto output = TestMathFunction<MinNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {   // ModXNode
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModXNode>({ "Vector3: Source", "Number: X" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetX(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModYNode
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModYNode>({ "Vector3: Source", "Number: Y" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetY(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModZNode
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModZNode>({ "Vector3: Source", "Number: Z" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetZ(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }    

    { // MultiplyAdd
        Data::Vector3Type a(2, 2, 2);
        Data::Vector3Type b(3, 3, 3);
        Data::Vector3Type c(4, 4, 4);
        auto result = a.GetMadd(b, c);
        auto output = TestMathFunction<MultiplyAddNode>({ "Vector3: A", "Vector3: B", "Vector3: C" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(c) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

    {   // MultiplyByNumber
        Data::Vector3Type a(1, 2, 3);
        Data::NumberType b(3.0);
        auto result = a * Data::ToVectorFloat(b);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Vector3: Source", "Number: Multiplier" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {  // MultiplyByVector
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(2, 2, 2);
        Data::Vector3Type c = a * b;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Vector3: Source", "Vector3: Multiplier" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    {   // Negate
        Data::Vector3Type source = one;
        auto output = TestMathFunction<NegateNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        auto result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));

        source = negativeOne;
        output = TestMathFunction<NegateNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Normalize
        Data::Vector3Type source = one;
        auto output = TestMathFunction<NormalizeNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        auto result = source.GetNormalizedSafe();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // NormalizeWithLength
        Data::Vector3Type source = one;
        auto output = TestMathFunction<NormalizeWithLengthNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Normalized: Vector3", "Length: Number" }, { Datum::CreateInitializedCopy(zero), Datum::CreateInitializedCopy(0) });
        auto result = source.NormalizeSafeWithLength();
        EXPECT_TRUE(source.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // Project
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(-2, -2, -2);
        auto output = TestMathFunction<ProjectNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(a) });
        a.Project(b);
        EXPECT_TRUE(a.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // Reciprocal
        Data::Vector3Type source(2, 2, 2);
        Data::Vector3Type result = source.GetReciprocal();
        auto output = TestMathFunction<ReciprocalNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // Sine
        Data::Vector3Type source(.75, .75, .75);
        Data::Vector3Type result = source.GetSin();
        auto output = TestMathFunction<SineNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }    

    { // SineCosine
        Data::Vector3Type source(.75, .75, .75);
        Data::Vector3Type sine, cosine;
        source.GetSinCos(sine, cosine);
        auto output = TestMathFunction<SineCosineNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Sine: Vector3", "Cosine: Vector3" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(sine.IsClose(*output[0].GetAs<Data::Vector3Type>()));
        EXPECT_TRUE(cosine.IsClose(*output[1].GetAs<Data::Vector3Type>()));
    }
#endif

    { // Slerp
        Data::Vector3Type from(0.f, 0.f, 0.f);
        Data::Vector3Type to(1.f, 1.f, 1.f);
        Data::NumberType t(0.5);

        auto slerp = from.Slerp(to, Data::ToVectorFloat(t));
        auto outputSlerp = TestMathFunction<SlerpNode>({ "Vector3: From", "Vector3: To", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(t) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(slerp.IsClose(*outputSlerp[0].GetAs<Data::Vector3Type>()));

        auto lerp = from.Lerp(to, Data::ToVectorFloat(t));
        auto outputLerp = TestMathFunction<LerpNode>({ "Vector3: From", "Vector3: To", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(t) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(lerp.IsClose(*outputLerp[0].GetAs<Data::Vector3Type>()));

        EXPECT_NE(lerp, slerp);
        EXPECT_NE(*outputLerp[0].GetAs<Data::Vector3Type>(), *outputSlerp[0].GetAs<Data::Vector3Type>());
    }

    {   // Subtract
        Data::Vector3Type a(1, 1, 1);
        Data::Vector3Type b(2, 2, 2);
        Data::Vector3Type c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Vector3: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // GetElement
        const Data::Vector3Type source(1, 2, 3);

        for (int index = 0; index < 3; ++index)
        {
            float result = aznumeric_caster(Data::FromVectorFloat(source.GetElement(index)));
            auto output = TestMathFunction<GetElementNode>({ "Vector3: Source", "Number: Index" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(index) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
            float outputNumber = aznumeric_caster(*output[0].GetAs<Data::NumberType>());
            EXPECT_FLOAT_EQ(result, outputNumber);
        }
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // XAxisCross
        Data::Vector3Type source(.75, .75, .75);
        auto result = source.XAxisCross();
        auto output = TestMathFunction<XAxisCrossNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // YAxisCross
        Data::Vector3Type source(.75, .75, .75);
        auto result = source.YAxisCross();
        auto output = TestMathFunction<YAxisCrossNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }

    { // ZAxisCross
        Data::Vector3Type source(.75, .75, .75);
        auto result = source.ZAxisCross();
        auto output = TestMathFunction<ZAxisCrossNode>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector3" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector3Type>()));
    }
#endif

} // Test Vector3Node

TEST_F(ScriptCanvasTestFixture, Vector4Nodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::Vector4Nodes;
    Data::Vector4Type zero(0, 0, 0, 0);
    Data::Vector4Type one(1.f, 1.f, 1.f, 1.f);
    Data::Vector4Type negativeOne(-1.f, -1.f, -1.f, -1.f);

    {   // Absolute
        Data::Vector4Type source(-1, -1, -1., -1.);
        Data::Vector4Type absolute = source.GetAbs();
        auto output = TestMathFunction<AbsoluteNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(absolute.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {   // Add
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(1, 1, 1, 1);
        Data::Vector4Type c = a + b;
        auto output = TestMathFunction<AddNode>({ "Vector4: A", "Vector4: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {   // DivideByNumber
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(3.0);
        auto result = a / Data::ToVectorFloat(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Vector4: Source", "Number: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {  // DivideByVector
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(2, 2, 2, 2);
        Data::Vector4Type c = a / b;
        auto output = TestMathFunction<DivideByVectorNode>({ "Vector4: Numerator", "Vector4: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // Dot
        Data::Vector4Type a(1, 2, 3, 4);
        Data::Vector4Type b(-4, -3, -2, -1);
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Vector4: A", "Vector4: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // Dot3
        Data::Vector4Type a(1, 2, 3, 4);
        Data::Vector3Type b(-4, -3, -2);
        auto result = a.Dot3(b);
        auto output = TestMathFunction<Dot3Node>({ "Vector4: A", "Vector3: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // From Element
        const Data::Vector4Type source(1, 2, 3, 4);

        for (int index = 0; index < 4; ++index)
        {
            auto result = source;
            result.SetElement(index, 5.0f);
            auto output = TestMathFunction<FromElementNode>({ "Vector4: Source", "Number: Index", "Number: Value" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(index), Datum::CreateInitializedCopy(5.0) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        }
    }
#endif

    { // FromValues
        Data::NumberType x(1);
        Data::NumberType y(2);
        Data::NumberType z(3);
        Data::NumberType w(4);
        auto result = Data::Vector4Type(Data::ToVectorFloat(x), Data::ToVectorFloat(y), Data::ToVectorFloat(z), Data::ToVectorFloat(w));
        auto output = TestMathFunction<FromValuesNode>({ "Number: X", "Number: Y", "Number: Z", "Number: W" }, { Datum::CreateInitializedCopy(x), Datum::CreateInitializedCopy(y), Datum::CreateInitializedCopy(z), Datum::CreateInitializedCopy(w) }, { "Result: Vector4", }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // GetElements
        auto source = Vector4Type(1, 2, 3, 4);
        auto output = TestMathFunction<GetElementsNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "X: Number", "Y: Number", "Z: Number", "W: Number" }, { Datum::CreateInitializedCopy(0), Datum::CreateInitializedCopy(0), Datum::CreateInitializedCopy(0), Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(source.GetX(), aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
        EXPECT_FLOAT_EQ(source.GetY(), aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
        EXPECT_FLOAT_EQ(source.GetZ(), aznumeric_caster(*output[2].GetAs<Data::NumberType>()));
        EXPECT_FLOAT_EQ(source.GetW(), aznumeric_caster(*output[3].GetAs<Data::NumberType>()));
    }
#endif

    { // IsCloseNode
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(2, 2, 2, 2);
        Data::Vector4Type c(1.001, 1.001, 1.001, 1.001);

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Vector4: A", "Vector4: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, Data::ToVectorFloat(2.1));
        output = TestMathFunction<IsCloseNode>({ "Vector4: A", "Vector4: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(2.1) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Vector4: A", "Vector4: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(c) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, Data::ToVectorFloat(0.9));
        output = TestMathFunction<IsCloseNode>({ "Vector4: A", "Vector4: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(0.9) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        Data::Vector4Type sourceFinite(1, 1, 1, 1);
        auto result = sourceFinite.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(sourceFinite) }, { "Result: Boolean", }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsNormal
        Data::Vector4Type normal(1.f, 0.f, 0.f, 0.f);
        Data::Vector4Type nearlyNormal(1.001f, 0.0f, 0.0f, 0.f);
        Data::Vector4Type notNormal(10.f, 0.f, 0.f, 0.f);

        {
            auto result = normal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(normal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = nearlyNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(nearlyNormal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = nearlyNormal.IsNormalized(2.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source", "Number: Tolerance" }, { Datum::CreateInitializedCopy(nearlyNormal), Datum::CreateInitializedCopy(2.0f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }

        {
            auto result = notNormal.IsNormalized();
            auto output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(notNormal) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

            result = notNormal.IsNormalized(12.0f);
            output = TestMathFunction<IsNormalizedNode>({ "Vector4: Source", "Number: Tolerance" }, { Datum::CreateInitializedCopy(notNormal), Datum::CreateInitializedCopy(12.0f) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
            EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
        }
    }

    { // IsZero
        auto result = zero.IsZero();
        auto output = TestMathFunction<IsZeroNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(zero) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = one.IsZero();
        output = TestMathFunction<IsZeroNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(one) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // Length
        Data::Vector4Type source(1, 2, 3, 4);
        auto result = source.GetLength();
        auto output = TestMathFunction<LengthNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthReciprocal
        Data::Vector4Type source(1, 2, 3, 4);
        auto result = source.GetLengthReciprocal();
        auto output = TestMathFunction<LengthReciprocalNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthSquared
        Data::Vector4Type source(1, 2, 3, 4);
        auto result = source.GetLengthSq();
        auto output = TestMathFunction<LengthSquaredNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    {   // ModXNode
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModXNode>({ "Vector4: Source", "Number: X" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetX(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModYNode
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModYNode>({ "Vector4: Source", "Number: Y" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetY(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModZNode
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModZNode>({ "Vector4: Source", "Number: Z" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetZ(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }

    {   // ModWNode
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(0);
        auto output = TestMathFunction<ModWNode>({ "Vector4: Source", "Number: W" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        auto result = a;
        result.SetW(Data::ToVectorFloat(b));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FALSE(result.IsClose(a));
    }
#endif

    {   // MultiplyByNumber
        Data::Vector4Type a(1, 2, 3, 4);
        Data::NumberType b(3.0);
        auto result = a * Data::ToVectorFloat(b);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Vector4: Source", "Number: Multiplier" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {  // MultiplyByVector
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(2, 2, 2, 2);
        Data::Vector4Type c = a * b;
        auto output = TestMathFunction<MultiplyByVectorNode>({ "Vector4: Source", "Vector4: Multiplier" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {   // Negate
        Data::Vector4Type source = one;
        auto output = TestMathFunction<NegateNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        auto result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));

        source = negativeOne;
        output = TestMathFunction<NegateNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // Normalize
        Data::Vector4Type source = one;
        auto output = TestMathFunction<NormalizeNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(zero) });
        auto result = source.GetNormalizedSafe();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // NormalizeWithLength
        Data::Vector4Type source = one;
        auto output = TestMathFunction<NormalizeWithLengthNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Normalized: Vector4", "Length: Number" }, { Datum::CreateInitializedCopy(zero), Datum::CreateInitializedCopy(0) });
        auto result = source.NormalizeSafeWithLength();
        EXPECT_TRUE(source.IsClose(*output[0].GetAs<Data::Vector4Type>()));
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // Reciprocal
        Data::Vector4Type source(2, 2, 2, 2);
        Data::Vector4Type result = source.GetReciprocal();
        auto output = TestMathFunction<ReciprocalNode>({ "Vector4: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(source) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    {   // Subtract
        Data::Vector4Type a(1, 1, 1, 1);
        Data::Vector4Type b(2, 2, 2, 2);
        Data::Vector4Type c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Vector4: A", "Vector4: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Vector4" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::Vector4Type>()));
    }

    { // GetElement
        const Data::Vector4Type source(1, 2, 3, 4);

        for (int index = 0; index < 4; ++index)
        {
            float result = aznumeric_caster(Data::FromVectorFloat(source.GetElement(index)));
            auto output = TestMathFunction<GetElementNode>({ "Vector4: Source", "Number: Index" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(index) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
            float outputNumber = aznumeric_caster(*output[0].GetAs<Data::NumberType>());
            EXPECT_FLOAT_EQ(result, outputNumber);
        }
    }

    // Missing Nodes:
    // HomogenizeNode
    // FromVector3AndNumberNode

} // Test Vector4Node

TEST_F(ScriptCanvasTestFixture, RotationNodes)
{
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace ScriptCanvas::RotationNodes;
    const Data::RotationType esclates(0.125, 0.25, 0.50, 0.75);
    const Data::RotationType negativeOne(-1.f, -1.f, -1.f, -1.f);
    const Data::RotationType one(1.f, 1.f, 1.f, 1.f);
    const Data::RotationType zero(0, 0, 0, 0);
    const Data::RotationType identity(Data::RotationType::CreateIdentity());

    {   // Add
        Data::RotationType a(1, 1, 1, 1);
        Data::RotationType b(1, 1, 1, 1);
        Data::RotationType c = a + b;
        auto output = TestMathFunction<AddNode>({ "Rotation: A", "Rotation: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    {   // Conjugate
        Data::RotationType source = esclates;
        auto output = TestMathFunction<ConjugateNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(zero) });
        auto result = esclates.GetConjugate();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    {   // DivideByNumber
        Data::RotationType a(1, 2, 3, 4);
        Data::NumberType b(3.0);
        auto result = a / Data::ToVectorFloat(b);
        auto output = TestMathFunction<DivideByNumberNode>({ "Rotation: Numerator", "Number: Divisor" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // Dot
        Data::RotationType a(1, 2, 3, 4);
        Data::RotationType b(-4, -3, -2, -1);
        auto result = a.Dot(b);
        auto output = TestMathFunction<DotNode>({ "Rotation: A", "Rotation: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0.0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // From AxisAngleDegrees
        const Vector3Type axis = Vector3Type(1, 1, 1).GetNormalized();
        const float angle(2.0);
        auto result = RotationType::CreateFromAxisAngle(axis, AZ::DegToRad(angle));
        auto output = TestMathFunction<FromAxisAngleDegreesNode>({ "Vector3: Axis", "Number: Degrees" }, { Datum::CreateInitializedCopy(axis), Datum::CreateInitializedCopy(angle) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // From Element
        const Data::RotationType source(1, 2, 3, 4);

        for (int index = 0; index < 4; ++index)
        {
            auto result = source;
            result.SetElement(index, 5.0f);
            auto output = TestMathFunction<FromElementNode>({ "Rotation: Source", "Number: Index", "Number: Value" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(index), Datum::CreateInitializedCopy(5.0) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(zero) });
            EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
        }
    }
#endif

    { // FromMatrix3x3
        const Matrix3x3Type matrix = Matrix3x3Type::CreateFromValue(3.0);
        auto result = RotationType::CreateFromMatrix3x3(matrix);
        auto output = TestMathFunction<FromMatrix3x3Node>({ "Matrix3x3: Source" }, { Datum::CreateInitializedCopy(matrix) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // FromMatrix4x4
        const Matrix4x4Type matrix = Matrix4x4Type::CreateFromValue(3.0);
        auto result = RotationType::CreateFromMatrix4x4(matrix);
        auto output = TestMathFunction<FromMatrix4x4Node>({ "Matrix4x4: Source" }, { Datum::CreateInitializedCopy(matrix) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // FromTransform
        const TransformType transform = TransformType::CreateFromValue(3.0);
        auto result = RotationType::CreateFromTransform(transform);
        auto output = TestMathFunction<FromTransformNode>({ "Transform: Source" }, { Datum::CreateInitializedCopy(transform) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // FromVector3
        const Vector3Type vector3 = Vector3Type(3, 3, 3);
        auto result = RotationType::CreateFromVector3(vector3);
        auto output = TestMathFunction<FromVector3Node>({ "Vector3: Source" }, { Datum::CreateInitializedCopy(vector3) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    // Note: This is just raw setting the elements to the value, it is not creating from axis and angle
    //       There is another method which does create from axis angle.

    { // FromVector3AndValueNode
        const Vector3Type axis = Vector3Type(1, 1, 1).GetNormalized();
        const float angle(2.0);
        auto result = RotationType::CreateFromVector3AndValue(axis, angle);
        auto output = TestMathFunction<FromVector3AndValueNode>({ "Vector3: Imaginary", "Number: Real" }, { Datum::CreateInitializedCopy(axis), Datum::CreateInitializedCopy(angle) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // GetElements
        auto source = RotationType(1, 2, 3, 4);
        auto output = TestMathFunction<GetElementsNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "X: Number", "Y: Number", "Z: Number", "W: Number" }, { Datum::CreateInitializedCopy(0), Datum::CreateInitializedCopy(0), Datum::CreateInitializedCopy(0), Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(source.GetX(), aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
        EXPECT_FLOAT_EQ(source.GetY(), aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
        EXPECT_FLOAT_EQ(source.GetZ(), aznumeric_caster(*output[2].GetAs<Data::NumberType>()));
        EXPECT_FLOAT_EQ(source.GetW(), aznumeric_caster(*output[3].GetAs<Data::NumberType>()));
    }
#endif

    { // InvertFullNode
        const RotationType source = RotationType::CreateFromVector3AndValue(AZ::Vector3(1, 1, 1), 3);
        auto result = source.GetInverseFull();
        auto output = TestMathFunction<InvertFullNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // IsCloseNode
        Data::RotationType a(1, 1, 1, 1);
        Data::RotationType b(2, 2, 2, 2);
        Data::RotationType c(1.001, 1.001, 1.001, 1.001);

        bool resultFalse = a.IsClose(b);
        auto output = TestMathFunction<IsCloseNode>({ "Rotation: A", "Rotation: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());

        bool resultTrue = a.IsClose(b, Data::ToVectorFloat(2.1));
        output = TestMathFunction<IsCloseNode>({ "Rotation: A", "Rotation: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(2.1) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultTrue = a.IsClose(c);
        output = TestMathFunction<IsCloseNode>({ "Rotation: A", "Rotation: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(c) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(resultTrue, *output[0].GetAs<Data::BooleanType>());

        resultFalse = a.IsClose(b, Data::ToVectorFloat(0.9));
        output = TestMathFunction<IsCloseNode>({ "Rotation: A", "Rotation: B", "Number: Tolerance" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b), Datum::CreateInitializedCopy(0.9) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(resultFalse, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsFinite
        Data::RotationType sourceFinite(1, 1, 1, 1);
        auto result = sourceFinite.IsFinite();
        auto output = TestMathFunction<IsFiniteNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(sourceFinite) }, { "Result: Boolean", }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsIdentityNode
        auto result = identity.IsIdentity();
        auto output = TestMathFunction<IsIdentityNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(identity) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = zero.IsIdentity();
        output = TestMathFunction<IsIdentityNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(zero) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // IsZero
        auto result = zero.IsZero();
        auto output = TestMathFunction<IsZeroNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(zero) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(false) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());

        result = one.IsZero();
        output = TestMathFunction<IsZeroNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(one) }, { "Result: Boolean" }, { Datum::CreateInitializedCopy(true) });
        EXPECT_EQ(result, *output[0].GetAs<Data::BooleanType>());
    }

    { // Length
        Data::RotationType source(1, 2, 3, 4);
        auto result = source.GetLength();
        auto output = TestMathFunction<LengthNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthReciprocal
        Data::RotationType source(1, 2, 3, 4);
        auto result = source.GetLengthReciprocal();
        auto output = TestMathFunction<LengthReciprocalNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // LengthSquared
        Data::RotationType source(1, 2, 3, 4);
        auto result = source.GetLengthSq();
        auto output = TestMathFunction<LengthSquaredNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

    { // Lerp
        Data::RotationType from(zero);
        Data::RotationType to(Data::RotationType::CreateFromAxisAngle(AZ::Vector3(1, 1, 1).GetNormalized(), 0.75));
        Data::NumberType t(0.5);

        auto result = from.Lerp(to, Data::ToVectorFloat(t));
        auto output = TestMathFunction<LerpNode>({ "Rotation: From", "Rotation: To", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(t) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    {   // MultiplyByNumber
        Data::RotationType a(1, 2, 3, 4);
        Data::NumberType b(3.0);
        auto result = a * Data::ToVectorFloat(b);
        auto output = TestMathFunction<MultiplyByNumberNode>({ "Rotation: Source", "Number: Multiplier" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(zero) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    {  // MultiplyByRotation
        Data::RotationType a(1, 1, 1, 1);
        Data::RotationType b(2, 2, 2, 2);
        Data::RotationType c = a * b;
        auto output = TestMathFunction<MultiplyByRotationNode>({ "Rotation: A", "Rotation: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    {   // Negate
        Data::RotationType source = one;
        auto output = TestMathFunction<NegateNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(zero) });
        auto result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));

        source = negativeOne;
        output = TestMathFunction<NegateNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(zero) });
        result = -source;
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // Normalize
        Data::RotationType source = one;
        auto output = TestMathFunction<NormalizeNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(zero) });
        auto result = source.GetNormalized();
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // NormalizeWithLength
        Data::RotationType source = one;
        auto output = TestMathFunction<NormalizeWithLengthNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Normalized: Rotation", "Length: Number" }, { Datum::CreateInitializedCopy(zero), Datum::CreateInitializedCopy(0) });
        auto result = source.NormalizeWithLength();
        EXPECT_TRUE(source.IsClose(*output[0].GetAs<Data::RotationType>()));
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[1].GetAs<Data::NumberType>()));
    }
#endif

    { // RotationXDegreesNode
        auto output = TestMathFunction<RotationXDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(30) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        auto result = RotationType::CreateRotationX(AZ::DegToRad(30));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<RotationType>()));
    }

    { // RotationYDegreesNode
        auto output = TestMathFunction<RotationYDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(30) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        auto result = RotationType::CreateRotationY(AZ::DegToRad(30));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<RotationType>()));
    }

    { // RotationZDegreesNode
        auto output = TestMathFunction<RotationZDegreesNode>({ "Number: Degrees" }, { Datum::CreateInitializedCopy(30) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        auto result = RotationType::CreateRotationZ(AZ::DegToRad(30));
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<RotationType>()));
    }

    { // ShortestArcNode
        Data::Vector3Type from(-1, 0, 0);
        Data::Vector3Type to(1, 1, 1);

        auto result = RotationType::CreateShortestArc(from, to);
        auto output = TestMathFunction<ShortestArcNode>({ "Vector3: From", "Vector3: To" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // Slerp
        Data::RotationType from(zero);
        Data::RotationType to(Data::RotationType::CreateFromAxisAngle(AZ::Vector3(1, 1, 1).GetNormalized(), 0.75));
        Data::NumberType t(0.5);

        auto result = from.Slerp(to, Data::ToVectorFloat(t));
        auto output = TestMathFunction<SlerpNode>({ "Rotation: From", "Rotation: To", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(t) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // SquadNode
        Data::RotationType from(Data::RotationType::CreateFromAxisAngle(Vector3Type(-1, 0, 0).GetNormalized(), 0.75));
        Data::RotationType to(Data::RotationType::CreateFromAxisAngle(Vector3Type(1, 1, 1).GetNormalized(), 0.75));
        Data::RotationType out(Data::RotationType::CreateFromAxisAngle(Vector3Type(1, 0, 1).GetNormalized(), 0.75));
        Data::RotationType in(Data::RotationType::CreateFromAxisAngle(Vector3Type(1, 1, 0).GetNormalized(), 0.75));
        Data::NumberType t(0.5);

        auto result = from.Squad(to, in, out, Data::ToVectorFloat(t));
        auto output = TestMathFunction<SquadNode>({ "Rotation: From", "Rotation: To", "Rotation: In", "Rotation: Out", "Number: T" }, { Datum::CreateInitializedCopy(from), Datum::CreateInitializedCopy(to), Datum::CreateInitializedCopy(in), Datum::CreateInitializedCopy(out), Datum::CreateInitializedCopy(t) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(identity) });
        EXPECT_TRUE(result.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    {   // Subtract
        Data::RotationType a(1, 1, 1, 1);
        Data::RotationType b(2, 2, 2, 2);
        Data::RotationType c = a - b;
        auto output = TestMathFunction<SubtractNode>({ "Rotation: A", "Rotation: B" }, { Datum::CreateInitializedCopy(a), Datum::CreateInitializedCopy(b) }, { "Result: Rotation" }, { Datum::CreateInitializedCopy(a) });
        EXPECT_TRUE(c.IsClose(*output[0].GetAs<Data::RotationType>()));
    }

    { // ToAngleDegreesNode
        Data::RotationType source(Data::RotationType::CreateFromAxisAngle(Vector3Type(-1, 0, 0).GetNormalized(), 0.75));
        auto output = TestMathFunction<ToAngleDegreesNode>({ "Rotation: Source" }, { Datum::CreateInitializedCopy(source) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
        float result = AZ::RadToDeg(source.GetAngle());
        EXPECT_FLOAT_EQ(result, aznumeric_caster(*output[0].GetAs<Data::NumberType>()));
    }

#if ENABLE_EXTENDED_MATH_SUPPORT
    { // GetElement
        const Data::RotationType source(1, 2, 3, 4);

        for (int index = 0; index < 4; ++index)
        {
            float result = aznumeric_caster(Data::FromVectorFloat(source.GetElement(index)));
            auto output = TestMathFunction<GetElementNode>({ "Rotation: Source", "Number: Index" }, { Datum::CreateInitializedCopy(source), Datum::CreateInitializedCopy(index) }, { "Result: Number" }, { Datum::CreateInitializedCopy(0) });
            float outputNumber = aznumeric_caster(*output[0].GetAs<Data::NumberType>());
            EXPECT_FLOAT_EQ(result, outputNumber);
        }
    }
#endif

    // Missing Units Tests:
    // ModXNode
    // ModYNode
    // ModZNode
    // ModWNode
    // ToImaginary

} // Test RotationNode
