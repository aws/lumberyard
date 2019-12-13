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

#include <AzCore/Serialization/Json/JsonSerializer.h>

#include <AzCore/Serialization/Json/EnumSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/any.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    JsonSerializationResult::ResultCode JsonSerializer::Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const void* object, const void* defaultObject, const Uuid& typeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        const SerializeContext::ClassData* classData = settings.m_serializeContext->FindClassData(typeId);
        if (!classData)
        {
            return settings.m_reporting(
                OSString::format("Failed to retrieve serialization information for %s.", typeId.ToString<OSString>().c_str()),
                ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
        }

        if (!defaultObject && !settings.m_keepDefaults)
        {
            ResultCode result(Tasks::WriteValue);
            AZStd::any defaultObjectInstance = settings.m_serializeContext->CreateAny(typeId);
            if (defaultObjectInstance.empty())
            {
                result = settings.m_reporting("No factory available to create a default object for comparison.",
                    ResultCode(Tasks::CreateDefault, Outcomes::Unsupported), path);
            }
            void* defaultObjectPtr = AZStd::any_cast<void>(&defaultObjectInstance);
            ResultCode conversionResult = StoreWithClassData(output, allocator, object, defaultObjectPtr, *classData, StoreTypeId::No, path, settings);
            return ResultCode::Combine(result, conversionResult);
        }
        else
        {
            return StoreWithClassData(output, allocator, object, defaultObject, *classData, StoreTypeId::No, path, settings);
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreFromPointer(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const void* object, const void* defaultObject, const Uuid& typeId,
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        const SerializeContext::ClassData* classData = settings.m_serializeContext->FindClassData(typeId);
        if (!classData)
        {
            return settings.m_reporting(
                OSString::format("Failed to retrieve serialization information for %s.", typeId.ToString<OSString>().c_str()),
                ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
        }
        if (!classData->m_azRtti)
        {
            return settings.m_reporting(
                OSString::format("Failed to retrieve rtti information for %s.", classData->m_name),
                ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
        }
        AZ_Assert(classData->m_azRtti->GetTypeId() == typeId, "Type id mismatch in '%s' during serialization to a json file. (%s vs %s)",
            classData->m_name, classData->m_azRtti->GetTypeId().ToString<OSString>().c_str(), typeId.ToString<OSString>().c_str());

        return StoreWithClassDataFromPointer(output, allocator, object, defaultObject, *classData, path, settings);
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreWithClassData(rapidjson::Value& node, rapidjson::Document::AllocatorType& allocator,
        const void* object, const void* defaultObject, const SerializeContext::ClassData& classData, StoreTypeId storeTypeId,
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        // Start by setting the object to be an explicit default.
        node.SetObject();

        auto serializer = settings.m_registrationContext->GetSerializerForType(classData.m_typeId);
        if (serializer)
        {
            if (storeTypeId == StoreTypeId::Yes)
            {
                return settings.m_reporting("Unable to store type information in a JSON Serializer primitive.",
                    ResultCode(Tasks::WriteValue, Outcomes::Catastrophic), path);
            }

            return serializer->Store(node, allocator, object, defaultObject, classData.m_typeId, path, settings);
        }

        if (classData.m_azRtti && (classData.m_azRtti->GetTypeTraits() & AZ::TypeTraits::is_enum) == AZ::TypeTraits::is_enum)
        {
            serializer = settings.m_registrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>());
            if (serializer)
            {
                if (storeTypeId == StoreTypeId::Yes)
                {
                    return settings.m_reporting("Unable to store type information in a JSON Serializer enum.",
                        ResultCode(Tasks::WriteValue, Outcomes::Catastrophic), path);
                }
                return serializer->Store(node, allocator, object, defaultObject, classData.m_typeId, path, settings);
            }
        }

        if (classData.m_azRtti && classData.m_azRtti->GetGenericTypeId() != classData.m_typeId)
        {
            serializer = settings.m_registrationContext->GetSerializerForType(classData.m_azRtti->GetGenericTypeId());
            if (serializer)
            {
                if (storeTypeId == StoreTypeId::Yes)
                {
                    return settings.m_reporting("Unable to store type information in a JSON Serializer primitive.",
                        ResultCode(Tasks::WriteValue, Outcomes::Catastrophic), path);
                }

                return serializer->Store(node, allocator, object, defaultObject, classData.m_typeId, path, settings);
            }
        }
        
        if (classData.m_container)
        {
            if (storeTypeId == StoreTypeId::Yes)
            {
                return settings.m_reporting("Unable to store type information in a JSON Serializer container.",
                    ResultCode(Tasks::WriteValue, Outcomes::Catastrophic), path);
            }
            return StoreContainer(node.SetArray(), allocator, object, classData, path, settings);
        }
        else
        {
            if (storeTypeId == StoreTypeId::Yes)
            {
                node.AddMember(rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier),
                    StoreTypeName(allocator, classData, settings), allocator);
            }
            return StoreClass(node, allocator, object, defaultObject, classData, path, settings);
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreWithClassDataFromPointer(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const void* object, const void* defaultObject, const SerializeContext::ClassData& classData,
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        StoreTypeId storeTypeId = StoreTypeId::No;
        Uuid resolvedTypeId = classData.m_typeId;
        const SerializeContext::ClassData* resolvedClassData = &classData;
        AZStd::any defaultPointerObject;

        ResultCode result(Tasks::RetrieveInfo);
        ResolvePointerResult pointerResolution = ResolvePointer(result, storeTypeId, object, defaultObject, defaultPointerObject,
            resolvedClassData, *classData.m_azRtti, path, settings);
        if (pointerResolution == ResolvePointerResult::FullyProcessed)
        {
            return result;
        }
        else if (pointerResolution == ResolvePointerResult::WriteNull)
        {
            output = rapidjson::Value(rapidjson::kNullType);
            return result;
        }
        else
        {
            return StoreWithClassData(output, allocator, object, defaultObject, *resolvedClassData, storeTypeId, path, settings);
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreWithClassElement(rapidjson::Value& parentNode, rapidjson::Document::AllocatorType& allocator,
        const void* object, const void* defaultObject, const SerializeContext::ClassElement& classElement,
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        ScopedStackedString elementPath(path, classElement.m_name);

        const SerializeContext::ClassData* elementClassData =
            settings.m_serializeContext->FindClassData(classElement.m_typeId);
        if (!elementClassData)
        {
            return settings.m_reporting("Failed to retrieve serialization information.",
                ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), elementPath);
        }
        if (!elementClassData->m_azRtti)
        {
            return settings.m_reporting(
                OSString::format("Failed to retrieve rtti information for %s.", elementClassData->m_name),
                ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
        }
        AZ_Assert(elementClassData->m_azRtti->GetTypeId() == elementClassData->m_typeId, "Type id mismatch in '%s' during serialization to a json file. (%s vs %s)",
            elementClassData->m_name, elementClassData->m_azRtti->GetTypeId().ToString<OSString>().c_str(), elementClassData->m_typeId.ToString<OSString>().c_str());

        if (classElement.m_flags & SerializeContext::ClassElement::FLG_NO_DEFAULT_VALUE)
        {
            defaultObject = nullptr;
        }

        if (classElement.m_flags & SerializeContext::ClassElement::FLG_BASE_CLASS)
        {
            // Base class information can be reconstructed so doesn't need to be written to the final json. StoreClass
            // will simply pick up where this left off and write to the same element.
            return StoreClass(parentNode, allocator, object, defaultObject, *elementClassData, elementPath, settings);
        }
        else
        {
            rapidjson::Value value;
            ResultCode result = classElement.m_flags & SerializeContext::ClassElement::FLG_POINTER ?
                StoreWithClassDataFromPointer(value, allocator, object, defaultObject, *elementClassData, elementPath, settings):
                StoreWithClassData(value, allocator, object, defaultObject, *elementClassData, StoreTypeId::No, elementPath, settings);
            if (result.GetProcessing() != Processing::Halted)
            {
                if (parentNode.IsObject())
                {
                    if (settings.m_keepDefaults || result.GetOutcome() != Outcomes::DefaultsUsed)
                    {
                        parentNode.AddMember(rapidjson::StringRef(classElement.m_name), AZStd::move(value), allocator);
                    }
                }
                else if (parentNode.IsArray())
                {
                    // Always add a value to the array, even if it's fully defaulted, to make sure the correct indexes are assigned.
                    parentNode.PushBack(AZStd::move(value), allocator);
                }
                else
                {
                    result = settings.m_reporting("StoreWithClassElement can only write to objects or arrays. Unexpected type encountered.",
                        ResultCode(Tasks::WriteValue, Outcomes::Catastrophic), elementPath);
                }
            }
            return result;
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreClass(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const void* object, const void* defaultObject, const SerializeContext::ClassData& classData,
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZ_Assert(output.IsObject(), "Unable to write class to the json node as it's not an object.");
        if (!classData.m_elements.empty())
        {
            ResultCode result(Tasks::WriteValue);
            for (const SerializeContext::ClassElement& element : classData.m_elements)
            {
                const void* elementPtr = reinterpret_cast<const uint8_t*>(object) + element.m_offset;
                const void* elementDefaultPtr = defaultObject ?
                    (reinterpret_cast<const uint8_t*>(defaultObject) + element.m_offset) : nullptr;

                result.Combine(StoreWithClassElement(output, allocator,
                    elementPtr, elementDefaultPtr, element, path, settings));
            }
            return result;
        }
        else
        {
            return settings.m_reporting("Class didn't contain any elements to store.",
                ResultCode(Tasks::WriteValue, settings.m_keepDefaults ? Outcomes::Success : Outcomes::DefaultsUsed), path);
        }
    }

    JsonSerializationResult::ResultCode JsonSerializer::StoreContainer(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const void* object, const SerializeContext::ClassData& classData, StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        AZ_Assert(output.IsArray(), "Unable to write array to the json as the node isn't a json array.");
        if (classData.m_container->GetAssociativeContainerInterface())
        {
            return settings.m_reporting("Serializer does not currently support storing associative containers.",
                ResultCode(Tasks::WriteValue, Outcomes::Unsupported), path);
        }

        if (classData.m_container->Size(const_cast<void*>(object)) > 0)
        {
            ResultCode result(Tasks::WriteValue);

            uint32_t counter = 0;
            auto elementCallback = [&output, &result, &allocator, &counter, &path, &settings]
                (void* elementPtr, const Uuid& elementId, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement)
            {
                ScopedStackedString containerPath(path, counter);

                // Note that the default for containers is to be considered as empty, even if the provided default has elements in the container.
                // Create a default object per instance since a polymorphic object could be stored.
                AZStd::any defaultObject;
                void* defaultObjectPtr = nullptr;
                if (!settings.m_keepDefaults && !(classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER))
                {
                    defaultObject = settings.m_serializeContext->CreateAny(elementId);
                    if (defaultObject.empty())
                    {
                        ResultCode errorCode = settings.m_reporting("No factory available to create a default array object for comparison.",
                            ResultCode(Tasks::CreateDefault, Outcomes::Unsupported), containerPath);
                        result.Combine(errorCode);
                    }
                    defaultObjectPtr = AZStd::any_cast<void>(&defaultObject);
                }

                ResultCode conversionResult(Tasks::WriteValue);
                if (classElement)
                {
                    conversionResult = StoreWithClassElement(output, allocator, elementPtr, defaultObjectPtr,
                        *classElement, containerPath, settings);
                }
                else if (classData)
                {
                    rapidjson::Value elementValue;
                    conversionResult = StoreWithClassData(elementValue, allocator, elementPtr, defaultObjectPtr, *classData, 
                        StoreTypeId::No, containerPath, settings);
                    if (conversionResult.GetProcessing() != Processing::Halted)
                    {
                        // Always add a value to the array, even if it's fully defaulted, to make sure the correct indexes are assigned.
                        output.PushBack(AZStd::move(elementValue), allocator);
                    }
                    else
                    {
                        conversionResult = settings.m_reporting("Failed to convert and add entry to array. Any empty value will be added.",
                            ResultCode(Tasks::WriteValue, Outcomes::Unsupported), containerPath);
                        output.PushBack(rapidjson::Value(rapidjson::kObjectType), allocator);
                    }
                }
                else
                {
                    conversionResult = settings.m_reporting(
                        "Unable to convert container entry because no class or element data was provided. Any empty value will be added.",
                        ResultCode(Tasks::WriteValue, Outcomes::Unknown), containerPath);
                    output.PushBack(rapidjson::Value(rapidjson::kObjectType), allocator);
                }
                result.Combine(conversionResult);
                counter++;
                return true;
            };
            classData.m_container->EnumElements(const_cast<void*>(object), elementCallback);

            if (result.GetOutcome() == Outcomes::DefaultsUsed)
            {
                SetExplicitDefault(output);
            }
            else if (classData.m_container->IsSmartPointer())
            {
                // A smart pointer will only have one entry, which should be presented as a pointer instead of an
                // array, so in this case simply replace the array with the pointer information.
                if (output.Empty())
                {
                    output.SetNull();
                }
                else
                {
                    output = AZStd::move(output[0]);
                }
            }
            
            return result;
        }
        else
        {
            if (!settings.m_keepDefaults)
            {
                SetExplicitDefault(output);
            }
            return settings.m_reporting("Nothing written as target container is empty.",
                ResultCode(Tasks::WriteValue, settings.m_keepDefaults ? Outcomes::Success : Outcomes::DefaultsUsed), path);
        }
    }

    JsonSerializer::ResolvePointerResult JsonSerializer::ResolvePointer(
        JsonSerializationResult::ResultCode& status, StoreTypeId& storeTypeId,
        const void*& object, const void*& defaultObject, AZStd::any& defaultObjectStorage,
        const SerializeContext::ClassData*& elementClassData, const AZ::IRttiHelper& rtti,
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        object = *reinterpret_cast<const void* const*>(object);
        defaultObject = defaultObject ? *reinterpret_cast<const void* const*>(defaultObject) : nullptr;
        if (!object)
        {
            status = ResultCode(status.GetTask(), (settings.m_keepDefaults || defaultObject)? Outcomes::Success : Outcomes::DefaultsUsed);
            return ResolvePointerResult::WriteNull;
        }

        // The pointer is pointing to an instance, so determine if the pointer is polymorphic. If so, make to 
        // tell the caller of this function to write the type id and provide a default object, if requested, for
        // the specific polymorphic instance the pointer is pointing to.
        const AZ::Uuid& actualClassId = rtti.GetActualUuid(object);
        const AZ::Uuid& actualDefaultClassId = rtti.GetActualUuid(defaultObject);

        if (actualClassId != rtti.GetTypeId())
        {
            elementClassData = settings.m_serializeContext->FindClassData(actualClassId);
            if (!elementClassData)
            {
                status = settings.m_reporting(AZ::OSString::format(
                    "Failed to retrieve serialization information for because type '%s' is unknown.", actualClassId.ToString<OSString>().c_str()),
                    ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
                return ResolvePointerResult::FullyProcessed;
            }

            storeTypeId = (actualClassId != actualDefaultClassId || settings.m_keepDefaults) ?
                StoreTypeId::Yes : StoreTypeId::No;
        }
        if (actualDefaultClassId != actualClassId)
        {
            // Defaults is pointing to a different type than is stored, so those defaults can be used.
            defaultObject = nullptr;
        }

        if (!defaultObject && !settings.m_keepDefaults)
        {
            defaultObjectStorage = settings.m_serializeContext->CreateAny(actualClassId);
            if (defaultObjectStorage.empty())
            {
                status = settings.m_reporting(AZ::OSString::format(
                    "No factory available to create a default pointer object for comparison for type %s.", actualClassId.ToString<OSString>().c_str()),
                    ResultCode(Tasks::CreateDefault, Outcomes::Unsupported), path);
                return ResolvePointerResult::FullyProcessed;
            }
            defaultObject = AZStd::any_cast<void>(&defaultObjectStorage);
        }
        return ResolvePointerResult::ContinueProcessing;
    }

    rapidjson::Value JsonSerializer::StoreTypeName(rapidjson::Document::AllocatorType& allocator,
        const SerializeContext::ClassData& classData, const JsonSerializerSettings& settings)
    {
        rapidjson::Value result;
        AZStd::vector<Uuid> ids = settings.m_serializeContext->FindClassId(Crc32(classData.m_name));
        if (ids.size() <= 1)
        {
            result = rapidjson::StringRef(classData.m_name);
        }
        else
        {
            // Only write the Uuid for the class if there are multiple classes sharing the same name.
            // In this case it wouldn't be enough to determine which class needs to be used. The 
            // class name is still added as a comment for be friendlier for users to read.
            AZStd::string fullName = classData.m_typeId.ToString<OSString>();
            fullName += ' ';
            fullName += classData.m_name;
            result.SetString(fullName.c_str(), aznumeric_caster(fullName.size()), allocator);
        }
        return result;
    }

    void JsonSerializer::SetExplicitDefault(rapidjson::Value& value)
    {
        value.SetObject();
    }
} // namespace AZ