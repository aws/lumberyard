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
#include <AzCore/Name/NameJsonSerializer.h>
#include <AzCore/Name/NameDictionary.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    class NameJsonSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        AZStd::unique_ptr<AZ::NameJsonSerializer> m_serializer;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            AZ::NameDictionary::Create();
            m_serializer = AZStd::make_unique<AZ::NameJsonSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            AZ::NameDictionary::Destroy();
            BaseJsonSerializerFixture::TearDown();
        }

        const char* m_testString = "Hello world";
        const char* m_testStringAlternative = "Goodbye cruel world";
    };

    TEST_F(NameJsonSerializerTests, Load_InvalidTypeOfObjectType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kObjectType);
        testValue.AddMember("value", "hello", this->m_jsonDocument->GetAllocator());

        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Name(), convertedValue);
    }

    TEST_F(NameJsonSerializerTests, Load_InvalidTypeOfkNullType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kNullType);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Name(), convertedValue);
    }

    TEST_F(NameJsonSerializerTests, Load_InvalidTypeOfArrayType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kArrayType);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Name(), convertedValue);
    }

    TEST_F(NameJsonSerializerTests, Load_FalseBoolean_FalseAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(false);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STRCASEEQ("False", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_TrueBoolean_TrueAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(true);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STRCASEEQ("True", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseStringValue_StringIsReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(rapidjson::StringRef(this->m_testString));
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ(this->m_testString, convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseUnsignedIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint(42);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("42", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseUnsigned64BitIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint64(42);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("42", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseSignedIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt(-42);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("-42", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseSigned64BitIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt64(-42);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("-42", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseFloatingPointValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetDouble(3.1415);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("3.141500", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Store_StoreValue_ValueStored)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::Name value{this->m_testString};
        rapidjson::Value convertedValue(rapidjson::kObjectType); // set to object to ensure we reset the value to expected type later

        ResultCode result = this->m_serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(), &value, nullptr,
            azrtti_typeid<AZ::Name>(), this->m_path, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsString());
        EXPECT_STREQ(this->m_testString, convertedValue.GetString());
    }

    TEST_F(NameJsonSerializerTests, Store_StoreSameAsDefault_ValueIsIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::Name value{this->m_testString};
        AZ::Name defaultValue{this->m_testString};
        rapidjson::Value convertedValue = this->CreateExplicitDefault();

        ResultCode result = this->m_serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<AZ::Name>(), this->m_path, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        this->Expect_ExplicitDefault(convertedValue);
    }

    TEST_F(NameJsonSerializerTests, Store_StoreDifferentFromDefault_ValueIsStored)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::Name value{this->m_testString};
        AZ::Name defaultValue{this->m_testStringAlternative};
        rapidjson::Value convertedValue(rapidjson::kObjectType);

        ResultCode result = this->m_serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<AZ::Name>(), this->m_path, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsString());
        EXPECT_STREQ(this->m_testString, convertedValue.GetString());
    }
} // namespace JsonSerializationTests
