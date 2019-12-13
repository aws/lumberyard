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

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/parallel/containers/concurrent_fixed_unordered_set.h>

#include <AzCore/Memory/AllocatorManager.h>

AZ::AllocatorStorage::LazyAllocatorRef::~LazyAllocatorRef()
{
    m_destructor(*m_allocator);
}

void AZ::AllocatorStorage::LazyAllocatorRef::Init(size_t size, size_t alignment, CreationFn creationFn, DestructionFn destructionFn)
{
    m_allocator = AZ::AllocatorManager::CreateLazyAllocator(size, alignment, creationFn);
    m_destructor = destructionFn;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// New overloads
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// In monolithic builds, our new and delete become the global new and delete
// As such, we must support array new/delete
#if defined(AZ_MONOLITHIC_BUILD) || AZ_TRAIT_OS_MEMORY_ALWAYS_NEW_DELETE_SUPPORT_ARRAYS
#define AZ_NEW_DELETE_SUPPORT_ARRAYS
#endif

void* AZ::OperatorNew(std::size_t size, const char* fileName, int lineNum, const char* name)
{
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, name ? name : "global operator aznew", fileName, lineNum);
}
// General new operators
void* AZ::OperatorNew(std::size_t byteSize)
{
    return AllocatorInstance<SystemAllocator>::Get().Allocate(byteSize, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator aznew");
}
void AZ::OperatorDelete(void* ptr)
{
    AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr);
}

void* AZ::OperatorNewArray(std::size_t size, const char* fileName, int lineNum, const char*)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator aznew[]", fileName, lineNum);
#else
    (void)size;
    (void)fileName;
    (void)lineNum;
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                    new[] inserts a header (platform dependent) to keep track of the array size!\n\
                    Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
    return AZ_INVALID_POINTER;
#endif
}
void* AZ::OperatorNewArray(std::size_t size)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    //AZ_Warning("Memory",false,"You should use aznew instead of new! It has better tracking and guarantees you will call the SystemAllocator always!");
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator new[]", nullptr, 0);
#else
    (void)size;
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                    new[] inserts a header (platform dependent) to keep track of the array size!\n\
                    Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
    return AZ_INVALID_POINTER;
#endif
}
void AZ::OperatorDeleteArray(void* ptr)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr);
#else
    (void)ptr;
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                    new[] inserts a header (platform dependent) to keep track of the array size!\n\
                    Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
#endif
}
