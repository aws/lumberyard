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
#ifndef AZSTD_TYPE_TRAITS_IS_SIGNED_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_SIGNED_INCLUDED

#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/internal/ice_or.h>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <class T>
        struct is_signed_helper
        {
            typedef typename remove_cv<T>::type no_cv_t;
            AZSTD_STATIC_CONSTANT(bool, value = (static_cast<no_cv_t>(-1) < 0));
        };

        template <bool integral_type>
        struct is_signed_select_helper
        {
            template <class T>
            struct rebind
            {
                typedef is_signed_helper<T> type;
            };
        };

        template <>
        struct is_signed_select_helper<false>
        {
            template <class T>
            struct rebind
            {
                typedef false_type type;
            };
        };

        template <class T>
        struct is_signed_imp
        {
            typedef is_signed_select_helper<
                    ::AZStd::type_traits::ice_or<
                        ::AZStd::is_integral<T>::value,
                        ::AZStd::is_enum<T>::value>::value
                > selector;
            typedef typename selector::template rebind<T> binder;
            typedef typename binder::type type;
            AZSTD_STATIC_CONSTANT(bool, value = type::value);
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_signed, T, ::AZStd::Internal::is_signed_imp<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_SIGNED_INCLUDED
#pragma once