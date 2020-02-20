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

#include <AzFramework/Windowing/NativeWindow.h>

namespace AzFramework
{
    //////////////////////////////////////////////////////////////////////////
    // NativeWindow class implementation
    //////////////////////////////////////////////////////////////////////////

    NativeWindow::NativeWindow(const AZStd::string& title, const WindowGeometry& geometry, WindowType windowType)
        : m_windowType(windowType)
    {
        m_pimpl.reset(Implementation::Create());

        m_pimpl->InitWindow(title, geometry, m_windowType);
    }
    
    NativeWindow::~NativeWindow()
    {
        Deactivate();
        m_pimpl.reset();
    }

    void NativeWindow::Activate()
    {
        // If the underlying window implementation failed to create a window,
        // don't attach to the window bus
        if (m_pimpl != nullptr)
        {
            auto windowHandle = m_pimpl->GetWindowHandle();

            WindowRequestBus::Handler::BusConnect(windowHandle);
            m_pimpl->Activate();
        }
    }

    void NativeWindow::Deactivate()
    {
        if (m_pimpl != nullptr)
        {
            auto windowHandle = m_pimpl->GetWindowHandle();
            m_pimpl->Deactivate();
            WindowRequestBus::Handler::BusDisconnect(windowHandle);
        }
    }

    bool NativeWindow::IsActive() const
    {
        return m_pimpl != nullptr && m_pimpl->IsActive();
    }

    void NativeWindow::SetWindowTitle(const AZStd::string& title)
    {
        m_pimpl->SetWindowTitle(title);
    }

    WindowSize NativeWindow::GetClientAreaSize() const
    {
        return m_pimpl->GetClientAreaSize();
    }

    //////////////////////////////////////////////////////////////////////////
    // NativeWindow::Implementation class implementation
    //////////////////////////////////////////////////////////////////////////

    void NativeWindow::Implementation::Activate()
    {
        if (!m_activated)
        {
            m_activated = true;
            WindowNotificationBus::Event(GetWindowHandle(), &WindowNotificationBus::Events::OnWindowResized, m_width, m_height);
        }
    }

    void NativeWindow::Implementation::Deactivate()
    {
        if (m_activated) // nothing to do if window was already deactivated
        {
            m_activated = false;
            WindowNotificationBus::Event(GetWindowHandle(), &WindowNotificationBus::Events::OnWindowClosed);
        }
    }

    void NativeWindow::Implementation::SetWindowTitle(const AZStd::string& title)
    {
        // For most implementations this does nothing
        AZ_UNUSED(title);
    }

    WindowSize NativeWindow::Implementation::GetClientAreaSize() const
    {
        return WindowSize(m_width, m_height);
    }

} // namespace AzFramework