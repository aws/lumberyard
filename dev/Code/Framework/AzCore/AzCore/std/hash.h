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
#ifndef AZSTD_HASH_H
#define AZSTD_HASH_H 1

#include <AzCore/std/utils.h>

namespace AZStd
{
    /**
     * \name HashFuntions Hash Functions
     * Follow the standard 20.8.12
     * @{
     */

    /// Default template (just try to cast the value to size_t)
    template<class T>
    struct hash
    {
        typedef T               argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(const argument_type& value) const { return static_cast<result_type>(value); }
    };

    template< class T >
    struct hash< const T* >
    {
        typedef const T*        argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            // Implementation by Alberto Barbati and Dave Harris.
            AZStd::size_t x = static_cast<AZStd::size_t>(reinterpret_cast<AZStd::ptrdiff_t>(value));
            return x + (x >> 3);
        }
    };

    template< class T >
    struct hash< T* >
    {
        typedef T* argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            // Implementation by Alberto Barbati and Dave Harris.
            AZStd::size_t x = static_cast<AZStd::size_t>(reinterpret_cast<AZStd::ptrdiff_t>(value));
            return x + (x >> 3);
        }
    };

    template <class T>
    AZ_FORCE_INLINE void hash_combine(AZStd::size_t& seed, T const& v)
    {
        hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template <class It>
    AZ_FORCE_INLINE AZStd::size_t hash_range(It first, It last)
    {
        AZStd::size_t seed = 0;
        for (; first != last; ++first)
        {
            hash_combine(seed, *first);
        }
        return seed;
    }

    template <class It>
    AZ_FORCE_INLINE void hash_range(AZStd::size_t& seed, It first, It last)
    {
        for (; first != last; ++first)
        {
            hash_combine(seed, *first);
        }
    }

    template< class T, unsigned N >
    struct hash< const T[N] >
    {
        //typedef const T[N]        argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(const T(&value)[N]) const { return hash_range(value, value + N); }
    };

    template< class T, unsigned N >
    struct hash< T[N] >
        : public hash<const T[N]>
    {
        //typedef T[N]          argument_type;
    };

    template<>
    struct hash< float >
    {
        typedef float           argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            union // cast via union to get around "strict type" compiler warnings
            {
                float* cast_float;
                AZ::u32* cast_u32;
            };
            cast_float = &value;
            AZ::u32 bits = *cast_u32;
            return static_cast<result_type>((bits == 0x80000000 ? 0 : bits));
        }
    };

    template<>
    struct hash< double >
    {
        typedef double          argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            union // cast via union to get around "strict type" compiler warnings
            {
                double* cast_double;
                AZ::u64* cast_u64;
            };
            cast_double = &value;
            AZ::u64 bits = *cast_u64;
            return static_cast<result_type>((bits == 0x7fffffffffffffff ? 0 : bits));
        }
    };

    template<>
    struct hash< long double >
    {
        typedef long double     argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            union // cast via union to get around "strict type" compiler warnings
            {
                long double* cast_long_double;
                AZ::u64* cast_u64; // use first 64 bits
            };
            cast_long_double = &value;
            AZ::u64 bits = *cast_u64;
            return static_cast<result_type>((bits == 0x7fffffffffffffff ? 0 : bits));
        }
    };

    template<class A, class B>
    struct hash< AZStd::pair<A, B> >
    {
        typedef AZStd::pair<A, B>    argument_type;
        typedef AZStd::size_t       result_type;
        inline result_type operator()(const argument_type& value) const
        {
            result_type seed = 0;
            hash_combine(seed, value.first);
            hash_combine(seed, value.second);
            return seed;
        }
    };
    //@} Hash functions

    // Bucket size suitable to hold n elements.
    AZStd::size_t hash_next_bucket_size(AZStd::size_t n);
}

#endif // AZSTD_HASH_H
#pragma once