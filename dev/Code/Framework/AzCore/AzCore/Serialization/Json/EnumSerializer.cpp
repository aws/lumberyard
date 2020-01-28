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

#include <AzCore/Serialization/Json/EnumSerializer.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonStringConversionUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/set.h>

namespace AZ
{
    inline namespace EnumSerializerInternal
    {
        template<typename UnderlyingType>
        struct SerializeEnumValue
        {
            UnderlyingType& m_value{};
            const SerializeContext::ClassData* m_enumClassData{};
        };

        template<typename UnderlyingType>
        JsonSerializationResult::Result LoadInt(EnumSerializerInternal::SerializeEnumValue<UnderlyingType>& outputValue, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            JsonSerializationResult::ResultCode result = JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported);
            if (inputValue.IsInt64())
            {
                result = JsonNumericCast(outputValue.m_value, inputValue.GetInt64(), path, settings.m_reporting);
            }
            else if (inputValue.IsUint64())
            {
                result = JsonNumericCast(outputValue.m_value, inputValue.GetUint64(), path, settings.m_reporting);
            }

            return JsonSerializationResult::Result(settings, result.GetOutcome() == JsonSerializationResult::Outcomes::Success ?
                "Successfully read enum value from number field." : "Failed to read enum value from number field.", result, path);
        }

        template<typename UnderlyingType>
        JsonSerializationResult::Result LoadString(EnumSerializerInternal::SerializeEnumValue<UnderlyingType>& outputValue, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            const SerializeContext::ClassData* enumClassData{ outputValue.m_enumClassData };
            AZStd::string_view enumString(inputValue.GetString(), inputValue.GetStringLength());
            if (enumString.empty())
            {
                return JsonSerializationResult::Result(settings, "Input string is empty, no enumeration can be parsed from it.",
                    JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::DefaultsUsed,
                    path);
            }

            JsonSerializationResult::Result textToValueResult = SerializerInternal::TextToValue(&outputValue.m_value, enumString.data(), path, settings);
            if (textToValueResult.GetResultCode().GetOutcome() == JsonSerializationResult::Outcomes::Success)
            {
                return textToValueResult;
            }

            for (const AZ::AttributeSharedPair& enumAttributePair : enumClassData->m_attributes)
            {
                if (enumAttributePair.first != AZ::Serialize::Attributes::EnumValueKey)
                {
                    continue;
                }

                using EnumConstantPtr = AZStd::unique_ptr<SerializeContextEnumInternal::EnumConstantBase>;
                auto enumConstantAttribute = azrtti_cast<AZ::AttributeData<EnumConstantPtr>*>(enumAttributePair.second.get());
                if (!enumConstantAttribute)
                {
                    JsonSerializationResult::ResultCode enumConstantResult(JsonSerializationResult::Tasks::RetrieveInfo, JsonSerializationResult::Outcomes::Unknown);
                    settings.m_reporting("EnumConstant element was unable to be read from attribute ID 'EnumValue'", enumConstantResult, path);
                    continue;
                }

                const EnumConstantPtr& enumConstantPtr = enumConstantAttribute->Get(nullptr);
                if (enumString.size() == enumConstantPtr->GetEnumValueName().size()
                    && azstrnicmp(enumString.data(),enumConstantPtr->GetEnumValueName().data(), enumString.size()) == 0)
                {
                    UnderlyingType enumConstantAsInt = aznumeric_cast<UnderlyingType>(AZStd::is_signed<UnderlyingType>::value ? enumConstantPtr->GetEnumValueAsInt() : enumConstantPtr->GetEnumValueAsUInt());
                    outputValue.m_value = enumConstantAsInt;
                    return JsonSerializationResult::Result(settings, "Successfully loaded enum value from string",
                        JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Success, path);
                }
            }

            return JsonSerializationResult::Result(settings, "Unsupported value for string. Enum value could not read.",
                JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported, path);
        }

