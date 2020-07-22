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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/variant.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <EditorWhiteBoxComponentModeBus.h>
#include <EditorWhiteBoxComponentModeTypes.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    class DefaultMode;
    class EdgeRestoreMode;

    //! The type of edge selection the component mode is in (either normal selection of
    //! 'user' edges or selection of all edges ('mesh') in restoration mode).
    enum class EdgeSelectionType
    {
        Polygon,
        All
    };

    //! The Component Mode responsible for handling all interactions with the White Box Tool.
    class EditorWhiteBoxComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private EditorWhiteBoxComponentNotificationBus::Handler
        , public EditorWhiteBoxComponentModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EditorWhiteBoxComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        EditorWhiteBoxComponentMode(EditorWhiteBoxComponentMode&&) = default;
        EditorWhiteBoxComponentMode& operator=(EditorWhiteBoxComponentMode&&) = default;
        ~EditorWhiteBoxComponentMode();

        // EditorBaseComponentMode ...
        void Refresh() override;
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;

        // EditorWhiteBoxComponentModeRequestBus ...
        void MarkWhiteBoxIntersectionDataDirty() override;

    private:
        // AzFramework::EntityDebugDisplayEventBus ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // TransformNotificationBus ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorWhiteBoxComponentNotificationBus ...
        void OnDefaultShapeTypeChanged(DefaultShapeType defaultShape) override;

        //! Rebuild the intermediate intersection data from the source white box data.
        //! @param edgeSelectionMode Determines whether to include all edges ('mesh' + 'user') or
        //! just 'user' edges when generating the intersection data.
        void RecalculateWhiteBoxIntersectionData(EdgeSelectionType edgeSelectionMode);

        //! The current set of 'sub' modes the white box component mode can be in.
        AZStd::variant<AZStd::unique_ptr<DefaultMode>, AZStd::unique_ptr<EdgeRestoreMode>> m_modes;

        //! The most up to date intersection and render data for the white box (edge and polygon bounds).
        AZStd::optional<IntersectionAndRenderData> m_intersectionAndRenderData;
        AZ::Transform m_worldFromLocal; //!< The world transform of the entity this ComponentMode is on.
    };
} // namespace WhiteBox
