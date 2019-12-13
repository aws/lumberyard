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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/UuidSerializer.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonUuidSerializer, SystemAllocator, 0);

    JsonUuidSerializer::JsonUuidSerializer()
    {
        m_uuidFormat = AZStd::regex(R"(\{[a-f0-9]{8}-?[a-f0-9]{4}-?[a-f0-9]{4}-?[a-f0-9]{4}-?[a-f0-9]{12}\})",
            AZStd::regex_constants::icase | AZStd::regex_constants::optimize);
    }

    JsonSerializationResult::Result JsonUuidSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Uuid>() == outputValueTypeId,
            "Unable to deserialize Uuid to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        Uuid* valAsUuid = reinterpret_cast<Uuid*>(outputValue);

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType: // fallthrough
        case rapidjson::kObjectType:// fallthrough
        case rapidjson::kFalseType: // fallthrough
        case rapidjson::kTrueType:  // fallthrough
        case rapidjson::kNumberType:// fallthrough
        case rapidjson::kNullType:
            return JSR::Result(settings, "Unsupported type. Uuids can only be read from strings.",
                Tasks::ReadField, Outcomes::Unsupported, path);

        case rapidjson::kStringType:
        {
            AZStd::smatch uuidMatch;
            if (AZStd::regex_search(inputValue.GetString(), inputValue.GetString() + inputValue.GetStringLength(), uuidMatch, m_uuidFormat))
            {
                AZ_Assert(uuidMatch.size() > 0, "Regex found uuid pattern, but zero matches were returned.");
                size_t stringLength = uuidMatch.suffix().first - uuidMatch[0].first;
                AZ::Uuid temp = Uuid::CreateString(uuidMatch[0].first, stringLength);
                if (temp.IsNull())
                {
                    return JSR::Result(settings, "Failed to create uuid from string.",
                        Tasks::ReadField, Outcomes::Unsupported, path);
                }
                *valAsUuid = temp;
                return JSR::Result(settings, "Successfully read uuid.", ResultCode::Success(Tasks::ReadField), path);
            }
            else
            {
                return JSR::Result(settings, "No part of the string could be interpreted as a uuid.", Tasks::ReadField, Outcomes::Unsupported, path);
            }
        }

        default:
            return JSR::Result(settings, "Unknown json type encountered in Uuid.", Tasks::ReadField, Outcomes::Unknown, path);
        }
    }

    JsonSerializationResult::Result JsonUuidSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, 
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Uuid>() == valueTypeId, "Unable to serialize Uuid to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const Uuid& valAsUuid = *reinterpret_cast<const Uuid*>(inputValue);
        if (settings.m_keepDefaults || !defaultValue || (valAsUuid != *reinterpret_cast<const Uuid*>(defaultValue)))
        {
            AZStd::string valAsString = valAsUuid.ToString<AZStd::string>();
            outputValue.SetString(valAsString.c_str(), aznumeric_caster(valAsString.length()), allocator);
            return JSR::Result(settings, "Uuid successfully stored", ResultCode::Success(Tasks::WriteValue), path);
        }
        
        return JSR::Result(settings, "Default Uuid used.", ResultCode::Default(Tasks::WriteValue), path);
    }
} // namespace AZ
