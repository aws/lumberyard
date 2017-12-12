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
#ifndef AZCORE_PLATFORM_DEF_H
#define AZCORE_PLATFORM_DEF_H 1

//////////////////////////////////////////////////////////////////////////
// Platforms

#if defined(AZ_MONOLITHIC_BUILD)
#define ENABLE_TYPE_INFO_NAMES 1
#endif

#if   defined(_WIN32)

#define AZ_PLATFORM_WINDOWS
#define AZ_COMPILER_MSVC        _MSC_VER

//#ifndef _CRT_SECURE_NO_WARNINGS
//# define _CRT_SECURE_NO_WARNINGS
//#endif // _CRT_SECURE_NO_WARNINGS
//#ifndef _CRT_SECURE_NO_DEPRECATE
//# define _CRT_SECURE_NO_DEPRECATE
//#endif // _CRT_SECURE_NO_DEPRECATE
//#ifndef _CRT_NONSTDC_NO_DEPRECATE
//# define _CRT_NONSTDC_NO_DEPRECATE
//#endif // _CRT_NONSTDC_NO_DEPRECATE

#if defined(_WIN64)
    #define AZ_PLATFORM_WINDOWS_X64
// There is a bug, where the compiler will report C4324 for every struct class with a virtual func and a aligned memeber.
// Remove this ASAP
    #pragma warning(disable:4324)
#endif


#elif defined(__ANDROID__)

#define AZ_PLATFORM_ANDROID

#if defined(__clang__)
#   define AZ_COMPILER_CLANG    __clang_major__
#elif defined(__GNUC__)
#   define AZ_COMPILER_GCC    __GNUC__
#else
#   error This compiler is not supported
#endif //

#elif defined(__linux__)

#define AZ_PLATFORM_LINUX

#if defined(__x86_64__)
    #define AZ_PLATFORM_LINUX_X64
    #define AZ_COMPILER_CLANG   __clang_major__
#else
    #error This plaform is not supported
#endif

#elif defined(__MWERKS__)

#define AZ_PLATFORM_WII
#define AZ_COMPILER_MWERKS

#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if (TARGET_OS_TV)
        #define AZ_PLATFORM_APPLE_TV
    #elif (TARGET_OS_IPHONE)
        #define AZ_PLATFORM_APPLE_IOS
    #elif (TARGET_OS_MAC)
        #define AZ_PLATFORM_APPLE_OSX
    #else
        #error Unknown Apple platform
    #endif

    #define AZ_PLATFORM_APPLE

    #if defined(__clang__)
        #define AZ_COMPILER_CLANG __clang_major__
    #elif defined(__GNUC__)
        #define AZ_COMPILER_GCC __GNUC__
    #else
        #error This compiler is not supported
    #endif

#else
#error This plaform is not supported
#endif //
//////////////////////////////////////////////////////////////////////////

#define AZ_INLINE       inline

#if defined(AZ_COMPILER_MSVC)

#   define AZ_FORCE_INLINE  __forceinline
#if !defined(_DEBUG)
#   pragma warning(disable:4714) //warning C4714 marked as __forceinline not inlined. Sadly this happens when LTCG during linking. We tried to NOT use force inline but VC 2012 is bad at inlining.
#endif

/// Aligns a declaration.
#   define AZ_ALIGN(_decl, _alignment) __declspec(align(_alignment)) _decl
/// Return the alignment of a type. This if for internal use only (use AZStd::alignment_of<>())
#   define AZ_INTERNAL_ALIGNMENT_OF(_type) __alignof(_type)
/// Pointer is not aliased. (ref __restrict)
#   define AZ_RESTRICT  __restrict
/// Pointer will be aliased.
#   define AZ_MAY_ALIAS
/// Deprecated macro
#   define AZ_DEPRECATED(_decl, _message)    __declspec(deprecated(_message)) _decl
/// DLL import/export macros
#   define AZ_DLL_EXPORT __declspec(dllexport)
#   define AZ_DLL_IMPORT __declspec(dllimport)
/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __FUNCSIG__

#   if AZ_COMPILER_MSVC >= 1700
#       define AZ_HAS_RVALUE_REFS
#       define AZ_HAS_NULLPTR_T
#   else
#       define nullptr NULL
#   endif // _MSC_VER < 1700

