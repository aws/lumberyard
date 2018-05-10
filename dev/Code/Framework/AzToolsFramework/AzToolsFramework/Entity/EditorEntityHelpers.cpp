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
#include "EditorEntityHelpers.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>

#include <AzCore/std/sort.h>

namespace AzToolsFramework
{
    AZ::Entity* GetEntityById(const AZ::EntityId& entityId)
    {
        AZ_Assert(entityId.IsValid(), "Invalid EntityId provided.");
        if (!entityId.IsValid())
        {
            return nullptr;
        }

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        return entity;
    }

    void GetAllComponentsForEntity(const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        if (entity)
        {
            //build a set of all active and pending components associated with the entity
            componentsOnEntity.insert(componentsOnEntity.end(), entity->GetComponents().begin(), entity->GetComponents().end());
            EditorPendingCompositionRequestBus::Event(entity->GetId(), &EditorPendingCompositionRequests::GetPendingComponents, componentsOnEntity);
            EditorDisabledCompositionRequestBus::Event(entity->GetId(), &EditorDisabledCompositionRequests::GetDisabledComponents, componentsOnEntity);
        }
    }

    void GetAllComponentsForEntity(const AZ::EntityId& entityId, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        GetAllComponentsForEntity(GetEntity(entityId), componentsOnEntity);
    }

    AZ::Uuid GetComponentTypeId(const AZ::Component* component)
    {
        return GetUnderlyingComponentType(*component);
    }

    const AZ::SerializeContext::ClassData* GetComponentClassData(const AZ::Component* component)
    {
        return GetComponentClassDataForType(GetComponentTypeId(component));
    }

