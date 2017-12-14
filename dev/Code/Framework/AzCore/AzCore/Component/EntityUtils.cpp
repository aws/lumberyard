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

        struct StackDataType
        {
            const SerializeContext::ClassData* m_classData;
            const SerializeContext::ClassElement* m_elementData;
            void* m_dataPtr;
            bool m_isModifiedContainer;
        };

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
