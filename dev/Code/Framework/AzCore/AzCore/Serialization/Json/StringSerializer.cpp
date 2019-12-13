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
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Serialization/Json/StringSerializer.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace StringSerializerInternal
    {
        template<typename StringType>
        static JsonSerializationResult::Result Load(void* outputValue, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

            StringType* valAsString = reinterpret_cast<StringType*>(outputValue);

            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
                // fallthrough
            case rapidjson::kObjectType:
                // fallthrough
            case rapidjson::kNullType:
                return JSR::Result(settings, "Unsupported type. String values can't be read from arrays, objects or null.",
                    Tasks::ReadField, Outcomes::Unsupported, path);

            case rapidjson::kStringType:
                *valAsString = StringType(inputValue.GetString(), inputValue.GetStringLength());
                return JSR::Result(settings, "Successfully read string.", ResultCode::Success(Tasks::ReadField), path);

            case rapidjson::kFalseType:
                *valAsString = "False";
                return JSR::Result(settings, "Successfully read string from boolean.", ResultCode::Success(Tasks::ReadField), path);
            case rapidjson::kTrueType:
                *valAsString = "True";
                return JSR::Result(settings, "Successfully read string from boolean.", ResultCode::Success(Tasks::ReadField), path);

            case rapidjson::kNumberType:
            {
                if (inputValue.IsInt64())
                {
                    AZStd::to_string(*valAsString, inputValue.GetInt64());
                }
                else if (inputValue.IsDouble())
                {
                    AZStd::to_string(*valAsString, inputValue.GetDouble());
                }
                else
                {
                    AZStd::to_string(*valAsString, inputValue.GetUint64());
                }
                return JSR::Result(settings, "Successfully read string from number.", ResultCode::Success(Tasks::ReadField), path);
            }

            default:
                return JSR::Result(settings, "Unknown json type encountered for string value.", Tasks::ReadField, Outcomes::Unknown, path);
            }
        }

        template<typename StringType>
        static JsonSerializationResult::Result StoreWithDefault(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
            const void* inputValue, const void* defaultValue, StackedString& path, const JsonSerializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            const StringType& valAsString = *reinterpret_cast<const StringType*>(inputValue);
            if (settings.m_keepDefaults || !defaultValue || (valAsString != *reinterpret_cast<const StringType*>(defaultValue)))
            {
                outputValue.SetString(valAsString.c_str(), aznumeric_caster(valAsString.length()), allocator);
                return JSR::Result(settings, "String successfully stored.", ResultCode::Success(Tasks::WriteValue), path);
            }

            return JSR::Result(settings, "Default String used.", ResultCode::Default(Tasks::WriteValue), path);
        }
    } // namespace Internal

    AZ_CLASS_ALLOCATOR_IMPL(JsonStringSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonStringSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<AZStd::string>() == outputValueTypeId,
            "Unable to deserialize AZStd::string to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return StringSerializerInternal::Load<AZStd::string>(outputValue, inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonStringSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, 
        StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<AZStd::string>() == valueTypeId, "Unable to serialize AZStd::string to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        return StringSerializerInternal::StoreWithDefault<AZStd::string>(outputValue, allocator, inputValue, defaultValue, path, settings);
    }


    AZ_CLASS_ALLOCATOR_IMPL(JsonOSStringSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonOSStringSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<OSString>() == outputValueTypeId,
            "Unable to deserialize OSString to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return StringSerializerInternal::Load<OSString>(outputValue, inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonOSStringSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, 
        StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<OSString>() == valueTypeId, "Unable to serialize OSString to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return StringSerializerInternal::StoreWithDefault<OSString>(outputValue, allocator, inputValue, defaultValue, path, settings);
    }
} // namespace AZ
