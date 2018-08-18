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

// Description : Precompiled Header.


#pragma once

#include <AzCore/PlatformIncl.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#include <AzCore/PlatformRestrictedFileDef.h>
#undef AZ_RESTRICTED_SECTION
#define STDAFX_H_SECTION_1 1
#define STDAFX_H_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_1
#include AZ_RESTRICTED_FILE(StdAfx_h, AZ_RESTRICTED_PLATFORM)
#elif defined(LINUX) // Scrubber friendly negated define pattern
#elif !defined(APPLE)
    #include <memory.h>
    #include <malloc.h>
#endif

//on mac the precompiled header is auto included in every .c and .cpp file, no include line necessary.
//.c files don't like the cpp things in here
#ifdef __cplusplus

#include <vector>

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_2
#include AZ_RESTRICTED_FILE(StdAfx_h, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(APPLE) // Scrubber friendly negated define pattern
#elif defined(ANDROID) // Scrubber friendly negated define pattern
#elif defined(LINUX)
    #   include <sys/io.h>
#else
    #   include <io.h>
#endif

//#define DEFINE_MODULE_NAME "CrySystem"

#define CRYSYSTEM_EXPORTS

#include <platform.h>

#if defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX)
#if defined(DEDICATED_SERVER)
// enable/disable map load slicing functionality from the build
#define MAP_LOADING_SLICING
#endif
#endif

#ifdef WIN32
#include <CryWindows.h>
#include <tlhelp32.h>
#undef GetCharWidth
#undef GetUserName
#endif

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "Cry_Math.h"
#include <Cry_Camera.h>
#include <smartptr.h>
#include <Range.h>
#include <CrySizer.h>
#include <StlUtils.h>


static inline int RoundToClosestMB(size_t memSize)
{
    // add half a MB and shift down to get closest MB
    return((int) ((memSize + (1 << 19)) >> 20));
}


//////////////////////////////////////////////////////////////////////////
// For faster compilation
//////////////////////////////////////////////////////////////////////////
#include <IRenderer.h>
#include <CryFile.h>
#include <ISystem.h>
#include <IScriptSystem.h>
#include <IEntitySystem.h>
#include <I3DEngine.h>
#include <ITimer.h>
#include <IAudioSystem.h>
#include <IPhysics.h>
#include <IAISystem.h>
#include <IXml.h>
#include <ICmdLine.h>
#include <IConsole.h>
#include <ILog.h>

/////////////////////////////////////////////////////////////////////////////
//forward declarations for common Interfaces.
/////////////////////////////////////////////////////////////////////////////
class ITexture;
struct IRenderer;
struct ISystem;
struct IScriptSystem;
struct ITimer;
struct IFFont;
struct IInput;
struct IKeyboard;
struct ICVar;
struct IConsole;
struct IGame;
struct IEntitySystem;
struct IProcess;
struct ICryPak;
struct ICryFont;
struct I3DEngine;
struct IMovieSystem;
struct IAudioSystem;
struct IPhysicalWorld;

#endif //__cplusplus



