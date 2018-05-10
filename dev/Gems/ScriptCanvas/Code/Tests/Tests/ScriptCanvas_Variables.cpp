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

#include <Tests/ScriptCanvasTestFixture.h>
#include <Tests/ScriptCanvasTestUtilities.h>

#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>

namespace ScriptCanvasTests
{
    class StringArray
    {
    public:
        AZ_TYPE_INFO(StringArray, "{0240E221-3800-4BD3-91F3-0304F097F9A7}");

        StringArray() = default;

        static AZStd::string StringArrayToString(AZStd::vector<AZStd::string> inputArray, AZStd::string_view separator = " ")
        {
            if (inputArray.empty())
            {
                return "";
            }

            AZStd::string value;
            auto currentIt = inputArray.begin();
            auto lastIt = inputArray.end();
            for (value = *currentIt; currentIt != lastIt; ++currentIt)
            {
                value += separator;
                value += *currentIt;
            }

            return value;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<StringArray>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<StringArray>("StringArray")
                    ->Method("StringArrayToString", &StringArray::StringArrayToString)
                    ->Method("Equal", [](const StringArray&, const StringArray&) -> bool {return true; }, { {} })
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ;
            }
        }
    };
}

using namespace ScriptCanvasTests;

TEST_F(ScriptCanvasTestFixture, CreateVariableTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    propertyEntity->CreateComponent<GraphVariableManagerComponent>();
    propertyEntity->Init();
    propertyEntity->Activate();

    auto vector3Datum1 = Datum(Data::Vector3Type(1.1f, 2.0f, 3.6f));
    auto vector3Datum2 = Datum(Data::Vector3Type(0.0f, -86.654f, 134.23f));
    auto vector4Datum = Datum(Data::Vector4Type(6.0f, 17.5f, -41.75f, 400.875f));
    
    TestBehaviorContextObject testObject;
    auto behaviorMatrix4x4Datum = Datum(testObject);
        
    auto stringArrayDatum = Datum(StringArray());

    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "FirstVector3", vector3Datum1);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "SecondVector3", vector3Datum2);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "FirstVector4", vector4Datum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "ProjectionMatrix", behaviorMatrix4x4Datum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "My String Array", stringArrayDatum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    AZStd::vector<AZ::Outcome<VariableId, AZStd::string>> addVariablesOutcome;
    AZStd::vector<AZStd::pair<AZStd::string_view, Datum>> datumsToAdd;
    datumsToAdd.emplace_back("FirstBoolean", Datum(true));
    datumsToAdd.emplace_back("FirstString", Datum(AZStd::string("Test")));
    GraphVariableManagerRequestBus::EventResult(addVariablesOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariables<decltype(datumsToAdd)::iterator>, datumsToAdd.begin(), datumsToAdd.end());
    EXPECT_EQ(2, addVariablesOutcome.size());
    EXPECT_TRUE(addVariablesOutcome[0]);
    EXPECT_TRUE(addVariablesOutcome[0].GetValue().IsValid());
    EXPECT_TRUE(addVariablesOutcome[1]);
    EXPECT_TRUE(addVariablesOutcome[1].GetValue().IsValid());
    
    propertyEntity.reset();
    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, AddVariableFailTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    propertyEntity->CreateComponent<GraphVariableManagerComponent>();
    propertyEntity->Init();
    propertyEntity->Activate();

    auto vector3Datum1 = Datum(Data::Vector3Type(1.1f, 2.0f, 3.6f));
    auto vector3Datum2 = Datum(Data::Vector3Type(0.0f, -86.654f, 134.23f));

    const AZStd::string_view propertyName = "SameName";
   
    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, propertyName, vector3Datum1);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, propertyName, vector3Datum2);
    EXPECT_FALSE(addPropertyOutcome);

    propertyEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, RemoveVariableTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    propertyEntity->CreateComponent<GraphVariableManagerComponent>();
    propertyEntity->Init();
    propertyEntity->Activate();

    auto vector3Datum1 = Datum(Data::Vector3Type(1.1f, 2.0f, 3.6f));
    auto vector3Datum2 = Datum(Data::Vector3Type(0.0f, -86.654f, 134.23f));
    auto vector4Datum = Datum(Data::Vector4Type(6.0f, 17.5f, -41.75f, 400.875f));
    
    TestBehaviorContextObject testObject;
    auto behaviorMatrix4x4Datum = Datum(testObject);
	
    auto stringArrayDatum = Datum(StringArray());

    size_t numVariablesAdded = 0U;
    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "FirstVector3", vector3Datum1);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId firstVector3Id = addPropertyOutcome.GetValue();
    ++numVariablesAdded;

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "SecondVector3", vector3Datum2);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId secondVector3Id = addPropertyOutcome.GetValue();
    ++numVariablesAdded;

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "FirstVector4", vector4Datum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId firstVector4Id = addPropertyOutcome.GetValue();
    ++numVariablesAdded;

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "ProjectionMatrix", behaviorMatrix4x4Datum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId projectionMatrixId = addPropertyOutcome.GetValue();
    ++numVariablesAdded;

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "My String Array", stringArrayDatum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId stringArrayId = addPropertyOutcome.GetValue();
    ++numVariablesAdded;

    AZStd::vector<AZ::Outcome<VariableId, AZStd::string>> addVariablesOutcome;
    AZStd::vector<AZStd::pair<AZStd::string_view, Datum>> datumsToAdd;
    datumsToAdd.emplace_back("FirstBoolean", Datum(true));
    datumsToAdd.emplace_back("FirstString", Datum(AZStd::string("Test")));
    GraphVariableManagerRequestBus::EventResult(addVariablesOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariables<decltype(datumsToAdd)::iterator>, datumsToAdd.begin(), datumsToAdd.end());
    EXPECT_EQ(2, addVariablesOutcome.size());
    EXPECT_TRUE(addVariablesOutcome[0]);
    EXPECT_TRUE(addVariablesOutcome[0].GetValue().IsValid());
    EXPECT_TRUE(addVariablesOutcome[1]);
    EXPECT_TRUE(addVariablesOutcome[1].GetValue().IsValid());
    numVariablesAdded += addVariablesOutcome.size();

    const AZStd::unordered_map<VariableId, VariableNameValuePair>* properties{};
    GraphVariableManagerRequestBus::EventResult(properties, propertyEntity->GetId(), &GraphVariableManagerRequests::GetVariables);
    ASSERT_NE(nullptr, properties);
    EXPECT_EQ(numVariablesAdded, (*properties).size());

    {
        // Remove Property By Id
        bool removePropertyResult = false;
        GraphVariableManagerRequestBus::EventResult(removePropertyResult, propertyEntity->GetId(), &GraphVariableManagerRequests::RemoveVariable, stringArrayId);
        EXPECT_TRUE(removePropertyResult);

        properties = {};
        GraphVariableManagerRequestBus::EventResult(properties, propertyEntity->GetId(), &GraphVariableManagerRequests::GetVariables);
        ASSERT_NE(nullptr, properties);
        EXPECT_EQ(numVariablesAdded, (*properties).size() + 1);

        // Attempt to remove already removed property
        GraphVariableManagerRequestBus::EventResult(removePropertyResult, propertyEntity->GetId(), &GraphVariableManagerRequests::RemoveVariable, stringArrayId);
        EXPECT_FALSE(removePropertyResult);
    }

    {
        // Remove Property by name
        size_t numVariablesRemoved = 0U;
        GraphVariableManagerRequestBus::EventResult(numVariablesRemoved, propertyEntity->GetId(), &GraphVariableManagerRequests::RemoveVariableByName, "ProjectionMatrix");
        EXPECT_EQ(1U, numVariablesRemoved);

        properties = {};
        GraphVariableManagerRequestBus::EventResult(properties, propertyEntity->GetId(), &GraphVariableManagerRequests::GetVariables);
        ASSERT_NE(nullptr, properties);
        EXPECT_EQ(numVariablesAdded, (*properties).size() + 2);

        // Attempt to remove property again.
        GraphVariableManagerRequestBus::EventResult(numVariablesRemoved, propertyEntity->GetId(), &GraphVariableManagerRequests::RemoveVariableByName, "ProjectionMatrix");
        EXPECT_EQ(0U, numVariablesRemoved);
    }

    {
        // Re-add removed Property
        addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
        GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "ProjectionMatrix", behaviorMatrix4x4Datum);
        EXPECT_TRUE(addPropertyOutcome);
        EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
        
        properties = {};
        GraphVariableManagerRequestBus::EventResult(properties, propertyEntity->GetId(), &GraphVariableManagerRequests::GetVariables);
        EXPECT_EQ(numVariablesAdded, (*properties).size() + 1);
    }

    propertyEntity.reset();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, FindVariableTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    propertyEntity->CreateComponent<GraphVariableManagerComponent>();
    propertyEntity->Init();
    propertyEntity->Activate();

    auto stringVariableDatum = Datum(Data::StringType("SABCDQPE"));

    const AZStd::string_view propertyName = "StringProperty";

    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, propertyName, stringVariableDatum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId stringVariableId = addPropertyOutcome.GetValue();

    VariableDatum* propertyDatumByName{};
    {
        // Find Property by name
        GraphVariableManagerRequestBus::EventResult(propertyDatumByName, propertyEntity->GetId(), &GraphVariableManagerRequests::FindVariable, propertyName);
        ASSERT_NE(nullptr, propertyDatumByName);
        EXPECT_EQ(stringVariableDatum, propertyDatumByName->GetData());
    }

    VariableNameValuePair* propertyPair{};
    VariableDatum* propertyDatumById;
    {
        // Find Property by id
        GraphVariableManagerRequestBus::EventResult(propertyPair, propertyEntity->GetId(), &GraphVariableManagerRequests::FindVariableById, stringVariableId);
        ASSERT_NE(nullptr, propertyPair);
        propertyDatumById = &propertyPair->m_varDatum;
        EXPECT_EQ(stringVariableDatum, propertyDatumById->GetData());
        EXPECT_EQ(*propertyDatumById, *propertyDatumByName); 
    }

    {
        // Remove Property
        size_t numVariablesRemoved = false;
        GraphVariableManagerRequestBus::EventResult(numVariablesRemoved, propertyEntity->GetId(), &GraphVariableManagerRequests::RemoveVariableByName, propertyName);
        EXPECT_EQ(1U, numVariablesRemoved);
    }

    {
        // Attempt to re-lookup property
        propertyPair = {};
        GraphVariableManagerRequestBus::EventResult(propertyDatumById, propertyEntity->GetId(), &GraphVariableManagerRequests::FindVariable, propertyName);
        EXPECT_EQ(nullptr, propertyDatumById);

        GraphVariableManagerRequestBus::EventResult(propertyPair, propertyEntity->GetId(), &GraphVariableManagerRequests::FindVariableById, stringVariableId);
        EXPECT_EQ(nullptr, propertyPair);
    }

    propertyEntity.reset();
}

