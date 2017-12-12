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

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AzFramework
{
    /*!
    * Behaves like an \ref AZ::EntityId but comes with a serializer that will deep-copy the referenced entity, as if an
    * \ref AZ::Entity* was held.
    */
    class EntityReference
        : protected AZ::ComponentApplicationEventBus::Handler
    {
    public:
        AZ_TYPE_INFO(EntityReference, "{DFBE586C-1030-47DE-A3A1-DCEABD26A00B}");
        AZ_CLASS_ALLOCATOR(EntityReference, AZ::SystemAllocator, 0)

        static void Reflect(AZ::ReflectContext* context);

        explicit EntityReference(AZ::EntityId id = AZ::EntityId())
            : m_entityId(id)
        { 
            AZ::ComponentApplicationEventBus::Handler::BusConnect();
            AZ::ComponentApplicationBus::BroadcastResult(m_entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
        }

        explicit EntityReference(AZ::Entity* entity)
            : m_entityId(entity ? entity->GetId() : AZ::EntityId{})
            , m_entity(entity)
        {
            AZ::ComponentApplicationEventBus::Handler::BusConnect();
        }

        ~EntityReference()
        {
            AZ::ComponentApplicationEventBus::Handler::BusDisconnect();
        }

        EntityReference& operator=(AZ::EntityId entityId)
        {
            m_entityId = entityId;
            AZ::ComponentApplicationBus::BroadcastResult(m_entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
            return *this;
        }

        EntityReference& operator=(AZ::Entity* entity)
        {
            if (entity)
            {
                m_entityId = entity->GetId();
                m_entity = entity;
            }
            return *this;
        }

        operator AZ::EntityId() const { return m_entityId; }

        bool operator==(const EntityReference& other) const { return m_entityId == other.m_entityId; }
        bool operator==(AZ::EntityId entityId) const { return m_entityId == entityId; }
        bool operator!=(AZ::EntityId entityId) const { return m_entityId != entityId; }
        friend bool operator==(AZ::EntityId entityId, const EntityReference& ref) { return ref.m_entityId == entityId; }
        friend bool operator!=(AZ::EntityId entityId, const EntityReference& ref) { return ref.m_entityId != entityId; }

        AZ::EntityId GetEntityId() const { return m_entityId; }
        AZ::Entity* GetEntity() const
        {
            AZ_Assert(!m_entity || m_entity->GetId() == m_entityId, "EntityReference m_entityId member differs from m_entity::m_id member."
                " This normally occurs when the Entity object has been remapped without this EntityReference being part of the remapping structure");
            return m_entity;
        }

        //! ComponentApplicationEventBus
        /// Notifies listeners that an entity has been added to the application.
        void OnEntityAdded(AZ::Entity* entity) override
        {
            if (entity->GetId() == m_entityId)
            {
                m_entity = entity;
            }
        }

        /// Notifies listeners that an entity has been removed to the application.
        void OnEntityRemoved(const AZ::EntityId& entityId) override
        {
            if (entityId == m_entityId)
            {
                m_entity = nullptr;
            }
        }

    private:
        AZ::EntityId m_entityId;
        AZ::Entity* m_entity = nullptr;
        friend class EntityReferenceEventHandler;
    };

    AZStd::vector<AZ::EntityId> ResolveEntityReferences(const AZStd::vector<EntityReference>& source);

    AZStd::vector<EntityReference> CreateEntityReferences(const AZStd::vector<AZ::EntityId>& source);
}

namespace AZStd
{
    template<>
    struct hash<AzFramework::EntityReference>
    {
        using argument_type = AzFramework::EntityReference;
        using result_type = AZStd::size_t;
        AZ_FORCE_INLINE size_t operator()(const argument_type& ref) const
        {
            AZStd::hash<AZ::EntityId> hasher;
            return hasher(ref.GetEntityId());
        }

    };
}