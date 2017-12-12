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

#include <aws/core/utils/memory/MemorySystemInterface.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace LmbrAWS
{
    class LmbrAWSMemoryManager
        : public Aws::Utils::Memory::MemorySystemInterface
    {
    public:
        void Begin() override {}
        void End() override {}

        void* AllocateMemory(std::size_t blockSize, std::size_t alignment, const char* allocationTag = nullptr) override
        {
            return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(blockSize, alignment, 0, allocationTag);
        }

        void FreeMemory(void* memoryPtr) override { AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(memoryPtr); }
    };
}