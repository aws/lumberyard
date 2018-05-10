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

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <wchar.h>
#include "Platform.h"
#include <stdint.h>

// compilers
#define MCORE_COMPILER_MSVC         1
#define MCORE_COMPILER_INTELC       2
#define MCORE_COMPILER_CODEWARRIOR  3
#define MCORE_COMPILER_GCC          4
#define MCORE_COMPILER_MINGW        5
#define MCORE_COMPILER_CLANG        6
#define MCORE_COMPILER_LLVM         7
#define MCORE_COMPILER_SNC          8


// finds the compiler type and version
#if (defined(__GNUC__) || defined(__GNUC) || defined(__gnuc))
    #define MCORE_COMPILER MCORE_COMPILER_GCC
    #if defined(__GNUC_PATCHLEVEL__)
        #define MCORE_COMPILER_VERSION_PATCH __GNUC_PATCHLEVEL__
        #define __GNUC_VERSION__ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #else
        #define MCORE_COMPILER_VERSION_PATCH 0
        #define __GNUC_VERSION__ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
    #endif
    #define MCORE_COMPILER_VERSION_MAJOR __GNUC__
    #define MCORE_COMPILER_VERSION_MINOR __GNUC_MINOR__
//#define MCORE_COMPILER_VERSION __VERSION__
#elif (defined(__ICL) || defined(__INTEL_COMPILER))
    #define MCORE_COMPILER MCORE_COMPILER_INTELC
    #define MCORE_COMPILER_VERSION_MAJOR __INTEL_COMPILER
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#elif defined(__MINGW64__)
    #define MCORE_COMPILER MCORE_COMPILER_MINGW
    #if defined(__MINGW32_MAJOR_VERSION)
        #define MCORE_COMPILER_VERSION_MAJOR __MINGW64_MAJOR_VERSION
        #define MCORE_COMPILER_VERSION_MINOR __MINGW64_MAJOR_VERSION
        #define MCORE_COMPILER_VERSION_PATCH 0
    #else
        #define MCORE_COMPILER_VERSION_MAJOR 0
        #define MCORE_COMPILER_VERSION_MINOR 0
        #define MCORE_COMPILER_VERSION_PATCH 0
    #endif
#elif defined(__SNC__)
    #define MCORE_COMPILER MCORE_COMPILER_SNC
    #define MCORE_COMPILER_VERSION_MAJOR 0
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#elif defined(__llvm__)
    #define MCORE_COMPILER MCORE_COMPILER_LLVM
    #define MCORE_COMPILER_VERSION_MAJOR 0
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#elif defined(__clang__)
    #define MCORE_COMPILER MCORE_COMPILER_CLANG
    #define MCORE_COMPILER_VERSION_MAJOR __clang_major__
    #define MCORE_COMPILER_VERSION_MINOR __clang_minor__
    #define MCORE_COMPILER_VERSION_PATCH __clang_patchlevel__
#elif (defined(__MWERKS__) || defined(__CWCC__))
    #define MCORE_COMPILER MCORE_COMPILER_CODEWARRIOR
    #if defined(__CWCC__)
        #define MCORE_COMPILER_VERSION_MAJOR __CWCC__
    #else
        #define MCORE_COMPILER_VERSION_MAJOR __MWERKS__
    #endif
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#elif defined(__MINGW32__)
    #define MCORE_COMPILER MCORE_COMPILER_MINGW
    #if defined(__MINGW32_MAJOR_VERSION)
        #define MCORE_COMPILER_VERSION_MAJOR __MINGW32_MAJOR_VERSION
        #define MCORE_COMPILER_VERSION_MINOR __MINGW32_MINOR_VERSION
        #define MCORE_COMPILER_VERSION_PATCH 0
    #else
        #define MCORE_COMPILER_VERSION_MAJOR 0
        #define MCORE_COMPILER_VERSION_MINOR 0
        #define MCORE_COMPILER_VERSION_PATCH 0
    #endif
#elif defined(_MSC_VER)
    #define MCORE_COMPILER MCORE_COMPILER_MSVC
    #define MCORE_COMPILER_VERSION_MAJOR _MSC_VER
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
    #if (MCORE_COMPILER_VERSION_MAJOR <= 1700)  // allow more variadic template parameters for VS2012
        #ifndef _VARIADIC_MAX
            #define _VARIADIC_MAX 10
        #endif
    #endif
#else
// unsupported compiler!
    #define MCORE_COMPILER MCORE_COMPILER_UNKNOWN
    #define MCORE_COMPILER_VERSION 0
