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
#ifndef AZSTD_TYPE_TRAITS_IS_POD_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_POD_INCLUDED

#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/is_scalar.h>
#include <AzCore/std/typetraits/internal/ice_or.h>
#include <AzCore/std/typetraits/intrinsics.h>
#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct is_pod_impl
        {
            AZSTD_STATIC_CONSTANT(
                bool, value =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_scalar<T>::value,
                             ::AZStd::is_void<T>::value,
                         AZSTD_IS_POD(T)
                         >::value));
        };

        template <typename T, AZStd::size_t sz>
        struct is_pod_impl<T[sz]>
            : public is_pod_impl<T>
        {};
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_pod, T, ::AZStd::Internal::is_pod_impl<T>::value)
}

#if defined(AZSTD_HAS_TYPE_TRAITS_INTRINSICS)
// Supported by compiler.
    #define AZSTD_DECLARE_POD_TYPE(_Type)
    #define AZSTD_DECLARE_POD_TYPE_T1(_Type, _T1)
    #define AZSTD_DECLARE_POD_TYPE_T2(_Type, _T1, _T2)
    #define AZSTD_DECLARE_POD_TYPE_T3(_Type, _T1, _T2, _T3)
#else
    #define AZSTD_DECLARE_POD_TYPE(_Type)                   namespace AZStd { template<>             \
                                                                              struct is_pod< _Type > \
                                                                                  : public true_type {}; }
    #define AZSTD_DECLARE_POD_TYPE_T1(_Type, _T1)            namespace AZStd { template< typename _T1 >    \
                                                                               struct is_pod< _Type<_T1> > \
                                                                                   : public true_type {}; }
    #define AZSTD_DECLARE_POD_TYPE_T2(_Type, _T1, _T2)        namespace AZStd { template< typename _T1, typename _T2 > \
                                                                                struct is_pod< _Type<_T1, _T2> >       \
                                                                                    : public true_type {}; }
    #define AZSTD_DECLARE_POD_TYPE_T3(_Type, _T1, _T2, _T3)    namespace AZStd { template< typename _T1, typename _T2, typename _T3 > \
                                                                                 struct is_pod< _Type<_T1, _T2, _T3> >                \
                                                                                     : public true_type {}; }
#endif

#endif // AZSTD_TYPE_TRAITS_IS_POD_INCLUDED
#pragma once