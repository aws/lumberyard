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

#include "ComponentModeDelegate.h"

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        static const char* const s_componentModeEnterDescription =
            "In this mode, you can only edit properties for this component. "
            "All other components on the entity are locked.";

        static const char* const s_componentModeLeaveDescription =
            "Return to normal viewport editing";

        // was the double click on the component or off it (select/deselect)
        enum class DoubleClickOutcome
        {
            OnComponent,
            OffComponent,
            None,
        };

        static bool EnterComponentModeButtonVisible()
        {
            return !InComponentMode();
        }

        static bool LeaveComponentModeButtonVisible()
        {
            return InComponentMode();
        }

        static DoubleClickOutcome DoubleClickedComponent(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction,
            EditorComponentSelectionRequestsBus::Handler* editorComponentSelection)
        {
            if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick &&
                mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                if (editorComponentSelection)
                {
                    AZ::VectorFloat distance;
                    if (editorComponentSelection->EditorSelectionIntersectRayViewport(
                        { mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId },
                        mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin,
                        mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection,
                        distance))
                    {
                        return DoubleClickOutcome::OnComponent;
                    }
                }

                return DoubleClickOutcome::OffComponent;
            }

            return DoubleClickOutcome::None;
        }

        static bool ShouldDetectEnterLeaveComponentMode(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick;
        }

        void ComponentModeDelegate::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ComponentModeDelegate>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ComponentModeDelegate>(
                        "Component Mode", "Provides advanced editing of Components.")
                        ->UIElement(AZ::Edit::UIHandlers::Button, "", s_componentModeEnterDescription)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ComponentModeDelegate::OnComponentModeEnterButtonPressed)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Edit")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EnterComponentModeButtonVisible)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, &ComponentModeDelegate::ComponentModeButtonInactive)
                        ->UIElement(AZ::Edit::UIHandlers::Button, "", s_componentModeLeaveDescription)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ComponentModeDelegate::OnComponentModeLeaveButtonPressed)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Done")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &LeaveComponentModeButtonVisible)
                            ;
                }
            }
        }

        bool ComponentModeDelegate::AddedToComponentMode()
        {
            bool addedToComponentMode = false;
            ComponentModeSystemRequestBus::BroadcastResult(
                addedToComponentMode, &ComponentModeSystemRequests::AddedToComponentMode,
                m_entityComponentIdPair, m_componentType);

            return addedToComponentMode;
        }

        void ComponentModeDelegate::SetAddComponentModeCallback(
            const AZStd::function<void(const AZ::EntityComponentIdPair&)>& addComponentModeCallback)
        {
            m_addComponentMode = addComponentModeCallback;
        }

        void ComponentModeDelegate::OnComponentModeEnterButtonPressed()
        {
            if (!InComponentMode())
            {
                // move all selected components into ComponentMode
                ComponentModeSystemRequestBus::Broadcast(
                    &ComponentModeSystemRequests::AddSelectedComponentModesOfType,
                    m_componentType);
            }
        }

        void ComponentModeDelegate::OnComponentModeLeaveButtonPressed()
        {
            if (InComponentMode())
            {
                // move the editor out of ComponentMode
                ComponentModeSystemRequestBus::Broadcast(
                    &ComponentModeSystemRequests::EndComponentMode);
            }
        }

        void ComponentModeDelegate::AddComponentMode()
        {
            if (m_addComponentMode)
            {
                m_addComponentMode(m_entityComponentIdPair);
            }
        }

        void ComponentModeDelegate::ConnectInternal(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType,
            EditorComponentSelectionRequestsBus::Handler* handler)
        {
            m_handler = handler; // could be null
            m_entityComponentIdPair = entityComponentIdPair;
            m_componentType = componentType;

            EntitySelectionEvents::Bus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
            EditorEntityVisibilityNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
            EditorEntityLockComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        }

        void ComponentModeDelegate::Disconnect()
        {
            EditorEntityLockComponentNotificationBus::Handler::BusDisconnect();
            EditorEntityVisibilityNotificationBus::Handler::BusDisconnect();
            EntitySelectionEvents::Bus::Handler::BusDisconnect();
        }

        void ComponentModeDelegate::OnSelected()
        {
            ComponentModeDelegateRequestBus::Handler::BusConnect(m_entityComponentIdPair);
        }

        void ComponentModeDelegate::OnDeselected()
        {
            ComponentModeDelegateRequestBus::Handler::BusDisconnect();
        }

        bool ComponentModeDelegate::DetectEnterComponentModeInteraction(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            if (ShouldDetectEnterLeaveComponentMode(mouseInteraction))
            {
                if (!IsSelectableInViewport(m_entityComponentIdPair.GetEntityId()))
                {
                    return false;
                }

                if (DoubleClickedComponent(mouseInteraction, m_handler) == DoubleClickOutcome::OnComponent)
                {
                    EntityIdList entityIds;
                    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                        entityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

                    AZStd::vector<AZ::Uuid> componentTypes;
                    AZ::Entity::ComponentArrayType components;
                    // reserve small initial buffer for common case
                    components.reserve(8);
                    componentTypes.reserve(8);
                    // build a list of all components on each entity in the current selection
                    for (const AZ::EntityId entityId : entityIds)
                    {
                        components.clear();

                        // get all components related to the entity
                        GetAllComponentsForEntity(GetEntity(entityId), components);
                        RemoveHiddenComponents(components);

                        AZStd::transform(
                            components.begin(), components.end(), AZStd::back_inserter(componentTypes),
                            [](const AZ::Component* component) { return component->GetUnderlyingComponentType(); });
                    }

                    // count how many components of our type are in the selection
                    const size_t componentCount =
                        AZStd::count_if(componentTypes.begin(), componentTypes.end(),
                        [this](const AZ::Uuid& componentType) { return componentType == m_componentType; });

                    // if the count matches the entity selection size, we know each entity has a
                    // component of that type, and so it will be displaying in the Entity Outliner
                    // if this is the case we know it is safe to enter ComponentMode
                    if (componentCount == entityIds.size())
                    {
                        AddComponentMode();
                    }

                    // we still want to notify the outside world an attempt was made to enter
                    // ComponentMode - ComponentModeCollection::ModesAdded() must be called
                    // to determine if ComponentMode was actually entered
                    return true;
                }
            }

            return false;
        }

        bool ComponentModeDelegate::DetectLeaveComponentModeInteraction(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            if (ShouldDetectEnterLeaveComponentMode(mouseInteraction))
            {
                if (DoubleClickedComponent(mouseInteraction, m_handler) == DoubleClickOutcome::OffComponent)
                {
                    ComponentModeSystemRequestBus::Broadcast(
                        &ComponentModeSystemRequests::EndComponentMode);

                    return true;
                }
            }

            return false;
        }

        void ComponentModeDelegate::AddComponentModeOfType(const AZ::Uuid componentType)
        {
            if (m_componentType == componentType)
            {
                AddComponentMode();
            }
        }

        bool CouldBeginComponentModeWithEntity(const AZ::EntityId entityId)
        {
            EntityIdList selectedEntityIds;
            ToolsApplicationRequests::Bus::BroadcastResult(
                selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

            // handles both having no entities selected and when an entity inspector is
            // pinned on an entity that's not selected and a different entity is selected
            bool canBegin = AZStd::find(
                selectedEntityIds.cbegin(), selectedEntityIds.cend(), entityId) != selectedEntityIds.cend();

            if (canBegin)
            {
                for (const AZ::EntityId selectedEntityId : selectedEntityIds)
                {
                    // if any entities in the selection are not selectable (invisible/locked)
                    // then still make it impossible to enter begin ComponentMode
                    if (!IsSelectableInViewport(selectedEntityId))
                    {
                        canBegin = false;
                        break;
                    }
                }
            }

            return canBegin;
        }

        bool ComponentModeDelegate::ComponentModeButtonInactive() const
        {
            return !CouldBeginComponentModeWithEntity(m_entityComponentIdPair.GetEntityId());
        }

        void ComponentModeDelegate::OnEntityVisibilityChanged(bool /*visibility*/)
        {
            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_AttributesAndValues);
        }

        void ComponentModeDelegate::OnEntityLockChanged(bool /*locked*/)
        {
            ToolsApplicationNotificationBus::Broadcast(
                &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_AttributesAndValues);
        }
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