#endif


// disable conversion compile warning
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC)
    #pragma warning (disable : 4324)    // structure was padded due to __declspec(align())
    #pragma warning (disable : 4251)    // class 'type' needs to have dll-interface to be used by clients of class 'type2'

// disable this warning only on VS2010 or older
    #if (MCORE_COMPILER_VERSION_MAJOR <= 1600)
        #pragma warning (disable : 4481)    // nonstandard extension used: override specifier 'override'
    #endif
#endif


// define the basic types
#include <BaseTypes.h>


//---------------------
// stringised version of line number (must be done in two steps)
#define MCORE_STRINGISE(N) #N
#define MCORE_EXPAND_THEN_STRINGISE(N) MCORE_STRINGISE(N)
#define MCORE_LINE_STR MCORE_EXPAND_THEN_STRINGISE(__LINE__)

// MSVC-suitable routines for formatting <#pragma message>
#define MCORE_LOC __FILE__ "(" MCORE_LINE_STR ")"
#define MCORE_OUTPUT_FORMAT(type) MCORE_LOC " : [" type "] "

// specific message types for <#pragma message>
#define MCORE_WARNING MCORE_OUTPUT_FORMAT("WARNING")
#define MCORE_ERROR MCORE_OUTPUT_FORMAT("ERROR")
#define MCORE_MESSAGE MCORE_OUTPUT_FORMAT("INFO")
#define MCORE_INFO MCORE_OUTPUT_FORMAT("INFO")
#define MCORE_TODO MCORE_OUTPUT_FORMAT("TODO")

// USAGE:
// #pragma message ( MCORE_MESSAGE "my message" )
//---------------------

// some special types that are missing inside Visual C++ 6
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC)
    #if (MCORE_COMPILERVERSION < 1300)
typedef unsigned long uintPointer;
    #else
        #include <stddef.h>
typedef uintptr_t uintPointer;
    #endif
#else
    #include <stddef.h>
    #include <stdint.h>
typedef uintptr_t uintPointer;
#endif


// disable conversion compile warning
#if (MCORE_COMPILER == MCORE_COMPILER_INTELC)
    #pragma warning(disable : 1744)     // about missing DLL export declare inside a class that gets exported, but this is triggered when using templates
//  #pragma warning(disable : 3199)     // defined is always false in a macro expansion in Microsoft mode (used to disable some Qt warnings)
#endif


// set the inline macro
// we want to use __inline for Visual Studio, so that we can still enable these inline functions in debug mode
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
    #define MCORE_INLINE __inline
    #define MCORE_FORCEINLINE __forceinline
#elif (MCORE_COMPILER == MCORE_COMPILER_GCC)
    #define MCORE_INLINE __inline
    #define MCORE_FORCEINLINE __attribute__((always_inline))
#else
    #define MCORE_INLINE inline
    #define MCORE_FORCEINLINE inline
#endif


// debug macro
#ifdef _DEBUG
    #define MCORE_DEBUG
#endif

// date define
#define MCORE_WIDEN2(x) L ## x
#define MCORE_WIDEN(x) MCORE_WIDEN2(x)
#define __WDATE__ MCORE_WIDEN(__DATE__)
#define MCORE_WDATE __WDATE__
#define MCORE_DATE __DATE__

// time define
#define __WTIME__ MCORE_WIDEN(__TIME__)
#define MCORE_WTIME __WTIME__
#define MCORE_TIME __TIME__


#ifdef _DEBUG
    #define __WFILE__ MCORE_WIDEN(__FILE__)
    #define MCORE_WFILE __WFILE__
    #define MCORE_WLINE __LINE__
    #define MCORE_FILE __FILE__
    #define MCORE_LINE __LINE__
#else
    #define MCORE_FILE nullptr
    #define MCORE_LINE 0xFFFFFFFF
    #define MCORE_WFILE nullptr
    #define MCORE_WLINE 0xFFFFFFFF
#endif


// define the __cdecl
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC || MCORE_COMPILER == MCORE_COMPILER_CLANG)
    #define MCORE_CDECL __cdecl
#else
    #define MCORE_CDECL
#endif


// endian conversion, defaults to big endian
#if defined(MCORE_BIG_ENDIAN)
    #define MCORE_FROM_LITTLE_ENDIAN16(buffer, count)   MCore::Endian::ConvertEndian16(buffer, count)
    #define MCORE_FROM_LITTLE_ENDIAN32(buffer, count)   MCore::Endian::ConvertEndian32(buffer, count)
    #define MCORE_FROM_LITTLE_ENDIAN64(buffer, count)   MCore::Endian::ConvertEndian64(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN16(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN32(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN64(buffer, count)
