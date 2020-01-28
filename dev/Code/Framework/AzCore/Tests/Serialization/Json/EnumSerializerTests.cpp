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
#include <AzCore/Serialization/Json/IntSerializer.h>
#include <AzCore/std/string/conversions.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/EnumSerializer.h>

namespace JsonSerializationTests
{
    enum UnscopedEnumFlags : int32_t
    {
        UnscopedFlags0 = 0,
        UnscopedFlags1,
        UnscopedFlags2,
        UnscopedFlags3
    };

    enum UnscopedEnumBitFlags : uint8_t
    {
        U8UnscopedBitFlags0 = 0,
        U8UnscopedBitFlags1,
        U8UnscopedBitFlags2,
        U8UnscopedBitFlags3
    };

    enum class ScopedEnumFlagsU8 : uint8_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2,
        ScopedFlag3 = 129
    };

    enum class ScopedEnumFlagsS16 : int16_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2 = -8,
        ScopedFlag3 = 22
    };

    enum class ScopedEnumFlagsU16 : uint16_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2,
        ScopedFlag3 = 32768
    };

    enum class ScopedEnumFlagsS32 : int32_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2 = -8,
        ScopedFlag3 = 22
    };

    enum class ScopedEnumFlagsU32 : uint32_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2,
        ScopedFlag3 = 2147483648
    };

    enum class ScopedEnumFlagsS64 : int64_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2 = -8,
        ScopedFlag3 = 22
    };

    enum class ScopedEnumFlagsU64 : uint64_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2,
        ScopedFlag3 = (1 < 63) + 5
    };

    enum class ScopedEnumBitFlagsS64 : int64_t
    {
        BitFlagNegative = 0b1000'0000'0000'0000'0000'0000'0000'0100'0000'0001'0000'0000'0000'0000'0000'0000,
        BitFlag0 = 0,
        BitFlag1 = 0b1,
        BitFlag2 = 0b10,
        BitFlag3 = 0b100,
        BitFlag4 = 0b1000,

        BitFlag1And2 = BitFlag1 | BitFlag2,
        BitFlag2And4 = BitFlag2 | BitFlag4,
    };

    enum class ScopedEnumBitFlagsU64 : uint64_t
    {
        BitFlag0 = 0,
        BitFlag1 = 0b1,
        BitFlag2 = 0b10,
        BitFlag3 = 0b100,
        BitFlag4 = 0b1000,

        BitFlag1And2 = BitFlag1 | BitFlag2,
        BitFlag2And4 = BitFlag2 | BitFlag4,
    };

    enum class ScopedEnumBitFlagsNoZero : uint64_t
    {
        BitFlag1 = 0b1,
        BitFlag2 = 0b10,
        BitFlag3 = 0b100,
        BitFlag4 = 0b1000,

        BitFlag1And2 = BitFlag1 | BitFlag2,
        BitFlag2And4 = BitFlag2 | BitFlag4,
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::UnscopedEnumFlags, "{2F8F49D7-0AEC-4F73-9DC9-0B883B86ACDB}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::UnscopedEnumBitFlags, "{A9B115BD-A3E5-48E6-B60C-0C7FA52DC532}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsU8, "{2972770B-5738-4373-B893-E35D3008FE8F}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsS16, "{867590AB-E5BC-4092-A8A4-4652B14953B3}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsU16, "{4D30ECAB-F211-4416-9AD7-B8F1DCF7BE5C}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsS32, "{2231A213-1AC1-46E2-80FD-0C45389F3ABC}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsU32, "{0A45EFB3-D2C6-4504-ABD0-494E31257722}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsS64, "{77897A4E-D1DC-440F-B209-3A961685D4DA}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsU64, "{97A03F02-A02E-4ED3-94DA-80A825FBCEE6}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumBitFlagsS64, "{F06D6834-59FA-4BE3-A80E-EAB4D5B42CA7}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumBitFlagsU64, "{10DCE70D-1362-4726-BB9B-678473076E6E}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumBitFlagsNoZero, "{8DE2AE7B-85F6-459A-AC6C-82C2F3225C8E}");
}

