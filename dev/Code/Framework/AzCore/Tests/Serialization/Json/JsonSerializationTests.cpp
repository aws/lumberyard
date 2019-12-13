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

#include <AzCore/JSON/pointer.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerMock.h>
#include <Tests/Serialization/Json/TestCases.h>

namespace JsonSerializationTests
{
    namespace A
    {
        struct Inherited
            : public BaseClass
        {
            AZ_RTTI(Inherited, "{E7829F37-C577-4F2B-A85B-6F331548354C}", BaseClass);

            ~Inherited() override = default;
        };
    }

    namespace B
    {
        struct Inherited
            : public BaseClass
        {
            AZ_RTTI(Inherited, "{0DF033C4-3EEC-4F61-8B79-59FE31545029}", BaseClass);

            ~Inherited() override = default;
        };
    }

    namespace C
    {
        struct Inherited
        {
            AZ_RTTI(Inherited, "{682089DB-9794-4590-87A4-9AF70BD1C202}");
        };
    }

    struct ConflictingNameTestClass
    {
        AZ_RTTI(ConflictingNameTestClass, "{370EFD10-781B-47BF-B0A1-6FC4E9D55CBC}");

        virtual ~ConflictingNameTestClass() = default;

        AZStd::shared_ptr<BaseClass> m_pointer;
        ConflictingNameTestClass()
        {
            m_pointer = AZStd::make_shared<A::Inherited>();
        }
    };

    class JsonSerializationTests
        : public BaseJsonSerializerFixture
    {
    public:
        ~JsonSerializationTests() override = default;
    };

    template<typename T>
    class TypedJsonSerializationTests 
        : public JsonSerializationTests
    {
    public:
        using SerializableStruct = T;

        ~TypedJsonSerializationTests() override = default;

        void Reflect(bool fullReflection)
        {
            SerializableStruct::Reflect(m_serializeContext, fullReflection);
            m_fullyReflected = fullReflection;
        }

        bool m_fullyReflected = true;
    };

