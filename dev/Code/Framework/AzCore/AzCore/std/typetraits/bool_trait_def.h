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
#ifndef AZSTD_BOOL_TYPE_TRAITS_DEFINES_H
#define AZSTD_BOOL_TYPE_TRAITS_DEFINES_H 1

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/integral_constant.h>

/**
* Helper macros to declare bool type traits.
*/
#define AZSTD_TYPE_TRAIT_BOOL_DEF1(_TraitName, _Type, _BoolValue) \
    template< typename _Type >                                    \
    struct _TraitName                                             \
        : public ::AZStd::integral_constant<bool, _BoolValue>     \
    {};

#define AZSTD_TYPE_TRAIT_BOOL_DEF2(_TraitName, _Type1, _Type2, _BoolValue) \
    template< typename _Type1, typename _Type2 >                           \
    struct _TraitName                                                      \
        : public ::AZStd::integral_constant<bool, _BoolValue>              \
    {};

// Specialize the trait for a type.
#define AZSTD_TYPE_TRAIT_BOOL_SPEC1(_TraitName, _SpecializedType, _BoolValue) \
    template<>                                                                \
    struct _TraitName< _SpecializedType >                                     \
        : public ::AZStd::integral_constant<bool, _BoolValue>                 \
    {};

// Specialize the trait implementation for a type. (Implementations are up to user where needed and should always end "_impl")
#define AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(_TraitName, _SpecializedType, _BoolValue) \
    template<>                                                                     \
    struct _TraitName##_impl< _SpecializedType >                                   \
    {                                                                              \
        AZSTD_STATIC_CONSTANT(bool, value = (_BoolValue));                         \
    };

#define AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC2(_TraitName, _SpecializedType1, _SpecializedType2, _BoolValue) \
    template<>                                                                                         \
    struct _TraitName##_impl< _SpecializedType1, _SpecializedType2 >                                   \
    {                                                                                                  \
        AZSTD_STATIC_CONSTANT(bool, value = (_BoolValue));                                             \
    };

#define AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(_Param, _TraitName, _SpecializedType1, _SpecializedType2, _BoolValue) \
    template< _Param >                                                                                                   \
    struct _TraitName##_impl< _SpecializedType1, _SpecializedType2 >                                                     \
    {                                                                                                                    \
        AZSTD_STATIC_CONSTANT(bool, value = (_BoolValue));                                                               \
    };

// Partial specializations.
#define AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_1(_Param, _TraitName, _SpecializedType, _BoolValue) \
    template< _Param >                                                                          \
    struct _TraitName< _SpecializedType >                                                       \
        : public ::AZStd::integral_constant<bool, _BoolValue>                                   \
    {};

#define AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_2(_Param1, _Param2, _TraitName, _SpecializedType, _BoolValue) \
    template< _Param1, _Param2 >                                                                          \
    struct _TraitName< _SpecializedType >                                                                 \
        : public ::AZStd::integral_constant<bool, _BoolValue>                                             \
    {};

#define AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC2_1(_Param, _TraitName, _SpecializedType1, _SpecializedType2, _BoolValue) \
    template< _Param >                                                                                              \
    struct _TraitName< _SpecializedType1, _SpecializedType2 >                                                       \
        : public ::AZStd::integral_constant<bool, _BoolValue>                                                       \
    {};

#define AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC2_2(_Param1, _Param2, _TraitName, _SpecializedType1, _SpecializedType2, _BoolValue) \
    template< _Param1, _Param2 >                                                                                              \
    struct _TraitName< _SpecializedType1, _SpecializedType2 >                                                                 \
        : public ::AZStd::integral_constant<bool, _BoolValue>                                                                 \
    {};

// Specialize the trait for a type plus it's constant and volatile specializations.
#define AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(_TraitName, _SpecializedType, _BoolValue) \
    AZSTD_TYPE_TRAIT_BOOL_SPEC1(_TraitName, _SpecializedType, _BoolValue)                    \
    AZSTD_TYPE_TRAIT_BOOL_SPEC1(_TraitName, _SpecializedType const, _BoolValue)              \
    AZSTD_TYPE_TRAIT_BOOL_SPEC1(_TraitName, _SpecializedType volatile, _BoolValue)           \
    AZSTD_TYPE_TRAIT_BOOL_SPEC1(_TraitName, _SpecializedType const volatile, _BoolValue)

#endif // AZSTD_BOOL_TYPE_TRAITS_DEFINES_H
#pragma once