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
#ifndef AZSTD_TYPE_TRAITS_IS_EMPTY_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_EMPTY_INCLUDED

#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/internal/ice_or.h>
#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/intrinsics.h>

#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/add_reference.h>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct empty_helper_t1
            : public T
        {
            empty_helper_t1();  // hh compiler bug workaround
            int i[256];
        private:
            // suppress compiler warnings:
            empty_helper_t1(const empty_helper_t1&);
            empty_helper_t1& operator=(const empty_helper_t1&);
        };

        struct empty_helper_t2
        {
            int i[256];
        };

        template <typename T, bool is_a_class = false>
        struct empty_helper
        {
            AZSTD_STATIC_CONSTANT(bool, value = false);
        };

        template <typename T>
        struct empty_helper<T, true>
        {
            AZSTD_STATIC_CONSTANT(bool, value = (sizeof(empty_helper_t1<T>) == sizeof(empty_helper_t2)));
        };

        template <typename T>
        struct is_empty_impl
        {
            typedef typename remove_cv<T>::type cvt;
            AZSTD_STATIC_CONSTANT(bool, value = (
                            ::AZStd::type_traits::ice_or<
                                ::AZStd::Internal::empty_helper<cvt, ::AZStd::is_class<T>::value>::value
                            , AZSTD_IS_EMPTY(cvt)
                            >::value));
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_empty, T, ::AZStd::Internal::is_empty_impl<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_EMPTY_INCLUDED
#pragma once
