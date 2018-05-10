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


/*!
    CryLibrary

    Convenience-Macros which abstract the use of DLLs/shared libraries in a platform independent way.
    A short explanation of the different macros follows:

    CrySharedLibrarySupported:
        This macro can be used to test if the current active platform supports shared library calls. The default
        value is false. This gets redefined if a certain platform (WIN32 or LINUX) is desired.

    CrySharedLibraryPrefix:
        The default prefix which will get prepended to library names in calls to CryLoadLibraryDefName
        (see below).

    CrySharedLibraryExtension:
        The default extension which will get appended to library names in calls to CryLoadLibraryDefName
        (see below).

    CryLoadLibrary(libName):
        Loads a shared library.

    CryLoadLibraryDefName(libName):
        Loads a shared library. The platform-specific default library prefix and extension are appended to the libName.
        This allows writing of somewhat platform-independent library loading code and is therefore the function
        which should be used most of the time, unless some special extensions are used (e.g. for plugins).

    CryGetProcAddress(libHandle, procName):
        Import function from the library presented by libHandle.

    CryFreeLibrary(libHandle):
        Unload the library presented by libHandle.

    HISTORY:
        03.03.2004 MarcoK
            - initial version
            - added to CryPlatform
*/

#include <stdio.h>
#include <AzCore/PlatformDef.h>

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(CryLibrary_h, AZ_RESTRICTED_PLATFORM)
#elif defined(WIN32)
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <CryWindows.h>
    #define CryLoadLibrary(libName) ::LoadLibraryA(libName)
    #define CRYLIBRARY_H_TRAIT_USE_WINDOWS_DLL 1
#elif defined(LINUX) || defined(AZ_PLATFORM_APPLE)
    #include <dlfcn.h>
    #include <stdlib.h>
    #include <libgen.h>
    #include "platform.h"
    #include <AzCore/Debug/Trace.h>

// for compatibility with code written for windows
    #define CrySharedLibrarySupported true
    #define CrySharedLibraryPrefix "lib"
#if defined(AZ_PLATFORM_APPLE)
    #include <mach-o/dyld.h>
    #define CrySharedLibraryExtension ".dylib"
#else
    #define CrySharedLibraryExtension ".so"
#endif

    #define CryGetProcAddress(libHandle, procName) ::dlsym(libHandle, procName)
        #define CryFreeLibrary(libHandle)  (::dlclose(libHandle) == 0)
    #define HMODULE void*
static const char* gEnvName("MODULE_PATH");

static const char* GetModulePath()
{
    return getenv(gEnvName);
}

static void SetModulePath(const char* pModulePath)
{
    setenv(gEnvName, pModulePath ? pModulePath : "", true);
}

static HMODULE CryLoadLibrary(const char* libName, bool bLazy = false, bool bInModulePath = true)
{
    char path[_MAX_PATH] = { 0 };

#if !defined(AZ_PLATFORM_ANDROID)
    if (bInModulePath)
    {
        const char* modulePath = GetModulePath();
        if (!modulePath)
        {
            modulePath = ".";
            #if defined(LINUX)
                char exePath[MAX_PATH + 1] = { 0 };
                int len = readlink("/proc/self/exe", exePath, MAX_PATH);
                if (len != -1)
                {
                    exePath[len] = 0;
                    modulePath = dirname(exePath);
                }
            #elif defined(AZ_PLATFORM_APPLE)
                uint32_t bufsize = MAX_PATH;
                char exePath[MAX_PATH + 1] = { 0 };
                if (_NSGetExecutablePath(exePath, &bufsize) == 0)
                {
                    exePath[bufsize] = 0;
                    modulePath = dirname(exePath);
                }
            #endif
        }
        sprintf_s(path, "%s/%s", modulePath, libName);
        libName = path;
    }
#endif

    HMODULE retVal;
#if defined(LINUX) && !defined(ANDROID)
    retVal = ::dlopen(libName, (bLazy ? RTLD_LAZY : RTLD_NOW) | RTLD_DEEPBIND);
#else
    retVal = ::dlopen(libName, bLazy ? RTLD_LAZY : RTLD_NOW);
#endif
    AZ_Warning("LMBR", retVal, "Can't load library [%s]: %s", libName, dlerror());
    return retVal;
}
#endif

#if CRYLIBRARY_H_TRAIT_USE_WINDOWS_DLL
#define CrySharedLibrarySupported true
#define CrySharedLibraryPrefix ""
#define CrySharedLibraryExtension ".dll"
#define CryGetProcAddress(libHandle, procName) ::GetProcAddress((HMODULE)(libHandle), procName)
#define CryFreeLibrary(libHandle) ::FreeLibrary((HMODULE)(libHandle))
#elif !defined(CrySharedLibrarySupported)
#define CrySharedLibrarySupported false
#define CrySharedLibraryPrefix ""
#define CrySharedLibraryExtension ""
#define CryLoadLibrary(libName) NULL
#define CryGetProcAddress(libHandle, procName) NULL
#define CryFreeLibrary(libHandle)
#define GetModuleHandle(x)  0
#endif
#define CryLibraryDefName(libName) CrySharedLibraryPrefix libName CrySharedLibraryExtension
#define CryLoadLibraryDefName(libName) CryLoadLibrary(CryLibraryDefName(libName))
