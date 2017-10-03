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
#ifndef AZSTD_ALIGNED_STORAGE_INCLUDED
#define AZSTD_ALIGNED_STORAGE_INCLUDED

#include <AzCore/std/typetraits/config.h>
#include <AzCore/std/typetraits/is_pod.h>
#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    // align_to for POD type, otherwise the class should be aligned internally
    // \todo move to a separate file
    template<typename T, AZStd::size_t Alignment>
    struct align_to
    {};

    // In the future this should replaced with just: typedef AZ_ALIGN(_T,_Alignment) type,
    // but this doesn't compiler properly on gcc 4.1.1.
    // The reason to fix this in the future is that the union can break/limit some potential optimizations.
    #define AZSTD_ALIGN_TEMPLATE_TYPE(_T, _Alignment) \
    union type {                                      \
        AZ_ALIGN(char m_align, _Alignment);           \
        _T  m_data; } m_data

    template<typename T>
    struct align_to<T, 1>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 1);
    };
    template<typename T>
    struct align_to<T, 2>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 2);
    };
    template<typename T>
    struct align_to<T, 4>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 4);
    };
    template<typename T>
    struct align_to<T, 8>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 8);
    };
    template<typename T>
    struct align_to<T, 16>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 16);
    };
    template<typename T>
    struct align_to<T, 32>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 32);
    };
    template<typename T>
    struct align_to<T, 64>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 64);
    };
    template<typename T>
    struct align_to<T, 128>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 128);
    };
    template<typename T>
    struct align_to<T, 256>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 256);
    };
    template<typename T>
    struct align_to<T, 512>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 512);
    };
    template<typename T>
    struct align_to<T, 1024>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 1024);
    };
    template<typename T>
    struct align_to<T, 2048>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 2048);
    };
    template<typename T>
    struct align_to<T, 4096>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 4096);
    };
    template<typename T>
    struct align_to<T, 8192>
    {
        AZSTD_ALIGN_TEMPLATE_TYPE(T, 8192);
    };

#if !defined(AZSTD_HAS_TYPE_TRAITS_INTRINSICS)
    // Make sure that is_pod recognizes align_to
    template<class T, AZStd::size_t Alignment>
    struct is_pod< align_to<T, Alignment> >
        : public true_type {};
    //////////////////////////////////////////////////////////////////////////
#endif

    namespace Internal
    {
        template<AZStd::size_t Size, AZStd::size_t Alignment>
        struct aligned_storage
        {
            union //data_type
            {
                align_to<char, Alignment>    m_alignment;
                char                        m_buffer[Size];
            };
        };
    }

    /**
     * As it \ref CTR1, this class will statically allocate an aligned buffer
     * of \b Size bytes with \b Alignment.
     */
    template<AZStd::size_t Size, AZStd::size_t Alignment>
    struct aligned_storage
    {
        typedef typename Internal::aligned_storage<Size, Alignment> type;

        AZ_STATIC_ASSERT(Size > 0, "You can not have 0 size");

        //#if defined(AZ_COMPILER_GCC) && (AZ_COMPILER_GCC >  3) || (AZ_COMPILER_GCC == 3 && (__GNUC_MINOR__ >  2 || (__GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ >=3)))
        //  private: // noncopyable
        //      aligned_storage(const aligned_storage&);
        //      aligned_storage& operator=(const aligned_storage&);
        //#else // gcc less than 3.2.3
        //  public: // _should_ be noncopyable, but GCC compiler emits error
        //      aligned_storage(const aligned_storage&);
        //      aligned_storage& operator=(const aligned_storage&);
        //#endif // gcc < 3.2.3 workaround
    };

#if !defined(AZSTD_HAS_TYPE_TRAITS_INTRINSICS)
    // Make sure that is_pod recognizes align_to
    template<AZStd::size_t Size, AZStd::size_t Alignment>
    struct is_pod< Internal::aligned_storage<Size, Alignment> >
        : public true_type {};
#endif
}

#endif // AZSTD_ALIGNED_STORAGE_INCLUDED
#pragma once
