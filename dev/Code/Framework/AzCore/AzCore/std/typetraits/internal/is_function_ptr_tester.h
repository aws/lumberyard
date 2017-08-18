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
#ifndef AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_TESTER_INCLUDED
#define AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_TESTER_INCLUDED

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/internal/yes_no_type.h>

namespace AZStd
{
    namespace type_traits
    {
        // Note it is acceptible to use ellipsis here, since the argument will
        // always be a pointer type of some sort (JM 2005/06/04):
        no_type AZSTD_TYPE_TRAITS_DECL is_function_ptr_tester(...);

        template <class R >
        yes_type is_function_ptr_tester(R (*)());
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R >
        yes_type is_function_ptr_tester(R (*)(...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R >
        yes_type is_function_ptr_tester(R (__stdcall*)());
        template <class R >
        yes_type is_function_ptr_tester(R (__stdcall*)(...));
        #ifndef _MANAGED
        template <class R >
        yes_type is_function_ptr_tester(R (__fastcall*)());
        template <class R >
        yes_type is_function_ptr_tester(R (__fastcall*)(...));
        #endif
        template <class R >
        yes_type is_function_ptr_tester(R (__cdecl*)());
        template <class R >
        yes_type is_function_ptr_tester(R (__cdecl*)(...));
        #endif
        template <class R, class T0 >
        yes_type is_function_ptr_tester(R (*)(T0));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0 >
        yes_type is_function_ptr_tester(R (*)(T0 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0));
        template <class R, class T0 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0 ...));
        #ifndef _MANAGED
        template <class R, class T0 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0));
        template <class R, class T0 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0 ...));
        #endif
        template <class R, class T0 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0));
        template <class R, class T0 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0 ...));
        #endif
        template <class R, class T0, class T1 >
        yes_type is_function_ptr_tester(R (*)(T0, T1));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1 >
        yes_type is_function_ptr_tester(R (*)(T0, T1 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1));
        template <class R, class T0, class T1 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1));
        template <class R, class T0, class T1 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1 ...));
        #endif
        template <class R, class T0, class T1 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1));
        template <class R, class T0, class T1 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1 ...));
        #endif
        template <class R, class T0, class T1, class T2 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2));
        template <class R, class T0, class T1, class T2 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2));
        template <class R, class T0, class T1, class T2 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2 ...));
        #endif
        template <class R, class T0, class T1, class T2 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2));
        template <class R, class T0, class T1, class T2 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3));
        template <class R, class T0, class T1, class T2, class T3 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3));
        template <class R, class T0, class T1, class T2, class T3 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3));
        template <class R, class T0, class T1, class T2, class T3 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4));
        template <class R, class T0, class T1, class T2, class T3, class T4 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4));
        template <class R, class T0, class T1, class T2, class T3, class T4 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4));
        template <class R, class T0, class T1, class T2, class T3, class T4 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10 ...));
        #endif
#if 0 // 10 should be enough for now
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
        #ifndef AZSTD_TYPE_TRAITS_NO_ELLIPSIS_IN_FUNC_TESTING
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
        yes_type is_function_ptr_tester(R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...));
        #endif
        #ifdef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
        yes_type is_function_ptr_tester(R (__stdcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...));
        #ifndef _MANAGED
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
        yes_type is_function_ptr_tester(R (__fastcall*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...));
        #endif
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24));
        template <class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class T21, class T22, class T23, class T24 >
        yes_type is_function_ptr_tester(R (__cdecl*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24 ...));
        #endif
#endif // 0
    }
}

#endif // AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_TESTER_INCLUDED
#pragma once