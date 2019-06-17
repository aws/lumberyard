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

#include <AzToolsFramework/ComponentMode/ComponentModeCollection.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>

#include <QWidget>

namespace AzToolsFramework
{
    /// The default selection/input handler for the editor (includes handling ComponentMode).
    class EditorDefaultSelection
        : public ViewportInteraction::ViewportSelectionRequests
        , private ActionOverrideRequestBus::Handler
        , private ComponentModeFramework::ComponentModeSystemRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        /// @cond
        explicit EditorDefaultSelection(const EditorVisibleEntityDataCache* entityDataCache);
        virtual ~EditorDefaultSelection();
        /// @endcond

    private:
        AZ_DISABLE_COPY_MOVE(EditorDefaultSelection)

        // ViewportInteraction::ViewportSelectionRequests
        bool HandleMouseInteraction(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        void DisplayViewportSelection(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;
        void DisplayViewportSelection2d(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // ActionOverrideRequestBus
        void SetupActionOverrideHandler(QWidget* parent) override;
        void TeardownActionOverrideHandler() override;
        void AddActionOverride(const ActionOverride& actionOverride) override;
        void RemoveActionOverride(AZ::Crc32 actionOverrideUri) override;
        void ClearActionOverrides() override;

        // ComponentModeSystemRequestBus
        void BeginComponentMode(
            const AZStd::vector<ComponentModeFramework::EntityAndComponentModeBuilders>& entityAndComponentModeBuilders) override;
        void AddComponentModes(const ComponentModeFramework::EntityAndComponentModeBuilders& entityAndComponentModeBuilders) override;
        void EndComponentMode() override;
        bool InComponentMode() override { return m_componentModeCollection.InComponentMode(); }
        void Refresh(const AZ::EntityComponentIdPair& entityComponentIdPair) override;
        bool AddedToComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType) override;
        void AddSelectedComponentModesOfType(const AZ::Uuid& componentType) override;
        void SelectNextActiveComponentMode() override;
        void SelectPreviousActiveComponentMode() override;
        void SelectActiveComponentMode(const AZ::Uuid& componentType) override;
        void RefreshActions() override;

        //  Helpers to deal with moving in and out of ComponentMode
        void TransitionToComponentMode();
        void TransitionFromComponentMode();

        QWidget m_overrideWidget; ///< The override (phantom) widget responsible for holding QActions while in ComponentMode
        ComponentModeFramework::ComponentModeCollection m_componentModeCollection; ///< Handles all active ComponentMode types.
        AZStd::unique_ptr<EditorTransformComponentSelection> m_transformComponentSelection = nullptr; ///< Viewport selection (responsible for
                                                                                                      ///< manipulators and transform modifications).
        const EditorVisibleEntityDataCache* m_entityDataCache = nullptr; ///< Reference to cached visible EntityData.

        /// Mapping between passed ActionOverride (AddActionOverride) and allocated QAction.
        struct ActionOverrideMapping
        {
            ActionOverrideMapping(
                const AZ::Crc32 uri, const AZStd::vector<AZStd::function<void()>>& callbacks,
                AZStd::unique_ptr<QAction> action)
                : m_uri(uri)
                , m_callbacks(callbacks)
                , m_action(AZStd::move(action)) {}

            AZ::Crc32 m_uri; ///< Unique identifier for the Action. (In the form 'com.amazon.action.---").
            AZStd::vector<AZStd::function<void()>> m_callbacks; ///< Callbacks associated with this Action (note: with multi-selections there
                                                                ///< will be a callback per Entity/Component).
            AZStd::unique_ptr<QAction> m_action; ///< The QAction associated with the overrideWidget for all ComponentMode actions.
        };

        AZStd::vector<AZStd::shared_ptr<ActionOverrideMapping>> m_actions; ///< Currently bound actions (corresponding to those set
                                                                           ///< on the override widget).
    };
} // namespace AzToolsFramework