TEST_F(ScriptCanvasTestFixture, ModifyVariableTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    propertyEntity->CreateComponent<GraphVariableManagerComponent>();
    propertyEntity->Init();
    propertyEntity->Activate();

    auto stringVariableDatum = Datum(Data::StringType("Test1"));

    const AZStd::string_view propertyName = "StringProperty";

    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, propertyName, stringVariableDatum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());
    const VariableId stringVariableId = addPropertyOutcome.GetValue();

    VariableDatum* propertyDatum{};
    GraphVariableManagerRequestBus::EventResult(propertyDatum, propertyEntity->GetId(), &GraphVariableManagerRequests::FindVariable, propertyName);
    ASSERT_NE(nullptr, propertyDatum);

    // Modify the added property
    AZStd::string_view modifiedString = "High Functioning S... *<silenced>";
    auto testString = propertyDatum->GetData().ModAs<Data::StringType>();
    ASSERT_NE(nullptr, testString);
    *testString = modifiedString;

    // Re-lookup Property and test against modifiedString
    VariableNameValuePair* propertyPair{};
    GraphVariableManagerRequestBus::EventResult(propertyPair, propertyEntity->GetId(), &GraphVariableManagerRequests::FindVariableById, stringVariableId);
    ASSERT_NE(nullptr, propertyPair);
    propertyDatum = &propertyPair->m_varDatum;
    auto resultString = propertyDatum->GetData().ModAs<Data::StringType>();
    ASSERT_NE(nullptr, resultString);
    EXPECT_EQ(modifiedString, *resultString);
}

