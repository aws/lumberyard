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

#include <AzCore/Slice/SliceMetadataInfoComponent.h>

namespace AZ
{

    SliceMetadataInfoComponent::SliceMetadataInfoComponent(bool persistent)
        : m_persistent(persistent)
    {
    }

    SliceMetadataInfoComponent::~SliceMetadataInfoComponent()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void SliceMetadataInfoComponent::Activate()
    {
        AZ::SliceMetadataInfoNotificationBus::Bind(m_notificationBus, m_entity->GetId());

        // This bus provides an interface for keeping the component synchronized
        SliceMetadataInfoManipulationBus::Handler::BusConnect(GetEntityId());
        // General information requests for the component
        SliceMetadataInfoRequestBus::Handler::BusConnect(GetEntityId());
        // Application Events used to maintain synchronization
        ComponentApplicationEventBus::Handler::BusConnect();
    }

    void SliceMetadataInfoComponent::Deactivate()
    {
        SliceMetadataInfoManipulationBus::Handler::BusDisconnect();
        SliceMetadataInfoRequestBus::Handler::BusDisconnect();
        ComponentApplicationEventBus::Handler::BusDisconnect();
        m_notificationBus = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    // SliceMetadataInfoManipulationBus
    void SliceMetadataInfoComponent::AddChildMetadataEntity(EntityId childEntityId)
    {
        AZ_Assert(m_children.find(childEntityId) == m_children.end(), "Attempt to establish a child connection that already exists.");
        m_children.insert(childEntityId);
    }

    void SliceMetadataInfoComponent::RemoveChildMetadataEntity(EntityId childEntityId)
    {
        AZ_Assert(m_children.find(childEntityId) != m_children.end(), "Entity Specified is not an existing child metadata entity");
        m_children.erase(childEntityId);

        CheckDependencyCount();
    }

    void SliceMetadataInfoComponent::SetParentMetadataEntity(EntityId parentEntityId)
    {
        m_parent = parentEntityId;
    }

    void SliceMetadataInfoComponent::AddAssociatedEntity(EntityId associatedEntityId)
    {
        if (associatedEntityId.IsValid())
        {
            m_associatedEntities.insert(associatedEntityId);
        }
    }

    void SliceMetadataInfoComponent::RemoveAssociatedEntity(EntityId associatedEntityId)
    {
        if (!associatedEntityId.IsValid())
        {
            return;
        }

        AZ_Assert(m_associatedEntities.find(associatedEntityId) != m_associatedEntities.end(), "Entity Specified is not an existing associated editor entity");
        m_associatedEntities.erase(associatedEntityId);

        // During asset processing and level loading, the component may not yet have an
        // associated entity. These cases occur when loading and processing older levels,
        // during cloning of intermediate slice instantiation. In these cases, skipping
        // dependency checking is okay because the components on the final cloned entities
        // are attached to valid entities will be checked properly during instantiation.
        if (GetEntity())
        {
            CheckDependencyCount();
        }
    }

    void SliceMetadataInfoComponent::MarkAsPersistent(bool persistent)
    {
        m_persistent = persistent;
    }

    //////////////////////////////////////////////////////////////////////////
    // ComponentApplicationEventBus
    void SliceMetadataInfoComponent::OnEntityRemoved(const AZ::EntityId& entityId)
    {
        if (!entityId.IsValid())
        {
            return;
        }

        // The given ID may be either a dependent editor entity or a child metadata entity. Remove it from
        // either list if it exists.
        m_associatedEntities.erase(entityId);
        m_children.erase(entityId);
        CheckDependencyCount();
    }

    //////////////////////////////////////////////////////////////////////////
    // SliceMetadataInfoRequestBus
    bool SliceMetadataInfoComponent::IsAssociated(EntityId entityId)
    {
        return m_associatedEntities.find(entityId) != m_associatedEntities.end();
    }

    void SliceMetadataInfoComponent::GetAssociatedEntities(AZStd::unordered_set<EntityId>& associatedEntityIds)
    {
        associatedEntityIds.insert(m_associatedEntities.begin(), m_associatedEntities.end());
    }

    EntityId SliceMetadataInfoComponent::GetParentId()
    {
        return m_parent;
    }

    void SliceMetadataInfoComponent::GetChildIDs(AZStd::unordered_set<EntityId>& childEntityIds)
    {
        childEntityIds.insert(m_children.begin(), m_children.end());
    }

    size_t SliceMetadataInfoComponent::GetAssociationCount()
    {
        return m_children.size() + m_associatedEntities.size();
    }

    //////////////////////////////////////////////////////////////////////////

    void SliceMetadataInfoComponent::Reflect(AZ::ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SliceMetadataInfoComponent, Component>()->
                Version(1)->
                Field("AssociatedIds", &SliceMetadataInfoComponent::m_associatedEntities)->
                Field("ParentId", &SliceMetadataInfoComponent::m_parent)->
                Field("ChildrenIds", &SliceMetadataInfoComponent::m_children)->
                Field("PersistenceFlag", &SliceMetadataInfoComponent::m_persistent);
        }
    }

    void SliceMetadataInfoComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SliceMetadataInfoService", 0xdaaa6bb4));
    }

    void SliceMetadataInfoComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SliceMetadataInfoService", 0xdaaa6bb4));
    }

    void SliceMetadataInfoComponent::CheckDependencyCount()
    {
        // When the dependency count reaches zero, non-persistent metadata entities are no longer useful and need to be removed from the metadata entity context.
        // While removing all entities belonging to any non-nested slice will trigger the destruction of that slice, the deletion of its metadata entity,
        // and the removal of the metadata entity from the metadata entity context, nested slices are only properly destroyed when their non-nested ancestor
        // is destroyed. To ensure that the context knows the metadata entity is no longer relevant, we dispatch an OnMetadataDependenciesRemoved event.
        bool isActive = GetEntity()->GetState() == AZ::Entity::State::ES_ACTIVE;
        bool hasNoDependencies = m_associatedEntities.empty() && m_children.empty();
        if (isActive && !m_persistent && hasNoDependencies)
        {
            SliceMetadataInfoNotificationBus::Event(m_notificationBus, &SliceMetadataInfoNotificationBus::Events::OnMetadataDependenciesRemoved);
        }
    }

} // Namespace AZ