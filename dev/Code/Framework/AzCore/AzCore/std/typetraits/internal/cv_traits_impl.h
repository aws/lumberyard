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
#ifndef AZSTD_TYPE_TRAINTS_INTERNAL_CV_TRAITS_IMPL_INCLUDED
#define AZSTD_TYPE_TRAINTS_INTERNAL_CV_TRAITS_IMPL_INCLUDED

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/integral_constant.h>

// implementation helper:
namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct cv_traits_imp {};

        template <typename T>
        struct cv_traits_imp<T*>
        {
            AZSTD_STATIC_CONSTANT(bool, is_const = false);
            AZSTD_STATIC_CONSTANT(bool, is_volatile = false);
            typedef T unqualified_type;
        };

        template <typename T>
        struct cv_traits_imp<const T*>
        {
            AZSTD_STATIC_CONSTANT(bool, is_const = true);
            AZSTD_STATIC_CONSTANT(bool, is_volatile = false);
            typedef T unqualified_type;
        };

        template <typename T>
        struct cv_traits_imp<volatile T*>
        {
            AZSTD_STATIC_CONSTANT(bool, is_const = false);
            AZSTD_STATIC_CONSTANT(bool, is_volatile = true);
            typedef T unqualified_type;
        };

        template <typename T>
        struct cv_traits_imp<const volatile T*>
        {
            AZSTD_STATIC_CONSTANT(bool, is_const = true);
            AZSTD_STATIC_CONSTANT(bool, is_volatile = true);
            typedef T unqualified_type;
        };
    }
}

#endif // AZSTD_TYPE_TRAINTS_INTERNAL_CV_TRAITS_IMPL_INCLUDED
#pragma once