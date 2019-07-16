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

#pragma once

#include <AzCore/std/typetraits/decay.h>
#include <AzCore/std/typetraits/void_t.h>

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    template <class... T>
    struct common_type {};

    template<class... T>
    using common_type_t = typename common_type<T...>::type;

    // one type
    template <class T>
    struct common_type<T>
    {
        using type = AZStd::decay_t<T>;
    };

    // two types
    namespace Internal
    {
        template<class T1, class T2>
        using common_ternary_t = decltype(false ? AZStd::declval<T1>() : AZStd::declval<T2>());

        // Void parameter is for SFINAE as part for the ternary expression
        template<class T1, class T2, class = void>
        struct common_type_2_default {};

        template<class T1, class T2>
        struct common_type_2_default<T1, T2, AZStd::void_t<common_ternary_t<T1, T2>>>
        {
            using type = AZStd::decay_t<common_ternary_t<T1, T2>>;
        };

        template<class T1, class T2, class D1 = AZStd::decay_t<T1>, class D2 = AZStd::decay_t<T2>>
        struct common_type_2_impl : common_type<D1, D2> {};

        template<class D1, class D2>
        struct common_type_2_impl<D1, D2, D1, D2> : common_type_2_default<D1, D2> {};

        // Void argument is for SFINAE when the common_type_t<T1, T2> template provides no type
        template<class Void, class T1, class T2, class... R>
        struct common_type_multi_impl {};

        template<class T1, class T2, class... R>
        struct common_type_multi_impl<AZStd::void_t<AZStd::common_type_t<T1, T2>>, T1, T2, R...>
            : common_type<AZStd::common_type_t<T1, T2>, R...> {};
    }

    template <class T1, class T2>
    struct common_type<T1, T2> : Internal::common_type_2_impl<T1, T2> {};

    // three or more types
    template <class T1, class T2, class... R>
    struct common_type<T1, T2, R...> : Internal::common_type_multi_impl<void, T1, T2, R...> {};
}