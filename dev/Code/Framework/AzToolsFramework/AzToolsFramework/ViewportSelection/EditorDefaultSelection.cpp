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

#include "EditorDefaultSelection.h"

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorDefaultSelection, AZ::SystemAllocator, 0)

    EditorDefaultSelection::EditorDefaultSelection(const EditorVisibleEntityDataCache* entityDataCache)
        : m_overrideWidget(nullptr)
        , m_entityDataCache(entityDataCache)
    {
        ActionOverrideRequestBus::Handler::BusConnect(GetEntityContextId());
        ComponentModeFramework::ComponentModeSystemRequestBus::Handler::BusConnect();

        // only create EditorTransformComponentSelection if we are using the new viewport interaction model
        // note: EditorDefaultSelection is still used when the new viewport interaction model is disabled to support
        // Component Mode when using legacy viewport interaction model
        if (IsNewViewportInteractionModelEnabled())
        {
            m_transformComponentSelection = AZStd::make_unique<EditorTransformComponentSelection>(entityDataCache);
        }
    }

    EditorDefaultSelection::~EditorDefaultSelection()
    {
        ComponentModeFramework::ComponentModeSystemRequestBus::Handler::BusDisconnect();
        ActionOverrideRequestBus::Handler::BusDisconnect();
    }

    void EditorDefaultSelection::BeginComponentMode(
        const AZStd::vector<ComponentModeFramework::EntityAndComponentModeBuilders>& entityAndComponentModeBuilders)
    {
        for (const auto& componentModeBuilder : entityAndComponentModeBuilders)
        {
            AddComponentModes(componentModeBuilder);
        }

        TransitionToComponentMode();
    }

    void EditorDefaultSelection::AddComponentModes(
        const ComponentModeFramework::EntityAndComponentModeBuilders& entityAndComponentModeBuilders)
    {
        for (const auto& componentModeBuilder : entityAndComponentModeBuilders.m_componentModeBuilders)
        {
            m_componentModeCollection.AddComponentMode(
                AZ::EntityComponentIdPair(
                    entityAndComponentModeBuilders.m_entityId, componentModeBuilder.m_componentId),
                componentModeBuilder.m_componentType,
                componentModeBuilder.m_componentModeBuilder);
        }
    }

    void EditorDefaultSelection::TransitionToComponentMode()
    {
        // entering ComponentMode - disable all default actions in the ActionManager
        EditorActionRequestBus::Broadcast(
            &EditorActionRequests::DisableDefaultActions);

        // attach widget to store ComponentMode specific actions
        EditorActionRequestBus::Broadcast(
            &EditorActionRequests::AttachOverride,
            &m_overrideWidget);

        if (m_transformComponentSelection)
        {
            // hide manipulators
            m_transformComponentSelection->UnregisterManipulator();
        }

        m_componentModeCollection.BeginComponentMode();

        // refresh button ui
        ToolsApplicationEvents::Bus::Broadcast(
            &ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
            PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorDefaultSelection::TransitionFromComponentMode()
    {
        m_componentModeCollection.EndComponentMode();

        if (m_transformComponentSelection)
        {
            // safe to show manipulators again
            m_transformComponentSelection->RegisterManipulator();
        }

        EditorActionRequestBus::Broadcast(
            &EditorActionRequests::DetachOverride);

        ClearActionOverrides();

        // leaving ComponentMode - enable all default actions in ActionManager
        EditorActionRequestBus::Broadcast(
            &EditorActionRequests::EnableDefaultActions);

        // refresh button ui
        ToolsApplicationEvents::Bus::Broadcast(
            &ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
            PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorDefaultSelection::EndComponentMode()
    {
        TransitionFromComponentMode();
    }

    void EditorDefaultSelection::Refresh(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_componentModeCollection.Refresh(entityComponentIdPair);
    }

    bool EditorDefaultSelection::AddedToComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType)
    {
        return m_componentModeCollection.AddedToComponentMode(entityComponentIdPair, componentType);
    }

    void EditorDefaultSelection::AddSelectedComponentModesOfType(const AZ::Uuid& componentType)
    {
        ComponentModeFramework::ComponentModeDelegateRequestBus::EnumerateHandlers(
            [componentType](ComponentModeFramework::ComponentModeDelegateRequestBus::InterfaceType* componentModeMouseRequests)
        {
            componentModeMouseRequests->AddComponentModeOfType(componentType);
            return true;
        });

        TransitionToComponentMode();
    }

    void EditorDefaultSelection::SelectNextActiveComponentMode()
    {
        m_componentModeCollection.SelectNextActiveComponentMode();
    }

    void EditorDefaultSelection::SelectPreviousActiveComponentMode()
    {
        m_componentModeCollection.SelectPreviousActiveComponentMode();
    }

    void EditorDefaultSelection::SelectActiveComponentMode(const AZ::Uuid& componentType)
    {
        m_componentModeCollection.SelectActiveComponentMode(componentType);
    }

    void EditorDefaultSelection::RefreshActions()
    {
        m_componentModeCollection.RefreshActions();
    }

    bool EditorDefaultSelection::HandleMouseInteraction(
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        bool enterComponentModeAttempted = false;
        const bool componentModeBefore = InComponentMode();
        if (!componentModeBefore)
        {
            // enumerate all ComponentModeDelegateRequestBus and check if any triggered AddComponentModes
            ComponentModeFramework::ComponentModeDelegateRequestBus::EnumerateHandlers(
                [&mouseInteraction, &enterComponentModeAttempted]
                (ComponentModeFramework::ComponentModeDelegateRequestBus::InterfaceType* componentModeMouseRequests)
            {
                // detect if a double click happened on any Component in the viewport, attempting
                // to move it into ComponentMode (note: this is not guaranteed to succeed as an
                // incompatible multi-selection may prevent it)
                enterComponentModeAttempted = componentModeMouseRequests->DetectEnterComponentModeInteraction(mouseInteraction);
                return !enterComponentModeAttempted;
            });

            // here we know ComponentMode was entered successfully and was not prohibited
            if (m_componentModeCollection.ModesAdded())
            {
                // for other entities in current selection, if they too support the same
                // ComponentMode, add them as well (same effect as pressing Component
                // Mode button in the Property Grid)
                m_componentModeCollection.AddOtherSelectedEntityModes();
                TransitionToComponentMode();
            }
        }
        else
        {
            bool handled = false;
            ComponentModeFramework::ComponentModeRequestBus::EnumerateHandlers(
                [&mouseInteraction, &handled]
                (ComponentModeFramework::ComponentModeRequestBus::InterfaceType* componentModeRequest)
            {
                handled = handled || componentModeRequest->HandleMouseInteraction(mouseInteraction);
                return true;
            });

            if (!handled)
            {
                ComponentModeFramework::ComponentModeDelegateRequestBus::EnumerateHandlers(
                    [&mouseInteraction]
                    (ComponentModeFramework::ComponentModeDelegateRequestBus::InterfaceType* componentModeDelegateRequests)
                {
                    return !componentModeDelegateRequests->DetectLeaveComponentModeInteraction(mouseInteraction);
                });
            }
        }

        // we do not want a double click on a Component while attempting to enter ComponentMode
        // to fall through to normal input handling (as this will cause a deselect to happen).
        // a double click on a Component that prevents entering ComponentMode due to an invalid
        // multi-selection will be a noop
        if (!componentModeBefore && !InComponentMode() && !enterComponentModeAttempted)
        {
            if (m_transformComponentSelection)
            {
                // no components being edited (not in ComponentMode), use standard selection
                return m_transformComponentSelection->HandleMouseInteraction(mouseInteraction);
            }
        }

        return false;
    }

    void EditorDefaultSelection::DisplayViewportSelection(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_transformComponentSelection)
        {
            m_transformComponentSelection->DisplayViewportSelection(viewportInfo, debugDisplay);
        }
    }

    void EditorDefaultSelection::DisplayViewportSelection2d(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (m_transformComponentSelection)
        {
            m_transformComponentSelection->DisplayViewportSelection2d(viewportInfo, debugDisplay);
        }
    }

    void EditorDefaultSelection::SetupActionOverrideHandler(QWidget* parent)
    {
        m_overrideWidget.setParent(parent);
        // note: widget must be 'visible' for actions to fire
        // hide by setting size to zero dimensions
        m_overrideWidget.setFixedSize(0, 0);
    }

    void EditorDefaultSelection::TeardownActionOverrideHandler()
    {
        m_overrideWidget.setParent(nullptr);
    }

    void EditorDefaultSelection::AddActionOverride(const ActionOverride& actionOverride)
    {
        // check if an action with this uri is already added
        const auto actionIt = AZStd::find_if(m_actions.begin(), m_actions.end(),
            [actionOverride](const AZStd::shared_ptr<ActionOverrideMapping>& actionOverrideMapping)
        {
            return actionOverride.m_uri == actionOverrideMapping->m_uri;
        });

        // if an action with the same uri is already added, store the callback for this action
        if (actionIt != m_actions.end())
        {
            (*actionIt)->m_callbacks.push_back(actionOverride.m_callback);
        }
        else
        {
            // create a new action with the override widget as the parent
            AZStd::unique_ptr<QAction> action = AZStd::make_unique<QAction>(&m_overrideWidget);

            // setup action specific data for the editor
            action->setShortcut(actionOverride.m_keySequence);
            action->setStatusTip(actionOverride.m_statusTip);
            action->setText(actionOverride.m_title);

            // bind action to widget
            m_overrideWidget.addAction(action.get());

            // set callbacks that should happen when this action is triggered
            auto index = static_cast<int>(m_actions.size());
            QObject::connect(action.get(), &QAction::triggered, [this, index]()
            {
                const auto vec = m_actions; // increment ref count of shared_ptr, callback may clear actions
                for (auto& callback : vec[index]->m_callbacks)
                {
                    callback();
                }
            });

            m_actions.emplace_back(
                AZStd::make_shared<ActionOverrideMapping>(
                    actionOverride.m_uri, AZStd::vector<AZStd::function<void()>>{ actionOverride.m_callback },
                    AZStd::move(action)));

            // register action with edit menu
            EditorMenuRequestBus::Broadcast(
                &EditorMenuRequests::AddEditMenuAction, m_actions.back()->m_action.get());
        }
    }

    void EditorDefaultSelection::ClearActionOverrides()
    {
        AZStd::for_each(m_actions.begin(), m_actions.end(),
            [this](const AZStd::shared_ptr<ActionOverrideMapping>& actionMapping)
        {
            m_overrideWidget.removeAction(actionMapping->m_action.get());
        });

        m_actions.clear();
    }

    void EditorDefaultSelection::RemoveActionOverride(const AZ::Crc32 actionOverrideUri)
    {
        const auto it = AZStd::find_if(m_actions.begin(), m_actions.end(),
            [actionOverrideUri](const AZStd::shared_ptr<ActionOverrideMapping>& actionMapping)
        {
            return actionMapping->m_uri == actionOverrideUri;
        });

        if (it != m_actions.end())
        {
            m_overrideWidget.removeAction((*it)->m_action.get());
            m_actions.erase(it);
        }
    }
} // namespace AzToolsFramework