    const AZ::SerializeContext::ClassData* GetComponentClassDataForType(const AZ::Uuid& componentTypeId)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        const AZ::SerializeContext::ClassData* componentClassData = serializeContext->FindClassData(componentTypeId);
        return componentClassData;
    }

    AZStd::string GetFriendlyComponentName(const AZ::Component* component)
    {
        auto className = component->RTTI_GetTypeName();
        auto classData = GetComponentClassData(component);
        if (!classData)
        {
            return className;
        }

        if (!classData->m_editData)
        {
            return classData->m_name;
        }

        if (auto editorData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
        {
            if (auto nameAttribute = editorData->FindAttribute(AZ::Edit::Attributes::NameLabelOverride))
            {
                AZStd::string name;
                AZ::AttributeReader nameReader(const_cast<AZ::Component*>(component), nameAttribute);
                nameReader.Read<AZStd::string>(name);
                return name;
            }
        }

        return classData->m_editData->m_name;
    }

    const char* GetFriendlyComponentDescription(const AZ::Component* component)
    {
        auto classData = GetComponentClassData(component);
        if (!classData || !classData->m_editData)
        {
            return "";
        }
        return classData->m_editData->m_description;
    }

    AZ::ComponentDescriptor* GetComponentDescriptor(const AZ::Component* component)
    {
        AZ::ComponentDescriptor* componentDescriptor = nullptr;
        AZ::ComponentDescriptorBus::EventResult(componentDescriptor, component->RTTI_GetType(), &AZ::ComponentDescriptor::GetDescriptor);
        return componentDescriptor;
    }

    Components::EditorComponentDescriptor* GetEditorComponentDescriptor(const AZ::Component* component)
    {
        Components::EditorComponentDescriptor* editorComponentDescriptor = nullptr;
        AzToolsFramework::Components::EditorComponentDescriptorBus::EventResult(editorComponentDescriptor, component->RTTI_GetType(), &AzToolsFramework::Components::EditorComponentDescriptor::GetEditorDescriptor);
        return editorComponentDescriptor;
    }

    Components::EditorComponentBase* GetEditorComponent(AZ::Component* component)
    {
        auto editorComponentBaseComponent = azrtti_cast<Components::EditorComponentBase*>(component);
        AZ_Assert(editorComponentBaseComponent, "Editor component does not derive from EditorComponentBase");
        return editorComponentBaseComponent;
    }

    bool ShouldInspectorShowComponent(const AZ::Component* component)
    {
        if (!component)
        {
            return false;
        }

        const AZ::SerializeContext::ClassData* classData = GetComponentClassData(component);

        // Don't show components without edit data
        if (!classData || !classData->m_editData)
        {
            return false;
        }

        // Don't show components that are set to invisible.
        if (const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
        {
            if (AZ::Edit::Attribute* visibilityAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Visibility))
            {
                PropertyAttributeReader reader(const_cast<AZ::Component*>(component), visibilityAttribute);
                AZ::u32 visibilityValue;
                if (reader.Read<AZ::u32>(visibilityValue))
                {
                    if (visibilityValue == AZ::Edit::PropertyVisibility::Hide)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    AZ::EntityId GetEntityIdForSortInfo(const AZ::EntityId parentId)
    {
        AZ::EntityId sortEntityId = parentId;
        if (!sortEntityId.IsValid())
        {
            AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequestBus::Events::GetEditorEntityContextId);
            AZ::SliceComponent* rootSliceComponent = nullptr;
            AzFramework::EntityContextRequestBus::EventResult(rootSliceComponent, editorEntityContextId, &AzFramework::EntityContextRequestBus::Events::GetRootSlice);
            if (rootSliceComponent)
            {
                return rootSliceComponent->GetMetadataEntity()->GetId();
            }
        }
        return sortEntityId;
    }

    void AddEntityIdToSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, bool forceAddToBack)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        AZ::EntityId sortEntityId = GetEntityIdForSortInfo(parentId);

        bool success = false;
        EditorEntitySortRequestBus::EventResult(success, sortEntityId, &EditorEntitySortRequestBus::Events::AddChildEntity, childId, !parentId.IsValid() || forceAddToBack);
        if (success && parentId != sortEntityId)
        {
            EditorEntitySortNotificationBus::Event(parentId, &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }
    }

    void RecoverEntitySortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, AZ::u64 sortIndex)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        EntityOrderArray entityOrderArray;
        EditorEntitySortRequestBus::EventResult(entityOrderArray, GetEntityIdForSortInfo(parentId), &EditorEntitySortRequestBus::Events::GetChildEntityOrderArray);

        // Make sure the child entity isn't already in order array.
        auto sortIter = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), childId);
        if (sortIter != entityOrderArray.end())
        {
            entityOrderArray.erase(sortIter);
        }
        // Make sure we don't overwrite the bounds of our vector.
        if (sortIndex > entityOrderArray.size())
        {
            sortIndex = entityOrderArray.size();
        }
        entityOrderArray.insert(entityOrderArray.begin() + sortIndex, childId);
        // Push the final array back to the sort component
        AzToolsFramework::SetEntityChildOrder(parentId, entityOrderArray);
    }

    void RemoveEntityIdFromSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        AZ::EntityId sortEntityId = GetEntityIdForSortInfo(parentId);

        bool success = false;
        EditorEntitySortRequestBus::EventResult(success, sortEntityId, &EditorEntitySortRequestBus::Events::RemoveChildEntity, childId);
        if (success && parentId != sortEntityId)
        {
            EditorEntitySortNotificationBus::Event(parentId, &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }
    }

    void SetEntityChildOrder(const AZ::EntityId parentId, const EntityIdList& children)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        auto sortEntityId = GetEntityIdForSortInfo(parentId);

        bool success = false;
        EditorEntitySortRequestBus::EventResult(success, sortEntityId, &EditorEntitySortRequestBus::Events::SetChildEntityOrderArray, children);
        if (success && parentId != sortEntityId)
        {
            EditorEntitySortNotificationBus::Event(parentId, &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }
    }

    EntityIdList GetEntityChildOrder(const AZ::EntityId parentId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        EntityIdList children;
        EditorEntityInfoRequestBus::EventResult(children, parentId, &EditorEntityInfoRequestBus::Events::GetChildren);

        EntityIdList entityChildOrder;
        AZ::EntityId sortEntityId = GetEntityIdForSortInfo(parentId);
        EditorEntitySortRequestBus::EventResult(entityChildOrder, sortEntityId, &EditorEntitySortRequestBus::Events::GetChildEntityOrderArray);

        // Prune out any entries in the child order array that aren't currently known to be children
        entityChildOrder.erase(
            AZStd::remove_if(
                entityChildOrder.begin(), 
                entityChildOrder.end(), 
                [&children](const AZ::EntityId& entityId)
                {
                    // Return true to remove if entity id was not in the child array
                    return AZStd::find(children.begin(), children.end(), entityId) == children.end();
                }
            ),
            entityChildOrder.end()
        );

        return entityChildOrder;
    }

    //build an address based on depth and order of entities
    void GetEntityLocationInHierarchy(const AZ::EntityId& entityId, std::list<AZ::u64>& location)
    {
        if(entityId.IsValid())
        {
            AZ::EntityId parentId;
            EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);
            AZ::u64 entityOrder = 0;
            EditorEntitySortRequestBus::EventResult(entityOrder, GetEntityIdForSortInfo(parentId), &EditorEntitySortRequestBus::Events::GetChildEntityIndex, entityId);
            location.push_front(entityOrder);
            GetEntityLocationInHierarchy(parentId, location);
        }
    }

    //sort vector of entities by how they're arranged
    void SortEntitiesByLocationInHierarchy(EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        //cache locations for faster sort
        AZStd::unordered_map<AZ::EntityId, std::list<AZ::u64>> locations;
        for (auto entityId : entityIds)
        {
            GetEntityLocationInHierarchy(entityId, locations[entityId]);
        }
        AZStd::sort(entityIds.begin(), entityIds.end(), [&locations](const AZ::EntityId& e1, const AZ::EntityId& e2) {
            //sort by container contents
            return locations[e1] < locations[e2];
        });
    }

    bool EntityHasComponentOfType(const AZ::EntityId& entityId, AZ::Uuid componentType)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

        if (entity)
        {
            const AZ::Entity::ComponentArrayType components = entity->GetComponents();
            for (const AZ::Component* component : components)
            {
                if (component)
                {
                    if (GetUnderlyingComponentType(*component) == componentType)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool IsComponentWithServiceRegistered(const AZ::Crc32& serviceId)
    {
        bool result = false;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (serializeContext)
        {
            serializeContext->EnumerateDerived<AZ::Component>(
                [&](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                EBUS_EVENT_ID_RESULT(componentDescriptor, componentClass->m_typeId, AZ::ComponentDescriptorBus, GetDescriptor);
                if (componentDescriptor)
                {
                    AZ::ComponentDescriptor::DependencyArrayType providedServices;
                    componentDescriptor->GetProvidedServices(providedServices, nullptr);

                    if (AZStd::find(providedServices.begin(), providedServices.end(), serviceId) != providedServices.end())
                    {
                        result = true;
                        return false;
                    }
                }

                return true;
            }
            );
        }
        return result;
    }

} // namespace AzToolsFramework