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
#ifndef AZSTD_BIND_PLACEHOLDERS_HPP_INCLUDED
#define AZSTD_BIND_PLACEHOLDERS_HPP_INCLUDED

// Based on boost 1.39.0
#include <AzCore/std/base.h>
#include <AzCore/std/bind/arg.h>

namespace AZStd
{
    namespace placeholders
    {
#if defined(AZ_COMPILER_MSVC) || defined(AZ_COMPILER_MWERKS) || defined(AZ_COMPILER_GCC) || defined(AZ_COMPILER_SNC) || defined(AZ_COMPILER_CLANG)
        static arg<1> _1;
        static arg<2> _2;
        static arg<3> _3;
        static arg<4> _4;
        static arg<5> _5;
        static arg<6> _6;
        static arg<7> _7;
        static arg<8> _8;
        static arg<9> _9;
#else
        arg<1> _1;
        arg<2> _2;
        arg<3> _3;
        arg<4> _4;
        arg<5> _5;
        arg<6> _6;
        arg<7> _7;
        arg<8> _8;
        arg<9> _9;
#endif
    }
}

#endif // #ifndef AZSTD_BIND_PLACEHOLDERS_HPP_INCLUDED
#pragma once