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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocatorManager.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/MemoryDrillerBus.h>

#include <AzCore/std/functional.h>

#include <AzCore/Debug/Profiler.h>

#define AZCORE_SYS_ALLOCATOR_HPPA  // If you disable this make sure you start building the heapschema.cpp

#ifdef AZCORE_SYS_ALLOCATOR_HPPA
#   include <AzCore/Memory/HphaSchema.h>
#else
#   include <AzCore/Memory/heapschema.h>
#endif

using namespace AZ;

//////////////////////////////////////////////////////////////////////////
// Globals - we use global storage for the first memory schema, since we can't use dynamic memory!
static bool g_isSystemSchemaUsed = false;
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
static AZStd::aligned_storage<sizeof(HphaSchema), AZStd::alignment_of<HphaSchema>::value>::type g_systemSchema;
#else
static AZStd::aligned_storage<sizeof(HeapSchema), AZStd::alignment_of<HeapSchema>::value>::type g_systemSchema;
#endif

//////////////////////////////////////////////////////////////////////////

//=========================================================================
// SystemAllocator
// [9/2/2009]
//=========================================================================
SystemAllocator::SystemAllocator()
    : m_isCustom(false)
    , m_allocator(nullptr)
{
}

//=========================================================================
// ~SystemAllocator
//=========================================================================
SystemAllocator::~SystemAllocator()
{
    if (IsReady())
    {
        Destroy();
    }
}

//=========================================================================
// ~Create
// [9/2/2009]
//=========================================================================
bool
SystemAllocator::Create(const Descriptor& desc)
{
    AZ_Assert(IsReady() == false, "System allocator was already created!");
    if (IsReady())
    {
        return false;
    }

    if (!AllocatorInstance<OSAllocator>::IsReady())
    {
        AllocatorInstance<OSAllocator>::Create();  // debug allocator is such that there is no much point to free it
    }
    bool isReady = false;
    if (desc.m_custom)
    {
        m_isCustom = true;
        m_allocator = desc.m_custom;
        isReady = true;
    }
    else
    {
        m_isCustom = false;
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
        HphaSchema::Descriptor      heapDesc;
        heapDesc.m_pageSize     = desc.m_heap.m_pageSize;
        heapDesc.m_poolPageSize = desc.m_heap.m_poolPageSize;
        AZ_Assert(desc.m_heap.m_numMemoryBlocks <= 1, "We support max1 memory block at the moment!");
        if (desc.m_heap.m_numMemoryBlocks > 0)
        {
            heapDesc.m_memoryBlock = desc.m_heap.m_memoryBlocks[0];
            heapDesc.m_memoryBlockByteSize = desc.m_heap.m_memoryBlocksByteSize[0];
        }
        heapDesc.m_subAllocator = desc.m_heap.m_subAllocator;
        heapDesc.m_isPoolAllocations = desc.m_heap.m_isPoolAllocations;
#else
        HeapSchema::Descriptor      heapDesc;
        memcpy(heapDesc.m_memoryBlocks, desc.m_heap.m_memoryBlocks, sizeof(heapDesc.m_memoryBlocks));
        memcpy(heapDesc.m_memoryBlocksByteSize, desc.m_heap.m_memoryBlocksByteSize, sizeof(heapDesc.m_memoryBlocksByteSize));
        heapDesc.m_numMemoryBlocks = desc.m_heap.m_numMemoryBlocks;
#endif
        if (&AllocatorInstance<SystemAllocator>::Get() == (void*)this) // if we are the system allocator
        {
            AZ_Assert(!g_isSystemSchemaUsed, "AZ::SystemAllocator MUST be created first! It's the source of all allocations!");
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
            m_allocator = new(&g_systemSchema)HphaSchema(heapDesc);
#else
            m_allocator = new(&g_systemSchema)HeapSchema(heapDesc);
#endif
            g_isSystemSchemaUsed = true;
            isReady = true;
        }
        else
        {
            // this class should be inheriting from SystemAllocator
            AZ_Assert(AllocatorInstance<SystemAllocator>::IsReady(), "System allocator must be created before any other allocator! They allocate from it.");
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
            m_allocator = azcreate(HphaSchema, (heapDesc), SystemAllocator);
#else
            m_allocator = azcreate(HeapSchema, heapDesc, SystemAllocator);
#endif
            if (m_allocator == NULL)
            {
                isReady = false;
            }
            else
            {
                isReady = true;
            }
        }
    }

#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (isReady && desc.m_allocationRecords)
    {
        EBUS_EVENT(Debug::MemoryDrillerBus, RegisterAllocator, this, desc.m_stackRecordLevels, !m_isCustom, !m_isCustom);
    }
#endif

    if (isReady)
    {
        OnCreated();
    }

    return IsReady();
}

//=========================================================================
// Allocate
// [9/2/2009]
//=========================================================================
void
SystemAllocator::Destroy()
{
    OnDestroy();

#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records)
    {
        EBUS_EVENT(Debug::MemoryDrillerBus, UnregisterAllocator, this);
    }
#endif

    if (g_isSystemSchemaUsed)
    {
        int dummy;
        (void)dummy;
    }

    if (!m_isCustom)
    {
        if ((void*)m_allocator == (void*)&g_systemSchema)
        {
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
            static_cast<HphaSchema*>(m_allocator)->~HphaSchema();
#else
            static_cast<HeapSchema*>(m_allocator)->~HeapSchema();
#endif
            g_isSystemSchemaUsed = false;
        }
        else
        {
            azdestroy(m_allocator);
        }
    }
}

