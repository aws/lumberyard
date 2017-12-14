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
#ifndef AZSTD_UTILS_H
#define AZSTD_UTILS_H 1

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/intrinsics.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_const.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/underlying_type.h>
#include <AzCore/std/typetraits/add_reference.h>

#ifdef AZ_HAS_RVALUE_REFS
#   include <AzCore/std/typetraits/decay.h>
#   include <AzCore/std/typetraits/remove_reference.h>
#   include <AzCore/std/typetraits/is_convertible.h>
#   include <AzCore/std/typetraits/is_lvalue_reference.h>
#endif // AZ_HAS_RVALUE_REFS
#ifndef AZSTD_HAS_TYPE_TRAITS_INTRINSICS
    #include <AzCore/std/typetraits/has_trivial_constructor.h>
    #include <AzCore/std/typetraits/has_trivial_destructor.h>
    #include <AzCore/std/typetraits/has_trivial_copy.h>
    #include <AzCore/std/typetraits/has_trivial_assign.h>
#endif

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    // rvalue
#ifdef AZ_HAS_RVALUE_REFS
    // rvalue move
    template<class T>
    typename AZStd::remove_reference<T>::type && move(T && t)
    {
        return static_cast<typename AZStd::remove_reference<T>::type &&>(t);
    }

    // rvalue forward
    /*template<class T, class U>
    inline T&& forward(U&& u)   { return static_cast<T&&>(u); }*/
    template<class T, class U>
    inline T && forward(U && u
        , typename AZStd::Utils::enable_if_c<AZStd::is_lvalue_reference<T>::value ? AZStd::is_lvalue_reference<U>::value : true>::type * = 0
        , typename AZStd::Utils::enable_if_c<AZStd::is_convertible<typename AZStd::remove_reference<U>::type*, typename AZStd::remove_reference<T>::type*>::value>::type * = 0)
    {
        return static_cast<T &&>(u);
    }
#else
    template<class T>
    inline const T& move(const T& t)
    {
        return t;
    }
    template<class T, class U>
    inline const T& forward(const U& u)
    {
        return static_cast<const T&>(u);
    }

