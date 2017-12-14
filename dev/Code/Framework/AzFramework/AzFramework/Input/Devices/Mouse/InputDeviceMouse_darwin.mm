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
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_darwin.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/parallel/thread.h>

#include <AppKit/NSApplication.h>
#include <AppKit/NSEvent.h>
#include <AppKit/NSScreen.h>
#include <AppKit/NSWindow.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // NSEventType enum constant names were changed in macOS 10.12, but our min-spec is still 10.10
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101200 // __MAC_10_12 may not be defined by all earlier sdks
    static const NSEventType NSEventTypeLeftMouseDown       = NSLeftMouseDown;
    static const NSEventType NSEventTypeLeftMouseUp         = NSLeftMouseUp;
    static const NSEventType NSEventTypeRightMouseDown      = NSRightMouseDown;
    static const NSEventType NSEventTypeRightMouseUp        = NSRightMouseUp;
    static const NSEventType NSEventTypeOtherMouseDown      = NSOtherMouseDown;
    static const NSEventType NSEventTypeOtherMouseUp        = NSOtherMouseUp;
    static const NSEventType NSEventTypeScrollWheel         = NSScrollWheel;
    static const NSEventType NSEventTypeMouseMoved          = NSMouseMoved;
    static const NSEventType NSEventTypeLeftMouseDragged    = NSLeftMouseDragged;
    static const NSEventType NSEventTypeRightMouseDragged   = NSRightMouseDragged;
    static const NSEventType NSEventTypeOtherMouseDragged   = NSOtherMouseDragged;
