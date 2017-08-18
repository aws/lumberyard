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
#ifndef AZSTD_TYPE_TRAITS_ALIGNMENT_OF_INCLUDED
#define AZSTD_TYPE_TRAITS_ALIGNMENT_OF_INCLUDED

#include <AzCore/std/typetraits/intrinsics.h>
#include <AzCore/std/typetraits/size_t_trait_def.h>

#ifdef AZ_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable: 4121 4512) // alignment is sensitive to packing
#endif

namespace AZStd
{
    template <typename T>
    struct alignment_of;

    // get the alignment of some arbitrary type:
    namespace Internal
    {
        #ifdef AZ_COMPILER_MSVC
            #pragma warning(push)
            #pragma warning(disable:4324) // structure was padded due to __declspec(align())
        #endif
        template <typename T>
        struct alignment_of_hack
        {
            char c;
            T t;
            alignment_of_hack();
        };
        #ifdef AZ_COMPILER_MSVC
            #pragma warning(pop)
        #endif

        template <unsigned A, unsigned S>
        struct alignment_logic
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = A < S ? A : S);
        };

        template< typename T >
        struct alignment_of_impl
        {
        #if !defined(AZ_INTERNAL_ALIGNMENT_OF)
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value =
                    (::AZStd::Internal::alignment_logic<sizeof(::AZStd::Internal::alignment_of_hack<T>) - sizeof(T), sizeof(T)>::value));
        #else
            // We need this to handle the case with alignment of an abstract class,
            // this is not supported by standard implementations, so consider it an extension.
            // As alternative we can have a support for defined_alignment_of, but as of now we are trying to
            // support the generic case
            template<bool IsAbstract, class U>
            struct is_abstract_class
            {
                static U& u;
                AZSTD_STATIC_CONSTANT(AZStd::size_t, value = AZ_INTERNAL_ALIGNMENT_OF(u));
            };
            template<class U>
            struct is_abstract_class<false, U>
            {
                AZSTD_STATIC_CONSTANT(AZStd::size_t, value = AZ_INTERNAL_ALIGNMENT_OF(U));
            };

            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (is_abstract_class<AZSTD_IS_ABSTRACT(T), T>::value));
        #endif
        };
    }

    AZSTD_TYPE_TRAITS_SIZE_T_TRAIT_DEF1(alignment_of, T, ::AZStd::Internal::alignment_of_impl<T>::value)

    // references have to be treated specially, assume that a reference is just a special pointer:
    template <typename T>
    struct alignment_of<T&>
        : public alignment_of<T*>
    {};

    // void has to be treated specially:
    AZSTD_TYPE_TRAITS_SIZE_T_TRAIT_SPEC1(alignment_of, void, 0)
}

#ifdef AZ_COMPILER_MSVC
    #pragma warning(pop)
#endif

#endif // AZSTD_TYPE_TRAITS_ALIGNMENT_OF_INCLUDED
#pragma once