TEST_F(ScriptCanvasTestFixture, SerializationTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> propertyEntity = AZStd::make_unique<AZ::Entity>("PropertyGraph");
    auto propertyEntityId = propertyEntity->GetId();
    propertyEntity->CreateComponent<GraphVariableManagerComponent>();
    propertyEntity->Init();
    propertyEntity->Activate();

    auto stringArrayDatum = Datum(StringArray());

    AZ::Outcome<VariableId, AZStd::string> addPropertyOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntity->GetId(), &GraphVariableManagerRequests::AddVariable, "My String Array", stringArrayDatum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    VariableDatum* stringArrayProperty{};
    GraphVariableManagerRequestBus::EventResult(stringArrayProperty, propertyEntityId, &GraphVariableManagerRequests::FindVariable, "My String Array");
    ASSERT_NE(nullptr, stringArrayProperty);
    EXPECT_EQ(stringArrayDatum, stringArrayProperty->GetData());
    const VariableId stringArrayVariableId = stringArrayProperty->GetId();

    // Save Property Component Entity
    AZStd::vector<AZ::u8> binaryBuffer;
    AZ::IO::ByteContainerStream<decltype(binaryBuffer)> byteStream(&binaryBuffer);
    const bool objectSaved = AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, propertyEntity.get(), m_serializeContext);
    EXPECT_TRUE(objectSaved);

    // Delete the Property Component 
    propertyEntity.reset();

    // Attempt to lookup the My String Array property using the old property component entity id
    // PropertyRequestBus should be disconnected
    stringArrayProperty = {};
    GraphVariableManagerRequestBus::EventResult(stringArrayProperty, propertyEntityId, &GraphVariableManagerRequests::FindVariable, "My String Array");
    EXPECT_EQ(nullptr, stringArrayProperty);

    // Attempt to add a new property after deleting the Property Component Entity
    auto identityMatrixDatum = Datum(Data::Matrix3x3Type::CreateIdentity());
    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntityId, &GraphVariableManagerRequests::AddVariable, "Super Matrix Bros", identityMatrixDatum);
    EXPECT_FALSE(addPropertyOutcome);

    // Load Variable Component Entity
    {
        byteStream.Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        propertyEntity.reset(AZ::Utils::LoadObjectFromStream<AZ::Entity>(byteStream, m_serializeContext));
        ASSERT_TRUE(propertyEntity);
        propertyEntity->Init();
        propertyEntity->Activate();
    }

    // Attempt to lookup the My String Array property after loading from object stream
    stringArrayProperty = {};
    GraphVariableManagerRequestBus::EventResult(stringArrayProperty, propertyEntityId, &GraphVariableManagerRequests::FindVariable, "My String Array");
    ASSERT_NE(nullptr, stringArrayProperty);
    EXPECT_EQ(stringArrayVariableId, stringArrayProperty->GetId());

    addPropertyOutcome = AZ::Failure(AZStd::string("Uninitialized"));
    GraphVariableManagerRequestBus::EventResult(addPropertyOutcome, propertyEntityId, &GraphVariableManagerRequests::AddVariable, "Super Matrix Bros", identityMatrixDatum);
    EXPECT_TRUE(addPropertyOutcome);
    EXPECT_TRUE(addPropertyOutcome.GetValue().IsValid());

    VariableNameValuePair* propertyPair{};
    GraphVariableManagerRequestBus::EventResult(propertyPair, propertyEntityId, &GraphVariableManagerRequests::FindVariableById, addPropertyOutcome.GetValue());
    ASSERT_NE(nullptr, propertyPair);
    VariableDatum* superMatrixProperty = &propertyPair->m_varDatum;
    const Datum& matrix3x3Datum= superMatrixProperty->GetData();
    EXPECT_EQ(identityMatrixDatum, matrix3x3Datum);

    propertyEntity.reset();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    StringArray::Reflect(m_serializeContext);
    StringArray::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, GetVariableNodeTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace Nodes;
    using namespace TestNodes;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("VariableGraph");
    SystemRequestBus::Broadcast(&SystemRequests::CreateEngineComponentsOnEntity, graphEntity.get());
    Graph* graph = AZ::EntityUtils::FindFirstDerivedComponent<Graph>(graphEntity.get());
    ASSERT_NE(nullptr, graph);

    AZ::EntityId graphEntityId = graphEntity->GetId();
    AZ::EntityId graphUniqueId = graph->GetUniqueId();

    graphEntity->Init();

    Datum planeDatum(Data::PlaneType::CreateFromCoefficients(3.0f, -1.0f, 2.0f, 0.0f));

    // Add in Plane Variable to Variable Component
    AZStd::string_view variableName = "TestPlane";
    AZ::Outcome<VariableId, AZStd::string> addVariableOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addVariableOutcome, graphUniqueId, &GraphVariableManagerRequests::AddVariable, variableName, planeDatum);
    EXPECT_TRUE(addVariableOutcome);
    EXPECT_TRUE(addVariableOutcome.GetValue().IsValid());
    const VariableId planeVariableId = addVariableOutcome.GetValue();

    // Create Get Variable Node
    AZ::EntityId outID;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, outID);
    auto getVariableNode = CreateTestNode<Nodes::Core::GetVariableNode>(graphUniqueId, outID);
    auto getNormalNode = CreateTestNode<PlaneNodes::GetNormalNode>(graphUniqueId, outID);

    auto vector3ResultNode = CreateDataNode(graphUniqueId, Data::Vector3Type::CreateZero(), outID);

    auto printNode = CreateTestNode<TestResult>(graphUniqueId, outID);
    auto normalResultTestResultNode = CreateTestNode<TestResult>(graphUniqueId, outID);
    auto planeDistanceTestResultNode = CreateTestNode<TestResult>(graphUniqueId, outID);

    // data
    // This should fail to connect until the variableNode has a valid Variable associated with it
    {
        ScopedOutputSuppression supressOutput;
        EXPECT_FALSE(graph->Connect(getVariableNode->GetEntityId(), getVariableNode->GetDataOutSlotId(), getNormalNode->GetEntityId(), getNormalNode->GetSlotId("Plane: Source")));
    }
    EXPECT_FALSE(getVariableNode->GetId().IsValid());
    EXPECT_FALSE(getVariableNode->GetDataOutSlotId().IsValid());
    getVariableNode->SetId(planeVariableId); // This associates the variable with the node and adds the input slot
    auto variableDataOutSlotId = getVariableNode->GetDataOutSlotId();
    EXPECT_TRUE(graph->Connect(getVariableNode->GetEntityId(), variableDataOutSlotId, getNormalNode->GetEntityId(), getNormalNode->GetSlotId("Plane: Source")));
    EXPECT_TRUE(graph->Connect(getVariableNode->GetEntityId(), variableDataOutSlotId, printNode->GetEntityId(), printNode->GetSlotId("Value")));

    // Connects Get Variable Node(normal: Vector3) data output slot to the TestResult Node(Set) data input slot
    // Connects Get Variable Node(distance: Vector3) data output slot to the TestResult Node(Set) data input slot
    auto normalDataOutSlotId = getVariableNode->GetSlotId("normal: Vector3");
    EXPECT_TRUE(graph->Connect(getVariableNode->GetEntityId(), normalDataOutSlotId, normalResultTestResultNode->GetEntityId(), normalResultTestResultNode->GetSlotId("Value")));
    auto distanceDataOutSlotId = getVariableNode->GetSlotId("distance: Number");
    EXPECT_TRUE(graph->Connect(getVariableNode->GetEntityId(), distanceDataOutSlotId, planeDistanceTestResultNode->GetEntityId(), planeDistanceTestResultNode->GetSlotId("Value")));

    EXPECT_TRUE(Connect(*graph, getNormalNode->GetEntityId(), "Result: Vector3", vector3ResultNode->GetEntityId(), "Set"));

    // logic
    EXPECT_TRUE(Connect(*graph, startNode->GetEntityId(), "Out", getVariableNode->GetEntityId(), "In"));
    EXPECT_TRUE(Connect(*graph, getVariableNode->GetEntityId(), "Out", getNormalNode->GetEntityId(), "In"));
    EXPECT_TRUE(Connect(*graph, getVariableNode->GetEntityId(), "Out", printNode->GetEntityId(), "In"));
    EXPECT_TRUE(Connect(*graph, getVariableNode->GetEntityId(), "Out", normalResultTestResultNode->GetEntityId(), "In"));
    EXPECT_TRUE(Connect(*graph, getVariableNode->GetEntityId(), "Out", planeDistanceTestResultNode->GetEntityId(), "In"));

    // execute
    {
        ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();
    }

    EXPECT_FALSE(graph->IsInErrorState());
    graphEntity->Deactivate();

    AZ::Entity* connectionEntity{};
    EXPECT_TRUE(graph->FindConnection(connectionEntity, { getVariableNode->GetEntityId(), variableDataOutSlotId }, { getNormalNode->GetEntityId(), getNormalNode->GetSlotId("Plane: Source") }));
    getVariableNode->SetId({});
    EXPECT_FALSE(graph->FindConnection(connectionEntity, { getVariableNode->GetEntityId(), variableDataOutSlotId }, { getNormalNode->GetEntityId(), getNormalNode->GetSlotId("Plane: Source") }));
    EXPECT_FALSE(getVariableNode->GetId().IsValid());
    EXPECT_FALSE(getVariableNode->GetDataOutSlotId().IsValid());

    VariableDatum* variableDatum{};
    GraphVariableManagerRequestBus::EventResult(variableDatum, graphUniqueId, &GraphVariableManagerRequests::FindVariable, variableName);
    ASSERT_NE(nullptr, variableDatum);

    auto variablePlane = variableDatum->GetData().ModAs<Data::PlaneType>();
    ASSERT_NE(nullptr, variablePlane);

    auto getResultPlane = printNode->GetInput_UNIT_TEST<Data::PlaneType>("Value");
    ASSERT_NE(nullptr, getResultPlane);
    EXPECT_EQ(*variablePlane, *getResultPlane);

    auto resultNormal = vector3ResultNode->GetInput_UNIT_TEST<Data::Vector3Type>("Set");
    ASSERT_NE(nullptr, resultNormal);
    auto expectedNormal = variablePlane->GetNormal();
    EXPECT_EQ(expectedNormal, *resultNormal);

    auto planeNormalPropertyVector3 = normalResultTestResultNode->GetInput_UNIT_TEST<Data::Vector3Type>("Value");
    ASSERT_NE(nullptr, planeNormalPropertyVector3);
    EXPECT_EQ(Data::Vector3Type(3.0f, -1.0f, 2.0f), *planeNormalPropertyVector3);

    auto planeDistancePropertyNumber = planeDistanceTestResultNode->GetInput_UNIT_TEST<Data::NumberType>("Value");
    ASSERT_NE(nullptr, planeDistancePropertyNumber);
    EXPECT_EQ(0.0f, *planeDistancePropertyNumber);
}

