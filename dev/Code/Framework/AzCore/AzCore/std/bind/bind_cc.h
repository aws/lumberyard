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

// Based on boost 1.39.0


template<class R>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (), Internal::list0>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) ())
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)();
    typedef Internal::list0 list_type;
    return Internal::bind_t<R, F, list_type> (f, list_type());
}

template<class R, class B1, class A1>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1), typename Internal::list_av_1<A1>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1), A1 a1)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1);
    typedef typename Internal::list_av_1<A1>::type list_type;
    return Internal::bind_t<R, F, list_type> (f, list_type(a1));
}

template<class R, class B1, class B2, class A1, class A2>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1, B2), typename Internal::list_av_2<A1, A2>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1, B2), A1 a1, A2 a2)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1, B2);
    typedef typename Internal::list_av_2<A1, A2>::type list_type;
    return Internal::bind_t<R, F, list_type> (f, list_type(a1, a2));
}

template<class R,
    class B1, class B2, class B3,
    class A1, class A2, class A3>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1, B2, B3), typename Internal::list_av_3<A1, A2, A3>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1, B2, B3), A1 a1, A2 a2, A3 a3)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1, B2, B3);
    typedef typename Internal::list_av_3<A1, A2, A3>::type list_type;
    return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3));
}

template<class R,
    class B1, class B2, class B3, class B4,
    class A1, class A2, class A3, class A4>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1, B2, B3, B4), typename Internal::list_av_4<A1, A2, A3, A4>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1, B2, B3, B4), A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1, B2, B3, B4);
    typedef typename Internal::list_av_4<A1, A2, A3, A4>::type list_type;
    return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4));
}

template<class R,
    class B1, class B2, class B3, class B4, class B5,
    class A1, class A2, class A3, class A4, class A5>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1, B2, B3, B4, B5), typename Internal::list_av_5<A1, A2, A3, A4, A5>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1, B2, B3, B4, B5), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1, B2, B3, B4, B5);
    typedef typename Internal::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5));
}

template<class R,
    class B1, class B2, class B3, class B4, class B5, class B6,
    class A1, class A2, class A3, class A4, class A5, class A6>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1, B2, B3, B4, B5, B6), typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1, B2, B3, B4, B5, B6), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1, B2, B3, B4, B5, B6);
    typedef typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6));
}

template<class R,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1, B2, B3, B4, B5, B6, B7), typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1, B2, B3, B4, B5, B6, B7), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1, B2, B3, B4, B5, B6, B7);
    typedef typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7));
}

template<class R,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1, B2, B3, B4, B5, B6, B7, B8), typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1, B2, B3, B4, B5, B6, B7, B8), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1, B2, B3, B4, B5, B6, B7, B8);
    typedef typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

template<class R,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8, class B9,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
Internal::bind_t<R, AZSTD_BIND_ST R (AZSTD_BIND_CC*) (B1, B2, B3, B4, B5, B6, B7, B8, B9), typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
AZSTD_BIND(AZSTD_BIND_ST R (AZSTD_BIND_CC* f) (B1, B2, B3, B4, B5, B6, B7, B8, B9), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef AZSTD_BIND_ST R (AZSTD_BIND_CC * F)(B1, B2, B3, B4, B5, B6, B7, B8, B9);
    typedef typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return Internal::bind_t<R, F, list_type>(f, list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}
