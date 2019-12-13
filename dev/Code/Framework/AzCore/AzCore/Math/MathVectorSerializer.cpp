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
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/MathVectorSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    namespace JsonMathVectorSerializerInternal
    {
        template<typename VectorType, size_t ElementCount>
        JsonSerializationResult::Result LoadArray(VectorType& output, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(ElementCount >= 2 && ElementCount <= 4, "MathVectorSerializer only support Vector2, Vector3 and Vector4.");
            
            rapidjson::SizeType arraySize = inputValue.Size();
            if (arraySize < ElementCount)
            {
                return JSR::Result(settings, "Not enough numbers in array to load math vector from.",
                    Tasks::ReadField, Outcomes::Unsupported, path);
            }

            AZ::BaseJsonSerializer* floatSerializer = settings.m_registrationContext->GetSerializerForType(azrtti_typeid<float>());
            if (!floatSerializer)
            {
                return JSR::Result(settings, "Failed to find the json float serializer.", Tasks::ReadField, Outcomes::Catastrophic, path);
            }

            constexpr const char* names[4] = { "0", "1", "2", "3" };
            float values[ElementCount];
            for (int i = 0; i < ElementCount; ++i)
            {
                ScopedStackedString subPath(path, names[i]);
                JSR::Result intermediate = floatSerializer->Load(values + i, azrtti_typeid<float>(), inputValue[i], subPath, settings);
                if (intermediate.GetResultCode().GetProcessing() != Processing::Completed)
                {
                    return intermediate;
                }
            }

            for (int i = 0; i < ElementCount; ++i)
            {
                output.SetElement(i, values[i]);
            }

            return JSR::Result(settings, "Successfully read math vector.", ResultCode::Success(Tasks::ReadField), path);
        }

        template<typename VectorType, size_t ElementCount>
        JsonSerializationResult::Result LoadObject(VectorType& output, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(ElementCount >= 2 && ElementCount <= 4, "MathVectorSerializer only support Vector2, Vector3 and Vector4.");

            constexpr const char* names[8] = { "X", "x", "Y", "y", "Z", "z", "W", "w" };

            AZ::BaseJsonSerializer* floatSerializer = settings.m_registrationContext->GetSerializerForType(azrtti_typeid<float>());
            if (!floatSerializer)
            {
                return JSR::Result(settings, "Failed to find the json float serializer.", Tasks::ReadField, Outcomes::Catastrophic, path);
            }

            float values[ElementCount];
            for (int i = 0; i < ElementCount; ++i)
            {
                size_t nameIndex = i * 2;
                auto iterator = inputValue.FindMember(rapidjson::StringRef(names[nameIndex]));
                if (iterator == inputValue.MemberEnd())
                {
                    nameIndex++;
                    iterator = inputValue.FindMember(rapidjson::StringRef(names[nameIndex]));
                    if (iterator == inputValue.MemberEnd())
                    {
                        // Technically the object could be initialized from a partial declaration and set the missing field to 0.0, but
                        // this would create inconsistencies with the array version which wouldn't be able to offer this option.
                        return JSR::Result(settings, OSString::format("Unable to find field for '%s'.", names[i * 2]),
                            Tasks::ReadField, Outcomes::Unsupported, path);
                    }
                }

                ScopedStackedString subPath(path, names[nameIndex]);
                JSR::Result intermediate = floatSerializer->Load(values + i, azrtti_typeid<float>(), iterator->value, subPath, settings);
                if (intermediate.GetResultCode().GetProcessing() != Processing::Completed)
                {
                    return intermediate;
                }
            }

            for (int i = 0; i < ElementCount; ++i)
            {
                output.SetElement(i, values[i]);
            }

            return JSR::Result(settings, "Successfully read math vector.", ResultCode::Success(Tasks::ReadField), path);
        }

        template<typename VectorType, size_t ElementCount>
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(ElementCount >= 2 && ElementCount <= 4, "MathVectorSerializer only support Vector2, Vector3 and Vector4.");
            
            AZ_Assert(azrtti_typeid<VectorType>() == outputValueTypeId,
                "Unable to deserialize Vector%zu to json because the provided type is %s",
                ElementCount, outputValueTypeId.ToString<OSString>().c_str());
            AZ_UNUSED(outputValueTypeId);
            
            VectorType* vector = reinterpret_cast<VectorType*>(outputValue);
            AZ_Assert(vector, "Output value for JsonVector%iSerializer can't be null.", ElementCount);

            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
                return LoadArray<VectorType, ElementCount>(*vector, inputValue, path, settings);
            case rapidjson::kObjectType:
                return LoadObject<VectorType, ElementCount>(*vector, inputValue, path, settings);

            case rapidjson::kStringType: // fall through
            case rapidjson::kNumberType: // fall through
            case rapidjson::kNullType:   // fall through
            case rapidjson::kFalseType:  // fall through
            case rapidjson::kTrueType:
                return JSR::Result(settings, "Unsupported type. Math vectors can only be read from arrays or objects.",
                    Tasks::ReadField, Outcomes::Unsupported, path);

            default:
                return JSR::Result(settings, "Unknown json type encountered in math vector.",
                    Tasks::ReadField, Outcomes::Unknown, path);
            }
        }
        
        template<typename VectorType, size_t ElementCount>
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
            const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, 
            StackedString& path, const JsonSerializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(ElementCount >= 2 && ElementCount <= 4, "MathVectorSerializer only support Vector2, Vector3 and Vector4.");
            AZ_Assert(azrtti_typeid<VectorType>() == valueTypeId, "Unable to serialize Vector%i to json because the provided type is %s",
                ElementCount, valueTypeId.ToString<OSString>().c_str());
            AZ_UNUSED(valueTypeId);

            const VectorType* vector = reinterpret_cast<const VectorType*>(inputValue);
            AZ_Assert(vector, "Input value for JsonVector%zuSerializer can't be null.", ElementCount);
            const VectorType* defaultVector = reinterpret_cast<const VectorType*>(defaultValue);

            if (!settings.m_keepDefaults && defaultVector && *vector == *defaultVector)
            {
                return JSR::Result(settings, "Default math Vector used.", ResultCode::Default(Tasks::WriteValue), path);
            }

            outputValue.SetArray();
            for (int i = 0; i < ElementCount; ++i)
            {
                outputValue.PushBack(vector->GetElement(i), allocator);
            }

            return JSR::Result(settings, "Math Vector successfully stored.", ResultCode::Success(Tasks::WriteValue), path);
        }
    }

    
    // Vector2

    AZ_CLASS_ALLOCATOR_IMPL(JsonVector2Serializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonVector2Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        return JsonMathVectorSerializerInternal::Load<Vector2, 2>(outputValue, outputValueTypeId,
            inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonVector2Serializer::Store(rapidjson::Value& outputValue,
        rapidjson::Document::AllocatorType& allocator, const void* inputValue, const void* defaultValue, 
        const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        return JsonMathVectorSerializerInternal::Store<Vector2, 2>(outputValue, allocator, inputValue, 
            defaultValue, valueTypeId, path, settings);
    }


    // Vector3

    AZ_CLASS_ALLOCATOR_IMPL(JsonVector3Serializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonVector3Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        return JsonMathVectorSerializerInternal::Load<Vector3, 3>(outputValue, outputValueTypeId,
            inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonVector3Serializer::Store(rapidjson::Value& outputValue,
        rapidjson::Document::AllocatorType& allocator, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        return JsonMathVectorSerializerInternal::Store<Vector3, 3>(outputValue, allocator, inputValue, 
            defaultValue, valueTypeId, path, settings);
    }


    // Vector4

    AZ_CLASS_ALLOCATOR_IMPL(JsonVector4Serializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonVector4Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        return JsonMathVectorSerializerInternal::Load<Vector4, 4>(outputValue, outputValueTypeId,
            inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonVector4Serializer::Store(rapidjson::Value& outputValue,
        rapidjson::Document::AllocatorType& allocator, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        return JsonMathVectorSerializerInternal::Store<Vector4, 4>(outputValue, allocator, inputValue, 
            defaultValue, valueTypeId, path, settings);
    }
}