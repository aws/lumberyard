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

#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Debug/StackTracer.h>

namespace AZ
{
    namespace Debug
    {
        //=========================================================================
        // MemoryDriller
        // [2/6/2013]
        //=========================================================================
        MemoryDriller::MemoryDriller(const Descriptor& desc)
        {
            (void)desc;
            BusConnect();
        }

        //=========================================================================
        // ~MemoryDriller
        // [2/6/2013]
        //=========================================================================
        MemoryDriller::~MemoryDriller()
        {
            BusDisconnect();
        }

        //=========================================================================
        // Start
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;

            // dump current allocations for all allocators with tracking
            for (int i = 0; i < AllocatorManager::Instance().GetNumAllocators(); ++i)
            {
                IAllocator* allocator = AllocatorManager::Instance().GetAllocator(i);
                if (allocator->m_records)
                {
                    RegisterAllocatorOutput(allocator);
                    const AllocationRecordsType& allocMap = allocator->m_records->GetMap();
                    for (AllocationRecordsType::const_iterator allocIt = allocMap.begin(); allocIt != allocMap.end(); ++allocIt)
                    {
                        RegisterAllocationOutput(allocator, allocIt->first, &allocIt->second);
                    }
                }
            }
        }

        //=========================================================================
        // Stop
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::Stop()
        {
        }

        //=========================================================================
        // RegisterAllocator
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::RegisterAllocator(IAllocator* allocator, unsigned char stackRecordLevels, bool isMemoryGuard, bool isMarkUnallocatedMemory)
        {
            AZ_Assert(allocator->m_records == NULL, "This allocator is already registered with the memory driller!");
            allocator->m_records = aznew Debug::AllocationRecords(stackRecordLevels, isMemoryGuard, isMarkUnallocatedMemory);

            if (m_output == NULL)
            {
                return;                    // we have no active output
            }
            RegisterAllocatorOutput(allocator);
        }

        //=========================================================================
        // RegisterAllocatorOutput
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::RegisterAllocatorOutput(IAllocator* allocator)
        {
            m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            m_output->BeginTag(AZ_CRC("RegisterAllocator", 0x19f08114));
            m_output->Write(AZ_CRC("Name", 0x5e237e06), allocator->GetName());
            m_output->Write(AZ_CRC("Id", 0xbf396750), allocator);
            m_output->Write(AZ_CRC("Capacity", 0xb5e8b174), allocator->Capacity());
            m_output->Write(AZ_CRC("RecordsId", 0x7caaca88), allocator->m_records);
            if (allocator->m_records)
            {
                m_output->Write(AZ_CRC("RecordsMode", 0x764c147a), (char)allocator->m_records->GetMode());
                m_output->Write(AZ_CRC("NumStackLevels", 0xad9cff15), allocator->m_records->GetNumStackLevels());
            }
            m_output->EndTag(AZ_CRC("RegisterAllocator", 0x19f08114));
            m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
        }

        //=========================================================================
        // UnregisterAllocator
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::UnregisterAllocator(IAllocator* allocator)
        {
            AZ_Assert(allocator->m_records != NULL, "This allocator is not registered with the memory driller!");
            delete allocator->m_records;
            allocator->m_records = NULL;

            if (m_output == NULL)
            {
                return;                    // we have no active output
            }
            m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            m_output->Write(AZ_CRC("UnregisterAllocator", 0xb2b54f93), allocator);
            m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
        }

        //=========================================================================
        // RegisterAllocation
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::RegisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount)
        {
            AZ_Assert(allocator->m_records != NULL, "This allocator is not registered with the memory driller!");
            const AllocationInfo* info = allocator->m_records->RegisterAllocation(address, byteSize, alignment, name, fileName, lineNum, stackSuppressCount + 1);

            if (m_output == NULL)
            {
                return;                   // we have no active output
            }
            RegisterAllocationOutput(allocator, address, info);
        }

        //=========================================================================
        // RegisterAllocationOutput
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::RegisterAllocationOutput(IAllocator* allocator, void* address, const AllocationInfo* info)
        {
            m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            m_output->BeginTag(AZ_CRC("RegisterAllocation", 0x992a9780));
            m_output->Write(AZ_CRC("RecordsId", 0x7caaca88), allocator->m_records);
            m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
            if (info)
            {
                if (info->m_name)
                {
                    m_output->Write(AZ_CRC("Name", 0x5e237e06), info->m_name);
                }
                m_output->Write(AZ_CRC("Alignment", 0x2cce1e5c), info->m_alignment);
                m_output->Write(AZ_CRC("Size", 0xf7c0246a), info->m_byteSize);
                if (info->m_fileName)
                {
                    m_output->Write(AZ_CRC("FileName", 0x3c0be965), info->m_fileName);
                    m_output->Write(AZ_CRC("FileLine", 0xb33c2395), info->m_lineNum);
                }
                // copy the stack frames directly, resolving the stack should happen later as this is a SLOW procedure.
                if (info->m_stackFrames)
                {
                    m_output->Write(AZ_CRC("Stack", 0x41a87b6a), info->m_stackFrames, info->m_stackFrames + allocator->m_records->GetNumStackLevels());
                }
            }
            m_output->EndTag(AZ_CRC("RegisterAllocation", 0x992a9780));
            m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
        }

        //=========================================================================
        // UnRegisterAllocation
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::UnregisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AllocationInfo* info)
        {
            AZ_Assert(allocator->m_records != NULL, "This allocator is not registered with the memory driller!");
            allocator->m_records->UnregisterAllocation(address, byteSize, alignment, info);

            if (m_output == NULL)
            {
                return;                    // we have no active output
            }
            m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            m_output->BeginTag(AZ_CRC("UnRegisterAllocation", 0xea5dc4cd));
            m_output->Write(AZ_CRC("RecordsId", 0x7caaca88), allocator->m_records);
            m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
            m_output->EndTag(AZ_CRC("UnRegisterAllocation", 0xea5dc4cd));
            m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
        }

        //=========================================================================
        // ResizeAllocation
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::ResizeAllocation(IAllocator* allocator, void* address, size_t newSize)
        {
            AZ_Assert(allocator->m_records != NULL, "This allocator is not registered with the memory driller!");
            allocator->m_records->ResizeAllocation(address, newSize);

            if (m_output == NULL)
            {
                return;                    // we have no active output
            }
            m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            m_output->BeginTag(AZ_CRC("ResizeAllocation", 0x8a9c78dc));
            m_output->Write(AZ_CRC("RecordsId", 0x7caaca88), allocator->m_records);
            m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), newSize);
            m_output->EndTag(AZ_CRC("ResizeAllocation", 0x8a9c78dc));
            m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
        }
    }// namespace Debug
} // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
