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
#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/integral_constant.h>

#define AZSTD_TYPE_TRAITS_SIZE_T_TRAIT_DEF1(_TraitName, _TypeName, _SizeValue) \
    template< typename _TypeName >                                             \
    struct _TraitName                                                          \
        : public ::AZStd::integral_constant<AZStd::size_t, _SizeValue>         \
    {};
/**/

#define AZSTD_TYPE_TRAITS_SIZE_T_TRAIT_SPEC1(_TraitName, _Specialization, _SizeValue) \
    template<>                                                                        \
    struct _TraitName<_Specialization>                                                \
        : public ::AZStd::integral_constant<AZStd::size_t, _SizeValue>                \
    {};
/**/

#define AZSTD_TYPE_TRAITS_SIZE_T_TRAIT_PARTIAL_SPEC1_1(_Param, _TraitName, _Specialization, _SizeValue) \
    template< _Param >                                                                                  \
    struct _TraitName<_Specialization>                                                                  \
        : public ::AZStd::integral_constant<AZStd::size_t, _SizeValue>                                  \
    {};
/**/
#pragma once