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
#ifndef AZSTD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_INCLUDED

#include <AzCore/std/typetraits/internal/is_mem_fun_pointer_tester.h>
#include <AzCore/std/typetraits/remove_cv.h>

namespace AZStd
{
    template<typename T>
    struct is_member_function_pointer : Internal::is_member_function_pointer_tester<AZStd::remove_cv_t<T>> {};
}

#endif // AZSTD_TYPE_TRAITS_IS_MEMBER_FUNCTION_POINTER_INCLUDED
#pragma once