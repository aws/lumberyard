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
#ifndef AZSTD_TYPE_TRAITS_IS_POLYMORPHIC
#define AZSTD_TYPE_TRAITS_IS_POLYMORPHIC

#include <AzCore/std/typetraits/intrinsics.h>
#ifndef AZSTD_IS_POLYMORPHIC
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/remove_cv.h>
#endif

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    #ifndef AZSTD_IS_POLYMORPHIC
    namespace Internal
    {
        template <class T>
        struct is_polymorphic_imp1
        {
            typedef typename remove_cv<T>::type ncvT;
            struct d1
                : public ncvT
            {
                d1();
            #if !defined(AZ_COMPILER_GCC) // this raises warnings with some classes, and buys nothing with GCC
                ~d1() throw();
            #endif
                char padding[256];
            private:
                // keep some picky compilers happy:
                d1(const d1&);
                d1& operator=(const d1&);
            };

            struct d2
                : public ncvT
            {
                d2();
                virtual ~d2() throw();
            #if !defined(AZ_COMPILER_MSVC)
                // for some reason this messes up VC++ when T has virtual bases,
                // probably likewise for compilers that use the same ABI:
                struct unique{};
                virtual void unique_name_to_boost5487629(unique*);
            #endif
                char padding[256];
            private:
                // keep some picky compilers happy:
                d2(const d2&);
                d2& operator=(const d2&);
            };

            AZSTD_STATIC_CONSTANT(bool, value = (sizeof(d2) == sizeof(d1)));
        };

        template <class T>
        struct is_polymorphic_imp2
        {
            AZSTD_STATIC_CONSTANT(bool, value = false);
        };

        template <bool is_class>
        struct is_polymorphic_selector
        {
            template <class T>
            struct rebind
            {
                typedef is_polymorphic_imp2<T> type;
            };
        };

        template <>
        struct is_polymorphic_selector<true>
        {
            template <class T>
            struct rebind
            {
                typedef is_polymorphic_imp1<T> type;
            };
        };

        template <class T>
        struct is_polymorphic_imp
        {
            typedef is_polymorphic_selector< ::AZStd::is_class<T>::value> selector;
            typedef typename selector::template rebind<T> binder;
            typedef typename binder::type imp_type;
            AZSTD_STATIC_CONSTANT(bool, value = imp_type::value);
        };
    }     // namespace Internal

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_polymorphic, T, ::AZStd::Internal::is_polymorphic_imp<T>::value)

    #else // AZSTD_IS_POLYMORPHIC

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_polymorphic, T, AZSTD_IS_POLYMORPHIC(T))

    #endif
}

#endif // AZSTD_TYPE_TRAITS_IS_POLYMORPHIC
#pragma once