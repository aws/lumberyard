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

#include <AzCore/Serialization/Json/BasicContainerSerializer.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/iterator.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    template<typename Container>
    class BasicContainerConformityTestDescriptor :
        public JsonSerializerConformityTestDescriptor<Container>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonBasicContainerSerializer>();
        }

        AZStd::shared_ptr<Container> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Container>();
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.m_supportsPartialInitialization = false;
            features.m_requiresTypeIdLookups = true;
        }
    };

    template<typename Container>
    class SimpleTestDescription final :
        public BasicContainerConformityTestDescriptor<Container>
    {
    public: 
        AZStd::shared_ptr<Container> CreateFullySetInstance() override
        {
            return AZStd::make_shared<Container>(Container{ 188, 288, 388 });
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[188, 288, 388]";
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Container>();
        }

        bool AreEqual(const Container& lhs, const Container& rhs) override
        {
            return lhs == rhs;
        }
    };

    struct SimplePointerTestDescriptionCompare
    {
        AZ_TYPE_INFO(SimplePointerTestDescriptionCompare, "{115495B5-2572-4DC2-A102-6B59BC85B974}");
        bool operator()(const int* lhs, const int* rhs)
        {
            return *lhs == *rhs;
        };
    };
    struct SimplePointerTestDescriptionLess
    {
        AZ_TYPE_INFO(SimplePointerTestDescriptionLess, "{2C883A26-A2C6-481D-8F10-96E619FB56EE}");
        bool operator()(const int* lhs, const int* rhs)
        {
            return *lhs < *rhs;
        };
    };

    template<typename Container>
    class SimplePointerTestDescription final :
        public BasicContainerConformityTestDescriptor<Container>
    {
    public:
        static void Delete(Container* container)
        {
            for (auto* entry : *container)
            {
                azfree(entry);
            }
            delete container;
        }

        AZStd::shared_ptr<Container> CreateDefaultInstance() override
        {
            return AZStd::shared_ptr<Container>(new Container{}, &SimplePointerTestDescription::Delete);
        }

        AZStd::shared_ptr<Container> CreateFullySetInstance() override
        {
            int* value0 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
            int* value1 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
            int* value2 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));

            *value0 = 188;
            *value1 = 288;
            *value2 = 388;

            return AZStd::shared_ptr<Container>(new Container{ value0, value1, value2 },
                &SimplePointerTestDescription::Delete);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[188, 288, 388]";
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Container>();
        }

        bool AreEqual(const Container& lhs, const Container& rhs) override
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }

            auto compare = [](const int* lhs, const int* rhs) -> bool
            {
                return *lhs == *rhs;
            };
            return AZStd::equal(lhs.begin(), lhs.end(), rhs.begin(), SimplePointerTestDescriptionCompare{});
        }
    };

    template<typename Container>
    class ComplextTestDescription final :
        public BasicContainerConformityTestDescriptor<Container>
    {
    public:
        AZStd::shared_ptr<Container> CreateFullySetInstance() override
        {
            SimpleClass values[3];
            values[0].m_var1 = 188;
            values[0].m_var2 = 288.0;

            values[1].m_var1 = 388;
            values[1].m_var2 = 488.0;

            values[2].m_var1 = 588;
            values[2].m_var2 = 688.0;

            auto instance = AZStd::make_shared<Container>();
            *instance = { values[0], values[1], values[2] };
            return instance;
        }

        AZStd::shared_ptr<Container> CreatePartialDefaultInstance() override
        {
            SimpleClass values[3];
            // These defaults are specifically chosen to make sure the elements
            // are kept in the same order in AZStd::set.
            values[0].m_var1 = 10;
            values[2].m_var2 = 688.0;

            auto instance = AZStd::make_shared<Container>();
            *instance = { values[0], values[1], values[2] };
            return instance;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"([
                {
                    "var1": 188,
                    "var2": 288.0
                }, 
                {
                    "var1": 388,
                    "var2": 488.0
                }, 
                {
                    "var1": 588,
                    "var2": 688.0
                }])";
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"([
                {
                    "var1": 10
                }, 
                {}, 
                {
                    "var2": 688.0
                }])";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            BasicContainerConformityTestDescriptor<Container>::ConfigureFeatures(features);
            features.m_supportsPartialInitialization = true;
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            SimpleClass::Reflect(context, true);
            context->RegisterGenericType<Container>();
        }

        bool AreEqual(const Container& lhs, const Container& rhs) override
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }
            
            return AZStd::equal(lhs.begin(), lhs.end(), rhs.begin());
        }
    };

    using BasicContainerConformityTestTypes = ::testing::Types
    <
        SimpleTestDescription<AZStd::list<int>>,
        SimpleTestDescription<AZStd::set<int>>,
        SimpleTestDescription<AZStd::vector<int>>,
        SimplePointerTestDescription<AZStd::list<int*>>,
        SimplePointerTestDescription<AZStd::set<int*, SimplePointerTestDescriptionLess>>,
        SimplePointerTestDescription<AZStd::vector<int*>>,
        ComplextTestDescription<AZStd::list<SimpleClass>>,
        ComplextTestDescription<AZStd::set<SimpleClass>>,
        ComplextTestDescription<AZStd::vector<SimpleClass>>
    >;
    INSTANTIATE_TYPED_TEST_CASE_P(BasicContainers, JsonSerializerConformityTests, BasicContainerConformityTestTypes);

    class JsonSetSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        using Set = AZStd::set<int>;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<AZ::JsonBasicContainerSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext) override
        {
            serializeContext->RegisterGenericType<Set>();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonBasicContainerSerializer> m_serializer;
    };

    TEST_F(JsonSetSerializerTests, Load_DuplicateEntry_CompletesWithDataLeft)
    {
        using namespace AZ::JsonSerializationResult;
        Set instance;

        rapidjson::Value testVal(rapidjson::kArrayType);
        testVal.PushBack(rapidjson::Value(188), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value(288), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value(288), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value(388), m_jsonDocument->GetAllocator());
        ResultCode result = m_serializer->Load(&instance, azrtti_typeid(&instance), testVal, m_path, m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unavailable, result.GetOutcome());
        EXPECT_EQ(Processing::PartialAlter, result.GetProcessing());
        
        EXPECT_NE(instance.end(), instance.find(188));
        EXPECT_NE(instance.end(), instance.find(288));
        EXPECT_NE(instance.end(), instance.find(388));
    }

    TEST_F(JsonSetSerializerTests, Load_EntryAlreadyInContainer_EntryIgnored)
    {
        using namespace AZ::JsonSerializationResult;
        Set instance;
        instance.insert(188);

        rapidjson::Value testVal(rapidjson::kArrayType);
        testVal.PushBack(rapidjson::Value(188), m_jsonDocument->GetAllocator());

        ResultCode result = m_serializer->Load(&instance, azrtti_typeid(&instance), testVal, m_path, m_deserializationSettings);

        EXPECT_EQ(Processing::Altered, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unavailable, result.GetOutcome());
        EXPECT_NE(instance.end(), instance.find(188));
    }
} // namespace JsonSerializationTests
