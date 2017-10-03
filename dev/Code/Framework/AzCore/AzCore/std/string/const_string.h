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
#ifndef AZSTD_CONST_STRING_H
#define AZSTD_CONST_STRING_H

#include <AzCore/std/string/string.h>

namespace AZStd
{
    /**
     * Immutable string wrapper based on boost::const_string and std::string_view. When we operate on
     * const char* we don't know if this points to NULL terminated string or just a char array.
     * to have a clear distinction between them we provide this wrapper.
     */
    template <class Element, class Traits = AZStd::char_traits<Element>>
    class basic_const_string 
    {
    public:
        using traits_type = Traits;
        using value_type  = Element;

        using pointer       = value_type*;
        using const_pointer = const value_type*;

        using reference       = value_type&;
        using const_reference = const value_type&;

        using size_type    = AZStd::size_t;

        static const size_type npos = AZStd::basic_string<value_type, traits_type>::npos;

        using iterator               = const value_type*;
        using const_iterator         = const value_type*;
        using reverse_iterator       = AZStd::reverse_iterator<iterator>;
        using const_reverse_iterator = AZStd::reverse_iterator<const_iterator>;

        basic_const_string()
            : m_begin(nullptr)
            , m_end(nullptr)
        { }

        template <typename Allocator>
        basic_const_string(const basic_string<value_type, traits_type, Allocator>& s)
            : m_begin(s.c_str())
            , m_end(m_begin + s.length()) 
        { }

        basic_const_string(const_pointer s)
            : m_begin(s)
            , m_end(s ? s + traits_type::length(s) : nullptr)
        { }

        basic_const_string(const_pointer s, size_type length)
            : m_begin(s)
            , m_end(m_begin + length)
        {
            if (length == 0) erase();
        }

        basic_const_string(const_pointer first, const_pointer last)
            : m_begin(first)
            , m_end(last)
        { }

        basic_const_string(const basic_const_string&) = default;
        basic_const_string(basic_const_string&& other)
            : basic_const_string()
        {
            swap(other);
        }

        const_reference operator[](size_type index) const { return m_begin[index]; }
        /// Returns value, not reference. If index is out of bounds, 0 is returned (can't be reference).
        value_type at(size_type index) const
        {
            return index > length()
                ? value_type(0)
                : m_begin[index];
        }

        const_pointer data() const { return m_begin; }

        size_type length() const { return m_end - m_begin; }
        size_type size() const   { return m_end - m_begin; }
        bool      empty() const  { return m_end == m_begin; }

        void erase()                       { m_begin = m_end = nullptr; }
        void resize(size_type new_len)     { if (m_begin + new_len < m_end) m_end = m_begin + new_len; }
        void rshorten(size_type shift = 1) { m_end -= shift; if (m_end <= m_begin) erase(); }
        void lshorten(size_type shift = 1) { m_begin += shift; if (m_end <= m_begin) erase(); }

        operator basic_string<value_type, traits_type, AZStd::allocator>() const
        {
            return to_string<AZStd::allocator>();
        }

        template <typename Allocator = AZStd::allocator>
        basic_string<value_type, traits_type, Allocator> to_string(Allocator alloc = Allocator()) const
        {
            return basic_string<value_type, traits_type, Allocator>(m_begin, m_end, alloc);
        }

        basic_const_string& operator=(const basic_const_string& s) { if( &s != this ) { m_begin = s.m_begin; m_end = s.m_end; } return *this; }
        basic_const_string& operator=(const_pointer s)             { return *this = basic_const_string(s); }

        template <typename Allocator>
        basic_const_string& operator=(const basic_string<value_type, traits_type, Allocator>& s)
        {
            return *this = basic_const_string(s);
        }

        basic_const_string& assign(const basic_const_string& s)          { return *this = s; }
        basic_const_string& assign(const_pointer s)                      { return *this = basic_const_string(s); }
        basic_const_string& assign(const_pointer s, size_type len)       { return *this = basic_const_string(s, len); }
        basic_const_string& assign(const_pointer f, const_pointer l)     { return *this = basic_const_string(f, l); }

        template <typename Allocator>
        basic_const_string& assign(const basic_string<value_type, traits_type, Allocator>& s)
        {
            return *this = basic_const_string(s);
        }

        void swap(basic_const_string& s)
        {
            const_pointer tmp1 = m_begin;
            const_pointer tmp2 = m_end;
            m_begin = s.m_begin;
            m_end = s.m_end;
            s.m_begin = tmp1;
            s.m_end = tmp2;
        }
        friend bool operator==(const basic_const_string& s1, const basic_const_string& s2)
        {
            return s1.length() == s2.length() && (s1.m_begin == nullptr || traits_type::compare(s1.m_begin, s2.m_begin, s1.length()) == 0);
        }
        friend bool operator==(const basic_const_string& s1, const_pointer s2)       { return s1 == basic_const_string(s2); }
        template <typename Allocator>
        friend bool operator==(const basic_const_string& s1, const basic_string<value_type, traits_type, Allocator>& s2) { return s1 == basic_const_string(s2); }

        friend bool operator!=(const basic_const_string& s1, const basic_const_string& s2) { return !(s1 == s2); }
        friend bool operator!=(const basic_const_string& s1, const_pointer s2) { return !(s1 == s2); }
        template <typename Allocator>
        friend bool operator!=(const basic_const_string& s1, const basic_string<value_type, traits_type, Allocator>&  s2)      { return !(s1 == s2); }

        friend bool operator==(const_pointer s2, const basic_const_string& s1) { return s1 == s2; }
        template <typename Allocator>
        friend bool operator==(const basic_string<value_type, traits_type, Allocator>&  s2, const basic_const_string& s1) { return s1 == s2; }

        friend bool operator!=(const_pointer s2, const basic_const_string& s1) { return !(s1 == s2); }
        template <typename Allocator>
        friend bool operator!=(const basic_string<value_type, traits_type, Allocator>&  s2, const basic_const_string& s1) { return !(s1 == s2); }

        iterator         begin() const  { return m_begin; }
        iterator         end() const    { return m_end;   }
        const_iterator   cbegin() const { return m_begin; }
        const_iterator   cend() const   { return m_end;   }
        reverse_iterator rbegin() const { return reverse_iterator(m_end);   }
        reverse_iterator rend() const   { return reverse_iterator(m_begin); }

    private:
        const_pointer m_begin;
        const_pointer m_end;
    };

    // Convenience aliases for common use.
    using const_string  = basic_const_string<char>;
    using const_wstring = basic_const_string<wchar_t>;

    template<class Element, class Traits>
    struct hash<basic_const_string<Element, Traits>>
    {
        using argument_type = basic_const_string<Element, Traits>;
        using result_type   = AZStd::size_t;

        inline result_type operator()(const argument_type& value) const 
        { 
            // from the string class
            return hash_string(value.begin(),value.length());
        }
    };
} // namespace AZStd

#endif // AZSTD_CONST_STRING_H
