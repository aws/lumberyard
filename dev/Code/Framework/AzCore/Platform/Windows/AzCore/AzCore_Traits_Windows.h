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

#if defined(AZ_RESTRICTED_PLATFORM)

    // xxx/PlatformDef_h_xxx.inl is a sectioned file. Each time we #include it, we need to specify which section of the file to load,
    // and rather than using hard-coded integers, we'll just register the sections here at the top
    #define PLATFORMDEF_H_SECTION_COMPILER_PLATFORM 1
    #define PLATFORMDEF_H_SECTION_TRAITS 2
    #define PLATFORMDEF_H_SECTION_OS 3
    #define PLATFORMDEF_H_SECTION_DLL 4
    #define PLATFORMDEF_H_SECTION_DLL_EXPORT 5

    #define PLATFORMDEF_H_SECTION PLATFORMDEF_H_SECTION_COMPILER_PLATFORM
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/PlatformDef_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/PlatformDef_h_provo.inl"
    #endif

#elif defined(_WIN32)

    #define AZ_PLATFORM_WINDOWS

    #if defined(__clang__)
        #define AZ_COMPILER_CLANG    __clang_major__
    #elif defined(_MSC_VER)
        #define AZ_COMPILER_MSVC     _MSC_VER
    #endif

    #if defined(_WIN64)
        #define AZ_PLATFORM_WINDOWS_X64
    // There is a bug, where the compiler will report C4324 for every struct class with a virtual func and a aligned memeber.
    // Remove this ASAP
        #pragma warning(disable:4324)
    #endif

#elif defined(__ANDROID__)

    #define AZ_PLATFORM_ANDROID

    #if defined(__aarch64__)
        #define AZ_PLATFORM_ANDROID_X64
    #elif defined(__ARM_ARCH_7A__)
        #define AZ_PLATFORM_ANDROID_X32
    #else
        #error This plaform is not supported
    #endif

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

#if defined(AZ_RESTRICTED_PLATFORM)
    #define PLATFORMDEF_H_SECTION PLATFORMDEF_H_SECTION_TRAITS
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/PlatformDef_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/PlatformDef_h_provo.inl"
    #endif
