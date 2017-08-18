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
#ifndef AZSTD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_INCLUDED

#include <AzCore/std/typetraits/integral_constant.h>

#ifndef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
    #include <AzCore/std/typetraits/internal/is_mem_fun_pointer_impl.h>
    #include <AzCore/std/typetraits/remove_cv.h>
#else
    #include <AzCore/std/typetraits/is_reference.h>
    #include <AzCore/std/typetraits/is_array.h>
    #include <AzCore/std/typetraits/internal/yes_no_type.h>
    #include <AzCore/std/typetraits/internal/false_result.h>
    #include <AzCore/std/typetraits/internal/ice_or.h>
    #include <AzCore/std/typetraits/internal/is_mem_fun_pointer_tester.h>
#endif

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
#ifndef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_member_function_pointer, T, ::AZStd::type_traits::is_mem_fun_pointer_impl<typename remove_cv<T>::type>::value)
#else
    namespace Internal
    {
        template <bool>
        struct is_mem_fun_pointer_select
            : ::AZStd::type_traits::false_result
        {};

        template <>
        struct is_mem_fun_pointer_select<false>
        {
            template <typename T>
            struct result_
            {
            #if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
                #pragma warning(push)
                #pragma warning(disable:6334)
            #endif
                static T* make_t;
                typedef result_<T> self_type;

                AZSTD_STATIC_CONSTANT(
                    bool, value = (1 == sizeof(::AZStd::type_traits::is_mem_fun_pointer_tester(self_type::make_t))
                                   ));
            #if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
                #pragma warning(pop)
            #endif
            };
        };

        template <typename T>
        struct is_member_function_pointer_impl
            : is_mem_fun_pointer_select<
                    ::AZStd::type_traits::ice_or<
                        ::AZStd::is_reference<T>::value
                    , ::AZStd::is_array<T>::value
                    >::value
                >::template result_<T>
        {};

        template <typename T>
        struct is_member_function_pointer_impl<T&>
            : public false_type {};

        AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(is_member_function_pointer, void, false)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(is_member_function_pointer, void const, false)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(is_member_function_pointer, void volatile, false)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(is_member_function_pointer, void const volatile, false)
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_member_function_pointer, T, ::AZStd::Internal::is_member_function_pointer_impl<T>::value)
#endif
}

#endif // AZSTD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_INCLUDED
#pragma once