TEST_F(ScriptCanvasTestFixture, SetVariableNodeTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("VariableGraph");
    SystemRequestBus::Broadcast(&SystemRequests::CreateEngineComponentsOnEntity, graphEntity.get());
    Graph* graph = AZ::EntityUtils::FindFirstDerivedComponent<Graph>(graphEntity.get());
    ASSERT_NE(nullptr, graph);

    AZ::EntityId graphEntityId = graphEntity->GetId();
    AZ::EntityId graphUniqueId = graph->GetUniqueId();

    graphEntity->Init();

    Datum planeDatum(Data::PlaneType::CreateFromCoefficients(0.0f, 0.0f, 0.0f, 0.0f));

    // Add in Plane Variable to Variable Component
    AZStd::string_view varName = "TestPlane";
    AZ::Outcome<VariableId, AZStd::string> addVariableOutcome(AZ::Failure(AZStd::string("Uninitialized")));
    GraphVariableManagerRequestBus::EventResult(addVariableOutcome, graphUniqueId, &GraphVariableManagerRequests::AddVariable, varName, planeDatum);
    EXPECT_TRUE(addVariableOutcome);
    EXPECT_TRUE(addVariableOutcome.GetValue().IsValid());
    const VariableId planeVariableId = addVariableOutcome.GetValue();

    // Create Set Variable Node
    AZ::EntityId outID;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, outID);
    auto setVariableNode = CreateTestNode<Nodes::Core::SetVariableNode>(graphUniqueId, outID);
    auto fromNormalAndPointNode = CreateTestNode<PlaneNodes::FromNormalAndPointNode>(graphUniqueId, outID);

    auto testPlane = Data::PlaneType::CreateFromNormalAndPoint(Data::Vector3Type(3.0f, -1.0f, 2.0f), Data::Vector3Type::CreateZero());
    auto vector3NormalNode = CreateDataNode(graphUniqueId, testPlane.GetNormal(), outID);
    auto vector3PointNode = CreateDataNode(graphUniqueId, Data::Vector3Type::CreateZero(), outID);

    // data
    EXPECT_TRUE(Connect(*graph, vector3NormalNode->GetEntityId(), "Get", fromNormalAndPointNode->GetEntityId(), "Vector3: Normal"));
    EXPECT_TRUE(Connect(*graph, vector3PointNode->GetEntityId(), "Get", fromNormalAndPointNode->GetEntityId(), "Vector3: Point"));

    // This should fail to connect until the SetVariableNode has a valid Variable associated with it
    {
        ScopedOutputSuppression suppressOutput;
        EXPECT_FALSE(graph->Connect(fromNormalAndPointNode->GetEntityId(), fromNormalAndPointNode->GetSlotId("Plane: Result"), setVariableNode->GetEntityId(), setVariableNode->GetDataInSlotId()));
    }
    EXPECT_FALSE(setVariableNode->GetId().IsValid());
    EXPECT_FALSE(setVariableNode->GetDataInSlotId().IsValid());
    setVariableNode->SetId(planeVariableId); // This associates the variable with the node and adds the input slot
    auto dataInputSlotId = setVariableNode->GetDataInSlotId();
    EXPECT_TRUE(graph->Connect(fromNormalAndPointNode->GetEntityId(), fromNormalAndPointNode->GetSlotId("Result: Plane"), setVariableNode->GetEntityId(), dataInputSlotId));

    // logic
    EXPECT_TRUE(Connect(*graph, startNode->GetEntityId(), "Out", fromNormalAndPointNode->GetEntityId(), "In"));
    EXPECT_TRUE(Connect(*graph, fromNormalAndPointNode->GetEntityId(), "Out", setVariableNode->GetEntityId(), "In"));

    // execute
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    graphEntity->Deactivate();

    AZ::Entity* connectionEntity{};
    EXPECT_TRUE(graph->FindConnection(connectionEntity, { fromNormalAndPointNode->GetEntityId(), fromNormalAndPointNode->GetSlotId("Result: Plane") }, { setVariableNode->GetEntityId(), dataInputSlotId }));
    setVariableNode->SetId({});
    EXPECT_FALSE(graph->FindConnection(connectionEntity, { fromNormalAndPointNode->GetEntityId(), fromNormalAndPointNode->GetSlotId("Result: Plane") }, { setVariableNode->GetEntityId(), dataInputSlotId }));
    EXPECT_FALSE(setVariableNode->GetId().IsValid());
    EXPECT_FALSE(setVariableNode->GetDataInSlotId().IsValid());

    VariableDatum* variableDatum{};
    GraphVariableManagerRequestBus::EventResult(variableDatum, graphUniqueId, &GraphVariableManagerRequests::FindVariable, varName);
    ASSERT_NE(nullptr, variableDatum);

    // Get Variable Plane and verify that it is the same as the plane created from
    auto variablePlane = variableDatum->GetData().ModAs<Data::PlaneType>();
    ASSERT_NE(nullptr, variablePlane);

    EXPECT_EQ(testPlane, *variablePlane);
}