#endif //AZ_HAS_RVALUE_REFS
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Swap
    template<class T>
    AZ_INLINE void swap(T& a, T& b)
    {
#ifdef AZ_HAS_RVALUE_REFS
        T tmp = AZStd::move(a);
        a = AZStd::move(b);
        b = AZStd::move(tmp);
#else
        T tmp = a;
        a = b;
        b = tmp;
#endif
    }

    template<class ForewardIterator1, class ForewardIterator2>
    AZ_FORCE_INLINE void                    iter_swap(ForewardIterator1 a, ForewardIterator2 b)
    {
        AZStd::swap(*a, *b);
    }

    template<class ForewardIterator1, class ForewardIterator2>
    AZ_FORCE_INLINE ForewardIterator2       swap_ranges(ForewardIterator1 first1, ForewardIterator1 last1, ForewardIterator2 first2)
    {
        for (; first1 != last1; ++first1, ++first2)
        {
            AZStd::iter_swap(first1, first2);
        }

        return first2;
    }
    //////////////////////////////////////////////////////////////////////////

    // Pairs (this should be obsolete, replace fully with tuples)
    namespace Internal
    {
        template<class L1, class L2, class R1, class R2>
        struct are_pair_args_comparable
        {
            static const bool value =
                is_same
                <
                    typename AZStd::remove_reference<typename AZStd::remove_const<L1>::type>::type,
                    typename AZStd::remove_reference<typename AZStd::remove_const<R1>::type>::type
                >::value &&
                is_same
                <
                    typename AZStd::remove_reference<typename AZStd::remove_const<L2>::type>::type,
                    typename AZStd::remove_reference<typename AZStd::remove_const<R2>::type>::type
                >::value;
        };
    }

    // The structure that encapsulates index lists
    template <size_t... Is>
    struct index_sequence
    {
    };

    // Collects internal details for generating index ranges [MIN, MAX)
    namespace Internal
    {
        // Declare primary template for index range builder
        template <size_t MIN, size_t N, size_t... Is>
        struct range_builder;

        // Base step
        template <size_t MIN, size_t... Is>
        struct range_builder<MIN, MIN, Is...>
        {
            typedef index_sequence<Is...> type;
        };

        // Induction step
        template <size_t MIN, size_t N, size_t... Is>
        struct range_builder : public range_builder<MIN, N - 1, N - 1, Is...>
        {
        };
    }

    // Create index range [0,N]
    template<size_t N>
    using make_index_sequence = typename Internal::range_builder<0, N>::type;

    struct piecewise_construct_t {};
    static const piecewise_construct_t piece_construct{};

    template<class T1, class T2>
    struct pair
    {   // store a pair of values
        typedef pair<T1, T2>    this_type;
        typedef T1              first_type;
        typedef T2              second_type;

        /// Construct from defaults
        AZ_FORCE_INLINE pair()
            : first(T1())
            , second(T2()) {}
        /// Constructs only the first element, default the second.
        AZ_FORCE_INLINE pair(const T1& value1)
            : first(value1)
            , second(T2()) {}
        /// Construct from specified values.
        AZ_FORCE_INLINE pair(const T1& value1, const T2& value2)
            : first(value1)
            , second(value2) {}
        /// Copy constructor
        pair(const this_type& rhs)
            : first(rhs.first)
            , second(rhs.second) {}
        // construct from compatible pair
        template<class Other1, class Other2>
        AZ_FORCE_INLINE pair(const pair<Other1, Other2>& rhs)
            : first(rhs.first)
            , second(rhs.second) {}

#ifdef AZ_HAS_RVALUE_REFS
        typedef typename AZStd::remove_reference<T1>::type TT1;
        typedef typename AZStd::remove_reference<T2>::type TT2;

        pair(TT1&& value1, TT2&& value2)
            : first(AZStd::move(value1))
            , second(AZStd::move(value2)) {}
        pair(const TT1& value1, TT2&& value2)
            : first(value1)
            , second(AZStd::move(value2)) {}
        pair(TT1&& value1, const TT2& value2)
            : first(AZStd::move(value1))
            , second(value2) {}
        template<class Other1, class Other2>
        pair(Other1&& value1, Other2&& value2)
            : first(AZStd::forward<Other1>(value1))
            , second(AZStd::forward<Other2>(value2)) {}
        pair(pair&& rhs)
            : first(AZStd::move(rhs.first))
            , second(AZStd::move(rhs.second)) {}
        template<class Other1, class Other2>
        pair(pair<Other1, Other2>&& rhs)
            : first(AZStd::forward<Other1>(rhs.first))
            , second(AZStd::forward<Other2>(rhs.second)) {}

        template<template<class...> class TupleType, class... Args1, class... Args2>
        pair(piecewise_construct_t,
            TupleType<Args1...> first_args,
            TupleType<Args2...> second_args);

        template<template<class...> class TupleType, class... Args1, class... Args2, size_t... I1, size_t... I2>
        pair(piecewise_construct_t, TupleType<Args1...>& first_args,
            TupleType<Args2...>& second_args, AZStd::index_sequence<I1...>,
            AZStd::index_sequence<I2...>);

        this_type& operator=(this_type&& rhs)
        {
            first = AZStd::move(rhs.first);
            second = AZStd::move(rhs.second);
            return *this;
        }

        template<class Other1, class Other2>
        this_type& operator=(const pair<Other1, Other2>&& rhs)
        {
            first = AZStd::move(rhs.first);
            second = AZStd::move(rhs.second);
            return *this;
        }

        void swap(this_type&& rhs)
        {
            if (this != &rhs)
            {
                first = AZStd::move(rhs.first);
                second = AZStd::move(rhs.second);
            }
        }
#endif // AZ_HAS_RVALUE_REFS

        AZ_FORCE_INLINE void swap(this_type& rhs)
        {
            // exchange contents with _Right
            if (this != &rhs)
            {   // different, worth swapping
                AZStd::swap(first, rhs.first);
                AZStd::swap(second, rhs.second);
            }
        }

        this_type& operator=(const this_type& rhs)
        {
            first = rhs.first;
            second = rhs.second;
            return *this;
        }

        template<class Other1, class Other2>
        this_type& operator=(const pair<Other1, Other2>& rhs)
        {
            first = rhs.first;
            second = rhs.second;
            return *this;
        }

        T1 first;   // the first stored value
        T2 second;  // the second stored value
    };

    // pair
#ifdef AZ_HAS_RVALUE_REFS
    template<class T1, class T2>
    inline
    void swap(pair<T1, T2>& left, pair<T1, T2>&& right)
    {
        typedef pair<T1, T2> pair_type;
        left.swap(AZStd::forward<pair_type>(right));
    }

    template<class T1, class T2>
    inline
    void swap(pair<T1, T2>&& left, pair<T1, T2>& right)
    {
        typedef pair<T1, T2> pair_type;
        right.swap(AZStd::forward<pair_type>(left));
    }
