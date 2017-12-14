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
#ifndef AZSTD_TYPE_TRAITS_IS_FUNCTION_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_FUNCTION_INCLUDED

#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/internal/is_function_ptr_tester.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/remove_pointer.h>

namespace AZStd
{
    template<class T>
    struct is_function : Internal::is_function_pointer_tester<T> {};

    template<class T>
    struct is_function_pointer
        : AZStd::conditional_t<AZStd::is_pointer<T>::value, AZStd::is_function<AZStd::remove_pointer_t<T>>, AZStd::false_type>
    {};
}

#endif // AZSTD_TYPE_TRAITS_IS_FUNCTION_INCLUDED
#pragma once