//=========================================================================
// Allocate
// [9/2/2009]
//=========================================================================
SystemAllocator::pointer_type
SystemAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    AZ_Assert(byteSize > 0, "You can not allocate 0 bytes!");
    AZ_Assert((alignment & (alignment - 1)) == 0, "Alignment must be power of 2!");

#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records)
    {
        byteSize += m_records->MemoryGuardSize();
    }
#endif // AZCORE_ENABLE_MEMORY_TRACKING

    SystemAllocator::pointer_type address = m_allocator->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord + 1);

    if (address == 0)
    {
        // Free all memory we can and try again!
        AllocatorManager::Instance().GarbageCollect();

        address = m_allocator->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord + 1);
    }

    if (address == 0)
    {
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
        if (m_records)  // restore original size
        {
            byteSize -= m_records->MemoryGuardSize();
        }
#endif // AZCORE_ENABLE_MEMORY_TRACKING

        if (!OnOutOfMemory(byteSize, alignment, flags, name, fileName, lineNum))
        {
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
            if (m_records)
            {
                // print what allocation we have.
                m_records->EnumerateAllocations(Debug::PrintAllocationsCB(false));
            }
#endif // AZCORE_ENABLE_MEMORY_TRACKING
        }
    }

    AZ_Assert(address != 0, "SystemAllocator: Failed to allocate %d bytes aligned on %d (flags: 0x%08x) %s : %s (%d)!", byteSize, alignment, flags, name ? name : "(no name)", fileName ? fileName : "(no file name)", lineNum);

#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records)
    {
#if defined(AZ_HAS_VARIADIC_TEMPLATES) && defined(AZ_DEBUG_BUILD)
        ++suppressStackRecord; // one more for the fact the ebus is a function
#endif // AZ_HAS_VARIADIC_TEMPLATES
        EBUS_EVENT(Debug::MemoryDrillerBus, RegisterAllocation, this, address, byteSize, alignment, name, fileName, lineNum, suppressStackRecord + 1);
    }
#endif // AZCORE_ENABLE_MEMORY_TRACKING

    AZ_PROFILE_MEMORY_ALLOC_EX(AZ::Debug::ProfileCategory::MemoryReserved, fileName, lineNum, address, byteSize, name);

    return address;
}

//=========================================================================
// DeAllocate
// [9/2/2009]
//=========================================================================
void
SystemAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    AZ_PROFILE_MEMORY_FREE(AZ::Debug::ProfileCategory::MemoryReserved, ptr);
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records)
    {
        if (byteSize != 0)
        {
            byteSize += m_records->MemoryGuardSize();
        }
        EBUS_EVENT(Debug::MemoryDrillerBus, UnregisterAllocation, this, ptr, byteSize, alignment, nullptr);
    }
#endif
    m_allocator->DeAllocate(ptr, byteSize, alignment);
}

//=========================================================================
// ReAllocate
// [9/13/2011]
//=========================================================================
SystemAllocator::pointer_type
SystemAllocator::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    Debug::AllocationInfo info;
    info.m_name = "(no name)";
    info.m_fileName = "(no filename)";
    info.m_lineNum = 0;
    if (m_records)
    {
        newSize += m_records->MemoryGuardSize();
        EBUS_EVENT(Debug::MemoryDrillerBus, UnregisterAllocation, this, ptr, 0, 0, &info);
    }
#endif

    pointer_type newAddress = m_allocator->ReAllocate(ptr, newSize, newAlignment);

#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    AZ_Assert(newAddress != 0, "SystemAllocator: Failed to reallocate %p to %d bytes aligned on %d %s : %s (%d)!", ptr, newSize, newAlignment, info.m_name, info.m_fileName, info.m_lineNum);
    if (m_records)
    {
        unsigned int suppressStackRecord = 1;
#if defined(AZ_HAS_VARIADIC_TEMPLATES) && defined(AZ_DEBUG_BUILD)
        ++suppressStackRecord;
#endif // AZ_HAS_VARIADIC_TEMPLATES
        EBUS_EVENT(Debug::MemoryDrillerBus, RegisterAllocation, this, newAddress, newSize, newAlignment, info.m_name, info.m_fileName, info.m_lineNum, suppressStackRecord);
    }
#endif
    return newAddress;
}

//=========================================================================
// Resize
// [8/12/2011]
//=========================================================================
SystemAllocator::size_type
SystemAllocator::Resize(pointer_type ptr, size_type newSize)
{
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records)
    {
        newSize += m_records->MemoryGuardSize();
    }
#endif // AZCORE_ENABLE_MEMORY_TRACKING
    size_type resizedSize = m_allocator->Resize(ptr, newSize);
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records && resizedSize)
    {
        EBUS_EVENT(Debug::MemoryDrillerBus, ResizeAllocation, this, ptr, resizedSize);
        resizedSize -= m_records->MemoryGuardSize();
    }
#endif
    return resizedSize;
}

//=========================================================================
//
// [8/12/2011]
//=========================================================================
SystemAllocator::size_type
SystemAllocator::AllocationSize(pointer_type ptr)
{
    size_type allocSize = m_allocator->AllocationSize(ptr);
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records && allocSize > 0)
    {
        allocSize -= m_records->MemoryGuardSize();
    }
#endif
    return allocSize;
}

#endif // #ifndef AZ_UNITY_BUILD
