
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
#import <AppKit/NSEvent.h>

#include "StdAfx.h"

#include "QtEditorApplication.h"

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_darwin.h>

#include <QCursor>

namespace Editor
{
    bool EditorQtApplication::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
    {
        NSEvent* event = (NSEvent*)message;
        if (GetIEditor()->IsInGameMode())
        {
            EBUS_EVENT(AzFramework::RawInputNotificationBusOsx, OnRawInputEvent, event);
            return true;
        }

        if ([event type] == NSEventTypeMouseEntered)
        {
            // filter out mouse enter events when the widget the mouse entered into
            // does not exist anymore. This happened occasionally on startup when hovering
            // over the splash screen - LY-84100
            if (widgetAt(QCursor::pos()) == nullptr)
            {
                return true;
            }
        }

        return false;
    }
} // end namespace Editor
