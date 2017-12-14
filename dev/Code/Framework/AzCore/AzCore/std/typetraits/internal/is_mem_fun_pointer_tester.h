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
#ifndef AZSTD_TYPE_TRAITS_INTERNAL_IS_MEM_FUN_POINTER_TESTER_INCLUDED
#define AZSTD_TYPE_TRAITS_INTERNAL_IS_MEM_FUN_POINTER_TESTER_INCLUDED

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/internal/is_function_ptr_tester.h>

namespace AZStd
{
    namespace Internal
    {
        template<class>
        struct is_member_function_pointer_tester : AZStd::false_type {};

        template<class R, class ClassType>
        struct is_member_function_pointer_tester<R ClassType::*> : is_function_pointer_tester<AZStd::remove_cv_t<R>>{};

        // VS 2013 does not deduce a member function pointer type to the <R ClassType::*> above
        // So explicit specializations are added for it.
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
        template<class R, class ClassType, class... Args>
        struct is_member_function_pointer_tester<R (ClassType::*)(Args...)> : AZStd::true_type {};

        template<class R, class ClassType, class... Args>
        struct is_member_function_pointer_tester<R(ClassType::*)(Args...) const> : AZStd::true_type {};

        template<class R, class ClassType, class... Args>
        struct is_member_function_pointer_tester<R(ClassType::*)(Args...) volatile> : AZStd::true_type {};

        template<class R, class ClassType, class... Args>
        struct is_member_function_pointer_tester<R(ClassType::*)(Args...) const volatile> : AZStd::true_type {};
#endif
    }
}

#endif // AZSTD_TYPE_TRAITS_INTERNAL_IS_MEM_FUN_POINTER_TESTER_INCLUDED
#pragma once