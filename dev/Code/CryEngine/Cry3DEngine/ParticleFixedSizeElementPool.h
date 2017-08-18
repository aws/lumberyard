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

#pragma once

#if !defined(_RELEASE)
#define TRACK_PARTICLE_POOL_USAGE
#endif

#if defined(TRACK_PARTICLE_POOL_USAGE)
#include <AzCore/std/parallel/atomic.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// thread safe pool for faster allocation/deallocation of particle container and alike objects
// allocate/deallocate are only a single atomic operations on a freelist
class ParticleObjectPool
{
public:
    ParticleObjectPool();
    ~ParticleObjectPool();

    void Init(size_t bytesToAllocate);

    void* Allocate(size_t size);
    void Deallocate(void* memoryToDeallocate, size_t size);

    stl::SPoolMemoryUsage GetTotalMemory() const;
    void GetMemoryUsage(ICrySizer* sizer);

    void ResetUsage();
private:
    void Init4KBFreeList();

    void* Allocate_128Byte();
    void* Allocate_256Byte();
    void* Allocate_512Byte();

    void Deallocate_128Byte(void* ptr);
    void Deallocate_256Byte(void* ptr);
    void Deallocate_512Byte(void* ptr);

    SLockFreeSingleLinkedListHeader m_freeList4KB;  // master freelist of 4KB blocks they are splitted into the sub lists (merged back during ResetUsage)
    SLockFreeSingleLinkedListHeader m_freeList128Bytes;
    SLockFreeSingleLinkedListHeader m_freeList256Bytes;
    SLockFreeSingleLinkedListHeader m_freeList512Bytes;

    void* m_poolMemory = nullptr;
    size_t m_poolCapacity = 0;

#if defined(TRACK_PARTICLE_POOL_USAGE)
    AZStd::atomic_size_t m_usedMemory;
    AZStd::atomic_size_t m_maxUsedMemory;
    AZStd::atomic_size_t m_memory128Bytes;
    AZStd::atomic_size_t m_memory256Bytes;
    AZStd::atomic_size_t m_memory512Bytes;
    AZStd::atomic_size_t m_memory128BytesUsed;
    AZStd::atomic_size_t m_memory256BytesUsed;
    AZStd::atomic_size_t m_memory512BytesUsed;
#endif
};

///////////////////////////////////////////////////////////////////////////////
inline stl::SPoolMemoryUsage ParticleObjectPool::GetTotalMemory() const
{
#if defined(TRACK_PARTICLE_POOL_USAGE)
    return stl::SPoolMemoryUsage(
        m_poolCapacity,
        m_maxUsedMemory,
        m_usedMemory);
#else
    return stl::SPoolMemoryUsage(0, 0, 0);
#endif
}

///////////////////////////////////////////////////////////////////////////////
inline void ParticleObjectPool::GetMemoryUsage(ICrySizer* pSizer)
{
    pSizer->AddObject(m_poolMemory, m_poolCapacity);
}

///////////////////////////////////////////////////////////////////////////////
inline void ParticleObjectPool::ResetUsage()
{
    // merge all allocations back into 4KB
    Init4KBFreeList();
#if defined(TRACK_PARTICLE_POOL_USAGE)
    m_usedMemory = 0;
    m_maxUsedMemory = 0;
#endif
}
