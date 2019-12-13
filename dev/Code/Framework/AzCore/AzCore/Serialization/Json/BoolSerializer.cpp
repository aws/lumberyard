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
#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonBoolSerializer, SystemAllocator, 0);

    namespace SerializerInternal
    {
        static JsonSerializationResult::Result TextToValue(bool* outputValue, const char* text, size_t textLength,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            if (text && textLength > 0)
            {
                static constexpr const char trueString[] = "true";
                static constexpr const char falseString[] = "false";
                // remove null terminator for string length counts
                // rapidjson stringlength doesn't include it in length calculations, but sizeof() will
                static constexpr size_t trueStringLength = sizeof(trueString) - 1;
                static constexpr size_t falseStringLength = sizeof(falseString) - 1;

                if ((textLength == trueStringLength) && azstrnicmp(text, trueString, trueStringLength) == 0)
                {
                    *outputValue = true;
                    return JSR::Result(settings, "Successfully read boolean from string.", ResultCode::Success(Tasks::ReadField), path);
                }
                else if ((textLength == falseStringLength) && azstrnicmp(text, falseString, falseStringLength) == 0)
                {
                    *outputValue = false;
                    return JSR::Result(settings, "Successfully read boolean from string.", ResultCode::Success(Tasks::ReadField), path);
                }
                else
                {
                    // Intentionally not checking errno here for under or overflow, we only care that the value parsed is not zero.
                    char* parseEnd = nullptr;
                    AZ::s64 num = strtoll(text, &parseEnd, 0);
                    if (parseEnd != text)
                    {
                        *outputValue = (num != 0);
                        return JSR::Result(settings, "Successfully read boolean from string with number.",
                            ResultCode::Success(Tasks::ReadField), path);
                    }
                }
                return JSR::Result(settings, "The string didn't contain anything that can be interpreted as a boolean.",
                    Tasks::ReadField, Outcomes::Unsupported, path);
            }
            else
            {
                return JSR::Result(settings, "The string used to read a boolean from was empty.", 
                    Tasks::ReadField, Outcomes::Unsupported, path);
            }
        }
    } // namespace SerializerInternal

    JsonSerializationResult::Result JsonBoolSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<bool>() == outputValueTypeId,
            "Unable to deserialize bool to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        bool* valAsBool = reinterpret_cast<bool*>(outputValue);

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
        // fallthrough
        case rapidjson::kObjectType:
        // fallthrough
        case rapidjson::kNullType:
            return JSR::Result(settings, "Unsupported type. Booleans can't be read from arrays, objects or null.",
                Tasks::ReadField, Outcomes::Unsupported, path);

        case rapidjson::kStringType:
            return SerializerInternal::TextToValue(valAsBool, inputValue.GetString(), inputValue.GetStringLength(),
                path, settings);

        case rapidjson::kFalseType:
        // fallthrough
        case rapidjson::kTrueType:
            *valAsBool = inputValue.GetBool();
            return JSR::Result(settings, "Successfully read boolean.", ResultCode::Success(Tasks::ReadField), path);

        case rapidjson::kNumberType:
            if (inputValue.IsInt64())
            {
                *valAsBool = inputValue.GetInt64() != 0;
            }
            else if (inputValue.IsDouble())
            {
                double value = inputValue.GetDouble();
                *valAsBool = value != 0.0 && value != -0.0;
            }
            else
            {
                *valAsBool = inputValue.GetUint64() != 0;
            }
            return JSR::Result(settings, "Successfully read boolean from number.", ResultCode::Success(Tasks::ReadField), path);
        
        default:
            return JSR::Result(settings, "Unknown json type encountered for boolean.", ResultCode(Tasks::ReadField, Outcomes::Unknown), path);
        }
    }

    JsonSerializationResult::Result JsonBoolSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path,  const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<bool>() == valueTypeId, "Unable to serialize boolean to json because the provided type is %s",
            valueTypeId.ToString<AZ::OSString>().c_str());
        AZ_UNUSED(valueTypeId);

        const bool valAsBool = *reinterpret_cast<const bool*>(inputValue);
        if (settings.m_keepDefaults || !defaultValue || (valAsBool != *reinterpret_cast<const bool*>(defaultValue)))
        {
            outputValue.SetBool(valAsBool);
            return JSR::Result(settings, "Boolean successfully stored", ResultCode::Success(Tasks::WriteValue), path);
        }
        
        return JSR::Result(settings, "Default boolean used.", ResultCode::Default(Tasks::WriteValue), path);
    }
} // namespace AZ
