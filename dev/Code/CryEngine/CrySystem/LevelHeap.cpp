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

#include "StdAfx.h"

#if USE_LEVEL_HEAP

#include "LevelHeap.h"
#include "PageMappingHeap.h"

#include "BucketAllocatorImpl.h"

int CLevelHeap::s_sys_LevelHeap;

void CLevelHeap::RegisterCVars()
{
    REGISTER_CVAR2("sys_LevelHeap", &s_sys_LevelHeap, 1, VF_REQUIRE_APP_RESTART, "Enable the level heap");
}

CLevelHeap::CLevelHeap()
    : m_baseAddress(0)
{
}

void CLevelHeap::Initialise()
{
    m_baseAddress = (UINT_PTR)CMemoryAddressRange::ReserveSpace(DLAddressSpace + BucketAddressSpace);
    stl::reconstruct(m_dlHeap, m_baseAddress, (size_t)DLAddressSpace, (size_t)0, "Level Heap");

    m_dlEndAddress = m_baseAddress + DLAddressSpace;
    m_endAddress = m_dlEndAddress + BucketAddressSpace;

    stl::reconstruct(m_buckets, reinterpret_cast<void*>(m_dlEndAddress));

    fprintf(stdout, "Level heap initialised at %08x, level buckets at %08x\n", m_baseAddress, m_dlEndAddress);
}

CLevelHeap::~CLevelHeap()
{
}

void* CLevelHeap::Calloc(size_t nmemb, size_t size)
{
    size_t allocSize = nmemb * size;

    void* ptr = Malloc(allocSize);
    memset(ptr, 0, allocSize);

    return ptr;
}

void* CLevelHeap::Realloc(void* ptr, size_t sz)
{
    if (!ptr)
    {
        return Malloc(sz);
    }

    if (!sz)
    {
        Free(ptr);
        return NULL;
    }

    size_t oldSize = UsableSize(ptr);

    if (sz <= m_buckets.MaxSize)
    {
        void* newPtr = m_buckets.allocate(sz);
        memcpy(newPtr, ptr, std::min<size_t>(oldSize, sz));

        if (m_buckets.IsInAddressRange(ptr))
        {
            m_buckets.deallocate(ptr);
        }
        else
        {
            m_dlHeap.Free(ptr);
        }

        ptr = newPtr;
    }
    else
    {
        if (m_buckets.IsInAddressRange(ptr))
        {
            void* newPtr = m_dlHeap.Malloc(sz);

            memcpy(newPtr, ptr, std::min<size_t>(oldSize, sz));

            m_buckets.deallocate(ptr);

            ptr = newPtr;
        }
        else
        {
            ptr = m_dlHeap.Realloc(ptr, sz);
        }
    }

#if TRACK_LEVEL_HEAP_USAGE
    CryInterlockedAdd(&m_sizeLiveAllocs, static_cast<int>(UsableSize(ptr) - oldSize));
#endif

    return ptr;
}

void* CLevelHeap::ReallocAlign(void* ptr, size_t size, size_t alignment)
{
    if (!ptr)
    {
        return Memalign(alignment, size);
    }

    if (!size)
    {
        Free(ptr);
        return NULL;
    }

    size_t oldSize = UsableSize(ptr);
    void* newPtr = NULL;

    if (size <= m_buckets.MaxSize && m_buckets.CanGuaranteeAlignment(size, alignment))
    {
        newPtr = m_buckets.allocate(size);
    }
    else
    {
        newPtr = m_dlHeap.Memalign(alignment, size);
    }

    memcpy(newPtr, ptr, std::min<size_t>(oldSize, size));

    if (m_buckets.IsInAddressRange(ptr))
    {
        m_buckets.deallocate(ptr);
    }
    else
    {
        m_dlHeap.Free(ptr);
    }

    ptr = newPtr;

#if TRACK_LEVEL_HEAP_USAGE
    CryInterlockedAdd(&m_sizeLiveAllocs, static_cast<int>(UsableSize(ptr) - oldSize));
#endif

    return ptr;
}

void* CLevelHeap::Memalign(size_t boundary, size_t size)
{
    void* ptr = NULL;

    if (size <= m_buckets.MaxSize && m_buckets.CanGuaranteeAlignment(size, boundary))
    {
        ptr = m_buckets.allocate(size);
    }
    else
    {
        ptr = m_dlHeap.Memalign(boundary, size);
    }

#if TRACK_LEVEL_HEAP_USAGE
    CryInterlockedIncrement(&m_numLiveAllocs);
    CryInterlockedAdd(&m_sizeLiveAllocs, static_cast<int>(UsableSize(ptr)));
#endif

    return ptr;
}

size_t CLevelHeap::UsableSize(void* ptr)
{
    UINT_PTR ptri = reinterpret_cast<UINT_PTR>(ptr);

    if (size_t size = m_buckets.getSizeEx(ptr))
    {
        return size;
    }
    else if (m_baseAddress <= ptri && ptri < m_dlEndAddress)
    {
        return m_dlHeap.UsableSize(ptr);
    }
    else
    {
        return 0;
    }
}

void CLevelHeap::Cleanup()
{
    if (IsValid())
    {
        m_dlHeap.Cleanup();
        m_buckets.cleanup();
    }
}

void CLevelHeap::GetState(size_t& numLiveAllocsOut, size_t& sizeLiveAllocsOut) const
{
#if TRACK_LEVEL_HEAP_USAGE
    numLiveAllocsOut = static_cast<size_t>(m_numLiveAllocs);
    sizeLiveAllocsOut = static_cast<size_t>(m_sizeLiveAllocs);
#else
    numLiveAllocsOut = 0;
    sizeLiveAllocsOut = 0;
#endif
}

void CLevelHeap::ReplayRegisterAddressRanges()
{
#if CAPTURE_REPLAY_LOG
    m_buckets.ReplayRegisterAddressRange("Level Buckets");
#endif
}

#if TRACK_LEVEL_HEAP_USAGE

void CLevelHeap::TrackAlloc(void* ptr)
{
    CryInterlockedIncrement(&m_numLiveAllocs);
    CryInterlockedAdd(&m_sizeLiveAllocs, static_cast<int>(UsableSize(ptr)));
}

void CLevelHeap::TrackFree(size_t sz)
{
    CryInterlockedDecrement(&m_numLiveAllocs);
    CryInterlockedAdd(&m_sizeLiveAllocs, -static_cast<int>(sz));
}

#endif

#endif
