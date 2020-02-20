
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

#include <AppKit/AppKit.h>

@class NSWindow;

namespace AzFramework
{
    class NativeWindowImpl_Darwin final
        : public NativeWindow::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(NativeWindowImpl_Darwin, AZ::SystemAllocator, 0);
        NativeWindowImpl_Darwin() = default;
        ~NativeWindowImpl_Darwin() override;
        
        // NativeWindow::Implementation overrides...
        void InitWindow(const AZStd::string& title, const WindowGeometry& geometry, const WindowType windowType) override;
        NativeWindowHandle GetWindowHandle() const override;
        void SetWindowTitle(const AZStd::string& title) override;

    private:
        NSWindow* m_nativeWindow;
        NSString* m_windowTitle;
    };

    NativeWindow::Implementation* NativeWindow::Implementation::Create()
    {
        return aznew NativeWindowImpl_Darwin();
    }

    NativeWindowImpl_Darwin::~NativeWindowImpl_Darwin()
    {
        [m_nativeWindow release];
        m_nativeWindow = nil;
    }

    void NativeWindowImpl_Darwin::InitWindow(const AZStd::string& title, const WindowGeometry& geometry, const WindowType windowType)
    {
        AZ_UNUSED(windowType);

        m_width = geometry.m_width;
        m_height = geometry.m_height;

        SetWindowTitle(title);

        CGRect screenBounds = CGRectMake(geometry.m_posX, geometry.m_posY, geometry.m_width, geometry.m_height);
        
        //Create the window
        NSUInteger styleMask = NSWindowStyleMaskClosable|NSWindowStyleMaskTitled|NSWindowStyleMaskMiniaturizable;
        m_nativeWindow = [[NSWindow alloc] initWithContentRect: screenBounds styleMask: styleMask backing: NSBackingStoreBuffered defer:false];
        
        //Make the window active
        [m_nativeWindow makeKeyAndOrderFront:nil];
        m_nativeWindow.title = m_windowTitle;
        
        bool fullScreen = false; //Todo: This need to be set by higher level code
        if(fullScreen)
        {
            [m_nativeWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
            [m_nativeWindow toggleFullScreen:nil];
            [m_nativeWindow setFrame:screenBounds display: true animate: true];
        }
        else
        {
            CGRect windowFrame = [m_nativeWindow frame];
            [m_nativeWindow setFrame:NSMakeRect(0, 0, NSWidth(windowFrame), NSHeight(windowFrame)) display:true];
        }
    }

    NativeWindowHandle NativeWindowImpl_Darwin::GetWindowHandle() const
    {
        return m_nativeWindow;
    }

    void NativeWindowImpl_Darwin::SetWindowTitle(const AZStd::string& title)
    {
        m_windowTitle = [NSString stringWithCString:title.c_str() encoding:NSUTF8StringEncoding];
        m_nativeWindow.title = m_windowTitle;
    }

} // namespace AzFramework
