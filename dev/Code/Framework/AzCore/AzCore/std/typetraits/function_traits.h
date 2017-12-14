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
#ifndef AZSTD_TYPE_TRAITS_FUNCTION_TRAITS_INCLUDED
#define AZSTD_TYPE_TRAITS_FUNCTION_TRAITS_INCLUDED

#include <AzCore/std/typetraits/is_function.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/typetraits/internal/type_sequence_traits.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename Function>
        struct function_traits_helper : function_traits_helper<decltype(&Function::operator())> {};

        #define AZSTD_FUNCTION_TRAIT_NOARG

        #define AZSTD_FUNCTION_TRAIT_HELPER(cv_ref_qualifier) \
            template <typename C, typename R, typename... Args> \
            struct function_traits_helper<R(C::*)(Args...) cv_ref_qualifier> : function_traits_helper<R(Args...)> \
            { \
                using class_fp_type = R(C::*)(Args...) cv_ref_qualifier; \
                using class_type = C; \
            }; \
            template <typename C, typename R, typename... Args> \
            struct function_traits_helper<R(C::*)(Args..., ...) cv_ref_qualifier> : function_traits_helper<R(Args...)> \
            { \
                using class_fp_type = R(C::*)(Args..., ...) cv_ref_qualifier; \
                using class_type = C; \
            };

        // specializations for functions with const volatile ref qualifiers
        AZSTD_FUNCTION_TRAIT_HELPER(AZSTD_FUNCTION_TRAIT_NOARG);
        AZSTD_FUNCTION_TRAIT_HELPER(const);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile);
#if !defined(AZ_COMPILER_MSVC) || AZ_COMPILER_MSVC >= 1900
        AZSTD_FUNCTION_TRAIT_HELPER(&);
        AZSTD_FUNCTION_TRAIT_HELPER(const&);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile&);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile&);
        AZSTD_FUNCTION_TRAIT_HELPER(&&);
        AZSTD_FUNCTION_TRAIT_HELPER(const&&);
        AZSTD_FUNCTION_TRAIT_HELPER(volatile&&);
        AZSTD_FUNCTION_TRAIT_HELPER(const volatile&&);
#endif
        #undef AZSTD_FUNCTION_TRAIT_HELPER
        #undef AZSTD_FUNCTION_TRAIT_NOARG

        template <typename R, typename ...Args>
        struct function_traits_helper<R(Args...)>
        {
            AZSTD_STATIC_CONSTANT(size_t, arity = sizeof...(Args));
            AZSTD_STATIC_CONSTANT(size_t, num_args = sizeof...(Args));
            using result_type = R;
            using raw_fp_type = result_type(*)(Args...);
            
            template<size_t index>
            using get_arg_t = pack_traits_get_arg_t<index, Args...>;
            using arg_sequence = pack_traits_arg_sequence<Args...>;
        };

        template <typename R, typename ...Args>
        struct function_traits_helper<R(Args..., ...)>
        {
            AZSTD_STATIC_CONSTANT(size_t, arity = sizeof...(Args));
            AZSTD_STATIC_CONSTANT(size_t, num_args = sizeof...(Args));
            using result_type = R;
            using raw_fp_type = result_type(*)(Args...);

            template<size_t index>
            using get_arg_t = pack_traits_get_arg_t<index, Args...>;
            using arg_sequence = pack_traits_arg_sequence<Args...>;
        };
    }

    template <typename Function>
    struct function_traits : public Internal::function_traits_helper<Function>{};

    template <typename Function>
    struct function_traits<Function*> : public Internal::function_traits_helper<remove_pointer_t<Function>> {};

    template <typename Function>
    using function_traits_get_result_t = typename function_traits<Function>::result_type;

    template <typename Function, size_t index>
    using function_traits_get_arg_t = typename function_traits<Function>::template get_arg_t<index>;
}

#endif // AZSTD_TYPE_TRAITS_FUNCTION_TRAITS_INCLUDED
#pragma once