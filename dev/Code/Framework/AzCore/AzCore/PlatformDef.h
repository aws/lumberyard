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

//////////////////////////////////////////////////////////////////////////
// Platforms

#if defined(AZ_MONOLITHIC_BUILD)
    #define ENABLE_TYPE_INFO_NAMES 1
#endif

#if defined(__clang__)
    #define AZ_COMPILER_CLANG   __clang_major__
#elif defined(_MSC_VER)
    #define AZ_COMPILER_MSVC    _MSC_VER
#else
#   error This compiler is not supported
#endif

#include <AzCore/AzCore_Traits_Platform.h>

//////////////////////////////////////////////////////////////////////////

#define AZ_INLINE                       inline
#define AZ_THREAD_LOCAL                 AZ_TRAIT_COMPILER_THREAD_LOCAL
#define AZ_DYNAMIC_LIBRARY_PREFIX       AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX
#define AZ_DYNAMIC_LIBRARY_EXTENSION    AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION

#if defined(AZ_COMPILER_CLANG)
    #define AZ_DLL_EXPORT               AZ_TRAIT_OS_DLL_EXPORT_CLANG
    #define AZ_DLL_IMPORT               AZ_TRAIT_OS_DLL_IMPORT_CLANG
#elif defined(AZ_COMPILER_MSVC)
    #define AZ_DLL_EXPORT               __declspec(dllexport)
    #define AZ_DLL_IMPORT               __declspec(dllimport)
#endif

// These defines will be deprecated in the future with LY-99152
#if defined(AZ_PLATFORM_MAC)
    #define AZ_PLATFORM_APPLE_OSX
#endif
#if defined(AZ_PLATFORM_IOS)
    #define AZ_PLATFORM_APPLE_IOS
#endif
#if AZ_TRAIT_OS_PLATFORM_APPLE
    #define AZ_PLATFORM_APPLE
#endif

#if AZ_TRAIT_OS_HAS_DLL_SUPPORT
    #define AZ_HAS_DLL_SUPPORT
#endif

/// Deprecated macro
#define AZ_DEPRECATED(_decl, _message) [[deprecated(_message)]] _decl


#define AZ_STRINGIZE_I(text) #text

#if defined(AZ_COMPILER_MSVC)
#    define AZ_STRINGIZE(text) AZ_STRINGIZE_A((text))
#    define AZ_STRINGIZE_A(arg) AZ_STRINGIZE_I arg
#elif defined(AZ_COMPILER_MWERKS)
#    define AZ_STRINGIZE(text) AZ_STRINGIZE_OO((text))
#    define AZ_STRINGIZE_OO(par) AZ_STRINGIZE_I ## par
#else
#    define AZ_STRINGIZE(text) AZ_STRINGIZE_I(text)
#endif


#if defined(AZ_COMPILER_MSVC)

/// Disables a warning using push style. For use matched with an AZ_POP_WARNING
#define AZ_PUSH_DISABLE_WARNING(_msvcOption, __)    \
    __pragma(warning(push))                         \
    __pragma(warning(disable : _msvcOption))

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                      \
    __pragma(warning(pop))

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
/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __FUNCSIG__

#   if AZ_COMPILER_MSVC >= 1700
#       define AZ_HAS_NULLPTR_T
#   else
#       define nullptr NULL
#   endif // _MSC_VER < 1700

#   if AZ_COMPILER_MSVC >= 1800
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

/// Disables a warning using push style. For use matched with an AZ_POP_WARNING
#define AZ_PUSH_DISABLE_WARNING(__, _gccOption)         \
    _Pragma("GCC diagnostic push")                      \
    _Pragma(AZ_STRINGIZE(GCC diagnostic ignored _gccOption))

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                          \
    _Pragma("GCC diagnostic pop")

/// Forces a function to be inlined. \todo check __attribute__( ( always_inline ) )
#   define AZ_FORCE_INLINE  inline
#ifdef  AZ_COMPILER_SNC
#   define AZ_INTERNAL_ALIGNMENT_OF(_type) __alignof__(_type)
#   if __option(cpp11)
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

/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __PRETTY_FUNCTION__


#if !defined(AZ_HAS_NULLPTR_T)
#   define nullptr __null
#endif

#elif defined(AZ_COMPILER_CLANG)

/// Disables a single warning using push style. For use matched with an AZ_POP_WARNING
#define AZ_PUSH_DISABLE_WARNING(__, _clangOption)           \
    _Pragma("clang diagnostic push")                        \
    _Pragma(AZ_STRINGIZE(clang diagnostic ignored _clangOption))

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                              \
    _Pragma("clang diagnostic pop")

#   define AZ_FORCE_INLINE  inline
/// Aligns a declaration.
#   define AZ_ALIGN(_decl, _alignment) _decl __attribute__((aligned(_alignment)))
/// Return the alignment of a type. This if for internal use only (use AZStd::alignment_of<>())
#   define AZ_INTERNAL_ALIGNMENT_OF(_type) __alignof__(_type)
/// Pointer is not aliased. (ref __restrict)
#   define AZ_RESTRICT  __restrict
/// Pointer will be aliased.
#   define AZ_MAY_ALIAS __attribute__((__may_alias__))
/// Compiler has AZStd::nullptr_t (std::nullptr_t)
#   define AZ_HAS_NULLPTR_T
/// std::underlying_type for enums
#   define AZSTD_UNDERLAYING_TYPE
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

#else
    #error Compiler not supported
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
