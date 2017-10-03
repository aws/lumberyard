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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#if 1

#include <tchar.h>

// DH Stuff:
#include <AzCore/base.h>
#include <AzCore/platformincl.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/debug/trace.h>
#include <AzCore/std/time.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/ring_buffer.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/streamer.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <WinIoCtl.h>

// QT

// Warning 4512: Assignment operator could not be generated - disabled because QT forgets to make a private assignment operator
// when they want to prohibit copy.  they don't forget every time, just a few times, but that's enough to cause spam.
#pragma warning(push)
#pragma warning(disable : 4512)
#pragma warning(disable : 4189)
#pragma warning(disable : 4718) // warning C4718: 'QMapNode<int,bool>::destroySubTree' : recursive call has no side effects, deleting
#include <QtWidgets/QtWidgets>
#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/qmap.h>
#include <QtQml/QtQml>
#pragma warning(pop)

#endif


namespace LuaIDEMetrics
{
    namespace Category
    {
        static const char* EventName = "LuaIDE";
    }

    static const char* Operation = "Operation";

    namespace Events
    {
        static const char* OpenEditor = "OpenEditor";
        static const char* CloseEditor = "CloseEditor";
        static const char* FileCreated = "FileCreated";
        static const char* FileOpened = "FileOpened";
        static const char* FileSaved = "FileSaved";
        static const char* StartDebugger = "StartDebugger";
        static const char* StopDebugger = "StopDebugger";
        static const char* DebuggerRefused = "DebuggerRefused";
        static const char* TargetConnected = "TargetConnected";
        static const char* TargetDisconnected = "TargetDisconnected";
        static const char* BreakpointHit = "BreakpointHit";
    }
}