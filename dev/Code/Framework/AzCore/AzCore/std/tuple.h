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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/void_t.h>
#include <tuple>

namespace AZStd
{
    template<class... Types>
    using tuple = std::tuple<Types...>;

    template<class T>
    using tuple_size = std::tuple_size<T>;

    template<size_t I, class T>
    using tuple_element = std::tuple_element<I, T>;

    template<size_t I, class T>
    using tuple_element_t = typename std::tuple_element<I, T>::type;

    // Placeholder structure that can be assigned any value with no effect.
    // This is used by AZStd::tie as placeholder for unused arugments
    using ignore_t = AZStd::decay_t<decltype(std::ignore)>;
    decltype(std::ignore) ignore = std::ignore;

    using std::make_tuple;
    using std::tie;
    using std::forward_as_tuple;
    using std::tuple_cat;
    using std::get;

#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
    /* The below template class helpers are used to implement the C++14 functionality for std::get function which accepts a type parameter
    instead of an index in VS2013.
    Section 23.5.3.7 [tuple.elem] of the ISO N4687 draft of the C++ standard added 4 implementations of the get method that allowed access of a
    tuple element by specifying it's type. VS2013 does not have this support and therefore it is implemented here.

    template <class T, class... Types>
    constexpr T& get(tuple<Types...>& t) noexcept;
    template <class T, class... Types>
    constexpr T&& get(tuple<Types...>&& t) noexcept;
    template <class T, class... Types>
    constexpr const T& get(const tuple<Types...>& t) noexcept;
    template <class T, class... Types>
    constexpr const T&& get(const tuple<Types...>&& t) noexcept;
    */
    namespace Internal
    {
        template <typename Type, size_t ElementIndex, typename MatchIndices, typename ElementTypes>
        struct FindTypeInTupleLikeTypeMatchHelper;

        template <typename Type, size_t ElementIndex, size_t... MatchIndices>
        struct FindTypeInTupleLikeTypeMatchHelper<Type, ElementIndex, AZStd::index_sequence<MatchIndices...>, AZStd::make_void<> >
        {
            template<size_t, size_t... LastIndices>
            struct GetLastIndex : GetLastIndex<LastIndices...>
            {};

            template<size_t Index>
            struct GetLastIndex<Index>
            {
                static const size_t value = Index;
            };
            static const size_t size = sizeof...(MatchIndices);
            static const size_t indices[];
            static const size_t last_index = GetLastIndex<MatchIndices...>::value;
        };
        template <typename Type, size_t ElementIndex, size_t... MatchIndices>
        const size_t FindTypeInTupleLikeTypeMatchHelper<Type, ElementIndex, AZStd::index_sequence<MatchIndices...>, AZStd::make_void<>>::indices[] = { MatchIndices... };

        template <typename Type, size_t ElementIndex, size_t... MatchIndices, typename ElementType, typename... ElementTypeRest>
        struct FindTypeInTupleLikeTypeMatchHelper<Type, ElementIndex, AZStd::index_sequence<MatchIndices...>, AZStd::make_void<ElementType, ElementTypeRest...> >
            : FindTypeInTupleLikeTypeMatchHelper<Type, ElementIndex + 1,
            AZStd::conditional_t<AZStd::is_same<Type, ElementType>::value, AZStd::index_sequence<MatchIndices..., ElementIndex>, AZStd::index_sequence<MatchIndices...>>,
            AZStd::make_void<ElementTypeRest...>>
        {};

        template <typename Type, typename... Types>
        struct FindTypeInTupleLikeTypeHelper : FindTypeInTupleLikeTypeMatchHelper<Type, 0U, AZStd::index_sequence<>, AZStd::make_void<Types...>>
        {};

        template <class Type>
        struct FindTypeInTupleLikeTypeHelper<Type>
        {
            static_assert(sizeof(Type) == sizeof(Type) + 1, "Type cannot exist in empty tuple");
        };

        template <typename Type, typename... Types>
        struct FindTypeInTupleLikeType : public FindTypeInTupleLikeTypeHelper<Type, Types...> {};

