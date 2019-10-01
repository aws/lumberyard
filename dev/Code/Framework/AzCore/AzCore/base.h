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

#include <AzCore/PlatformDef.h> ///< Platform/compiler specific defines
#include <AzCore/base_Platform.h>

#ifndef AZ_ARRAY_SIZE
/// Return an array size for static arrays.
#   define  AZ_ARRAY_SIZE(__a)  (sizeof(__a)/sizeof(__a[0]))
#endif

#ifndef AZ_SIZE_ALIGN_UP
/// Aign to the next bigger/up size
#   define AZ_SIZE_ALIGN_UP(_size, _align)       ((_size+(_align-1)) & ~(_align-1))
#endif // AZ_SIZE_ALIGN_UP
#ifndef AZ_SIZE_ALIGN_DOWN
/// Align to the next smaller/down size
#   define AZ_SIZE_ALIGN_DOWN(_size, _align)     ((_size) & ~(_align-1))
#endif // AZ_SIZE_ALIGN_DOWN
#ifndef AZ_SIZE_ALIGN
/// Same as AZ_SIZE_ALIGN_UP
    #define AZ_SIZE_ALIGN(_size, _align)         AZ_SIZE_ALIGN_UP(_size, _align)
#endif // AZ_SIZE_ALIGN

namespace AZ
{
    static inline bool IsBigEndian(PlatformID /*id*/)
    {
        return false;
    }

    const char* GetPlatformName(PlatformID platform);
} // namespace AZ


#if defined(AZ_COMPILER_GCC) && ((AZ_COMPILER_GCC > 3) || ((AZ_COMPILER_GCC == 3) && (__GNUC_MINOR__ >= 4)))
#define AZSTD_STATIC_ASSERT_BOOL_CAST(_x) ((_x) == 0 ? false : true)
#else
#define AZSTD_STATIC_ASSERT_BOOL_CAST(_x) (bool)(_x)
#endif

#define AZ_JOIN(X, Y) AZSTD_DO_JOIN(X, Y)
#define AZSTD_DO_JOIN(X, Y) AZSTD_DO_JOIN2(X, Y)
#define AZSTD_DO_JOIN2(X, Y) X##Y

#ifdef AZ_COMPILER_MSVC
#   if AZ_COMPILER_MSVC < 1600
#       define AZ_STATIC_ASSERT(_Exp, _Str)                                        \
    typedef ::AZ::static_assert_test<                                              \
    sizeof(::AZ::STATIC_ASSERTION_FAILURE< AZSTD_STATIC_ASSERT_BOOL_CAST(_Exp) >)> \
        AZ_JOIN (azstd_static_assert_typedef_, __COUNTER__)
#   else // AZ_COMPILER_MSVC >= 1600
#       define  AZ_STATIC_ASSERT(_Exp, _Str) static_assert((_Exp), _Str)
#endif //
#elif defined(AZ_COMPILER_CLANG) || defined(AZ_COMPILER_GCC)
#   define AZ_STATIC_ASSERT(_Exp, _Str) static_assert((_Exp), _Str)
#else
// generic version
namespace AZ
{
    template<bool x>
    struct STATIC_ASSERTION_FAILURE;
    template<>
    struct STATIC_ASSERTION_FAILURE<true>
    {
        enum
        {
            value = 1
        };
    };
    template<int x>
    struct static_assert_test{};
} //namespace AZ
#define AZ_STATIC_ASSERT(_Exp, _Str)                                               \
    typedef ::AZ::static_assert_test<                                              \
    sizeof(::AZ::STATIC_ASSERTION_FAILURE< AZSTD_STATIC_ASSERT_BOOL_CAST(_Exp) >)> \
        AZ_JOIN (azstd_static_assert_typedef_, __LINE__)
#endif //

//////////////////////////////////////////////////////////////////////////

