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

#include <AzCore/Serialization/Json/JsonDeserializer.h>

#include <AzCore/Serialization/Json/EnumSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    JsonSerializationResult::ResultCode JsonDeserializer::Load(void* object, const Uuid& typeId, const rapidjson::Value& value,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace AZ::JsonSerializationResult;

        if (IsExplicitDefault(value))
        {
            return settings.m_reporting("Value has an explicit default.", ResultCode::Default(Tasks::ReadField), path);
        }

        BaseJsonSerializer* serializer = settings.m_registrationContext->GetSerializerForType(typeId);
        if (serializer)
        {
            return serializer->Load(object, typeId, value, path, settings);
        }
        
        const SerializeContext::ClassData* classData = settings.m_serializeContext->FindClassData(typeId);
        if (!classData)
        {
            return settings.m_reporting(
                OSString::format("Failed to retrieve serialization information for %s.", typeId.ToString<OSString>().c_str()),
                ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
        }

        if (classData->m_azRtti && (classData->m_azRtti->GetTypeTraits() & AZ::TypeTraits::is_enum) == AZ::TypeTraits::is_enum)
        {
            serializer = settings.m_registrationContext->GetSerializerForSerializerType(azrtti_typeid<AZ::JsonEnumSerializer>());
            if (serializer)
            {
                return serializer->Load(object, typeId, value, path, settings);
            }
        }

        if (classData->m_azRtti && classData->m_azRtti->GetGenericTypeId() != typeId)
        {
            serializer = settings.m_registrationContext->GetSerializerForType(classData->m_azRtti->GetGenericTypeId());
            if (serializer)
            {
                return serializer->Load(object, typeId, value, path, settings);
            }
        }
        
        if (classData->m_container)
        {
            return LoadContainer(object, *classData, value, path, settings);
        }
        else if (value.IsObject())
        {
            return LoadClass(object, *classData, value, path, settings);
        }
        else
        {
            return settings.m_reporting(
                OSString::format("Reading into targets of type '%s' is not supported.", classData->m_name),
                ResultCode(Tasks::ReadField, Outcomes::Unsupported), path);
        }
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadToPointer(void* object, const Uuid& typeId, const rapidjson::Value& value,
        StackedString& path, const JsonDeserializerSettings& settings)
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
        AZ_Assert(classData->m_azRtti->GetTypeId() == typeId, "Type id mismatch during deserialization of a json file. (%s vs %s)");

        void* resolvedObject = object;
        Uuid resolvedTypeId = typeId;
        ResultCode status(Tasks::RetrieveInfo);

        JsonDeserializer::ResolvePointerResult pointerResolution =
            JsonDeserializer::ResolvePointer(resolvedObject, resolvedTypeId, status, value,
                *classData->m_azRtti, path, settings);

        if (pointerResolution == JsonDeserializer::ResolvePointerResult::FullyProcessed)
        {
            return status;
        }
        else
        {
            return JsonDeserializer::Load(resolvedObject, resolvedTypeId, value, path, settings);
        }
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadWithClassElement(void* object, const rapidjson::Value& value,
        const SerializeContext::ClassElement& classElement, StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace AZ::JsonSerializationResult;

        if (classElement.m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
        {
            if (!classElement.m_azRtti)
            {
                return settings.m_reporting(
                    OSString::format("Failed to retrieve rtti information for %s.", classElement.m_name),
                    ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
            }
            AZ_Assert(classElement.m_azRtti->GetTypeId() == classElement.m_typeId,
                "Type id mismatch during deserialization of a json file. (%s vs %s)");
            return LoadToPointer(object, classElement.m_typeId, value, path, settings);
        }
        else
        {
            return Load(object, classElement.m_typeId, value, path, settings);
        }
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadClass(void* object, const SerializeContext::ClassData& classData,
        const rapidjson::Value& value, StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace AZ::JsonSerializationResult;

        AZ_Assert(settings.m_registrationContext && settings.m_serializeContext, "Expected valid registration context and serialize context.");

        if (value.ObjectEmpty())
        {
            return settings.m_reporting("Object doesn't contain any elements.", ResultCode::Success(Tasks::ReadField), path);
        }

        size_t numLoads = 0;
        ResultCode retVal(Tasks::ReadField);
        for (auto iter = value.MemberBegin(); iter != value.MemberEnd(); ++iter)
        {
            const rapidjson::Value& val = iter->value;
            AZStd::string_view name(iter->name.GetString(), iter->name.GetStringLength());
            if (name == JsonSerialization::TypeIdFieldIdentifier)
            {
                continue;
            }
            Crc32 nameCrc(name);
            ElementDataResult foundElementData = FindElementByNameCrc(*settings.m_serializeContext, object, classData, nameCrc);

            ScopedStackedString subPath(path, name);
            if (foundElementData.m_found)
            {
                ResultCode result = LoadWithClassElement(foundElementData.m_data, val, *foundElementData.m_info, subPath, settings);
                retVal.Combine(result);

                if (result.GetProcessing() == Processing::Halted)
                {
                    return settings.m_reporting("Loading of element has failed.", retVal, path);
                }
                else if (result.GetProcessing() != Processing::Altered)
                {
                    numLoads++;
                }
            }
            else
            {
                retVal.Combine(settings.m_reporting("Skipping field as there's no matching variable in the target.",
                    ResultCode(Tasks::ReadField, Outcomes::Skipped), subPath));
            }
        }

        size_t elementCount = CountElements(*settings.m_serializeContext, classData);
        if (elementCount > numLoads)
        {
            retVal.Combine(ResultCode(Tasks::ReadField, numLoads == 0 ? Outcomes::DefaultsUsed : Outcomes::PartialDefaults));
        }

        return retVal;
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadContainer(void* containerPtr, const SerializeContext::ClassData& containerClass,
        const rapidjson::Value& value, StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace AZ::JsonSerializationResult;

        AZ_Assert(containerClass.m_container, "LoadContainer called for a class that doesn't contain a container.");
        SerializeContext::IDataContainer* container = containerClass.m_container;
        
        // Check if the container stores data that can't be imported.
        if (container->GetAssociativeContainerInterface())
        {
            return settings.m_reporting("Serializer does not currently support reading associative containers. "
                "If this message is encountered then a specialized serializer for associative containers is missing.",
                ResultCode(Tasks::ReadField, Outcomes::Unsupported), path);
        }
        size_t typesCounted = 0;
        auto typeCounter = [&typesCounted](const Uuid&, const SerializeContext::ClassElement*)
        {
            typesCounted++;
            return true;
        };
        container->EnumTypes(typeCounter);
        if (typesCounted > 1)
        {
            return settings.m_reporting("Serializer does not currently reading to containers that with multiple types. "
                "If this message is encountered then a specialized serializer is missing.",
                ResultCode(Tasks::ReadField, Outcomes::Unsupported), path);
        }

        ResultCode result(Tasks::ReadField);
        SerializeContext::IEventHandler* eventHandler = containerClass.m_eventHandler;
        if (eventHandler)
        {
            eventHandler->OnWriteBegin(containerPtr);
        }

        if (value.IsArray())
        {
            if (!value.Empty())
            {
                uint32_t counter = 0;
                for (auto iter = value.Begin(); iter != value.End(); ++iter)
                {
                    ScopedStackedString subPath(path, counter);

                    ResultCode intermediateResult = LoadItemInContainer(containerPtr, *container, containerClass, *iter, subPath, settings);
                    result.Combine(intermediateResult);
                    if (intermediateResult.GetProcessing() == Processing::Halted)
                    {
                        break;
                    }
                    
                    counter++;
                }
            }
            else
            {
                result = settings.m_reporting("Array doesn't contain any entries.", ResultCode::Success(Tasks::ReadField), path);
            }
        }
        else
        {
            result = LoadItemInContainer(containerPtr, *container, containerClass, value, path, settings);
        }

        if (eventHandler)
        {
            eventHandler->OnWriteEnd(containerPtr);
            eventHandler->OnLoadedFromObjectStream(containerPtr);
        }

        return result;
    }

    JsonSerializationResult::ResultCode JsonDeserializer::LoadItemInContainer(void* containerPtr, SerializeContext::IDataContainer& container,
        const SerializeContext::ClassData& containerClassData, const rapidjson::Value& value, 
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace AZ::JsonSerializationResult;

        ResultCode retVal(Tasks::ReadField);
        auto loader = [containerPtr, &container, &containerClassData, &retVal, &value, &path, &settings]
            (const Uuid& elementTypeId, const SerializeContext::ClassElement* classElement)
        {
            AZ_Assert(classElement, "Received nullptr for classElement.");
            AZ_Assert(elementTypeId == classElement->m_typeId, "Mismatching class element types.");
            AZ_UNUSED(elementTypeId);

            void* allocatedItem = container.ReserveElement(containerPtr, classElement);
            if (!allocatedItem)
            {
                if ((container.IsFixedCapacity() || container.IsFixedSize()) &&
                    container.Size(containerPtr) >= container.Capacity(containerPtr))
                {
                    retVal = settings.m_reporting("Failed to allocate an item in the container, because it is fixed size and already full.",
                        ResultCode::Success(Tasks::ReadField), path);
                }
                else
                {
                    retVal = settings.m_reporting("Failed to allocate an item in the target container.",
                        ResultCode(Tasks::ReadField, Outcomes::Catastrophic), path);
                }
                return false;
            }
            
            if (classElement->m_flags & SerializeContext::ClassElement::Flags::FLG_POINTER)
            {
                *reinterpret_cast<void**>(allocatedItem) = nullptr;
            }

            ResultCode result = LoadWithClassElement(allocatedItem, value, *classElement, path, settings);
            if (result.GetProcessing() == Processing::Halted || result.GetProcessing() == Processing::Altered)
            {
                container.RemoveElement(containerPtr, allocatedItem, settings.m_serializeContext);
            }
            else
            {
                container.StoreElement(containerPtr, allocatedItem);
            }

            retVal.Combine(result);
            if (result.GetProcessing() == Processing::Halted)
            {
                return false;
            }

            return true;
        };
        container.EnumTypes(loader);

        return retVal;
    }

    JsonDeserializer::ResolvePointerResult JsonDeserializer::ResolvePointer(void*& object, Uuid& objectType, JsonSerializationResult::ResultCode& status,
        const rapidjson::Value& pointerData, const AZ::IRttiHelper& rtti, StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace AZ::JsonSerializationResult;

        void* storedAddress = *reinterpret_cast<void**>(object);
        if (pointerData.IsNull())
        {
            // Json files stores a nullptr, but there has been data assigned to the pointer, so the data needs to be freed.
            if (storedAddress)
            {
                const AZ::Uuid& actualClassId = rtti.GetActualUuid(storedAddress);
                if (actualClassId != objectType)
                {
                    const SerializeContext::ClassData* actualClassData = settings.m_serializeContext->FindClassData(actualClassId);
                    if (!actualClassData)
                    {
                        status = settings.m_reporting(
                            OSString::format("Unable to find serialization information for type %s.", actualClassId.ToString<OSString>().c_str()),
                            ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
                        return ResolvePointerResult::FullyProcessed;
                    }

                    if (actualClassData->m_factory)
                    {
                        actualClassData->m_factory->Destroy(storedAddress);
                        *reinterpret_cast<void**>(object) = nullptr;
                    }
                    else
                    {
                        status = settings.m_reporting("Unable to find the factory needed to clear out the default value.",
                            ResultCode(Tasks::RetrieveInfo, Outcomes::Catastrophic), path);
                        return ResolvePointerResult::FullyProcessed;
                    }
                }
            }
            status = ResultCode::Success(Tasks::ReadField);
            return ResolvePointerResult::FullyProcessed;
        }
        else
        {
            // There's data stored in the JSON document so try to determine the type, create an instance of it and 
            // return the new object to continue loading.
            LoadTypeIdResult loadedTypeId = LoadTypeIdFromJsonObject(pointerData, rtti, path, settings);
            if (loadedTypeId.m_determination == TypeIdDetermination::FailedToDetermine ||
                loadedTypeId.m_determination == TypeIdDetermination::FailedDueToMultipleTypeIds)
            {
                AZStd::string_view message = loadedTypeId.m_determination == TypeIdDetermination::FailedDueToMultipleTypeIds ?
                    "Unable to resolve provided type because the same name points to multiple types." :
                    "Unable to resolve provided type.";
                status = settings.m_reporting(message, ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
                return ResolvePointerResult::FullyProcessed;
            }

            if (loadedTypeId.m_typeId != objectType)
            {
                const SerializeContext::ClassData* targetClassData = settings.m_serializeContext->FindClassData(loadedTypeId.m_typeId);
                if (targetClassData)
                {
                    if (!settings.m_serializeContext->CanDowncast(loadedTypeId.m_typeId, objectType, targetClassData->m_azRtti, &rtti))
                    {
                        status = settings.m_reporting("Requested type can't be cast to the type of the target value.",
                            ResultCode(Tasks::Convert, Outcomes::TypeMismatch), path);
                        return ResolvePointerResult::FullyProcessed;
                    }
                }
                else
                {
                    status = settings.m_reporting("Serialization information for target type not found.",
                        ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
                    return ResolvePointerResult::FullyProcessed;
                }
                objectType = loadedTypeId.m_typeId;
            }

            if (storedAddress)
            {
                // The pointer already has an assigned value so look if the type is the same, in which case load
                // on top of it, or clean it up and create a new instance if they're not the same.
                if (loadedTypeId.m_determination == TypeIdDetermination::ExplicitTypeId)
                {
                    const AZ::Uuid& actualClassId = rtti.GetActualUuid(storedAddress);
                    if (actualClassId != loadedTypeId.m_typeId)
                    {
                        const SerializeContext::ClassData* actualClassData = settings.m_serializeContext->FindClassData(actualClassId);
                        if (!actualClassData)
                        {
                            status = settings.m_reporting(
                                OSString::format("Unable to find serialization information for type %s.", actualClassId.ToString<OSString>().c_str()),
                                ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
                            return ResolvePointerResult::FullyProcessed;
                        }

                        if (actualClassData->m_factory)
                        {
                            actualClassData->m_factory->Destroy(storedAddress);
                            *reinterpret_cast<void**>(object) = nullptr;
                            storedAddress = nullptr;
                        }
                        else
                        {
                            status = settings.m_reporting("Unable to find the factory needed to clear out the default value.",
                                ResultCode(Tasks::RetrieveInfo, Outcomes::Catastrophic), path);
                            return ResolvePointerResult::FullyProcessed;
                        }
                    }
                }
                else
                {
                    // The json file doesn't explicitly specify a type so use the type that's already assigned and assume it's the
                    // default.
                    objectType = rtti.GetActualUuid(storedAddress);
                }
            }

            if (!storedAddress)
            {
                // There's no object yet, so create an instance, otherwise reuse the existing object if possible.
                const SerializeContext::ClassData* actualClassData = settings.m_serializeContext->FindClassData(loadedTypeId.m_typeId);
                if (actualClassData)
                {
                    if (actualClassData->m_factory)
                    {
                        if (actualClassData->m_azRtti)
                        {
                            if (actualClassData->m_azRtti->IsAbstract())
                            {
                                status = settings.m_reporting("Unable to create an instance of a base class.",
                                    ResultCode(Tasks::CreateDefault, Outcomes::Catastrophic), path);
                                return ResolvePointerResult::FullyProcessed;
                            }
                        }

                        void* instance = actualClassData->m_factory->Create("Json Serializer");
                        if (!instance)
                        {
                            status = settings.m_reporting("Unable to create a new instance.",
                                ResultCode(Tasks::CreateDefault, Outcomes::Catastrophic), path);
                            return ResolvePointerResult::FullyProcessed;
                        }
                        *reinterpret_cast<void**>(object) = instance;
                        storedAddress = instance;
                    }
                    else
                    {
                        status = settings.m_reporting("Unable to find the factory needed to clear to create a new value.",
                            ResultCode(Tasks::RetrieveInfo, Outcomes::Catastrophic), path);
                        return ResolvePointerResult::FullyProcessed;
                    }
                }
                else
                {
                    status = settings.m_reporting(
                        OSString::format("Unable to find serialization information for type %s.", loadedTypeId.m_typeId.ToString<OSString>().c_str()),
                        ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
                    return ResolvePointerResult::FullyProcessed;
                }
            }
            object = *reinterpret_cast<void**>(object);
        }
        return ResolvePointerResult::ContinueProcessing;
    }

    JsonDeserializer::LoadTypeIdResult JsonDeserializer::LoadTypeIdFromJsonObject(const rapidjson::Value& node, const AZ::IRttiHelper& rtti,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        LoadTypeIdResult result;

        // Node doesn't contain an object, so there's no room to store the type id field. In this case the element can't be 
        // specialized, so return it's type.
        if (!node.IsObject())
        {
            result.m_determination = TypeIdDetermination::ImplicitTypeId;
            result.m_typeId = rtti.GetTypeId();
            return result;
        }

        auto typeField = node.FindMember(JsonSerialization::TypeIdFieldIdentifier);
        if (typeField == node.MemberEnd())
        {
            result.m_determination = TypeIdDetermination::ImplicitTypeId;
            result.m_typeId = rtti.GetTypeId();
            return result;
        }

        if (typeField->value.IsString())
        {
            return LoadTypeIdFromJsonString(typeField->value, &rtti, path, settings);
        }

        result.m_determination = TypeIdDetermination::FailedToDetermine;
        result.m_typeId = Uuid::CreateNull();
        return result;
    }

    JsonDeserializer::LoadTypeIdResult JsonDeserializer::LoadTypeIdFromJsonString(const rapidjson::Value& node, const AZ::IRttiHelper* baseClassRtti,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;

        LoadTypeIdResult result;

        if (node.IsString())
        {
            // First check if the string contains a Uuid and if so, use that as the type id.
            BaseJsonSerializer* uuidSerializer = settings.m_registrationContext->GetSerializerForType(azrtti_typeid<Uuid>());
            if (uuidSerializer)
            {
                ResultCode serializerResult = uuidSerializer->Load(&result.m_typeId, azrtti_typeid<Uuid>(), node, path, settings);
                if (serializerResult.GetOutcome() == Outcomes::Success)
                {
                    result.m_determination = TypeIdDetermination::ExplicitTypeId;
                    return result;
                }
            }

            // There was no uuid, so use the name to look the info in the Serialize Context
            AZStd::string_view typeName(node.GetString(), node.GetStringLength());
            // Remove any leading and/or trailing whitespace.
            auto spaceCheck = [](char chr) -> bool { return !AZStd::is_space(chr); };
            typeName.remove_prefix(AZStd::find_if(typeName.begin(), typeName.end(), spaceCheck) - typeName.begin());
            typeName.remove_suffix(AZStd::find_if(typeName.rbegin(), typeName.rend(), spaceCheck) - typeName.rbegin());

            Crc32 nameCrc = Crc32(typeName);
            AZStd::vector<Uuid> typeIdCandidates = settings.m_serializeContext->FindClassId(nameCrc);
            if (typeIdCandidates.empty())
            {
                result.m_determination = TypeIdDetermination::FailedToDetermine;
                result.m_typeId = Uuid::CreateNull();
                return result;
            }
            else if (typeIdCandidates.size() == 1)
            {
                result.m_determination = TypeIdDetermination::ExplicitTypeId;
                result.m_typeId = typeIdCandidates[0];
                return result;
            }
            else if (baseClassRtti)
            {
                size_t numApplicableCandidates = 0;
                const Uuid* lastApplicableCandidate = nullptr;
                for (const Uuid& typeId : typeIdCandidates)
                {
                    const SerializeContext::ClassData* classData = settings.m_serializeContext->FindClassData(typeId);
                    if (settings.m_serializeContext->CanDowncast(typeId, baseClassRtti->GetTypeId(), 
                        classData ? classData->m_azRtti : nullptr, baseClassRtti))
                    {
                        ++numApplicableCandidates;
                        lastApplicableCandidate = &typeId;
                    }
                }

                if (numApplicableCandidates == 1)
                {
                    result.m_determination = TypeIdDetermination::ExplicitTypeId;
                    result.m_typeId = *lastApplicableCandidate;
                    return result;
                }
            }

            // Multiple classes can fulfill this requested type so this needs to be disambiguated through an explicit
            // uuid instead of a name.
            result.m_determination = TypeIdDetermination::FailedDueToMultipleTypeIds;
            result.m_typeId = Uuid::CreateNull();
            return result;
        }
        
        result.m_determination = TypeIdDetermination::FailedToDetermine;
        result.m_typeId = Uuid::CreateNull();
        return result;
    }

    JsonDeserializer::ElementDataResult JsonDeserializer::FindElementByNameCrc(SerializeContext& serializeContext, 
        void* object, const SerializeContext::ClassData& classData, const Crc32 nameCrc)
    {
        // The class data stores base class element information first in the set of m_elements
        // We use a reverse iterator to ensure that derived class data takes precedence over base classes' data
        // for the case of naming conflicts in the serialized data between base and derived classes
        for (auto elementData = classData.m_elements.crbegin(); elementData != classData.m_elements.crend(); ++elementData)
        {
            if (elementData->m_nameCrc == nameCrc)
            {
                ElementDataResult result;
                result.m_data = reinterpret_cast<void*>(reinterpret_cast<char*>(object) + elementData->m_offset);
                result.m_info = &*elementData;
                result.m_found = true;
                return result;
            }
                
            if (elementData->m_flags & SerializeContext::ClassElement::Flags::FLG_BASE_CLASS)
            {
                const SerializeContext::ClassData* baseClassData = serializeContext.FindClassData(elementData->m_typeId);
                if (baseClassData)
                {
                    void* subclassOffset = reinterpret_cast<char*>(object) + elementData->m_offset;
                    ElementDataResult foundElementData = FindElementByNameCrc(
                        serializeContext, subclassOffset, *baseClassData, nameCrc);
                    if (foundElementData.m_found)
                    {
                        return foundElementData;
                    }
                }
            }
        }

        return ElementDataResult{};
    }

    size_t JsonDeserializer::CountElements(SerializeContext& serializeContext, const SerializeContext::ClassData& classData)
    {
        size_t count = 0;
        for (auto& element : classData.m_elements)
        {
            if (element.m_flags & SerializeContext::ClassElement::Flags::FLG_BASE_CLASS)
            {
                const SerializeContext::ClassData* baseClassData = serializeContext.FindClassData(element.m_typeId);
                if (baseClassData)
                {
                    count += CountElements(serializeContext, *baseClassData);
                }
            }
            else
            {
                count++;
            }
        }
        return count;
    }

    bool JsonDeserializer::IsExplicitDefault(const rapidjson::Value& value)
    {
        return value.IsObject() && value.MemberCount() == 0;
    }
} // namespace AZ