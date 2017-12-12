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
#ifndef AZCORE_UNITTEST_USERTYPES_H
#define AZCORE_UNITTEST_USERTYPES_H

#include <AzCore/base.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Memory/AllocationRecords.h>

namespace UnitTest
{
    class AllocatorsFixture
        : public ::testing::Test
    {
        void* m_memBlock;
        AZ::Debug::DrillerManager* m_drillerManager;
    public:
        AllocatorsFixture(unsigned int heapSizeMB = 15, bool isMemoryDriller = true)
        {
            if (isMemoryDriller)
            {
                m_drillerManager = AZ::Debug::DrillerManager::Create();
                m_drillerManager->Register(aznew AZ::Debug::MemoryDriller);
            }
            else
            {
                m_drillerManager = nullptr;
            }

            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_numMemoryBlocks = 1;
            desc.m_heap.m_memoryBlocksByteSize[0] = heapSizeMB * 1024 * 1024;
            m_memBlock = DebugAlignAlloc(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            desc.m_heap.m_memoryBlocks[0] = m_memBlock;

            desc.m_allocationRecords = true;
            desc.m_stackRecordLevels = 10;

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
            AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords();
            if (records)
            {
                records->SetMode(AZ::Debug::AllocationRecords::Mode::RECORD_FULL);
                records->SetSaveNames(true);
            }
        }

        virtual ~AllocatorsFixture()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            DebugAlignFree(m_memBlock);

            if (m_drillerManager)
            {
                AZ::Debug::DrillerManager::Destroy(m_drillerManager);
            }
        }
    };
}

#endif // AZCORE_UNITTEST_USERTYPES_H
