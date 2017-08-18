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

#ifndef CRYINCLUDE_TOOLS_DBAPI_UNITTEST_STRINGUTIL_H
#define CRYINCLUDE_TOOLS_DBAPI_UNITTEST_STRINGUTIL_H
#pragma once

#include <string>
#include <stdarg.h>

const char str_default_trim_chars[] = " \t\n\r";

template<class _Strt>
_Strt& str_inner_trim_left(_Strt& str, const char* to_remove = str_default_trim_chars)
{
    if (!str.empty())
    {
        _Strt::size_type pos = str.find_first_not_of(to_remove);
        if (pos != _Strt::npos)
        {
            str.erase(0, pos);
        }
        else
        {
            str.clear();
        }
    }
    return str;
}

template<class _Strt>
_Strt& str_inner_trim_right(_Strt& str, const char* to_remove = str_default_trim_chars)
{
    if (!str.empty())
    {
        _Strt::size_type pos = str.find_last_not_of(to_remove);
        if (pos != _Strt::npos)
        {
            str.erase(pos + 1);
        }
        else
        {
            str.clear();
        }
    }
    return str;
}

template<class _Strt>
_Strt& str_inner_trim(_Strt& str, const char* to_remove = str_default_trim_chars)
{
    str_inner_trim_right(str, to_remove);
    return str_inner_trim_left(str, to_remove);
}

//////////
template<class _Strt>
_Strt str_trim_left(const _Strt& str, const char* to_remove = str_default_trim_chars)
{
    _Strt::size_type pos = str.find_first_not_of(to_remove);
    return pos == _Strt::npos ? _Strt(str.get_allocator()) : str.substr(pos, _Strt::npos);
}

template<class _Strt>
_Strt str_trim_right(const _Strt& str, const char* to_remove = str_default_trim_chars)
{
    _Strt::size_type pos = str.find_last_not_of(to_remove);
    return pos == _Strt::npos ? _Strt(str.get_allocator()) : str.substr(0, pos + 1);
}

template<class _Strt>
_Strt str_trim(const _Strt& str, const char* to_remove = str_default_trim_chars)
{
    return str_inner_trim_right(str_trim_left(str, to_remove), to_remove);
}


////////
// formattable string
class fm_string
    : public std::string
{
public:
    typedef fm_string _Myt;
    typedef std::string _Mybase;
#ifdef STLPORT
    typedef _Mybase::allocator_type _Alloc;
#else
    typedef _Mybase::_Alloc _Alloc;
#endif // STLPORT

    typedef _Mybase::const_pointer const_pointer;
    typedef _Mybase::iterator iterator;
    typedef _Mybase::const_iterator const_iterator;
    typedef _Mybase::value_type value_type;

#ifdef STLPORT
    typedef _Mybase::value_type _Elem;
#else
    typedef iterator::value_type _Elem;
#endif // STLPORT

    fm_string()
        : _Mybase() {}

    explicit fm_string(const _Alloc& _Al)
        : _Mybase(_Al)
    {   // construct empty string with allocator
        _Tidy();
    }

    fm_string(const _Elem* _Ptr, size_type _Count)
        : _Mybase(_Ptr, _Count) {}

    fm_string(const _Myt& _Right, size_type _Roff, size_type _Count,
        const _Alloc& _Al)
        : _Mybase(_Al)
    {   // construct from _Right [_Roff, _Roff + _Count) with allocator
        _Tidy();
        assign(_Right, _Roff, _Count);
    }


    fm_string(const _Elem* _Ptr)
        : _Mybase(_Ptr) {}

    fm_string(size_type _Count, _Elem _Ch)
        : _Mybase(_Count, _Ch) {}

    template<class _It>
    fm_string(_It _First, _It _Last)
        : _Mybase(_First, _Last) {}

    fm_string(const_pointer _First, const_pointer _Last)
        : _Mybase(_First, _Last) {}

    //fm_string(const_iterator _First, const_iterator _Last) : _Mybase(_First, _Last) {}

    fm_string(const _Myt& _Right)
        : _Mybase(_Right) {}

    int format(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        int result = format_arg_list(fmt, args);
        va_end(args);
        return result;
    }

    _Myt substr(size_type _Off = 0, size_type _Count = npos) const
    {   // return [_Off, _Off + _Count) as new string
        return (_Myt(*this, _Off, _Count, get_allocator()));
    }

    _Myt trim(const char* to_remove = str_default_trim_chars) const
    {
        return str_trim(*this, to_remove);
    }

    _Myt trim_left(const char* to_remove = str_default_trim_chars) const
    {
        return str_trim_left(*this, to_remove);
    }

    _Myt trim_right(const char* to_remove = str_default_trim_chars) const
    {
        return str_trim_right(*this, to_remove);
    }

    _Myt& inner_trim(const char* to_remove = str_default_trim_chars)
    {
        return inner_trim_left(to_remove).inner_trim_right(to_remove);
    }

    _Myt& inner_trim_left(const char* to_remove = str_default_trim_chars)
    {
        return str_inner_trim_left(*this, to_remove);
    }

    _Myt& inner_trim_right(const char* to_remove = str_default_trim_chars)
    {
        return str_inner_trim_right(*this, to_remove);
    }

    const char* operator*() const
    {
        return c_str();
    }

    _Myt& operator=(const std::string& _Right)
    {
        assign(_Right);
        return *this;
    }

protected:
#ifdef STLPORT
    void _Tidy() {}
#endif
    int format_arg_list(const char* format, va_list args)
    {
        if (!format)
        {
            clear();
            return 0;
        }

        int   result = -1, length = 1024;
        while (result == -1 && length <= 1024 * 1024)
        {
            resize(length, '\0');
            result = _vsnprintf_s(const_cast<_Elem*>(data()), length, _TRUNCATE, format, args);
            length *= 2;
        }
        resize(result > 0 ? result : 0);
        return result;
    }
};

#endif // CRYINCLUDE_TOOLS_DBAPI_UNITTEST_STRINGUTIL_H
