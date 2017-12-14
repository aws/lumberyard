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
#ifndef AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_TESTER_INCLUDED
#define AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_TESTER_INCLUDED

#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    namespace Internal
    {
        template<class>
        struct is_function_pointer_tester : AZStd::false_type{};

        #define AZSTD_FUNCTION_TESTER_NOARG

        #define AZSTD_FUNCTION_POINTER_TESTER(cv_ref_qualifier) \
                template<class R, class... Args> \
                struct is_function_pointer_tester<R(Args...) cv_ref_qualifier> : AZStd::true_type {}; \
                template<class R, class... Args> \
                struct is_function_pointer_tester<R(Args..., ...) cv_ref_qualifier> : AZStd::true_type {};
        
        // specializations for functions with const volatile ref qualifiers
        AZSTD_FUNCTION_POINTER_TESTER(AZSTD_FUNCTION_TESTER_NOARG);
        // VS2013 does not support const volatile ref-qualifiers on non-member function signatures 
#if !defined(AZ_COMPILER_MSVC) || AZ_COMPILER_MSVC >= 1900
        AZSTD_FUNCTION_POINTER_TESTER(const);
        AZSTD_FUNCTION_POINTER_TESTER(volatile);
        AZSTD_FUNCTION_POINTER_TESTER(const volatile);
        AZSTD_FUNCTION_POINTER_TESTER(&);
        AZSTD_FUNCTION_POINTER_TESTER(const&);
        AZSTD_FUNCTION_POINTER_TESTER(volatile&);
        AZSTD_FUNCTION_POINTER_TESTER(const volatile&);
        AZSTD_FUNCTION_POINTER_TESTER(&&);
        AZSTD_FUNCTION_POINTER_TESTER(const&&);
        AZSTD_FUNCTION_POINTER_TESTER(volatile&&);
        AZSTD_FUNCTION_POINTER_TESTER(const volatile&&);
#endif
        #undef AZSTD_FUNCTION_POINTER_TESTER

        #undef AZSTD_FUNCTION_TESTER_NOARG
    }
}

#endif // AZSTD_TYPE_TRAITS_INTERNAL_IS_FUNCTION_PTR_TESTER_INCLUDED
#pragma once