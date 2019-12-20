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

#include <limits>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Serialization/Json/DoubleSerializer.h>
#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

#pragma push_macro("AZ_NUMERICCAST_ENABLED") // pushing macro for uber file protection
#undef AZ_NUMERICCAST_ENABLED
#define AZ_NUMERICCAST_ENABLED 1    // enable asserts for numeric casting to ensure the json cast helper is working as expected

namespace JsonSerializationTests
{
    class JsonDoubleSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_floatSerializer = AZStd::make_unique<AZ::JsonFloatSerializer>();
            m_doubleSerializer = AZStd::make_unique<AZ::JsonDoubleSerializer>();
        }

        void TearDown() override
        {
            m_floatSerializer.reset();
            m_doubleSerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        void TestSerializers(rapidjson::Value& testVal, double expectedValue, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            using namespace AZ::JsonSerializationResult;

            float value;
            ASSERT_EQ(Outcomes::Success, AZ::JsonNumericCast(value, expectedValue, m_path, m_deserializationSettings.m_reporting).GetOutcome());
            TestSerializers(testVal, expectedValue, value, expectedOutcome);
        }

        void TestSerializers(rapidjson::Value& testVal, double expectedDouble, float expectedFloat, 
            AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            using namespace AZ::JsonSerializationResult;

            // double
            double testDouble = DEFAULT_DOUBLE;
            ResultCode doubleResult = m_doubleSerializer->Load(&testDouble, azrtti_typeid<double>(), testVal, m_path, m_deserializationSettings);
            EXPECT_EQ(expectedOutcome, doubleResult.GetOutcome());
            EXPECT_DOUBLE_EQ(testDouble, expectedDouble);

            // float
            float testFloat = DEFAULT_FLOAT;
            ResultCode floatResult = m_floatSerializer->Load(&testFloat, azrtti_typeid <float>(), testVal, m_path, m_deserializationSettings);
            EXPECT_EQ(expectedOutcome, floatResult.GetOutcome());
            EXPECT_FLOAT_EQ(testFloat, expectedFloat);
        }

        void TestString(const char* testString, double expectedDouble, float expectedFloat, 
            AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            rapidjson::Value testVal;
            testVal.SetString(rapidjson::GenericStringRef<char>(testString));
            EXPECT_STREQ(testString, testVal.GetString());

            TestSerializers(testVal, expectedDouble, expectedFloat, expectedOutcome);
        }

        void TestInt(int64_t testInt, double expectedValue)
        {
            using namespace AZ::JsonSerializationResult;

            rapidjson::Value testVal;
            testVal.SetInt64(testInt);
            EXPECT_EQ(testVal.GetInt64(), testInt);

            TestSerializers(testVal, expectedValue, Outcomes::Success);
        }

        void TestDouble(double testDouble, double expectedValue)
        {
            using namespace AZ::JsonSerializationResult;

            rapidjson::Value testVal;
            testVal.SetDouble(testDouble);
            EXPECT_EQ(testVal.GetDouble(), testDouble);

            TestSerializers(testVal, expectedValue, Outcomes::Success);
        }

        void TestBool(bool testBool, double expectedValue)
        {
            using namespace AZ::JsonSerializationResult;

            rapidjson::Value testVal;
            testVal.SetBool(testBool);
            EXPECT_EQ(testVal.GetBool(), testBool);

            TestSerializers(testVal, expectedValue, Outcomes::Success);
        }

        void TestInvalidType(rapidjson::Type testType)
        {
            using namespace AZ::JsonSerializationResult;

            rapidjson::Value testVal(testType);
            EXPECT_EQ(testVal.GetType(), testType);

            TestSerializers(testVal, DEFAULT_DOUBLE, DEFAULT_FLOAT, Outcomes::Unsupported);
        }

    protected:
        AZStd::unique_ptr<AZ::JsonFloatSerializer> m_floatSerializer;
        AZStd::unique_ptr<AZ::JsonDoubleSerializer> m_doubleSerializer;

        constexpr static float DEFAULT_FLOAT = 1.23456789f;
        constexpr static double DEFAULT_DOUBLE = -9.87654321;
    };

    TEST_F(JsonDoubleSerializerTests, ValidStringToFloatingPoint)
    {
        using namespace AZ::JsonSerializationResult;

        TestString("-1", -1.0, -1.0f, Outcomes::Success);
        TestString("34", 34.0, 34.0f, Outcomes::Success);
        TestString("0", 0.0, 0.0f, Outcomes::Success);
        TestString("345", 345.0, 345.0f, Outcomes::Success);
        TestString("12.5", 12.5, 12.5f, Outcomes::Success);
        TestString("-1.5f", -1.5, -1.5f, Outcomes::Success); // The f isn't processed as the interpretation is a double.
        TestString("2.4 hello", 2.4, 2.4f, Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, InvalidStringToFloatingPoint)
    {
        using namespace AZ::JsonSerializationResult;

        TestString("", DEFAULT_DOUBLE, DEFAULT_FLOAT, Outcomes::Unsupported);
        TestString("hello", DEFAULT_DOUBLE, DEFAULT_FLOAT, Outcomes::Unsupported);
        TestString("hello 2.4", DEFAULT_DOUBLE, DEFAULT_FLOAT, Outcomes::Unsupported);
    }

    TEST_F(JsonDoubleSerializerTests, IntToFloatingPoint)
    {
        TestInt(-1, -1.0);
        TestInt(34, 34.0);
        TestInt(0, 0.0);
        TestInt(19, 19.0);
        TestInt(120, 120.0);
    }

    TEST_F(JsonDoubleSerializerTests, IntToFloatingPointLimits)
    {
        int64_t maxLongDoubleAsInt = azlossy_caster(std::numeric_limits<long double>::max());
        int64_t minLongDoubleAsInt = azlossy_caster(std::numeric_limits<long double>::lowest());

        TestInt(maxLongDoubleAsInt, azlossy_caster(maxLongDoubleAsInt));
        TestInt(minLongDoubleAsInt, azlossy_caster(minLongDoubleAsInt));
    }

    TEST_F(JsonDoubleSerializerTests, DoubleToFloatingPoint)
    {
        TestDouble(-1.0, -1.0);
        TestDouble(34.0, 34.0);
        TestDouble(0.0, 0.0);
        TestDouble(19.0, 19.0);
        TestDouble(120.0, 120.0);
        TestDouble(123456.89, 123456.89);
    }

    TEST_F(JsonDoubleSerializerTests, DoubleToFloatingPointLimits)
    {
        using namespace AZ::JsonSerializationResult;

        constexpr double maxDouble = std::numeric_limits<double>::max();
        rapidjson::Value testVal;
        testVal.SetDouble(maxDouble);
        EXPECT_EQ(maxDouble, testVal.GetDouble());

        float value;
        ResultCode result = m_floatSerializer->Load(&value, azrtti_typeid<float>(), testVal, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonDoubleSerializerTests, BoolToFloatingPoint)
    {
        TestBool(false, 0.0);
        TestBool(true, 1.0);
    }

    TEST_F(JsonDoubleSerializerTests, InvalidToFloatingPoint)
    {
        TestInvalidType(rapidjson::kNullType);
        TestInvalidType(rapidjson::kArrayType);
        TestInvalidType(rapidjson::kObjectType);
    }

    TEST_F(JsonDoubleSerializerTests, Store_StoreFloatValue_ValueStored)
    {
        using namespace AZ::JsonSerializationResult;

        float value = 1.0f;
        rapidjson::Value convertedValue;

        ResultCode result = m_floatSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, nullptr,
            azrtti_typeid<float>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsDouble());
        EXPECT_FLOAT_EQ(value, convertedValue.GetDouble());
    }

    TEST_F(JsonDoubleSerializerTests, Store_StoreSameFloatAsDefault_ValueIsIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        float value = 1.0f;
        float defaultValue = 1.0f;
        rapidjson::Value convertedValue = CreateExplicitDefault();

        ResultCode result = m_floatSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<float>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        Expect_ExplicitDefault(convertedValue);
    }

    TEST_F(JsonDoubleSerializerTests, Store_StoreDifferentFloatFromDefault_ValueIsStored)
    {
        using namespace AZ::JsonSerializationResult;

        float value = 1.0f;
        float defaultValue = 2.0f;
        rapidjson::Value convertedValue;

        ResultCode result = m_floatSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<float>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsDouble());
        EXPECT_FLOAT_EQ(value, convertedValue.GetDouble());
    }

    TEST_F(JsonDoubleSerializerTests, Store_StoreDoubleValue_ValueStored)
    {
        using namespace AZ::JsonSerializationResult;

        double value = 1.0;
        rapidjson::Value convertedValue;

        ResultCode result = m_doubleSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, nullptr,
            azrtti_typeid<double>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsDouble());
        EXPECT_DOUBLE_EQ(value, convertedValue.GetDouble());
    }

    TEST_F(JsonDoubleSerializerTests, Store_StoreSameDoubleAsDefault_ValueIsIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        double value = 1.0;
        double defaultValue = 1.0;
        rapidjson::Value convertedValue = CreateExplicitDefault();

        ResultCode result = m_doubleSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<double>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        ASSERT_TRUE(convertedValue.IsObject());
        Expect_ExplicitDefault(convertedValue);
    }

    TEST_F(JsonDoubleSerializerTests, Store_StoreDifferentDoubleFromDefault_ValueIsStored)
    {
        using namespace AZ::JsonSerializationResult;

        double value = 1.0;
        double defaultValue = 2.0;
        rapidjson::Value convertedValue;

        ResultCode result = m_doubleSerializer->Store(convertedValue, m_jsonDocument->GetAllocator(), &value, &defaultValue,
            azrtti_typeid<double>(), m_path, m_serializationSettings);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(convertedValue.IsDouble());
        EXPECT_DOUBLE_EQ(value, convertedValue.GetDouble());
    }
} // namespace JsonSerializationTests

#pragma pop_macro("AZ_NUMERICCAST_ENABLED")
