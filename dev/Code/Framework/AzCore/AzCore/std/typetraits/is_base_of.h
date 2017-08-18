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
#ifndef AZSTD_TYPE_TRAITS_IS_BASE_OF_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_BASE_OF_INCLUDED

#include <AzCore/std/typetraits/is_base_and_derived.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/internal/ice_or.h>
#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    AZSTD_TYPE_TRAIT_BOOL_DEF2(is_base_of, Base, Derived
        , (::AZStd::type_traits::ice_or<
               (::AZStd::Internal::is_base_and_derived_impl<Base, Derived>::value),
               (::AZStd::is_same<Base, Derived>::value)>::value)
        )

    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC2_2(typename Base, typename Derived, is_base_of, Base &, Derived, false)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC2_2(typename Base, typename Derived, is_base_of, Base, Derived &, false)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC2_2(typename Base, typename Derived, is_base_of, Base &, Derived &, false)
}

#endif // AZSTD_TYPE_TRAITS_IS_BASE_OF_INCLUDED
#pragma once