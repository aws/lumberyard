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

#include <AzCore/Memory/MallocSchema.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/algorithm.h>

namespace AZ
{
    namespace Internal
    {
        struct Header
        {
            uint32_t offset;
            uint32_t size;
        };
    }
}

#ifdef MALLOC_SCHEMA_USE_AZ_OS_MALLOC
#   define MALLOC_SCHEMA_MALLOC(x)     AZ_OS_MALLOC((x), 0)
#   define MALLOC_SCHEMA_FREE(x)       AZ_OS_FREE(x)
#else
#   define MALLOC_SCHEMA_MALLOC(x)     malloc(x)
#   define MALLOC_SCHEMA_FREE(x)       free(x)
#endif


//---------------------------------------------------------------------
// MallocSchema methods
//---------------------------------------------------------------------

AZ::MallocSchema::MallocSchema(const Descriptor& desc) : 
    m_bytesAllocated(0)
{
    (void)desc;
}

AZ::MallocSchema::~MallocSchema()
{
}

AZ::MallocSchema::pointer_type AZ::MallocSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    (void)flags;
    (void)name;
    (void)fileName;
    (void)lineNum;
    (void)suppressStackRecord;

    if (alignment == 0)
    {
        alignment = sizeof(double);
    }

    AZ_Assert(byteSize < 0x100000000ull, "Malloc allocator only allocates up to 4GB");

    size_type required = byteSize + sizeof(Internal::Header) + ((alignment > sizeof(double)) ? alignment : 0);  // Malloc will align to a minimum boundary for native objects, so we only pad if aligning to a large value
    void* data = MALLOC_SCHEMA_MALLOC(required);
    void* result = PointerAlignUp(reinterpret_cast<void*>(reinterpret_cast<size_t>(data) + sizeof(Internal::Header)), alignment);
    Internal::Header* header = PointerAlignDown<Internal::Header>((Internal::Header*)(reinterpret_cast<size_t>(result) - sizeof(Internal::Header)), AZStd::alignment_of<Internal::Header>::value);

    header->offset = static_cast<uint32_t>(reinterpret_cast<size_type>(result) - reinterpret_cast<size_type>(data));
    header->size = static_cast<uint32_t>(byteSize);
    m_bytesAllocated += byteSize;

    return result;
}

void AZ::MallocSchema::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    (void)byteSize;
    (void)alignment;

    if (!ptr)
    {
        return;
    }

    Internal::Header* header = PointerAlignDown(reinterpret_cast<Internal::Header*>(reinterpret_cast<size_t>(ptr) - sizeof(Internal::Header)), AZStd::alignment_of<Internal::Header>::value);
    void* freePtr = reinterpret_cast<void*>(reinterpret_cast<size_t>(ptr) - static_cast<size_t>(header->offset));

    m_bytesAllocated -= header->size;
    MALLOC_SCHEMA_FREE(freePtr);
}

AZ::MallocSchema::pointer_type AZ::MallocSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    void* newPtr = Allocate(newSize, newAlignment, 0);
    size_t oldSize = AllocationSize(ptr);

    memcpy(newPtr, ptr, AZStd::min(oldSize, newSize));
    DeAllocate(ptr, 0, 0);

    return newPtr;
}

AZ::MallocSchema::size_type AZ::MallocSchema::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;

    return 0;
}

AZ::MallocSchema::size_type AZ::MallocSchema::AllocationSize(pointer_type ptr)
{
    if (ptr)
    {
        Internal::Header* header = PointerAlignDown(reinterpret_cast<Internal::Header*>(reinterpret_cast<size_t>(ptr) - sizeof(Internal::Header)), AZStd::alignment_of<Internal::Header>::value);
        return header->size;
    }
    return 0;
}

AZ::MallocSchema::size_type AZ::MallocSchema::NumAllocatedBytes() const
{
    return m_bytesAllocated;
}

AZ::MallocSchema::size_type AZ::MallocSchema::Capacity() const
{
    return 0;
}

AZ::MallocSchema::size_type AZ::MallocSchema::GetMaxAllocationSize() const
{
    return 0xFFFFFFFFull;
}

AZ::IAllocatorAllocate* AZ::MallocSchema::GetSubAllocator()
{
    return nullptr;
}

void AZ::MallocSchema::GarbageCollect()
{
}
