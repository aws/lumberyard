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

#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/Component.h>

// Temporary workaround for allocator issues. We can't yet use aznew in all build configurations.
// For more details, see comments at the top of ToolsApplication.cpp.
#ifdef aznew
#   undef aznew
#   define aznew new
#endif

// QT
#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#include <QtGui/qevent.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qtoolbutton.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qcolordialog.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qscrollarea.h>
#include <QtWidgets/qinputdialog.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qobjectdefs.h>

#ifdef Q_OS_MACOS
typedef void* HWND;
typedef void* HMODULE;
typedef quint32 DWORD;
#define _MAX_PATH 260
#define MAX_PATH 260
#endif
