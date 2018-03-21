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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER              // Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0600 // Include Vista-specific functions
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x600      // Allow use of features specific to Windows Vista
#endif

#ifndef _WIN32_WINDOWS      // Allow use of features specific to Windows 98 or later.
//#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE           // Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0501    // Change this to the appropriate value to target IE 5.0 or later.
#endif

// prevent inclusion of conflicting definitions of INT8_MIN etc
#define _INTSAFE_H_INCLUDED_

#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif

#ifdef APPLE
#include <QUuid>
#endif

#define RWI_NAME_TAG "RayWorldIntersection(Editor)"
#define PWI_NAME_TAG "PrimitiveWorldIntersection(Editor)"

#define _CRT_RAND_S
#define _HAS_EXCEPTIONS 1

#ifndef NOT_USE_CRY_MEMORY_MANAGER
// do not let QListData::realloc use QListData::CryModuleCRTRealloc
#ifdef realloc
#undef realloc
#include <QList>
#define realloc CryModuleCRTRealloc
#endif
#include <QList>
#endif

#include <platform.h>

#include "ProjectDefines.h"

#pragma warning(disable: 4103)  // Alignment change after include
#pragma warning(once: 4264) // Virtual function override warnings, as MFC headers do not pass them.
#pragma warning(disable: 4266)
#pragma warning(once : 4263)

#ifdef NOMINMAX
#include "Cry_Math.h"
#endif //NOMINMAX


// Resource includes
#include "Resource.h"

#ifdef _DEBUG

#if defined(AZ_PLATFORM_WINDOWS) && !defined(WIN64)
#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>
//#define   calloc(s,t)       _calloc_dbg(s, t, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
//#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif //WIN64


//////////////////////////////////////////////////////////////////////////
// Main Editor include.
//////////////////////////////////////////////////////////////////////////
#include "EditorDefs.h"

#ifdef _DEBUG
#ifdef assert
#undef assert
#define assert CRY_ASSERT
#endif
#endif

#include "CryEdit.h"
#include "EditTool.h"
#include "PluginManager.h"

#include "Util/Variable.h"

#include <IGame.h>
#include <ISplines.h>
#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <CryListenerSet.h>

#define USE_PYTHON_SCRIPTING

#ifdef max
#undef max
#undef min
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef RemoveDirectory
#undef RemoveDirectory
#endif

#ifdef CopyFile
#undef CopyFile
#endif

#ifdef GetUserName
#undef GetUserName
#endif

#ifdef LoadCursor
#undef LoadCursor
#endif
