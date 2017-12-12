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
#ifndef AZSTD_BIND_ARG_HPP_INCLUDED
#define AZSTD_BIND_ARG_HPP_INCLUDED

// Based on boost 1.39.0

#include <AzCore/std/base.h>

namespace AZStd
{
    template< class T >
    struct is_placeholder
    {
        enum _vt
        {
            value = 0
        };
    };

    template< int I >
    struct arg
    {
        arg()
        {
        }

        template< class T >
        arg(T const& /* t */)
        {
            AZ_STATIC_ASSERT(I == is_placeholder<T>::value, "T must be placeholder");
        }
    };

    template< int I >
    bool operator==(arg<I> const&, arg<I> const&)
    {
        return true;
    }

    template< int I >
    struct is_placeholder< arg<I> >
    {
        enum _vt
        {
            value = I
        };
    };

    template< int I >
    struct is_placeholder< arg<I> (*)() >
    {
        enum _vt
        {
            value = I
        };
    };
} // namespace AZStd

#endif // #ifndef AZSTD_BIND_ARG_HPP_INCLUDED
#pragma once