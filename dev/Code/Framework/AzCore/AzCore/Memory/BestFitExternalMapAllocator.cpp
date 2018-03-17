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

#include <AzCore/Memory/BestFitExternalMapAllocator.h>
#include <AzCore/Memory/BestFitExternalMapSchema.h>

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/MemoryDrillerBus.h>

#include <AzCore/std/functional.h>

using namespace AZ;

//=========================================================================
// BestFitExternalMapAllocator
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::BestFitExternalMapAllocator()
    : m_schema(NULL)
{}

//=========================================================================
// Create
// [1/28/2011]
//=========================================================================
bool
BestFitExternalMapAllocator::Create(const Descriptor& desc)
{
    AZ_Assert(IsReady() == false, "BestFitExternalMapAllocator was already created!");
    if (IsReady())
    {
        return false;
    }

    bool isReady = true;

    BestFitExternalMapSchema::Descriptor schemaDesc;
    schemaDesc.m_mapAllocator = desc.m_mapAllocator;
    schemaDesc.m_memoryBlock = desc.m_memoryBlock;
    schemaDesc.m_memoryBlockByteSize = desc.m_memoryBlockByteSize;

    m_schema = azcreate(BestFitExternalMapSchema, (schemaDesc), SystemAllocator);
    if (m_schema == NULL)
    {
        isReady = false;
    }

#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (isReady && desc.m_allocationRecords)
    {
        EBUS_EVENT(Debug::MemoryDrillerBus, RegisterAllocator, this, desc.m_stackRecordLevels, false, false);
    }
#endif
    if (isReady)
    {
        OnCreated();
    }

    return IsReady();
}

//=========================================================================
// Destroy
// [1/28/2011]
//=========================================================================
void
BestFitExternalMapAllocator::Destroy()
{
    OnDestroy();

    azdestroy(m_schema, SystemAllocator);
    m_schema = NULL;
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records)
    {
        EBUS_EVENT(Debug::MemoryDrillerBus, UnregisterAllocator, this);
    }
#endif
}

//=========================================================================
// Allocate
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::pointer_type
BestFitExternalMapAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    AZ_Assert(byteSize > 0, "You can not allocate 0 bytes!");
    AZ_Assert((alignment & (alignment - 1)) == 0, "Alignment must be power of 2!");

    BestFitExternalMapAllocator::pointer_type address = m_schema->Allocate(byteSize, alignment, flags);
    if (address == 0)
    {
        if (!OnOutOfMemory(byteSize, alignment, flags, name, fileName, lineNum))
        {
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
            if (m_records)
            {
                // print what allocation we have.
                m_records->EnumerateAllocations(Debug::PrintAllocationsCB(false));
            }
#endif
        }
    }

    AZ_Assert(address != 0, "BestFitExternalMapAllocator: Failed to allocate %d bytes aligned on %d (flags: 0x%08x) %s : %s (%d)!", byteSize, alignment, flags, name ? name : "(no name)", fileName ? fileName : "(no file name)", lineNum);
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records)
    {
#if defined(AZ_HAS_VARIADIC_TEMPLATES) && defined(AZ_DEBUG_BUILD)
        ++suppressStackRecord; // one more for the fact the ebus is a function
#endif // AZ_HAS_VARIADIC_TEMPLATES
        EBUS_EVENT(Debug::MemoryDrillerBus, RegisterAllocation, this, address, byteSize, alignment, name, fileName, lineNum, suppressStackRecord + 1);
    }
#else
    (void)name;
    (void)fileName;
    (void)lineNum;
    (void)suppressStackRecord;
#endif
    return address;
}

//=========================================================================
// DeAllocate
// [1/28/2011]
//=========================================================================
void
BestFitExternalMapAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
    if (m_records)
    {
        EBUS_EVENT(Debug::MemoryDrillerBus, UnregisterAllocation, this, ptr, byteSize, alignment, nullptr);
    }
#endif
    (void)byteSize;
    (void)alignment;
    m_schema->DeAllocate(ptr);
}

//=========================================================================
// Resize
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;
    /* todo */
    return 0;
}

//=========================================================================
// ReAllocate
// [9/13/2011]
//=========================================================================
BestFitExternalMapAllocator::pointer_type
BestFitExternalMapAllocator::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    (void)ptr;
    (void)newSize;
    (void)newAlignment;
    AZ_Assert(false, "Not supported!");
    return NULL;
}

//=========================================================================
// AllocationSize
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::AllocationSize(pointer_type ptr)
{
    return m_schema->AllocationSize(ptr);
}

//=========================================================================
// NumAllocatedBytes
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::NumAllocatedBytes() const
{
    return m_schema->NumAllocatedBytes();
}

//=========================================================================
// Capacity
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::Capacity() const
{
    return m_schema->Capacity();
}

//=========================================================================
// GetMaxAllocationSize
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::GetMaxAllocationSize() const
{
    return m_schema->GetMaxAllocationSize();
}

//=========================================================================
// GetSubAllocator
// [1/28/2011]
//=========================================================================
IAllocatorAllocate*
BestFitExternalMapAllocator::GetSubAllocator()
{
    return m_schema->GetSubAllocator();
}

#endif // #ifndef AZ_UNITY_BUILD
