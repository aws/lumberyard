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
#if !defined(AZCORE_EXCLUDE_LUA)

#include <stdlib.h>

// Functions that Lua expects to be externally defined
float script_frand0_1()
{
    return (float)rand() / (float)(RAND_MAX);
}

void script_randseed(unsigned int seed)
{
    srand(seed);
}

// AZCORE_COMPILES_LUA means Lua source is compiled by AzCore.
// If undefined then Lua must be in a linked library.
#if defined (AZCORE_COMPILES_LUA)

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable:4324) // structure was padded due to __declspec(align())
#   pragma warning(disable:4702) // unreachable code
#endif

// Note: First merge with the CryLua, we should probably remove loslib, print, etc. libs that we already provide functionality for

// Handle CryTek defines
#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/lua_c_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/lua_c_provo.inl"
    #endif
#endif

#if defined(__ANDROID__)
#   ifndef ANDROID
#       define ANDROID
#   endif
#endif

#if defined(__APPLE__)
#   ifndef APPLE
#       define APPLE
#   endif
#endif

#if defined(__linux__)
#   ifndef LINUX
#       define LINUX
#   endif
#endif

#   include <Lua/lapi.c>
#   include <Lua/lauxlib.c>
#   include <Lua/lbaselib.c>
#   include <Lua/lbitlib.c>
#   include <Lua/lcode.c>
#   include <Lua/ldblib.c>
#   include <Lua/ldo.c>
#   include <Lua/ldump.c>
#   include <Lua/lfunc.c>
#   include <Lua/lgc.c>
#   include <Lua/linit.c>
#   include <Lua/liolib.c>
#   include <Lua/llex.c>
#   include <Lua/lmathlib.c>
#   include <Lua/lmem.c>
#   include <Lua/loadlib.c>
#   include <Lua/lobject.c>
#   include <Lua/lopcodes.c>
#   include <Lua/lparser.c>
#   include <Lua/lstate.c>
#   include <Lua/lstring.c>
#   include <Lua/lstrlib.c>
#   include <Lua/ltable.c>
#   include <Lua/ltablib.c>
#   include <Lua/ltm.c>
#   include <Lua/lundump.c>
#   include <Lua/lvm.c>
#   include <Lua/lzio.c>
#   include <Lua/print.c>
#   include <Lua/ldebug.c>

#define LUA_TMPNAMBUFSIZE   L_tmpnam
#define lua_tmpnam(b, e)     { e = (tmpnam(b) == NULL); }
#   include <Lua/loslib.c>

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#endif // AZCORE_COMPILES_LUA
#endif // AZCORE_EXCLUDE_LUA
