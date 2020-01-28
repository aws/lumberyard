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

#include <AzCore/JSON/writer.h>
#include <AzCore/std/iterator.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    void BaseJsonSerializerFixture::SetUp()
    {
        AllocatorsTestFixture::SetUp();

        auto reportCallback = [](AZStd::string_view /*message*/, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view /*target*/)->
            AZ::JsonSerializationResult::ResultCode
        {
            return result;
        };

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

        m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();
        m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
        RegisterAdditional(m_serializeContext);
        RegisterAdditional(m_jsonRegistrationContext);

        m_jsonDocument = AZStd::make_unique<rapidjson::Document>();

        m_serializationSettings.m_serializeContext = m_serializeContext.get();
        m_serializationSettings.m_registrationContext = m_jsonRegistrationContext.get();
        m_serializationSettings.m_reporting = reportCallback;

        m_deserializationSettings.m_serializeContext = m_serializeContext.get();
        m_deserializationSettings.m_registrationContext = m_jsonRegistrationContext.get();
        m_deserializationSettings.m_reporting = reportCallback;
    }

    void BaseJsonSerializerFixture::TearDown()
    {
        m_path.Reset();

        m_jsonDocument.reset();
        
        m_jsonRegistrationContext->EnableRemoveReflection();
        RegisterAdditional(m_jsonRegistrationContext);
        m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
        m_jsonRegistrationContext->DisableRemoveReflection();
     
        m_serializeContext->EnableRemoveReflection();
        RegisterAdditional(m_serializeContext);
        m_jsonSystemComponent->Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();

        m_jsonSystemComponent.reset();
        m_jsonRegistrationContext.reset();
        m_serializeContext.reset();

        AllocatorsTestFixture::TearDown();
    }

    void BaseJsonSerializerFixture::RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& /*serializeContext*/)
    {
    }

    void BaseJsonSerializerFixture::RegisterAdditional(AZStd::unique_ptr<AZ::JsonRegistrationContext>& /*serializeContext*/)
    {
    }

    rapidjson::Value BaseJsonSerializerFixture::CreateExplicitDefault()
    {
        return rapidjson::Value(rapidjson::kObjectType);
    }

    void BaseJsonSerializerFixture::Expect_ExplicitDefault(rapidjson::Value& value)
    {
        ASSERT_TRUE(value.IsObject());
        EXPECT_EQ(0, value.MemberCount());
    }

    void BaseJsonSerializerFixture::Expect_DocStrEq(AZStd::string_view testString)
    {
        Expect_DocStrEq(testString, true);
    }

    void BaseJsonSerializerFixture::Expect_DocStrEq(AZStd::string_view testString, bool stripWhitespace)
    {
        rapidjson::StringBuffer scratchBuffer;
        rapidjson::Writer<decltype(scratchBuffer)> writer(scratchBuffer);
        m_jsonDocument->Accept(writer);
        
        if (stripWhitespace)
        {
            AZStd::string cleanedString = testString;
            cleanedString.erase(
                AZStd::remove_if(cleanedString.begin(), cleanedString.end(), ::isspace), cleanedString.end());

            EXPECT_STRCASEEQ(cleanedString.c_str(), scratchBuffer.GetString());
        }
        else
        {
            EXPECT_STRCASEEQ(testString.data(), scratchBuffer.GetString());
        }
    }

    rapidjson::Value BaseJsonSerializerFixture::TypeToInjectionValue(rapidjson::Type type)
    {
        rapidjson::Value result;
        switch (type)
        {
        case rapidjson::Type::kNullType:
            result.SetNull();
            break;
        case rapidjson::Type::kFalseType:
            result.SetBool(false);
            break;
        case rapidjson::Type::kTrueType:
            result.SetBool(true);
            break;
        case rapidjson::Type::kObjectType:
            result.SetObject();
            break;
        case rapidjson::Type::kArrayType:
            result.SetArray();
            break;
        case rapidjson::Type::kStringType:
            result.SetString("Added to object for testing purposes");
            break;
        case rapidjson::Type::kNumberType:
            result.SetUint(0xbadf00d);
            break;
        default:
            AZ_Assert(false, "Unsupported RapidJSON type: %i.", static_cast<int>(type));
        }
        return result;
    }

    void BaseJsonSerializerFixture::InjectAdditionalFields(rapidjson::Value& value, rapidjson::Type typeToInject, 
        rapidjson::Document::AllocatorType& allocator)
    {
        if (value.IsObject())
        {
            uint32_t counter = 0;
            rapidjson::Value temp(rapidjson::kObjectType);
            for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
            {
                AZStd::string name = AZStd::string::format("Comment %i", counter);
                temp.AddMember(rapidjson::Value(name.c_str(), aznumeric_caster(name.length()), allocator),
                    TypeToInjectionValue(typeToInject), allocator);
                InjectAdditionalFields(it->value, typeToInject, allocator);
                temp.AddMember(AZStd::move(it->name), AZStd::move(it->value), allocator);
                counter++;
            }
            temp.Swap(value);
        }
        else if (value.IsArray())
        {
            rapidjson::Value temp(rapidjson::kArrayType);
            for (auto it = value.Begin(); it != value.End(); ++it)
            {
                temp.PushBack(TypeToInjectionValue(typeToInject), allocator);
                InjectAdditionalFields(*it, typeToInject, allocator);
                temp.PushBack(AZStd::move(*it), allocator);
            }
            temp.Swap(value);
        }
    }

    void BaseJsonSerializerFixture::CorruptFields(rapidjson::Value& value, rapidjson::Type typeToInject)
    {
        if (value.IsObject())
        {
            rapidjson::SizeType count = value.MemberCount();
            if (count > 0)
            {
                count /= 2;
                auto memberIt = value.MemberBegin();
                std::advance(memberIt, count);
                CorruptFields(memberIt->value, typeToInject);
            }
        }
        else if (value.IsArray())
        {
            rapidjson::SizeType count = value.Size();
            if (count > 0)
            {
                count /= 2;
                CorruptFields(value[count], typeToInject);
            }
        }
        else
        {
            value = TypeToInjectionValue(typeToInject);
        }
    }
} // namespace UnitTest
