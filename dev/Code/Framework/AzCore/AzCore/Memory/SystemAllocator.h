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
#ifndef AZCORE_SYS_ALLOCATOR_H
#define AZCORE_SYS_ALLOCATOR_H

#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class HphaSchema;
    class HeapSchema;

    /**
     * System allocator
     * The system allocator is the base allocator for
     * AZ memory lib. It is a singleton (like all other allocators), but
     * must be initialized first and destroyed last. All other allocators
     * will use them for internal allocations. This doesn't mean all other allocators
     * will be sub allocators, because we might have different memory system on consoles.
     * But the allocator utility system will use the system allocator.
     */
    class SystemAllocator
        : public IAllocator
    {
    public:
        AZ_TYPE_INFO(SystemAllocator, "{424C94D8-85CF-4E89-8CD6-AB5EC173E875}")

        SystemAllocator();
        ~SystemAllocator();

        /**
         * Description - configure the system allocator. By default
         * we will allocate system memory using system calls. You can
         * provide arenas (spaces) with pre-allocated memory, and use the
         * flag to specify which arena you want to allocate from.
         * You are also allowed to supply IAllocatorAllocate, but if you do
         * so you will need to take care of all allocations, we will not use
         * the default HeapSchema.
         * \ref HeapSchema::Descriptor
         */
        struct Descriptor
        {
            Descriptor()
                : m_custom(0)
                , m_allocationRecords(true)
                , m_stackRecordLevels(5)
            {}
            IAllocatorAllocate*         m_custom;   ///< You can provide our own allocation scheme. If NULL a HeapScheme will be used with the provided Descriptor.

            struct Heap
            {
                Heap()
                    : m_pageSize(m_defaultPageSize)
                    , m_poolPageSize(m_defaultPoolPageSize)
                    , m_isPoolAllocations(true)
                    , m_numMemoryBlocks(0)
                    , m_subAllocator(0)
                {}
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE) // ACCEPTED_USE
                static const int        m_defaultPageSize = 64 * 1024;
#else
                static const int        m_defaultPageSize = 4 * 1024;
#endif
                static const int        m_defaultPoolPageSize = 4 * 1024;
                static const int        m_memoryBlockAlignment = m_defaultPageSize;
                static const int        m_maxNumBlocks = 3;
                unsigned int            m_pageSize;                                 ///< Page allocation size must be 1024 bytes aligned. (default m_defaultPageSize)
                unsigned int            m_poolPageSize;                             ///< Page size used to small memory allocations. Must be less or equal to m_pageSize and a multiple of it. (default m_defaultPoolPageSize)
                bool                    m_isPoolAllocations;                        ///< True (default) if we use pool for small allocations (< 256 bytes), otherwise false. IMPORTANT: Changing this to false will degrade performance!
                int                     m_numMemoryBlocks;                          ///< Number of memory blocks to use.
                void*                   m_memoryBlocks[m_maxNumBlocks];             ///< Pointers to provided memory blocks or NULL if you want the system to allocate them for you with the System Allocator.
                size_t                  m_memoryBlocksByteSize[m_maxNumBlocks];     ///< Sizes of different memory blocks (MUST be multiple of m_pageSize), if m_memoryBlock is 0 the block will be allocated for you with the System Allocator.
                IAllocatorAllocate*     m_subAllocator;                             ///< Allocator that m_memoryBlocks memory was allocated from or should be allocated (if NULL).
            }                           m_heap;
            bool                        m_allocationRecords;    ///< True if we want to track memory allocations, otherwise false.
            unsigned char               m_stackRecordLevels;    ///< If stack recording is enabled, how many stack levels to record.
        };

        bool Create(const Descriptor& desc);

        void Destroy();

        //////////////////////////////////////////////////////////////////////////
        // IAllocator

        virtual const char*     GetName() const                 { return "SystemAllocator"; }
        virtual const char*     GetDescription() const          { return "Fundamental generic memory allocator"; }

        virtual pointer_type    Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0);
        virtual void            DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0);
        virtual pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment);
        virtual size_type       Resize(pointer_type ptr, size_type newSize);
        virtual size_type       AllocationSize(pointer_type ptr);
        virtual void            GarbageCollect()                 { m_allocator->GarbageCollect(); }

        virtual size_type       NumAllocatedBytes() const       { return m_allocator->NumAllocatedBytes(); }
        virtual size_type       Capacity() const                { return m_allocator->Capacity(); }
        /// Keep in mind this operation will execute GarbageCollect to make sure it returns, max allocation. This function WILL be slow.
        virtual size_type       GetMaxAllocationSize() const    { return m_allocator->GetMaxAllocationSize(); }
        virtual size_type       GetUnAllocatedMemory(bool isPrint = false) const    { return m_allocator->GetUnAllocatedMemory(isPrint); }
        virtual IAllocatorAllocate*  GetSubAllocator()          { return m_isCustom ? m_allocator : m_allocator->GetSubAllocator(); }
        //////////////////////////////////////////////////////////////////////////

    protected:
        SystemAllocator(const SystemAllocator&);
        SystemAllocator& operator=(const SystemAllocator&);

        bool                        m_isCustom;
        IAllocatorAllocate*         m_allocator;
    };
}

#endif // AZCORE_SYS_ALLOCATOR_H
#pragma once


