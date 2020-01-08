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

#include <AzCore/Serialization/Json/DoubleSerializer.h>

#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/typetraits/is_floating_point.h>
#include <AzCore/Math/MathUtils.h>
#include <limits>
#include <cerrno>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonDoubleSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonFloatSerializer, SystemAllocator, 0);

    namespace SerializerFloatingPointInternal
    {
        template <typename T>
        static JsonSerializationResult::Result TextToValue(T* outputValue, const char* text,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(AZStd::is_floating_point<T>(), "Expected T to be a floating point type");

            char* parseEnd = nullptr;

            errno = 0;
            double parsedDouble = strtod(text, &parseEnd);
            if (errno == ERANGE)
            {
                return JSR::Result(settings, "Floating point value it too big or too small to fit in the target.",
                    Tasks::ReadField, Outcomes::Unsupported, path);
            }

            if (parseEnd != text)
            {
                ResultCode result = JsonNumericCast<T>(*outputValue, parsedDouble, path, settings.m_reporting);
                AZStd::string_view message = result.GetOutcome() == Outcomes::Success ?
                    "Successfully read floating point number from string." : "Failed to read floating point number from string.";
                return JSR::Result(settings, message, result, path);
            }
            else
            {
                AZStd::string_view message = (text && text[0]) ?
                    "Unable to read floating point value from provided string." : "String used to read floating point value from was empty.";
                return JSR::Result(settings, message, Tasks::ReadField, Outcomes::Unsupported, path);
            }
        }

        template <typename T>
        static JsonSerializationResult::Result Load(T* outputValue, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(AZStd::is_floating_point<T>::value, "Expected T to be a floating point type");
            AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
            // fallthrough
            case rapidjson::kObjectType:
            // fallthrough
            case rapidjson::kNullType:
                return JSR::Result(settings, "Unsupported type. Floating point values can't be read from arrays, objects or null.",
                    Tasks::ReadField, Outcomes::Unsupported, path);

            case rapidjson::kStringType:
                return TextToValue(outputValue, inputValue.GetString(), path, settings);

            case rapidjson::kFalseType:
            // fallthrough
            case rapidjson::kTrueType:
                *outputValue = inputValue.GetBool() ? 1.0f : 0.0f;
                return JSR::Result(settings, "Successfully converted boolean to floating point value.",
                    ResultCode::Success(Tasks::ReadField), path);

            case rapidjson::kNumberType:
            {
                ResultCode result(Tasks::ReadField);
                if (inputValue.IsDouble())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetDouble(), path, settings.m_reporting);
                }
                else if (inputValue.IsInt64())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetInt64(), path, settings.m_reporting);
                }
                else
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetUint64(), path, settings.m_reporting);
                }

                return JSR::Result(settings, result.GetOutcome() == Outcomes::Success ?
                    "Successfully read floating point value from number field." : "Failed to read floating point value from number field.", result, path);
            }

            default:
                return JSR::Result(settings, "Unknown json type encountered for floating point value.",
                    Tasks::ReadField, Outcomes::Unknown, path);
            }
        }

        template <typename T>
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            StackedString& path, const JsonSerializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            const T inputValAsType = *reinterpret_cast<const T*>(inputValue);
            if (settings.m_keepDefaults || !defaultValue || 
                !AZ::IsClose(inputValAsType, *reinterpret_cast<const T*>(defaultValue), std::numeric_limits<T>::epsilon()))
            {
                outputValue.SetDouble(inputValAsType);
                return JSR::Result(settings, "Floating point number successfully stored.", ResultCode::Success(Tasks::WriteValue), path);
            }

            return JSR::Result(settings, "Default floating point number used.", ResultCode::Default(Tasks::WriteValue), path);
        }
    } // namespace SerializerFloatingPointInternal

    JsonSerializationResult::Result JsonDoubleSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<double>() == outputValueTypeId,
            "Unable to deserialize double to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerFloatingPointInternal::Load(reinterpret_cast<double*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonDoubleSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, 
        StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<double>() == valueTypeId, "Unable to serialize double to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerFloatingPointInternal::Store<double>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonFloatSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<float>() == outputValueTypeId,
            "Unable to deserialize float to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerFloatingPointInternal::Load(reinterpret_cast<float*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonFloatSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId,
        StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<float>() == valueTypeId, "Unable to serialize float to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerFloatingPointInternal::Store<float>(outputValue, inputValue, defaultValue, path, settings);
    }
} // namespace AZ
