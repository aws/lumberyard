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
#ifndef AZSTD_ARRAY_H
#define AZSTD_ARRAY_H 1

#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>

namespace AZStd
{
    /**
     * Array container is complaint with \ref CTR1 (6.2.2)
     * It's just a static array container. You are allowed to initialize it like C array:
     * array<int,5> myData = {{1,2,3,4,5}};
     * Check the array \ref AZStdExamples.
     */
    template<class T, AZStd::size_t N>
    class array
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef array<T, N>                              this_type;
    public:
        //#pragma region Type definitions
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;

        typedef T&                                      reference;
        typedef const T&                                const_reference;
        typedef typename AZStd::ptrdiff_t               difference_type;
        typedef typename AZStd::size_t                  size_type;
        typedef pointer                                 iterator;
        typedef const_pointer                           const_iterator;

        typedef AZStd::reverse_iterator<iterator>       reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef T                                       value_type;

        // AZSTD extension.
        typedef value_type                              node_type;
        //#pragma endregion

        enum
        {
            array_size = N
        };

        AZ_FORCE_INLINE iterator            begin()         { return m_elements; }
        AZ_FORCE_INLINE const_iterator      begin() const   { return m_elements; }
        AZ_FORCE_INLINE iterator            end()           { return m_elements + N; }
        AZ_FORCE_INLINE const_iterator      end() const     { return m_elements + N; }