        template<typename UnderlyingType>
        JsonSerializationResult::Result LoadContainer(EnumSerializerInternal::SerializeEnumValue<UnderlyingType>& outputValue, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            const rapidjson::SizeType arraySize = inputValue.Size();
            if (arraySize == 0)
            {
                return JsonSerializationResult::Result(settings, "Input array is empty, no enumeration can be parsed from it.",
                    JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::DefaultsUsed, path);
            }

            // Zero the underlying type output as any enum values will be bitwise-or'ed into the output
            outputValue.m_value = {};

            JsonSerializationResult::ResultCode result = JsonSerializationResult::ResultCode(
                JsonSerializationResult::Tasks::ReadField,
                JsonSerializationResult::Outcomes::Success);

            for (rapidjson::SizeType i = 0; i < arraySize; ++i)
            {
                UnderlyingType enumConstant{};
                EnumSerializerInternal::SerializeEnumValue<UnderlyingType> enumConstantElement{ enumConstant, outputValue.m_enumClassData };
                
                ScopedStackedString enumArraySubPath(path, i);
                JsonSerializationResult::ResultCode retVal = JsonSerializationResult::ResultCode::Success(JsonSerializationResult::Tasks::ReadField);
                const rapidjson::Value& jsonInputValueElement = inputValue[i];
                switch (jsonInputValueElement.GetType())
                {
                case rapidjson::kStringType:
                    retVal = LoadString(enumConstantElement, jsonInputValueElement, enumArraySubPath, settings);
                    result.Combine(retVal);
                    break;
                case rapidjson::kNumberType:
                    retVal = LoadInt(enumConstantElement, jsonInputValueElement, enumArraySubPath, settings);
                    result.Combine(retVal);
                    break;
                case rapidjson::kArrayType:
                case rapidjson::kObjectType:
                case rapidjson::kNullType:
                case rapidjson::kFalseType:
                case rapidjson::kTrueType:
                    result = JsonSerializationResult::Result(settings, "Unsupported type. Enum values within an array must either be a string or a number.",
                        JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported, enumArraySubPath);
                    break;

                default:
                    result = JsonSerializationResult::Result(settings, "Unknown json type encountered for deserialization from a enumeration.",
                        JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unknown, enumArraySubPath);
                }

                if (result.GetProcessing() == JsonSerializationResult::Processing::Halted)
                {
                    break;
                }

                //! bitwise-or the enumConstant value to the outputValue parameter.
                //! enumConstant is zero-initialized using {}, so for non-string/non-int path this operations is a no-op
                outputValue.m_value |= enumConstant;
            }

            AZStd::string_view message(result.GetProcessing() != JsonSerializationResult::Processing::Halted ?
                "Enum value has been parsed from array" :
                "Enum value could not be parsed from array"
            );
            return JsonSerializationResult::Result(settings, message, result, path);
        }

        template<typename UnderlyingType>
        static JsonSerializationResult::Result LoadUnderlyingType(EnumSerializerInternal::SerializeEnumValue<UnderlyingType>& outputValue,
            const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
        {
            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
                return EnumSerializerInternal::LoadContainer(outputValue, inputValue, path, settings);
            case rapidjson::kStringType:
                return EnumSerializerInternal::LoadString(outputValue, inputValue, path, settings);
            case rapidjson::kNumberType:
                return EnumSerializerInternal::LoadInt(outputValue, inputValue, path, settings);
            default:
                return JsonSerializationResult::Result(settings, "Unknown json type encountered for deserialization. This function should only be called from Load() which guarantees this never occurs.",
                    JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unknown, path);
            }
        }

