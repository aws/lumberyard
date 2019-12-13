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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Debug allocator and common STL classes using the allocator.
// Provides the MSVC heap runtime with the information about the source of allocation:
// prevents NO_SOURCE_0 for the allocation source for STL classes.
// USAGE:
// Include vector, string, set, map
// Include crtdbg
// inlcude this file
// Use cry::vector instead of std::vector,
//     cry::string instead of string,
//     cry::set instead of std::set
//     cry::map instead of std::map
//
// In order to specify your own source of allocation, in the constructor specify one or two parameters
// like:
//   ...
//   cry::map<int,cry::string> m_mapSomething;
//   ...
//   (in the constructor initializer list:)
//   ...
//   m_mapSomething ("My class.mapSomething", 0),
//   ...
// This works for vector, set and map. For string, specify:
//   cry::string strMyString ("Your string", cry::string::_Allocator("My Source.strMyString", 0));
//
// NOTE:
//   You MUST include the map, string, vector and set in order to cry:: classes to be defined here.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CRYINCLUDE_CRYCOMMON_STLDBGALLOC_H
#define CRYINCLUDE_CRYCOMMON_STLDBGALLOC_H
#pragma once


#include <iterator>
#ifdef WIN32
#include <xmemory>
#endif

#include "TAlloc.h"

namespace cry
{
#if defined(_DEBUG) && defined(_INC_CRTDBG) && defined(WIN32)
    // This is just a debug mode build allocator. In release mode build it just defaults to the std::allocator
    // The szSource is the actual reason for this class - it will call operator new using this as the source file allocating the block
    // (of course this can be any name). THe number is just because the std lib interface already accepts line number
    // This allocator has static fields, meaning that copying it won't copy the szSource and nSource properties

    template<class T>
    class dbg_allocator
        : public TSimpleAllocator<T>
    {   // generic allocator for objects of class T
    public:
        typedef _SIZT size_type;
        typedef _PDFT difference_type;
        typedef T _FARQ* pointer;
        typedef const T _FARQ* const_pointer;
        typedef T _FARQ& reference;
        typedef const T _FARQ& const_reference;
        typedef T value_type;

        template<class _Other>
        struct rebind
        {   // convert an allocator<T> to an allocator <_Other>
            typedef dbg_allocator<_Other> other;
        };

        pointer address(reference _Val) const
        {   // return address of mutable _Val
            return (&_Val);
        }

        const_pointer address(const_reference _Val) const
        {   // return address of nonmutable _Val
            return (&_Val);
        }

        dbg_allocator<T>(const char* szParentObject = "cry_dbg_alloc", int nParentIndex = 0) :
        m_szParentObject(szParentObject),
        m_nParentIndex (nParentIndex)
        {   // construct default allocator (do nothing)
        }

        dbg_allocator(const dbg_allocator<T>& rThat)
        {   // construct by copying (do nothing)
            m_szParentObject = rThat.m_szParentObject;
            m_nParentIndex = rThat.m_nParentIndex;
        }

        template<class _Other>
        dbg_allocator(const dbg_allocator<_Other>& rThat)

        {   // construct from a related allocator (do nothing)
            m_szParentObject = rThat.m_szParentObject;
            m_nParentIndex = rThat.m_nParentIndex;
        }

        template<class _Other>
        dbg_allocator<T>& operator=(const dbg_allocator<_Other>& rThat)
        {   // assign from a related allocator (do nothing)
            m_szParentObject = rThat.m_szParentObject;
            m_nParentIndex = rThat.m_nParentIndex;
            return (*this);
        }


        pointer allocate(size_type _Count, const void*)
        {   // allocate array of _Count elements, ignore hint
            return allocate (_Count);
        }

        pointer allocate(size_type _Count)
        {   // allocate array of _Count elements
            return (pointer)_malloc_dbg(_Count * sizeof(T), _NORMAL_BLOCK, m_szParentObject, m_nParentIndex);
        }

        void deallocate(pointer _Ptr, size_type)
        {   // deallocate object at _Ptr, ignore size
            _free_dbg(_Ptr, _NORMAL_BLOCK);
        }

        void construct(pointer _Ptr, const T& _Val)
        {   // construct object at _Ptr with value _Val
            std::_Construct(_Ptr, _Val);
        }

        void destroy(pointer _Ptr)
        {   // destroy object at _Ptr
            _Ptr->~T();
        }

        _SIZT max_size() const
        {   // estimate maximum array size
            _SIZT _Count = (_SIZT)(-1) / sizeof (T);
            return (0 < _Count ? _Count : 1);
        }

        // the parent object (on behalf of which to call the new operator)
        const char* m_szParentObject; // the file name
        int m_nParentIndex; // the file line number
    };

