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
Internal::AZSTD_MEM_FN_NAME(mf0) < R, T > mem_fn(R (AZSTD_MEM_FN_CC T::* f) ())
{
    return Internal::AZSTD_MEM_FN_NAME(mf0) < R, T > (f);
}

template<class R, class T>
Internal::AZSTD_MEM_FN_NAME(cmf0) < R, T > mem_fn(R (AZSTD_MEM_FN_CC T::* f) () const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf0) < R, T > (f);
}

template<class R, class T, class A1>
Internal::AZSTD_MEM_FN_NAME(mf1) < R, T, A1 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1))
{
    return Internal::AZSTD_MEM_FN_NAME(mf1) < R, T, A1 > (f);
}

template<class R, class T, class A1>
Internal::AZSTD_MEM_FN_NAME(cmf1) < R, T, A1 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1) const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf1) < R, T, A1 > (f);
}

template<class R, class T, class A1, class A2>
Internal::AZSTD_MEM_FN_NAME(mf2) < R, T, A1, A2 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2))
{
    return Internal::AZSTD_MEM_FN_NAME(mf2) < R, T, A1, A2 > (f);
}

template<class R, class T, class A1, class A2>
Internal::AZSTD_MEM_FN_NAME(cmf2) < R, T, A1, A2 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2) const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf2) < R, T, A1, A2 > (f);
}

template<class R, class T, class A1, class A2, class A3>
Internal::AZSTD_MEM_FN_NAME(mf3) < R, T, A1, A2, A3 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3))
{
    return Internal::AZSTD_MEM_FN_NAME(mf3) < R, T, A1, A2, A3 > (f);
}

template<class R, class T, class A1, class A2, class A3>
Internal::AZSTD_MEM_FN_NAME(cmf3) < R, T, A1, A2, A3 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3) const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf3) < R, T, A1, A2, A3 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4>
Internal::AZSTD_MEM_FN_NAME(mf4) < R, T, A1, A2, A3, A4 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4))
{
    return Internal::AZSTD_MEM_FN_NAME(mf4) < R, T, A1, A2, A3, A4 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4>
Internal::AZSTD_MEM_FN_NAME(cmf4) < R, T, A1, A2, A3, A4 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4) const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf4) < R, T, A1, A2, A3, A4 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4, class A5>
Internal::AZSTD_MEM_FN_NAME(mf5) < R, T, A1, A2, A3, A4, A5 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4, A5))
{
    return Internal::AZSTD_MEM_FN_NAME(mf5) < R, T, A1, A2, A3, A4, A5 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4, class A5>
Internal::AZSTD_MEM_FN_NAME(cmf5) < R, T, A1, A2, A3, A4, A5 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4, A5) const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf5) < R, T, A1, A2, A3, A4, A5 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6>
Internal::AZSTD_MEM_FN_NAME(mf6) < R, T, A1, A2, A3, A4, A5, A6 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4, A5, A6))
{
    return Internal::AZSTD_MEM_FN_NAME(mf6) < R, T, A1, A2, A3, A4, A5, A6 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6>
Internal::AZSTD_MEM_FN_NAME(cmf6) < R, T, A1, A2, A3, A4, A5, A6 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4, A5, A6) const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf6) < R, T, A1, A2, A3, A4, A5, A6 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
Internal::AZSTD_MEM_FN_NAME(mf7) < R, T, A1, A2, A3, A4, A5, A6, A7 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4, A5, A6, A7))
{
    return Internal::AZSTD_MEM_FN_NAME(mf7) < R, T, A1, A2, A3, A4, A5, A6, A7 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
Internal::AZSTD_MEM_FN_NAME(cmf7) < R, T, A1, A2, A3, A4, A5, A6, A7 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4, A5, A6, A7) const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf7) < R, T, A1, A2, A3, A4, A5, A6, A7 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
Internal::AZSTD_MEM_FN_NAME(mf8) < R, T, A1, A2, A3, A4, A5, A6, A7, A8 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4, A5, A6, A7, A8))
{
    return Internal::AZSTD_MEM_FN_NAME(mf8) < R, T, A1, A2, A3, A4, A5, A6, A7, A8 > (f);
}

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
Internal::AZSTD_MEM_FN_NAME(cmf8) < R, T, A1, A2, A3, A4, A5, A6, A7, A8 > mem_fn(R (AZSTD_MEM_FN_CC T::* f) (A1, A2, A3, A4, A5, A6, A7, A8) const)
{
    return Internal::AZSTD_MEM_FN_NAME(cmf8) < R, T, A1, A2, A3, A4, A5, A6, A7, A8 > (f);
}
