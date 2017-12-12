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
#ifndef AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_HELPER_INCLUDED
#define AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_HELPER_INCLUDED

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    namespace type_traits
    {
        template <class R>
        struct is_function_ptr_helper
        {
            AZSTD_STATIC_CONSTANT(bool, value = false);
        };

        template <class R >
        struct is_function_ptr_helper<R (*)()>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R >
        struct is_function_ptr_helper<R (*)(...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0>
        struct is_function_ptr_helper<R (*)(T0)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0>
        struct is_function_ptr_helper<R (*)(T0 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1>
        struct is_function_ptr_helper<R (*)(T0, T1)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1>
        struct is_function_ptr_helper<R (*)(T0, T1 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2>
        struct is_function_ptr_helper<R (*)(T0, T1, T2)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2>
        struct is_function_ptr_helper<R (*)(T0, T1, T2 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
#if 0  // 10 should be enough for now
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24>
        struct is_function_ptr_helper<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...)>
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };
        #endif
#endif // 0
    }
}

#endif // AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_HELPER_INCLUDED
#pragma once