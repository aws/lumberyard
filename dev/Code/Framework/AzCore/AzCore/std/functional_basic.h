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
#ifndef AZSTD_FUNCTIONAL_BASIC_H
#define AZSTD_FUNCTIONAL_BASIC_H 1

#include <AzCore/std/base.h>

namespace AZStd
{
    // Functors as required by the standard.

    // 20.3.1, base:
    template <class Arg, class Result>
    struct unary_function
    {
        typedef Arg     argument_type;
        typedef Result  result_type;
    };
    template <class Arg1, class Arg2, class Result>
    struct binary_function
    {
        typedef Arg1    first_argument_type;
        typedef Arg2    second_argument_type;
        typedef Result  result_type;
    };

    // 20.3.2, arithmetic operations:
    template <class T>
    struct plus
        : public binary_function<T, T, T>
    {
        AZ_FORCE_INLINE T operator()(const T& left, const T& right) const { return left + right; }
    };
    template <class T>
    struct minus
        : public binary_function<T, T, T>
    {
        AZ_FORCE_INLINE T operator()(const T& left, const T& right) const { return left - right; }
    };
    template <class T>
    struct multiplies
        : public binary_function<T, T, T>
    {
        AZ_FORCE_INLINE T operator()(const T& left, const T& right) const { return left * right; }
    };
    template <class T>
    struct divides
        : public binary_function<T, T, T>
    {
        AZ_FORCE_INLINE T operator()(const T& left, const T& right) const { return left / right; }
    };
    template <class T>
    struct modulus
        : public binary_function<T, T, T>
    {
        AZ_FORCE_INLINE T operator()(const T& left, const T& right) const { return left % right; }
    };
    template <class T>
    struct negate
        : public unary_function<T, T>
    {
        AZ_FORCE_INLINE T operator()(const T& left) const { return -left; }
    };
    // 20.3.3, comparisons:
    template <class T>
    struct equal_to
        : public binary_function<T, T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left, const T& right) const { return left == right; }
    };
    template <class T>
    struct not_equal_to
        : public binary_function<T, T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left, const T& right) const { return left != right; }
    };
    template <class T>
    struct greater
        : public binary_function<T, T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left, const T& right) const { return left > right; }
    };
    template <class T>
    struct less
        : public binary_function<T, T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left, const T& right) const { return left < right; }
    };
    template <class T>
    struct greater_equal
        : public binary_function<T, T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left, const T& right) const { return left >= right; }
    };
    template <class T>
    struct less_equal
        : public binary_function<T, T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left, const T& right) const { return left <= right; }
    };

    // 20.3.4, logical operations:
    template <class T>
    struct logical_and
        : public binary_function<T, T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left, const T& right) const { return left && right; }
    };
    template <class T>
    struct logical_or
        : public binary_function<T, T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left, const T& right) const { return left || right; }
    };
    template <class T>
    struct logical_not
        : public unary_function<T, bool>
    {
        AZ_FORCE_INLINE bool operator()(const T& left) const { return !left; }
    };
    // 20.3.5, negators:
    template <class Functor>
    class unary_negate
        : public unary_function<typename Functor::argument_type, bool>
    {
    public:
        unary_negate() = default;
        AZ_FORCE_INLINE explicit unary_negate(const Functor& funct)
            : m_functor(funct) {}
        AZ_FORCE_INLINE bool operator()(const typename Functor::argument_type& left) const { return !m_functor(left); }
    protected:
        Functor m_functor;
    };
    template <class Functor>
    AZ_FORCE_INLINE unary_negate<Functor> not1(const Functor& funct) { return AZStd::unary_negate<Functor>(funct); }
    template <class Functor>
    struct binary_negate
        : public binary_function<typename Functor::first_argument_type, typename Functor::second_argument_type, bool>
    {
    public:
        binary_negate() = default;
        AZ_FORCE_INLINE explicit binary_negate(const Functor& funct)
            : m_functor(funct) {}
        AZ_FORCE_INLINE bool operator()(const typename Functor::first_argument_type& left, const typename Functor::second_argument_type& right) const { return !m_functor(left, right); }
    protected:
        Functor m_functor;
    };
    template <class Functor>
    AZ_FORCE_INLINE binary_negate<Functor> not2(const Functor& func)    { return AZStd::binary_negate<Functor>(func); }
}

#endif // AZSTD_FUNCTIONAL_BASIC_H
#pragma once