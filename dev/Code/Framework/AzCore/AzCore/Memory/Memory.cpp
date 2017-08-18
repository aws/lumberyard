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

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// New overloads
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void* AZ::OperatorNew(std::size_t size, const char* fileName, int lineNum)
{
    AZ_Warning("Memory", false, "We highly recommend using the AZ_CLASS_ALLOCATOR inside you class struct, to define the proper allocation! For now we use default new, which you can override!");
    return AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator aznew", fileName, lineNum);
}
// General new operators
void* AZ::OperatorNew(std::size_t byteSize)
{
    AZ_Warning("Memory", false, "You should use aznew instead of new! It has better tracking and guarantees you will call the SystemAllocator always!");
    return AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(byteSize, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "operator new", 0, 0);
}
void AZ::OperatorDelete(void* ptr)
{
    AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr);
}

void* AZ::OperatorNewArray(std::size_t size, const char* fileName, int lineNum)
{
    (void)size;
    (void)fileName;
    (void)lineNum;
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                    new[] inserts a header (platform dependent) to keep track of the array size!\n\
                    Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!"); \
    return AZ_INVALID_POINTER;
    //or enable allocations if you don't care about alignment
    //return AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(size,MY_MEMORY_DEFAULT_ALIGNMENT,0,"global operator aznew[]",fileName,lineNum);
}
void* AZ::OperatorNewArray(std::size_t byteSize)
{
    (void)byteSize;
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                    new[] inserts a header (platform dependent) to keep track of the array size!\n\
                    Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!"); \
    return AZ_INVALID_POINTER;
    //or enable allocations if you don't care about alignment
    //AZ_Warning("Memory",false,"You should use aznew instead of new! It has better tracking and guarantees you will call the SystemAllocator always!");
    //return AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(byteSize,AZCORE_GLOBAL_NEW_ALIGNMENT,0,"operator new[]",0,0);
}
void AZ::OperatorDeleteArray(void* ptr)
{
    (void)ptr;
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                    new[] inserts a header (platform dependent) to keep track of the array size!\n\
                    Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!"); \
    //or enable allocations if you don't care about alignment
    //AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr);
}

#endif // #ifndef AZ_UNITY_BUILD