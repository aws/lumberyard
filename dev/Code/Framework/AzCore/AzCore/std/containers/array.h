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


// VS2015 C++14 support removes constexpr from non-const member functions
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1900
#define AZSTD_ARRAY_CONSTEXPR
#else
#define AZSTD_ARRAY_CONSTEXPR constexpr
#endif
namespace AZStd
{
    /**
     * Array container is complaint with \ref CTR1 (6.2.2)
     * It's just a static array container. You are allowed to initialize it like C array:
     * array<int,5> myData = {{1,2,3,4,5}};
     * Check the array \ref AZStdExamples.
     */
    template<class T, size_t N>
    class array
    {
        typedef array<T, N> this_type;
    public:
        //#pragma region Type definitions
        typedef T* pointer;
        typedef const T* const_pointer;

        typedef T& reference;
        typedef const T& const_reference;
        typedef ptrdiff_t difference_type;
        typedef size_t size_type;
        typedef pointer iterator;
        typedef const_pointer const_iterator;

        typedef AZStd::reverse_iterator<iterator> reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef T value_type;

        // AZSTD extension.
        typedef value_type node_type;
        //#pragma endregion

        enum
        {
            array_size = N
        };

        AZSTD_ARRAY_CONSTEXPR iterator begin() { return m_elements; }
        constexpr const_iterator begin() const { return m_elements; }
        AZSTD_ARRAY_CONSTEXPR iterator end() { return m_elements + N; }
        constexpr const_iterator end() const { return m_elements + N; }

        AZSTD_ARRAY_CONSTEXPR reverse_iterator rbegin() { return reverse_iterator(end()); }
        constexpr const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        AZSTD_ARRAY_CONSTEXPR reverse_iterator rend() { return reverse_iterator(begin()); }
        constexpr const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

        constexpr const_iterator cbegin() const { return m_elements; }
        constexpr const_iterator cend() const { return m_elements + N; }
        constexpr const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
        constexpr const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

        AZSTD_ARRAY_CONSTEXPR reference front() { return m_elements[0]; }
        constexpr const_reference front() const { return m_elements[0]; }
        AZSTD_ARRAY_CONSTEXPR reference back() { return m_elements[N - 1]; }
        constexpr const_reference back() const { return m_elements[N - 1]; }

        AZSTD_ARRAY_CONSTEXPR reference operator[](size_type i)
        {
            AZSTD_CONTAINER_ASSERT(i < N, "out of range");
            return m_elements[i];
        }

        constexpr const_reference operator[](size_type i) const
        {
            AZSTD_CONTAINER_ASSERT(i < N, "out of range");
            return m_elements[i];
        }

        AZSTD_ARRAY_CONSTEXPR reference at(size_type i) { AZSTD_CONTAINER_ASSERT(i < N, "out of range");  return m_elements[i]; }
        constexpr const_reference at(size_type i) const { AZSTD_CONTAINER_ASSERT(i < N, "out of range");  return m_elements[i]; }

        // size is constant
        constexpr static size_type size() { return N; }
        constexpr static bool empty() { return false; }
        constexpr static size_type max_size() { return N; }


        // swap
        void swap(this_type& other) { AZStd::swap_ranges(m_elements, pointer(m_elements + N), other.m_elements); }

        // direct access to data (read-only)
        constexpr const T* data() const { return m_elements; }
        AZSTD_ARRAY_CONSTEXPR T* data() { return m_elements; }

        // assignment with type conversion
        template <typename T2>
        array<T, N>& operator = (const array<T2, N>& rhs)
        {
            AZStd::Internal::copy(rhs.m_elements, rhs.m_elements + N, m_elements, Internal::is_fast_copy<T*, T*>());
            return *this;
        }

        void fill(const T& value)
        {
            AZStd::Internal::fill_n(m_elements, N, value, Internal::is_fast_fill<pointer>());
        }