    template<class _Ty,
        class _Other>
    inline
    bool operator==(const dbg_allocator<_Ty>& rLeft, const dbg_allocator<_Other>& rRight)
    {   // test for allocator equality (always true)
        return rLeft.m_szParentObject == rRight.m_szParentObject && rLeft.m_nParentIndex == rRight.m_nParentIndex;
    }

    template<class _Ty,
        class _Other>
    inline
    bool operator!=(const dbg_allocator<_Ty>&, const dbg_allocator<_Other>&)
    {   // test for allocator inequality (always false)
        return rLeft.m_szParentObject != rRight.m_szParentObject && rLeft.m_nParentIndex != rRight.m_nParentIndex;
    }


    /*
    template<class T>
    class dbg_allocator: public TSimpleAllocator<T>
        {   // generic allocator for objects of class T
    public:
        dbg_allocator<T>()
        {
        }

        dbg_allocator<T>(const char* szParentObject, int nParentIndex = 0):
            TSimpleAllocator<T>(szParentObject, nParentIndex)
        {   // construct default allocator (do nothing)
        }

        dbg_allocator<T>(const dbg_allocator<T>& rThat):
            TSimpleAllocator<T>(rThat)
        {   // construct by copying (do nothing)
        }

        template<class _Other>
            dbg_allocator(const dbg_allocator<_Other>&rThat):
                TSimpleAllocator<T>(rThat)
            {   // construct from a related allocator (do nothing)
            }

        template<class _Other>
            dbg_allocator<T>& operator=(const dbg_allocator<_Other>&rThat)
            {   // assign from a related allocator (do nothing)
                *static_cast<TSimpleAllocator<T>*>(this) = rThat;
            return (*this);
            }
        };
            */