/**
 * Macros for calling into strXXX functions. These are simple wrappers that call into the platform
 * implementation. Definitions provide inputs for destination size to allow calling to strXXX_s functions
 * on supported platforms. With that said there are no guarantees that this size will be respected on all of them.
 * The main goal here is to help devs avoid #ifdefs when calling to platform API. The goal here is wrap only the function
 * that differ, otherwise you should call directly into functions like strcmp, strstr, etc.
 * If you want guranteed "secure" functions, you should not use those wrappers. In general we recommend using AZStd::string/wstring for manipulating strings.
 */
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
#   define azsnprintf(_buffer, _size, ...)        _snprintf_s(_buffer, _size, _size-1, __VA_ARGS__)
#   define azvsnprintf(_buffer, _size, ...)       _vsnprintf_s(_buffer, _size, _size-1, __VA_ARGS__)
#   define azsnwprintf(_buffer, _size, ...)       _snwprintf_s(_buffer, _size, _size-1, __VA_ARGS__)
#   define azvsnwprintf(_buffer, _size, ...)      _vsnwprintf_s(_buffer, _size, _size-1, __VA_ARGS__)
#   define azstrtok(_buffer, _size, _delim, _context)  strtok_s(_buffer, _delim, _context)
#   define azstrcat         strcat_s
#   define azstrncat        strncat_s
#   define strtoll          _strtoi64
#   define strtoull         _strtoui64
#   define azsscanf         sscanf_s
#   define azstrcpy         strcpy_s
#   define azstrncpy        strncpy_s
#   define azstricmp        _stricmp
#   define azstrnicmp       _strnicmp
#   define azisfinite       _finite
#   define azltoa           _ltoa_s
#   define azitoa           _itoa_s
#   define azui64toa        _ui64toa_s
#   define azswscanf        swscanf_s
#   define azwcsicmp        _wcsicmp
#   define azwcsnicmp       _wcsnicmp
#   define azmemicmp        _memicmp

// note: for cross-platform compatibility, do not use the return value of azfopen. On Windows, it's an errno_t and 0 indicates success. On other platforms, the return value is a FILE*, and a 0 value indicates failure.
#   define azfopen          fopen_s

#   define azsprintf(_buffer, ...)      sprintf_s(_buffer, AZ_ARRAY_SIZE(_buffer), __VA_ARGS__)
#   define azstrlwr         _strlwr_s
#   define azvsprintf       vsprintf_s
#   define azwcscpy         wcscpy_s
#   define azstrtime        _strtime_s
#   define azstrdate        _strdate_s
#   define azlocaltime(time, result) localtime_s(result, time)
#else
#   define azsnprintf       snprintf
#   define azvsnprintf      vsnprintf
#   if AZ_TRAIT_COMPILER_DEFINE_AZSWNPRINTF_AS_SWPRINTF
#       define azsnwprintf  swprintf
#       define azvsnwprintf vswprintf
#   else
#       define azsnwprintf  snwprintf
#       define azvsnwprintf vsnwprintf
#   endif
#   define azstrtok(_buffer, _size, _delim, _context)  strtok(_buffer, _delim)
#   define azstrcat(_dest, _destSize, _src) strcat(_dest, _src)
#   define azstrncat(_dest, _destSize, _src, _count) strncat(_dest, _src, _count)
#   define azsscanf         sscanf
#   define azstrcpy(_dest, _destSize, _src) strcpy(_dest, _src)
#   define azstrncpy(_dest, _destSize, _src, _count) strncpy(_dest, _src, _count)
#   define azstricmp        strcasecmp
#   define azstrnicmp       strncasecmp
#   if defined(NDK_REV_MAJOR) && NDK_REV_MAJOR < 16
#       define azisfinite   __isfinitef
#   else
#       define azisfinite   isfinite
#   endif
#   define azltoa(_value, _buffer, _size, _radix) ltoa(_value, _buffer, _radix)
#   define azitoa(_value, _buffer, _size, _radix) itoa(_value, _buffer, _radix)
#   define azui64toa(_value, _buffer, _size, _radix) _ui64toa(_value, _buffer, _radix)
#   define azswscanf        swscanf
#   define azwcsicmp        wcsicmp
#   define azwcsnicmp       wcsnicmp
#   define azmemicmp        memicmp

// note: for cross-platform compatibility, do not use the return value of azfopen. On Windows, it's an errno_t and 0 indicates success. On other platforms, the return value is a FILE*, and a 0 value indicates failure.
#   define azfopen(_fp, _filename, _attrib) *(_fp) = fopen(_filename, _attrib)

#   define azsprintf       sprintf
#   define azstrlwr(_buffer, _size)             strlwr(_buffer)
#   define azvsprintf       vsprintf
#   define azwcscpy(_dest, _size, _buffer)      wcscpy(_dest, _buffer)
#   define azstrtime        _strtime
#   define azstrdate        _strdate
#   define azlocaltime      localtime_r
#endif

#if AZ_TRAIT_USE_POSIX_STRERROR_R
#   define azstrerror_s(_dst, _num, _err)   strerror_r(_err, _dst, _num)
#else
#   define azstrerror_s strerror_s
#endif

#define AZ_INVALID_POINTER  reinterpret_cast<void*>(0x0badf00dul)

// Variadic MACROS util functions

/**
 * AZ_VA_NUM_ARGS
 * counts number of parameters (up to 10).
 * example. AZ_VA_NUM_ARGS(x,y,z) -> expands to 3
 */