        template<typename UnderlyingType>
        static JsonSerializationResult::Result StoreUnderlyingType(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
            const SerializeEnumValue<const UnderlyingType>& inputValue, const UnderlyingType* defaultValue, StackedString& path, const JsonSerializerSettings& settings)
        {
            /*
             * The implementation strategy for storing an enumeration with Json is to first look for an exact value match for the enumeration
             * using the values registered within the SerializeContext
             * If that is found, the enum option as a Json string
             * If an exact value cannot be found, then the each value registered with the SerializeContext are assumed to be bitflags are checked
             * within the inputValue to determine if all the bits for the value are set.
             * For each registered enum value that is found, its bit value is cleared from enum and processing continues.
             * Afterwards the enum option is added as a string to a Json array
             * Finally if there is any non-zero value that is left after processing each registered enum option,
             * it gets added as an integral value to a Json array
             */

            UnderlyingType enumInputValue{ inputValue.m_value };
            const SerializeContext::ClassData* enumClassData{ inputValue.m_enumClassData };

            using EnumConstantBase = SerializeContextEnumInternal::EnumConstantBase;

            using EnumConstantBasePtr = AZStd::unique_ptr<SerializeContextEnumInternal::EnumConstantBase>;
            //! Comparison function which treats each enum value as unsigned for parsing purposes.
            //! The set below is for parsing each enum value as a set of bit flags, where it is configured
            //! to parse values with higher unsigned enum values first
            struct EnumUnsignedCompare
            {
                bool operator()(const AZStd::reference_wrapper<EnumConstantBase>& lhs, const AZStd::reference_wrapper<EnumConstantBase>& rhs)
                {
                    return static_cast<EnumConstantBase&>(lhs).GetEnumValueAsUInt() > static_cast<EnumConstantBase&>(rhs).GetEnumValueAsUInt();
                }
            };

            AZStd::set<AZStd::reference_wrapper<EnumConstantBase>, EnumUnsignedCompare> enumConstantSet;
            for (const AZ::AttributeSharedPair& enumAttributePair : enumClassData->m_attributes)
            {
                if (enumAttributePair.first == AZ::Serialize::Attributes::EnumValueKey)
                {
                    auto enumConstantAttribute = azrtti_cast<AZ::AttributeData<EnumConstantBasePtr>*>(enumAttributePair.second.get());
                    if (!enumConstantAttribute)
                    {
                        JsonSerializationResult::ResultCode enumConstantResult(JsonSerializationResult::Tasks::RetrieveInfo, JsonSerializationResult::Outcomes::Unknown);
                        settings.m_reporting("EnumConstant element was unable to be read from attribute ID 'EnumValue'", enumConstantResult, path);
                        continue;
                    }

                    const EnumConstantBasePtr& enumConstantPtr = enumConstantAttribute->Get(nullptr);
                    UnderlyingType enumConstantAsInt = static_cast<UnderlyingType>(AZStd::is_signed<UnderlyingType>::value ? enumConstantPtr->GetEnumValueAsInt() : enumConstantPtr->GetEnumValueAsUInt());

                    if (enumInputValue == enumConstantAsInt)
                    {
                        if (settings.m_keepDefaults || !defaultValue || enumInputValue != *defaultValue)
                        {
                            outputValue.SetString(enumConstantPtr->GetEnumValueName().data(), aznumeric_cast<rapidjson::SizeType>(enumConstantPtr->GetEnumValueName().size()), allocator);
                            return JsonSerializationResult::Result(settings, "Successfully stored Enum value as string",
                                JsonSerializationResult::ResultCode::Success(JsonSerializationResult::Tasks::WriteValue), path);
                        }
                        else
                        {
                            return JsonSerializationResult::Result(settings, "Enum value defaulted as string",
                                JsonSerializationResult::ResultCode::Default(JsonSerializationResult::Tasks::WriteValue), path);
                        }
                    }

                    //! A registered enum with a value of 0 that doesn't match the exact enum value is not output as part of the json array
                    if (enumConstantAsInt == 0)
                    {
                        continue;
                    }

                    //! Add registered enum value where all of bits set within the input enum value
                    if ((enumConstantAsInt & enumInputValue) == enumConstantAsInt)
                    {
                        enumConstantSet.emplace(*enumConstantPtr);
                    }
                }
            }

            if (!settings.m_keepDefaults && defaultValue && enumInputValue == *(defaultValue))
            {
                return JsonSerializationResult::Result(settings, enumConstantSet.empty() ? "Enum value defaulted as int" : "Enum value defaulted as array",
                    JsonSerializationResult::ResultCode::Default(JsonSerializationResult::Tasks::WriteValue), path);

            }

            // Special case: If none of the enum values share bits with the current input value, write out the value as an integral
            if (enumConstantSet.empty())
            {
                outputValue = enumInputValue;
                return JsonSerializationResult::Result(settings, "Successfully stored Enum value as int",
                    JsonSerializationResult::ResultCode::Success(JsonSerializationResult::Tasks::WriteValue), path);
            }
            else
            {
                // Mask of each enum constant bits from the input value and add that value to the JsonArray
                outputValue.SetArray();
                UnderlyingType enumInputBitset = enumInputValue;
                for (const EnumConstantBase& enumConstant : enumConstantSet)
                {
                    enumInputBitset = enumInputBitset & ~enumConstant.GetEnumValueAsUInt();
                    rapidjson::Value enumConstantNameValue(enumConstant.GetEnumValueName().data(), aznumeric_cast<rapidjson::SizeType>(enumConstant.GetEnumValueName().size()), allocator);
                    outputValue.PushBack(AZStd::move(enumConstantNameValue), allocator);
                }

                //! If after all the enum values whose bitflags were mask'ed out of the input enum
                //! the input enum value is still non-zero, that means that the remaining value is represented 
                //! is a raw integral of the left over flags
                //! Therefore it gets written to the json array has an integral value
                if (enumInputBitset != 0)
                {
                    outputValue.PushBack(rapidjson::Value(enumInputBitset), allocator);
                }
            }

            return JsonSerializationResult::Result(settings, "Enum value defaulted as array",
                JsonSerializationResult::ResultCode::Success(JsonSerializationResult::Tasks::WriteValue), path);
        }
    }

    JsonSerializationResult::Result JsonEnumSerializer::Load(void* outputValue, const TypeId& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        if (!outputValue)
        {
            return JsonSerializationResult::Result(settings, "Output for enum is instance is nullptr. Json data cannot be loaded into it",
                JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Catastrophic, path);
        }

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType: // supported types
        case rapidjson::kStringType:
        case rapidjson::kNumberType:
            break;
        case rapidjson::kObjectType: // unsupported types
        case rapidjson::kNullType:
        case rapidjson::kFalseType:
        case rapidjson::kTrueType:
            return JsonSerializationResult::Result(settings, "Unsupported type. Enums must be read from an array, string or number",
                JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported, path);

        default:
            return JsonSerializationResult::Result(settings, "Unknown json type encountered for deserialization from a enumeration.",
                JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unknown, path);
        }

        const SerializeContext::ClassData* enumClassData = settings.m_serializeContext->FindClassData(outputValueTypeId);
        if (!enumClassData)
        {
            return JsonSerializationResult::Result(settings, "Unable to retrieve information for definition of the enum.",
                JsonSerializationResult::Tasks::RetrieveInfo, JsonSerializationResult::Outcomes::Unsupported, path);
        }

        AZ::AttributeReader attributeReader(nullptr, AZ::FindAttribute(AZ::Serialize::Attributes::EnumUnderlyingType, enumClassData->m_attributes));
        AZ::TypeId underlyingTypeId = AZ::TypeId::CreateNull();
        if (!attributeReader.Read<AZ::TypeId>(underlyingTypeId))
        {
            return JsonSerializationResult::Result(settings, "Unable to find underlying type of enum in class data.",
                JsonSerializationResult::Tasks::RetrieveInfo, JsonSerializationResult::Outcomes::Unsupported, path);
        }

        const SerializeContext::ClassData* underlyingClassData = settings.m_serializeContext->FindClassData(underlyingTypeId);
        if (!underlyingClassData)
        {
            return JsonSerializationResult::Result(settings, "Unable to retrieve information for underlying integral type of enum.",
                JsonSerializationResult::Tasks::RetrieveInfo, JsonSerializationResult::Outcomes::Unsupported, path);
        }

        IRttiHelper* underlyingTypeRtti = underlyingClassData->m_azRtti;
        if (!underlyingTypeRtti)
        {
            return JsonSerializationResult::Result(settings, AZ::OSString::format("Rtti for underlying type %s, of enum is not registered. Enum Value can not be stored", underlyingClassData->m_name),
                JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Convert, JsonSerializationResult::Outcomes::Unknown),
                path);
        }

        const size_t underlyingTypeSize = underlyingTypeRtti->GetTypeSize();
        const TypeTraits underlyingTypeTraits = underlyingTypeRtti->GetTypeTraits();
        const bool isSigned = (underlyingTypeTraits & TypeTraits::is_signed) == TypeTraits::is_signed;
        const bool isUnsigned = (underlyingTypeTraits & TypeTraits::is_unsigned) == TypeTraits::is_unsigned;
        if (isSigned)
        {
            // Cast to either an int16_t, int32_t, int64_t depending on the size of the underlying type
            // int8_t is not supported.
            switch (underlyingTypeSize)
            {
            case 2:
            {
                EnumSerializerInternal::SerializeEnumValue<int16_t> serializeValue{ *reinterpret_cast<int16_t*>(outputValue), enumClassData };
                return EnumSerializerInternal::LoadUnderlyingType(serializeValue, inputValue, path, settings);
            }
            case 4:
            {
                EnumSerializerInternal::SerializeEnumValue<int32_t> serializeValue{ *reinterpret_cast<int32_t*>(outputValue), enumClassData };
                return EnumSerializerInternal::LoadUnderlyingType(serializeValue, inputValue, path, settings);
            }
            case 8:
            {
                EnumSerializerInternal::SerializeEnumValue<int64_t> serializeValue{ *reinterpret_cast<int64_t*>(outputValue), enumClassData };
                return EnumSerializerInternal::LoadUnderlyingType(serializeValue, inputValue, path, settings);
            }
            default:
                return JsonSerializationResult::Result(settings, AZ::OSString::format("Signed Underlying type with size=%zu is not supported by Enum Serializer", underlyingTypeSize),
                    JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Convert, JsonSerializationResult::Outcomes::Unknown),
                    path);
            }
        }
        else if (isUnsigned)
        {
            // Cast to either an uint8_t, uint16_t, uint32_t, uint64_t depending on the size of the underlying type
            switch (underlyingTypeSize)
            {
            case 1:
            {
                EnumSerializerInternal::SerializeEnumValue<uint8_t> serializeValue{ *reinterpret_cast<uint8_t*>(outputValue), enumClassData };
                return EnumSerializerInternal::LoadUnderlyingType(serializeValue, inputValue, path, settings);
            }
            case 2:
            {
                EnumSerializerInternal::SerializeEnumValue<uint16_t> serializeValue{ *reinterpret_cast<uint16_t*>(outputValue), enumClassData };
                return EnumSerializerInternal::LoadUnderlyingType(serializeValue, inputValue, path, settings);
            }
            case 4:
            {
                EnumSerializerInternal::SerializeEnumValue<uint32_t> serializeValue{ *reinterpret_cast<uint32_t*>(outputValue), enumClassData };
                return EnumSerializerInternal::LoadUnderlyingType(serializeValue, inputValue, path, settings);
            }
            case 8:
            {
                EnumSerializerInternal::SerializeEnumValue<uint64_t> serializeValue{ *reinterpret_cast<uint64_t*>(outputValue), enumClassData };
                return EnumSerializerInternal::LoadUnderlyingType(serializeValue, inputValue, path, settings);
            }
            default:
                return JsonSerializationResult::Result(settings, AZ::OSString::format("Unsigned Underlying type with size=%zu is not supported by Enum Serializer", underlyingTypeSize),
                    JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Convert, JsonSerializationResult::Outcomes::Unknown),
                    path);
            }
        }

        return JsonSerializationResult::Result(settings, AZ::OSString::format("Unsupported underlying type %s for enum %s. Cannot load into enum output.",
            underlyingClassData->m_name, enumClassData->m_name),
            JsonSerializationResult::Tasks::WriteValue, JsonSerializationResult::Outcomes::Catastrophic,
            path);
    }

    JsonSerializationResult::Result JsonEnumSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
        const void* inputValue, const void* defaultValue, const TypeId& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        if (!inputValue)
        {
            return JsonSerializationResult::Result(settings, "Input for enum is instance is nullptr. It cannot be stored",
                JsonSerializationResult::Tasks::WriteValue, JsonSerializationResult::Outcomes::Catastrophic, path);
        }

        const SerializeContext::ClassData* enumClassData = settings.m_serializeContext->FindClassData(valueTypeId);
        if (!enumClassData)
        {
            return JsonSerializationResult::Result(settings, "Unable to retrieve information for definition of the enum.",
                JsonSerializationResult::Tasks::RetrieveInfo, JsonSerializationResult::Outcomes::Unsupported, path);
        }

        AZ::AttributeReader attributeReader(nullptr, AZ::FindAttribute(AZ::Serialize::Attributes::EnumUnderlyingType, enumClassData->m_attributes));
        AZ::TypeId underlyingTypeId = AZ::TypeId::CreateNull();
        if (!attributeReader.Read<AZ::TypeId>(underlyingTypeId))
        {
            return JsonSerializationResult::Result(settings, "Unable to find underlying type of enum in class data.",
                JsonSerializationResult::Tasks::RetrieveInfo, JsonSerializationResult::Outcomes::Unsupported, path);
        }

        const SerializeContext::ClassData* underlyingClassData = settings.m_serializeContext->FindClassData(underlyingTypeId);
        if (!underlyingClassData)
        {
            return JsonSerializationResult::Result(settings, "Unable to retrieve information for underlying integral type of enum.",
                JsonSerializationResult::Tasks::RetrieveInfo, JsonSerializationResult::Outcomes::Unsupported, path);
        }

        IRttiHelper* underlyingTypeRtti = underlyingClassData->m_azRtti;
        if (!underlyingTypeRtti)
        {
            return JsonSerializationResult::Result(settings, AZ::OSString::format("Rtti for underlying type %s, of enum is not registered. Enum Value can not be stored", underlyingClassData->m_name),
                JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Convert, JsonSerializationResult::Outcomes::Unknown),
                path);
        }

        const size_t underlyingTypeSize = underlyingTypeRtti->GetTypeSize();
        const TypeTraits underlyingTypeTraits = underlyingTypeRtti->GetTypeTraits();
        const bool isSigned = (underlyingTypeTraits & TypeTraits::is_signed) == TypeTraits::is_signed;
        const bool isUnsigned = (underlyingTypeTraits & TypeTraits::is_unsigned) == TypeTraits::is_unsigned;
        if (isSigned)
        {
            // Cast to either an int16_t, int32_t, int64_t depending on the size of the underlying type
            // int8_t is not supported.
            switch (underlyingTypeSize)
            {
            case 2:
            {
                const EnumSerializerInternal::SerializeEnumValue<const int16_t> serializeValue{ *reinterpret_cast<const int16_t*>(inputValue), enumClassData };
                return EnumSerializerInternal::StoreUnderlyingType(outputValue, allocator, serializeValue, reinterpret_cast<const int16_t*>(defaultValue), path, settings);
            }
            case 4:
            {
                const EnumSerializerInternal::SerializeEnumValue<const int32_t> serializeValue{ *reinterpret_cast<const int32_t*>(inputValue), enumClassData };
                return EnumSerializerInternal::StoreUnderlyingType(outputValue, allocator, serializeValue, reinterpret_cast<const int32_t*>(defaultValue), path, settings);
            }
            case 8:
            {
                const EnumSerializerInternal::SerializeEnumValue<const int64_t> serializeValue{ *reinterpret_cast<const int64_t*>(inputValue), enumClassData };
                return EnumSerializerInternal::StoreUnderlyingType(outputValue, allocator, serializeValue, reinterpret_cast<const int64_t*>(defaultValue), path, settings);
            }
            default:
                return JsonSerializationResult::Result(settings, AZ::OSString::format("Signed Underlying type with size=%zu is not supported by Enum Serializer", underlyingTypeSize),
                    JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Convert, JsonSerializationResult::Outcomes::Unknown),
                    path);
            }
        }
        else if (isUnsigned)
        {
            // Cast to either an uint8_t, uint16_t, uint32_t, uint64_t depending on the size of the underlying type
            switch (underlyingTypeSize)
            {
            case 1:
            {
                const EnumSerializerInternal::SerializeEnumValue<const uint8_t> serializeValue{ *reinterpret_cast<const uint8_t*>(inputValue), enumClassData };
                return EnumSerializerInternal::StoreUnderlyingType(outputValue, allocator, serializeValue, reinterpret_cast<const uint8_t*>(defaultValue), path, settings);
            }
            case 2:
            {
                const EnumSerializerInternal::SerializeEnumValue<const uint16_t> serializeValue{ *reinterpret_cast<const uint16_t*>(inputValue), enumClassData };
                return EnumSerializerInternal::StoreUnderlyingType(outputValue, allocator, serializeValue, reinterpret_cast<const uint16_t*>(defaultValue), path, settings);
            }
            case 4:
            {
                const EnumSerializerInternal::SerializeEnumValue<const uint32_t> serializeValue{ *reinterpret_cast<const uint32_t*>(inputValue), enumClassData };
                return EnumSerializerInternal::StoreUnderlyingType(outputValue, allocator, serializeValue, reinterpret_cast<const uint32_t*>(defaultValue), path, settings);
            }
            case 8:
            {
                const EnumSerializerInternal::SerializeEnumValue<const uint64_t> serializeValue{ *reinterpret_cast<const uint64_t*>(inputValue), enumClassData };
                return EnumSerializerInternal::StoreUnderlyingType(outputValue, allocator, serializeValue, reinterpret_cast<const uint64_t*>(defaultValue), path, settings);
            }
            default:
                return JsonSerializationResult::Result(settings, AZ::OSString::format("Unsigned Underlying type with size=%zu is not supported by Enum Serializer", underlyingTypeSize),
                    JsonSerializationResult::ResultCode(JsonSerializationResult::Tasks::Convert, JsonSerializationResult::Outcomes::Unknown),
                    path);
            }
        }

        return JsonSerializationResult::Result(settings, AZ::OSString::format("Unsupported underlying type %s for enum %s. Enum value can not be stored.",
            underlyingClassData->m_name, enumClassData->m_name),
            JsonSerializationResult::Tasks::WriteValue, JsonSerializationResult::Outcomes::Catastrophic,
            path);
        }
} // namespace AZ