        AZ_FORCE_INLINE reverse_iterator        rbegin()            { return reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator  rbegin() const      { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE reverse_iterator        rend()              { return reverse_iterator(begin()); }
        AZ_FORCE_INLINE const_reverse_iterator  rend() const        { return const_reverse_iterator(end()); }

        AZ_FORCE_INLINE const_iterator          cbegin() const      { return m_elements; }
        AZ_FORCE_INLINE const_iterator          cend() const        { return m_elements + N; }
        AZ_FORCE_INLINE const_reverse_iterator  crbegin() const     { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator  crend() const       { return const_reverse_iterator(end()); }

        AZ_FORCE_INLINE reference       front()         { return m_elements[0]; }
        AZ_FORCE_INLINE const_reference front() const   { return m_elements[0]; }
        AZ_FORCE_INLINE reference       back()          { return m_elements[N - 1]; }
        AZ_FORCE_INLINE const_reference back() const    { return m_elements[N - 1]; }

        AZ_FORCE_INLINE reference operator[](size_type i)
        {
            AZSTD_CONTAINER_ASSERT(i < N, "out of range");
            return m_elements[i];
        }

        AZ_FORCE_INLINE const_reference operator[](size_type i) const
        {
            AZSTD_CONTAINER_ASSERT(i < N, "out of range");
            return m_elements[i];
        }

        AZ_FORCE_INLINE reference           at(size_type i) { AZSTD_CONTAINER_ASSERT(i < N, "out of range");  return m_elements[i]; }
        AZ_FORCE_INLINE const_reference     at(size_type i) const { AZSTD_CONTAINER_ASSERT(i < N, "out of range");  return m_elements[i]; }

        // size is constant
        AZ_FORCE_INLINE static size_type size()                 { return N; }
        AZ_FORCE_INLINE static bool empty()                     { return false; }
        AZ_FORCE_INLINE static size_type max_size()             { return N; }


        // swap
        AZ_FORCE_INLINE void swap(this_type& other)             { AZStd::swap_ranges(m_elements, pointer(m_elements + N), other.m_elements); }

#if defined(AZ_HAS_RVALUE_REFS)
        AZ_FORCE_INLINE void swap(this_type&& rhs)              { AZStd::move(rhs.m_elements, pointer(rhs.m_elements + N), m_elements); }
#endif

        // direct access to data (read-only)
        AZ_FORCE_INLINE const T* data() const                   { return m_elements; }
        AZ_FORCE_INLINE T* data()                               { return m_elements; }

        // assignment with type conversion
        template <typename T2>
        AZ_FORCE_INLINE array<T, N>& operator = (const array<T2, N>& rhs)
        {
            AZStd::Internal::copy(rhs.m_elements, rhs.m_elements + N, m_elements, Internal::is_fast_copy<T*, T*>());
            return *this;
        }

        AZ_FORCE_INLINE void fill(const T& value)
        {
            AZStd::Internal::fill_n(m_elements, N, value, Internal::is_fast_fill<pointer>());
        }

        AZ_FORCE_INLINE bool    validate() const    { return true; }
        // Validate iterator.
        AZ_FORCE_INLINE int     validate_iterator(const iterator& iter) const
        {
            if (iter < m_elements || iter > (m_elements + N))
            {
                return isf_none;
            }
            else if (iter == (m_elements + N))
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        //private:
        T   m_elements[N];
    };

    template<class T>
    class array<T, 0U>
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef array<T, 0>                              this_type;
    public:
        //#pragma region Type definitions
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;

        typedef T&                                      reference;
        typedef const T&                                const_reference;
        typedef typename AZStd::ptrdiff_t               difference_type;
        typedef typename AZStd::size_t                  size_type;
        typedef pointer                                 iterator;
        typedef const_pointer                           const_iterator;

        typedef AZStd::reverse_iterator<iterator>       reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef T                                       value_type;

        // AZSTD extension.
        typedef value_type                              node_type;
        //#pragma endregion

        enum
        {
            array_size = 0
        };

        AZ_FORCE_INLINE iterator            begin() { return m_elements; }
        AZ_FORCE_INLINE const_iterator      begin() const { return m_elements; }
        AZ_FORCE_INLINE iterator            end() { return m_elements; }
        AZ_FORCE_INLINE const_iterator      end() const { return m_elements; }

        AZ_FORCE_INLINE reverse_iterator        rbegin() { return reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator  rbegin() const { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE reverse_iterator        rend() { return reverse_iterator(begin()); }
        AZ_FORCE_INLINE const_reverse_iterator  rend() const { return const_reverse_iterator(end()); }

        AZ_FORCE_INLINE const_iterator          cbegin() const { return m_elements; }
        AZ_FORCE_INLINE const_iterator          cend() const { return m_elements; }
        AZ_FORCE_INLINE const_reverse_iterator  crbegin() const { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator  crend() const { return const_reverse_iterator(end()); }

        AZ_FORCE_INLINE reference       front() { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }
        AZ_FORCE_INLINE const_reference front() const { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }
        AZ_FORCE_INLINE reference       back() { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }
        AZ_FORCE_INLINE const_reference back() const { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }

        AZ_FORCE_INLINE reference operator[](size_type)
        {
            AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0")
            return m_elements[0];
        }

        AZ_FORCE_INLINE const_reference operator[](size_type) const
        {
            AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0")
            return m_elements[0];
        }

        AZ_FORCE_INLINE reference           at(size_type) { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }
        AZ_FORCE_INLINE const_reference     at(size_type) const { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }

        // size is constant
        AZ_FORCE_INLINE static size_type size() { return 0; }
        AZ_FORCE_INLINE static bool empty() { return true; }
        AZ_FORCE_INLINE static size_type max_size() { return 0; }


        // swap
        AZ_FORCE_INLINE void swap(this_type&) {}

        // direct access to data (read-only)
        AZ_FORCE_INLINE const T* data() const { return m_elements; }
        AZ_FORCE_INLINE T* data() { return m_elements; }

        // assignment with type conversion
        template <typename T2>
        AZ_FORCE_INLINE array<T, 0>& operator = (const array<T2, 0>& rhs) { return *this; }

        AZ_FORCE_INLINE void fill(const T&){}

        AZ_FORCE_INLINE bool    validate() const { return true; }
        // Validate iterator.
        AZ_FORCE_INLINE int     validate_iterator(const iterator& iter) const { return iter == m_elements ? isf_valid : isf_none; }

        T m_elements[1]; // The minimum size of a class in C++ is 1 byte, so use an array of size 1 as the sentinel value
    };

    //#pragma region Vector equality/inequality
    template<class T, AZStd::size_t N>
    AZ_FORCE_INLINE bool operator==(const array<T, N>& a, const array<T, N>& b)
    {
        return equal(a.begin(), a.end(), b.begin());
    }

    template<class T, AZStd::size_t N>
    AZ_FORCE_INLINE bool operator!=(const array<T, N>& a, const array<T, N>& b)
    {
        return !(a == b);
    }
    //#pragma endregion

    template<size_t I, class T, size_t N>
    T& get(AZStd::array<T, N>& arr)
    {
        AZ_STATIC_ASSERT(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return arr[I];
    };

    template<size_t I, class T, size_t N>
    const T& get(const AZStd::array<T, N>& arr)
    {
        AZ_STATIC_ASSERT(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return arr[I];
    };

    template<size_t I, class T, size_t N>
    T&& get(AZStd::array<T, N>&& arr)
    {
        AZ_STATIC_ASSERT(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return AZStd::move(arr[I]);
    };

    template<size_t I, class T, size_t N>
    const T&& get(const AZStd::array<T, N>&& arr)
    {
        AZ_STATIC_ASSERT(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return AZStd::move(arr[I]);
    };
}

#endif // AZSTD_ARRAY_H
#pragma once