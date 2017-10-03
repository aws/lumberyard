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
#ifndef AZCORE_ENTITY_UTILS_H
#define AZCORE_ENTITY_UTILS_H

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    namespace EntityUtils
    {
        /// Return default application serialization context.
        SerializeContext* GetApplicationSerializeContext();

        /**
         * Serializable container for entities, useful for data patching and serializer enumeration.
         * Does not assume ownership of stored entities.
         */
        struct SerializableEntityContainer
        {
            AZ_CLASS_ALLOCATOR(SerializableEntityContainer, SystemAllocator, 0);
            AZ_TYPE_INFO(SerializableEntityContainer, "{E98CF1B5-6B72-46C5-AB87-3DB85FD1B48D}");

            AZStd::vector<AZ::Entity*> m_entities;
        };

        /**
         * Reflect entity utils data types.
         */
        void Reflect(ReflectContext* context);

        /**
        * Given key, return the EntityId to map to
        */
        typedef AZStd::function< EntityId(const EntityId& /*originalId*/, bool /*isEntityId*/) > EntityIdMapper;
        typedef AZStd::function< void(const AZ::EntityId& /*id*/, bool /*isEntityId*/, const AZ::SerializeContext::ClassElement* /*elementData*/) > EntityIdVisitor;

        /**
        * Enumerates all entity references in the object's hierarchy and remaps them with the result returned by mapper.
        * "entity reference" is considered any EntityId type variable, except Entity::m_id. Entity class had only one EntityId member (m_id)
        * which represents an actual entity id, every other stored EntityId is considered a reference and will be remapped if when needed.
        * What if I want to store and EntityId that should never be remapped. You should NOT do that, as we clone entities a lot and remap all
        * references to maintain the same behavior. If you really HAVE TO store entity id which is not no be remapped, just use "u64" type variable.
        */
        unsigned int ReplaceEntityRefs(void* classPtr, const Uuid& classUuid, const EntityIdMapper& mapper, SerializeContext* context = nullptr);

        template<class T>
        unsigned int ReplaceEntityRefs(T* classPtr, const EntityIdMapper& mapper, SerializeContext* context = nullptr)
        {
            return ReplaceEntityRefs(classPtr, SerializeTypeInfo<T>::GetUuid(classPtr), mapper, context);
        }

        /**
        * Enumerates all entity references in the object's hierarchy and invokes the specified visitor.
        * \param classPtr - the object instance to enumerate.
        * \param classUuid - the object instance's type Id.
        * \param visitor - the visitor callback to be invoked for Entity::m_id (isEntityId==true) or reflected EntityId field, aka reference (isEntityId==false).
        */
        void EnumerateEntityIds(const void* classPtr, const Uuid& classUuid, const EntityIdVisitor& visitor, SerializeContext* context = nullptr);

        template<class T>
        void EnumerateEntityIds(const T* classPtr, const EntityIdVisitor& visitor, SerializeContext* context = nullptr)
        {
            EnumerateEntityIds(classPtr, SerializeTypeInfo<T>::GetUuid(classPtr), visitor, context);
        }

        /**
         * Replaces all entity ids in the object's hierarchy and remaps them with the result returned by the mapper.
         * "entity id" is only Entity::m_id every other EntityId variable is considered a reference \ref ReplaceEntityRefs
         */
        unsigned int ReplaceEntityIds(void* classPtr, const Uuid& classUuid, const EntityIdMapper& mapper, SerializeContext* context = nullptr);

        template<class T>
        unsigned int ReplaceEntityIds(T* classPtr, const EntityIdMapper& mapper, SerializeContext* context = nullptr)
        {
            return ReplaceEntityIds(classPtr, SerializeTypeInfo<T>::GetUuid(classPtr), mapper, context);
        }

        /**
         * Replaces all EntityId objects (entity ids and entity refs) in the object's hierarchy.
         */
        unsigned int ReplaceEntityIdsAndEntityRefs(void* classPtr, const Uuid& classUuid, const EntityIdMapper& mapper, SerializeContext* context = nullptr);

        template<class T>
        unsigned int ReplaceEntityIdsAndEntityRefs(T* classPtr, const EntityIdMapper& mapper, SerializeContext* context = nullptr)
        {
            return ReplaceEntityIdsAndEntityRefs(classPtr, SerializeTypeInfo<T>::GetUuid(classPtr), mapper, context);
        }

        /**
        * Generate new entity ids, and remap all reference
        */
        template<class T, class MapType>
        void GenerateNewIdsAndFixRefs(T* object, MapType& newIdMap, SerializeContext* context = nullptr)
        {
            if (!context)
            {
                context = GetApplicationSerializeContext();
                if (!context)
                {
                    AZ_Error("Serialization", false, "Not serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                    return;
                }
            }

            ReplaceEntityIds(
                object,
                [&newIdMap](const EntityId& originalId, bool /*isEntityId*/) -> EntityId
                {
                    auto it = newIdMap.insert(AZStd::make_pair(originalId, Entity::MakeId()));
                    return it.first->second;
                }, context);

            ReplaceEntityRefs(
                object,
                [&newIdMap](const EntityId& originalId, bool /*isEntityId*/) -> EntityId
                {
                    auto findIt = newIdMap.find(originalId);
                    if (findIt == newIdMap.end())
                    {
                        return originalId; // entityId is not being remapped
                    }
                    else
                    {
                        return findIt->second; // return the remapped id
                    }
                }, context);
        }

        /**
         * Clone the object T, generate new ids for all entities in the hierarchy, and fix all entityId.
         */
        template<class T, class Map>
        T* CloneObjectAndFixEntities(const T* object, Map& newIdMap, SerializeContext* context = nullptr)
        {
            if (!context)
            {
                context = GetApplicationSerializeContext();
                if (!context)
                {
                    AZ_Error("Serialization", false, "Not serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                    return nullptr;
                }
            }

            T* clonedObject = context->CloneObject(object);

            GenerateNewIdsAndFixRefs(clonedObject, newIdMap, context);

            return clonedObject;
        }

        /**
         * Clones entities, generates new entityIds \ref ReplaceEntityIds and fixes all entity references \ref ReplaceEntityRefs
         */
        template<class InputIterator, class OutputIterator, class TempAllocatorType >
        void CloneAndFixEntities(InputIterator first, InputIterator last, OutputIterator result, const TempAllocatorType& allocator, SerializeContext* context = nullptr)
        {
            for (; first != last; ++first, ++result)
            {
                *result = CloneObjectAndFixEntities(*first, allocator, context);
            }
        }

        template<class InputIterator, class OutputIterator>
        void CloneAndFixEntities(InputIterator first, InputIterator last, OutputIterator result, SerializeContext* context = nullptr)
        {
            for (; first != last; ++first, ++result)
            {
                *result = CloneObjectAndFixEntities(*first, context);
            }
        }


        /// Return the first component that is either of the specified type or derive from the specified type
        Component* FindFirstDerivedComponent(Entity* entity, const Uuid& typeId);

        /// Return the first component that is either of the specified type or derive from the specified type
        template<class ComponentType>
        inline ComponentType* FindFirstDerivedComponent(Entity* entity)
        {
            return azrtti_cast<ComponentType*>(FindFirstDerivedComponent(entity, AzTypeInfo<ComponentType>::Uuid()));
        }

        /// Return a vector of all components that are either of the specified type or derive from the specified type
        Entity::ComponentArrayType FindDerivedComponents(Entity* entity, const Uuid& typeId);

        /// Return a vector of all components that are either of the specified type or derive from the specified type
        template<class ComponentType>
        inline AZStd::vector<ComponentType*> FindDerivedComponents(Entity* entity)
        {
            AZStd::vector<ComponentType*> result;
            for (AZ::Component* component : entity->GetComponents())
            {
                auto derivedComponent = azrtti_cast<ComponentType*>(component);
                if (derivedComponent)
                {
                    result.push_back(derivedComponent);
                }
            }
            return result;
        }
    } // namespace EntityUtils
}   // namespace AZ

#endif  // AZCORE_ENTITY_UTILS_H
#pragma once