#   if AZ_COMPILER_MSVC >= 1800
#       define AZ_HAS_VARIADIC_TEMPLATES
// std::underlying_type for enums
#       define AZSTD_UNDERLAYING_TYPE
/// Enabled if we have initializers list support
#       define AZ_HAS_INITIALIZERS_LIST
/// Enabled if we can alias templates with the using keyword
#       define AZ_HAS_TEMPLATE_ALIAS
/// Used to delete a method from a class
#       define AZ_DELETE_METHOD = delete
/// Use the default implementation of a class method
#       define AZ_DEFAULT_METHOD = default
#   else
/// Delete a method from a class, not implemented
#       define AZ_DELETE_METHOD
/// Default implementation of a class method, not implemented
#       define AZ_DEFAULT_METHOD
#   endif

//////////////////////////////////////////////////////////////////////////
#elif defined(AZ_COMPILER_GCC) || defined(AZ_COMPILER_SNC)
/// Forces a function to be inlined. \todo check __attribute__( ( always_inline ) )

#   define AZ_FORCE_INLINE  inline
#ifdef  AZ_COMPILER_SNC
#   define AZ_INTERNAL_ALIGNMENT_OF(_type) __alignof__(_type)
#   if __option(cpp11)
/// RValue ref, move constructors
#       define AZ_HAS_RVALUE_REFS
/// Variadic templates
#       define AZ_HAS_VARIADIC_TEMPLATES
/// nullptr_t
#       define AZ_HAS_NULLPTR_T
/// Enabled if we have initializers list support
#       define AZ_HAS_INITIALIZERS_LIST
/// Enabled if we can alias templates with the using keyword
#       define AZ_HAS_TEMPLATE_ALIAS
/// Used to delete a method from a class
#       define AZ_DELETE_METHOD = delete
/// Use the default implementation of a class method
#       define AZ_DEFAULT_METHOD = default
#   else
/// Delete a method from a class, not implemented
#       define AZ_DELETE_METHOD
/// Default implementation of a class method, not implemented
#       define AZ_DEFAULT_METHOD
#   endif // __option(cpp11)
#else
// GCC we support version above 4.4
#   define AZ_HAS_RVALUE_REFS
/// Variadic templates
#   define AZ_HAS_VARIADIC_TEMPLATES
// std::underlying_type for enums
#   define AZSTD_UNDERLAYING_TYPE
/// Note this work properly with templates on older GCC.
#   define AZ_INTERNAL_ALIGNMENT_OF(_type) __alignof__(_type)
/// Compiler has AZStd::nullptr_t (std::nullptr_t)
#   define AZ_HAS_NULLPTR_T
/// Enabled if we have initializers list support
#   define AZ_HAS_INITIALIZERS_LIST
/// Enabled if we can alias templates with the using keyword
#   define AZ_HAS_TEMPLATE_ALIAS
/// Used to delete a method from a class
#   define AZ_DELETE_METHOD = delete
/// Use the default implementation of a class method
#   define AZ_DEFAULT_METHOD = default

#endif
// Aligns a declaration.
#   define AZ_ALIGN(_decl, _alignment) _decl __attribute__((aligned(_alignment)))
/// Pointer is not aliased. (ref __restrict)
#   define AZ_RESTRICT  __restrict
/// Pointer will be aliased.
#   define AZ_MAY_ALIAS __attribute__((__may_alias__))

/// Deprecated macro
#   define AZ_DEPRECATED(_decl, _message) __attribute__((deprecated)) _decl
/// DLL import/export macros
#   define AZ_DLL_EXPORT __attribute__ ((visibility ("default")))
#   define AZ_DLL_IMPORT __attribute__ ((visibility ("default")))
/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __PRETTY_FUNCTION__


#if !defined(AZ_HAS_NULLPTR_T)
#   define nullptr __null
#endif

