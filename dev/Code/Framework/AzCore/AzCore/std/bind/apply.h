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
#ifndef AZSTD_BIND_APPLY_HPP_INCLUDED
#define AZSTD_BIND_APPLY_HPP_INCLUDED

// Based on boost 1.39.0

namespace AZStd
{
    template<class R>
    struct apply
    {
        typedef R result_type;

        template<class F>
        result_type operator()(F& f) const
        {
            return f();
        }

        template<class F, class A1>
        result_type operator()(F& f, A1& a1) const
        {
            return f(a1);
        }

        template<class F, class A1, class A2>
        result_type operator()(F& f, A1& a1, A2& a2) const
        {
            return f(a1, a2);
        }

        template<class F, class A1, class A2, class A3>
        result_type operator()(F& f, A1& a1, A2& a2, A3& a3) const
        {
            return f(a1, a2, a3);
        }

        template<class F, class A1, class A2, class A3, class A4>
        result_type operator()(F& f, A1& a1, A2& a2, A3& a3, A4& a4) const
        {
            return f(a1, a2, a3, a4);
        }

        template<class F, class A1, class A2, class A3, class A4, class A5>
        result_type operator()(F& f, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5) const
        {
            return f(a1, a2, a3, a4, a5);
        }

        template<class F, class A1, class A2, class A3, class A4, class A5, class A6>
        result_type operator()(F& f, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5, A6& a6) const
        {
            return f(a1, a2, a3, a4, a5, a6);
        }

        template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
        result_type operator()(F& f, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5, A6& a6, A7& a7) const
        {
            return f(a1, a2, a3, a4, a5, a6, a7);
        }

        template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
        result_type operator()(F& f, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5, A6& a6, A7& a7, A8& a8) const
        {
            return f(a1, a2, a3, a4, a5, a6, a7, a8);
        }

        template<class F, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
        result_type operator()(F& f, A1& a1, A2& a2, A3& a3, A4& a4, A5& a5, A6& a6, A7& a7, A8& a8, A9& a9) const
        {
            return f(a1, a2, a3, a4, a5, a6, a7, a8, a9);
        }
    };
}

#endif // #ifndef AZSTD_BIND_APPLY_HPP_INCLUDED
