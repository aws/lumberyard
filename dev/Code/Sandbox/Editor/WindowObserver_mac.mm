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

#include "WindowObserver_mac.h"

#include <AppKit/AppKit.h>

#include <QApplication>
#include <QtGui/qpa/qplatformnativeinterface.h>


@interface NSWindowObserver : NSObject
{
    Editor::WindowObserver* m_observer;
}
@end

@implementation NSWindowObserver

- (id)initWithWindow:(NSWindow*)window :(Editor::WindowObserver*)observer
{
    if ((self = [super init])) {
        m_observer = observer;

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onWindowWillMove)
                                                     name:NSWindowWillMoveNotification
                                                   object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onWindowDidMove)
                                                     name:NSWindowDidMoveNotification
                                                   object:window];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onWindowWillStartLiveResize)
                                                     name:NSWindowWillStartLiveResizeNotification
                                                   object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onWindowDidLiveResize)
                                                     name:NSWindowDidEndLiveResizeNotification
                                                   object:window];

    }
    return self;
}

-(void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)onWindowWillMove
{
    m_observer->setWindowIsMoving(true);
}

- (void)onWindowDidMove
{
    m_observer->setWindowIsMoving(false);
}

- (void)onWindowWillStartLiveResize
{
    m_observer->setWindowIsResizing(true);
}

- (void)onWindowDidLiveResize
{
    m_observer->setWindowIsResizing(false);
}

@end

namespace Editor
{
    WindowObserver::WindowObserver(QWindow* window, QObject* parent)
        : QObject(parent)
        , m_windowIsMoving(false)
        , m_windowIsResizing(false)
        , m_windowObserver(nullptr)
    {
        if (!window)
        {
            return;
        }

        QPlatformNativeInterface* nativeInterface = QApplication::platformNativeInterface();
        NSWindow* nsWindow = static_cast<NSWindow*>(nativeInterface->nativeResourceForWindow("nswindow", window));
        if (nsWindow == nullptr)
        {
            return;
        }

        m_windowObserver = [[NSWindowObserver alloc] initWithWindow:nsWindow:this];
    }

    WindowObserver::~WindowObserver()
    {
        if (m_windowObserver != nil)
        {
            [m_windowObserver release];
            m_windowObserver = nil;
        }
    }

    void WindowObserver::setWindowIsMoving(bool isMoving)
    {
        if (isMoving == m_windowIsMoving)
        {
            return;
        }

        const bool wasMovingOrResizing = m_windowIsMoving || m_windowIsResizing;
        m_windowIsMoving = isMoving;
        const bool isMovingOrResizing = m_windowIsMoving || m_windowIsResizing;

        if (isMovingOrResizing != wasMovingOrResizing)
        {
            emit windowIsMovingOrResizingChanged(isMovingOrResizing);
        }
    }

    void WindowObserver::setWindowIsResizing(bool isResizing)
    {
        if (isResizing == m_windowIsResizing)
        {
            return;
        }

        const bool wasMovingOrResizing = m_windowIsMoving || m_windowIsResizing;
        m_windowIsResizing = isResizing;
        const bool isMovingOrResizing = m_windowIsMoving || m_windowIsResizing;

        if (isMovingOrResizing != wasMovingOrResizing)
        {
            emit windowIsMovingOrResizingChanged(isMovingOrResizing);
        }
    }
}

#include <WindowObserver_mac.moc>
