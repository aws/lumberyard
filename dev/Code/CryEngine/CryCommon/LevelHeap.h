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

#ifndef CRYINCLUDE_CRYCOMMON_LEVELHEAP_H
#define CRYINCLUDE_CRYCOMMON_LEVELHEAP_H
#pragma once


#if USE_LEVEL_HEAP

#include "BucketAllocator.h"
#include "CryDLMalloc.h"
#include "GeneralMemoryHeap.h"

class CLevelHeap
{
public:
    static void RegisterCVars();
    static bool CanBeUsed() { return s_sys_LevelHeap; }

public:
    CLevelHeap();
    ~CLevelHeap();

    void Initialise();

    ILINE bool IsValid() const { return m_baseAddress != 0; }
    ILINE bool IsInAddressRange(void* ptr) const
    {
        UINT_PTR ptri = reinterpret_cast<UINT_PTR>(ptr);
        return m_baseAddress <= ptri && ptri < m_endAddress;
    }

    void* Calloc(size_t nmemb, size_t size);

    void* Malloc(size_t sz)
    {
        void* ptr = NULL;

        if (sz <= m_buckets.MaxSize)
        {
            ptr = m_buckets.allocate(sz);
        }
        else
        {
            ptr = m_dlHeap.Malloc(sz);
        }

        TrackAlloc(ptr);
        return ptr;
    }

    size_t Free(void* ptr)
    {
        size_t sz = 0;

        if (reinterpret_cast<UINT_PTR>(ptr) >= m_dlEndAddress)
        {
            sz = m_buckets.deallocate(ptr);
        }
        else
        {
            sz = m_dlHeap.Free(ptr);
        }

        TrackFree(sz);
        return sz;
    }

    void* Realloc(void* ptr, size_t sz);
    void* ReallocAlign(void* ptr, size_t size, size_t alignment);
    void* Memalign(size_t boundary, size_t size);
    size_t UsableSize(void* ptr);

    void Cleanup();

    void GetState(size_t& numLiveAllocsOut, size_t& sizeLiveAllocsOut) const;
    void EnableExpandCleanups(bool enable) { m_buckets.EnableExpandCleanups(enable); }
    void ReplayRegisterAddressRanges();

private:
    CLevelHeap(const CLevelHeap&);
    CLevelHeap& operator = (const CLevelHeap&);

private:
#if TRACK_LEVEL_HEAP_USAGE
    void TrackAlloc(void* ptr);
    void TrackFree(size_t sz);
#else
    void TrackAlloc(void*) {}
    void TrackFree(size_t) {}
#endif

private:
    enum
    {
        DLAddressSpace = 512 * 1024 * 1024,
        BucketAddressSpace = 384 * 1024 * 1024,
    };

    typedef BucketAllocator<BucketAllocatorDetail::DefaultTraits<BucketAddressSpace, BucketAllocatorDetail::SyncPolicyLocked, false> > LevelBuckets;

private:
    static int s_sys_LevelHeap;

private:
    UINT_PTR m_baseAddress;
    UINT_PTR m_dlEndAddress;
    UINT_PTR m_endAddress;

    volatile long m_active;

    CGeneralMemoryHeap m_dlHeap;
    LevelBuckets m_buckets;

#if TRACK_LEVEL_HEAP_USAGE
    volatile int m_numLiveAllocs;
    volatile int m_sizeLiveAllocs;
#endif
};

#else
// dummy implementation to prevent too many ifdefs in client code
class CLevelHeap
{
public:
    void* Malloc(size_t sz) { __debugbreak(); return NULL; }
    size_t Free(void* ptr) { __debugbreak(); return 0; }
    void* Realloc(void* ptr, size_t sz){ __debugbreak(); return NULL; }
    void* ReallocAlign(void* ptr, size_t size, size_t alignment){ __debugbreak(); return NULL; }
    void* Memalign(size_t boundary, size_t size){ __debugbreak(); return NULL; }
    size_t UsableSize(void* ptr){ __debugbreak(); return 0; }
    bool IsInAddressRange(void* ptr) const { return false; }
};

#endif

#endif // CRYINCLUDE_CRYCOMMON_LEVELHEAP_H
