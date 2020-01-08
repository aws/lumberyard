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

#include <AzCore/Math/UuidSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    class JsonUuidSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<AZ::JsonUuidSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonUuidSerializer> m_serializer;
        AZ::Uuid m_testValue = AZ::Uuid("{8651B6DA-9792-407C-A46D-A6E2E39CA900}");
        AZ::Uuid m_testValueAlternative = AZ::Uuid("{01A6F1D2-BFCE-431A-B33A-D1C29672EDBC}");
    };

    TEST_F(JsonUuidSerializerTests, Load_InvalidTypeOfObjectType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kObjectType);
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_InvalidTypeOfkNullType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kNullType);
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_InvalidTypeOfArrayType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue(rapidjson::kArrayType);
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_InvalidTypeOfTrueBooleanType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(true);
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_InvalidTypeOfFalseBooleanType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(false);
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_InvalidTypeOfUnsignedIntegerType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint64(42);
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_InvalidTypeOfSignedIntegerType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt64(-42);
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_InvalidTypeOfDoubleType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetDouble(42.0);
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringValueFormatWithDashes_ValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(m_testValue.ToString<AZStd::string>(true, true).c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringValueFormatWithoutDashes_ValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(m_testValue.ToString<AZStd::string>(true, false).c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringValueWithExtraCharacters_ValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        AZStd::string uuidString = m_testValue.ToString<AZStd::string>();
        uuidString += " hello world";
        testValue.SetString(uuidString.c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringValueWithEmbeddedUuid_ValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        AZStd::string uuidString = "Hello";
        uuidString += m_testValue.ToString<AZStd::string>();
        uuidString += "world";
        testValue.SetString(uuidString.c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseInvalidStringValue_ErrorReturnedAndUuidNotChanged)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(rapidjson::StringRef("Hello world"));
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseInvalidUuid_ErrorReturnedAndUuidNotChanged)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(rapidjson::StringRef("{8651B6DA-9792-407C-A46D-AXXX39CA900}"));
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringWithMultipleUuids_FirstValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        AZStd::string uuidString = m_testValue.ToString<AZStd::string>();
        uuidString += m_testValueAlternative.ToString<AZStd::string>();
        testValue.SetString(uuidString.c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Store_StoreValue_ValueStored)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value convertedValue(rapidjson::kObjectType); // set to object to ensure we reset the value to expected type later

        ResultCode result = m_serializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &m_testValue, nullptr,
            azrtti_typeid<AZ::Uuid>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsString());
        EXPECT_STREQ(m_testValue.ToString<AZStd::string>().c_str(), convertedValue.GetString());
    }

    TEST_F(JsonUuidSerializerTests, Store_StoreSameAsDefault_ValueIsIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value convertedValue = CreateExplicitDefault();

        ResultCode result = m_serializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &m_testValue, &m_testValue,
            azrtti_typeid<AZ::Uuid>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        Expect_ExplicitDefault(convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Store_StoreDifferentFromDefault_ValueIsStored)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value convertedValue(rapidjson::kObjectType); // set to object to ensure we reset the value to expected type later

        ResultCode result = m_serializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &m_testValue, &m_testValueAlternative,
            azrtti_typeid<AZ::Uuid>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsString());
        EXPECT_STREQ(m_testValue.ToString<AZStd::string>().c_str(), convertedValue.GetString());
    }
} // namespace JsonSerializationTests
