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
#ifndef AZCORE_POOL_ALLOCATOR_H
#define AZCORE_POOL_ALLOCATOR_H

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolSchema.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Memory/MemoryDrillerBus.h>


namespace AZ
{
    template<class Allocator>
    class PoolAllocation;
    namespace Internal
    {
        /*!
        * Template you can use to create your own thread pool allocators, as you can't inherit from ThreadPoolAllocator.
        * This is the case because we use tread local storage and we need separate "static" instance for each allocator.
        */
        template<class Schema>
        class PoolAllocatorHelper
            : public IAllocator
        {
        public:
            struct Descriptor
                : public Schema::Descriptor
            {
                Descriptor()
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
                    : m_allocationRecords(true)
                    , m_isMarkUnallocatedMemory(true)
#else
                    : m_allocationRecords(false)
                    , m_isMarkUnallocatedMemory(false)
#endif
                    , m_isMemoryGuards(false)
                    , m_stackRecordLevels(5)
                {}

                bool m_allocationRecords; ///< True if we want to track memory allocations, otherwise false.
                bool m_isMarkUnallocatedMemory; ///< Sets unallocated memory to default values (if m_allocationRecords is true)
                bool m_isMemoryGuards; ///< Enables memory guards for stomp detection. Keep in mind that this will change the memory "profile" a lot as we usually double every pool allocation.
                unsigned char m_stackRecordLevels; ///< If stack recording is enabled, how many stack levels to record. (if m_allocationRecords is true)
            };


            bool Create(const Descriptor& descriptor)
            {
                AZ_Assert(IsReady() == false, "Allocator was already created!");
                if (IsReady())
                {
                    return false;
                }

                Descriptor desc = descriptor;

                if (desc.m_minAllocationSize < 8)
                {
                    desc.m_minAllocationSize = 8;
                }
                if (desc.m_maxAllocationSize < desc.m_minAllocationSize)
                {
                    desc.m_maxAllocationSize = desc.m_minAllocationSize;
                }

#ifdef AZCORE_ENABLE_MEMORY_TRACKING
                if (desc.m_allocationRecords && desc.m_isMemoryGuards)
                {
                    desc.m_maxAllocationSize = AZ_SIZE_ALIGN_UP(desc.m_maxAllocationSize + AZ_SIZE_ALIGN_UP(sizeof(Debug::GuardValue), desc.m_minAllocationSize), desc.m_minAllocationSize);
                }
#endif // AZCORE_ENABLE_MEMORY_TRACKING

                bool isReady = m_schema.Create(desc);

#ifdef AZCORE_ENABLE_MEMORY_TRACKING
                if (isReady && desc.m_allocationRecords)
                {
                    EBUS_EVENT(Debug::MemoryDrillerBus, RegisterAllocator, this, desc.m_stackRecordLevels, desc.m_isMemoryGuards, desc.m_isMarkUnallocatedMemory);
                }
#endif
                if (isReady)
                {
                    OnCreated();
                }

                return IsReady();
            }

            void Destroy()
            {
                OnDestroy();
                m_schema.Destroy();
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
                if (m_records)
                {
                    EBUS_EVENT(Debug::MemoryDrillerBus, UnregisterAllocator, this);
                }
#endif
            }

            //////////////////////////////////////////////////////////////////////////
            // IAllocator
            pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
            {
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
                if (m_records)
                {
                    byteSize += m_records->MemoryGuardSize();
                }
#endif // AZCORE_ENABLE_MEMORY_TRACKING

                pointer_type address = m_schema.Allocate(byteSize, alignment, flags);
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

            void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override
            {
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
                (void)byteSize;
                (void)alignment;
                m_schema.DeAllocate(ptr);
            }

            pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
            {
                (void)ptr;
                (void)newSize;
                (void)newAlignment;
                AZ_Assert(false, "Not supported!");
                return nullptr;
            }

            size_type Resize(pointer_type ptr, size_type newSize) override
            {
                (void)ptr;
                (void)newSize;
                // \todo return the node size
                return 0;
            }

            size_type AllocationSize(pointer_type ptr) override
            {
                size_type allocSize = m_schema.AllocationSize(ptr);
#ifdef AZCORE_ENABLE_MEMORY_TRACKING
                if (m_records && allocSize > 0)
                {
                    allocSize -= m_records->MemoryGuardSize();
                }
#endif
                return allocSize;
            }

            /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
            size_t NumAllocatedBytes() const override
            {
                return m_schema.NumAllocatedBytes();
            }

            size_t Capacity() const override
            {
                return m_schema.Capacity();
            }

            void GarbageCollect() override
            {
                m_schema.GarbageCollect();
            }

            IAllocatorAllocate* GetSubAllocator() override
            {
                return m_schema.GetPageAllocator();
            }
            //////////////////////////////////////////////////////////////////////////

        protected:
            PoolAllocatorHelper& operator=(const PoolAllocatorHelper&);
            Schema m_schema;
        };
    }

    /*!
     * Pool allocator
     * Specialized allocation for extremely fast small object memory allocations.
     * It can allocate sized from m_minAllocationSize to m_maxPoolSize.
     * Pool Allocator is NOT thread safe, if you if need a thread safe version
     * use PoolAllocatorThreadSafe or do the sync yourself.
     */
    class PoolAllocator
        : public Internal::PoolAllocatorHelper<PoolSchema>
    {
    public:
        AZ_CLASS_ALLOCATOR(PoolAllocator, SystemAllocator, 0)
        AZ_TYPE_INFO(PoolAllocator, "{D3DC61AF-0949-4BFA-87E0-62FA03A4C025}")

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        virtual const char*     GetName() const
        {
            return "PoolAllocator";
        }
        virtual const char*     GetDescription() const
        {
            return "Generic pool allocator for small objects";
        }
    };

    template<class Allocator>
    using ThreadPoolBase = Internal::PoolAllocatorHelper<ThreadPoolSchemaHelper<Allocator> >;

    /*!
     * Thread safe pool allocator. If you want to create your own thread pool heap,
     * inherit from ThreadPoolBase, as we need unique static variable for allocator type.
     */
    class ThreadPoolAllocator final
        : public ThreadPoolBase<ThreadPoolAllocator>
    {
    public:
        AZ_CLASS_ALLOCATOR(ThreadPoolAllocator, SystemAllocator, 0)
        AZ_TYPE_INFO(ThreadPoolAllocator, "{05B4857F-CD06-4942-99FD-CA6A7BAE855A}")

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        const char* GetName() const override
        {
            return "PoolAllocatorThreadSafe";
        }

        const char* GetDescription() const override
        {
            return "Generic thread safe pool allocator for small objects";
        }
        //////////////////////////////////////////////////////////////////////////
    };
}

#endif // AZCORE_POOL_ALLOCATOR_H
#pragma once