    template<class T>
    class dbg_struct_allocator
        : public TSimpleStructAllocator<T>
    {   // generic allocator for objects of class T
    public:
        dbg_struct_allocator<T>()
        {
        }

        dbg_struct_allocator<T>(const char* szParentObject, int nParentIndex = 0) :
        TSimpleStructAllocator<T>(szParentObject, nParentIndex)
        {   // construct default allocator (do nothing)
        }

        dbg_struct_allocator<T>(const dbg_struct_allocator<T>&rThat) :
        TSimpleStructAllocator<T>(rThat)
        {   // construct by copying (do nothing)
        }

        template<class _Other>
        dbg_struct_allocator(const dbg_struct_allocator<_Other>& rThat)
            : TSimpleStructAllocator<T>(rThat)
        {   // construct from a related allocator (do nothing)
        }

        template<class _Other>
        dbg_struct_allocator<T>& operator=(const dbg_struct_allocator<_Other>& rThat)
        {   // assign from a related allocator (do nothing)
            *static_cast<TSimpleStructAllocator<T>*>(this) = rThat;
            return (*this);
        }
    };
#else
    template<class T>
    class dbg_allocator
        : public std::allocator<T>
    {
    public:
        dbg_allocator(const char* szSource = "CryDbgAlloc", int nSource = 0)
        {   // construct default allocator (do nothing)
        }

        dbg_allocator(const dbg_allocator<T>& rThat)
            : std::allocator<T> (rThat)
        {   // construct by copying (do nothing)
        }
        /*
        template<class _Other>
        dbg_allocator(const dbg_allocator<_Other>&rThat):
            std::allocator<T>(rThat)
        {   // construct from a related allocator (do nothing)
        }

        template<class _Other>
        dbg_allocator<T>& operator=(const dbg_allocator<_Other>&rThat):
            std::allocator<T>(rThat)
        {   // assign from a related allocator (do nothing)
        return (*this);
        }

        dbg_allocator(const std::allocator<T>& rThat):
            std::allocator<T> (rThat)
        {   // construct by copying (do nothing)
        }
        */
        template<class _Other>
        dbg_allocator(const std::allocator<_Other>& rThat)
            : std::allocator<T>(rThat)
        {   // construct from a related allocator (do nothing)
        }

        template<class _Other>
        dbg_allocator<T>& operator=(const std::allocator<_Other>& rThat)
            : std::allocator<T>(rThat)
        {   // assign from a related allocator (do nothing)
            return (*this);
        }
    };
#endif

#if defined(_DEBUG) && defined(WIN32)
#if defined(_MAP_)
    template <class Key, class Type, class Traits = std::less<Key> >
    class map
        : public std::map <Key, Type, Traits, dbg_allocator<std::pair<Key, Type> > >
    {
    public:
        typedef dbg_allocator<std::pair<Key, Type> > _Allocator;
        typedef std::map <Key, Type, Traits, _Allocator > _Base;
        map (const char* szParentObject = "STL map", int nParentIndex = 0)
            : _Base (_Base::key_compare(), _Base::allocator_type(szParentObject, nParentIndex))
        {
        }
    };
#endif

#if defined(_SET_)
    template <class Key, class Traits = std::less<Key> >
    class set
        : public std::set <Key, Traits, dbg_allocator<Key> >
    {
    public:
        typedef dbg_allocator<Key> _Allocator;
        typedef std::set <Key, Traits, _Allocator> _Base;
        set (const char* szParentObject = "STL set", int nParentIndex = 0)
            : _Base (_Base::key_compare(), _Allocator(szParentObject, nParentIndex))
        {
        }
    };
#endif

#if defined(_VECTOR_)
    template <class Type>
    class vector
        : public std::vector <Type, dbg_allocator<Type> >
    {
    public:
        typedef dbg_allocator<Type> _Allocator;
        typedef std::vector <Type, _Allocator> _Base;
        typedef vector<Type> _Myt;
        vector (const char* szParentObject = "STL vector", int nParentIndex = 0)
            : _Base (_Allocator(szParentObject, nParentIndex))
        {
        }
        vector (const _Myt& that)
            : _Base (that)
        {
        }
    };
#endif

#if defined(_STRING_)
    class string
        : public std::basic_string<char, std::char_traits<char>, dbg_allocator<char> >
    {
    public:
        typedef dbg_allocator<char> _Allocator;
        typedef std::basic_string<char, std::char_traits<char>, _Allocator> _Base;
        explicit string (const _Allocator allocator = _Allocator("STL string", 0))
            : _Base (allocator)
        {
        }
        string (const char* szThat, const _Allocator allocator = _Allocator("STL string", 0))
            : _Base (szThat, 0, _Base::npos, allocator)
        {
        }
        string (const char* szBegin, const char* szEnd, const _Allocator allocator = _Allocator("STL string", 0))
            : _Base (szBegin, szEnd, allocator)
        {
        }
        string (const _Base& strRight, const _Allocator allocator = _Allocator("STL string", 0))
            : _Base (strRight, 0, npos, allocator)
        {
        }
    };
#endif
#else
    template <class Key, class Type, class Traits = std::less<Key> >
    class map
        : public std::map <Key, Type, Traits >
    {
    public:
        typedef std::map <Key, Type, Traits> _Base;
        map ()
        {
        }
        map (const char* szParentObject, int nParentIndex = 0)
        {
        }
    };

    template <class Key, class Traits = std::less<Key> >
    class set
        : public std::set <Key, Traits>
    {
    public:
        typedef std::set <Key, Traits> _Base;
        set ()
        {
        }
        set (const char* szParentObject, int nParentIndex = 0)
        {
        }
    };

    template <class Type>
    class vector
        : public std::vector <Type>
    {
    public:
        typedef std::vector <Type> _Base;
        vector ()
        {
        }
        vector (const char* szParentObject, int nParentIndex = 0)
        {
        }
    };

    class string
        : public string
    {
    public:
        typedef dbg_allocator<char> _Allocator;
        typedef string _Base;
        string ()
        {
        }
        explicit string (const _Allocator allocator)
        {
        }
        string (const char* szThat)
            : _Base (szThat)
        {
        }
        string (const char* szThat, const _Allocator allocator)
            : _Base (szThat)
        {
        }
        string (const char* szBegin, const char* szEnd)
            : _Base (szBegin, szEnd)
        {
        }
        string (const char* szBegin, const char* szEnd, const _Allocator allocator)
            : _Base (szBegin, szEnd)
        {
        }
        string (const _Base& strRight)
            : _Base (strRight)
        {
        }
        string (const _Base& strRight, const _Allocator allocator)
            : _Base (strRight)
        {
        }
    };
#endif
}
#endif // CRYINCLUDE_CRYCOMMON_STLDBGALLOC_H
