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

#include <AzCore/Math/MathVectorSerializer.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

#pragma push_macro("AZ_NUMERICCAST_ENABLED") // pushing macro for uber file protection
#undef AZ_NUMERICCAST_ENABLED
#define AZ_NUMERICCAST_ENABLED 1    // enable asserts for numeric casting to ensure the json cast helper is working as expected

namespace JsonSerializationTests
{
    template<typename T>
    class JsonMathVectorSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        using Descriptor = T;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<typename T::Serializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        constexpr static const char* upperCaseFieldNames[4] = { "X", "Y", "Z", "W" };
        constexpr static const char* lowerCaseFieldNames[4] = { "x", "y", "z", "w" };

        AZStd::unique_ptr<typename T::Serializer> m_serializer;
    };

    struct Vector2Descriptor
    {
        using VectorType = AZ::Vector2;
        using Serializer = AZ::JsonVector2Serializer;
        constexpr static size_t ElementCount = 2;
    };

    struct Vector3Descriptor
    {
        using VectorType = AZ::Vector3;
        using Serializer = AZ::JsonVector3Serializer;
        constexpr static size_t ElementCount = 3;
    };

    struct Vector4Descriptor
    {
        using VectorType = AZ::Vector4;
        using Serializer = AZ::JsonVector4Serializer;
        constexpr static size_t ElementCount = 4;
    };

    using JsonMathVectorSerializerTypes = ::testing::Types <
        Vector2Descriptor, Vector3Descriptor, Vector4Descriptor>;
    TYPED_TEST_CASE(JsonMathVectorSerializerTests, JsonMathVectorSerializerTypes);


    // Load invalid types

    TYPED_TEST(JsonMathVectorSerializerTests, Load_InvalidTypes_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        const rapidjson::Type unsupportedTypes[] =
        {
            rapidjson::kNullType,
            rapidjson::kFalseType,
            rapidjson::kTrueType,
            rapidjson::kStringType,
            rapidjson::kNumberType
        };

        for (size_t i = 0; i < AZ_ARRAY_SIZE(unsupportedTypes); ++i)
        {
            rapidjson::Value value(unsupportedTypes[i]);
            typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
            ResultCode result = this->m_serializer->Load(
                &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), value, this->m_path, this->m_deserializationSettings);
            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        }
    }


    // Load array tests

    TYPED_TEST(JsonMathVectorSerializerTests, Load_ValidArray_ReturnsSuccessAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(static_cast<float>(i + 1), output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_OversizedArray_ReturnsPartialConvertAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount + 1; ++i)
        {
            arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(static_cast<float>(i + 1), output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_UndersizedArray_ReturnsUnsupportedAndLeavesVectorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount - 1; ++i)
        {
            arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());

        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(0.0f, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_InvalidEntries_ReturnsUnsupportedAndLeavesVectorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            if (i == 1)
            {
                arrayValue.PushBack(rapidjson::StringRef("Invalid"), this->m_jsonDocument->GetAllocator());
            }
            else
            {
                arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
            }
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());

        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(0.0f, output.GetElement(i));
        }
    }


    // Load object tests
    TYPED_TEST(JsonMathVectorSerializerTests, Load_ValidObjectUpperCase_ReturnsSuccessAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;
        const char* fieldNames[4] =
        {
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[0],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[1],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[2],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[3]
        };

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            objectValue.AddMember(rapidjson::StringRef(fieldNames[i]), static_cast<float>(i + 1),
                this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(static_cast<float>(i + 1), output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_ValidObjectLowerCase_ReturnsSuccessAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;
        const char* fieldNames[4] =
        {
            JsonMathVectorSerializerTests<TypeParam>::lowerCaseFieldNames[0],
            JsonMathVectorSerializerTests<TypeParam>::lowerCaseFieldNames[1],
            JsonMathVectorSerializerTests<TypeParam>::lowerCaseFieldNames[2],
            JsonMathVectorSerializerTests<TypeParam>::lowerCaseFieldNames[3]
        };
        
        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            objectValue.AddMember(rapidjson::StringRef(fieldNames[i]), static_cast<float>(i + 1),
                this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(static_cast<float>(i + 1), output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_ValidObjectWithExtraFields_ReturnsPartialConvertAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;
        const char* fieldNames[4] =
        {
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[0],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[1],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[2],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[3]
        };

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            objectValue.AddMember(rapidjson::StringRef(fieldNames[i]), static_cast<float>(i + 1),
                this->m_jsonDocument->GetAllocator());
        }
        this->InjectAdditionalFields(objectValue, rapidjson::kStringType, this->m_jsonDocument->GetAllocator());

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(static_cast<float>(i + 1), output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_MissingFields_ReturnsPartialDefaultValueAndLeavesVectorUntouched)
    {
        using namespace AZ::JsonSerializationResult;
        const char* fieldNames[4] =
        {
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[0],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[1],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[2],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[3]
        };

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            if (i == 1)
            {
                continue;
            }
            objectValue.AddMember(rapidjson::StringRef(fieldNames[i]), static_cast<float>(i + 1),
                this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Unsupported, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(0.0f, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_InvalidFields_ReturnsUnsupportedAndLeavesVectorUntouched)
    {
        using namespace AZ::JsonSerializationResult;
        const char* fieldNames[4] =
        {
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[0],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[1],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[2],
            JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[3]
        };

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            if (i == 1)
            {
                objectValue.AddMember(rapidjson::StringRef(fieldNames[i]), rapidjson::StringRef("Invalid"),
                    this->m_jsonDocument->GetAllocator());
            }
            else
            {
                objectValue.AddMember(rapidjson::StringRef(fieldNames[i]), static_cast<float>(i + 1),
                    this->m_jsonDocument->GetAllocator());
            }
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Unsupported, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(0.0f, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_LoadEmtpyObject_ReturnsUnsupportedAndLeavesVectorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_jsonDocument->SetObject();
        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Unsupported, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(0.0f, output.GetElement(i));
        }
    }


    // Test storing

    TYPED_TEST(JsonMathVectorSerializerTests, Store_ValidVector_ReturnsSuccessAndStoresVector)
    {
        using namespace AZ::JsonSerializationResult;

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType input;
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            input.SetElement(i, static_cast<float>(i + 1));
        }

        ResultCode result = this->m_serializer->Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(), &input,
            nullptr, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), this->m_path, this->m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());

        ASSERT_TRUE(this->m_jsonDocument->IsArray());
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_TRUE((*this->m_jsonDocument)[i].IsDouble());
            double value = (*this->m_jsonDocument)[i].GetDouble();
            EXPECT_DOUBLE_EQ(static_cast<double>(i + 1), value);
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Store_DefaultVector_ReturnsDefaultValueAndNothingWritten)
    {
        using namespace AZ::JsonSerializationResult;

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType input;
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            input.SetElement(i, static_cast<float>(i + 1));
        }

        rapidjson::Value convertedValue = this->CreateExplicitDefault();
        ResultCode result = this->m_serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(), &input,
            &input, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), this->m_path, this->m_serializationSettings);
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        this->Expect_ExplicitDefault(convertedValue);
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Store_PartialDefault_ReturnsSuccessAndStoresVector)
    {
        using namespace AZ::JsonSerializationResult;

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType input;
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            input.SetElement(i, static_cast<float>(i + 1));
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType defaultInput = input;
        defaultInput.SetElement(1, 0.0f);

        this->m_jsonDocument->SetNull();
        ResultCode result = this->m_serializer->Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(), &input,
            &defaultInput, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), this->m_path, this->m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());

        ASSERT_TRUE(this->m_jsonDocument->IsArray());
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_TRUE((*this->m_jsonDocument)[i].IsDouble());
            double value = (*this->m_jsonDocument)[i].GetDouble();
            EXPECT_DOUBLE_EQ(static_cast<double>(i + 1), value);
        }
    }
} // namespace JsonSerializationTests

#pragma pop_macro("AZ_NUMERICCAST_ENABLED")
