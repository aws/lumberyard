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
#include <AzCore/std/containers/fixed_vector.h>

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

        Component* FindFirstDerivedComponent(EntityId entityId, const Uuid& typeId)
        {
            Entity* entity{};
            ComponentApplicationBus::BroadcastResult(entity, &ComponentApplicationRequests::FindEntity, entityId);
            return entity ? FindFirstDerivedComponent(entity, typeId) : nullptr;
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

        Entity::ComponentArrayType FindDerivedComponents(EntityId entityId, const Uuid& typeId)
        {
            Entity* entity{};
            ComponentApplicationBus::BroadcastResult(entity, &ComponentApplicationRequests::FindEntity, entityId);
            return entity ? FindDerivedComponents(entity, typeId) : Entity::ComponentArrayType();
        }

       
        bool CheckDeclaresSerializeBaseClass(SerializeContext* context, const TypeId& typeToFind, const TypeId& typeToExamine)
        {
            AZ_Assert(context, "CheckDeclaresSerializeBaseClass called with no serialize context.");
            if (!context)
            {
                return false;
            }

            AZStd::fixed_vector<TypeId, 64> knownBaseClasses = { typeToExamine };  // avoid allocating heap here if possible.  64 types are 64*sizeof(Uuid) which is only 1k.
            bool foundBaseClass = false;
            auto baseClassVisitorFn = [&typeToFind, &foundBaseClass, &knownBaseClasses](const AZ::SerializeContext::ClassData* reflectedBase, const TypeId& /*rttiBase*/)
            {
                // SerializeContext::EnumerateBase() iterates:
                // - the immediate base classes reflected to SerializeContext.
                // - then it iterates the entire tree of base classes from the AZ_RTTI info.
                // We only care about base classes reflected to SerializeContext,
                // so stop iterating once we stop receiving SerializeContext::ClassData*.
                if (!reflectedBase)
                {
                    return false;
                }

                // Stop iterating if we've found what we are looking for!
                if (reflectedBase->m_typeId == typeToFind)
                {
                    foundBaseClass = true;
                    return false;
                }

                // EnumerateBase() only iterates 1 level of reflected base classes.
                // if we haven't found what we are looking for yet, push base classes into queue for further exploration.
                if (AZStd::find(knownBaseClasses.begin(), knownBaseClasses.end(), reflectedBase->m_typeId) == knownBaseClasses.end())
                {
                    if (knownBaseClasses.size() == 64)
                    {
                        // this should be pretty unlikely since a single class would have to have many other classes in its heirarchy
                        // and it'd all have to be basically in one layer, as we are popping as we explore.
                        AZ_WarningOnce("EntityUtils", false, "While trying to find a base class, all available slots were consumed.  consider increasing the size of knownBaseClasses.\n");
                        // we cannot continue any futher, assume we did not find it.
                        return false;
                    }
                    knownBaseClasses.push_back(reflectedBase->m_typeId);
                }

                return true; // keep iterating
            };

            while (!knownBaseClasses.empty())
            {
                TypeId toExamine = knownBaseClasses.back();
                knownBaseClasses.pop_back();

                context->EnumerateBase(baseClassVisitorFn, toExamine);
            }

            return foundBaseClass;
        }
    } // namespace EntityUtils
}   // namespace AZ

#endif  // AZ_UNITY_BUILD
