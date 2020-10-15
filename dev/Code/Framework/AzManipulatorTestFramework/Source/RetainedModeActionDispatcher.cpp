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

#include <AzManipulatorTestFramework/RetainedModeActionDispatcher.h>
#include <exception>

namespace AzManipulatorTestFramework
{
    using KeyboardModifier = AzToolsFramework::ViewportInteraction::KeyboardModifier;

    RetainedModeActionDispatcher::RetainedModeActionDispatcher(
        ManipulatorViewportInteractionInterface& viewportManipulatorInteraction)
        : m_dispatcher(viewportManipulatorInteraction)
    {
    }

    void RetainedModeActionDispatcher::AddActionToSequence(Action&& action)
    {
        if (m_locked)
        {
            const char* error = "Couldn't add action to sequence, dispatcher is locked (you must call ResetSequence() \
                                 before adding actions to this dispatcher)!";
            Log(error);
            throw(std::runtime_error(error));
        }

        m_actions.emplace_back(action);
    }

    void RetainedModeActionDispatcher::EnableSnapToGridImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.EnableSnapToGrid(); });
    }

    void RetainedModeActionDispatcher::DisableSnapToGridImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.DisableSnapToGrid(); });
    }

    void RetainedModeActionDispatcher::GridSizeImpl(float size)
    {
        AddActionToSequence([=]() { m_dispatcher.GridSize(size); });
    }

    void RetainedModeActionDispatcher::CameraStateImpl(const AzFramework::CameraState& cameraState)
    {
        AddActionToSequence([=]() { m_dispatcher.CameraState(cameraState); });
    }

    void RetainedModeActionDispatcher::MouseLButtonDownImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.MouseLButtonDown(); });
    }

    void RetainedModeActionDispatcher::MouseLButtonUpImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.MouseLButtonUp(); });
    }

    void RetainedModeActionDispatcher::KeyboardModifierDownImpl(const KeyboardModifier& keyModifier)
    {
        AddActionToSequence([=]() { m_dispatcher.KeyboardModifierDown(keyModifier); });
    }

    void RetainedModeActionDispatcher::KeyboardModifierUpImpl(const KeyboardModifier& keyModifier)
    {
        AddActionToSequence([=]() { m_dispatcher.KeyboardModifierUp(keyModifier); });
    }

    void RetainedModeActionDispatcher::MouseRayMoveImpl(const AZ::Vector3& delta)
    {
        AddActionToSequence([=]() { m_dispatcher.MouseRayMove(delta); });
    }

    void RetainedModeActionDispatcher::ExpectManipulatorBeingInteractedImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.ExpectManipulatorBeingInteracted(); });
    }

    void RetainedModeActionDispatcher::ExpectNotManipulatorBeingInteractedImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.ExpectNotManipulatorBeingInteracted(); });
    }

    RetainedModeActionDispatcher* RetainedModeActionDispatcher::ResetSequence()
    {
        Log("Resetting the action sequence");
        m_actions.clear();
        m_dispatcher.ResetEvent();
        m_locked = false;
        return this;
    }

    RetainedModeActionDispatcher* RetainedModeActionDispatcher::Execute()
    {
        Log("Executing %u actions", m_actions.size());
        for (auto& action : m_actions)
        {
            action();
        }
        m_dispatcher.ResetEvent();
        m_locked = true;
        return this;
    }
} // namespace AzManipulatorTestFramework