#else   // little endian
    #define MCORE_FROM_LITTLE_ENDIAN16(buffer, count)
    #define MCORE_FROM_LITTLE_ENDIAN32(buffer, count)
    #define MCORE_FROM_LITTLE_ENDIAN64(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN16(buffer, count)  MCore::Endian::ConvertEndian16(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN32(buffer, count)  MCore::Endian::ConvertEndian32(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN64(buffer, count)  MCore::Endian::ConvertEndian64(buffer, count)
#endif

// setup if we support SSE or not
// we assume here that all x86 machines support SSE today
#if (defined(MCORE_PLATFORM_WINDOWS) && (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC))
    #define MCORE_SSE_ENABLED
#endif

#ifndef NULL
    #define NULL 0
#endif

// alignment macro
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
    #define MCORE_ALIGN(NUMBYTES, X) __declspec(align(NUMBYTES)) X
    #define MCORE_ALIGN_PRE(NUMBYTES) __declspec(align(NUMBYTES))
    #define MCORE_ALIGN_POST(NUMBYTES)
#elif (MCORE_COMPILER == MCORE_COMPILER_GCC)
    #define MCORE_ALIGN(NUMBYTES, X) X __attribute__((aligned(NUMBYTES)))
    #define MCORE_ALIGN_PRE(NUMBYTES)
    #define MCORE_ALIGN_POST(NUMBYTES) __attribute__((aligned(NUMBYTES)))
#else
    #define MCORE_ALIGN(NUMBYTES, X) X
    #define MCORE_ALIGN_PRE(NUMBYTES)
    #define MCORE_ALIGN_POST(NUMBYTES)
#endif


// detect and enable OpenMP support
#if defined(_OPENMP)
    #define MCORE_OPENMP_ENABLED
#endif

// shared lib import/export
#if defined(MCORE_PLATFORM_WINDOWS)
    #define MCORE_EXPORT __declspec(dllexport)
    #define MCORE_IMPORT __declspec(dllimport)
#else
    #define MCORE_EXPORT __attribute__((visibility("default")))
    #define MCORE_IMPORT
#endif


// shared lib importing/exporting
#if defined(MCORE_DLL_EXPORT)
    #define MCORE_API       MCORE_EXPORT
#else
    #if defined(MCORE_DLL)
        #define MCORE_API   MCORE_IMPORT
    #else
        #define MCORE_API
    #endif
#endif


// check if fast float math operations such as sinf etc are available, or if we need to stick with standard calls to sin etc
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(Config_h, AZ_RESTRICTED_PLATFORM)
#elif ((defined(MCORE_PLATFORM_WINDOWS) || defined(MCORE_PLATFORM_MAC) || defined(MCORE_PLATFORM_IPHONE) || defined(MCORE_PLATFORM_ANDROID)))
    #define MCORE_FASTFLOAT_MATH
#endif


// define a custom assert macro
#ifndef MCORE_NO_ASSERT
    #define MCORE_ASSERT(x) assert(x)
#else
    #define MCORE_ASSERT(x)
#endif


// check wchar_t size
#if (WCHAR_MAX > 0xFFFFu)
    #define MCORE_UTF32
#elif (WCHAR_MAX > 0xFFu)
    #define MCORE_UTF16
#else
    #define MCORE_UTF8
// ERROR: unsupported
    #error "MCore: sizeof(wchar_t)==8, while MCore does not support this!"
#endif

// mark as unused to prevent compiler warnings
#define MCORE_UNUSED(x) static_cast<void>(x)

/**
 * Often there are functions that allow you to search for objects. Such functions return some index value that points
 * inside for example the array of objects. However, in case the object we are searching for cannot be found, some
 * value has to be returned that identifies that the object cannot be found. The MCORE_INVALIDINDEX32 value is used
 * used as this value. The real value is 0xFFFFFFFF.
 */
#define MCORE_INVALIDINDEX32 0xFFFFFFFF

/**
 * The 16 bit index variant of MCORE_INVALIDINDEX32.
 * The real value is 0xFFFF.
 */
#define MCORE_INVALIDINDEX16 0xFFFF

/**
 * The 8 bit index variant of MCORE_INVALIDINDEX32.
 * The real value is 0xFF.
 */
#define MCORE_INVALIDINDEX8 0xFF

