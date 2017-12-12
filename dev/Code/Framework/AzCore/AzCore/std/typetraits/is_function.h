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
#ifndef AZSTD_TYPE_TRAITS_IS_FUNCTION_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_FUNCTION_INCLUDED

#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/typetraits/internal/false_result.h>

#if !defined(AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS)
    #include <AzCore/std/typetraits/internal/is_function_ptr_helper.h>
#else
    #include <AzCore/std/typetraits/internal/is_function_ptr_tester.h>
    #include <AzCore/std/typetraits/internal/yes_no_type.h>
#endif

#include <AzCore/std/typetraits/bool_trait_def.h>

// is a type a function?
// Please note that this implementation is unnecessarily complex:
// we could just use !is_convertible<T*, const volatile void*>::value,
// except that some compilers erroneously allow conversions from
// function pointers to void*.

namespace AZStd
{
    namespace Internal
    {
    #ifndef AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template<bool is_ref = true>
        struct is_function_chooser
            : public ::AZStd::type_traits::false_result
        {
        };

        template <>
        struct is_function_chooser<false>
        {
            template< typename T >
            struct result_
                : public ::AZStd::type_traits::is_function_ptr_helper<T*>
            {
            };
        };

        template <typename T>
        struct is_function_impl
            : public  is_function_chooser< ::AZStd::is_reference<T>::value >
                ::template result_<T>
        {};
    #else  // AZSTD_TYPE_TRAITS_TEST_MS_FUNC_SIGS
        template <typename T>
        struct is_function_impl
        {
        #if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
            #pragma warning(push)
            #pragma warning(disable:6334)
        #endif
            static T* t;
            AZSTD_STATIC_CONSTANT(
                bool, value = sizeof(::AZStd::type_traits::is_function_ptr_tester(t))
                    == sizeof(::AZStd::type_traits::yes_type)
                );
        #if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
            #pragma warning(pop)
        #endif
        };

        template <typename T>
        struct is_function_impl<T&>
            : public false_type
        {};
    #endif
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_function, T, ::AZStd::Internal::is_function_impl<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_FUNCTION_INCLUDED
#pragma once