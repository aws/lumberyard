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

#pragma once

#include "LegacyAllocator.h"

#include <AzCore/std/algorithm.h>

namespace AZ
{
    namespace Internal
    {
        extern void* GlobalAlloc(size_t size, size_t alignment, const char* fileName, int lineNum, const char* name);
        extern void GlobalFree(void* ptr);
        extern bool IsGlobalAlloc(void* ptr);
    }
}

//-----------------------------------------------------------------------------
// CryModule allocation API
//-----------------------------------------------------------------------------
#define CryModuleMalloc(size) CryModuleMallocImpl(size, __FILE__, __LINE__)

inline void* CryModuleMallocImpl(size_t size, const char* file, const int line)
{
#if defined(AZ_MONOLITHIC_BUILD)
    if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
    {
        return AZ::Internal::GlobalAlloc(size, AZ_DEFAULT_ALIGNMENT, file, line, "LegacyAllocator malloc");
    }
#endif
    void* ptr = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(size, 0, 0, "LegacyAllocator malloc", file, line);
    return ptr;
}

#define CryModuleFree(ptr) CryModuleFreeImpl(ptr, __FILE__, __LINE__)
#define CryModuleMemalignFree(ptr) CryModuleFreeImpl(ptr, __FILE__, __LINE__)

inline void CryModuleFreeImpl(void* ptr, const char* file, const int line)
{
    if (AZ::Internal::IsGlobalAlloc(ptr))
    {
        AZ::Internal::GlobalFree(ptr);
        return;
    }
    // there are a ton of Cry statics that we cannot clean up on time without major refactor
    // Instead, we'll just ignore the free, which isn't ideal, but we're shutting down anyway
    if (!AZ::Environment::IsReady() || !AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
    {
        return;
    }
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().DeAllocate(ptr, file, line);
}

#define CryModuleMemalign(size, alignment) CryModuleMemalignImpl(size, alignment, __FILE__, __LINE__)

inline void* CryModuleMemalignImpl(size_t size, size_t alignment, const char* file, const int line)
{
    void* ptr = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(size, alignment, 0, "LegacyAllocator memalign", file, line);
    return ptr;
}

#define CryModuleCalloc(num, size) CryModuleCallocImpl(num, size, __FILE__, __LINE__)

inline void* CryModuleCallocImpl(size_t num, size_t size, const char* file, const int line)
{
    void* ptr = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(num * size, 0, 0, "LegacyAllocator calloc", file, line);
    ::memset(ptr, 0, num * size);
    return ptr;
}

#define CryModuleRealloc(ptr, size) CryModuleReallocAlignImpl(ptr, size, 0, __FILE__, __LINE__)
#define CryModuleReallocAlign(ptr, size, alignment) CryModuleReallocAlignImpl(ptr, size, alignment, __FILE__, __LINE__)

inline void* CryModuleReallocAlignImpl(void* prev, size_t size, size_t alignment, const char* file, const int line)
{
    if (!prev)
    {
        // map realloc(nullptr, ...) -> alloc() so that we can track the location of the initial alloc
        return CryModuleMemalignImpl(size, alignment, file, line);
    }
    if (size == 0)
    {
        CryModuleFreeImpl(prev, file, line);
        return nullptr;
    }
    // There should not be any code using CryRealloc during static-init time or before the allocators
// are initialized.
#if defined(AZ_MONOLITHIC_BUILD)
    if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
    {
        AZ_Assert(false, "CryRealloc/CryReallocAlign cannot be used unless the LegacyAllocator has been initialized");
        return nullptr;
    }
#endif
    void* ret = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().ReAllocate(prev, size, alignment, file, line);
    return ret;
}

//-----------------------------------------------------------------------------
// CryCrt alloc API
//-----------------------------------------------------------------------------
inline size_t CryCrtSize(void* p)
{
    if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
    {
        return 0;
    }
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().AllocationSize(p);
}

inline void* CryCrtMalloc(size_t size)
{
    return CryModuleMalloc(size);
}

inline size_t CryCrtFree(void* p)
{
    size_t size = CryCrtSize(p);
    CryModuleFree(p);
    return size;
};

//-----------------------------------------------------------------------------
// CrySystemCrt alloc API
//-----------------------------------------------------------------------------
inline size_t CrySystemCrtSize(void* p)
{
    if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
    {
        return 0;
    }
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().AllocationSize(p);
}

inline void* CrySystemCrtMalloc(size_t size)
{
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(size, 0, 0, "AZ::LegacyAllocator");
}

inline void* CrySystemCrtRealloc(void* p, size_t size)
{
    // Use LegacyAllocator's special ReAllocate
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().ReAllocate(p, size, 0);
}

inline size_t CrySystemCrtFree(void* p)
{
    size_t size = CrySystemCrtSize(p);
    CryModuleFree(p);
    return size;
}

inline size_t CrySystemCrtGetUsedSpace()
{
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().NumAllocatedBytes();
}

//-----------------------------------------------------------------------------
// CryMalloc API
//-----------------------------------------------------------------------------
inline void* CryMalloc(size_t size, size_t& allocated, size_t alignment)
{
    if (!size)
    {
        allocated = 0;
        return nullptr;
    }

    // The original implementation guaranteed 16 byte min alignment
    alignment = AZStd::GetMax<size_t>(alignment, 16);
    void* ptr = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(size, alignment, 0, "CryMalloc", __FILE__, __LINE__);
    allocated = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().AllocationSize(ptr);
    return ptr;
}

inline void* CryRealloc(void* memblock, size_t size, size_t& allocated, size_t& oldsize, size_t alignment)
{
    oldsize = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().AllocationSize(memblock);
    void* ptr = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().ReAllocate(memblock, size, alignment);
    allocated = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().AllocationSize(ptr);
    return ptr;
}

inline size_t CryFree(void* p, size_t /*alignment*/)
{
    size_t size = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().AllocationSize(p);
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().DeAllocate(p, size);
    return size;
}

inline size_t CryGetMemSize(void* memblock, size_t /*sourceSize*/)
{
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().AllocationSize(memblock);
}

inline int CryMemoryGetAllocatedSize()
{
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().NumAllocatedBytes();
}

//////////////////////////////////////////////////////////////////////////
inline int CryMemoryGetPoolSize()
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////
inline int CryStats(char* buf)
{
    return 0;
}

inline int CryGetUsedHeapSize()
{
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().NumAllocatedBytes();
}

inline int CryGetWastedHeapSize()
{
    return 0;
}

inline void CryCleanup()
{
    AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().GarbageCollect();
}

inline void CryResetStats(void)
{
}
