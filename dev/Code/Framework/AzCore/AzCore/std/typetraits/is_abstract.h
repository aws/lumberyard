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
#ifndef AZSTD_TYPE_TRAITS_IS_ABSTRACT_CLASS
#define AZSTD_TYPE_TRAITS_IS_ABSTRACT_CLASS

// Compile type discovery whether given type is abstract class or not.
//
//   Requires DR 337 to be supported by compiler
//   (http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/cwg_active.html#337).
//
//
// Believed (Jan 2004) to work on:
//  - GCC 3.4
//  - VC++ 7.1
//  - compilers with new EDG frontend (Intel C++ 7, Comeau 4.3.2)
//
// Doesn't work on:
//  - VC++6, VC++7.0 and less
//  - GCC 3.3.X and less
//  - Borland C++ 6 and less

#include <AzCore/std/typetraits/intrinsics.h>
#ifndef AZSTD_IS_ABSTRACT
    #include <AzCore/std/typetraits/internal/yes_no_type.h>
    #include <AzCore/std/typetraits/is_class.h>
    #include <AzCore/std/typetraits/internal/ice_and.h>
#endif

#include <AzCore/std/typetraits/bool_trait_def.h>


namespace AZStd
{
    namespace Internal
    {
#ifdef AZSTD_IS_ABSTRACT
        template <class T>
        struct is_abstract_imp
        {
            AZSTD_STATIC_CONSTANT(bool, value = AZSTD_IS_ABSTRACT(T));
        };
#else
        template<class T>
        struct is_abstract_imp2
        {
            // Deduction fails if T is void, function type,
            // reference type (14.8.2/2)or an abstract class type
            // according to review status issue #337
            //
            template<class U>
            static type_traits::no_type check_sig(U (*)[1]);
            template<class U>
            static type_traits::yes_type check_sig(...);
            //
            // T must be a complete type, further if T is a template then
            // it must be instantiated in order for us to get the right answer:
            //
            AZ_STATIC_ASSERT(sizeof(T) != 0, "T must be a complete type, size can't be 0");

            // GCC2 won't even parse this template if we embed the computation
            // of s1 in the computation of value.
#ifdef AZ_COMPILER_GCC
            AZSTD_STATIC_CONSTANT(unsigned, s1 = sizeof(is_abstract_imp2<T>::template check_sig<T>(0)));
#else
#if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
#   pragma warning(push)
#   pragma warning(disable:6334)
#endif
            AZSTD_STATIC_CONSTANT(unsigned, s1 = sizeof(check_sig<T>(0)));
#if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
#   pragma warning(pop)
#endif
#endif
            AZSTD_STATIC_CONSTANT(bool, value = (s1 == sizeof(type_traits::yes_type)));
        };

        template <bool v>
        struct is_abstract_select
        {
            template <class T>
            struct rebind
            {
                typedef is_abstract_imp2<T> type;
            };
        };
        template <>
        struct is_abstract_select<false>
        {
            template <class T>
            struct rebind
            {
                typedef false_type type;
            };
        };

        template <class T>
        struct is_abstract_imp
        {
            typedef is_abstract_select< ::AZStd::is_class<T>::value> selector;
            typedef typename selector::template rebind<T> binder;
            typedef typename binder::type type;

            AZSTD_STATIC_CONSTANT(bool, value = type::value);
        };
    #endif
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_abstract, T, ::AZStd::Internal::is_abstract_imp<T>::value)
}

#endif //AZSTD_TYPE_TRAITS_IS_ABSTRACT_CLASS
#pragma once