namespace JsonSerializationTests
{
    template<typename EnumType>
    class TypedJsonEnumSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        using SerializerType = AZ::JsonEnumSerializer;
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
    };

    using EnumTypes = ::testing::Types<
        UnscopedEnumFlags,
        UnscopedEnumBitFlags,
        ScopedEnumFlagsU8,
        ScopedEnumFlagsS16,
        ScopedEnumFlagsU16,
        ScopedEnumFlagsS32,
        ScopedEnumFlagsU32,
        ScopedEnumFlagsS32,
        ScopedEnumFlagsU32,
        ScopedEnumBitFlagsS64,
        ScopedEnumBitFlagsU64,
        ScopedEnumBitFlagsNoZero
    >;
    TYPED_TEST_CASE(TypedJsonEnumSerializerTests, EnumTypes);

    TYPED_TEST(TypedJsonEnumSerializerTests, Load_UnregistredEnum_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::SerializeContext* serializeContext = this->m_deserializationSettings.m_serializeContext;
        rapidjson::Value testValue(rapidjson::kArrayType);
        TypeParam convertedValue{};

        Result loadResult = this->m_serializer->Load(&convertedValue, azrtti_typeid<TypeParam>(),
            testValue, this->m_path, this->m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();
        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<TypeParam>(0), convertedValue);
    }

    TYPED_TEST(TypedJsonEnumSerializerTests, Store_UnregistredEnum_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::SerializeContext* serializeContext = this->m_deserializationSettings.m_serializeContext;
        TypeParam testValue{};

        rapidjson::Value outputValue;
        Result loadResult = this->m_serializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, nullptr,
            azrtti_typeid<decltype(testValue)>(), this->m_path, m_serializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();
        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(rapidjson::kNullType, outputValue.GetType());
    }

    TYPED_TEST(TypedJsonEnumSerializerTests, Load_InvalidTypeOfObjectType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::SerializeContext* serializeContext = this->m_deserializationSettings.m_serializeContext;
        serializeContext->Enum<TypeParam>();
        rapidjson::Value testValue(rapidjson::kObjectType);
        TypeParam convertedValue{};

        Result loadResult = this->m_serializer->Load(&convertedValue, azrtti_typeid<TypeParam>(),
            testValue, this->m_path, this->m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();
        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<TypeParam>(0), convertedValue);

        serializeContext->EnableRemoveReflection();
        serializeContext->Enum<TypeParam>();
        serializeContext->DisableRemoveReflection();
    }

    TYPED_TEST(TypedJsonEnumSerializerTests, Load_EmptyInstanceOfArrayType_ReturnsDefault)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::SerializeContext* serializeContext = this->m_deserializationSettings.m_serializeContext;
        serializeContext->Enum<TypeParam>();
        rapidjson::Value testValue(rapidjson::kArrayType);
        TypeParam convertedValue{};

        Result loadResult = this->m_serializer->Load(&convertedValue, azrtti_typeid<TypeParam>(),
            testValue, this->m_path, this->m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<TypeParam>(0), convertedValue);

        serializeContext->EnableRemoveReflection();
        serializeContext->Enum<TypeParam>();
        serializeContext->DisableRemoveReflection();
    }

    TYPED_TEST(TypedJsonEnumSerializerTests, Load_EmptyString_ReturnsDefault)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::SerializeContext* serializeContext = this->m_deserializationSettings.m_serializeContext;
        serializeContext->Enum<TypeParam>();
        rapidjson::Value testValue(rapidjson::kStringType);
        TypeParam convertedValue{};

        Result loadResult = this->m_serializer->Load(&convertedValue, azrtti_typeid<TypeParam>(),
            testValue, this->m_path, this->m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<TypeParam>(0), convertedValue);

        serializeContext->EnableRemoveReflection();
        serializeContext->Enum<TypeParam>();
        serializeContext->DisableRemoveReflection();
    }

    class JsonEnumSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        struct ScopedTestJsonReflector
        {
            template<typename Reflector>
            ScopedTestJsonReflector(BaseJsonSerializerFixture& testFixture, Reflector&& reflectFunctor)
                : m_reflectFunctor{ AZStd::forward<Reflector>(reflectFunctor) }
                , m_serializeContext(*testFixture.m_serializeContext)
                , m_jsonRegistrationContext(*testFixture.m_jsonRegistrationContext)
            {
                m_reflectFunctor();
            }
            ~ScopedTestJsonReflector()
            {
                m_serializeContext.EnableRemoveReflection();
                m_jsonRegistrationContext.EnableRemoveReflection();
                m_reflectFunctor();
                m_jsonRegistrationContext.DisableRemoveReflection();
                m_serializeContext.DisableRemoveReflection();
            }

            AZStd::function<void()> m_reflectFunctor;
            AZ::SerializeContext& m_serializeContext;
            AZ::JsonRegistrationContext& m_jsonRegistrationContext;
        };
    };

    TEST_F(JsonEnumSerializerTests, Load_Int_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<UnscopedEnumFlags>()
                ->Value("Flags0", UnscopedEnumFlags::UnscopedFlags0)
                ->Value("Flags1", UnscopedEnumFlags::UnscopedFlags1)
                ->Value("Flags2", UnscopedEnumFlags::UnscopedFlags2)
                ->Value("Flags3", UnscopedEnumFlags::UnscopedFlags3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue;
        testValue.SetInt64(UnscopedEnumFlags::UnscopedFlags1);

        UnscopedEnumFlags convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(UnscopedEnumFlags::UnscopedFlags1, convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_IntOutOfRange_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = UnscopedEnumBitFlags;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::U8UnscopedBitFlags0)
                ->Value("Flags1", EnumType::U8UnscopedBitFlags1)
                ->Value("Flags2", EnumType::U8UnscopedBitFlags2)
                ->Value("Flags3", EnumType::U8UnscopedBitFlags3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue;
        testValue.SetInt64(256);

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_IntInRangeButNotEnumOption_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = UnscopedEnumBitFlags;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::U8UnscopedBitFlags0)
                ->Value("Flags1", EnumType::U8UnscopedBitFlags1)
                ->Value("Flags2", EnumType::U8UnscopedBitFlags2)
                ->Value("Flags3", EnumType::U8UnscopedBitFlags3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue;
        testValue.SetInt64(255);

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(255), convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_InvalidString_ReturnsFailure)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU8;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue("Johannesburg");

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_StringWithEnumOption_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS16;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue("Flags3");

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag3, convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_StringWithInt_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU16;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue("2");

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag2, convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_StringWithIntOutOfRange_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU16;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue("-1");

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_ArrayWithSingleStringEnumOption_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS32;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("Flags1", m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag1, convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_ArrayWithStringIntValue_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS32;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("452", m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(452), convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_ArrayWithStringIntOutOfRange_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS16;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("32769", m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_ArrayWithSingleIntOption_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU32;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack(rapidjson::Value(2), m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag2, convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_ArrayWithIntOptionOutOfRange_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU8;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack(rapidjson::Value(256), m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_ArrayWithInvalidString_ReturnsFailure)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("Cape Town", m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Load_ArrayWithMultipleBitFlags_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("BitFlag1And2", m_jsonDocument->GetAllocator());
        testValue.PushBack("BitFlag2And4", m_jsonDocument->GetAllocator());
        testValue.PushBack(0b1010'0000, m_jsonDocument->GetAllocator());
        EnumType expectedResult(static_cast<EnumType>(static_cast<int64_t>(EnumType::BitFlag1And2) | static_cast<int64_t>(EnumType::BitFlag2And4) | 0b1010'0000));

        EnumType convertedValue{ EnumType::BitFlagNegative };
        Result loadResult = enumJsonSerializer->Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, m_path, m_deserializationSettings);
        ResultCode resultCode = loadResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(expectedResult, convertedValue);
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithSingleOption_ValueIsStored)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue(EnumType::BitFlag3);
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kStringType, outputValue.GetType());
        EXPECT_STREQ("BitFlag3", outputValue.GetString());
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithZeroValueAndReflectionOfZeroOption_ValueIsStoredAsString)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue{};
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kStringType, outputValue.GetType());
        EXPECT_STREQ("BitFlag0", outputValue.GetString());
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithZeroValueAndNoReflectionOfZeroOption_ValueIsStoredAsInt)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsNoZero;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue{};
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kNumberType, outputValue.GetType());
        EXPECT_EQ(0, outputValue.GetInt64());
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithSignedUnderlyingTypeAndNegativeBitFlag_RoundTripsValueSuccessfully)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue{ EnumType::BitFlagNegative };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kNumberType, outputValue.GetType());
        EXPECT_EQ(EnumType::BitFlagNegative, static_cast<EnumType>(outputValue.GetInt64()));
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithMultipleOptionsWhichMatchesAggregateOption_StoresValueInString)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag2) | static_cast<uint64_t>(EnumType::BitFlag4)) };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kStringType, outputValue.GetType());
        EXPECT_STREQ("BitFlag2And4", outputValue.GetString());
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithMultipleOptions_StoresAllOptionsInArray)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag1) | static_cast<uint64_t>(EnumType::BitFlag3)
            | static_cast<uint64_t>(EnumType::BitFlag4)) };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kArrayType, outputValue.GetType());

        ASSERT_EQ(3, outputValue.Size());
        // All three elements are string and the elements
        // are written from largest unsigned integral value to smallest
        ASSERT_EQ(rapidjson::kStringType, outputValue[0].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[1].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[2].GetType());

        EXPECT_STREQ("BitFlag4", outputValue[0].GetString());
        EXPECT_STREQ("BitFlag3", outputValue[1].GetString());
        EXPECT_STREQ("BitFlag1", outputValue[2].GetString());
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithMultipleOptionsAndNonOptionBits_StoresAllValuesInArray)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        constexpr uint64_t nonOptionBits = 0b1011'0110'0000'0000;
        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag1) | static_cast<uint64_t>(EnumType::BitFlag3)
            | static_cast<uint64_t>(EnumType::BitFlag4) | nonOptionBits) };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kArrayType, outputValue.GetType());

        ASSERT_EQ(4, outputValue.Size());
        // The first three elements should be strings
        // The final element should be an integer of the left over bit values
        // that doesn't correspond to a value
        // are written from largest unsigned integral value to smallest
        ASSERT_EQ(rapidjson::kStringType, outputValue[0].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[1].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[2].GetType());
        ASSERT_EQ(rapidjson::kNumberType, outputValue[3].GetType());

        EXPECT_STREQ("BitFlag4", outputValue[0].GetString());
        EXPECT_STREQ("BitFlag3", outputValue[1].GetString());
        EXPECT_STREQ("BitFlag1", outputValue[2].GetString());
        EXPECT_EQ(nonOptionBits, outputValue[3].GetUint64());
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithSignedUnderlyingTypeAndMultipleOptions_StoresValuesInOrderOfHighestUnsignedBitValueFirst)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlagNegative", EnumType::BitFlagNegative)
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag1) | static_cast<uint64_t>(EnumType::BitFlag3)
            | static_cast<uint64_t>(EnumType::BitFlag4) | static_cast<uint64_t>(EnumType::BitFlagNegative)) };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kArrayType, outputValue.GetType());

        ASSERT_EQ(4, outputValue.Size());
        // All four elements should be strings
        // The first correspond element should be the element
        // with the largest unsigned bit va;ie
        ASSERT_EQ(rapidjson::kStringType, outputValue[0].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[1].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[2].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[3].GetType());

        EXPECT_STREQ("BitFlagNegative", outputValue[0].GetString());
        EXPECT_STREQ("BitFlag4", outputValue[1].GetString());
        EXPECT_STREQ("BitFlag3", outputValue[2].GetString());
        EXPECT_STREQ("BitFlag1", outputValue[3].GetString());
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithDefaultValueMatchingEnumOptions_ReturnsDefaulted)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag1And2) | static_cast<uint64_t>(EnumType::BitFlag2And4)) };
        const EnumType* defaultValue = &testValue;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
    }

    TEST_F(JsonEnumSerializerTests, Store_EnumWithDefaultValuesNotMatchingEnumOptions_ReturnsDefaulted)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [this]
        {
            this->m_serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ;
        };
        ScopedTestJsonReflector scopedJsonReflector(*this, AZStd::move(scopedReflector));
        AZ::BaseJsonSerializer* enumJsonSerializer{ m_jsonRegistrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>()) };
        ASSERT_NE(nullptr, enumJsonSerializer);

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(0b0100'0000)) };
        const EnumType* defaultValue = &testValue;
        rapidjson::Value outputValue;
        Result storeResult = enumJsonSerializer->Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue, azrtti_typeid<decltype(testValue)>(),
            m_path, m_serializationSettings);
        ResultCode resultCode = storeResult.GetResultCode();

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
    }

} // namespace JsonSerializationTests
