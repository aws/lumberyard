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

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_win.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <CrySystemBus.h> ///< Bus used to get the application's main window, which is compared to the result of ::GetForegroundWindow() to detects if application is active

#include <Windows.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    // These are scarcely documented, and do not seem to be publically accessible:
    // - https://msdn.microsoft.com/en-us/library/windows/desktop/ms645546(v=vs.85).aspx
    // - https://msdn.microsoft.com/en-us/library/ff543440.aspx
    const USHORT RAW_INPUT_MOUSE_USAGE_PAGE = 0x01;
    const USHORT RAW_INPUT_MOUSE_USAGE = 0x02;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /*  An extended version of ::GetFocus() WinApi call. 
        The ::GetFocus() call in this file is used to detect if application is active (in the foreground).
        What ::GetFocus() really does though is return the window with the KEYBOARD focus for the CURRENT THREAD's message queue.
        This basically means that the call returns the current control within the application window that will receive the keyboard events.
        Usually, when the main application window is sent to background, the text focus is reset (::GetFocus() returns nullptr), but not always -
        it is possible for the application to be in the background, yet ::GetFocus() to continue returning a valid window handle.
        The MSDN documentation is somewhat brief and vague in this area, making it easy to use this call incorrectly. It does recommend using  ::GetForegroundWindow()
        instead to get the window with which the user is currently working. GetFocusEx() uses that call to verify if the handle from ::GetFocus() should be used - 
        if ::GetFocus() handle is valid, but the application is not active, ignore it.
    */
    HWND GetFocusEx()
    {
        static void* mainHwnd = nullptr;
        if (!mainHwnd)
        {
            CrySystemRequestBus::BroadcastResult(mainHwnd, &CrySystemRequestBus::Events::GetMainWindowHandle);
        }
        HWND fgHwnd = ::GetForegroundWindow();
        return (static_cast<HWND>(mainHwnd) == fgHwnd) ? ::GetFocus() : nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get the focus window that should be used to clip and/or normalize the system cursor.
    //! \return The HWND that should currently be considered the applictaion's focus window.
    HWND GetSystemCursorFocusWindow()
    {
        void* systemCursorFocusWindow = nullptr;
        AzFramework::InputSystemCursorConstraintRequestBus::BroadcastResult(
            systemCursorFocusWindow,
            &AzFramework::InputSystemCursorConstraintRequests::GetSystemCursorConstraintWindow);
        return systemCursorFocusWindow ? static_cast<HWND>(systemCursorFocusWindow) : GetFocusEx();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for windows mouse input devices
    class InputDeviceMouseWin : public InputDeviceMouse::Implementation
                              , public RawInputNotificationBusWin::Handler
    {
        //! Count of the number instances of this class that have been created
        static int s_instanceCount;

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMouseWin, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceMouseWin(InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMouseWin() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorState
        void SetSystemCursorState(SystemCursorState systemCursorState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorState
        SystemCursorState GetSystemCursorState() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsWin::OnRawInputEvent
        void OnRawInputEvent(const RAWINPUT& rawInput) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Refresh system cursor clipping constraint
        void RefreshSystemCursorClippingConstraint();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Refresh system cursor viibility
        void RefreshSystemCursorVisibility();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The current system cursor state
        SystemCursorState m_systemCursorState;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Does the window attached to the input (main) thread's message queue have focus?
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms646294(v=vs.85).aspx
        bool m_hasFocus;

        //! The flags sent with the last received MOUSE_MOVE_RELATIVE or MOUSE_MOVE_ABSOLUTE event.
        USHORT m_lastMouseMoveEventFlags;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The last absolute mouse position, reported by a MOUSE_MOVE_ABSOLUTE raw input API event,
        //! which will more than likely only ever be received when running a remote desktop session.
        AZ::Vector2 m_lastMouseMoveEventAbsolutePosition;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::Implementation* InputDeviceMouse::Implementation::Create(InputDeviceMouse& inputDevice)
    {
        return aznew InputDeviceMouseWin(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    int InputDeviceMouseWin::s_instanceCount = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseWin::InputDeviceMouseWin(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_systemCursorState(SystemCursorState::Unknown)
        , m_hasFocus(false)
        , m_lastMouseMoveEventFlags(0)
        , m_lastMouseMoveEventAbsolutePosition()
    {
        if (s_instanceCount++ == 0)
        {
            // Register for raw mouse input
            RAWINPUTDEVICE rawInputDevice;
            rawInputDevice.usUsagePage = RAW_INPUT_MOUSE_USAGE_PAGE;
            rawInputDevice.usUsage     = RAW_INPUT_MOUSE_USAGE;
            rawInputDevice.dwFlags     = 0;
            rawInputDevice.hwndTarget  = 0;
            const BOOL result = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice));
            AZ_Assert(result, "Failed to register raw input device: mouse");
            AZ_UNUSED(result);
        }

        RawInputNotificationBusWin::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseWin::~InputDeviceMouseWin()
    {
        RawInputNotificationBusWin::Handler::BusDisconnect();

        // Cleanup system cursor visibility and constraint
        SetSystemCursorState(SystemCursorState::Unknown);

        if (--s_instanceCount == 0)
        {
            // Deregister from raw mouse input
            RAWINPUTDEVICE rawInputDevice;
            rawInputDevice.usUsagePage = RAW_INPUT_MOUSE_USAGE_PAGE;
            rawInputDevice.usUsage     = RAW_INPUT_MOUSE_USAGE;
            rawInputDevice.dwFlags     = RIDEV_REMOVE;
            rawInputDevice.hwndTarget  = 0;
            const BOOL result = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice));
            AZ_Assert(result, "Failed to deregister raw input device: mouse");
            AZ_UNUSED(result);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseWin::IsConnected() const
    {
        // If necessary, we can register raw input devices using RIDEV_DEVNOTIFY in order to receive
        // WM_INPUT_DEVICE_CHANGE windows messages in the WndProc function. These could then be sent
        // using an EBus (RawInputNotificationBusWin?) and used to keep track of the connected state.
        //
        // Doing this would allow (in one respect force) us to distinguish between multiple physical
        // devices of the same type. But given that support for multiple mice is a fairly niche need
        // we'll keep things simple (for now) and assume there is one (and only one) mouse connected
        // at all times. In practice this means that if multiple physical mice are connected we will
        // process input from them all, but treat all the input as if it comes from the same device.
        //
        // If it becomes necessary to determine the connected state of mouse devices (and/or support
        // distinguishing between multiple physical mice), we should implement this function as well
        // call BroadcastInputDeviceConnectedEvent/BroadcastInputDeviceDisconnectedEvent when needed.
        //
        // Note that doing so will require modifying how we create and manage mouse input devices in
        // InputSystemComponent/InputSystemComponentWin in order to create multiple InputDeviceMouse
        // instances (somehow associating each with a RID_DEVICE_INFO_MOUSE), and also modifying the
        // InputDeviceMouseWin::OnRawInputEvent function to filter incoming events by raw device id.
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWin::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        if (systemCursorState != m_systemCursorState)
        {
            m_systemCursorState = systemCursorState;
            RefreshSystemCursorClippingConstraint();
            RefreshSystemCursorVisibility();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SystemCursorState InputDeviceMouseWin::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWin::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        HWND focusWindow = GetSystemCursorFocusWindow();
        if (!focusWindow)
        {
            return;
        }

        // Get the content (client) rect of the focus window
        RECT clientRect;
        ::GetClientRect(focusWindow, &clientRect);
        const float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
        const float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

        // De-normalize the position relative to the focus window, then transform to screen coords
        POINT cursorPos;
        cursorPos.x = static_cast<LONG>(positionNormalized.GetX() * clientWidth);
        cursorPos.y = static_cast<LONG>(positionNormalized.GetY() * clientHeight);
        ::ClientToScreen(focusWindow, &cursorPos);
        ::SetCursorPos(cursorPos.x, cursorPos.y);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceMouseWin::GetSystemCursorPositionNormalized() const
    {
        HWND focusWindow = GetSystemCursorFocusWindow();
        if (!focusWindow)
        {
            return AZ::Vector2::CreateZero();
        }

        // Get the position of the cursor relative to the focus window
        POINT cursorPos;
        ::GetCursorPos(&cursorPos);
        ::ScreenToClient(focusWindow, &cursorPos);

        // Get the content (client) rect of the focus window
        RECT clientRect;
        ::GetClientRect(focusWindow, &clientRect);

        // Normalize the cursor position relative to the content (client rect) fo the focus window
        const float clientRectWidth = static_cast<float>(clientRect.right - clientRect.left);
        const float clientRectHeight = static_cast<float>(clientRect.bottom - clientRect.top);
        const float normalizedCursorPostionX = static_cast<float>(cursorPos.x) / clientRectWidth;
        const float normalizedCursorPostionY = static_cast<float>(cursorPos.y) / clientRectHeight;

        return AZ::Vector2(normalizedCursorPostionX, normalizedCursorPostionY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWin::TickInputDevice()
    {
        // The input event loop is pumped by the system on windows so all raw input events for this
        // frame have already been dispatched. But they are all queued until ProcessRawEventQueues
        // is called below so that all raw input events are processed at the same time every frame.

        const bool hadFocus = m_hasFocus;
        m_hasFocus = GetFocusEx() != nullptr;
        if (m_hasFocus)
        {
            // We have to refresh the system cursor clip rect each frame this application has focus
            // to combat the cursor being unclipped by the system or another application (which can
            // happen in a variety of ways due to the cursor being a shared resource), and also for
            // when this application transitions between fullscreen and windowed mode.
            RefreshSystemCursorClippingConstraint();

            // Process raw event queues once each frame while this thread's message queue has focus
            ProcessRawEventQueues();
        }
        else if (hadFocus)
        {
            // The window attached to this thread's message queue no longer has focus, process any
            // events that are queued, before resetting the state of all associated input channels.
            ProcessRawEventQueues();
            ResetInputChannelStates();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWin::OnRawInputEvent(const RAWINPUT& rawInput)
    {
        if (rawInput.header.dwType != RIM_TYPEMOUSE || !GetFocusEx())
        {
            return;
        }

        const RAWMOUSE& rawMouseData = rawInput.data.mouse;
        const USHORT buttonFlags = rawMouseData.usButtonFlags;

        // Left button
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Left, true);
        }
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Left, false);
        }

        // Right button
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Right, true);
        }
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Right, false);
        }

        // Middle button
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Middle, true);
        }
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Middle, false);
        }

        // X1 button (deprecated)
        if (buttonFlags & RI_MOUSE_BUTTON_4_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Other1, true);
        }
        if (buttonFlags & RI_MOUSE_BUTTON_4_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Other1, false);
        }

        // X2 button (deprecated)
        if (buttonFlags & RI_MOUSE_BUTTON_5_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Other2, true);
        }
        if (buttonFlags & RI_MOUSE_BUTTON_5_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Other2, false);
        }

        // Scroll wheel
        if (buttonFlags & RI_MOUSE_WHEEL)
        {
            QueueRawMovementEvent(InputDeviceMouse::Movement::Z, static_cast<short>(rawMouseData.usButtonData));
        }

        // Mouse movement
        if (rawMouseData.usFlags == MOUSE_MOVE_RELATIVE)
        {
            QueueRawMovementEvent(InputDeviceMouse::Movement::X, static_cast<float>(rawMouseData.lLastX));
            QueueRawMovementEvent(InputDeviceMouse::Movement::Y, static_cast<float>(rawMouseData.lLastY));
            m_lastMouseMoveEventFlags = rawMouseData.usFlags;
        }
        else if (rawMouseData.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            const bool isVirtualDesktop = (rawMouseData.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
            const int screenWidth = GetSystemMetrics(isVirtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            const int screenHeight = GetSystemMetrics(isVirtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            const float absoluteX = (static_cast<float>(rawMouseData.lLastX) / static_cast<float>(USHRT_MAX)) * static_cast<float>(screenWidth);
            const float absoluteY = (static_cast<float>(rawMouseData.lLastY) / static_cast<float>(USHRT_MAX)) * static_cast<float>(screenHeight);

            if (m_lastMouseMoveEventFlags & MOUSE_MOVE_ABSOLUTE)
            {
                // Only calculate and send the delta if we have previously cached a valid position
                const float deltaX = absoluteX - m_lastMouseMoveEventAbsolutePosition.GetX();
                const float deltaY = absoluteY - m_lastMouseMoveEventAbsolutePosition.GetY();
                QueueRawMovementEvent(InputDeviceMouse::Movement::X, deltaX);
                QueueRawMovementEvent(InputDeviceMouse::Movement::Y, deltaY);
            }

            m_lastMouseMoveEventFlags = rawMouseData.usFlags;
            m_lastMouseMoveEventAbsolutePosition.SetX(absoluteX);
            m_lastMouseMoveEventAbsolutePosition.SetY(absoluteY);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWin::RefreshSystemCursorClippingConstraint()
    {
        HWND focusWindow = GetSystemCursorFocusWindow();
        if (!focusWindow)
        {
            return;
        }

        const bool shouldBeConstrained = (m_systemCursorState == SystemCursorState::ConstrainedAndHidden) ||
                                         (m_systemCursorState == SystemCursorState::ConstrainedAndVisible);
        if (!shouldBeConstrained)
        {
            // Unconstrain the cursor
            ::ClipCursor(NULL);
            return;
        }

        // Constrain the cursor to the client (content) rect of the focus window
        RECT clientRect;
        ::GetClientRect(focusWindow, &clientRect);
        ::ClientToScreen(focusWindow, (LPPOINT)&clientRect.left);  // Converts the top-left point
        ::ClientToScreen(focusWindow, (LPPOINT)&clientRect.right); // Converts the bottom-right point
        ::ClipCursor(&clientRect);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWin::RefreshSystemCursorVisibility()
    {
        const bool shouldBeHidden = (m_systemCursorState == SystemCursorState::ConstrainedAndHidden) ||
                                    (m_systemCursorState == SystemCursorState::UnconstrainedAndHidden);

        // The windows system ShowCursor function stores and returns an application specific display
        // counter, and the cursor is displayed only when the display count is greater than or equal
        // to zero: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648396(v=vs.85).aspx
        if (shouldBeHidden)
        {
            while (::ShowCursor(false) >= 0) {}
        }
        else
        {
            while (::ShowCursor(true) < 0) {}
        }
    }
} // namespace AzFramework