#endif // __MAC_OS_X_VERSION_MAX_ALLOWED < 101200
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for osx mouse input devices
    class InputDeviceMouseOsx : public InputDeviceMouse::Implementation
                              , public RawInputNotificationBusOsx::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMouseOsx, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceMouseOsx(InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMouseOsx() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the event tap that exists while the cursor is constrained
        inline CFMachPortRef GetDisabledSystemCursorEventTap() const { return m_disabledSystemCursorEventTap; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the event tap that exists while the cursor is constrained
        inline void SetDisabledSystemCursorPosition(const CGPoint& point) { m_disabledSystemCursorPosition = point; }

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
        //! \ref AzFramework::RawInputNotificationsOsx::OnRawInputEvent
        void OnRawInputEvent(const NSEvent* nsEvent) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Update the system cursor visibility
        void UpdateSystemCursorVisibility();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create an event tap that effectively disables the system cursor, preventing it from
        //! being used to select any other application, but remaining visible and moving around
        //! the screen under user control with the usual system cursor ballistics being applied.
        //! \return True if the event tap was created (or had already been created), false otherwise
        bool CreateDisabledSystemCursorEventTap();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destroy the disabled system cursor event tap
        void DestroyDisabledSystemCursorEventTap();

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        CFRunLoopSourceRef m_disabledSystemCursorRunLoopSource; //!< The disabled cursor run loop
        CFMachPortRef      m_disabledSystemCursorEventTap;      //!< The disabled cursor event tap
        CGPoint            m_disabledSystemCursorPosition;      //!< The disabled cursor position
        SystemCursorState  m_systemCursorState;                 //!< Current system cursor state
        bool               m_hasFocus;                          //!< Does application have focus?
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouse::Implementation* InputDeviceMouse::Implementation::Create(InputDeviceMouse& inputDevice)
    {
        return aznew InputDeviceMouseOsx(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseOsx::InputDeviceMouseOsx(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_disabledSystemCursorRunLoopSource(nullptr)
        , m_disabledSystemCursorEventTap(nullptr)
        , m_disabledSystemCursorPosition()
        , m_systemCursorState(SystemCursorState::Unknown)
        , m_hasFocus(false)
    {
        RawInputNotificationBusOsx::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseOsx::~InputDeviceMouseOsx()
    {
        RawInputNotificationBusOsx::Handler::BusDisconnect();

        // Cleanup system cursor visibility and constraint
        DestroyDisabledSystemCursorEventTap();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseOsx::IsConnected() const
    {
        // If necessary we may be able to determine the connected state using the I/O Kit HIDManager:
        // https://developer.apple.com/library/content/documentation/DeviceDrivers/Conceptual/HID/new_api_10_5/tn2187.html
        //
        // Doing this may allow (and perhaps even force) us to distinguish between multiple physical
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
        // InputSystemComponent/InputSystemComponentOsX in order to create multiple InputDeviceMouse
        // instances (somehow associating each with a raw mouse device id), along with modifying the
        // InputDeviceMouseOsx::OnRawInputEvent function to filter incoming events by raw device id.
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseOsx::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        // Because osx does not provide a way to properly constrain the system cursor inside of a
        // window, when it's constrained we're actually just hiding it and modifying the location
        // of all incoming mouse events so they can't cause the applications window to lose focus.
        // This works fine when the cursor is hidden, provided we normalize its position relative
        // to the entire screen (see InputDeviceMouseOsx::GetSystemCursorPositionNormalized). But
        // when the cursor is constrained and visible, we must manually hide the cursor when it's
        // outside the active window (see InputDeviceMouseOsx::UpdateSystemCursorVisibility), and
        // we must normalize it relative to the active window or else the position will not match
        // that of the cursor while it is visible inside the active window. Long story short, the
        // ConstrainedAndVisible state on osx results in a 'dead-zone' where the system is moving
        // the cursor outside of the active window, but the position is clamped within the border
        // of the active window. This is fine when running in full screen on a single monitor but
        // may not provide the greatest user experience in windowed mode or a multi-monitor setup.
        AZ_Warning("InputDeviceMouseOsx",
                   systemCursorState != SystemCursorState::ConstrainedAndVisible,
                   "ConstrainedAndVisible does not work entirely as might be expected on osx when "
                   "running in windowed mode or with a multi-monitor setup. If your game needs to "
                   "support either of these it is recommended you use another system cursor state.");

        if (systemCursorState == m_systemCursorState)
        {
            return;
        }

        m_systemCursorState = systemCursorState;
        const bool shouldBeDisabled = (m_systemCursorState == SystemCursorState::ConstrainedAndHidden) ||
                                      (m_systemCursorState == SystemCursorState::ConstrainedAndVisible);
        if (!shouldBeDisabled)
        {
            DestroyDisabledSystemCursorEventTap();
            return;
        }

        const bool isDisabled = CreateDisabledSystemCursorEventTap();
        if (!isDisabled)
        {
            AZ_Warning("InputDeviceMouseOsx", false, "Failed create event tap; cursor cannot be constrained as requested");
            m_systemCursorState = SystemCursorState::UnconstrainedAndVisible;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SystemCursorState InputDeviceMouseOsx::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseOsx::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        CGPoint cursorPositionScreen;
        if (m_systemCursorState == SystemCursorState::ConstrainedAndHidden)
        {
            // Because osx does not provide a way to properly constrain the system cursor inside of
            // a window, when it is constrained we are actually just modifying the locations of all
            // incoming mouse events so they cannot cause the application window to lose focus. So
            // if the cursor is also hidden we simply need to de-normalize the position relative to
            // the entire screen. See comment in InputDeviceMouseOsx::SetSystemCursorState for more.
            CGRect cursorBounds = NSScreen.mainScreen.frame;
            cursorPositionScreen.x = positionNormalized.GetX() * cursorBounds.size.width;
            cursorPositionScreen.y = positionNormalized.GetY() * cursorBounds.size.height;
        }
        else
        {
            // However, if the system cursor is visible or not constrained, we need to de-normalize
            // the desired position relative to the content rect of the application's main window.
            NSWindow* mainWindow = NSApplication.sharedApplication.mainWindow;
            CGRect cursorBounds = [mainWindow contentRectForFrameRect: mainWindow.frame];
            cursorPositionScreen = [mainWindow convertRectToScreen: NSMakeRect(positionNormalized.GetX() * cursorBounds.size.width, positionNormalized.GetY() * cursorBounds.size.height, 0, 0)].origin;
        }

        CGWarpMouseCursorPosition(cursorPositionScreen);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceMouseOsx::GetSystemCursorPositionNormalized() const
    {
        // Because osx does not provide a way to properly constrain the system cursor inside of a
        // window, when it's constrained we are actually just disabling it by modifying locations
        // of all incoming mouse events so they cannot cause the application window to lose focus.
        //
        // So if the cursor is also hidden we simply need to normalize the position relative to the
        // entire screen, but we must use the disabled position because calling CGEventSetLocation
        // (see DisabledSysetmCursorEventTapCallback) results in NSEvent.mouseLocation returning
        // the clamped position (along with CGEventGetLocation and CGEventGetUnflippedLocation).
        NSRect cursorBounds = NSScreen.mainScreen.frame;
        NSPoint cursorPosition = m_disabledSystemCursorPosition;

        if (m_systemCursorState != SystemCursorState::ConstrainedAndHidden)
        {
            // However, if the system cursor is visible or not constrained, we need to normalize
            // the clamped position relative to the content rect of the application's main window.
            // (which in this case can be obtained from NSEvent.mouseLocation)
            NSWindow* mainWindow = NSApplication.sharedApplication.mainWindow;
            cursorBounds = [mainWindow contentRectForFrameRect: mainWindow.frame];
            cursorPosition = [mainWindow convertRectFromScreen: NSMakeRect(NSEvent.mouseLocation.x, NSEvent.mouseLocation.y, 0, 0)].origin;
        }

        // Normalize the cursor position and flip the y component so it is relative to the top
        const float cursorPostionNormalizedX = cursorBounds.size.width != 0.0f ? cursorPosition.x / cursorBounds.size.width : 0.0f;
        const float cursorPostionNormalizedY = cursorBounds.size.height != 0.0f ? 1.0f - (cursorPosition.y / cursorBounds.size.height) : 0.0f;

        return AZ::Vector2(cursorPostionNormalizedX, cursorPostionNormalizedY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseOsx::TickInputDevice()
    {
        // The osx event loop has just been pumped in InputSystemComponentOsx::PreTickInputDevices,
        // so we now just need to process any raw events that have been queued since the last frame
        const bool hadFocus = m_hasFocus;
        m_hasFocus = NSApplication.sharedApplication.mainWindow.keyWindow;
        if (m_hasFocus)
        {
            // Update the visibility of the system cursor, which unfortunately must be done every
            // frame to combat the cursor being displayed by the system under some circumstances.
            UpdateSystemCursorVisibility();

            // Process raw event queues once each frame while this application's window has focus
            ProcessRawEventQueues();
        }
        else if (hadFocus)
        {
            // This application's window no longer has focus, process any events that are queued,
            // before resetting the state of all this input device's associated input channels.
            ProcessRawEventQueues();
            ResetInputChannelStates();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseOsx::OnRawInputEvent(const NSEvent* nsEvent)
    {
        switch (nsEvent.type)
        {
            // Left button
            case NSEventTypeLeftMouseDown:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Left, true);
            }
            break;
            case NSEventTypeLeftMouseUp:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Left, false);
            }
            break;

            // Right button
            case NSEventTypeRightMouseDown:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Right, true);
            }
            break;
            case NSEventTypeRightMouseUp:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Right, false);
            }
            break;

            // Middle button
            case NSEventTypeOtherMouseDown:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Middle, true);
            }
            break;
            case NSEventTypeOtherMouseUp:
            {
                QueueRawButtonEvent(InputDeviceMouse::Button::Middle, false);
            }
            break;

            // Scroll wheel
            case NSEventTypeScrollWheel:
            {
                QueueRawMovementEvent(InputDeviceMouse::Movement::Z, nsEvent.scrollingDeltaY);
            }
            break;

            // Mouse movement
            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeOtherMouseDragged:
            {
                QueueRawMovementEvent(InputDeviceMouse::Movement::X, nsEvent.deltaX);
                QueueRawMovementEvent(InputDeviceMouse::Movement::Y, nsEvent.deltaY);
            }
            break;

            default:
            {
                // Ignore
            }
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsPointInsideActiveWindow(NSPoint point)
    {
        NSWindow* mainWindow = NSApplication.sharedApplication.mainWindow;
        if (mainWindow == nil || !mainWindow.keyWindow)
        {
            return false;
        }

        NSRect contentRect = [mainWindow contentRectForFrameRect: mainWindow.frame];
        return NSPointInRect(point, contentRect);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseOsx::UpdateSystemCursorVisibility()
    {
        bool shouldCursorBeVisible = true;
        switch (m_systemCursorState)
        {
            case SystemCursorState::ConstrainedAndHidden:
            {
                shouldCursorBeVisible = false;
            }
            break;
            case SystemCursorState::ConstrainedAndVisible:
            {
                shouldCursorBeVisible = IsPointInsideActiveWindow(m_disabledSystemCursorPosition);
            }
            break;
            case SystemCursorState::UnconstrainedAndHidden:
            {
                shouldCursorBeVisible = !IsPointInsideActiveWindow(NSEvent.mouseLocation);
            }
            break;
            case SystemCursorState::UnconstrainedAndVisible:
            case SystemCursorState::Unknown:
            {
                shouldCursorBeVisible = true;
            }
            break;
        }

        // CGCursorIsVisible has been deprecated but still works, and there is no other way to check
        // whether the system cursor is currently visible. Calls to CGDisplayHideCursor are meant to
        // increment an application specific 'cursor hidden' counter that must be balanced by a call
        // to CGDisplayShowCursor, however this doesn't seem to work if the cursor is shown again by
        // the system (or another application), which can happen when this application loses focus.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        const bool isCursorVisible = CGCursorIsVisible();
#pragma clang diagnostic pop
        if (isCursorVisible && !shouldCursorBeVisible)
        {
            CGDisplayHideCursor(kCGNullDirectDisplay);
        }
        else if (!isCursorVisible && shouldCursorBeVisible)
        {
            CGDisplayShowCursor(kCGNullDirectDisplay);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    CGEventRef DisabledSysetmCursorEventTapCallback(CGEventTapProxy eventTapProxy,
                                                       CGEventType eventType,
                                                       CGEventRef eventRef,
                                                       void* userInfo)
    {
        InputDeviceMouseOsx* inputDeviceMouse = static_cast<InputDeviceMouseOsx*>(userInfo);
        switch (eventType)
        {
            case kCGEventTapDisabledByTimeout:
            case kCGEventTapDisabledByUserInput:
            {
                // Re-enable the event tap if it gets disabled (important to do this first)
                CGEventTapEnable(inputDeviceMouse->GetDisabledSystemCursorEventTap(), true);
                return eventRef;
            }
            break;
        }

        // Get the location of the event relative to the bottom-left corner of the screen,
        // and store it before it is (potentially) modified below. This is needed because
        // calling CGEventSetLocation results in subsequent calls to CGEventGetLocation,
        // CGEventGetUnflippedLocation, or NSEvent::mouseLocation returning the clamped
        // position we just set instead of where the system has positioned the cursor.
        CGPoint eventLocation = CGEventGetUnflippedLocation(eventRef);
        inputDeviceMouse->SetDisabledSystemCursorPosition(eventLocation);

        NSWindow* mainWindow = NSApplication.sharedApplication.mainWindow;
        if (mainWindow == nil || !mainWindow.keyWindow)
        {
            // Do nothing if this application's main window does not have focus
            return eventRef;
        }

        NSRect contentRect = [mainWindow contentRectForFrameRect: mainWindow.frame];
        if (NSPointInRect(NSPointFromCGPoint(eventLocation), contentRect))
        {
            // Do nothing if the event occured inside of the main window's content rect
            return eventRef;
        }

        // Constrain the event location to the content rect, adjusting MaxX by -1 to account for
        // the right edge being considered outside the boundary, adjusting MinY by +1 to account
        // for the bottom edge being considerd outside the boundary, and then MinY by another +2
        // to account for the bottom corners of osx windows being rounded by 2 pixels. Basically,
        // if we don't make these adjustments then it is still possible for the user to deselect
        // the application by clicking outside of its main window, which is what we're trying to
        // prevent with this whole event tap/event location adjustment in the first place.
        eventLocation.x = AZ::GetClamp(eventLocation.x, NSMinX(contentRect), NSMaxX(contentRect) - 1.0f);
        eventLocation.y = AZ::GetClamp(eventLocation.y, NSMinY(contentRect) + 3.0f, NSMaxY(contentRect));

        // Reset the event location after flipping it back to be relative to the top of the screen
        eventLocation.y = NSMaxY(NSScreen.mainScreen.frame) - eventLocation.y;
        CGEventSetLocation(eventRef, eventLocation);

        return eventRef;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseOsx::CreateDisabledSystemCursorEventTap()
    {
        if (m_disabledSystemCursorEventTap != nullptr)
        {
            // It has already been created
            return true;
        }

        // Create an event tap that listens for all mouse events
        static const CGEventMask allMouseEventsMask = CGEventMaskBit(kCGEventLeftMouseDown) |
                                                      CGEventMaskBit(kCGEventLeftMouseDragged) |
                                                      CGEventMaskBit(kCGEventLeftMouseUp) |
                                                      CGEventMaskBit(kCGEventRightMouseDown) |
                                                      CGEventMaskBit(kCGEventRightMouseDragged) |
                                                      CGEventMaskBit(kCGEventRightMouseUp) |
                                                      CGEventMaskBit(kCGEventOtherMouseDown) |
                                                      CGEventMaskBit(kCGEventOtherMouseUp) |
                                                      CGEventMaskBit(kCGEventMouseMoved);
        m_disabledSystemCursorEventTap = CGEventTapCreate(kCGSessionEventTap,
                                                          kCGHeadInsertEventTap,
                                                          kCGEventTapOptionDefault,
                                                          allMouseEventsMask,
                                                          &DisabledSysetmCursorEventTapCallback,
                                                          this);
        if (m_disabledSystemCursorEventTap == nullptr)
        {
            return false;
        }

        // Create a run loop source
        m_disabledSystemCursorRunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_disabledSystemCursorEventTap, 0);
        if (m_disabledSystemCursorRunLoopSource == nullptr)
        {
            CFRelease(m_disabledSystemCursorEventTap);
            m_disabledSystemCursorEventTap = nullptr;
            return false;
        }

        // Add the run loop source and enable the event tap
        CFRunLoopAddSource(CFRunLoopGetCurrent(), m_disabledSystemCursorRunLoopSource, kCFRunLoopCommonModes);
        CGEventTapEnable(m_disabledSystemCursorEventTap, true);

        // Initialize the constrained system cursor position
        CGEventRef event = CGEventCreate(nil);
        m_disabledSystemCursorPosition = CGEventGetUnflippedLocation(event);
        CFRelease(event);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseOsx::DestroyDisabledSystemCursorEventTap()
    {
        if (m_disabledSystemCursorEventTap == nullptr)
        {
            // It has already been destroyed
            return;
        }

        // Disable the event tap and remove the run loop source
        CGEventTapEnable(m_disabledSystemCursorEventTap, false);
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), m_disabledSystemCursorRunLoopSource, kCFRunLoopCommonModes);

        // Destroy the run loop source
        CFRelease(m_disabledSystemCursorRunLoopSource);
        m_disabledSystemCursorRunLoopSource = nullptr;

        // Destroy the event tap
        CFRelease(m_disabledSystemCursorEventTap);
        m_disabledSystemCursorEventTap = nullptr;
    }
} // namespace AzFramework
