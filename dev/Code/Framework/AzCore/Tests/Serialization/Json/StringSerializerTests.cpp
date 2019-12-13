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

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Serialization/Json/StringSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    template<typename> struct SerializerInfo {};

    template<> struct SerializerInfo<AZ::JsonStringSerializer>
    {
        using DataType = AZStd::string;
    };

    template<> struct SerializerInfo<AZ::JsonOSStringSerializer>
    {
        using DataType = AZ::OSString;
    };

    template<typename SerializerType>
    class TypedJsonStringSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        AZStd::unique_ptr<SerializerType> m_serializer;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<SerializerType>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        const char* m_testString = "Hello world";
        const char* m_testStringAlternative = "Goodbye cruel world";
    };

    using StringSerializationTypes = ::testing::Types<
        AZ::JsonStringSerializer,
        AZ::JsonOSStringSerializer >;
    TYPED_TEST_CASE(TypedJsonStringSerializerTests, StringSerializationTypes);

    TYPED_TEST(TypedJsonStringSerializerTests, Load_InvalidTypeOfObjectType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kObjectType);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_InvalidTypeOfkNullType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kNullType);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_InvalidTypeOfArrayType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kArrayType);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_FalseBoolean_FalseAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(false);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STRCASEEQ("False", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_TrueBoolean_TrueAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(true);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STRCASEEQ("True", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseStringValue_StringIsReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(rapidjson::StringRef(this->m_testString));
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ(this->m_testString, convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseUnsignedIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint(42);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("42", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseUnsigned64BitIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint64(42);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("42", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseSignedIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt(-42);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("-42", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseSigned64BitIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt64(-42);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("-42", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseFloatingPointValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetDouble(3.1415);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("3.141500", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Store_StoreValue_ValueStored)
    {
        using namespace AZ::JsonSerializationResult;

        typename SerializerInfo<TypeParam>::DataType value = this->m_testString;
        rapidjson::Value convertedValue(rapidjson::kObjectType); // set to object to ensure we reset the value to expected type later

        ResultCode result = this->m_serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(), &value, nullptr,
            azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(), this->m_path, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsString());
        EXPECT_STREQ(this->m_testString, convertedValue.GetString());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Store_StoreSameAsDefault_ValueIsIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        typename SerializerInfo<TypeParam>::DataType value = this->m_testString;
        typename SerializerInfo<TypeParam>::DataType defaultValue = this->m_testString;
        rapidjson::Value convertedValue = this->CreateExplicitDefault();

        ResultCode result = this->m_serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(), this->m_path, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        this->Expect_ExplicitDefault(convertedValue);
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Store_StoreDifferentFromDefault_ValueIsStored)
    {
        using namespace AZ::JsonSerializationResult;

        typename SerializerInfo<TypeParam>::DataType value = this->m_testString;
        typename SerializerInfo<TypeParam>::DataType defaultValue = this->m_testStringAlternative;
        rapidjson::Value convertedValue(rapidjson::kObjectType);

        ResultCode result = this->m_serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(), this->m_path, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsString());
        EXPECT_STREQ(this->m_testString, convertedValue.GetString());
    }
} // namespace JsonSerializationTests
