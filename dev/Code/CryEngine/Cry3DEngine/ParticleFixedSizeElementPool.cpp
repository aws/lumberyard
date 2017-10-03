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

// Description : A fragmentation-free allocator from a fixed-size pool and which
//               only allocates elements of the same size.


#include "StdAfx.h"
#include "ParticleFixedSizeElementPool.h"
#include "ParticleUtils.h"

///////////////////////////////////////////////////////////////////////////////
ParticleObjectPool::ParticleObjectPool()
#if defined(TRACK_PARTICLE_POOL_USAGE)
    : m_usedMemory(0)
    , m_maxUsedMemory(0)
    , m_memory128Bytes(0)
    , m_memory256Bytes(0)
    , m_memory512Bytes(0)
    , m_memory128BytesUsed(0)
    , m_memory256BytesUsed(0)
    , m_memory512BytesUsed(0)
#endif
{
    CryInitializeSListHead(m_freeList4KB);
    CryInitializeSListHead(m_freeList128Bytes);
    CryInitializeSListHead(m_freeList256Bytes);
    CryInitializeSListHead(m_freeList512Bytes);
}

///////////////////////////////////////////////////////////////////////////////
ParticleObjectPool::~ParticleObjectPool()
{
    CryInterlockedFlushSList(m_freeList4KB);
    CryInterlockedFlushSList(m_freeList128Bytes);
    CryInterlockedFlushSList(m_freeList256Bytes);
    CryInterlockedFlushSList(m_freeList512Bytes);

    ScopedSwitchToGlobalHeap globalHeap;
    CryModuleMemalignFree(m_poolMemory);
}

///////////////////////////////////////////////////////////////////////////////
void ParticleObjectPool::Init(size_t bytesToAllocate)
{
    m_poolCapacity = Align(bytesToAllocate, 4 * 1024);

    ScopedSwitchToGlobalHeap globalHeap;
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Particles Pool");

    // allocate memory block to use as pool
    if (m_poolMemory)
    {
        CryModuleMemalignFree(m_poolMemory);
        m_poolMemory = nullptr;
    }
    if (m_poolCapacity)
    {
        m_poolMemory = CryModuleMemalign(m_poolCapacity, 128);
    }

    // fill 4KB freelist
    Init4KBFreeList();
}

///////////////////////////////////////////////////////////////////////////////
void* ParticleObjectPool::Allocate(size_t size)
{
    if (size <= 128)
    {
        return Allocate_128Byte();
    }
    
    if (size <= 256)
    {
        return Allocate_256Byte();
    }
    
    if (size <= 512)
    {
        return Allocate_512Byte();
    }

    // fallback for the case that an allocation is bigger than 512 bytes
    return malloc(size);
}

///////////////////////////////////////////////////////////////////////////////
void ParticleObjectPool::Deallocate(void* memoryToDeallocate, size_t size)
{
    if (size <= 128)
    {
        Deallocate_128Byte(memoryToDeallocate);
    }
    else if (size <= 256)
    {
        Deallocate_256Byte(memoryToDeallocate);
    }
    else if (size <= 512)
    {
        Deallocate_512Byte(memoryToDeallocate);
    }
    else
    {
        free(memoryToDeallocate);
    }
}

///////////////////////////////////////////////////////////////////////////////
void ParticleObjectPool::Init4KBFreeList()
{
    CryInterlockedFlushSList(m_freeList4KB);
    CryInterlockedFlushSList(m_freeList128Bytes);
    CryInterlockedFlushSList(m_freeList256Bytes);
    CryInterlockedFlushSList(m_freeList512Bytes);

    char* elementMemory = static_cast<char*>(m_poolMemory);
    for (int i = (m_poolCapacity / (4 * 1024)) - 1; i >= 0; --i)
    {
        // Build up the linked list of free nodes to begin with - do it in
        // reverse order so that consecutive allocations will be given chunks
        // in a cache-friendly order
        SLockFreeSingleLinkedListEntry* newNode = (SLockFreeSingleLinkedListEntry*)(elementMemory + (i * (4 * 1024)));
        CryInterlockedPushEntrySList(m_freeList4KB, *newNode);
    }
}

///////////////////////////////////////////////////////////////////////////////
void* ParticleObjectPool::Allocate_128Byte()
{
    const size_t allocationSize = 128;
    do
    {
        void* listEntry = CryInterlockedPopEntrySList(m_freeList128Bytes);
        if (listEntry)
        {
#if defined(TRACK_PARTICLE_POOL_USAGE)
            m_memory128BytesUsed += allocationSize;
            m_usedMemory += allocationSize;
            size_t ourHighWaterMark = m_usedMemory;
            size_t maxUsedMemory;
            do
            {
                maxUsedMemory = m_maxUsedMemory;

                // Don't try / Stop trying to update max used memory if the used memory dropped below
                if (ourHighWaterMark <= maxUsedMemory)
                {
                    break;
                }
                
            } while (!m_maxUsedMemory.compare_exchange_strong(maxUsedMemory, m_usedMemory));
#endif
            return listEntry;
        }

        // allocation failed, refill list from a 4KB block if we have one
        void* memoryBlock = CryInterlockedPopEntrySList(m_freeList4KB);

        // stop if we ran out of pool memory
        if (!memoryBlock)
        {
            return nullptr;
        }


#if defined(TRACK_PARTICLE_POOL_USAGE)
        m_memory128Bytes += 4 * 1024;
#endif

        char* elementMemory = static_cast<char*>(memoryBlock);
        for (int i = ((4 * 1024) / allocationSize) - 1; i >= 0; --i)
        {
            // Build up the linked list of free nodes to begin with - do it in
            // reverse order so that consecutive allocations will be given chunks
            // in a cache-friendly order
            SLockFreeSingleLinkedListEntry* newNode = reinterpret_cast<SLockFreeSingleLinkedListEntry*>(elementMemory + i * allocationSize);
            CryInterlockedPushEntrySList(m_freeList128Bytes, *newNode);
        }
    } while (true);
}

