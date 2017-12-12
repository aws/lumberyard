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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>

namespace AZ
{
    namespace EntityUtils
    {
        //=========================================================================
        // Reflect
        //=========================================================================
        void Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SerializableEntityContainer>()->
                    Version(1)->
                    Field("Entities", &SerializableEntityContainer::m_entities);
            }
        }
        // Type IDs of types of special interest (we copy the string as it should never change)
        static const AZ::Uuid s_pairID = AZ::Uuid::CreateString("{9F3F5302-3390-407a-A6F7-2E011E3BB686}"); // AZStd::pair 

        struct StackDataType
        {
            const SerializeContext::ClassData* m_classData;
            const SerializeContext::ClassElement* m_elementData;
            void* m_dataPtr;
            bool m_isModifiedContainer;
        };

        //=========================================================================
        // ReplaceEntityIdType
        // [4/16/2014]
        //=========================================================================
        unsigned int ReplaceEntityIdType(void* classPtr, const Uuid& classUuid, const EntityIdMapper& mapper, SerializeContext* context, bool isReplaceAll, bool isReplaceEntityIds)
        {
            if (!context)
            {
                context = GetApplicationSerializeContext();
                if (!context)
                {
                    AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                    return 0;
                }
            }

            unsigned int replaced = 0;
            AZStd::vector<StackDataType> parentStack;
            parentStack.reserve(30);
            auto beginCB = [&](void* ptr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* elementData) -> bool
                {
                    if (classData->m_typeId == SerializeTypeInfo<EntityId>::GetUuid())
                    {
                        // determine if this is entity ref or just entityId (please refer to the function documentation for more info)
                        bool isEntityId = false;
                        if (!parentStack.empty() && parentStack.back().m_classData->m_typeId == SerializeTypeInfo<Entity>::GetUuid())
                        {
                            // our parent in the entity (currently entity has only one EntityId member, but we can check the offset for future proof
                            AZ_Assert(elementData && strcmp(elementData->m_name, "Id") == 0, "class Entity, should have only ONE EntityId member, the actual entity id!");
                            isEntityId = true;
                        }

                        if (isReplaceAll || isReplaceEntityIds == isEntityId)
                        {
                            EntityId* entityIdPtr;
                            if (elementData->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                            {
                                entityIdPtr = *reinterpret_cast<EntityId**>(ptr);
                            }
                            else
                            {
                                entityIdPtr = reinterpret_cast<EntityId*>(ptr);
                            }
                            EntityId originalId = *entityIdPtr;
                            EntityId newId = mapper(originalId, isEntityId);
                            if (newId != originalId)
                            {
                                // notify the first non container parent about the modification
                                int parentToNotifyAboutWrite = parentStack.empty() ? -1 : (int)parentStack.size() - 1;

                                if (parentStack.size() >= 1)
                                {
                                    // check when the ID is in a container that will need to change as a result of the EntityID changing, keep in mind that is ONLY 
                                    // covering basic cases and internal containers. We can't guess if EntityID is used in a complex function that affects the container
                                    // or any other side effect. If this become the case, we can Reflect a function to handle when we remap EntityID (or any field) 
                                    // reflect it during the serialization and react anytime we change the content on the entity ID, via the OnWriteBegin/OnWriteEnd events
                                    StackDataType& parentInfo = parentStack.back();
                                    if (parentInfo.m_classData->m_typeId == s_pairID)
                                    {
                                        if (parentStack.size() >= 2) // check if the pair is part of a container
                                        {
                                            StackDataType& pairContainerInfo = parentStack[parentStack.size() - 2];
                                            if (pairContainerInfo.m_classData->m_container)
                                            {
                                                // make sure the EntityID is the first element in the pair, as this is what associative containers care about, the rest is a payload
                                                // we can skip this check, but it helps avoiding extra work on a high level)
                                                if (parentInfo.m_elementData->m_genericClassInfo->GetTemplatedTypeId(0) == classData->m_typeId) 
                                                {
                                                    pairContainerInfo.m_isModifiedContainer = true; // we will modify the container
                                                    parentToNotifyAboutWrite = parentStack.size() >= 3 ? (int)parentStack.size() - 3 : -1; // if the container has a parent
                                                }
                                            }
                                        }
                                    }
                                    else if (parentInfo.m_classData->m_container)
                                    {
                                        parentInfo.m_isModifiedContainer = true; // we will modify this container
                                        parentToNotifyAboutWrite = parentStack.size() >= 2 ? (int)parentStack.size() - 2 : -1; // if the container has a parent
                                    }
                                }
                                
                                if (parentToNotifyAboutWrite != -1 && parentStack[parentToNotifyAboutWrite].m_classData->m_eventHandler)
                                {
                                    parentStack[parentToNotifyAboutWrite].m_classData->m_eventHandler->OnWriteBegin(parentStack[parentToNotifyAboutWrite].m_dataPtr);
                                }

                                *entityIdPtr = newId;

                                if (parentToNotifyAboutWrite != -1 && parentStack[parentToNotifyAboutWrite].m_classData->m_eventHandler)
                                {
                                    parentStack[parentToNotifyAboutWrite].m_classData->m_eventHandler->OnWriteEnd(parentStack[parentToNotifyAboutWrite].m_dataPtr);
                                }

                                replaced++;
                            }
                        }
                    }

                    parentStack.push_back({ classData, elementData, ptr, false});
                    return true;
                };

            auto endCB = [&]() -> bool
                {
                    if (parentStack.back().m_isModifiedContainer)
                    {
                        parentStack.back().m_classData->m_container->ElementsUpdated(parentStack.back().m_dataPtr);
                    }
                    parentStack.pop_back();
                    return true;
                };

            context->EnumerateInstance(
                classPtr,
                classUuid,
                beginCB,
                endCB,
                SerializeContext::ENUM_ACCESS_FOR_WRITE,
                nullptr,
                nullptr,
                nullptr
                );
            return replaced;
        }

        //=========================================================================
        // ReplaceEntityRefs
        //=========================================================================
        unsigned int ReplaceEntityRefs(void* classPtr, const Uuid& classUuid, const EntityIdMapper& mapper, SerializeContext* context)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            return ReplaceEntityIdType(classPtr, classUuid, mapper, context, false, false);
        }

        //=========================================================================
        // EnumerateEntityIds
        //=========================================================================
        void EnumerateEntityIds(const void* classPtr, const Uuid& classUuid, const EntityIdVisitor& visitor, SerializeContext* context)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            if (!context)
            {
                context = GetApplicationSerializeContext();
                if (!context)
                {
                    AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                    return;
                }
            }
            AZStd::vector<const SerializeContext::ClassData*> parentStack;
            parentStack.reserve(30);
            auto beginCB = [ &](void* ptr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* elementData) -> bool
                {
                    (void)elementData;

                    if (classData->m_typeId == SerializeTypeInfo<EntityId>::GetUuid())
                    {
                        // determine if this is entity ref or just entityId (please refer to the function documentation for more info)
                        bool isEntityId = false;
                        if (!parentStack.empty() && parentStack.back()->m_typeId == SerializeTypeInfo<Entity>::GetUuid())
                        {
                            // our parent in the entity (currently entity has only one EntityId member, but we can check the offset for future proof
                            AZ_Assert(elementData && strcmp(elementData->m_name, "Id") == 0, "class Entity, should have only ONE EntityId member, the actual entity id!");
                            isEntityId = true;
                        }

                        EntityId* entityIdPtr = (elementData->m_flags & SerializeContext::ClassElement::FLG_POINTER) ?
                            *reinterpret_cast<EntityId**>(ptr) : reinterpret_cast<EntityId*>(ptr);
                        visitor(*entityIdPtr, isEntityId, elementData);
                    }

                    parentStack.push_back(classData);
                    return true;
                };

            auto endCB = [ &]() -> bool
                {
                    parentStack.pop_back();
                    return true;
                };

            context->EnumerateInstanceConst(
                classPtr,
                classUuid,
                beginCB,
                endCB,
                SerializeContext::ENUM_ACCESS_FOR_READ,
                nullptr,
                nullptr,
                nullptr
                );
        }

        //=========================================================================
        // ReplaceEntityIds
        //=========================================================================
        unsigned int ReplaceEntityIds(void* classPtr, const Uuid& classUuid, const EntityIdMapper& mapper, SerializeContext* context)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            return ReplaceEntityIdType(classPtr, classUuid, mapper, context, false, true);
        }

        //=========================================================================
        // ReplaceEntityIds
        //=========================================================================
        unsigned int ReplaceEntityIdsAndEntityRefs(void* classPtr, const Uuid& classUuid, const EntityIdMapper& mapper, SerializeContext* context)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            unsigned int replaced = ReplaceEntityIds(classPtr, classUuid, mapper, context);
            replaced += ReplaceEntityRefs(classPtr, classUuid, mapper, context);
            return replaced;
        }


        //=========================================================================
        // GetApplicationSerializeContext
        //=========================================================================
        SerializeContext* GetApplicationSerializeContext()
        {
            SerializeContext* context = nullptr;
            EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
            return context;
        }

        //=========================================================================
        // FindFirstDerivedComponent
        //=========================================================================
        Component* FindFirstDerivedComponent(Entity* entity, const Uuid& typeId)
        {
            for (AZ::Component* component : entity->GetComponents())
            {
                if (azrtti_istypeof(typeId, component))
                {
                    return component;
                }
            }
            return nullptr;
        }

        //=========================================================================
        // FindDerivedComponents
        //=========================================================================
        Entity::ComponentArrayType FindDerivedComponents(Entity* entity, const Uuid& typeId)
        {
            Entity::ComponentArrayType result;
            for (AZ::Component* component : entity->GetComponents())
            {
                if (azrtti_istypeof(typeId, component))
                {
                    result.push_back(component);
                }
            }
            return result;
        }

    } // namespace EntityUtils
}   // namespace AZ

#endif  // AZ_UNITY_BUILD
