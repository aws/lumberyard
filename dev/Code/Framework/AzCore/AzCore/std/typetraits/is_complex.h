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
#ifndef AZSTD_TYPE_TRAITS_IS_COMPLE
#define AZSTD_TYPE_TRAITS_IS_COMPLE

#if 0
#include <AzCore/std/typetraits/is_convertible.h>
#include <complex>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        struct is_convertible_from_tester
        {
            template <class T>
            is_convertible_from_tester(const AZSTD_STL::complex<T>&);
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_complex, T, (::AZStd::is_convertible<T, Internal::is_convertible_from_tester>::value))
}
#endif

#endif //AZSTD_TYPE_TRAITS_IS_COMPLE
#pragma once