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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    // These bus interfaces are designed to be used by any system that
    // implements any sort of window that provides a surface that a swapchain can
    // attach to.

    using NativeWindowHandle = void*; 

    //! A simple structure to contain window size.
    struct WindowSize
    {
        WindowSize() = default;

        WindowSize(const uint32_t width, const uint32_t height)
            : m_width(width)
            , m_height(height)
        {}

        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };

    //! Bus for sending requests to any kind of window.
    //! It could be a NativeWindow or an Editor window.
    class WindowRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~WindowRequests() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = NativeWindowHandle;

        //! For platforms that support it, set the title of the window.
        virtual void SetWindowTitle(const AZStd::string& title) = 0;

        //! Get the client area size. This is the size that can be rendered to.
        //! On some platforms this may not be the correct size until Activate is called.
        virtual WindowSize GetClientAreaSize() const = 0;
    };
    using WindowRequestBus = AZ::EBus<WindowRequests>;

    //! Bus for listening for notifications from a window.
    //! It could be a NativeWindow or an Editor window.
    class WindowNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~WindowNotifications() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = NativeWindowHandle;

        //! This is called once when the window is Activated and also called if the user resizes the window.
        virtual void OnWindowResized(uint32_t width, uint32_t height) { AZ_UNUSED(width); AZ_UNUSED(height); };

        //! This is called when the window is deactivated from code or if the user closes the window.
        virtual void OnWindowClosed() {};
    };
    using WindowNotificationBus = AZ::EBus<WindowNotifications>;

    //! The WindowSystemRequestBus is a broadcast bus for sending requests to the window system.
    class WindowSystemRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~WindowSystemRequests() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Get the window handle for the default window
        virtual NativeWindowHandle GetDefaultWindowHandle() = 0;
    };
    using WindowSystemRequestBus = AZ::EBus<WindowSystemRequests>;

    //! The WindowSystemNotificationBus is used to broadcast an event whenever a new window is created.
    class WindowSystemNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~WindowSystemNotifications() = default;

        // EBusTraits overrides ...
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! A notification that a new window was created with the given window ID
        virtual void OnWindowCreated(NativeWindowHandle windowHandle) = 0;
    };
    using WindowSystemNotificationBus = AZ::EBus<WindowSystemNotifications>;
} // namespace AzFramework