#endif // #ifdef AZ_HAS_RVALUE_REFS

    template<class T1, class T2>
    AZ_FORCE_INLINE void swap(pair<T1, T2>& left, pair<T1, T2>& _Right)
    {   // swap _Left and _Right pairs
        left.swap(_Right);
    }

    template<class L1, class L2, class R1, class R2, class = typename enable_if<Internal::are_pair_args_comparable<L1, L2, R1, R2>::value>::type>
    AZ_FORCE_INLINE bool operator==(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test for pair equality
        return (left.first == right.first && left.second == right.second);
    }

    template<class L1, class L2, class R1, class R2, class = typename enable_if<Internal::are_pair_args_comparable<L1, L2, R1, R2>::value>::type>
    AZ_FORCE_INLINE bool operator!=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test for pair inequality
        return !(left == right);
    }

    template<class L1, class L2, class R1, class R2, class = typename enable_if<Internal::are_pair_args_comparable<L1, L2, R1, R2>::value>::type>
    AZ_FORCE_INLINE bool operator<(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test if left < right for pairs
        return (left.first < right.first || (!(right.first < left.first) && left.second < right.second));
    }

    template<class L1, class L2, class R1, class R2, class = typename enable_if<Internal::are_pair_args_comparable<L1, L2, R1, R2>::value>::type>
    AZ_FORCE_INLINE bool operator>(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test if left > right for pairs
        return right < left;
    }

    template<class L1, class L2, class R1, class R2, class = typename enable_if<Internal::are_pair_args_comparable<L1, L2, R1, R2>::value>::type>
    AZ_FORCE_INLINE bool operator<=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test if left <= right for pairs
        return !(right < left);
    }

    template<class L1, class L2, class R1, class R2, class = typename enable_if<Internal::are_pair_args_comparable<L1, L2, R1, R2>::value>::type>
    AZ_FORCE_INLINE bool operator>=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test if left >= right for pairs
        return !(left < right);
    }

#ifndef AZSTD_HAS_TYPE_TRAITS_INTRINSICS
    // Without compiler help we should help a little.
    // the pair class doesn't have a dtor, so if the paired objects don't have one we don't need to call it.
    template< class T1, class T2>
    struct has_trivial_destructor< pair<T1, T2> >
        : public ::AZStd::integral_constant<bool, ::AZStd::type_traits::ice_and<has_trivial_destructor<T1>::value, has_trivial_destructor<T2>::value>::value> {};
    template< class T1, class T2>
    struct has_trivial_constructor< pair<T1, T2> >
        : public ::AZStd::integral_constant<bool, ::AZStd::type_traits::ice_and<has_trivial_constructor<T1>::value, has_trivial_constructor<T2>::value>::value> {};
    template< class T1, class T2>
    struct has_trivial_copy< pair<T1, T2> >
        : public ::AZStd::integral_constant<bool, ::AZStd::type_traits::ice_and<has_trivial_copy<T1>::value, has_trivial_copy<T2>::value>::value> {};
    template< class T1, class T2>
    struct has_trivial_assign< pair<T1, T2> >
        : public ::AZStd::integral_constant<bool, ::AZStd::type_traits::ice_and<has_trivial_assign<T1>::value, has_trivial_assign<T2>::value>::value> {};
#endif

    //////////////////////////////////////////////////////////////////////////
    // Address of
    //////////////////////////////////////////////////////////////////////////
    namespace Internal
    {
        template<class T>
        struct addr_impl_ref
        {
            T& m_v;
            AZ_FORCE_INLINE addr_impl_ref(T& v)
                : m_v(v) {}
            addr_impl_ref& operator=(const addr_impl_ref& v) { m_v = v; }
            AZ_FORCE_INLINE operator T& () const { return m_v; }
        };

        template<class T>
        struct addressof_impl
        {
            static AZ_FORCE_INLINE T* f(T& v, long) { return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(v))); }
            static AZ_FORCE_INLINE T* f(T* v, int)  { return v; }
        };
    }

    template<class T>
    T* addressof(T& v)
    {
        return Internal::addressof_impl<T>::f(Internal::addr_impl_ref<T>(v), 0);
    }
    // End addressof
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Ref wrapper
    template<class T>
    class reference_wrapper
    {
    public:
        typedef T type;
        explicit reference_wrapper(T& t)
            : m_t(AZStd::addressof(t)) {}
        operator T& () const {
            return *m_t;
        }
        T& get() const { return *m_t; }
        T* get_pointer() const { return m_t; }
    private:
        T* m_t;
    };

    template<class T>
    AZ_FORCE_INLINE reference_wrapper<T> const ref(T& t)
    {
        return reference_wrapper<T>(t);
    }

    template<class T>
    AZ_FORCE_INLINE reference_wrapper<T const> const cref(T const& t)
    {
        return reference_wrapper<T const>(t);
    }

    template<typename T>
    class is_reference_wrapper
        : public AZStd::false_type
    {
    };

#ifdef AZ_HAS_RVALUE_REFS
    template<class T>
    struct unwrap_reference
    {
        typedef typename AZStd::decay<T>::type type;
    };
