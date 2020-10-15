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

#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzCore/std/string/string.h>

namespace AzManipulatorTestFramework
{
    //! Base class for derived immediate and retained action dispatchers.
    template <typename DerivedDispatcherT>
    class ActionDispatcher
    {
    public:
        virtual ~ActionDispatcher() = default;

        //! Enable grid snapping.
        DerivedDispatcherT* EnableSnapToGrid();
        //! Disable grid snapping.
        DerivedDispatcherT* DisableSnapToGrid();
        //! Set the grid size.
        DerivedDispatcherT* GridSize(float size);
        //! Enable/disable action logging.
        DerivedDispatcherT* LogActions(bool logging);
        //! Output a trace debug message.
        template <typename... Args>
        DerivedDispatcherT* Trace(const char* format, const Args&... args);
        //! Set the camera state.
        DerivedDispatcherT* CameraState(const AzFramework::CameraState& ccameraState);
        //! Set the left mouse button down.
        DerivedDispatcherT* MouseLButtonDown();
        //! Set the left mouse button up.
        DerivedDispatcherT* MouseLButtonUp();
        //! Set the keyboard modifier button down.
        DerivedDispatcherT* KeyboardModifierDown(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier);
        //! Set the keyboard modifier button up.
        DerivedDispatcherT* KeyboardModifierUp(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier);
        //! Move the mouse position by the specified world space delta.
        DerivedDispatcherT* MouseRayMove(const AZ::Vector3& delta);
        //! Expect the selected manipulator to be interacting.
        DerivedDispatcherT* ExpectManipulatorBeingInteracted();
        //! Do not expect the selected manipulator to be interacting.
        DerivedDispatcherT* ExpectNotManipulatorBeingInteracted();
    protected:
        // Actions to be implemented by derived immediate and retained action dispatchers.
        virtual void EnableSnapToGridImpl() = 0;
        virtual void DisableSnapToGridImpl() = 0;
        virtual void GridSizeImpl(float size) = 0;
        virtual void CameraStateImpl(const AzFramework::CameraState& cameraState) = 0;
        virtual void MouseLButtonDownImpl() = 0;
        virtual void MouseLButtonUpImpl() = 0;
        virtual void MouseRayMoveImpl(const AZ::Vector3& delta) = 0;
        virtual void KeyboardModifierDownImpl(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier) = 0;
        virtual void KeyboardModifierUpImpl(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier) = 0;
        virtual void ExpectManipulatorBeingInteractedImpl() = 0;
        virtual void ExpectNotManipulatorBeingInteractedImpl() = 0;
        template <typename... Args>
        void Log(const char* format, const Args&... args);
        bool m_logging = false;
    private:
        const char* KeyboardModifierString(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier);
    };

    template <typename DerivedDispatcherT>
    template <typename... Args>
    void ActionDispatcher<DerivedDispatcherT>::Log(const char* format, const Args&... args)
    {
        if (m_logging)
        {
            AZStd::string message = AZStd::string::format(format, args...);
            std::cout << "[ActionDispatcher] " << message.c_str() << "\n";
        }
    }

    template <typename DerivedDispatcherT>
    template <typename... Args>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::Trace(const char* format, const Args&... args)
    {
        Log(format, args...);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::EnableSnapToGrid()
    {
        Log("Enabling SnapToGrid");
        EnableSnapToGridImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::DisableSnapToGrid()
    {
        Log("Disabling SnapToGrid");
        DisableSnapToGridImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::GridSize(float size)
    {
        Log("GridSize: %f", size);
        GridSizeImpl(size);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::LogActions(bool logging)
    {
        m_logging = logging;
        Log("Log actions: %s", m_logging ? "enabled" : "disabled");
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::CameraState(const AzFramework::CameraState& cameraState)
    {
        Log("Camera state: p(%f, %f, %f) d(%f, %f, %f)",
            float(cameraState.m_position.GetX()), float(cameraState.m_position.GetY()), float(cameraState.m_position.GetZ()),
            float(cameraState.m_forward.GetX()), float(cameraState.m_forward.GetY()), float(cameraState.m_forward.GetZ()));
        CameraStateImpl(cameraState);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MouseLButtonDown()
    {
        Log("Mouse left button down");
        MouseLButtonDownImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MouseLButtonUp()
    {
        Log("Mouse left button up");
        MouseLButtonUpImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }
    template <typename DerivedDispatcherT>
    const char* ActionDispatcher<DerivedDispatcherT>::KeyboardModifierString(
        const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier)
    {
        using namespace AzToolsFramework::ViewportInteraction;
        switch (keyModifier)
        {
        case KeyboardModifier::Alt:
            return "Alt";
        case KeyboardModifier::Control:
            return "Ctl";
        case KeyboardModifier::Shift:
            return "Shift";
        case KeyboardModifier::None:
            return "None";
        default: return "Unknown modifier";
        }
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::KeyboardModifierDown(
        const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier)
    {
        Log("Keyboard modifier down: %s", KeyboardModifierString(keyModifier));
        KeyboardModifierDownImpl(keyModifier);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::KeyboardModifierUp(
        const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier)
    {
        Log("Keyboard modifier up: %s", KeyboardModifierString(keyModifier));
        KeyboardModifierUpImpl(keyModifier);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::MouseRayMove(const AZ::Vector3& delta)
    {
        Log("Mouse move: delta(%f, %f, %f)",
            float(delta.GetX()), float(delta.GetY()), float(delta.GetZ()));
        MouseRayMoveImpl(delta);
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::ExpectManipulatorBeingInteracted()
    {
        Log("Expecting manipulator interacting");
        ExpectManipulatorBeingInteractedImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }

    template <typename DerivedDispatcherT>
    DerivedDispatcherT* ActionDispatcher<DerivedDispatcherT>::ExpectNotManipulatorBeingInteracted()
    {
        Log("Not expecting manipulator interacting");
        ExpectNotManipulatorBeingInteractedImpl();
        return static_cast<DerivedDispatcherT*>(this);
    }
} // namespace AzManipulatorTestFramework