        constexpr bool validate() const { return true; }
        // Validate iterator.
        int validate_iterator(const iterator& iter) const
        {
            if (iter < m_elements || iter >(m_elements + N))
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
        T m_elements[N];
    };

    template<class T>
    class array<T, 0U>
    {
        typedef array<T, 0> this_type;
    public:
        //#pragma region Type definitions
        typedef T* pointer;
        typedef const T* const_pointer;

        typedef T& reference;
        typedef const T& const_reference;
        typedef ptrdiff_t difference_type;
        typedef size_t size_type;
        typedef pointer iterator;
        typedef const_pointer const_iterator;

        typedef AZStd::reverse_iterator<iterator> reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef T value_type;

        // AZSTD extension.
        typedef value_type node_type;
        //#pragma endregion

        enum
        {
            array_size = 0
        };

        AZSTD_ARRAY_CONSTEXPR iterator begin() { return m_elements; }
        constexpr const_iterator begin() const { return m_elements; }
        AZSTD_ARRAY_CONSTEXPR iterator end() { return m_elements; }
        constexpr const_iterator end() const { return m_elements; }

        AZSTD_ARRAY_CONSTEXPR reverse_iterator rbegin() { return reverse_iterator(end()); }
        constexpr const_reverse_iterator  rbegin() const { return const_reverse_iterator(end()); }
        AZSTD_ARRAY_CONSTEXPR reverse_iterator rend() { return reverse_iterator(begin()); }
        constexpr const_reverse_iterator rend() const { return const_reverse_iterator(end()); }

        constexpr const_iterator cbegin() const { return m_elements; }
        constexpr const_iterator cend() const { return m_elements; }
        constexpr const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
        constexpr const_reverse_iterator crend() const { return const_reverse_iterator(end()); }

        AZSTD_ARRAY_CONSTEXPR reference front() { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }
        constexpr const_reference front() const { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }
        AZSTD_ARRAY_CONSTEXPR reference back() { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }
        constexpr const_reference back() const { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }

        AZSTD_ARRAY_CONSTEXPR reference operator[](size_type)
        {
            AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0");
            return m_elements[0];
        }

        constexpr const_reference operator[](size_type) const
        {
            AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0");
            return m_elements[0];
        }

        AZSTD_ARRAY_CONSTEXPR reference at(size_type) { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }
        constexpr const_reference at(size_type) const { AZSTD_CONTAINER_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }

        // size is constant
        constexpr static size_type size() { return 0; }
        constexpr static bool empty() { return true; }
        constexpr static size_type max_size() { return 0; }


        // swap
        AZSTD_ARRAY_CONSTEXPR void swap(this_type&) {}

        // direct access to data (read-only)
        constexpr const T* data() const { return m_elements; }
        AZSTD_ARRAY_CONSTEXPR T* data() { return m_elements; }

        // assignment with type conversion
        template <typename T2>
        AZSTD_ARRAY_CONSTEXPR array<T, 0>& operator = (const array<T2, 0>& rhs) { return *this; }

        AZSTD_ARRAY_CONSTEXPR void fill(const T&) {}

        constexpr bool validate() const { return true; }
        // Validate iterator.
        constexpr int validate_iterator(const iterator& iter) const { return iter == m_elements ? isf_valid : isf_none; }

        T m_elements[1]; // The minimum size of a class in C++ is 1 byte, so use an array of size 1 as the sentinel value
    };

    //#pragma region Vector equality/inequality
    template<class T, size_t N>
    bool operator==(const array<T, N>& a, const array<T, N>& b)
    {
        return equal(a.begin(), a.end(), b.begin());
    }

    template<class T, size_t N>
    bool operator!=(const array<T, N>& a, const array<T, N>& b)
    {
        return !(a == b);
    }
    //#pragma endregion
}

#undef AZSTD_ARRAY_CONSTEXPR
#endif // AZSTD_ARRAY_H
#pragma once