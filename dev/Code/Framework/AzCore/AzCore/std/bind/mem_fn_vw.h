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

template<class R, class T>
struct AZSTD_MEM_FN_NAME(mf0)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf0) < R
    , T
    , R (AZSTD_MEM_FN_CC T::*) () >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)();
    explicit AZSTD_MEM_FN_NAME(mf0)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf0) < R
        , T
        , F > (f) {}
};

template<class R, class T>
struct AZSTD_MEM_FN_NAME(cmf0)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf0) < R
    , T
    , R (AZSTD_MEM_FN_CC T::*) () const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)() const;
    explicit AZSTD_MEM_FN_NAME(cmf0)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf0) < R
        , T
        , F > (f) {}
};


template<class R, class T, class A1>
struct AZSTD_MEM_FN_NAME(mf1)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf1) < R
    , T
    , A1
    , R (AZSTD_MEM_FN_CC T::*) (A1) >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1);
    explicit AZSTD_MEM_FN_NAME(mf1)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf1) < R
        , T
        , A1
        , F > (f) {}
};

template<class R, class T, class A1>
struct AZSTD_MEM_FN_NAME(cmf1)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf1) < R
    , T
    , A1
    , R (AZSTD_MEM_FN_CC T::*) (A1) const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1) const;
    explicit AZSTD_MEM_FN_NAME(cmf1)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf1) < R
        , T
        , A1
        , F > (f) {}
};


template<class R, class T, class A1, class A2>
struct AZSTD_MEM_FN_NAME(mf2)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf2) < R
    , T
    , A1
    , A2
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2) >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2);
    explicit AZSTD_MEM_FN_NAME(mf2)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf2) < R
        , T
        , A1
        , A2
        , F > (f) {}
};

template<class R, class T, class A1, class A2>
struct AZSTD_MEM_FN_NAME(cmf2)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf2) < R
    , T
    , A1
    , A2
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2) const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2) const;
    explicit AZSTD_MEM_FN_NAME(cmf2)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf2) < R
        , T
        , A1
        , A2
        , F > (f) {}
};


template<class R, class T, class A1, class A2, class A3>
struct AZSTD_MEM_FN_NAME(mf3)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf3) < R
    , T
    , A1
    , A2
    , A3
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3) >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3);
    explicit AZSTD_MEM_FN_NAME(mf3)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf3) < R
        , T
        , A1
        , A2
        , A3
        , F > (f) {}
};

template<class R, class T, class A1, class A2, class A3>
struct AZSTD_MEM_FN_NAME(cmf3)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf3) < R
    , T
    , A1
    , A2
    , A3
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3) const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3) const;
    explicit AZSTD_MEM_FN_NAME(cmf3)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf3) < R
        , T
        , A1
        , A2
        , A3
        , F > (f) {}
};


template<class R, class T, class A1, class A2, class A3, class A4>
struct AZSTD_MEM_FN_NAME(mf4)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf4) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4) >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4);
    explicit AZSTD_MEM_FN_NAME(mf4)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf4) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , F > (f) {}
};

template<class R, class T, class A1, class A2, class A3, class A4>
struct AZSTD_MEM_FN_NAME(cmf4)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf4) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4) const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4) const;
    explicit AZSTD_MEM_FN_NAME(cmf4)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf4) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , F > (f) {}
};


template<class R, class T, class A1, class A2, class A3, class A4, class A5>
struct AZSTD_MEM_FN_NAME(mf5)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf5) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , A5
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4, A5) >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4, A5);
    explicit AZSTD_MEM_FN_NAME(mf5)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf5) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , A5
        , F > (f) {}
};

template<class R, class T, class A1, class A2, class A3, class A4, class A5>
struct AZSTD_MEM_FN_NAME(cmf5)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf5) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , A5
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4, A5) const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4, A5) const;
    explicit AZSTD_MEM_FN_NAME(cmf5)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf5) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , A5
        , F > (f) {}
};


template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6>
struct AZSTD_MEM_FN_NAME(mf6)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf6) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , A5
    , A6
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4, A5, A6) >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4, A5, A6);
    explicit AZSTD_MEM_FN_NAME(mf6)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf6) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , A5
        , A6
        , F > (f) {}
};

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6>
struct AZSTD_MEM_FN_NAME(cmf6)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf6) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , A5
    , A6
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4, A5, A6) const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4, A5, A6) const;
    explicit AZSTD_MEM_FN_NAME(cmf6)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf6) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , A5
        , A6
        , F > (f) {}
};


template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct AZSTD_MEM_FN_NAME(mf7)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf7) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , A5
    , A6
    , A7
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4, A5, A6, A7) >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4, A5, A6, A7);
    explicit AZSTD_MEM_FN_NAME(mf7)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf7) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , A5
        , A6
        , A7
        , F > (f) {}
};

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
struct AZSTD_MEM_FN_NAME(cmf7)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf7) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , A5
    , A6
    , A7
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4, A5, A6, A7) const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4, A5, A6, A7) const;
    explicit AZSTD_MEM_FN_NAME(cmf7)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf7) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , A5
        , A6
        , A7
        , F > (f) {}
};


template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct AZSTD_MEM_FN_NAME(mf8)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf8) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , A5
    , A6
    , A7
    , A8
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4, A5, A6, A7, A8) >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4, A5, A6, A7, A8);
    explicit AZSTD_MEM_FN_NAME(mf8)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(mf8) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , A5
        , A6
        , A7
        , A8
        , F > (f) {}
};

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
struct AZSTD_MEM_FN_NAME(cmf8)
    : public mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf8) < R
    , T
    , A1
    , A2
    , A3
    , A4
    , A5
    , A6
    , A7
    , A8
    , R (AZSTD_MEM_FN_CC T::*) (A1, A2, A3, A4, A5, A6, A7, A8) const >
{
    typedef R (AZSTD_MEM_FN_CC T::* F)(A1, A2, A3, A4, A5, A6, A7, A8) const;
    explicit AZSTD_MEM_FN_NAME(cmf8)(F f)
        : mf<R>::AZSTD_NESTED_TEMPLATE AZSTD_MEM_FN_NAME2(cmf8) < R
        , T
        , A1
        , A2
        , A3
        , A4
        , A5
        , A6
        , A7
        , A8
        , F > (f) {}
};
