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
#ifndef AZSTD_TYPE_TRAITS_IS_UNION_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_UNION_INCLUDED

#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/intrinsics.h>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct is_union_impl
        {
            typedef typename remove_cv<T>::type cvt;
            AZSTD_STATIC_CONSTANT(bool, value = AZSTD_IS_UNION(cvt));
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_union, T, ::AZStd::Internal::is_union_impl<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_UNION_INCLUDED
#pragma once
