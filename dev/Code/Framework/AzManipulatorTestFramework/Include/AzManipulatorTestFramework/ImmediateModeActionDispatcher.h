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

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/ActionDispatcher.h>

namespace AzManipulatorTestFramework
{
    //! Dispatches actions immediately to the manipulators.
    class ImmediateModeActionDispatcher
        : public ActionDispatcher<ImmediateModeActionDispatcher>
    {
    public:
        explicit ImmediateModeActionDispatcher(ManipulatorViewportInteractionInterface& viewportManipulatorInteraction);
        ~ImmediateModeActionDispatcher();

        //! Clear the current event state.
        ImmediateModeActionDispatcher* ResetEvent();
        //! Expect the expression to be true.
        ImmediateModeActionDispatcher* ExpectTrue(bool result);
        //! Expect the expression to be false.
        ImmediateModeActionDispatcher* ExpectFalse(bool result);
        //! Expect the two values to be equivalent.
        template<typename ActualT, typename ExpectedT>
        ImmediateModeActionDispatcher* ExpectEq(const ActualT& actual, const ExpectedT& expected);

    protected:
        // ActionDispatcher ...
        void EnableSnapToGridImpl() override;
        void DisableSnapToGridImpl() override;
        void GridSizeImpl(float size) override;
        void CameraStateImpl(const AzFramework::CameraState& cameraState) override;
        void MouseLButtonDownImpl() override;
        void MouseLButtonUpImpl() override;
        void MouseRayMoveImpl(const AZ::Vector3& delta) override;
        void KeyboardModifierDownImpl(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier) override;
        void KeyboardModifierUpImpl(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier) override;
        void ExpectManipulatorBeingInteractedImpl() override;
        void ExpectNotManipulatorBeingInteractedImpl() override;

    private:
        AzToolsFramework::ViewportInteraction::MouseInteractionEvent* GetEvent();
        AZStd::unique_ptr<AzToolsFramework::ViewportInteraction::MouseInteractionEvent> m_event;
        ManipulatorViewportInteractionInterface& m_viewportManipulatorInteraction;
    };

    template<typename ActualT, typename ExpectedT>
    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ExpectEq(const ActualT& actual, const ExpectedT& expected)
    {
        Log("Expecting equality");
        EXPECT_EQ(actual, expected);
        return this;
    }
} // namespace AzManipulatorTestFramework
