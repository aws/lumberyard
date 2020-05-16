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

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Windows.h>
#include <AzFramework/Windowing/NativeWindow.h>

#include <AzCore/PlatformIncl.h>

namespace AzFramework
{
    class NativeWindowImpl_Win32 final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Win32, AZ::SystemAllocator, 0);
        NativeWindowImpl_Win32() = default;
        ~NativeWindowImpl_Win32() override;

        // NativeWindow::Implementation overrides...
        void InitWindow(const AZStd::string& title, const WindowGeometry& geometry, const WindowType windowType) override;
        void Activate() override;
        void Deactivate() override;
        NativeWindowHandle GetWindowHandle() const override;
        void SetWindowTitle(const AZStd::string& title) override;

    private:
        static LRESULT CALLBACK WindowCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        static const char* s_defaultClassName;
        static const char* s_defaultWindowName;

        void WindowSizeChanged(const uint32_t width, const uint32_t height);

        HWND m_win32Handle;
    };

    const char* NativeWindowImpl_Win32::s_defaultClassName = "LumberyardWin32Class";

    NativeWindow::Implementation* NativeWindow::Implementation::Create()
    {
        return aznew NativeWindowImpl_Win32();
    }

    NativeWindowImpl_Win32::~NativeWindowImpl_Win32()
    {
        DestroyWindow(m_win32Handle);

        m_win32Handle = nullptr;
    }

    void NativeWindowImpl_Win32::InitWindow(const AZStd::string& title, const WindowGeometry& geometry, const WindowType windowType)
    {
        AZ_UNUSED(windowType);

        const HINSTANCE hInstance = GetModuleHandle(0);

        // register window class if it does not exist
        WNDCLASSEX windowClass;
        if (GetClassInfoEx(hInstance, s_defaultClassName, &windowClass) == false)
        {
            windowClass.cbSize = sizeof(WNDCLASSEX);
            windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            windowClass.lpfnWndProc = &NativeWindowImpl_Win32::WindowCallback;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = hInstance;
            windowClass.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
            windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            windowClass.lpszMenuName = NULL;
            windowClass.lpszClassName = s_defaultClassName;
            windowClass.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);

            if (!RegisterClassEx(&windowClass))
            {
                AZ_Error("Windowing", false, "Failed to register Win32 window class with error: %d", GetLastError());
            }
        }

        // These will change from the external size to the client area size once we receive a WM_Size message
        m_width = geometry.m_width;
        m_height = geometry.m_height;

        // create main window
        m_win32Handle = CreateWindow(
            s_defaultClassName, title.c_str(),
            WS_OVERLAPPEDWINDOW,
            geometry.m_posX, geometry.m_posY, geometry.m_width, geometry.m_height,
            NULL, NULL, hInstance, NULL);

        if (m_win32Handle == nullptr)
        {
            AZ_Error("Windowing", false, "Failed to create Win32 window with error: %d", GetLastError());
        }
        else
        {
            SetWindowLongPtr(m_win32Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        }
    }

    void NativeWindowImpl_Win32::Activate()
    {
        if (!m_activated)
        {
            m_activated = true;

            // This will result in a WM_SIZE message which will send the OnWindowResized notification
            ShowWindow(m_win32Handle, SW_SHOW);
            UpdateWindow(m_win32Handle);
        }
    }

    void NativeWindowImpl_Win32::Deactivate()
    {
        if (m_activated) // nothing to do if window was already deactivated
        {
            m_activated = false;

            WindowNotificationBus::Event(m_win32Handle, &WindowNotificationBus::Events::OnWindowClosed);

            ShowWindow(m_win32Handle, SW_HIDE);
            UpdateWindow(m_win32Handle);
        }
    }

    NativeWindowHandle NativeWindowImpl_Win32::GetWindowHandle() const
    {
        return m_win32Handle;
    }

    void NativeWindowImpl_Win32::SetWindowTitle(const AZStd::string& title)
    {
        SetWindowText(m_win32Handle, title.c_str());
    }

    // Handles Win32 Window Event callbacks
    LRESULT CALLBACK NativeWindowImpl_Win32::WindowCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        NativeWindowImpl_Win32* nativeWindowImpl = reinterpret_cast<NativeWindowImpl_Win32*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));      

        switch (message)
        {
        case WM_CLOSE:
        {
            nativeWindowImpl->Deactivate();
            break;
        }
        case WM_SIZE:
        {
            const uint16_t newWidth = LOWORD(lParam);
            const uint16_t newHeight = HIWORD(lParam);
            
            nativeWindowImpl->WindowSizeChanged(static_cast<uint32_t>(newWidth), static_cast<uint32_t>(newHeight));
            break;
        }
        case WM_INPUT:
        {
            UINT rawInputSize;
            const UINT rawInputHeaderSize = sizeof(RAWINPUTHEADER);
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &rawInputSize, rawInputHeaderSize);

            LPBYTE rawInputBytes = new BYTE[rawInputSize];
            const UINT bytesCopied = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBytes, &rawInputSize, rawInputHeaderSize);

            RAWINPUT* rawInput = (RAWINPUT*)rawInputBytes;
            AzFramework::RawInputNotificationBusWindows::Broadcast(
                &AzFramework::RawInputNotificationBusWindows::Events::OnRawInputEvent, *rawInput);
            break;
        }
        case WM_CHAR:
        {
            const unsigned short codeUnitUTF16 = static_cast<unsigned short>(wParam);
            AzFramework::RawInputNotificationBusWindows::Broadcast(&AzFramework::RawInputNotificationsWindows::OnRawInputCodeUnitUTF16Event, codeUnitUTF16);
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
        }

        return 0;
    }

    void NativeWindowImpl_Win32::WindowSizeChanged(const uint32_t width, const uint32_t height)
    {
        m_width = width;
        m_height = height;

        if (m_activated)
        {
            WindowNotificationBus::Event(m_win32Handle, &WindowNotificationBus::Events::OnWindowResized, width, height);
        }
    }

} // namespace AzFramework