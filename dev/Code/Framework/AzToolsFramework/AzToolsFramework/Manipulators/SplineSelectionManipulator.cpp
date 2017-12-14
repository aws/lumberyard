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

#include "SplineSelectionManipulator.h"

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    SplineSelectionManipulator::Action CalculateManipulationDataAction(
        const AZ::Transform& worldFromLocal, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, const AZStd::weak_ptr<const AZ::Spline> spline)
    {
        SplineSelectionManipulator::Action action;
        if (const AZStd::shared_ptr<const AZ::Spline> splinePtr = spline.lock())
        {
            AZ::Transform worldFromLocalNormalized = worldFromLocal;
            const AZ::Vector3 scale = worldFromLocalNormalized.ExtractScale();
            const AZ::Transform localFromWorldNormalized = worldFromLocalNormalized.GetInverseFast();

            const AZ::Vector3 localRayOrigin = localFromWorldNormalized * rayOrigin * scale.GetReciprocal();
            const AZ::Vector3 localRayDirection = localFromWorldNormalized.Multiply3x3(rayDirection);
            const AZ::RaySplineQueryResult splineQueryResult = splinePtr->GetNearestAddressRay(localRayOrigin, localRayDirection);

            action.m_localSplineHitPosition = splinePtr->GetPosition(splineQueryResult.m_splineAddress);
            action.m_splineAddress = splineQueryResult.m_splineAddress;
        }

        return action;
    }

    SplineSelectionManipulator::SplineSelectionManipulator(AZ::EntityId entityId)
        : BaseManipulator(entityId)
    {
        AttachLeftMouseDownImpl();
    }

    SplineSelectionManipulator::~SplineSelectionManipulator() {}

    void SplineSelectionManipulator::InstallLeftMouseDownCallback(MouseActionCallback onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void SplineSelectionManipulator::InstallLeftMouseUpCallback(MouseActionCallback onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void SplineSelectionManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        if (!interaction.m_keyboardModifiers.Ctrl())
        {
            return;
        }

        if (m_onLeftMouseDownCallback)
        {
            AZ::Transform worldFromLocal;
            AZ::TransformBus::EventResult(
                worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                worldFromLocal, interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, m_spline));
        }
    }

    void SplineSelectionManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (MouseOver() && m_onLeftMouseUpCallback)
        {
            AZ::Transform worldFromLocal;
            AZ::TransformBus::EventResult(
                worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                worldFromLocal, interaction.m_mousePick.m_rayOrigin,
                interaction.m_mousePick.m_rayDirection, m_spline));
        }
    }

    void SplineSelectionManipulator::Draw(
        AzFramework::EntityDebugDisplayRequests &display,
        const ViewportInteraction::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        // if the ctrl modifier key state has changed - set out bounds to dirty and
        // update the active state.
        if (m_keyboardModifiers != mouseInteraction.m_keyboardModifiers)
        {
            SetBoundsDirty();
            m_keyboardModifiers = mouseInteraction.m_keyboardModifiers;
        }

        if (mouseInteraction.m_keyboardModifiers.Ctrl() && !mouseInteraction.m_keyboardModifiers.Shift())
        {
            AZ::Transform worldFromLocal;
            AZ::TransformBus::EventResult(
                worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            m_manipulatorView->Draw(
                GetManipulatorManagerId(), GetManipulatorId(), MouseOver(),
                AZ::Vector3::CreateZero(), worldFromLocal, display, cameraState,
                mouseInteraction, ManipulatorSpace::Local);
        }
    }

    void SplineSelectionManipulator::SetView(AZStd::unique_ptr<ManipulatorView>&& view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void SplineSelectionManipulator::SetBoundsDirtyImpl()
    {
        m_manipulatorView->SetBoundDirty(GetManipulatorManagerId());
    }

    void SplineSelectionManipulator::InvalidateImpl()
    {
        m_manipulatorView->Invalidate(GetManipulatorManagerId());
    }
}