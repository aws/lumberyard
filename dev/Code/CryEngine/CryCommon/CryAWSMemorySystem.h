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

class CryAWSMemorySystem
    : public Aws::Utils::Memory::MemorySystemInterface
{
    void Begin() {};
    void End() {};

    static void* Malloc(std::size_t blockSize, std::size_t alignment, const char* allocationTag = nullptr)
    {
        return malloc(blockSize);
    };

    static void Free(void* memoryPtr)
    {
        free(memoryPtr);
    };

    void* AllocateMemory(std::size_t blockSize, std::size_t alignment, const char* allocationTag = nullptr)
    {
        return Malloc(blockSize, alignment, allocationTag);
    };

    void FreeMemory(void* memoryPtr)
    {
        Free(memoryPtr);
    };
};