        template <typename Type, typename... Types>
        struct FindTypeInTupleLikeTypeUnique
        {
            using find_type_tuple_t = FindTypeInTupleLikeType<Type, Types...>;
            static const size_t size = find_type_tuple_t::size;
            static_assert(size != 0, "Type does not exist in tuple");
            static_assert(size == 1, "Multiple of the same type exist in tuple");
            static const size_t value = find_type_tuple_t::last_index;
        };
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the tuple with the specified type T
    //! If the type T does not exist or there is more than one in the tuple, then this function fails to compile
    template<typename T, typename... Types>
    T& get(AZStd::tuple<Types...>& tupleObj)
    {
        return std::get<Internal::FindTypeInTupleLikeTypeUnique<T, Types...>::value>(tupleObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the tuple with the specified type T
    //! If the type T does not exist or there is more than one in the tuple, then this function fails to compile
    template<typename T, typename... Types>
    const T& get(const AZStd::tuple<Types...>& tupleObj)
    {
        return std::get<Internal::FindTypeInTupleLikeTypeUnique<T, Types...>::value>(tupleObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the tuple with the specified type T
    //! If the type T does not exist or there is more than one in the tuple, then this function fails to compile
    template<typename T, typename... Types>
    T&& get(AZStd::tuple<Types...>&& tupleObj)
    {
        return AZStd::forward<T&&>(std::get<Internal::FindTypeInTupleLikeTypeUnique<T, Types...>::value>(tupleObj));
    }
#endif

    namespace Internal
    {
        template<class Fn, class Tuple, size_t... Is>
        auto apply_impl(Fn&& f, Tuple&& tupleObj, AZStd::index_sequence<Is...>) -> decltype(AZStd::invoke(AZStd::declval<Fn>(), AZStd::get<Is>(AZStd::declval<Tuple>())...))
        {
            (void)tupleObj;
            return AZStd::invoke(AZStd::forward<Fn>(f), AZStd::get<Is>(AZStd::forward<Tuple>(tupleObj))...);
        }
    }

    template<class Fn, class Tuple>
    auto apply(Fn&& f, Tuple&& tupleObj) 
        -> decltype(Internal::apply_impl(AZStd::declval<Fn>(), AZStd::declval<Tuple>(), AZStd::make_index_sequence<AZStd::tuple_size<AZStd::decay_t<Tuple>>::value>{}))
    {
        return Internal::apply_impl(AZStd::forward<Fn>(f), AZStd::forward<Tuple>(tupleObj), AZStd::make_index_sequence<AZStd::tuple_size<AZStd::decay_t<Tuple>>::value>{});
    }

    //! Creates an hash specialization for tuple types using the hash_combine function
    //! The std::tuple implementation does not have this support. This is an extension
    template <typename... Types>
    struct hash<AZStd::tuple<Types...>>
    {
        template<size_t... Indices>
        size_t ElementHasher(const AZStd::tuple<Types...>& value, AZStd::index_sequence<Indices...>) const
        {
            size_t seed = 0;
            int dummy[]{ 0, (hash_combine(seed, AZStd::get<Indices>(value)), 0)... };
            (void)dummy;
            return seed;
        }

        size_t operator()(const AZStd::tuple<Types...>& value) const
        {
            return ElementHasher(value, AZStd::make_index_sequence<sizeof...(Types)>{});
        }
    };

    // pair code to inter operate with tuples
    template<class T1, class T2>
    template<template<class...> class TupleType, class... Args1, class... Args2, size_t... I1, size_t... I2>
    pair<T1, T2>::pair(piecewise_construct_t, TupleType<Args1...>& first_args, TupleType<Args2...>& second_args,
        AZStd::index_sequence<I1...>, AZStd::index_sequence<I2...>)
        : first(AZStd::forward<Args1>(AZStd::get<I1>(first_args))...)
        , second(AZStd::forward<Args2>(AZStd::get<I2>(second_args))...)
    {
        AZ_STATIC_ASSERT((AZStd::is_same<TupleType<Args2...>, tuple<Args2...>>::value), "AZStd::pair tuple constructor can be called with AZStd::tuple instances");
    }

    // Pair constructor overloads which take in a tuple is implemented here as tuple is not included at the place where pair declares the constructor
    template<class T1, class T2>
    template<template<class...> class TupleType, class... Args1, class... Args2>
    pair<T1, T2>::pair(piecewise_construct_t,
        TupleType<Args1...> first_args,
        TupleType<Args2...> second_args)
        : pair(first_args, second_args, AZStd::make_index_sequence<sizeof...(Args1)>{}, AZStd::make_index_sequence<sizeof...(Args2)>{})
    {
        AZ_STATIC_ASSERT((AZStd::is_same<TupleType<Args1...>, tuple<Args1...>>::value), "AZStd::pair tuple constructor can be called with AZStd::tuple instances");
    }

    namespace Internal
    {
        template<size_t> struct get_pair;

        template<>
        struct get_pair<0>
        {
            template<class T1, class T2>
            static T1& get(AZStd::pair<T1, T2>& pairObj) { return pairObj.first; }

            template<class T1, class T2>
            static const T1& get(const AZStd::pair<T1, T2>& pairObj) { return pairObj.first; }

            template<class T1, class T2>
            static T1&& get(AZStd::pair<T1, T2>&& pairObj) { return AZStd::forward<T1>(pairObj.first); }

            template<class T1, class T2>
            static const T1&& get(const AZStd::pair<T1, T2>&& pairObj) { return AZStd::forward<T1>(pairObj.first); }
        };
        template<>
        struct get_pair<1>
        {
            template<class T1, class T2>
            static T2& get(AZStd::pair<T1, T2>& pairObj) { return pairObj.second; }

            template<class T1, class T2>
            static const T2& get(const AZStd::pair<T1, T2>& pairObj) { return pairObj.second; }

            template<class T1, class T2>
            static T2&& get(AZStd::pair<T1, T2>&& pairObj) { return AZStd::forward<T2>(pairObj.second); }

            template<class T1, class T2>
            static const T2&& get(const AZStd::pair<T1, T2>&& pairObj) { return AZStd::forward<T2>(pairObj.second); }
        };
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    AZStd::tuple_element_t<I, AZStd::pair<T1, T2>>& get(AZStd::pair<T1, T2>& pairObj)
    {
        return Internal::get_pair<I>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    const AZStd::tuple_element_t<I, AZStd::pair<T1, T2>>& get(const AZStd::pair<T1, T2>& pairObj)
    {
        return Internal::get_pair<I>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    AZStd::tuple_element_t<I, AZStd::pair<T1, T2>>&& get(AZStd::pair<T1, T2>&& pairObj)
    {
        return Internal::get_pair<I>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    const AZStd::tuple_element_t<I, AZStd::pair<T1, T2>>&& get(const AZStd::pair<T1, T2>&& pairObj)
    {
        return Internal::get_pair<I>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    T& get(AZStd::pair<T, U>& pairObj)
    {
        return Internal::get_pair<0>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    T& get(AZStd::pair<U, T>& pairObj)
    {
        return Internal::get_pair<1>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    const T& get(const AZStd::pair<T, U>& pairObj)
    {
        return Internal::get_pair<0>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    const T& get(const AZStd::pair<U, T>& pairObj)
    {
        return Internal::get_pair<1>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    T&& get(AZStd::pair<T, U>&& pairObj)
    {
        return Internal::get_pair<0>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    T&& get(AZStd::pair<U, T>&& pairObj)
    {
        return Internal::get_pair<1>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    const T&& get(const AZStd::pair<T, U>&& pairObj)
    {
        return Internal::get_pair<0>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    const T&& get(const AZStd::pair<U, T>&& pairObj)
    {
        return Internal::get_pair<1>::get(AZStd::move(pairObj));
    }
}

// The tuple_size and tuple_element classes need to be specialized in the std:: namespace since the AZStd:: namespace alias them
// The tuple_size and tuple_element classes is to be specialized here for the AZStd::pair class
// The tuple_size and tuple_element classes is to be specialized here for the AZStd::array class
namespace std
{
    template<class T1, class T2>
    class tuple_size<AZStd::pair<T1, T2>> : public AZStd::integral_constant<size_t, 2> {};

    template<class T1, class T2>
    class tuple_element<0, AZStd::pair<T1, T2>>
    {
    public:
        using type = T1;
    };

    template<class T1, class T2>
    class tuple_element<1, AZStd::pair<T1, T2>>
    {
    public:
        using type = T2;
    };

    template<class T, size_t N>
    class tuple_size<AZStd::array<T, N>> : public AZStd::integral_constant<size_t, N> {};

    template<size_t I, class T, size_t N>
    class tuple_element<I, AZStd::array<T, N>>
    {
        AZ_STATIC_ASSERT(I < N, "AZStd::tuple_element has been called on array with an index that is out of bounds");
        using type = T;
    };
}

// Adds typeinfo specialization for tuple type
namespace AZ
{
    AZ_INTERNAL_VARIATION_SPECIALIZATION_TEMPLATE(AZStd::tuple, tuple, "{F99F9308-DC3E-4384-9341-89CBF1ABD51E}");
}