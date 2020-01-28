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

#include <AzCore/Serialization/Json/MapSerializer.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    template<typename Map, typename Serializer>
    class MapBaseTestDescription :
        public JsonSerializerConformityTestDescriptor<Map>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<Serializer>();
        }

        AZStd::shared_ptr<Map> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Map>();
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_requiresTypeIdLookups = true;
            features.m_supportsPartialInitialization = false;
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Map>();
        }

        bool AreEqual(const Map& lhs, const Map& rhs) override
        {
            return lhs == rhs;
        }
    };
    
    template<template<typename...> class T, typename Serializer>
    class MapArrayTestDescription final :
        public MapBaseTestDescription<T<int, double>, Serializer>
    {
    public:
        using Map = T<int, double>;

        AZStd::shared_ptr<Map> CreateFullySetInstance() override
        {
            auto instance = AZStd::make_shared<Map>();
            instance->emplace(AZStd::make_pair(142, 142.0));
            instance->emplace(AZStd::make_pair(242, 242.0));
            instance->emplace(AZStd::make_pair(342, 342.0));
            return instance;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                [ 
                    { "Key": 142, "Value": 142.0 },
                    { "Key": 242, "Value": 242.0 },
                    { "Key": 342, "Value": 342.0 }
                ]
            )";
        }
    };

    template<template<typename...> class T, typename Serializer>
    class MapObjectTestDescription final :
        public MapBaseTestDescription<T<AZStd::string, double>, Serializer>
    {
    public:
        using Map = T<AZStd::string, double>;

        AZStd::shared_ptr<Map> CreateFullySetInstance() override
        {
            auto instance = AZStd::make_shared<Map>();
            instance->emplace(AZStd::make_pair(AZStd::string("a"), 142.0));
            instance->emplace(AZStd::make_pair(AZStd::string("b"), 242.0));
            instance->emplace(AZStd::make_pair(AZStd::string("c"), 342.0));
            return instance;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                {
                    "a": 142.0,
                    "b": 242.0,
                    "c": 342.0
                }
            )";
        }
    };

    template<template<typename...> class T, typename Serializer>
    class MapPointerTestDescription final :
        public MapBaseTestDescription<T<SimpleClass*, SimpleClass*>, Serializer>
    {
    public:
        using Map = T<SimpleClass*, SimpleClass*>;

        static void Delete(Map* map)
        {
            for (auto& entry : *map)
            {
                delete entry.first;
                delete entry.second;
            }
            delete map;
        }

        AZStd::shared_ptr<Map> CreateDefaultInstance() override
        {
            return AZStd::shared_ptr<Map>(new Map{}, &Delete);
        }

        AZStd::shared_ptr<Map> CreatePartialDefaultInstance() 
        {
            auto instance = AZStd::shared_ptr<Map>(new Map{}, &Delete);
            instance->emplace(AZStd::make_pair(aznew SimpleClass(), aznew SimpleClass(188, 188.0)));
            instance->emplace(AZStd::make_pair(aznew SimpleClass(242, 242.0), aznew SimpleClass()));
            instance->emplace(AZStd::make_pair(aznew SimpleClass(342, 342.0), aznew SimpleClass(388, 388.0)));
            return instance;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        { 
            return R"(
                [ 
                    { "Key": {                            }, "Value": { "var1": 188, "var2": 188.0 } },
                    { "Key": { "var1": 242, "var2": 242.0 }, "Value": {                            } },
                    { "Key": { "var1": 342, "var2": 342.0 }, "Value": { "var1": 388, "var2": 388.0 } }
                ]
            )";
        }

        AZStd::shared_ptr<Map> CreateFullySetInstance() override
        {
            auto instance = AZStd::shared_ptr<Map>(new Map{}, &Delete);
            instance->emplace(AZStd::make_pair(aznew SimpleClass(142, 142.0), aznew SimpleClass(188, 188.0)));
            instance->emplace(AZStd::make_pair(aznew SimpleClass(242, 242.0), aznew SimpleClass(288, 288.0)));
            instance->emplace(AZStd::make_pair(aznew SimpleClass(342, 342.0), aznew SimpleClass(388, 388.0)));
            return instance;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                [ 
                    { "Key": { "var1": 142, "var2": 142.0 }, "Value": { "var1": 188, "var2": 188.0 } },
                    { "Key": { "var1": 242, "var2": 242.0 }, "Value": { "var1": 288, "var2": 288.0 } },
                    { "Key": { "var1": 342, "var2": 342.0 }, "Value": { "var1": 388, "var2": 388.0 } }
                ]
            )";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            MapBaseTestDescription<T<SimpleClass*, SimpleClass*>, Serializer>::ConfigureFeatures(features);
            features.m_supportsPartialInitialization = true;
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            SimpleClass::Reflect(context, true);
            MapBaseTestDescription<T<SimpleClass*, SimpleClass*>, Serializer>::Reflect(context);
        }

        bool AreEqual(const Map& lhs, const Map& rhs) override
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }

            auto compare = [](typename Map::const_reference lhs, typename Map::const_reference rhs) -> bool
            {
                return
                    lhs.first->Equals(*rhs.first, true) &&
                    lhs.second->Equals(*rhs.second, true);
            };
            return AZStd::equal(lhs.begin(), lhs.end(), rhs.begin(), compare);
        }
    };
    
    using MapSerializerConformityTestTypes = ::testing::Types<
        // Tests for storing to arrays.
        MapArrayTestDescription<AZStd::map, AZ::JsonMapSerializer>,
        MapArrayTestDescription<AZStd::unordered_map, AZ::JsonUnorderedMapSerializer>,
        MapArrayTestDescription<AZStd::unordered_multimap, AZ::JsonUnorderedMapSerializer>,
        // Tests for storing to strings.
        MapObjectTestDescription<AZStd::map, AZ::JsonMapSerializer>,
        MapObjectTestDescription<AZStd::unordered_map, AZ::JsonUnorderedMapSerializer>,
        MapObjectTestDescription<AZStd::unordered_multimap, AZ::JsonUnorderedMapSerializer>,
        // Test for storing pointers for both key and value.
        MapPointerTestDescription<AZStd::map, AZ::JsonMapSerializer>,
        MapPointerTestDescription<AZStd::unordered_map, AZ::JsonUnorderedMapSerializer>,
        MapPointerTestDescription<AZStd::unordered_multimap, AZ::JsonUnorderedMapSerializer>
    >;
    INSTANTIATE_TYPED_TEST_CASE_P(MapSerializer, JsonSerializerConformityTests, MapSerializerConformityTestTypes);
    
    
    struct TestString
    {
        AZ_TYPE_INFO(TestString, "{BBC6FCD2-9FED-425E-8523-0598575B960A}");
        AZStd::string m_value;
        TestString() : m_value("Hello") {}
        explicit TestString(AZStd::string_view value) : m_value(value) {}

        bool operator==(const TestString& rhs) const { return m_value.compare(rhs.m_value) == 0; }
        bool operator!=(const TestString& rhs) const { return m_value.compare(rhs.m_value) != 0; }
    };

    class TestStringSerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(TestStringSerializer, "{05012877-0A5C-4514-8AC2-695E753C77A2}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR(TestStringSerializer, AZ::SystemAllocator, 0);

        AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid&, const rapidjson::Value& inputValue,
            AZ::StackedString& path, const AZ::JsonDeserializerSettings& settings)
        {
            using namespace AZ::JsonSerializationResult;
            *reinterpret_cast<AZStd::string*>(outputValue) = AZStd::string_view(inputValue.GetString(), inputValue.GetStringLength());
            return Result(settings, "Test load", ResultCode::Success(Tasks::ReadField), path);
        }
        AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
            const void* inputValue, const void* defaultValue, const AZ::Uuid& valueTypeId,
            AZ::StackedString& path, const AZ::JsonSerializerSettings& settings)
        {
            using namespace AZ::JsonSerializationResult;
            const AZStd::string* inputString = reinterpret_cast<const AZStd::string*>(inputValue);
            const AZStd::string* defaultString = reinterpret_cast<const AZStd::string*>(defaultValue);
            if (defaultString && *inputString == *defaultString)
            {
                return Result(settings, "Test store default", ResultCode::Default(Tasks::WriteValue), path);
            }
            outputValue.SetString(inputString->c_str(), allocator);
            return Result(settings, "Test store", ResultCode::Success(Tasks::WriteValue), path);
        }
    };

    struct TestStringHasher
    {
        AZ_TYPE_INFO(TestStringHasher, "{A59EE3DA-5BB2-4F0B-8390-0832E69F5089}");

        AZStd::size_t operator()(const TestString& value) const
        {
            return AZStd::hash<AZStd::string>{}(value.m_value);
        }
    };

    struct SimpleClassHasher
    {
        AZ_TYPE_INFO(SimpleClassHasher, "{79CF2662-36BF-4183-9400-408F0CFB2AA6}");

        AZStd::size_t operator()(const SimpleClass& value) const
        {
            return
                AZStd::hash<decltype(value.m_var1)>{}(value.m_var1) << 32 |
                AZStd::hash<decltype(value.m_var2)>{}(value.m_var2);
        }
    };

    class JsonMapSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        using StringMap = AZStd::map<AZStd::string, AZStd::string>;
        using TestStringMultiMap = AZStd::unordered_multimap<TestString, TestString, TestStringHasher>;
        using SimpleClassMultiMap = AZStd::unordered_multimap<SimpleClass, SimpleClass, SimpleClassHasher>;

        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            SimpleClass::Reflect(context, true);
            
            context->Class<TestString>()->Field("Value", &TestString::m_value);

            context->RegisterGenericType<StringMap>();
            context->RegisterGenericType<TestStringMultiMap>();
            context->RegisterGenericType<SimpleClassMultiMap>();

        }

        void RegisterAdditional(AZStd::unique_ptr<AZ::JsonRegistrationContext>& context) override
        {
            context->Serializer<TestStringSerializer>()->HandlesType<TestString>();
        }

    protected:
        AZ::JsonMapSerializer m_mapSerializer;
        AZ::JsonUnorderedMapSerializer m_unorderedMapSerializer;
    };

    TEST_F(JsonMapSerializerTests, Load_DefaultsForStringKey_LoadedBackWithDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "{}": {},
                "{}": "value_242",
                "{}": "value_342",
                "World": "value_142"
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        TestStringMultiMap values;
        ResultCode result = m_unorderedMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument, 
            m_path, m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());

        EXPECT_EQ(4, values.size());
        auto worldKey = values.find(TestString("World"));
        EXPECT_NE(values.end(), worldKey);
        EXPECT_STRCASEEQ("value_142", worldKey->second.m_value.c_str());
        auto defaultRange = values.equal_range(TestString());
        int valueCounter = 0;
        for (auto it = defaultRange.first; it != defaultRange.second; ++it)
        {
            if (it->second == TestString())
            {
                ASSERT_EQ(0, valueCounter & 1);
                valueCounter |= 1;
            }
            else if(it->second.m_value == "value_242")
            {
                ASSERT_EQ(0, valueCounter & 2);
                valueCounter |= 2;
            }
            else if (it->second.m_value == "value_342")
            {
                ASSERT_EQ(0, valueCounter & 4);
                valueCounter |= 4;
            }
        }
        EXPECT_EQ(7, valueCounter);
    }

    TEST_F(JsonMapSerializerTests, Load_DuplicateKey_EntryIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "Hello": "World",
                "Hello": "Other"
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        StringMap values;
        ResultCode result = m_unorderedMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument,
            m_path, m_deserializationSettings);

        EXPECT_EQ(Processing::PartialAlter, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unavailable, result.GetOutcome());

        auto entry = values.find("Hello");
        ASSERT_NE(values.end(), entry);
        EXPECT_STRCASEEQ("World", entry->second.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_KeyAlreadyInContainer_EntryIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"({ "Hello": "World" })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        StringMap values;
        values.emplace("Hello", "World");
        ResultCode result = m_unorderedMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument,
            m_path, m_deserializationSettings);

        EXPECT_EQ(Processing::Altered, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unavailable, result.GetOutcome());

        auto entry = values.find("Hello");
        ASSERT_NE(values.end(), entry);
        EXPECT_STRCASEEQ("World", entry->second.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_ExplicitlyFailValueLoading_CatastrophicEventPropagated)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"({ "Hello": "World" })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        int counter = 0;
        auto reporting = [&counter](AZStd::string_view message, ResultCode result,
            AZStd::string_view path) -> ResultCode
        {
            return ++counter == 2
                ? ResultCode(result.GetTask(), Outcomes::Catastrophic)
                : result;
           
        };
        m_deserializationSettings.m_reporting = reporting;

        StringMap values;
        ResultCode result = m_unorderedMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument,
            m_path, m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());
    }

    TEST_F(JsonMapSerializerTests, Store_DefaultsWithStringKey_InitializedAsDefault)
    {
        using namespace AZ::JsonSerializationResult;
        
        TestStringMultiMap values;
        values.emplace(TestString("World"), TestString("value_142"));
        values.emplace(TestString(), TestString("value_242"));
        values.emplace(TestString("Hello"), TestString("value_342")); // The default for TestString is "Hello".
        values.emplace(TestString(), TestString());

        ResultCode result = m_unorderedMapSerializer.Store(*m_jsonDocument, m_jsonDocument->GetAllocator(),
            &values, nullptr, azrtti_typeid(&values), m_path, m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            {
                "{}": {},
                "{}": "value_242",
                "{}": "value_342",
                "World": "value_142"
            })");
    }

    TEST_F(JsonMapSerializerTests, Store_DefaultsWithObjectKey_InitializedAsDefault)
    {
        using namespace AZ::JsonSerializationResult;

        SimpleClassMultiMap values;
        values.emplace(SimpleClass(          ), SimpleClass(          ));
        values.emplace(SimpleClass(          ), SimpleClass(188       ));
        values.emplace(SimpleClass(          ), SimpleClass(288, 288.0));
        values.emplace(SimpleClass(342       ), SimpleClass(          ));
        values.emplace(SimpleClass(442       ), SimpleClass(488       ));
        values.emplace(SimpleClass(542       ), SimpleClass(588, 588.0));
        values.emplace(SimpleClass(642, 642.0), SimpleClass(          ));
        values.emplace(SimpleClass(742, 742.0), SimpleClass(788       ));
        values.emplace(SimpleClass(842, 842.0), SimpleClass(888, 888.0));

        ResultCode result = m_unorderedMapSerializer.Store(*m_jsonDocument, m_jsonDocument->GetAllocator(),
            &values, nullptr, azrtti_typeid(&values), m_path, m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            [
                { "Key": {                          }, "Value": {                          } },
                { "Key": {                          }, "Value": {"var1": 188               } },
                { "Key": {                          }, "Value": {"var1": 288, "var2": 288.0} },
                { "Key": {"var1": 342               }, "Value": {                          } },
                { "Key": {"var1": 442               }, "Value": {"var1": 488               } },
                { "Key": {"var1": 542               }, "Value": {"var1": 588, "var2": 588.0} },
                { "Key": {"var1": 642, "var2": 642.0}, "Value": {                          } },
                { "Key": {"var1": 742, "var2": 742.0}, "Value": {"var1": 788               } },
                { "Key": {"var1": 842, "var2": 842.0}, "Value": {"var1": 888, "var2": 888.0} }
            ])");
    }
} // namespace JsonSerializationTests
