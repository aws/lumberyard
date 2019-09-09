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

/*
    // possible non-restricted platform defines (autodetected)
    #define MCORE_PLATFORM_WINDOWS
    #define MCORE_PLATFORM_MAC
    #define MCORE_PLATFORM_IOS
    #define MCORE_PLATFORM_IOS_SIMULATOR
    #define MCORE_PLATFORM_ANDROID
    #define MCORE_PLATFORM_LINUX
    #define MCORE_PLATFORM_UNIX

    #define MCORE_PLATFORM_POSIX        // set if it is a posix system

    // are we a 32 or 64 bit architecture? (autodetected)
    #define MCORE_ARCHITECTURE_32BIT
    #define MCORE_ARCHITECTURE_64BIT

    // the architecture (autodetected) (NOTE: x64 is represented by x86 in 64 bit mode)
    #define MCORE_ARCHITECTURE_POWERPC
    #define MCORE_ARCHITECTURE_X86
    #define MCORE_ARCHITECTURE_IA64
    #define MCORE_ARCHITECTURE_ARM
    #define MCORE_ARCHITECTURE_MIPS
    #define MCORE_ARCHITECTURE_UNKNOWN
*/

#include <AzCore/PlatformDef.h>
#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/Platform_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/Platform_h_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif (defined(_WIN64) || defined(__WIN64__))
    #define MCORE_ARCHITECTURE_64BIT
    #define MCORE_PLATFORM_WINDOWS
    #define MCORE_LITTLE_ENDIAN
    #define MCORE_PLATFORM_NAME "Windows"
#elif (defined(_WIN32) || defined(__WIN32__))
    #define MCORE_ARCHITECTURE_32BIT
    #define MCORE_PLATFORM_WINDOWS
    #define MCORE_LITTLE_ENDIAN
    #define MCORE_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
        #define MCORE_PLATFORM_IOS_SIMULATOR
        #define MCORE_LITTLE_ENDIAN
        #define MCORE_PLATFORM_NAME "iOS"
    #elif TARGET_OS_IPHONE
        #define MCORE_PLATFORM_IOS
        #define MCORE_LITTLE_ENDIAN
        #define MCORE_PLATFORM_NAME "iOS"
    #else
        #define MCORE_PLATFORM_MAC
        #define MCORE_PLATFORM_NAME "Mac"
    #endif
#elif defined(__ANDROID__)
    #define MCORE_PLATFORM_ANDROID
    #define MCORE_PLATFORM_NAME "Android"
#elif (defined(linux) || defined(__linux) || defined(__linux__) || defined(__CYGWIN__))
    #define MCORE_PLATFORM_LINUX
    #define MCORE_PLATFORM_NAME "Linux"
#elif (defined(unix) || defined(__unix) || defined(__unix__))
    #define MCORE_PLATFORM_UNIX
    #define MCORE_PLATFORM_NAME "Unix"
#else
    #define MCORE_PLATFORM_UNKNOWN
    #define MCORE_PLATFORM_NAME ""
#endif

// detect posix systems
#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
    #define MCORE_PLATFORM_POSIX
#endif


// detect the architecture
#if (defined(__arm__) || defined(__thumb__) || defined(_ARM) || defined(_M_ARM) || defined(_M_ARMT) || defined(__arm) || defined(__ARMEB__) || defined(__ARMEL__))
    #define MCORE_ARCHITECTURE_ARM
    #define MCORE_ARCHITECTURE_32BIT
    #if (defined(__ARMEB__) || defined(__THUMBEB__) || defined(__BIG_ENDIAN__))
        #define MCORE_BIG_ENDIAN
    #else
        #if (defined(__ARMEL__) || defined(__THUMBEL__) || defined(__LITTLE_ENDIAN__))
            #define MCORE_LITTLE_ENDIAN
        #else
// if we can't figure out the endian, assume little endian if you didn't manually specify it
            #if (!defined(MCORE_LITTLE_ENDIAN) && !defined(MCORE_BIG_ENDIAN))
                #define MCORE_LITTLE_ENDIAN
            #endif
        #endif
    #endif
#elif (defined(__aarch64__) || defined(__AARCH64EB__) || defined(__AARCH64EL__))
    #define MCORE_ARCHITECTURE_ARM
    #define MCORE_ARCHITECTURE_64BIT
    #if (defined(__AARCH64EB__) || defined(__BIG_ENDIAN__))
        #define MCORE_BIG_ENDIAN
    #else
        #if (defined(__AARCH64EL__) || defined(__LITTLE_ENDIAN__))
            #define MCORE_LITTLE_ENDIAN
        #else
