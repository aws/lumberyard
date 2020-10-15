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

#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>

namespace AzManipulatorTestFramework
{
    template<typename FieldT, typename FlagT>
    void ToggleOn(FieldT& field, FlagT flag)
    {
        field |= static_cast<FieldT>(flag);
    }

    template<typename FieldT, typename FlagT>
    void ToggleOff(FieldT& field, FlagT flag)
    {
        field &= ~static_cast<FieldT>(flag);
    }

    using MouseButton = AzToolsFramework::ViewportInteraction::MouseButton;
    using KeyboardModifier = AzToolsFramework::ViewportInteraction::KeyboardModifier;
    using MouseInteractionEvent = AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    ImmediateModeActionDispatcher::ImmediateModeActionDispatcher(
        ManipulatorViewportInteractionInterface& viewportManipulatorInteraction)
        : m_viewportManipulatorInteraction(viewportManipulatorInteraction)
    {
    }

    ImmediateModeActionDispatcher::~ImmediateModeActionDispatcher() = default;

    void ImmediateModeActionDispatcher::EnableSnapToGridImpl()
    {
        m_viewportManipulatorInteraction.GetViewportInteraction().EnableGridSnaping();
    }

    void ImmediateModeActionDispatcher::DisableSnapToGridImpl()
    {
        m_viewportManipulatorInteraction.GetViewportInteraction().DisableGridSnaping();
    }

    void ImmediateModeActionDispatcher::GridSizeImpl(float size)
    {
        m_viewportManipulatorInteraction.GetViewportInteraction().SetGridSize(size);
    }

    void ImmediateModeActionDispatcher::CameraStateImpl(const AzFramework::CameraState& cameraState)
    {
        m_viewportManipulatorInteraction.GetViewportInteraction().SetCameraState(cameraState);
        GetEvent()->m_mouseInteraction.m_mousePick.m_rayOrigin = cameraState.m_position;
        GetEvent()->m_mouseInteraction.m_mousePick.m_rayDirection = cameraState.m_forward;
    }

    void ImmediateModeActionDispatcher::MouseLButtonDownImpl()
    {
        ToggleOn(GetEvent()->m_mouseInteraction.m_mouseButtons.m_mouseButtons, MouseButton::Left);
        GetEvent()->m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Down;
        m_viewportManipulatorInteraction.GetManipulatorManager().ConsumeMouseInteractionEvent(*m_event);
    }

    void ImmediateModeActionDispatcher::MouseLButtonUpImpl()
    {
        GetEvent()->m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Up;
        m_viewportManipulatorInteraction.GetManipulatorManager().ConsumeMouseInteractionEvent(*GetEvent());
        ToggleOff(GetEvent()->m_mouseInteraction.m_mouseButtons.m_mouseButtons, MouseButton::Left);
    }

    void ImmediateModeActionDispatcher::KeyboardModifierDownImpl(const KeyboardModifier& keyModifier)
    {
        ToggleOn(GetEvent()->m_mouseInteraction.m_keyboardModifiers.m_keyModifiers, keyModifier);
    }

    void ImmediateModeActionDispatcher::KeyboardModifierUpImpl(const KeyboardModifier& keyModifier)
    {
        ToggleOff(GetEvent()->m_mouseInteraction.m_keyboardModifiers.m_keyModifiers, keyModifier);
    }

    void ImmediateModeActionDispatcher::MouseRayMoveImpl(const AZ::Vector3& delta)
    {
        m_event->m_mouseInteraction.m_mousePick.m_rayOrigin += delta;
        m_event->m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Move;
        m_viewportManipulatorInteraction.GetManipulatorManager().ConsumeMouseInteractionEvent(*m_event);
    }

    AzToolsFramework::ViewportInteraction::MouseInteractionEvent* ImmediateModeActionDispatcher::GetEvent()
    {
        if (!m_event)
        {
            m_event = AZStd::unique_ptr<MouseInteractionEvent>(AZStd::make_unique<MouseInteractionEvent>());
            const int viewportId = m_viewportManipulatorInteraction.GetViewportInteraction().GetViewportId();
            m_event->m_mouseInteraction.m_interactionId.m_viewportId = viewportId;
        }
        return m_event.get();
    }

    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ExpectTrue(bool result)
    {
        Log("Expecting true");
        EXPECT_TRUE(result);
        return this;
    }

    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ExpectFalse(bool result)
    {
        Log("Expecting false");
        EXPECT_FALSE(result);
        return this;
    }

    void ImmediateModeActionDispatcher::ExpectManipulatorBeingInteractedImpl()
    {
        EXPECT_TRUE(m_viewportManipulatorInteraction.GetManipulatorManager().ManipulatorBeingInteracted());
    }

    void ImmediateModeActionDispatcher::ExpectNotManipulatorBeingInteractedImpl()
    {
        EXPECT_FALSE(m_viewportManipulatorInteraction.GetManipulatorManager().ManipulatorBeingInteracted());
    }

    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ResetEvent()
    {
        Log("Resetting the event state");
        m_event.reset();
        return this;
    }
} // namespace AzManipulatorTestFramework
