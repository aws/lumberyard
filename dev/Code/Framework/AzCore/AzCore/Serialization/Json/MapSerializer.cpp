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

#include <algorithm>
#include <AzCore/Serialization/Json/BasicContainerSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/MapSerializer.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonMapSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnorderedMapSerializer, SystemAllocator, 0);
    
    // JsonMapSerializer

    JsonSerializationResult::Result JsonMapSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType: // fall through
        case rapidjson::kObjectType: 
            return LoadContainer(outputValue, outputValueTypeId, inputValue, path, settings);

        case rapidjson::kNullType: // fall through
        case rapidjson::kStringType: // fall through
        case rapidjson::kFalseType: // fall through
        case rapidjson::kTrueType: // fall through
        case rapidjson::kNumberType:
            return JSR::Result(settings, "Unsupported type. Associative containers can only be read from an array or object.",
                JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, path);

        default:
            return JSR::Result(settings, "Unknown json type encountered for deserialization from an associative container.",
                JSR::Tasks::ReadField, JSR::Outcomes::Unknown, path);
        }
    }

    JsonSerializationResult::Result JsonMapSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        return Store(outputValue, allocator, inputValue, defaultValue, valueTypeId, path, settings, false);
    }

    JsonSerializationResult::Result JsonMapSerializer::LoadContainer(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        const SerializeContext::ClassData* containerClass = settings.m_serializeContext->FindClassData(outputValueTypeId);
        if (!containerClass)
        {
            return JSR::Result(settings, "Unable to retrieve information for definition of the associative container instance.",
                JSR::Tasks::RetrieveInfo, JSR::Outcomes::Unsupported, path);
        }

        SerializeContext::IDataContainer* container = containerClass->m_container;
        AZ_Assert(container, "Unable to retrieve information for representation of the associative container instance.");
        
        const SerializeContext::ClassElement* pairElement = nullptr;
        auto pairTypeEnumCallback = [&pairElement](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            AZ_Assert(!pairElement, "A map is expected to only have one element.");
            pairElement = genericClassElement;
            return true;
        };
        container->EnumTypes(pairTypeEnumCallback);
        AZ_Assert(pairElement, "A map is expected to have exactly one pair element.");

        const SerializeContext::ClassData* pairClass = settings.m_serializeContext->FindClassData(pairElement->m_typeId);
        AZ_Assert(pairClass, "Associative container was registered but not the pair that's used for storage.");
        SerializeContext::IDataContainer* pairContainer = pairClass->m_container;
        AZ_Assert(pairClass, "Associative container is missing the interface to the storage container.");
        const SerializeContext::ClassElement* keyElement = nullptr;
        const SerializeContext::ClassElement* valueElement = nullptr;
        auto keyValueTypeEnumCallback = [&keyElement, &valueElement](const Uuid&, const SerializeContext::ClassElement* genericClassElement)
        {
            if (keyElement)
            {
                AZ_Assert(!valueElement, "The pair element in a container can't have more than 2 elements.");
                valueElement = genericClassElement;
            }
            else
            {
                keyElement = genericClassElement;
            }
            return true;
        };
        pairContainer->EnumTypes(keyValueTypeEnumCallback);
        AZ_Assert(keyElement && valueElement, "Expected the pair element in a container to have exactly 2 elements.");

        size_t containerSize = container->Size(outputValue);
        rapidjson::SizeType maximumSize = 0;
        const rapidjson::Value defaultValue(rapidjson::kObjectType);
        JSR::ResultCode retVal(JSR::Tasks::ReadField);
        if (inputValue.IsObject())
        {
            maximumSize = inputValue.MemberCount();
            // Don't early out here because an empty object is also considered a default object.
            for (auto it = inputValue.MemberBegin(); it != inputValue.MemberEnd(); ++it)
            {
                AZStd::string_view keyName(it->name.GetString(), it->name.GetStringLength());
                ScopedStackedString subPath(path, keyName);

                const rapidjson::Value& key = (keyName == JsonSerialization::DefaultStringIdentifier) ? defaultValue : it->name;

                JSR::Result elementResult = LoadElement(outputValue, container, pairElement, pairContainer, 
                    keyElement, valueElement, key, it->value, subPath, settings);
                if (elementResult.GetResultCode().GetProcessing() != JSR::Processing::Halted)
                {
                    retVal.Combine(elementResult.GetResultCode());
                }
                else
                {
                    return elementResult;
                }
            }
        }
        else
        {
            AZ_Assert(inputValue.IsArray(), "Maps can only be loaded from an object or an array.");
            maximumSize = inputValue.Size();
            if (maximumSize == 0)
            {
                return JSR::Result(settings, "No values provided for map.", JSR::ResultCode::Success(JSR::Tasks::ReadField), path);
            }
            for (rapidjson::SizeType i = 0; i < maximumSize; ++i)
            {
                ScopedStackedString subPath(path, i);

                if (!inputValue[i].IsObject())
                {
                    retVal.Combine(JSR::Result(settings, AZStd::string::format(
                            R"(Unsupported type for elements in an associative container. If the array for is used, an object with "%s" and "%s" is expected)",
                            JsonSerialization::KeyFieldIdentifier, JsonSerialization::ValueFieldIdentifier),
                        JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, subPath).GetResultCode());
                    continue;
                }

                const rapidjson::Value::ConstMemberIterator keyMember = inputValue[i].FindMember(JsonSerialization::KeyFieldIdentifier);
                const rapidjson::Value::ConstMemberIterator valueMember = inputValue[i].FindMember(JsonSerialization::ValueFieldIdentifier);
                const rapidjson::Value& key = (keyMember != inputValue[i].MemberEnd()) ? keyMember->value : defaultValue;
                const rapidjson::Value& value = (valueMember != inputValue[i].MemberEnd()) ? valueMember->value : defaultValue;

                JSR::Result elementResult = LoadElement(outputValue, container, pairElement, pairContainer,
                    keyElement, valueElement, key, value, subPath, settings);
                if (elementResult.GetResultCode().GetProcessing() != JSR::Processing::Halted)
                {
                    retVal.Combine(elementResult.GetResultCode());
                }
                else
                {
                    return elementResult;
                }
            }
        }
        
        size_t addedCount = container->Size(outputValue) - containerSize;
        AZStd::string_view message =
            addedCount >= maximumSize ? "Successfully read associative container." :
            addedCount == 0 ? "Unable to read data for the associative container." : 
            "Partially read data for the associative container.";
        return JSR::Result(settings, message, retVal, path);
    }

    JsonSerializationResult::Result JsonMapSerializer::LoadElement(void* outputValue, SerializeContext::IDataContainer* container,
        const SerializeContext::ClassElement* pairElement, SerializeContext::IDataContainer* pairContainer,
        const SerializeContext::ClassElement* keyElement, const SerializeContext::ClassElement* valueElement,
        const rapidjson::Value& key, const rapidjson::Value& value, StackedString& path, const JsonDeserializerSettings& settings)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        size_t expectedSize = container->Size(outputValue) + 1;
        void* address = container->ReserveElement(outputValue, pairElement);
        if (!address)
        {
            return JSR::Result(settings, "Failed to allocate an item for an associative container.",
                JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic), path);
        }

        // Load key
        void* keyAddress = pairContainer->GetElementByIndex(address, pairElement, 0);
        AZ_Assert(keyAddress, "Element reserved for associative container, but unable to retrieve address of the key.");
        Flags keyLoadFlags = Flags::None;
        if (keyElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
        {
            keyLoadFlags = Flags::ResolvePointer;
            *reinterpret_cast<void**>(keyAddress) = nullptr;
        }
        JSR::ResultCode keyResult = ContinueLoading(keyAddress, keyElement->m_typeId, key, path, settings, keyLoadFlags);
        if (keyResult.GetProcessing() == JSR::Processing::Halted)
        {
            container->FreeReservedElement(outputValue, address, settings.m_serializeContext);
            return JSR::Result(settings, "Failed to read key for associative container.", keyResult, path);
        }

        // Load value
        void* valueAddress = pairContainer->GetElementByIndex(address, pairElement, 1);
        AZ_Assert(valueAddress, "Element reserved for associative container, but unable to retrieve address of the value.");
        Flags valueLoadFlags = Flags::None; 
        if (valueElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
        {
            valueLoadFlags = Flags::ResolvePointer;
            *reinterpret_cast<void**>(valueAddress) = nullptr;
        }
        JSR::ResultCode valueResult = ContinueLoading(valueAddress, valueElement->m_typeId, value, path, settings, valueLoadFlags);
        if (valueResult.GetProcessing() == JSR::Processing::Halted)
        {
            container->FreeReservedElement(outputValue, address, settings.m_serializeContext);
            return JSR::Result(settings, "Failed to read value for associative container.", valueResult, path);
        }

        // Finalize loading
        if (keyResult.GetProcessing() == JSR::Processing::Altered ||
            valueResult.GetProcessing() == JSR::Processing::Altered)
        {
            container->FreeReservedElement(outputValue, address, settings.m_serializeContext);
            return JSR::Result(settings, "Unable to fully process an element for the associative container.",
                JSR::Tasks::ReadField, JSR::Outcomes::Unavailable, path);
        }
        else
        {
            container->StoreElement(outputValue, address);
            if (container->Size(outputValue) != expectedSize)
            {
                return JSR::Result(settings, "Unable to store the element that was read to the associative container.", 
                    JSR::Tasks::ReadField, JSR::Outcomes::Unavailable, path);
            }
        }
        
        return JSR::Result(settings, "Successfully loaded an entry into the associative container.", 
            JSR::ResultCode::Combine(keyResult, valueResult), path);
    }

    JsonSerializationResult::Result JsonMapSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator, 
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings, 
        bool sortResult)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        JsonBasicContainerSerializer serializer;

        // This will fill up keyValues with an array of pairs.
        rapidjson::Value keyValues(rapidjson::kObjectType);
        JSR::Result result = serializer.Store(keyValues, allocator, inputValue, defaultValue, valueTypeId, path, settings);
        JSR::ResultCode code = result.GetResultCode();
        if (code.GetProcessing() == JSR::Processing::Halted)
        {
            return JSR::Result(settings,
                "Associative container failed to extract key/value pairs from the provided container.", code, path);
        }

        if (keyValues.IsArray())
        {
            if (settings.m_keepDefaults && keyValues.Empty())
            {
                outputValue.SetArray();
                return JSR::Result(settings, "Successfully stored associative container.", code, path);
            }

            AZ_Assert(!keyValues.Empty(), "Intermediate array for associative container can't be empty "
                "because an empty array would be stored as an empty default object.")
            
            if (sortResult)
            {
                // Using std::sort because AZStd::sort isn't implemented in terms of move operations.
                auto less = [](const rapidjson::Value& lhs, const rapidjson::Value& rhs) -> bool
                {
                    return JsonSerialization::Compare(lhs, rhs) == JsonSerializerCompareResult::Less;
                };
                std::sort(keyValues.Begin(), keyValues.End(), less);
            }

            bool isString = false;
            for (auto it = keyValues.Begin(); it != keyValues.End(); ++it)
            {
                // Search for the first entry that's not a default value and use that to determine if
                // the key-entry is a string or not.
                if (it->IsArray() && !it->Empty())
                {
                    rapidjson::Value& key = (*it)[0];
                    if (key.IsString())
                    {
                        isString = true;
                        break;
                    }
                    else if (!IsExplicitDefault(key))
                    {
                        isString = false;
                        break;
                    }
                }
            }

            if (isString)
            {
                // Convert the array to a Object with the member being the key.
                outputValue.SetObject();
                for (auto it = keyValues.Begin(); it != keyValues.End(); ++it)
                {
                    if (it->IsArray() && it->Size() >= 2)
                    {
                        AZ_Assert(it->Size() == 2, "Associative container expected the entry to be a key/value pair but the array contained (%u) entries.", it->Size());
                        rapidjson::Value& key = (*it)[0];
                        if (IsExplicitDefault(key))
                        {
                            outputValue.AddMember(rapidjson::StringRef(JsonSerialization::DefaultStringIdentifier), AZStd::move((*it)[1]), allocator);
                        }
                        else
                        {
                            outputValue.AddMember(AZStd::move((*it)[0]), AZStd::move((*it)[1]), allocator);
                        }
                    }
                    else
                    {
                        AZ_Assert(!it->IsArray(), "Associative container expected the entry to be a key/value pair but the array contained (%u) entries.", it->Size());
                        AZ_Assert(IsExplicitDefault(*it), "Associative container expected the entry to be an explicit default object.");
                        outputValue.AddMember(rapidjson::StringRef(JsonSerialization::DefaultStringIdentifier), rapidjson::Value(rapidjson::kObjectType), allocator);
                    }
                }
            }
            else
            {
                // The key isn't a string so store the map as an array of key/value pairs.
                outputValue.SetArray();
                for (auto it = keyValues.Begin(); it != keyValues.End(); ++it)
                {
                    rapidjson::Value entry(rapidjson::kObjectType);
                    if (it->IsArray() && it->Size() >= 2)
                    {
                        AZ_Assert(it->Size() == 2, "Associative container expected the entry to be a key/value pair but the array contained (%u) entries.", it->Size());
                        entry.AddMember(rapidjson::StringRef(JsonSerialization::KeyFieldIdentifier), AZStd::move((*it)[0]), allocator);
                        entry.AddMember(rapidjson::StringRef(JsonSerialization::ValueFieldIdentifier), AZStd::move((*it)[1]), allocator);
                    }
                    else
                    {
                        AZ_Assert(!it->IsArray(), "Associative container expected the entry to be a key/value pair but the array contained (%u) entries.", it->Size());
                        AZ_Assert(IsExplicitDefault(*it), "Associative container expected the entry to be an explicit default object.");
                        entry.AddMember(rapidjson::StringRef(JsonSerialization::KeyFieldIdentifier), rapidjson::Value(rapidjson::kObjectType), allocator);
                        entry.AddMember(rapidjson::StringRef(JsonSerialization::ValueFieldIdentifier), rapidjson::Value(rapidjson::kObjectType), allocator);
                    }
                    outputValue.PushBack(AZStd::move(entry), allocator);
                }
            }
        }

        return JSR::Result(settings, "Successfully stored associative container.", code, path);
    }


    
    // JsonUnorderedMapSerializer

    JsonSerializationResult::Result JsonUnorderedMapSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        return JsonMapSerializer::Store(outputValue, allocator, inputValue, defaultValue, valueTypeId, path, settings, true);
    }
}