#else
    template<typename T>
    struct unwrap_reference
    {
        typedef T type;
    };
#endif

#define AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(X) \
    template<typename T>                           \
    struct is_reference_wrapper< X >               \
        : public AZStd::true_type                  \
    {                                              \
    };                                             \
                                                   \
    template<typename T>                           \
    struct unwrap_reference< X >                   \
    {                                              \
        typedef T type;                            \
    };                                             \

    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T>)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> const)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> volatile)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> const volatile)
#undef AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF

    template<class T>
    AZ_FORCE_INLINE T* get_pointer(reference_wrapper<T> const& r)
    {
        return r.get_pointer();
    }
    // end reference_wrapper
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // get_pointer
    // get_pointer(p) extracts a ->* capable pointer from p
    template<class T>
    T* get_pointer(T* p)    { return p; }
    //template<class T> T * get_pointer(std::auto_ptr<T> const& p)
    //{
    //  return p.get();
    //}
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // make_pair
#ifdef AZ_HAS_RVALUE_REFS
    template<class T1, class T2>
    inline pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type>
    make_pair(T1&& value1, T2&& value2)
    {
        typedef pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type> pair_type;
        return pair_type(AZStd::forward<T1>(value1), AZStd::forward<T2>(value2));
    }

    template<class T1, class T2>
    inline pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type>
    make_pair(const T1& value1, T2&& value2)
    {
        typedef pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type> pair_type;
        return pair_type((typename AZStd::unwrap_reference<T1>::type)value1, AZStd::forward<T2>(value2));
    }

    template<class T1, class T2>
    inline pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type>
    make_pair(T1&& value1, const T2& value2)
    {
        typedef pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type> pair_type;
        return pair_type(AZStd::forward<T1>(value1), (typename AZStd::unwrap_reference<T2>::type)value2);
    }
    template<class T1, class T2>
    inline pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type>
    make_pair(const T1& value1, const T2& value2)
    {
        typedef pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type> pair_type;
        return pair_type((typename AZStd::unwrap_reference<T1>::type)value1, (typename AZStd::unwrap_reference<T2>::type)value2);
    }
#else
    template<class T1, class T2>
    inline pair<T1, T2> make_pair(const T1& value1, const T2& value2)
    {
        return pair<T1, T2>(value1, value2);
    }
#endif // AZ_HAS_RVALUE_REFS
       //////////////////////////////////////////////////////////////////////////

    template<class T, bool isEnum = AZStd::is_enum<T>::value>
    struct RemoveEnum
    {
        typedef typename AZStd::underlying_type<T>::type type;
    };

    template<class T>
    struct RemoveEnum<T, false>
    {
        typedef T type;
    };

    template<class T>
    using RemoveEnumT = typename RemoveEnum<T>::type;

    template<class T>
    struct HandleLambdaPointer;

    template<class R, class L, class... Args>
    struct HandleLambdaPointer<R(L::*)(Args...)>
    {
        typedef R(type)(Args...);
    };

    template<class R, class L, class... Args>
    struct HandleLambdaPointer<R(L::*)(Args...) const>
    {
        typedef R(type)(Args...);
    };

    template<class T>
    struct RemoveFunctionConst
    {
        typedef typename HandleLambdaPointer<decltype(&T::operator())>::type type; // lambda matching handling
    };

    template<class R, class... Args>
    struct RemoveFunctionConst<R(Args...)>
    {
        typedef R(type)(Args...);
    };

    template<class R, class C, class... Args>
    struct RemoveFunctionConst<R(C::*)(Args...)>
    {
        using type = R(C::*)(Args...);
    };

    template<class R, class C, class... Args>
    struct RemoveFunctionConst<R(C::*)(Args...) const>
    {
        using type = R(C::*)(Args...);
    };

    template <class T>
    typename AZStd::add_rvalue_reference<T>::type declval();

    template <template <class> class, typename...>
    struct sequence_and;

    template <template <class> class trait, typename T1, typename... Ts>
    struct sequence_and<trait, T1, Ts...>
    {
        static const bool value = trait<T1>::value && sequence_and<trait, Ts...>::value;
    };

    template <template <class> class trait>
    struct sequence_and<trait>
    {
        static const bool value = true;
    };

    template <template <class> class, typename...>
    struct sequence_or;

    template <template <class> class trait, typename T1, typename... Ts>
    struct sequence_or<trait, T1, Ts...>
    {
        static const bool value = trait<T1>::value || sequence_or<trait, Ts...>::value;
    };

    template <template <class> class trait>
    struct sequence_or<trait>
    {
        static const bool value = false;
    };
}

#endif // AZSTD_UTILS_H
#pragma once