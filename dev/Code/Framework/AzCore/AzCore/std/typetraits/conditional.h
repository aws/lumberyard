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
#ifndef AZSTD_TYPE_TRAITS_CONDITIONAL_INCLUDED
#define AZSTD_TYPE_TRAITS_CONDITIONAL_INCLUDED

#include <AzCore/std/typetraits/config.h>

namespace AZStd
{
    namespace Utils
    {
        //////////////////////////////////////////////////////////////////////////
        // Template if
        template<bool Condition, typename T1, typename T2>
        struct if_c
        {
            typedef T1 type;
        };

        template<typename T1, typename T2>
        struct if_c<false, T1, T2>
        {
            typedef T2 type;
        };

        template <bool B, class T = void>
        struct enable_if_c
        {
        };

        template <class T>
        struct enable_if_c<true, T> 
        {
            typedef T type;
        };
        //////////////////////////////////////////////////////////////////////////
    }

    template<bool Condition, typename T1, typename T2>
    struct conditional
    {
        typedef T2 type;
    };

    template<typename T1, typename T2>
    struct conditional<true, T1, T2>
    {
        typedef T1 type;
    };

    template <bool Condition, typename T1, typename T2>
    using conditional_t = typename conditional<Condition, T1, T2>::type;

    template<bool B, class T = void>
    struct enable_if
    {	
    };

    template<class T>
    struct enable_if<true, T>
    {
        typedef T type;
    };

    template<bool B, class T = void>
    using enable_if_t = typename enable_if<B, T>::type;
} // namespace AZStd

#endif // AZSTD_TYPE_TRAITS_CONDITIONAL_INCLUDED
#pragma once