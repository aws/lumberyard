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

// include standard core headers
#include "Platform.h"
#include "Config.h"
#include "Macros.h"
#include "MemoryCategoriesCore.h"

// standard includes
#include <math.h>
#include <string.h>
#include <stdlib.h>
//#include <limits.h>
#include <limits>
#include <assert.h>
#include <cstdio> // stdio.h doesn't work for std::rename and std::remove on all platforms
#include <float.h>
#include <stdarg.h>
#include <wchar.h>
//#include <complex.h>

#if (MCORE_COMPILER != MCORE_COMPILER_CODEWARRIOR)
//  #include <malloc.h>
//  #include <memory.h>
#endif

#if (MCORE_COMPILER != MCORE_COMPILER_GCC)
    #include <new.h>
#else
    #include <new>
    #include <ctype.h>  // strupr and strlwr
#endif


// include the Windows header
#if defined(MCORE_PLATFORM_WINDOWS)
/*
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0500
    #endif
    #ifndef WIN32
        #define WIN32
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
*/
    #include <windows.h>
#endif


#ifdef MCORE_PLATFORM_ANDROID
    #include <time.h>
    #include <android/log.h>
#endif


#ifdef MCORE_SSE_ENABLED
    #include <intrin.h>
    #ifdef AZ_COMPILER_MSVC
        #pragma intrinsic( _mm_hadd_ps )
    #endif
#endif

// include the system
#include "MCoreSystem.h"
#include "MemoryManager.h"
