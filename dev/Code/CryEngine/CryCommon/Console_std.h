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
#ifndef CRYINCLUDE_CRYCOMMON_CONSOLE_STD_H
#define CRYINCLUDE_CRYCOMMON_CONSOLE_STD_H
#pragma once


#include <memory>

namespace std
{
#if defined(MISSING_CONSOLE_STD_REQUIRED)
    // implement any std functionality console don't yet have here

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // make_unique not available on consoles yet, implement a non variadic template version
    template<typename T>
    unique_ptr<T> make_unique()
    {
        return unique_ptr<T>(new T);
    }

    template <typename T, typename U>
    unique_ptr<T> make_unique(U&& arg)
    {
        return std::unique_ptr<T>(new T(std::forward<U>(arg)));
    }

    template <typename T, typename U, typename V>
    unique_ptr<T> make_unique(U&& arg, V&& arg1)
    {
        return std::unique_ptr<T>(new T(std::forward<U>(arg), std::forward<V>(arg1)));
    }

    template <typename T, typename U, typename V, typename W>
    unique_ptr<T> make_unique(U&& arg, V&& arg1, W&& arg2)
    {
        return std::unique_ptr<T>(new T(std::forward<U>(arg), std::forward<V>(arg1), std::forward<W>(arg2)));
    }

    template <typename T, typename U, typename V, typename W, typename X>
    unique_ptr<T> make_unique(U&& arg, V&& arg1, W&& arg2, X&& arg3)
    {
        return std::unique_ptr<T>(new T(std::forward<U>(arg), std::forward<V>(arg1), std::forward<W>(arg2), std::forward<X>(arg3)));
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////

#endif // defined(DURANGO) || defined(ORBIS)
}

#endif//CRYINCLUDE_CRYCOMMON_CONSOLE_STD_H
