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

// 0

template<class Rt2, class R, class T,
    class A1>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf0) < R, T >, typename Internal::list_av_1<A1>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (), A1 a1)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf0) < R, T > F;
    typedef typename Internal::list_av_1<A1>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1));
}

template<class Rt2, class R, class T,
    class A1>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf0) < R, T >, typename Internal::list_av_1<A1>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) () const, A1 a1)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf0) < R, T > F;
    typedef typename Internal::list_av_1<A1>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1));
}

// 1

template<class Rt2, class R, class T,
    class B1,
    class A1, class A2>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf1) < R, T, B1 >, typename Internal::list_av_2<A1, A2>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1), A1 a1, A2 a2)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf1) < R, T, B1 > F;
    typedef typename Internal::list_av_2<A1, A2>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2));
}

template<class Rt2, class R, class T,
    class B1,
    class A1, class A2>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf1) < R, T, B1 >, typename Internal::list_av_2<A1, A2>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1) const, A1 a1, A2 a2)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf1) < R, T, B1 > F;
    typedef typename Internal::list_av_2<A1, A2>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2));
}

// 2

template<class Rt2, class R, class T,
    class B1, class B2,
    class A1, class A2, class A3>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf2) < R, T, B1, B2 >, typename Internal::list_av_3<A1, A2, A3>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2), A1 a1, A2 a2, A3 a3)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf2) < R, T, B1, B2 > F;
    typedef typename Internal::list_av_3<A1, A2, A3>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3));
}

template<class Rt2, class R, class T,
    class B1, class B2,
    class A1, class A2, class A3>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf2) < R, T, B1, B2 >, typename Internal::list_av_3<A1, A2, A3>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2) const, A1 a1, A2 a2, A3 a3)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf2) < R, T, B1, B2 > F;
    typedef typename Internal::list_av_3<A1, A2, A3>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3));
}

// 3

template<class Rt2, class R, class T,
    class B1, class B2, class B3,
    class A1, class A2, class A3, class A4>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf3) < R, T, B1, B2, B3 >, typename Internal::list_av_4<A1, A2, A3, A4>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3), A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf3) < R, T, B1, B2, B3 > F;
    typedef typename Internal::list_av_4<A1, A2, A3, A4>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3,
    class A1, class A2, class A3, class A4>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf3) < R, T, B1, B2, B3 >, typename Internal::list_av_4<A1, A2, A3, A4>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3) const, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf3) < R, T, B1, B2, B3 > F;
    typedef typename Internal::list_av_4<A1, A2, A3, A4>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4));
}

// 4

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4,
    class A1, class A2, class A3, class A4, class A5>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf4) < R, T, B1, B2, B3, B4 >, typename Internal::list_av_5<A1, A2, A3, A4, A5>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf4) < R, T, B1, B2, B3, B4 > F;
    typedef typename Internal::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4,
    class A1, class A2, class A3, class A4, class A5>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf4) < R, T, B1, B2, B3, B4 >, typename Internal::list_av_5<A1, A2, A3, A4, A5>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf4) < R, T, B1, B2, B3, B4 > F;
    typedef typename Internal::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5));
}

// 5

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5,
    class A1, class A2, class A3, class A4, class A5, class A6>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf5) < R, T, B1, B2, B3, B4, B5 >, typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4, B5), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf5) < R, T, B1, B2, B3, B4, B5 > F;
    typedef typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5,
    class A1, class A2, class A3, class A4, class A5, class A6>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf5) < R, T, B1, B2, B3, B4, B5 >, typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4, B5) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf5) < R, T, B1, B2, B3, B4, B5 > F;
    typedef typename Internal::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6));
}

// 6

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf6) < R, T, B1, B2, B3, B4, B5, B6 >, typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4, B5, B6), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf6) < R, T, B1, B2, B3, B4, B5, B6 > F;
    typedef typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf6) < R, T, B1, B2, B3, B4, B5, B6 >, typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4, B5, B6) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf6) < R, T, B1, B2, B3, B4, B5, B6 > F;
    typedef typename Internal::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7));
}

// 7

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf7) < R, T, B1, B2, B3, B4, B5, B6, B7 >, typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4, B5, B6, B7), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf7) < R, T, B1, B2, B3, B4, B5, B6, B7 > F;
    typedef typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf7) < R, T, B1, B2, B3, B4, B5, B6, B7 >, typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4, B5, B6, B7) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf7) < R, T, B1, B2, B3, B4, B5, B6, B7 > F;
    typedef typename Internal::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

// 8

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(mf8) < R, T, B1, B2, B3, B4, B5, B6, B7, B8 >, typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4, B5, B6, B7, B8), A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef Internal::AZSTD_BIND_MF_NAME (mf8) < R, T, B1, B2, B3, B4, B5, B6, B7, B8 > F;
    typedef typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
Internal::bind_t<Rt2, Internal::AZSTD_BIND_MF_NAME(cmf8) < R, T, B1, B2, B3, B4, B5, B6, B7, B8 >, typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
AZSTD_BIND(AZStd::type<Rt2>, R (AZSTD_BIND_MF_CC T::* f) (B1, B2, B3, B4, B5, B6, B7, B8) const, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef Internal::AZSTD_BIND_MF_NAME (cmf8) < R, T, B1, B2, B3, B4, B5, B6, B7, B8 > F;
    typedef typename Internal::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return Internal::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}
