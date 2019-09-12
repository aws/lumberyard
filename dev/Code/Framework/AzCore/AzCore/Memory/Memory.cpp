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

// On dlopen/unix platforms, our new/delete can get put into the dynamic symbol table at any time
// As such, the OS fallback is required, because we cannot guarantee that our allocators are up
#if defined(AZ_MONOLITHIC_BUILD) || AZ_TRAIT_OS_MEMORY_ALWAYS_USE_OS_FALLBACK
#define AZ_MEMORY_USE_OS_FALLBACK
#endif

namespace AZ 
{
    namespace Internal
    {
        bool IsSystemAlloc(void* ptr)
        {
            if (AllocatorInstance<SystemAllocator>::IsReady())
            {
                return AllocatorInstance<SystemAllocator>::Get().AllocationSize(ptr) > 0;
            }
            return false;
        }

        void* SystemAlloc(size_t size, size_t alignment, const char* fileName, int lineNum, const char* name)
        {
            return AllocatorInstance<SystemAllocator>::Get().Allocate(size, alignment, 0, name, fileName, lineNum);
        }

        void SystemFree(void* ptr)
        {
            AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr);
        }

#if defined(AZ_MEMORY_USE_OS_FALLBACK)
        // This must be wrapped in a function to ensure initialization when it is accessed
        using AllocSet = AZStd::concurrent_fixed_unordered_set<void*, 1023, 16 * 1024>;
        AllocSet& GetOSAllocations()
        {
            static AllocSet s_osAllocs;
            return s_osAllocs;
        }

        bool IsMonolithicAlloc(void* ptr)
        {
            const AllocSet& allocs = GetOSAllocations();
            if (allocs.find(ptr))
            {
                return true;
            }

            return IsSystemAlloc(ptr);
        }

        void* MonolithicAlloc(size_t size, size_t alignment, const char* fileName, int lineNum, const char* name)
        {
            if (AllocatorInstance<SystemAllocator>::IsReady())
            {
                return SystemAlloc(size, alignment, fileName, lineNum, name);
            }

            AZ_PROFILE_MEMORY_ALLOC(AZ::Debug::ProfileCategory::Global, fileName, lineNum, name);
            void* ptr = AZ_OS_MALLOC(size, alignment);
            AllocSet& allocs = GetOSAllocations();
            allocs.insert(ptr);
            return ptr;
        }

        void MonolithicFree(void* ptr)
        {
            if (ptr == nullptr)
            {
                return;
            }

            // Erase returns the number erased. To avoid 2 lookups (find then erase), we just see if it was erased
            // It is also vital to erase before freeing, because otherwise there can be a collision with another thread
            // which allocs the same address
            AllocSet& allocs = GetOSAllocations();
            if (allocs.erase(ptr) > 0)
            {
                AZ_PROFILE_MEMORY_FREE(AZ::Debug::ProfileCategory::Global, ptr);
                AZ_OS_FREE(ptr);
                return;
            }
            if (AllocatorInstance<SystemAllocator>::IsReady())
            {
                SystemFree(ptr);
                return;
            }
            // LegacyAllocator memory can end up here, as well as lifetime management mistakes of memory pulled from any other allocator
            // including the SystemAllocator. This will always happen at shutdown, so we prefer being robust instead of crashing.
            AZ_Warning("Memory", false, "Unknown memory freed: The memory belonged to an Allocator, but was freed after the Allocator was destroyed. This memory will leak! addr=0x%x", ptr);
        }
#endif
        bool IsGlobalAlloc(void* ptr)
        {
#if defined(AZ_MEMORY_USE_OS_FALLBACK)
            return IsMonolithicAlloc(ptr);
#else
            return IsSystemAlloc(ptr);
#endif
        }
        void* GlobalAlloc(size_t size, size_t alignment, const char* fileName, int lineNum, const char* name)
        {
#if defined(AZ_MEMORY_USE_OS_FALLBACK)
            return MonolithicAlloc(size, alignment, fileName, lineNum, name);
#else
            return SystemAlloc(size, alignment, fileName, lineNum, name);
#endif
        }

        void GlobalFree(void* ptr)
        {
#if defined(AZ_MEMORY_USE_OS_FALLBACK)
            MonolithicFree(ptr);
#else
            SystemFree(ptr);
#endif
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// New overloads
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// In monolithic builds, our new and delete become the global new and delete
// As such, we must support array new/delete
#if defined(AZ_MEMORY_USE_OS_FALLBACK) || AZ_TRAIT_OS_MEMORY_ALWAYS_NEW_DELETE_SUPPORT_ARRAYS
#define AZ_NEW_DELETE_SUPPORT_ARRAYS
#endif

void* AZ::OperatorNew(std::size_t size, const char* fileName, int lineNum, const char* name)
{
    return Internal::GlobalAlloc(size, AZCORE_GLOBAL_NEW_ALIGNMENT, fileName, lineNum, name ? name : "global operator aznew");
}
// General new operators
void* AZ::OperatorNew(std::size_t byteSize)
{
    return Internal::GlobalAlloc(byteSize, AZCORE_GLOBAL_NEW_ALIGNMENT, nullptr, 0, "global operator new");
}
void AZ::OperatorDelete(void* ptr)
{
    Internal::GlobalFree(ptr);
}

void* AZ::OperatorNewArray(std::size_t size, const char* fileName, int lineNum, const char*)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    return Internal::GlobalAlloc(size, AZCORE_GLOBAL_NEW_ALIGNMENT, fileName, lineNum, "global operator aznew[]");
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
    return Internal::GlobalAlloc(size, AZCORE_GLOBAL_NEW_ALIGNMENT, nullptr, 0, "global operator new[]");
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
    Internal::GlobalFree(ptr);
#else
    (void)ptr;
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n\
                    new[] inserts a header (platform dependent) to keep track of the array size!\n\
                    Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
#endif
}
