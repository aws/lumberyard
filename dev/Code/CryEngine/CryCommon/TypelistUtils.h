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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_INCLUDING_FROM_TYPELIST_H
#error should only include this file from typelist.h
#endif

// expand the type list library with some useful metafunctions
namespace NTypelist
{
    // meta-function to calculate the index of a type in a typelist, or fail
    // at compile time if said type does not exist
    // usage:
    //   typedef CConstruct<int, float>::TType MyTypelist;
    //   static const int index = IndexOf<int, MyTypelist>::value;
    //   assert( index == 0 );
    template <class T, class TL>
    class IndexOf;

    template <class T, class Head, class Tail>
    class IndexOf< T, CNode<Head, Tail> >
    {
    public:
        static const int value = 1 + IndexOf< T, Tail >::value;
    };

    template <class T, class Tail>
    class IndexOf< T, CNode<T, Tail> >
    {
    public:
        static const int value = 0;
    };

    template <int I, class TL>
    class TypeAt;

    template <int I, class Head, class Tail>
    class TypeAt< I, CNode<Head, Tail> >
        : public TypeAt<I - 1, Tail>
    {
    };

    template <class Head, class Tail>
    class TypeAt< 0, CNode<Head, Tail> >
    {
    public:
        typedef Head type;
    };

    // meta-function to calculate the length of a typelist
    // usage:
    //   typedef CConstruct<int, float>::TType MyTypelist;
    //   static const int length = Length<MyTypelist>::value;
    //   assert( length == 2 );
    template <class TL>
    class Length;

    template <class Head, class Tail>
    class Length< CNode<Head, Tail> >
    {
    public:
        static const int value = 1 + Length<Tail>::value;
    };
    template <>
    class Length< CEnd >
    {
    public:
        static const int value = 0;
    };

    // meta-function for determining the size of the largest type in a typelist
    template <class TL>
    class MaximumSize;

    template <class Head, class Tail>
    class MaximumSize< CNode<Head, Tail> >
    {
        static const size_t tailValue = MaximumSize<Tail>::value;
        static const size_t headValue = sizeof(Head);
    public:
        static const size_t value = tailValue > headValue ? tailValue : headValue;
    };
    template <>
    class MaximumSize<CEnd>
    {
    public:
        static const size_t value = 1;
    };

    // meta-function to return if a type is the same as another type
    template <typename A, typename B>
    struct IsSameType
    {
        static const bool value = false;
    };
    template <typename A>
    struct IsSameType<A, A>
    {
        static const bool value = true;
    };

    // meta-function to return if a given type is included in a typelist
    template <class TL, class T>
    struct IncludesType;

    template <class Head, class Tail, class T>
    struct IncludesType< CNode<Head, Tail>, T >
    {
        static const bool value = IsSameType<Head, T>::value || IncludesType<Tail, T>::value;
    };
    template <class T>
    struct IncludesType< CEnd, T >
    {
        static const bool value = false;
    };

    // convert an integer into a type
    template <int I>
    struct Int2Type
    {
        static const int value = I;
    };

    // emulates std::pair, but for types
    template <class T1, class T2>
    struct TypePair
    {
        typedef T1 TFirst;
        typedef T2 TSecond;
    };

    // join two lists together
    template <class TL1, class TL2>
    struct ListJoin;
    template <class Head1, class Tail1, class TL2>
    struct ListJoin< CNode<Head1, Tail1>, TL2 >
    {
        typedef CNode<Head1, typename ListJoin<Tail1, TL2>::TList> TList;
    };
    template <class TL2>
    struct ListJoin< CEnd, TL2 >
    {
        typedef TL2 TList;
    };
}
