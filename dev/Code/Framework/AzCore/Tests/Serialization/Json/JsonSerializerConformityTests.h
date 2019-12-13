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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    class JsonSerializerConformityTestDescriptorFeatures
    {
    public:
        inline void EnableJsonType(rapidjson::Type type)
        {
            AZ_Assert(type < AZ_ARRAY_SIZE(m_typesSupported), "RapidJSON Type not known.");
            m_typesSupported[type] = true;;
        }
        inline bool SupportsJsonType(rapidjson::Type type)
        {
            AZ_Assert(type < AZ_ARRAY_SIZE(m_typesSupported), "RapidJSON Type not known.");
            return m_typesSupported[type];
        }

        //! This type will be used for additional files or to corrupt a json file. Use a type
        //! that the serializer under test doesn't support.
        rapidjson::Type m_typeToInject = rapidjson::kStringType;
        //! Set to true if the rapidjson::kArrayType is expected to be a fixed size.
        bool m_fixedSizeArray{ false };
        //! Set to false if the type the serializer targets can't be partially initialized.
        bool m_supportsPartialInitialization{ true };
        //! The serializer will look up the type id in the Serialize Context.
        //! Normally serializer only get the exact type they support, but some serializers support
        //! multiple types such as templated code. By settings this option to true tests will be added
        //! give an unreflected type to make sure an invalid case is handled.
        bool m_requiresTypeIdLookups{ false };
        //! Several tests inject unsupported types into a json files or corrupt the file with unsupported
        //! values. If the serializer under test supports all json types then GetInjectedJson/GetCorruptedJson
        //! can be used to manually create these documents. If that is also not an option the tests can be
        //! disabled by setting this flag to false.
        bool m_supportsInjection{ true };

    private:
        // There's no way to retrieve the number of types from RapidJSON so they're hardcoded here.
        bool m_typesSupported[7] = { false, false, false, false, false, false, false };
    };

    namespace Internal
    {
        inline AZ::JsonSerializationResult::ResultCode VerifyCallback(AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
        {
            EXPECT_LT(0, message.length());
            EXPECT_TRUE(path.starts_with("Test"));
            return result;
        }
    }

    template<typename T>
    class JsonSerializerConformityTestDescriptor
    {
    public:
        using Type = T;
        virtual ~JsonSerializerConformityTestDescriptor() = default;

        virtual AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() = 0;
        
        //! Create an instance of the target type with all values set to default.
        virtual AZStd::shared_ptr<T> CreateDefaultInstance() = 0;
        //! Create an instance of the target type with some values set and some kept on defaults.
        //! If the target type doesn't support partial specialization this can be ignored and
        //! tests for partial support will be skipped.
        virtual AZStd::shared_ptr<T> CreatePartialDefaultInstance() { return nullptr; }
        //! Create an instance where all values are set to non-default values.
        virtual AZStd::shared_ptr<T> CreateFullySetInstance() = 0;

        //! Get the json that represents the default instance.
        //! If the target type doesn't support partial specialization this can be ignored and
        //! tests for partial support will be skipped.
        virtual AZStd::string_view GetJsonForPartialDefaultInstance() { return "";  }
        //! Get the json that represents the instance with all values set.
        virtual AZStd::string_view GetJsonForFullySetInstance() = 0;
        //! Get the json where additional values are added to the json file.
        //! If this function is not overloaded, but features.m_supportsInjection is enabled than
        //! the Json Serializer Conformity Tests will inject extra values in the json for a fully.
        //! set instance. It's recommended to use the automatic injection.
        virtual AZStd::string_view GetInjectedJson() { return ""; };
        //! Get the json where one or more values are replaced with invalid values.
        //! If this function is not overloaded, but features.m_supportsInjection is enabled than
        //! the Json Serializer Conformity Tests will corrupt the json for a fully set instance.
        //! It's recommended to use the automatic corruption.
        virtual AZStd::string_view GetCorruptedJson() { return "";  };

        virtual void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& /*features*/) = 0;
        virtual void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& /*context*/) {};

        virtual bool AreEqual(const T& lhs, const T& rhs) = 0;
    };

    template<typename T>
    class JsonSerializerConformityTests
        : public BaseJsonSerializerFixture
    {
    public:
        using Description = T;
        using Type = typename T::Type;

        void SetUp() override
        {
            using namespace AZ::JsonSerializationResult;
            
            BaseJsonSerializerFixture::SetUp();

            this->m_description.ConfigureFeatures(this->m_features);
            this->m_description.Reflect(m_serializeContext);

            this->m_path.Push("Test");
            this->m_deserializationSettings.m_reporting = &Internal::VerifyCallback;
            this->m_serializationSettings.m_reporting = &Internal::VerifyCallback;
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            this->m_description.Reflect(m_serializeContext);
            m_serializeContext->DisableRemoveReflection();

            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        void Load_InvalidType_ReturnsUnsupported(rapidjson::Type type)
        {
            using namespace AZ::JsonSerializationResult;

            if (!this->m_features.SupportsJsonType(type))
            {
                rapidjson::Value testValue(type);

                auto serializer = this->m_description.CreateSerializer();
                auto original = this->m_description.CreateDefaultInstance();
                auto instance = this->m_description.CreateDefaultInstance();

                ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*original), 
                    testValue, m_path, m_deserializationSettings);
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(Processing::Altered, result.GetProcessing());
                EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
            }
        }

        T m_description;
        JsonSerializerConformityTestDescriptorFeatures m_features;
    };

    class IncorrectClass
    {
    public:
        AZ_RTTI(IncorrectClass, "{E201252B-D653-4753-93AD-4F13C5FA2246}");
    };

    TYPED_TEST_CASE_P(JsonSerializerConformityTests);

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfNullType_ReturnsUnsupported)
    { 
        this->Load_InvalidType_ReturnsUnsupported(rapidjson::kNullType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfFalseType_ReturnsUnsupported)
    { 
        this->Load_InvalidType_ReturnsUnsupported(rapidjson::kFalseType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfTrueType_ReturnsUnsupported)
    {
        this->Load_InvalidType_ReturnsUnsupported(rapidjson::kTrueType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfObjectType_ReturnsUnsupported)
    {
        this->Load_InvalidType_ReturnsUnsupported(rapidjson::kObjectType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfArrayType_ReturnsUnsupported)
    {
        this->Load_InvalidType_ReturnsUnsupported(rapidjson::kArrayType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfStringType_ReturnsUnsupported)
    {
        this->Load_InvalidType_ReturnsUnsupported(rapidjson::kStringType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfNumberType_ReturnsUnsupported) 
    {
        this->Load_InvalidType_ReturnsUnsupported(rapidjson::kNumberType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeUnreflectedType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_requiresTypeIdLookups)
        {
            this->m_jsonDocument->Parse(this->m_description.GetJsonForFullySetInstance().data());
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            IncorrectClass instance;
            
            ResultCode result = serializer->Load(&instance, azrtti_typeid<IncorrectClass>(),
                *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);

            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
            EXPECT_EQ(Processing::Altered, result.GetProcessing());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeEmptyObject_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kObjectType))
        {
            this->m_jsonDocument->Parse("{}");
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultInstance();
            auto original = this->m_description.CreateDefaultInstance();

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*original),
                *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);

            EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeEmptyArray_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType))
        {
            this->m_jsonDocument->Parse("[]");
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultInstance();
            auto original = this->m_description.CreateDefaultInstance();

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);

            if (this->m_features.m_fixedSizeArray)
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(Processing::Altered, result.GetProcessing());
            }
            else
            {
                EXPECT_EQ(Outcomes::Success, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());
            }
            EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeFullySetInstance_SucceedsAndObjectMatchesFullySetInstance)
    {
        using namespace AZ::JsonSerializationResult;

        AZStd::string_view json = this->m_description.GetJsonForFullySetInstance();
        this->m_jsonDocument->Parse(json.data());
        ASSERT_FALSE(this->m_jsonDocument->HasParseError());

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateDefaultInstance();
        auto compare = this->m_description.CreateFullySetInstance();
        
        ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
            *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializePartialInstance_SucceedsAndObjectMatchesParialInstance)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            AZStd::string_view json = this->m_description.GetJsonForPartialDefaultInstance();
            this->m_jsonDocument->Parse(json.data());
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultInstance();
            auto compare = this->m_description.CreatePartialDefaultInstance();
            ASSERT_NE(nullptr, compare);

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);

            EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_HaltedThroughCallback_LoadFailsAndHaltReported)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_jsonDocument->Parse(this->m_description.GetJsonForFullySetInstance().data());
        ASSERT_FALSE(this->m_jsonDocument->HasParseError());

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateDefaultInstance();
        
        auto reporting = [](AZStd::string_view message, ResultCode result,
            AZStd::string_view path) -> ResultCode
        {
            Internal::VerifyCallback(message, result, path); 
            return ResultCode(result.GetTask(), Outcomes::Catastrophic);
        };
        this->m_deserializationSettings.m_reporting = reporting;

        ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
            *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);

        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());
        EXPECT_EQ(Processing::Halted, result.GetProcessing());
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InsertAdditionalData_SucceedsAndObjectMatchesFullySetInstance)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsInjection)
        {
            AZStd::string_view injectedJson = this->m_description.GetInjectedJson();
            if (injectedJson.empty())
            {
                this->m_jsonDocument->Parse(this->m_description.GetJsonForFullySetInstance().data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
                this->InjectAdditionalFields(*(this->m_jsonDocument), this->m_features.m_typeToInject,
                    this->m_jsonDocument->GetAllocator());
            }
            else
            {
                this->m_jsonDocument->Parse(injectedJson.data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
            }

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultInstance();
            auto compare = this->m_description.CreateFullySetInstance();

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);

            EXPECT_GE(result.GetOutcome(), Outcomes::Unavailable);
            EXPECT_GE(result.GetProcessing(), Processing::PartialAlter);
            EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_CorruptedValue_SucceedsWithDefaultValuesUsed)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsInjection)
        {
            AZStd::string_view corruptedJson = this->m_description.GetCorruptedJson();
            if (corruptedJson.empty())
            {
                this->m_jsonDocument->Parse(this->m_description.GetJsonForFullySetInstance().data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
                this->CorruptFields(*(this->m_jsonDocument), this->m_features.m_typeToInject);
            }
            else
            {
                this->m_jsonDocument->Parse(corruptedJson.data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
            }

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultInstance();
            
            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, this->m_path, this->m_deserializationSettings);

            EXPECT_GE(result.GetOutcome(), Outcomes::Unavailable);
            EXPECT_GE(result.GetProcessing(), Processing::PartialAlter);
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeUnreflectedType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_requiresTypeIdLookups)
        {
            auto serializer = this->m_description.CreateSerializer();
            IncorrectClass instance;

            ResultCode result = serializer->Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(), 
                &instance, nullptr, azrtti_typeid<IncorrectClass>(), this->m_path, this->m_serializationSettings);
                
            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
            EXPECT_EQ(Processing::Altered, result.GetProcessing());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeDefaultInstance_EmptyJsonReturned)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializationSettings.m_keepDefaults = false;
        
        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateDefaultInstance();
        rapidjson::Value convertedValue = this->CreateExplicitDefault();

        ResultCode result = serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(),
            instance.get(), instance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        this->Expect_ExplicitDefault(convertedValue);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeWithDefaultsKept_FullyWrittenJson)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializationSettings.m_keepDefaults = true;

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateFullySetInstance();

        ResultCode result = serializer->Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            instance.get(), instance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        this->Expect_DocStrEq(this->m_description.GetJsonForFullySetInstance());
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeFullySetInstance_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializationSettings.m_keepDefaults = false;

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateFullySetInstance();
        auto defaultInstance = this->m_description.CreateDefaultInstance();
        
        ResultCode result = serializer->Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            instance.get(), defaultInstance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        this->Expect_DocStrEq(this->m_description.GetJsonForFullySetInstance());
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializePartialInstance_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            this->m_serializationSettings.m_keepDefaults = false;

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreatePartialDefaultInstance();
            ASSERT_NE(nullptr, instance);
            auto defaultInstance = this->m_description.CreateDefaultInstance();
            
            ResultCode result = serializer->Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
                instance.get(), defaultInstance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);

            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
            this->Expect_DocStrEq(this->m_description.GetJsonForPartialDefaultInstance());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeEmptyArray_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType) && !this->m_features.m_fixedSizeArray)
        {
            this->m_serializationSettings.m_keepDefaults = true;

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultInstance();
            
            ResultCode result = serializer->Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
                instance.get(), instance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);

            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_EQ(Outcomes::Success, result.GetOutcome());
            this->Expect_DocStrEq("[]");
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_HaltedThroughCallback_StoreFailsAndHaltReported)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_jsonDocument->Parse(this->m_description.GetJsonForFullySetInstance().data());
        ASSERT_FALSE(this->m_jsonDocument->HasParseError());

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateFullySetInstance();
        auto defaultInstance = this->m_description.CreateDefaultInstance();

        auto reporting = [](AZStd::string_view message, ResultCode result,
            AZStd::string_view) -> ResultCode
        {
            EXPECT_FALSE(message.empty());
            return ResultCode(result.GetTask(), Outcomes::Catastrophic);
        };
        this->m_serializationSettings.m_reporting = reporting;

        ResultCode result = serializer->Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            instance.get(), defaultInstance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);

        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());
        EXPECT_EQ(Processing::Halted, result.GetProcessing());
    }

    TYPED_TEST_P(JsonSerializerConformityTests, StoreLoad_RoundTripWithPartialDefault_IdenticalInstances)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            this->m_serializationSettings.m_keepDefaults = false;

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreatePartialDefaultInstance();
            auto defaultInstance = this->m_description.CreateDefaultInstance();

            rapidjson::Value convertedValue = this->CreateExplicitDefault();
            ResultCode result = serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(),
                instance.get(), defaultInstance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            auto loadedInstance = this->m_description.CreateDefaultInstance();
            result = serializer->Load(loadedInstance.get(), azrtti_typeid(*loadedInstance),
                convertedValue, this->m_path, this->m_deserializationSettings);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            EXPECT_TRUE(this->m_description.AreEqual(*instance, *loadedInstance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, StoreLoad_RoundTripWithFullSet_IdenticalInstances)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            this->m_serializationSettings.m_keepDefaults = false;

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateFullySetInstance();
            auto defaultInstance = this->m_description.CreateDefaultInstance();

            rapidjson::Value convertedValue = this->CreateExplicitDefault();
            ResultCode result = serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(),
                instance.get(), defaultInstance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            auto loadedInstance = this->m_description.CreateDefaultInstance();
            result = serializer->Load(loadedInstance.get(), azrtti_typeid(*loadedInstance),
                convertedValue, this->m_path, this->m_deserializationSettings);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            EXPECT_TRUE(this->m_description.AreEqual(*instance, *loadedInstance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, StoreLoad_RoundTripWithDefaultsKept_IdenticalInstances)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            this->m_serializationSettings.m_keepDefaults = true;

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateFullySetInstance();
            
            rapidjson::Value convertedValue = this->CreateExplicitDefault();
            ResultCode result = serializer->Store(convertedValue, this->m_jsonDocument->GetAllocator(),
                instance.get(), instance.get(), azrtti_typeid(*instance), this->m_path, this->m_serializationSettings);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            auto loadedInstance = this->m_description.CreateDefaultInstance();
            result = serializer->Load(loadedInstance.get(), azrtti_typeid(*loadedInstance),
                convertedValue, this->m_path, this->m_deserializationSettings);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            EXPECT_TRUE(this->m_description.AreEqual(*instance, *loadedInstance));
        }
    }

    REGISTER_TYPED_TEST_CASE_P(JsonSerializerConformityTests,
        Load_InvalidTypeOfNullType_ReturnsUnsupported,
        Load_InvalidTypeOfFalseType_ReturnsUnsupported,
        Load_InvalidTypeOfTrueType_ReturnsUnsupported,
        Load_InvalidTypeOfObjectType_ReturnsUnsupported,
        Load_InvalidTypeOfArrayType_ReturnsUnsupported,
        Load_InvalidTypeOfStringType_ReturnsUnsupported,
        Load_InvalidTypeOfNumberType_ReturnsUnsupported,
        
        Load_DeserializeUnreflectedType_ReturnsUnsupported,
        Load_DeserializeEmptyObject_SucceedsAndObjectMatchesDefaults,
        Load_DeserializeEmptyArray_SucceedsAndObjectMatchesDefaults,
        Load_DeserializeFullySetInstance_SucceedsAndObjectMatchesFullySetInstance,
        Load_DeserializePartialInstance_SucceedsAndObjectMatchesParialInstance,
        Load_InsertAdditionalData_SucceedsAndObjectMatchesFullySetInstance,
        Load_HaltedThroughCallback_LoadFailsAndHaltReported,
        Load_CorruptedValue_SucceedsWithDefaultValuesUsed,

        Store_SerializeUnreflectedType_ReturnsUnsupported,
        Store_SerializeDefaultInstance_EmptyJsonReturned,
        Store_SerializeWithDefaultsKept_FullyWrittenJson,
        Store_SerializeFullySetInstance_StoredSuccessfullyAndJsonMatches,
        Store_SerializePartialInstance_StoredSuccessfullyAndJsonMatches,
        Store_SerializeEmptyArray_StoredSuccessfullyAndJsonMatches,
        Store_HaltedThroughCallback_StoreFailsAndHaltReported,
        
        StoreLoad_RoundTripWithPartialDefault_IdenticalInstances,
        StoreLoad_RoundTripWithFullSet_IdenticalInstances,
        StoreLoad_RoundTripWithDefaultsKept_IdenticalInstances);
} // namespace JsonSerializationTests
