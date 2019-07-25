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
#ifndef AZCORE_ALLOCATOR_BASE_H
#define AZCORE_ALLOCATOR_BASE_H 1

#include <AzCore/base.h>
#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/Debug/Profiler.h>

namespace AZ
{
    namespace Debug
    {
        class AllocationRecords;
        class MemoryDriller;
    }
    /**
     * Allocator alloc/free basic interface. It is separate because it can be used
     * for user provided allocators overrides
     */
    class IAllocatorAllocate
    {
    public:
        typedef void*           pointer_type;
        typedef size_t          size_type;
        typedef ptrdiff_t       difference_type;

        virtual ~IAllocatorAllocate() {}

        virtual pointer_type            Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) = 0;
        virtual void                    DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) = 0;
        /// Resize an allocated memory block. Returns the new adjusted size (as close as possible or equal to the requested one) or 0 (if you don't support resize at all).
        virtual size_type               Resize(pointer_type ptr, size_type newSize) = 0;
        /// Realloc an allocate memory memory block. Similar to Resize except it will move the memory block if needed. Return NULL if realloc is not supported or run out of memory.
        virtual pointer_type            ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) = 0;
        ///
        /// Returns allocation size for given address. 0 if the address doesn't belong to the allocator.
        virtual size_type               AllocationSize(pointer_type ptr) = 0;
        /**
         * Call from the system when we want allocators to free unused memory.
         * IMPORTANT: This function is/should be thread safe. We can call it from any context to free memory.
         * Freeing the actual memory is optional (if you can), thread safety is a must.
         */
        virtual void                    GarbageCollect()    {}

        virtual size_type               NumAllocatedBytes() const = 0;
        /// Returns the capacity of the Allocator in bytes. If the return value is 0 the Capacity is undefined (usually depends on another allocator)
        virtual size_type               Capacity() const    = 0;
        /// Returns max allocation size if possible. If not returned value is 0
        virtual size_type               GetMaxAllocationSize() const { return 0; }
        /**
         * Returns memory allocated by the allocator and available to the user for allocations.
         * IMPORTANT: this is not the overhead memory this is just the memory that is allocated, but not used. Example: the pool allocators
         * allocate in chunks. So we might be using one elements in that chunk and the rest is free/unallocated. This is the memory
         * that will be reported.
         */
        virtual size_type               GetUnAllocatedMemory(bool isPrint = false) const { (void)isPrint; return 0; }
        /// Returns a pointer to a sub-allocator or NULL.
        virtual IAllocatorAllocate*     GetSubAllocator() = 0;
    };

    /**
     * Interface class for all allocators.
     */
    class IAllocator
        : public IAllocatorAllocate
    {
        friend class Debug::MemoryDriller;
    public:
        // @{ IAlloctor will be registered in the AllocatorManager on construction and removed on destruction.
        IAllocator();
        virtual ~IAllocator();
        // @}

        // @{ Every system allocator is required to provide name this is how
        // it will be registered with the allocator manager.
        virtual const char*         GetName() const = 0;
        virtual const char*         GetDescription() const = 0;
        // @}

        /// Returns a pointer to the allocation records. They might be available or not depending on the build type. \ref Debug::AllocationRecords
        Debug::AllocationRecords*   GetRecords()    { return m_records; }

        bool                        IsReady() const { return m_isReady; }

    protected:
        /// Called when an allocator is created and we want to register in the system.
        void                        OnCreated();
        /// Called when an allocator is destroyed and we want to unregister from the the system.
        void                        OnDestroy();

        /// User allocator should call this function when they run out of memory!
        bool OnOutOfMemory(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum);

        Debug::AllocationRecords*   m_records;      ///< Cached pointer to allocation records. Works together with the MemoryDriller.

    private:

        bool                        m_isReady;
    };

    namespace Internal  {
        struct AllocatorDummy{};
    }

    template <class SchemaType, class DescriptorType = typename SchemaType::Descriptor>
    class AllocatorBase
        : public IAllocator
    {
    public:
        using Descriptor = DescriptorType;
        using Schema = SchemaType;

        AllocatorBase(const char* name, const char* desc)
            : m_schema(nullptr)
            , m_name(name)
            , m_desc(desc)
        {
        }

        virtual ~AllocatorBase()
        {
            if (m_schema)
            {
                m_schema->~Schema();
            }
            m_schema = nullptr;
        }

        bool Create(const Descriptor& desc = Descriptor())
        {
            m_schema = new (&m_schemaStorage) Schema(desc);
            OnCreated();
            return m_schema != nullptr;
        }

        void Destroy()
        {
            OnDestroy();
            m_schema->~Schema();
            m_schema = nullptr;
        }

        //---------------------------------------------------------------------
        // IAllocator
        //---------------------------------------------------------------------
        const char* GetName() const override
        {
            return m_name;
        }

        const char* GetDescription() const override
        {
            return m_desc;
        }

        //---------------------------------------------------------------------
        // IAllocatorAllocate
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
        {
            pointer_type ptr = m_schema->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
            AZ_PROFILE_MEMORY_ALLOC_EX(AZ::Debug::ProfileCategory::MemoryReserved, fileName, lineNum, ptr, byteSize, name ? name : GetName());
            return ptr;
        }

        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            AZ_PROFILE_MEMORY_FREE(AZ::Debug::ProfileCategory::MemoryReserved, ptr);
            m_schema->DeAllocate(ptr, byteSize, alignment);
        }

        size_type Resize(pointer_type ptr, size_type newSize) override
        {
            return m_schema->Resize(ptr, newSize);
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
        {
            AZ_PROFILE_MEMORY_FREE(AZ::Debug::ProfileCategory::MemoryReserved, ptr);
            pointer_type newPtr = m_schema->ReAllocate(ptr, newSize, newAlignment);
            AZ_PROFILE_MEMORY_ALLOC(AZ::Debug::ProfileCategory::MemoryReserved, newPtr, newSize, GetName());
            return newPtr;
        }
                
        size_type AllocationSize(pointer_type ptr) override
        {
            return m_schema->AllocationSize(ptr);
        }
        
        void GarbageCollect() override
        {
            m_schema->GarbageCollect();
        }

        size_type NumAllocatedBytes() const override
        {
            return m_schema->NumAllocatedBytes();
        }

        size_type Capacity() const override
        {
            return m_schema->Capacity();
        }
        
        size_type GetMaxAllocationSize() const override
        { 
            return m_schema->GetMaxAllocationSize();
        }

        size_type               GetUnAllocatedMemory(bool isPrint = false) const override
        { 
            return m_schema->GetUnAllocatedMemory(isPrint);
        }
        
        virtual IAllocatorAllocate*     GetSubAllocator()
        {
            return m_schema->GetSubAllocator();
        }

    protected:
        Schema* m_schema;
        const char* m_name;
        const char* m_desc;
        typename AZStd::aligned_storage<sizeof(Schema), AZStd::alignment_of<Schema>::value>::type m_schemaStorage;
    };
}

#endif // AZCORE_ALLOCATOR_BASE_H
#pragma once


