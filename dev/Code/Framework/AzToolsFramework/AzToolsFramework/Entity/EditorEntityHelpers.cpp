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
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>

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
        AZ_Assert(entity, "Unable to find entity for EntityID %llu", entityId);
        return entity;
    }

    AZ::Entity::ComponentArrayType GetAllComponentsForEntity(const AZ::Entity* entity)
    {
        if (!entity)
        {
            return AZ::Entity::ComponentArrayType();
        }

        //build a set of all active and pending components associated with the entity
        AZ::Entity::ComponentArrayType componentsForEntity = entity->GetComponents();

        AZ::Entity::ComponentArrayType pendingComponents;
        EditorPendingCompositionRequestBus::EventResult(pendingComponents, entity->GetId(), &EditorPendingCompositionRequests::GetPendingComponents);
        componentsForEntity.insert(componentsForEntity.end(), pendingComponents.begin(), pendingComponents.end());

        AZ::Entity::ComponentArrayType disabledComponents;
        EditorDisabledCompositionRequestBus::EventResult(disabledComponents, entity->GetId(), &EditorDisabledCompositionRequests::GetDisabledComponents);
        componentsForEntity.insert(componentsForEntity.end(), disabledComponents.begin(), disabledComponents.end());
        return componentsForEntity;
    }

    AZ::Entity::ComponentArrayType GetAllComponentsForEntity(const AZ::EntityId& entityId)
    {
        return GetAllComponentsForEntity(GetEntity(entityId));
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

    const char* GetFriendlyComponentName(const AZ::Component* component)
    {
        auto className = component->RTTI_GetTypeName();
        auto classData = GetComponentClassData(component);
        if (!classData)
        {
            return className;
        }
        return classData->m_editData
            ? classData->m_editData->m_name
            : classData->m_name;
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
} // namespace AzToolsFramework;