    TYPED_TEST_CASE(TypedJsonSerializationTests, JsonSerializationTestCases);

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializedDefaultInstance_EmptyJsonReturned)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings.m_keepDefaults = false;

        TypeParam instance;
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(), instance, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        this->Expect_DocStrEq("{}");
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_EmptyJson_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_jsonDocument->Parse("{}");

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::DefaultsUsed, loadResult.GetOutcome());
        
        TypeParam expectedInstance;
        EXPECT_TRUE(loadInstance.Equals(expectedInstance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializeWithoutDefaults_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings.m_keepDefaults = false;

        auto description = TypeParam::GetInstanceWithoutDefaults();
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, this->m_serializationSettings);
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        this->Expect_DocStrEq(description.m_json);
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_JsonWithoutDefaults_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        auto description = TypeParam::GetInstanceWithoutDefaults();
        this->m_jsonDocument->Parse(description.m_json);

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, loadResult.GetOutcome());
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializeWithSomeDefaults_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings.m_keepDefaults = false;

        auto description = TypeParam::GetInstanceWithSomeDefaults();
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, this->m_serializationSettings);
        
        bool partialDefaultsSupported = TypeParam::SupportsPartialDefaults;
        EXPECT_EQ(partialDefaultsSupported ? Outcomes::PartialDefaults : Outcomes::DefaultsUsed, result.GetOutcome());
        this->Expect_DocStrEq(description.m_jsonWithStrippedDefaults);
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_JsonWithSomeDefaults_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        auto description = TypeParam::GetInstanceWithSomeDefaults();
        this->m_jsonDocument->Parse(description.m_jsonWithStrippedDefaults);

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, this->m_deserializationSettings);
        bool validResult =
            loadResult.GetOutcome() == Outcomes::Success ||
            loadResult.GetOutcome() == Outcomes::DefaultsUsed ||
            loadResult.GetOutcome() == Outcomes::PartialDefaults;
        EXPECT_TRUE(validResult);
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializeWithSomeDefaultsKept_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings.m_keepDefaults = true;

        auto description = TypeParam::GetInstanceWithSomeDefaults();
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, this->m_serializationSettings);
        
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        this->Expect_DocStrEq(description.m_jsonWithKeptDefaults);
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_JsonWithSomeDefaultsKept_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        auto description = TypeParam::GetInstanceWithSomeDefaults();
        this->m_jsonDocument->Parse(description.m_jsonWithKeptDefaults);

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, loadResult.GetOutcome());
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializeWithUnknownType_UnknownTypeReturned)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(false);
        this->m_serializationSettings.m_keepDefaults = true;

        auto description = TypeParam::GetInstanceWithoutDefaults();
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
    }

    TYPED_TEST(TypedJsonSerializationTests, LoadAndStore_RoundTrip_StoredObjectCanBeLoadedAgain)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings.m_keepDefaults = true;
        auto description = TypeParam::GetInstanceWithoutDefaults();

        ResultCode storeResult = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, this->m_serializationSettings);
        ASSERT_NE(Processing::Halted, storeResult.GetProcessing());

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, this->m_deserializationSettings);
        ASSERT_NE(Processing::Halted, loadResult.GetProcessing());
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_JsonAdditionalFields_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        auto description = TypeParam::GetInstanceWithoutDefaults();
        this->m_jsonDocument->Parse(description.m_json);
        this->InjectAdditionalFields(*this->m_jsonDocument, rapidjson::kStringType, this->m_jsonDocument->GetAllocator());

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, this->m_deserializationSettings);
        ASSERT_NE(Processing::Halted, loadResult.GetProcessing());
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }



    // Additional tests for specific scenarios that didn't fit in the typed versions.


    // Load
    TEST_F(JsonSerializationTests, Load_PrimitiveAtTheRoot_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse("true");

        bool loadValue = false;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadValue, *m_jsonDocument, m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, loadResult.GetOutcome());
        EXPECT_TRUE(loadValue);
    }

    TEST_F(JsonSerializationTests, Load_ArrayAtTheRoot_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        // Since there's no class that will register the container as part of the reflection process
        // explicitly register the container by forcing the creation of generic info specifically
        // for the containers type and template argument.
        auto genericInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<int>>::GetGenericInfo();
        ASSERT_NE(nullptr, genericInfo);
        genericInfo->Reflect(m_serializeContext.get());

        m_jsonDocument->Parse("[13,42,88]");

        AZStd::vector<int> loadValues;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadValues, *m_jsonDocument, m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, loadResult.GetOutcome());
        EXPECT_EQ(loadValues, AZStd::vector<int>({ 13, 42, 88 }));
    }

    TEST_F(JsonSerializationTests, Load_TypeWithWhitespace_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        ComplexNullInheritedPointer::Reflect(m_serializeContext, true);

        m_jsonDocument->Parse(
            R"({
                    "pointer": 
                    {
                        "$type": "    SimpleInheritence    "
                    }
                })");

        ComplexNullInheritedPointer instance;
        ResultCode loadResult = AZ::JsonSerialization::Load(instance, *m_jsonDocument, m_deserializationSettings);
        ASSERT_EQ(Outcomes::DefaultsUsed, loadResult.GetOutcome());
    
        ASSERT_NE(nullptr, instance.m_pointer);
        EXPECT_EQ(azrtti_typeid(instance.m_pointer), azrtti_typeid <SimpleInheritence>());
    }

    // Tests if a "$type" that's not needed doesn't have side effects because it doesn't give a polymorphic instance.
    TEST_F(JsonSerializationTests, Load_PointerToSameClass_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        ComplexNullInheritedPointer::Reflect(m_serializeContext, true);

        m_jsonDocument->Parse(
            R"({
                    "pointer": 
                    {
                        "$type": "BaseClass"
                    }
                })");

        ComplexNullInheritedPointer instance;
        ResultCode loadResult = AZ::JsonSerialization::Load(instance, *m_jsonDocument, m_deserializationSettings);
        ASSERT_EQ(Outcomes::DefaultsUsed, loadResult.GetOutcome());

        ASSERT_NE(nullptr, instance.m_pointer);
        EXPECT_EQ(azrtti_typeid(instance.m_pointer), azrtti_typeid<BaseClass>());
    }

    TEST_F(JsonSerializationTests, Load_InvalidPointerName_FailsToConvert)
    {
        using namespace AZ::JsonSerializationResult;

        ComplexNullInheritedPointer::Reflect(m_serializeContext, true);

        m_jsonDocument->Parse(
            R"({
                    "pointer": 
                    {
                        "$type": "Invalid"
                    }
                })");

        ComplexNullInheritedPointer instance;
        ResultCode loadResult = AZ::JsonSerialization::Load(instance, *m_jsonDocument, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unknown, loadResult.GetOutcome());
        EXPECT_EQ(Processing::Halted, loadResult.GetProcessing());
    }

    TEST_F(JsonSerializationTests, Load_UnrelatedPointerType_FailsToCast)
    {
        using namespace AZ::JsonSerializationResult;

        ComplexNullInheritedPointer::Reflect(m_serializeContext, true);
        SimpleClass::Reflect(m_serializeContext, true);

        m_jsonDocument->Parse(
            R"({
                    "pointer": 
                    {
                        "$type": "SimpleClass"
                    }
                })");

        ComplexNullInheritedPointer instance;
        ResultCode loadResult = AZ::JsonSerialization::Load(instance, *m_jsonDocument, m_deserializationSettings);
        EXPECT_EQ(Outcomes::TypeMismatch, loadResult.GetOutcome());
        EXPECT_EQ(Processing::Halted, loadResult.GetProcessing());
    }

    // Store
    TEST_F(JsonSerializationTests, Store_PrimitiveAtTheRoot_ReturnsSuccessAndTheValueAtTheRoot)
    {
        using namespace AZ::JsonSerializationResult;

        m_serializationSettings.m_keepDefaults = true;

        bool value = true;
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), value, m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        Expect_DocStrEq("true");
    }

    TEST_F(JsonSerializationTests, Store_ArrayAtTheRoot_ReturnsSuccessAndTheArrayAtTheRoot)
    {
        using namespace AZ::JsonSerializationResult;

        // Since there's no class that will register the container as part of the reflection process
        // explicitly register the container by forcing the creation of generic info specifically
        // for the containers type and template argument.
        auto genericInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<int>>::GetGenericInfo();
        ASSERT_NE(nullptr, genericInfo);
        genericInfo->Reflect(m_serializeContext.get());

        m_serializationSettings.m_keepDefaults = true;

        AZStd::vector<int> values = { 13, 42, 88 };
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), values, m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        Expect_DocStrEq("[13,42,88]");
    }

    TEST_F(JsonSerializationTests, Store_PointerToClassWithSameNames_SuccessWithFullyQualifiedName)
    {
        using namespace AZ::JsonSerializationResult;

        BaseClass::Reflect(m_serializeContext);
        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<B::Inherited, BaseClass>();
        m_serializeContext->Class<ConflictingNameTestClass>()
            ->Field("pointer", &ConflictingNameTestClass::m_pointer);

        m_serializationSettings.m_keepDefaults = true;

        ConflictingNameTestClass instance;
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instance, m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());

        rapidjson::Value* pointerType = rapidjson::Pointer("/pointer/$type").Get(*m_jsonDocument);
        ASSERT_NE(nullptr, pointerType);
        const char* type = pointerType->GetString();
        EXPECT_STRCASEEQ("{E7829F37-C577-4F2B-A85B-6F331548354C} Inherited", type);
    }

    TEST_F(JsonSerializationTests, Load_TemplatedClassWithRegisteredHandler_LoadOnHandlerCalled)
    {
        using namespace AZ::JsonSerializationResult;
        using namespace ::testing;

        m_jsonDocument->Parse(
            R"({
                    "var1": 188, 
                    "var2": 288
                })");
        
        TemplatedClass<int>::Reflect(m_serializeContext, true);
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<TemplatedClass>();
        JsonSerializerMock* mock = reinterpret_cast<JsonSerializerMock*>(
            m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<TemplatedClass>()));
        EXPECT_CALL(*mock, Load(_, _, _, _, _))
            .Times(Exactly(1))
            .WillRepeatedly(Return(Result(m_deserializationSettings, "Test", ResultCode::Success(Tasks::ReadField), "")));
        
        TemplatedClass<int> instance;
        AZ::JsonSerialization::Load(instance, *m_jsonDocument, m_deserializationSettings);

        m_serializeContext->EnableRemoveReflection();
        TemplatedClass<int>::Reflect(m_serializeContext, true);
        m_serializeContext->DisableRemoveReflection();

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<TemplatedClass>();
        m_jsonRegistrationContext->DisableRemoveReflection();
       
    }

    TEST_F(JsonSerializationTests, Store_TemplatedClassWithRegisteredHandler_StoreOnHandlerCalled)
    {
        using namespace AZ::JsonSerializationResult;
        using namespace ::testing;

        TemplatedClass<int>::Reflect(m_serializeContext, true);
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<TemplatedClass>();
        JsonSerializerMock* mock = reinterpret_cast<JsonSerializerMock*>(
            m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<TemplatedClass>()));
        EXPECT_CALL(*mock, Store(_, _, _, _, _, _, _))
            .Times(Exactly(1))
            .WillRepeatedly(Return(Result(m_serializationSettings, "Test", ResultCode::Success(Tasks::WriteValue), "")));

        TemplatedClass<int> instance;
        AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instance, m_serializationSettings);

        m_serializeContext->EnableRemoveReflection();
        TemplatedClass<int>::Reflect(m_serializeContext, true);
        m_serializeContext->DisableRemoveReflection();

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<TemplatedClass>();
        m_jsonRegistrationContext->DisableRemoveReflection();
    }


    // Compare

    TEST_F(JsonSerializationTests, Compare_DifferentTypes_StringTypeLessThanNumberType)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kStringType);
        rapidjson::Value rhs(rapidjson::kNumberType);
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_NullType_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value(rapidjson::kNullType);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_TrueType_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value(rapidjson::kTrueType);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_FalseType_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value(rapidjson::kFalseType);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalStrings_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value;
        value.SetString(rapidjson::StringRef("Hello"));
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentStrings_HelloLessThanWorld)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetString(rapidjson::StringRef("Hello"));
        rhs.SetString(rapidjson::StringRef("World"));
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalIntegers_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value;
        value.SetUint64(42);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentIntegers_LessForSmallestNumber)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetUint64(42);
        rhs.SetUint64(88);
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalDoubles_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value value;
        value.SetDouble(42.0);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentDoubles_LessForSmallestNumber)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetDouble(42.0);
        rhs.SetDouble(88.0);
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalMixedNumbers_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetUint64(42);
        rhs.SetDouble(42.0);
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(lhs, rhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentMixedNumbers_LessForSmallestNumber)
    {
        using namespace AZ;

        rapidjson::Value lhs;
        rapidjson::Value rhs;
        lhs.SetUint64(42);
        rhs.SetDouble(88.0);
        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalObjects_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value& value = m_jsonDocument->SetObject();
        value.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        value.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        value.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalObjectsWithDifferentOrder_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value& lhs = m_jsonDocument->SetObject();
        lhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value& rhs = m_jsonDocument->SetObject();
        rhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(lhs , rhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentSizedObjects_LessForSmallestObject)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kObjectType);
        lhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kObjectType);
        rhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentMembersInObjects_AlphabeticallyLessBasedOnFieldNames)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kObjectType);
        lhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("x"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kObjectType);
        rhs.AddMember(rapidjson::StringRef("b"), 42, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("y"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("z"), true, m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentMemberTypesObjects_LessBasedOnTheMemberType)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kObjectType);
        lhs.AddMember(rapidjson::StringRef("a"), false, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("b"), 88.0, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kObjectType);
        rhs.AddMember(rapidjson::StringRef("c"), 88, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("a"), 42.0, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentValuesInObjectMembers_LessBasedOnMemberValue)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kObjectType);
        lhs.AddMember(rapidjson::StringRef("a"), 42, m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.AddMember(rapidjson::StringRef("c"), true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kObjectType);
        rhs.AddMember(rapidjson::StringRef("c"), false, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("a"), 88, m_jsonDocument->GetAllocator());
        rhs.AddMember(rapidjson::StringRef("b"), rapidjson::StringRef("World"), m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_IdenticalArray_ReturnEqual)
    {
        using namespace AZ;

        rapidjson::Value& value = m_jsonDocument->SetArray();
        value.PushBack(42, m_jsonDocument->GetAllocator());
        value.PushBack(rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        value.PushBack(true, m_jsonDocument->GetAllocator());
        EXPECT_EQ(JsonSerializerCompareResult::Equal, JsonSerialization::Compare(value, value));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentSizedArrays_LessForSmallestArray)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kArrayType);
        lhs.PushBack(42, m_jsonDocument->GetAllocator());
        lhs.PushBack(true, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kArrayType);
        rhs.PushBack(42, m_jsonDocument->GetAllocator());
        rhs.PushBack(rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        rhs.PushBack(true, m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentTypesInArrays_LessBasedOnType)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kArrayType);
        lhs.PushBack(false, m_jsonDocument->GetAllocator());
        lhs.PushBack(rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.PushBack(42, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kArrayType);
        rhs.PushBack(88.0, m_jsonDocument->GetAllocator());
        rhs.PushBack(false, m_jsonDocument->GetAllocator());
        rhs.PushBack(rapidjson::StringRef("World"), m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }

    TEST_F(JsonSerializationTests, Compare_DifferentValuesInArrays_LessBasedOnTheValue)
    {
        using namespace AZ;

        rapidjson::Value lhs(rapidjson::kArrayType);
        lhs.PushBack(42, m_jsonDocument->GetAllocator());
        lhs.PushBack(rapidjson::StringRef("Hello"), m_jsonDocument->GetAllocator());
        lhs.PushBack(42.0, m_jsonDocument->GetAllocator());

        rapidjson::Value rhs(rapidjson::kArrayType);
        rhs.PushBack(42, m_jsonDocument->GetAllocator());
        rhs.PushBack(rapidjson::StringRef("World"), m_jsonDocument->GetAllocator());
        rhs.PushBack(88.0, m_jsonDocument->GetAllocator());

        EXPECT_EQ(JsonSerializerCompareResult::Less, JsonSerialization::Compare(lhs, rhs));
        EXPECT_EQ(JsonSerializerCompareResult::Greater, JsonSerialization::Compare(rhs, lhs));
    }


    // LoadTypeId

    TEST_F(JsonSerializationTests, LoadTypeId_FromUuidString_Success)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        AZStd::string_view idString = "{0CC36DD4-2337-40B4-A86C-69C452C48555}";
        Uuid id(idString.data(), idString.length());
        Uuid output;
        
        ResultCode result = JsonSerialization::LoadTypeId(output,
            m_jsonDocument->SetString(rapidjson::StringRef(idString.data())), nullptr,
            AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(id, output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_FromUuidStringWithNoise_Success)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        AZStd::string_view idString = "Some comments {0CC36DD4-2337-40B4-A86C-69C452C48555} Vector3";
        Uuid id("{0CC36DD4-2337-40B4-A86C-69C452C48555}");
        Uuid output;

        ResultCode result = JsonSerialization::LoadTypeId(output,
            m_jsonDocument->SetString(rapidjson::StringRef(idString.data())), nullptr,
            AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(id, output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_LoadFromNameString_Success)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Vector3"), 
            nullptr, AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(azrtti_typeid<Vector3>(), output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_FromNameStringWithExtraWhitespace_Success)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("    Vector3      "),
            nullptr, AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(azrtti_typeid<Vector3>(), output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_UnknownName_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Unknown"),
            nullptr, AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }

    TEST_F(JsonSerializationTests, LoadTypeId_AmbigiousName_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<B::Inherited, BaseClass>();

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Inherited"),
            nullptr, AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }

    TEST_F(JsonSerializationTests, LoadTypeId_ResolvedAmbigiousName_SuccessAndBaseClassPicked)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        BaseClass::Reflect(m_serializeContext);
        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<C::Inherited>();

        Uuid baseClass = azrtti_typeid<BaseClass>();
        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Inherited"),
            &baseClass, AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(azrtti_typeid<A::Inherited>(), output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_UnresolvedAmbigiousName_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        BaseClass::Reflect(m_serializeContext);
        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<B::Inherited, BaseClass>();

        Uuid baseClass = azrtti_typeid<BaseClass>();
        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Inherited"),
            &baseClass, AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }

    TEST_F(JsonSerializationTests, LoadTypeId_InputIsNotAString_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetBool(true),
            nullptr, AZStd::string_view{}, m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }


    // StoreTypeId

    TEST_F(JsonSerializationTests, StoreTypeId_SingleInstanceType_StoresName)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid input = azrtti_typeid<Vector3>();
        ResultCode result = JsonSerialization::StoreTypeId(*m_jsonDocument, m_jsonDocument->GetAllocator(), input,
            AZStd::string_view{}, m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq(R"("Vector3")");
    }

    TEST_F(JsonSerializationTests, StoreTypeId_DuplicateInstanceType_StoresUuidWithName)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<B::Inherited, BaseClass>();
        
        Uuid input = azrtti_typeid<A::Inherited>();
        ResultCode result = JsonSerialization::StoreTypeId(*m_jsonDocument, m_jsonDocument->GetAllocator(), input,
            AZStd::string_view{}, m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq(R"("{E7829F37-C577-4F2B-A85B-6F331548354C} Inherited")", false);
    }

    TEST_F(JsonSerializationTests, StoreTypeId_UnknowmType_StoresUuidWithName)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid input = azrtti_typeid<BaseClass>();
        ResultCode result = JsonSerialization::StoreTypeId(*m_jsonDocument, m_jsonDocument->GetAllocator(), input,
            AZStd::string_view{}, m_serializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
    }
} // namespace JsonSerializationTests
