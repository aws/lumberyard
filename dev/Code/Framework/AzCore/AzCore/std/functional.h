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
#ifndef AZSTD_FUNCTIONAL_H
#define AZSTD_FUNCTIONAL_H 1

// only the basics used is most of the AZStd
#include <AzCore/std/functional_basic.h>

//////////////////////////////////////////////////////////////////////////
// TR1 MemFn and Bind
//////////////////////////////////////////////////////////////////////////
#include <AzCore/std/bind/mem_fn.h>
#include <AzCore/std/bind/bind.h>

//////////////////////////////////////////////////////////////////////////
// TR1 function
//////////////////////////////////////////////////////////////////////////
/**
* Function is a tuned version of the boost.function library. It's what TR1 based on.
* Till we have TR1 support we should have our implementation.
*/
#if /*defined(AZSTD_HAS_TYPE_TRAITS_INTRINSICS)*/ 0
#   ifdef AZ_COMPILER_MSVC
#       include <functional>
namespace AZStd
{
    // wait for varadic templates too much typing
    std::tr1::function <
}
#   endif
#else // !AZSTD_HAS_TYPE_TRAITS_INTRINSICS
    #include <AzCore/std/allocator.h>
    #include <AzCore/std/function/function_base.h>

    #define AZSTD_FUNCTION_NUM_ARGS 0
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 1
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 2
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 3
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 4
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 5
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 6
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 7
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 8
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 9
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
    #define AZSTD_FUNCTION_NUM_ARGS 10
    #include <AzCore/std/function/maybe_include.h>
    #undef AZSTD_FUNCTION_NUM_ARGS
#endif

#endif // AZSTD_FUNCTIONAL_H
#pragma once