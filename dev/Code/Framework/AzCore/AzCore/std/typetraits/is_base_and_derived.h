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
#ifndef AZSTD_TYPE_TRAITS_IS_BASE_AND_DERIVED_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_BASE_AND_DERIVED_INCLUDED

#include <AzCore/std/typetraits/intrinsics.h>

#ifndef AZSTD_IS_BASE_OF
    #include <AzCore/std/typetraits/is_class.h>
    #include <AzCore/std/typetraits/is_same.h>
    #include <AzCore/std/typetraits/is_convertible.h>
    #include <AzCore/std/typetraits/internal/ice_and.h>
    #include <AzCore/std/typetraits/remove_cv.h>
#endif

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        #ifndef AZSTD_IS_BASE_OF
        /*************************************************************************
        This version detects ambiguous base classes and private base classes
        correctly, and was devised by Rani Sharoni.

        Explanation by Terje Slettebo and Rani Sharoni.

        Let's take the multiple base class below as an example, and the following
        will also show why there's not a problem with private or ambiguous base
        class:

        struct B {};
        struct B1 : B {};
        struct B2 : B {};
        struct D : private B1, private B2 {};

        is_base_and_derived<B, D>::value;

        First, some terminology:

        SC  - Standard conversion
        UDC - User-defined conversion

        A user-defined conversion sequence consists of an SC, followed by an UDC,
        followed by another SC. Either SC may be the identity conversion.

        When passing the default-constructed Host object to the overloaded check_sig()
        functions (initialization 8.5/14/4/3), we have several viable implicit
        conversion sequences:

        For "static no_type check_sig(B const volatile *, int)" we have the conversion
        sequences:

        C -> C const (SC - Qualification Adjustment) -> B const volatile* (UDC)
        C -> D const volatile* (UDC) -> B1 const volatile* / B2 const volatile* ->
             B const volatile* (SC - Conversion)

        For "static yes_type check_sig(D const volatile *, T)" we have the conversion
        sequence:

        C -> D const volatile* (UDC)

        According to 13.3.3.1/4, in context of user-defined conversion only the
        standard conversion sequence is considered when selecting the best viable
        function, so it only considers up to the user-defined conversion. For the
        first function this means choosing between C -> C const and C -> C, and it
        chooses the latter, because it's a proper subset (13.3.3.2/3/2) of the
        former. Therefore, we have:

        C -> D const volatile* (UDC) -> B1 const volatile* / B2 const volatile* ->
             B const volatile* (SC - Conversion)
        C -> D const volatile* (UDC)

        Here, the principle of the "shortest subsequence" applies again, and it
        chooses C -> D const volatile*. This shows that it doesn't even need to
        consider the multiple paths to B, or accessibility, as that possibility is
        eliminated before it could possibly cause ambiguity or access violation.

        If D is not derived from B, it has to choose between C -> C const -> B const
        volatile* for the first function, and C -> D const volatile* for the second
        function, which are just as good (both requires a UDC, 13.3.3.2), had it not
        been for the fact that "static no_type check_sig(B const volatile *, int)" is
        not templated, which makes C -> C const -> B const volatile* the best choice
        (13.3.3/1/4), resulting in "no".

        Also, if Host::operator B const volatile* hadn't been const, the two
        conversion sequences for "static no_type check_sig(B const volatile *, int)", in
        the case where D is derived from B, would have been ambiguous.

        See also
        http://groups.google.com/groups?selm=df893da6.0301280859.522081f7%40posting.
        google.com and links therein.

        *************************************************************************/

        template <typename B, typename D>
        struct bd_helper
        {
            //
            // This VC7.1 specific workaround stops the compiler from generating
            // an internal compiler error when compiling with /vmg (thanks to
            // Aleksey Gurtovoy for figuring out the workaround).
            //
            #if defined(AZ_COMPILER_MSVC) && (AZ_COMPILER_MSVC == 1310)
            template <typename T>
            static type_traits::yes_type check_sig(D const volatile*, T);
            static type_traits::no_type  check_sig(B const volatile*, int);
            #else
            static type_traits::yes_type check_sig(D const volatile*, long);
            static type_traits::no_type  check_sig(B const volatile* const&, int);
            #endif
        };

        template<typename B, typename D>
        struct is_base_and_derived_impl2
        {
            #if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
            #pragma warning(push)
            #pragma warning(disable:6334)
            #endif
            //
            // May silently do the wrong thing with incomplete types
            // unless we trap them here:
            //
            AZ_STATIC_ASSERT(sizeof(B) != 0, "B must be a complete type");
            AZ_STATIC_ASSERT(sizeof(D) != 0, "D must be a complete type");

            struct Host
            {
            #if !defined(AZ_COMPILER_MSVC) || (AZ_COMPILER_MSVC != 1310)
                operator B const volatile* () const;
            #else
                operator B const volatile* const& () const;
            #endif
                operator D const volatile* ();
            };

            AZSTD_STATIC_CONSTANT(bool, value =
                    sizeof(bd_helper<B, D>::check_sig(Host(), 0)) == sizeof(type_traits::yes_type));

            #if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
            #pragma warning(pop)
            #endif
        };

        template <typename B, typename D>
        struct is_base_and_derived_impl3
        {
            AZSTD_STATIC_CONSTANT(bool, value = false);
        };

        template <bool ic1, bool ic2, bool iss>
        struct is_base_and_derived_select
        {
            template <class T, class U>
            struct rebind
            {
                typedef is_base_and_derived_impl3<T, U> type;
            };
        };

        template <>
        struct is_base_and_derived_select<true, true, false>
        {
            template <class T, class U>
            struct rebind
            {
                typedef is_base_and_derived_impl2<T, U> type;
            };
        };

        template <typename B, typename D>
        struct is_base_and_derived_impl
        {
            typedef typename remove_cv<B>::type ncvB;
            typedef typename remove_cv<D>::type ncvD;

            typedef is_base_and_derived_select<
                    ::AZStd::is_class<B>::value,
                    ::AZStd::is_class<D>::value,
                    ::AZStd::is_same<B, D>::value> selector;
            typedef typename selector::template rebind<ncvB, ncvD> binder;
            typedef typename binder::type bound_type;

            AZSTD_STATIC_CONSTANT(bool, value = bound_type::value);
        };
        #else
        template <typename B, typename D>
        struct is_base_and_derived_impl
        {
            AZSTD_STATIC_CONSTANT(bool, value = AZSTD_IS_BASE_OF(B, D));
        };
        #endif
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF2(is_base_and_derived, Base, Derived, (::AZStd::Internal::is_base_and_derived_impl<Base, Derived>::value))

    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC2_2(typename Base, typename Derived, is_base_and_derived, Base &, Derived, false)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC2_2(typename Base, typename Derived, is_base_and_derived, Base, Derived &, false)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC2_2(typename Base, typename Derived, is_base_and_derived, Base &, Derived &, false)
}

#endif // AZSTD_TYPE_TRAITS_IS_BASE_AND_DERIVED_INCLUDED
#pragma once