#ifndef AZ_VA_NUM_ARGS
// we add the zero to avoid the case when we require at least 1 param at the end...
#   define AZ_VA_NUM_ARGS(...) AZ_VA_NUM_ARGS_IMPL_((__VA_ARGS__, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define AZ_VA_NUM_ARGS_IMPL_(tuple) AZ_VA_NUM_ARGS_IMPL tuple
#   define AZ_VA_NUM_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, N, ...) N
//
// Expands a macro and calls a different macro based on the number of arguments.
//
// Example: We need to specialize a macro for 1 and 2 arguments
// #define AZ_MY_MACRO(...)   AZ_MACRO_SPECIALIZE(AZ_MY_MACRO_,AZ_VA_NUM_ARGS(__VA_ARGS__),(__VA_ARGS__))
//
// #define AZ_MY_MACRO_1(_1)    /* code for 1 param */
// #define AZ_MY_MACRO_2(_1,_2) /* code for 2 params */
// ... etc.
//
//
// We have 3 levels of macro expansion...
#   define AZ_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)    MACRO_NAME##NPARAMS PARAMS
#   define AZ_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)     AZ_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
#   define AZ_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS)       AZ_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

#endif // AZ_VA_NUM_ARGS


// Out of all supported compilers, mwerks is the only one
// that requires variadic macros to have at least 1 param.
// This is a pain they we use macros to call functions (with no params).

// we implement functions for up to 10 params
#define AZ_FUNCTION_CALL_1(_1)                                  _1()
#define AZ_FUNCTION_CALL_2(_1, _2)                               _1(_2)
#define AZ_FUNCTION_CALL_3(_1, _2, _3)                           _1(_2, _3)
#define AZ_FUNCTION_CALL_4(_1, _2, _3, _4)                        _1(_2, _3, _4)
#define AZ_FUNCTION_CALL_5(_1, _2, _3, _4, _5)                     _1(_2, _3, _4, _5)
#define AZ_FUNCTION_CALL_6(_1, _2, _3, _4, _5, _6)                  _1(_2, _3, _4, _5, _6)
#define AZ_FUNCTION_CALL_7(_1, _2, _3, _4, _5, _6, _7)               _1(_2, _3, _4, _5, _6, _7)
#define AZ_FUNCTION_CALL_8(_1, _2, _3, _4, _5, _6, _7, _8)            _1(_2, _3, _4, _5, _6, _7, _8)
#define AZ_FUNCTION_CALL_9(_1, _2, _3, _4, _5, _6, _7, _8, _9)         _1(_2, _3, _4, _5, _6, _7, _8, _9)
#define AZ_FUNCTION_CALL_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10)    _1(_2, _3, _4, _5, _6, _7, _8, _9, _10)

// We require at least 1 param FunctionName
#define AZ_FUNCTION_CALL(...)           AZ_MACRO_SPECIALIZE(AZ_FUNCTION_CALL_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))





// Based on boost macro expansion fix...
#define AZ_PREVENT_MACRO_SUBSTITUTION


//////////////////////////////////////////////////////////////////////////

#include <cstddef> // the macros NULL and offsetof as well as the types ptrdiff_t, wchar_t, and size_t.

// bring to global namespace, this is what string.h does anyway
using std::size_t;
using std::ptrdiff_t;

/**
 * Data types with specific length. They should be used ONLY when we serialize data if
 * they have native type equivalent, which should take precedence.
 */

#include <cstdint>

namespace AZ
{
    typedef int8_t    s8;
    typedef uint8_t   u8;
    typedef int16_t   s16;
    typedef uint16_t  u16;
    typedef int32_t   s32;
    typedef uint32_t  u32;
#   if AZ_TRAIT_COMPILER_INT64_T_IS_LONG // int64_t is long
    typedef signed long long        s64;
    typedef unsigned long long      u64;
#   else
    typedef int64_t   s64;
    typedef uint64_t  u64;
#   endif //


    typedef struct
    {
        s64 a, b;
    } s128;
    typedef struct
    {
        u64 a, b;
    } u128;

    template<typename T>
    inline T SizeAlignUp(T s, size_t a) { return (s+(a-1)) & ~(a-1); }

    template<typename T>
    inline T SizeAlignDown(T s, size_t a)   { return (s) & ~(a-1); }

    template<typename T>
    inline T* PointerAlignUp(T* p, size_t a)    { return reinterpret_cast<T*>((reinterpret_cast<size_t>(p)+(a-1)) & ~(a-1));    }