#else
    //----- Hardware traits ---------
    #if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)
        #define AZ_TRAIT_HARDWARE_ENABLE_EMM_INTRINSICS 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)
        #define AZ_TRAIT_HARDWARE_HAS_M128I 1
    #endif

    //----- OS traits ---------------
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_OS_ALLOW_DIRECT_ALLOCATIONS 1
    #endif
    #if !defined(AZ_PLATFORM_APPLE)
        #define AZ_TRAIT_OS_ALLOW_MULTICAST 1
    #endif
    #if defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_LINUX)
        #define AZ_TRAIT_OS_ALLOW_UNLIMITED_PATH_COMPONENT_LENGTH 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_OS_CAN_SET_FILES_WRITABLE 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        #define AZ_TRAIT_OS_DEFAULT_PAGE_SIZE (64 * 1024)
    #else
        #define AZ_TRAIT_OS_DEFAULT_PAGE_SIZE (4 * 1024)
    #endif
    #define AZ_TRAIT_OS_ENFORCE_STRICT_VIRTUAL_ALLOC_ALIGNMENT 0
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_OS_HPHA_MEMORYBLOCKBYTESIZE 512 * 1024 * 1024
    #else
        #define AZ_TRAIT_OS_HPHA_MEMORYBLOCKBYTESIZE 150 * 1024 * 1024
    #endif
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_OS_USE_FASTER_WINDOWS_SOCKET_CLOSE 1
    #endif
    #if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_ANDROID)
        #define AZ_TRAIT_OS_USE_POSIX_SOCKETS 1
    #endif
    #if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_OS_USE_SOCKETS 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_OS_USE_WINDOWS_ALIGNED_MALLOC 1
        #define AZ_TRAIT_OS_USE_CUSTOM_ALLOCATOR_FOR_MALLOC 0
        #define AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS 1
        #define AZ_TRAIT_OS_USE_WINDOWS_QUERY_PERFORMANCE_COUNTER 1
        #define AZ_TRAIT_OS_USE_WINDOWS_SET_EVENT 1
        #define AZ_TRAIT_OS_USE_WINDOWS_SOCKETS 1
        #define AZ_TRAIT_OS_USE_WINDOWS_THREADS 1
    #endif

    #if defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)
        #define AZ_TRAIT_OS_DELETE_THROW noexcept
    #else
        #define AZ_TRAIT_OS_DELETE_THROW
    #endif

    //----- Compiler traits ---------
    #if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        #define AZ_TRAIT_COMPILER_DEFINE_AZSWNPRINTF_AS_SWPRINTF 1
    #endif
    #if defined(LINUX) || defined(APPLE)
        #define AZ_TRAIT_COMPILER_DEFINE_FS_ERRNO_TYPE 1
    #endif
    #if defined(APPLE)
        #define AZ_TRAIT_COMPILER_DEFINE_FS_STAT_TYPE 1
    #endif
    #if defined(LINUX) || defined(APPLE)
        #define AZ_TRAIT_COMPILER_DEFINE_GETCURRENTPROCESSID 1
        #define AZ_TRAIT_COMPILER_DEFINE_SASSERTDATA_TYPE 1
    #endif
    #if defined(ANDROID)
        #define AZ_TRAIT_COMPILER_DEFINE_WCSICMP 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_COMPILER_ENABLE_WINDOWS_DLLS 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)
        #define AZ_TRAIT_COMPILER_INCLUDE_CSTDINT 1
    #endif
    #if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID_X64)
        #define AZ_TRAIT_COMPILER_INT64_T_IS_LONG 1
    #endif
    #if !defined(ANDROID)
        #define AZ_TRAIT_COMPILER_OPTIMIZE_MISSING_DEFAULT_SWITCH_CASE 1
    #endif
    #if defined(_WIN64) || defined(LINUX) || defined(APPLE)
        #define AZ_TRAIT_COMPILER_PASS_4PLUS_VECTOR_PARAMETERS_BY_VALUE 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_COMPILER_USE_OUTPUT_DEBUG_STRING 1
        #define AZ_TRAIT_COMPILER_USE_UNHANDLED_EXCEPTION_HANDLER 1
    #endif

    #if defined(AZ_COMPILER_MSVC) && (_MSC_VER <= 1800)
        #define AZ_TRAIT_COMPILER_USE_STATIC_STORAGE_FOR_NON_POD_STATICS 1
    #endif

    //----- Other -------------------
    #define AZ_TRAIT_DENY_ASSETPROCESSOR_LOOPBACK 0
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_PERF_MEMORYBENCHMARK_IS_AVAILABLE 1
    #endif
    #if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE_OSX)
        #define AZ_TRAIT_USE_WORKSTEALING_JOBS_IMPL 1
    #endif

    //----- Misc -------------------
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_USE_GET_MODULE_FILE_NAME 1
    #endif
    
    #define AZ_TRAIT_SKIP_CRYINTERLOCKED 0
    
    #if defined(AZ_PLATFORM_ANDROID)
        #define AZ_TRAIT_NO_SUPPORT_STACK_TRACE 1
    #endif

    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_USE_WINDOWS_FILE_API 1
    #endif

    #define AZ_TRAIT_DOES_NOT_SUPPORT_FILE_DISK_OFFSET 0

    #if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
        #define AZ_TRAIT_USE_SYSTEMFILE_HANDLE 1
    #endif

    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_USE_WINDOWS_PROCESSID 1
        #define AZ_TRAIT_USE_WINSOCK_API 1
        #define AZ_TRAIT_SUPPORT_WINDOWS_ALIGNED_MALLOC 1
        #define AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS 1
        #define AZ_TRAIT_USE_WINDOWS_CONDITIONAL_VARIABLE 1
    #endif

    #define AZ_TRAIT_USE_X64_ATOMIC_IMPL 0

    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_SUPPORTS_MICROSOFT_PPL 1
        #define AZ_TRAIT_UNITTEST_NON_PREALLOCATED_HPHA_TEST 1
    #endif

    #if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        #define AZ_TRAIT_PSUEDO_RANDOM_USE_FILE 1
    #endif

    #if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)
        #define AZ_TRAIT_PSUEDO_RANDOM_USE_SIMD 1
    #endif

    #define AZ_TRAIT_MAX_ALLOCATOR_SIZE_4GB 0

    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_TRAIT_USE_WINDOWS_SYNCHRONIZATION_LIBRARY 1
    #endif

    #if !defined(AZ_PLATFORM_APPLE_IOS) && !defined(AZ_PLATFORM_APPLE_TV)
        #define AZ_TRAIT_DEFINE_STUBBED_OPERATOR_NEW_OVERRIDES 1
    #endif

#endif

//////////////////////////////////////////////////////////////////////////

#define AZ_INLINE       inline

/// Deprecated macro
#   define AZ_DEPRECATED(_decl, _message) [[deprecated(_message)]] _decl

