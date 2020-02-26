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

#include <AzCore/Memory/SystemAllocator.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AzFramework
{
    //! A platform independent hint to decide what type of window to create internally.
    //! See InitWindow on the platform implementation to see how this is used. 
    using WindowType = AZ::Crc32;

    namespace WindowTypes
    {
        static const WindowType Main = AZ_CRC("Main", 0xbf28cd64);
    }

    //! A simple structure to contain window geometry.
    //! The defaults here reflect the defaults when creating a new
    //! WindowGeometry object. Different window implementations may 
    //! have their own separate default window sizes.
    struct WindowGeometry
    {
        WindowGeometry() = default;

        WindowGeometry(const uint32_t posX, const uint32_t posY, const uint32_t width, const uint32_t height)
            : m_posX(posX)
            , m_posY(posY)
            , m_width(width)
            , m_height(height)
        {}

        uint32_t m_posX = 0;
        uint32_t m_posY = 0;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };

    //! Provides a basic window.
    //!
    //! This is mainly designed to be used as a base window for rendering.
    //! The window provides just a simple surface and handles implementation
    //! details for each platform. 
    //! 
    //! Window events are pumped via the AzFramework's application. No event messaging
    //! or message translation/dispatch needs to be handled by the underlying implementation.
    //!
    //! Multiple NativeWindows are supported by this system. This is impractical for most standalone 
    //! game applications, and unnecessary for some platforms, but still possible.
    //!
    //! The Window implementation will be created when the NativeWindow is constructed and the
    //! platform specific native window will be created (if it doesn't already exist).
    //! On platforms that support it, the window will become visible when Activate is called and
    //! hidden when Deactivate is called. On other platforms the window becomes visible on construction
    //! and hidden on destruction.
    //! To provide a consistent API to the client across platforms:
    //! - Calling Activate (when not active) will result in the OnWindowResized notification
    //! - Calling Deactivate (when active) will result in the OnWindowClosed notification
    //! - destroying the NativeWindow when active will result in Deactivate being called
    class NativeWindow final
        : public WindowRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindow, AZ::SystemAllocator, 0);

        NativeWindow(const AZStd::string& title, const WindowGeometry& geometry, WindowType windowType = WindowTypes::Main);
        ~NativeWindow() override;

        //! Activate the window.
        //! The window will be visible after this call (on some platforms it will be also visible before activation).
        void Activate();

        //! Deactivate the window.
        //! This will result in the OnWindowClosed notification being sent.
        //! On some platforms this will hide the window. On others the window will remain visible.
        void Deactivate();

        bool IsActive() const;

        //! Get the native window handle. This is used as the bus id for the WindowRequestBus and WindowNotificationBus
        NativeWindowHandle GetWindowHandle() const { return m_pimpl->GetWindowHandle(); }

        // WindowRequestBus::Handler overrides ...
        void SetWindowTitle(const AZStd::string& title) override;
        WindowSize GetClientAreaSize() const override;

        //! The NativeWindow implementation.
        //! Extend this to provide windowing capabilities per platform.
        //! It's expected that only one Implementation::Create method will be available per-platform.
        class Implementation
        {
        public:
            static Implementation* Create();
            virtual ~Implementation() = default;

            virtual void InitWindow(const AZStd::string& title, const WindowGeometry& geometry, const WindowType windowType) = 0;

            virtual void Activate();
            virtual void Deactivate();
            bool IsActive() const { return m_activated; }

            virtual NativeWindowHandle GetWindowHandle() const = 0;

            virtual void SetWindowTitle(const AZStd::string& title);
            virtual WindowSize GetClientAreaSize() const;

        protected:
            bool m_activated;
            uint32_t m_width;
            uint32_t m_height;
        };

    private:
        AZStd::unique_ptr<Implementation> m_pimpl;

        WindowType m_windowType;
    };
} // namespace AzFramework