    template<typename T>
    inline T* PointerAlignDown(T* p, size_t a) { return reinterpret_cast<T*>((reinterpret_cast<size_t>(p)) & ~(a-1));   }

    /**
    * Does an safe alias cast using a union. This will allow you to properly cast types that when
    * strict aliasing is enabled.
    */
    template<typename T, typename S>
    T AliasCast(S source)
    {
        AZ_STATIC_ASSERT(sizeof(T) == sizeof(S), "Source and target should be the same size!");
        union
        {
            S source;
            T target;
        } cast;
        cast.source = source;
        return cast.target;
    }
}


#include <AzCore/Debug/Trace.h> // Global access to AZ_Assert,AZ_Error,AZ_Warning, etc.

// We have a separate force_inline define for math function. This is due to fact in the past there was an issue
// with SSE and __force_inline. This is not longer an issue, so we can just deprecate this.
# define AZ_MATH_FORCE_INLINE AZ_FORCE_INLINE

#if defined(AZ_COMPILER_SNC)
// warning 1646: two-argument (aligned) member new missing -- using single argument version <-- we handle this in our memory manager
#pragma diag_suppress=1646
#endif


// \ref AZ::AliasCast
#define azalias_cast AZ::AliasCast

// Macros to disable the auto-generated copy/move constructors and assignment operators for a class
// Note: AZ_DISABLE_COPY must also set the move constructor and move assignment operator
// to `=default`, otherwise they will be implicitly disabled by the compiler.
// If you wish to implement your own move constructor and assignment operator, you must
// explicitly delete the copy and assignment operator without the use of this macro.
#define AZ_DISABLE_COPY(_Class) \
    _Class(const _Class&) = delete; _Class& operator=(const _Class&) = delete; \
    _Class(_Class&&) = default; _Class& operator=(_Class&&) = default;
// Note: AZ_DISABLE_MOVE is DEPRECATED and will be removed.
// The preferred approach to use if a type is to be copy only is to simply provide copy and
// assignment operators (either `=default` or user provided), moves will be implicitly disabled.
#define AZ_DISABLE_MOVE(_Class) \
    _Class(_Class&&) = delete; _Class& operator=(_Class&&) = delete;
// Note: Setting the move constructor and move assignment operator to `=delete` here is not
// strictly necessary (as they will be implicitly disabled when the copy/assignment operators
// are declared) but is done to be explicit that this is the intention.
#define AZ_DISABLE_COPY_MOVE(_Class) \
    _Class(const _Class&) = delete; _Class& operator=(const _Class&) = delete; AZ_DISABLE_MOVE(_Class)

// Macros to default the auto-generated copy/move constructors and assignment operators for a class
#define AZ_DEFAULT_COPY(_Class) _Class(const _Class&) = default; _Class& operator=(const _Class&) = default;
// MSVC only supports default move constructors and assignment operators starting from version 14.0,
// so we can't add macros for AZ_DEFAULT_MOVE/AZ_DEFAULT_COPY_MOVE while we're supporting MSVC 12.0

// Macro that can be used to avoid unreferenced variable warnings
#define AZ_UNUSED(x) (void)x;

#define AZ_DEFINE_ENUM_BITWISE_OPERATORS(EnumType) \
inline EnumType operator | (EnumType a, EnumType b) \
    { return EnumType(((AZStd::underlying_type<EnumType>::type)a) | ((AZStd::underlying_type<EnumType>::type)b)); } \
inline EnumType &operator |= (EnumType &a, EnumType b) \
    { return (EnumType &)(((AZStd::underlying_type<EnumType>::type &)a) |= ((AZStd::underlying_type<EnumType>::type)b)); } \
inline EnumType operator & (EnumType a, EnumType b) \
    { return EnumType(((AZStd::underlying_type<EnumType>::type)a) & ((AZStd::underlying_type<EnumType>::type)b)); } \
inline EnumType &operator &= (EnumType &a, EnumType b) \
    { return (EnumType &)(((AZStd::underlying_type<EnumType>::type &)a) &= ((AZStd::underlying_type<EnumType>::type)b)); } \
inline EnumType operator ~ (EnumType a) \
    { return EnumType(~((AZStd::underlying_type<EnumType>::type)a)); } \
inline  EnumType operator ^ (EnumType a, EnumType b) \
    { return EnumType(((AZStd::underlying_type<EnumType>::type)a) ^ ((AZStd::underlying_type<EnumType>::type)b)); } \
inline EnumType &operator ^= (EnumType &a, EnumType b) \
    { return (EnumType &)(((AZStd::underlying_type<EnumType>::type &)a) ^= ((AZStd::underlying_type<EnumType>::type)b)); }
