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
#ifndef AZSTD_TYPE_TRAITS_DEFINES_H
#define AZSTD_TYPE_TRAITS_DEFINES_H 1

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/integral_constant.h>

/**
 * Helper macros to define type traits.
 */
#define AZSTD_TYPE_TRAIT_DEF1(_TraitName, _Type, _ResultType) \
    template< typename _Type >                                \
    struct _TraitName                                         \
    {                                                         \
        typedef _ResultType type;                             \
    };                                                        \

#define AZSTD_TYPE_TRAIT_SPEC1(_TraitName, _SpecializedType, _ResultType) \
    template<>                                                            \
    struct _TraitName< _SpecializedType >                                 \
    {                                                                     \
        typedef _ResultType type;                                         \
    };                                                                    \

#define AZSTD_TYPE_TRAIT_IMPL_SPEC1(_TraitName, _SpecializedType, _ResultType) \
    template<>                                                                 \
    struct _TraitName##_impl< _SpecializedType >                               \
    {                                                                          \
        typedef _ResultType type;                                              \
    };                                                                         \

#define AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(_Param, _TraitName, _SpecializedType, _ResultType) \
    template< _Param >                                                                      \
    struct _TraitName< _SpecializedType >                                                   \
    {                                                                                       \
        typedef _ResultType type;                                                           \
    };                                                                                      \

#define AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(_Param1, _Param2, _TraitName, _SpecializedType, _ResultType) \
    template< _Param1, _Param2 >                                                                      \
    struct _TraitName< _SpecializedType >                                                             \
    {                                                                                                 \
        typedef _ResultType;                                                                          \
    };                                                                                                \

#define AZSTD_TYPE_TRAIT_IMPL_PARTIAL_SPEC1_1(_Param, _TraitName, _SpecializedType, _ResultType) \
    template< _Param >                                                                           \
    struct _TraitName##_impl< _SpecializedType >                                                 \
    {                                                                                            \
        typedef _ResultType type;                                                                \
    };                                                                                           \

#endif // AZSTD_TYPE_TRAITS_DEFINES_H
#pragma once