///////////////////////////////////////////////////////////////////////////////
void* ParticleObjectPool::Allocate_256Byte()
{
    const size_t allocationSize = 256;
    do
    {
        void* listEntry = CryInterlockedPopEntrySList(m_freeList256Bytes);
        if (listEntry)
        {
#if defined(TRACK_PARTICLE_POOL_USAGE)
            m_memory256BytesUsed += allocationSize;
            m_usedMemory += allocationSize;
            size_t ourHighWaterMark = m_usedMemory;
            size_t maxUsedMemory = m_maxUsedMemory;
            do
            {
                maxUsedMemory = m_maxUsedMemory;

                // Don't try / Stop trying to update max used memory if the used memory dropped below
                if (ourHighWaterMark <= maxUsedMemory)
                {
                    break;
                }

            } while (!m_maxUsedMemory.compare_exchange_strong(maxUsedMemory, m_usedMemory));
#endif
            return listEntry;
        }

        // allocation failed, refill list from a 4KB block if we have one
        void* memoryBlock = CryInterlockedPopEntrySList(m_freeList4KB);

        // stop if we ran out of pool memory
        if (!memoryBlock)
        {
            return nullptr;
        }


#if defined(TRACK_PARTICLE_POOL_USAGE)
        m_memory256Bytes += 4 * 1024;
#endif

        char* elementMemory = static_cast<char*>(memoryBlock);
        for (int i = ((4 * 1024) / allocationSize) - 1; i >= 0; --i)
        {
            // Build up the linked list of free nodes to begin with - do it in
            // reverse order so that consecutive allocations will be given chunks
            // in a cache-friendly order
            SLockFreeSingleLinkedListEntry* newNode = reinterpret_cast<SLockFreeSingleLinkedListEntry*>(elementMemory + i * allocationSize);
            CryInterlockedPushEntrySList(m_freeList256Bytes, *newNode);
        }
    } while (true);
}

///////////////////////////////////////////////////////////////////////////////
void* ParticleObjectPool::Allocate_512Byte()
{
    const size_t allocationSize = 512;
    do
    {
        void* listEntry = CryInterlockedPopEntrySList(m_freeList512Bytes);
        if (listEntry)
        {
#if defined(TRACK_PARTICLE_POOL_USAGE)
            m_memory512BytesUsed += allocationSize;
            m_usedMemory += allocationSize;
            size_t ourHighWaterMark = m_usedMemory;
            size_t maxUsedMemory = m_maxUsedMemory;
            do
            {
                maxUsedMemory = m_maxUsedMemory;

                // Don't try / Stop trying to update max used memory if the used memory dropped below
                if (ourHighWaterMark <= maxUsedMemory)
                {
                    break;
                }

            } while (!m_maxUsedMemory.compare_exchange_strong(maxUsedMemory, m_usedMemory));
#endif
            return listEntry;
        }

        // allocation failed, refill list from a 4KB block if we have one
        void* memoryBlock = CryInterlockedPopEntrySList(m_freeList4KB);

        // stop if we ran out of pool memory
        if (!memoryBlock)
        {
            return nullptr;
        }

#if defined(TRACK_PARTICLE_POOL_USAGE)
        m_memory512Bytes += 4 * 1024;
#endif

        char* elementMemory = static_cast<char*>(memoryBlock);
        for (int i = ((4 * 1024) / allocationSize) - 1; i >= 0; --i)
        {
            // Build up the linked list of free nodes to begin with - do it in
            // reverse order so that consecutive allocations will be given chunks
            // in a cache-friendly order
            SLockFreeSingleLinkedListEntry* newNode = reinterpret_cast<SLockFreeSingleLinkedListEntry*>(elementMemory + i * allocationSize);
            CryInterlockedPushEntrySList(m_freeList512Bytes, *newNode);
        }
    } while (true);
}

///////////////////////////////////////////////////////////////////////////////
void ParticleObjectPool::Deallocate_128Byte(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    CryInterlockedPushEntrySList(m_freeList128Bytes, *alias_cast<SLockFreeSingleLinkedListEntry*>(ptr));
#if defined(TRACK_PARTICLE_POOL_USAGE)
    m_usedMemory -= 128;
    m_memory128BytesUsed -= 128;
#endif
}

///////////////////////////////////////////////////////////////////////////////
void ParticleObjectPool::Deallocate_256Byte(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    CryInterlockedPushEntrySList(m_freeList256Bytes, *alias_cast<SLockFreeSingleLinkedListEntry*>(ptr));
#if defined(TRACK_PARTICLE_POOL_USAGE)
    m_usedMemory -= 256;
    m_memory256BytesUsed -= 256;
#endif
}

///////////////////////////////////////////////////////////////////////////////
void ParticleObjectPool::Deallocate_512Byte(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    CryInterlockedPushEntrySList(m_freeList512Bytes, *alias_cast<SLockFreeSingleLinkedListEntry*>(ptr));
#if defined(TRACK_PARTICLE_POOL_USAGE)
    m_usedMemory -= 512;
    m_memory512BytesUsed -= 512;
#endif
}
