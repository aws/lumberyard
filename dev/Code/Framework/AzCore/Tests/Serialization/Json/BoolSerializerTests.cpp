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

#include <AzCore/Serialization/Json/BoolSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

#pragma push_macro("AZ_NUMERICCAST_ENABLED") // pushing macro for uber file protection
#undef AZ_NUMERICCAST_ENABLED
#define AZ_NUMERICCAST_ENABLED 1    // enable asserts for numeric casting to ensure the json cast helper is working as expected

namespace JsonSerializationTests
{
    class JsonBoolSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_boolSerializer = AZStd::make_unique<AZ::JsonBoolSerializer>();
        }

        void TearDown() override
        {
            m_boolSerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        void TestSerializers(rapidjson::Value& testVal, bool expectedBool, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            using namespace AZ::JsonSerializationResult;

            bool testBool = !expectedBool;
            ResultCode result = m_boolSerializer->Load(&testBool, azrtti_typeid<bool>(), testVal, m_path, m_deserializationSettings);
            EXPECT_EQ(expectedOutcome, result.GetOutcome());
            if (expectedOutcome == AZ::JsonSerializationResult::Outcomes::Success)
            {
                EXPECT_EQ(testBool, expectedBool);
            }
            else
            {
                EXPECT_NE(testBool, expectedBool);
            }
        }

        void TestString(const char* testString, bool expectedBool,
            AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            rapidjson::Value testVal;
            testVal.SetString(rapidjson::GenericStringRef<char>(testString));
            TestSerializers(testVal, expectedBool, expectedOutcome);
        }

        void TestInt(int64_t testInt, bool expectedBool, 
            AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            rapidjson::Value testVal;
            testVal.SetInt64(testInt);
            TestSerializers(testVal, expectedBool, expectedOutcome);
        }

        void TestDouble(double testDouble, bool expectedBool, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            rapidjson::Value testVal;
            testVal.SetDouble(testDouble);
            TestSerializers(testVal, expectedBool, expectedOutcome);
        }

        void TestBool(bool inputBool, bool expectedBool)
        {
            rapidjson::Value testVal;
            testVal.SetBool(inputBool);
            TestSerializers(testVal, expectedBool, AZ::JsonSerializationResult::Outcomes::Success);
        }

        void TestInvalidType(rapidjson::Type testType)
        {
            rapidjson::Value testVal(testType);
            TestSerializers(testVal, false, AZ::JsonSerializationResult::Outcomes::Unsupported);
        }

    protected:
        AZStd::unique_ptr<AZ::JsonBoolSerializer> m_boolSerializer;
    };

    TEST_F(JsonBoolSerializerTests, InvalidToBool)
    {
        using namespace AZ::JsonSerializationResult;

        TestInvalidType(rapidjson::kObjectType);
        TestInvalidType(rapidjson::kNullType);
        TestInvalidType(rapidjson::kArrayType);
    }

    TEST_F(JsonBoolSerializerTests, BoolToInt)
    {
        using namespace AZ::JsonSerializationResult;

        TestBool(false, false);
        TestBool(true, true);
    }

    TEST_F(JsonBoolSerializerTests, ValidStringsToFalse_Succeeds)
    {
        using namespace AZ::JsonSerializationResult;

        TestString("0.0", false, Outcomes::Success);
        TestString("0.0f", false, Outcomes::Success);
        TestString("false", false, Outcomes::Success);
        TestString("fALse", false, Outcomes::Success);
        TestString("0", false, Outcomes::Success);
        TestString("0eee", false, Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, ValidStringsToTrue_Succeeds)
    {
        using namespace AZ::JsonSerializationResult;

        TestString("34", true, Outcomes::Success);
        TestString("-1", true, Outcomes::Success);
        TestString("1", true, Outcomes::Success);
        TestString("true", true, Outcomes::Success);
        TestString("tRuE", true, Outcomes::Success);
        TestString("1.0", true, Outcomes::Success);
        TestString("1.0f", true, Outcomes::Success);
        TestString("-1.0f", true, Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, InvalidStringsToBool_Fails)
    {
        using namespace AZ::JsonSerializationResult;

        TestString("", false, Outcomes::Unsupported);
        TestString("ee0", false, Outcomes::Unsupported);
        TestString("  ", false, Outcomes::Unsupported);
        TestString("notValid", false, Outcomes::Unsupported);
    }

    TEST_F(JsonBoolSerializerTests, IntsToBool_Succeeds)
    {
        using namespace AZ::JsonSerializationResult;

        TestInt(1, true, Outcomes::Success);
        TestInt(0, false, Outcomes::Success);
        TestInt(-1, true, Outcomes::Success);
        TestInt(100, true, Outcomes::Success);
        TestInt(-100, true, Outcomes::Success);
        TestInt(2, true, Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, DoubleToBool_Succeeds)
    {
        using namespace AZ::JsonSerializationResult;

        TestDouble(1.0, true, Outcomes::Success);
        TestDouble(1.00001, true, Outcomes::Success);
        TestDouble(0.0, false, Outcomes::Success);
        TestDouble(-0.0, false, Outcomes::Success);
        TestDouble(-0.1, true, Outcomes::Success);
        TestDouble(0.0001, true, Outcomes::Success);
        TestDouble(-1.0, true, Outcomes::Success);
        TestDouble(2.0, true, Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Store_StoreTrueValue_ValueStored)
    {
        using namespace AZ::JsonSerializationResult;

        bool value = true;
        rapidjson::Value convertedValue;

        ResultCode result = m_boolSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, nullptr,
            azrtti_typeid<bool>(), m_path, m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        ASSERT_TRUE(convertedValue.IsBool());
        EXPECT_TRUE(convertedValue.GetBool());
    }

    TEST_F(JsonBoolSerializerTests, Store_StoreFalseValue_ValueStored)
    {
        using namespace AZ::JsonSerializationResult;

        bool value = false;
        rapidjson::Value convertedValue;

        ResultCode result = m_boolSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, nullptr,
            azrtti_typeid<bool>(), m_path, m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        ASSERT_TRUE(convertedValue.IsBool());
        EXPECT_FALSE(convertedValue.GetBool());
    }

    TEST_F(JsonBoolSerializerTests, Store_StoreSameAsDefault_ValueIsIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        bool value = true;
        bool defaultValue = true;
        rapidjson::Value convertedValue = CreateExplicitDefault();

        ResultCode result = m_boolSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<bool>(), m_path, m_serializationSettings);
        ASSERT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        Expect_ExplicitDefault(convertedValue);
    }

    TEST_F(JsonBoolSerializerTests, Store_StoreDiffersFromDefault_ValueIsStored)
    {
        using namespace AZ::JsonSerializationResult;

        bool value = true;
        bool defaultValue = false;
        rapidjson::Value convertedValue;

        ResultCode result = m_boolSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<bool>(), m_path, m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        ASSERT_TRUE(convertedValue.IsBool());
        EXPECT_TRUE(convertedValue.GetBool());
    }
} // namespace JsonSerializationTests

#pragma pop_macro("AZ_NUMERICCAST_ENABLED") // pushing macro for uber file protection
