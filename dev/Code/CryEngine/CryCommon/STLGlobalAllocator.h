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

#ifndef CRYINCLUDE_CRYCOMMON_STLGLOBALALLOCATOR_H
#define CRYINCLUDE_CRYCOMMON_STLGLOBALALLOCATOR_H
#pragma once


//---------------------------------------------------------------------------
// STL-compatible interface for an std::allocator using the global heap.
//---------------------------------------------------------------------------

#include <stddef.h>
#include <climits>

#include "CryMemoryManager.h"

class ICrySizer;
namespace stl
{
    template <class T>
    class STLGlobalAllocator
    {
    public:
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;
        typedef T*        pointer;
        typedef const T*  const_pointer;
        typedef T&        reference;
        typedef const T&  const_reference;
        typedef T         value_type;

        template <class U>
        struct rebind
        {
            typedef STLGlobalAllocator<U> other;
        };

        STLGlobalAllocator() throw()
        {
        }

        STLGlobalAllocator(const STLGlobalAllocator&) throw()
        {
        }

        template <class U>
        STLGlobalAllocator(const STLGlobalAllocator<U>&) throw()
        {
        }

        ~STLGlobalAllocator() throw()
        {
        }

        pointer address(reference x) const
        {
            return &x;
        }

        const_pointer address(const_reference x) const
        {
            return &x;
        }

        pointer allocate(size_type n = 1, const void* hint = 0)
        {
            (void)hint;
            ScopedSwitchToGlobalHeap useGlobalHeap;
            return static_cast<pointer>(CryModuleMalloc(n * sizeof(T)));
        }

        void deallocate(pointer p, size_type n = 1)
        {
            CryModuleFree(p);
        }

        size_type max_size() const throw()
        {
            return INT_MAX;
        }
#if !defined(_LIBCPP_VERSION)
        void construct(pointer p, const T& val)
        {
            new(static_cast<void*>(p))T(val);
        }

        void construct(pointer p)
        {
            new(static_cast<void*>(p))T();
        }
#endif // !_LIBCPP_VERSION
        void destroy(pointer p)
        {
            p->~T();
        }

        pointer new_pointer()
        {
            return new(allocate())T();
        }

        pointer new_pointer(const T& val)
        {
            return new(allocate())T(val);
        }

        void delete_pointer(pointer p)
        {
            p->~T();
            deallocate(p);
        }

        bool operator==(const STLGlobalAllocator&) const { return true; }
        bool operator!=(const STLGlobalAllocator&) const { return false; }

        static void GetMemoryUsage(ICrySizer* pSizer)
        {
        }
    };

    template <>
    class STLGlobalAllocator<void>
    {
    public:
        typedef void* pointer;
        typedef const void* const_pointer;
        typedef void value_type;
        template <class U>
        struct rebind
        {
            typedef STLGlobalAllocator<U> other;
        };
    };
}

#endif // CRYINCLUDE_CRYCOMMON_STLGLOBALALLOCATOR_H