// if we can't figure out the endian, assume little endian if you didn't manually specify it
            #if (!defined(MCORE_LITTLE_ENDIAN) && !defined(MCORE_BIG_ENDIAN))
                #define MCORE_LITTLE_ENDIAN
            #endif
        #endif
    #endif
#elif (defined(__mips__) || defined(__mips) || defined(__MIPS__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__))
    #define MCORE_ARCHITECTURE_MIPS
// if you didn't manually specify this, assume it is 64 bit
    #if (!defined(MCORE_ARCHITECTURE_32BIT) && !defined(MCORE_ARCHITECTURE_64BIT))
        #define MCORE_ARCHITECTURE_64BIT
    #endif
    #if (defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__) || defined(__BIG_ENDIAN__))
        #define MCORE_BIG_ENDIAN
    #else
        #if (defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || defined(__LITTLE_ENDIAN__))
            #define MCORE_LITTLE_ENDIAN
        #else
// if we can't figure out the endian, assume little endian if you didn't manually specify it
            #if (!defined(MCORE_LITTLE_ENDIAN) && !defined(MCORE_BIG_ENDIAN))
                #define MCORE_LITTLE_ENDIAN
            #endif
        #endif
    #endif
#elif (defined(__i386) || defined(_M_IX86))
    #define MCORE_ARCHITECTURE_X86
    #define MCORE_ARCHITECTURE_32BIT
    #define MCORE_LITTLE_ENDIAN
#elif (defined(__x86_64__) || defined(_M_X64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64) || defined(_M_AMD64))
    #define MCORE_ARCHITECTURE_X86
    #define MCORE_ARCHITECTURE_64BIT
    #define MCORE_LITTLE_ENDIAN
#elif (defined(__ia64__) || defined(_IA64) || defined(__IA64__) || defined(__ia64) || defined(_M_IA64) || defined(__itanium__))
    #define MCORE_ARCHITECTURE_IA64
    #define MCORE_ARCHITECTURE_64BIT
    #if defined(__LITTLE_ENDIAN__)
        #define MCORE_LITTLE_ENDIAN
    #elif defined(__BIG_ENDIAN__)
        #define MCORE_BIG_ENDIAN
    #else
// if we didn't figure out an endian and you didn't manually specify one lets assume little endian
        #if (!defined(MCORE_LITTLE_ENDIAN) && !defined(MCORE_BIG_ENDIAN))
            #define MCORE_LITTLE_ENDIAN
        #endif
    #endif
#elif (defined(__powerpc__) || defined(__ppc__) || defined(__PPC__))
    #define MCORE_ARCHITECTURE_POWERPC
    #if (defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || defined(__64BIT__) || defined(_LP64) || defined(__LP64__))
        #define MCORE_ARCHITECTURE_64BIT
    #else
        #define MCORE_ARCHITECTURE_32BIT
    #endif
    #define MCORE_BIG_ENDIAN
#else
    #define MCORE_ARCHITECTURE_UNKNOWN
    #define MCORE_ARCHITECTURE_64BIT

    #if defined(__LITTLE_ENDIAN__)
        #define MCORE_LITTLE_ENDIAN
    #elif defined(__BIG_ENDIAN__)
        #define MCORE_BIG_ENDIAN
    #else
// if we don't know what endian it is and you didn't manually set one, just default it to little endian
        #if (!defined(MCORE_LITTLE_ENDIAN) && !defined(MCORE_BIG_ENDIAN))
            #define MCORE_LITTLE_ENDIAN
        #endif
    #endif
#endif


// enable evaluation mode
//#define MCORE_EVALUATION


// disable the networked-license system when this is not an evaluation build
// platforms that don't support the networked license system automatically disable
// it as well, so you don't have to deal with that
#ifndef MCORE_EVALUATION
    #define MCORE_NO_LICENSESYSTEM
#endif

// if we're not using windows, make sure we don't use the license system
#ifndef MCORE_PLATFORM_WINDOWS
    #ifndef MCORE_NO_LICENSESYSTEM
        #define MCORE_NO_LICENSESYSTEM
    #endif
#endif
