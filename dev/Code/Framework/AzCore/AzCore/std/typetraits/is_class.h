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
#ifndef AZSTD_TYPE_TRAITS_IS_CLASS_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_CLASS_INCLUDED

#include <AzCore/std/typetraits/intrinsics.h>
#ifndef AZSTD_IS_CLASS
    #include <AzCore/std/typetraits/is_union.h>
    #include <AzCore/std/typetraits/internal/ice_and.h>
    #include <AzCore/std/typetraits/internal/ice_not.h>

#ifdef AZSTD_TYPE_TRAITS_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION
    #include <AzCore/std/typetraits/internal/yes_no_type.h>
#else
    #include <AzCore/std/typetraits/is_scalar.h>
    #include <AzCore/std/typetraits/is_array.h>
    #include <AzCore/std/typetraits/is_reference.h>
    #include <AzCore/std/typetraits/is_void.h>
    #include <AzCore/std/typetraits/is_function.h>
#endif
#endif // AZSTD_IS_CLASS

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
#ifndef AZSTD_IS_CLASS
    #ifdef AZSTD_TYPE_TRAITS_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION
        // is_class<> metafunction due to Paul Mensonides
        // (leavings@attbi.com). For more details:
        // http://groups.google.com/groups?hl=en&selm=000001c1cc83%24e154d5e0%247772e50c%40c161550a&rnum=1
        #if defined(AZ_COMPILER_GCC)

        template <class U>
            ::AZStd::type_traits::yes_type is_class_tester(void(U::*)(void));
        template <class U>
            ::AZStd::type_traits::no_type is_class_tester(...);

        template <typename T>
        struct is_class_impl
        {
            AZSTD_STATIC_CONSTANT(bool, value =
                    (::AZStd::type_traits::ice_and<
                         sizeof(is_class_tester<T>(0)) == sizeof(::AZStd::type_traits::yes_type),
                             ::AZStd::type_traits::ice_not< ::AZStd::is_union<T>::value >::value
                         >::value)
                );
        };
        #else
        template <typename T>
        struct is_class_impl
        {
            template <class U>
            static ::AZStd::type_traits::yes_type is_class_tester(void(U::*)(void));
            template <class U>
            static ::AZStd::type_traits::no_type is_class_tester(...);

            AZSTD_STATIC_CONSTANT(bool, value =
                    (::AZStd::type_traits::ice_and<
                         sizeof(is_class_tester<T>(0)) == sizeof(::AZStd::type_traits::yes_type),
                             ::AZStd::type_traits::ice_not< ::AZStd::is_union<T>::value >::value
                         >::value)
                );
        };
        #endif
    #else
        template <typename T>
        struct is_class_impl
        {
            AZSTD_STATIC_CONSTANT(bool, value =
                    (::AZStd::type_traits::ice_and<
                             ::AZStd::type_traits::ice_not< ::AZStd::is_union<T>::value >::value,
                             ::AZStd::type_traits::ice_not< ::AZStd::is_scalar<T>::value >::value,
                             ::AZStd::type_traits::ice_not< ::AZStd::is_array<T>::value >::value,
                             ::AZStd::type_traits::ice_not< ::AZStd::is_reference<T>::value>::value,
                             ::AZStd::type_traits::ice_not< ::AZStd::is_void<T>::value >::value,
                             ::AZStd::type_traits::ice_not< ::AZStd::is_function<T>::value >::value
                         >::value));
        };
    # endif // AZSTD_TYPE_TRAITS_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION
# else // AZSTD_IS_CLASS
        template <typename T>
        struct is_class_impl
        {
            AZSTD_STATIC_CONSTANT(bool, value = AZSTD_IS_CLASS(T));
        };
# endif // AZSTD_IS_CLASS
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_class, T, ::AZStd::Internal::is_class_impl<T>::value)
}
#endif // AZSTD_TYPE_TRAITS_IS_CLASS_INCLUDED
#pragma once