#elif defined(AZ_COMPILER_CLANG)
#   define AZ_FORCE_INLINE  inline
/// Aligns a declaration.
#   define AZ_ALIGN(_decl, _alignment) _decl __attribute__((aligned(_alignment)))
/// Return the alignment of a type. This if for internal use only (use AZStd::alignment_of<>())
#   define AZ_INTERNAL_ALIGNMENT_OF(_type) __alignof__(_type)
/// Pointer is not aliased. (ref __restrict)
#   define AZ_RESTRICT  __restrict
/// Pointer will be aliased.
#   define AZ_MAY_ALIAS __attribute__((__may_alias__))
/// Deprecated macro
#   define AZ_DEPRECATED(_decl, _message) __attribute__((deprecated(_message))) _decl
/// RValue ref, move constructors
#   define AZ_HAS_RVALUE_REFS
/// Variadic templates
#   define AZ_HAS_VARIADIC_TEMPLATES
/// Compiler has AZStd::nullptr_t (std::nullptr_t)
#   define AZ_HAS_NULLPTR_T
/// std::underlying_type for enums
#   define AZSTD_UNDERLAYING_TYPE
/// DLL import/export macros
#   define AZ_DLL_EXPORT __attribute__ ((visibility ("default")))
#   define AZ_DLL_IMPORT __attribute__ ((visibility ("default")))
/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __PRETTY_FUNCTION__
/// Enabled if we have initializers list support
#   define AZ_HAS_INITIALIZERS_LIST
/// Enabled if we can alias templates with the using keyword
#   define AZ_HAS_TEMPLATE_ALIAS
/// Used to delete a method from a class
#   define AZ_DELETE_METHOD = delete
/// Use the default implementation of a class method
#   define AZ_DEFAULT_METHOD = default

#elif defined(AZ_COMPILER_MWERKS)
/// Forces a function to be inlined.
#   define AZ_FORCE_INLINE  inline
/// Aligns a declaration.
#   define AZ_ALIGN(_decl, _alignment) _decl __attribute__((aligned(_alignment)))
/// Return the alignment of a type. This if for internal use only (use AZStd::alignment_of<>())
#   define AZ_INTERNAL_ALIGNMENT_OF(_type) __alignof__(_type)
/// Pointer is not aliased. (ref __restrict)
#   define AZ_RESTRICT
/// Pointer will be aliased.
#   define AZ_MAY_ALIAS
/// Deprecated macro (todo)
#   define AZ_DEPRECATED(_decl, _message)
/// Check which version will support that.
#   define nullptr NULL
/// Delete a method from a class, not implemented
#   define AZ_DELETE_METHOD
/// Default implementation of a class method, not implemented
#   define AZ_DEFAULT_METHOD
/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __PRETTY_FUNCTION__

#else
    #error Compiler not supported
#endif

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE
#   define AZ_THREAD_LOCAL  __declspec(thread)
#elif defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE_OSX) // ACCEPTED_USE
#   define AZ_THREAD_LOCAL  __thread
#endif

#if defined(AZ_PLATFORM_WINDOWS_X64) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_LINUX_X64) || defined(AZ_PLATFORM_APPLE) // ACCEPTED_USE
#   define AZ_OS64
#else
#   define AZ_OS32
#endif


#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined (AZ_PLATFORM_APPLE_OSX) // ACCEPTED_USE
#   define AZ_HAS_DLL_SUPPORT
#endif

// We need to define AZ_DEBUG_BUILD in debug mode. We can also define it in debug optimized mode (left up to the user).
// note that _DEBUG is not in fact always defined on all platforms, and only AZ_DEBUG_BUILD should be relied on.
#if !defined(AZ_DEBUG_BUILD) && defined(_DEBUG)
#   define AZ_DEBUG_BUILD
#endif

// note that many include ONLY PlatformDef.h and not base.h, so flags such as below need to be here.
// AZ_ENABLE_DEBUG_TOOLS - turns on and off interaction with the debugger.
// Things like being able to check whether the current process is being debugged, to issue a "debug break" command, etc.
#if defined(AZ_DEBUG_BUILD) && !defined(AZ_ENABLE_DEBUG_TOOLS)
#   define AZ_ENABLE_DEBUG_TOOLS
#endif

// AZ_ENABLE_TRACING - turns on and off the availability of AZ_TracePrintf / AZ_Assert / AZ_Error / AZ_Warning
#if defined(AZ_DEBUG_BUILD) && !defined(AZ_ENABLE_TRACING)
#   define AZ_ENABLE_TRACING
#endif

#if !defined(AZ_COMMAND_LINE_LEN)
#   define AZ_COMMAND_LINE_LEN 2048
#endif

#endif // AZCORE_PLATFORM_DEF_H
#pragma once