/// DLL import/export macros
#if defined(AZ_RESTRICTED_PLATFORM)
#   define PLATFORMDEF_H_SECTION PLATFORMDEF_H_SECTION_DLL_EXPORT
#   if defined(AZ_PLATFORM_XENIA)
#       include "Xenia/PlatformDef_h_xenia.inl"
#   elif defined(AZ_PLATFORM_PROVO)
#       include "Provo/PlatformDef_h_provo.inl"
#   endif
#elif defined(AZ_PLATFORM_WINDOWS)
#   if defined(AZ_COMPILER_CLANG)
#       define AZ_DLL_EXPORT __attribute__ ((dllexport))
#       define AZ_DLL_IMPORT __attribute__ ((dllimport))
#   elif defined(AZ_COMPILER_MSVC)
#       define AZ_DLL_EXPORT __declspec(dllexport)
#       define AZ_DLL_IMPORT __declspec(dllimport)
#   endif
#else
#   define AZ_DLL_EXPORT __attribute__ ((visibility ("default")))
#   define AZ_DLL_IMPORT __attribute__ ((visibility ("default")))
#endif

#if defined(AZ_COMPILER_MSVC)

/// Disables a warning using push style. For use matched with an AZ_POP_WARNING
#define AZ_PUSH_DISABLE_WARNING(_msvcOption, __)    \
    __pragma(warning(push));                        \
    __pragma(warning(disable : _msvcOption));

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                      \
    __pragma(warning(pop));

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

/// Disables a warning using push style. For use matched with an AZ_POP_WARNING
#define AZ_PUSH_DISABLE_WARNING(__, _gccOption)         \
    _Pragma("GCC diagnostic push");                     \
    _Pragma(AZ_STRINGIZE(GCC diagnostic ignored _gccOption));

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                          \
    _Pragma("GCC diagnostic pop");

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

/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __PRETTY_FUNCTION__


#if !defined(AZ_HAS_NULLPTR_T)
#   define nullptr __null
#endif

#elif defined(AZ_COMPILER_CLANG)

/// Disables a single warning using push style. For use matched with an AZ_POP_WARNING
#define AZ_PUSH_DISABLE_WARNING(__, _clangOption)           \
    _Pragma("clang diagnostic push");                       \
    _Pragma(AZ_STRINGIZE(clang diagnostic ignored _clangOption));

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                              \
    _Pragma("clang diagnostic pop");

#   define AZ_FORCE_INLINE  inline
/// Aligns a declaration.
#   define AZ_ALIGN(_decl, _alignment) _decl __attribute__((aligned(_alignment)))
/// Return the alignment of a type. This if for internal use only (use AZStd::alignment_of<>())
#   define AZ_INTERNAL_ALIGNMENT_OF(_type) __alignof__(_type)
/// Pointer is not aliased. (ref __restrict)
#   define AZ_RESTRICT  __restrict
/// Pointer will be aliased.
#   define AZ_MAY_ALIAS __attribute__((__may_alias__))
/// RValue ref, move constructors
#   define AZ_HAS_RVALUE_REFS
/// Variadic templates
#   define AZ_HAS_VARIADIC_TEMPLATES
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

#if defined(AZ_RESTRICTED_PLATFORM)
    #define PLATFORMDEF_H_SECTION PLATFORMDEF_H_SECTION_OS
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/PlatformDef_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/PlatformDef_h_provo.inl"
    #endif
#else
#   if defined(AZ_PLATFORM_WINDOWS)
#       define AZ_THREAD_LOCAL  __declspec(thread)
#   elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
#       define AZ_THREAD_LOCAL  __thread
#   endif

#   if defined(AZ_PLATFORM_WINDOWS_X64) || defined(AZ_PLATFORM_LINUX_X64) || defined(AZ_PLATFORM_APPLE)
#       define AZ_OS64
#   else
#       define AZ_OS32
#   endif

#   if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined (AZ_PLATFORM_APPLE_OSX)
#       define AZ_HAS_DLL_SUPPORT
#   endif
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

// Determine the dynamic library/module extension by platform
#if defined(AZ_RESTRICTED_PLATFORM)
    #define PLATFORMDEF_H_SECTION PLATFORMDEF_H_SECTION_DLL
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/PlatformDef_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/PlatformDef_h_provo.inl"
    #endif
#elif defined(AZ_PLATFORM_WINDOWS)
  #define AZ_DYNAMIC_LIBRARY_PREFIX
  #define AZ_DYNAMIC_LIBRARY_EXTENSION  ".dll"
#elif defined(AZ_PLATFORM_LINUX)
  #define AZ_DYNAMIC_LIBRARY_PREFIX     "lib"
  #define AZ_DYNAMIC_LIBRARY_EXTENSION  ".so"
#elif defined(AZ_PLATFORM_ANDROID)
  #define AZ_DYNAMIC_LIBRARY_PREFIX     "lib"
  #define AZ_DYNAMIC_LIBRARY_EXTENSION  ".so"
#elif defined(AZ_PLATFORM_APPLE)
  #define AZ_DYNAMIC_LIBRARY_PREFIX     "lib"
  #define AZ_DYNAMIC_LIBRARY_EXTENSION  ".dylib"
#else 
  #pragma error("Unrecognized platform for library extension")
#endif
