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
#ifndef AZSTD_TYPE_TRAITS_UNDERLYING_TYPE_INCLUDED
#define AZSTD_TYPE_TRAITS_UNDERLYING_TYPE_INCLUDED

#include <AzCore/std/typetraits/config.h>

#ifdef AZSTD_UNDERLAYING_TYPE

namespace AZStd
{
    using std::underlying_type;
}

#else

#   include <AzCore/std/typetraits/is_enum.h>

namespace AZStd
{
    namespace Internal
    {
        template<size_t size>
        struct underlaying_type_guess;

        template<>
        struct underlaying_type_guess<1>
        {
            typedef AZ::u8 type;
        };

        template<>
        struct underlaying_type_guess<2>
        {
            typedef AZ::u16 type;
        };

        template<>
        struct underlaying_type_guess<4>
        {
            typedef AZ::u32 type;
        };
    }

    template<class T>
    struct underlying_type
    {
        AZ_STATIC_ASSERT(AZStd::is_enum<T>::value == true, "This is supported only for enums!");
        typedef typename Internal::underlaying_type_guess<sizeof(T)>::type type;
    };
}
#endif // AZSTD_UNDERLAYING_TYPE

#endif // AZSTD_TYPE_TRAITS_UNDERLYING_TYPE_INCLUDED
#pragma once