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

#include "SurfaceManipulator.h"

#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    SurfaceManipulator::StartInternal SurfaceManipulator::CalculateManipulationDataStart(
        const AZ::Transform& worldFromLocal, const AZ::Vector3& worldSurfacePosition,
        const AZ::Vector3& localStartPosition, const bool snapping, const float gridSize, const int viewportId)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::VectorFloat scale = worldFromLocalNormalized.ExtractScale().GetMaxElement();
        const AZ::VectorFloat scaleRecip = Round3(scale.GetReciprocal());

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 localSurfacePosition = localFromWorldNormalized * worldSurfacePosition;

        const AZ::Vector3 localFinalSurfacePosition = snapping
            ? CalculateSnappedTerrainPosition(
                worldSurfacePosition, worldFromLocalNormalized, viewportId, gridSize * scaleRecip)
            : localFromWorldNormalized * worldSurfacePosition;

        // delta/offset between initial vertex position and terrain pick position
        const AZ::Vector3 localSurfaceOffset = localFinalSurfacePosition * scaleRecip - localStartPosition;

        StartInternal startInternal;
        startInternal.m_snapOffset = localSurfaceOffset;
        startInternal.m_localPosition = localStartPosition + localSurfaceOffset;
        startInternal.m_localHitPosition = localSurfacePosition * scaleRecip;
        return startInternal;
    }

    SurfaceManipulator::Action SurfaceManipulator::CalculateManipulationDataAction(
        const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
        const AZ::Vector3& worldSurfacePosition, const bool snapping, const float gridSize,
        const ViewportInteraction::KeyboardModifiers keyboardModifiers, const int viewportId)
    {
        AZ::Transform worldFromLocalNormalized = worldFromLocal;
        const AZ::VectorFloat scale = worldFromLocalNormalized.ExtractScale().GetMaxElement();
        const AZ::VectorFloat scaleRecip = Round3(scale.GetReciprocal());

        const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();
        const AZ::Vector3 localFinalSurfacePosition = snapping
            ? CalculateSnappedTerrainPosition(
                worldSurfacePosition, worldFromLocalNormalized, viewportId, gridSize * scaleRecip)
            : localFromWorldNormalized * worldSurfacePosition;

        Action action;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_snapOffset = startInternal.m_snapOffset;
        action.m_current.m_localOffset = localFinalSurfacePosition * scaleRecip - startInternal.m_localPosition;
        // record what modifier keys are held during this action
        action.m_modifiers = keyboardModifiers;
        return action;
    }

    SurfaceManipulator::SurfaceManipulator(const AZ::Transform& worldFromLocal)
        : m_worldFromLocal(worldFromLocal)
    {
        AttachLeftMouseDownImpl();
    }

    void SurfaceManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }
    void SurfaceManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void SurfaceManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void SurfaceManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(m_worldFromLocal);

        const bool snapping =
            GridSnapping(interaction.m_interactionId.m_viewportId);
        const float gridSize =
            GridSize(interaction.m_interactionId.m_viewportId);

        AZ::Vector3 worldSurfacePosition;
        ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
            worldSurfacePosition, interaction.m_interactionId.m_viewportId,
            &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::PickTerrain,
            ViewportInteraction::QPointFromScreenPoint(
                interaction.m_mousePick.m_screenCoordinates));

        m_startInternal = CalculateManipulationDataStart(
            worldFromLocalUniformScale, worldSurfacePosition, m_position,
            snapping, gridSize, interaction.m_interactionId.m_viewportId);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_startInternal, worldFromLocalUniformScale, worldSurfacePosition,
                snapping, gridSize, interaction.m_keyboardModifiers,
                interaction.m_interactionId.m_viewportId));
        }
    }

    void SurfaceManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            AZ::Vector3 worldSurfacePosition = AZ::Vector3::CreateZero();
            ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
                worldSurfacePosition, interaction.m_interactionId.m_viewportId,
                &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::PickTerrain,
                ViewportInteraction::QPointFromScreenPoint(
                    interaction.m_mousePick.m_screenCoordinates));

            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_startInternal, TransformUniformScale(m_worldFromLocal), worldSurfacePosition,
                GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId),
                interaction.m_keyboardModifiers, interaction.m_interactionId.m_viewportId));
        }
    }

    void SurfaceManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            AZ::Vector3 worldSurfacePosition = AZ::Vector3::CreateZero();
            ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
                worldSurfacePosition, interaction.m_interactionId.m_viewportId,
                &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::PickTerrain,
                ViewportInteraction::QPointFromScreenPoint(
                    interaction.m_mousePick.m_screenCoordinates));

            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_startInternal, TransformUniformScale(m_worldFromLocal), worldSurfacePosition,
                GridSnapping(interaction.m_interactionId.m_viewportId),
                GridSize(interaction.m_interactionId.m_viewportId),
                interaction.m_keyboardModifiers, interaction.m_interactionId.m_viewportId));
        }
    }

    void SurfaceManipulator::SetBoundsDirtyImpl()
    {
        m_manipulatorView->SetBoundDirty(GetManipulatorManagerId());
    }

    void SurfaceManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState,
            GetManipulatorId(), {
                TransformUniformScale(m_worldFromLocal),
                m_position, MouseOver()
            },
            debugDisplay, cameraState, mouseInteraction);
    }

    void SurfaceManipulator::InvalidateImpl()
    {
        m_manipulatorView->Invalidate(GetManipulatorManagerId());
    }

    void SurfaceManipulator::SetView(AZStd::unique_ptr<ManipulatorView>&& view)
    {
        m_manipulatorView = AZStd::move(view);
    }
}