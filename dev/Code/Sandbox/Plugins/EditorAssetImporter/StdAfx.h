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

#include <QtGlobal>
#include <QUuid>
#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_APPLE)
#include "AzCore/Math/Guid.h"
#endif

#if defined(AZ_PLATFORM_WINDOWS)
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0600 // Change this to the appropriate value to target other versions of Windows.
#endif

#include <afxwin.h>
#include <afxext.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// CryTek
/////////////////////////////////////////////////////////////////////////////
#include <platform.h>

/////////////////////////////////////////////////////////////////////////////
// Qt
/////////////////////////////////////////////////////////////////////////////
#include <qglobal.h>
#include <QtUtil.h>
#include <QString>
#include <QList>