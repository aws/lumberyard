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

#include <AzCore/Memory/AllocatorBase.h>

#ifdef LEGACYALLOCATOR_MODULE_STATIC
#include <AzCore/std/typetraits/static_storage.h>
#endif

#define AZCORE_SYS_ALLOCATOR_HPPA
//#define AZCORE_SYS_ALLOCATOR_MALLOC

#ifdef AZCORE_SYS_ALLOCATOR_HPPA
#   include <AzCore/Memory/HphaSchema.h>
#elif defined(AZCORE_SYS_ALLOCATOR_MALLOC)
#   include <AzCore/Memory/MallocSchema.h>
#else
#   include <AzCore/Memory/HeapSchema.h>
#endif

namespace AZ
{

#ifdef AZCORE_SYS_ALLOCATOR_HPPA
    typedef AZ::HphaSchema LegacyAllocatorSchema;
#elif defined(AZCORE_SYS_ALLOCATOR_MALLOC)
    typedef AZ::MallocSchema LegacyAllocatorSchema;
#else
    typedef AZ::HeapSchema LegacyAllocatorSchema;
#endif

    struct LegacyAllocatorDescriptor
        : public LegacyAllocatorSchema::Descriptor
    {
        LegacyAllocatorDescriptor()
        {
            // pull 32MB from the OS at a time
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
            m_systemChunkSize = 32 * 1024 * 1024;
#endif
        }
    };

    class LegacyAllocator
        : public AllocatorBase<LegacyAllocatorSchema, LegacyAllocatorDescriptor>
    {
    public:
        AZ_TYPE_INFO(LegacyAllocator, "{17FC25A4-92D9-48C5-BB85-7F860FCA2C6F}");

        using Descriptor = LegacyAllocatorDescriptor;
        using Base = AllocatorBase<LegacyAllocatorSchema, LegacyAllocatorDescriptor>;

        LegacyAllocator()
            : Base("LegacyAllocator", "Allocator for Legacy CryEngine systems")
        {
            // unlike most allocators, this allocator is lazily created when it's used
            // and unfortunately that's at static init time, so we have no choice
            // but to force creation now
            Create(Descriptor());
        }

        LegacyAllocator(const char* name, const char* desc)
            : Base(name, desc)
        {
            // unlike most allocators, this allocator is lazily created when it's used
            // and unfortunately that's at static init time, so we have no choice
            // but to force creation now
            Create(Descriptor());
        }

        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
        {
            if (alignment == 0)
            {
                // Some STL containers, like std::vector, are assuming a specific minimum alignment. seems to have a requirement
                // Take a look at _Allocate_manually_vector_aligned in xmemory0
                alignment = sizeof(void*) * 2; 
            }
            return Base::Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
        }

        // DeAllocate with file/line, to track when allocs were freed from Cry
        void DeAllocate(pointer_type ptr, const char* file, const int line, size_type byteSize = 0, size_type alignment = 0)
        {
            AZ_PROFILE_MEMORY_FREE_EX(AZ::Debug::ProfileCategory::MemoryReserved, file, line, ptr);
            m_schema->DeAllocate(ptr, byteSize, alignment);
        }

        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            Base::DeAllocate(ptr, byteSize, alignment);
        }

        // Realloc with file/line, because Cry uses realloc(nullptr) and realloc(ptr, 0) to mimic malloc/free
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment, const char* file, const int line)
        {
            if (newAlignment == 0)
            {
                // Some STL containers, like std::vector, are assuming a specific minimum alignment. seems to have a requirement
                // Take a look at _Allocate_manually_vector_aligned in xmemory0
                newAlignment = sizeof(void*) * 2;
            }
            AZ_PROFILE_MEMORY_FREE_EX(AZ::Debug::ProfileCategory::MemoryReserved, file, line, ptr);
            pointer_type newPtr = m_schema->ReAllocate(ptr, newSize, newAlignment);
            AZ_PROFILE_MEMORY_ALLOC_EX(AZ::Debug::ProfileCategory::MemoryReserved, file, line, newPtr, newSize, "LegacyAllocator Realloc");
            return newPtr;
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
        {
            if (newAlignment == 0)
            {
                // Some STL containers, like std::vector, are assuming a specific minimum alignment. seems to have a requirement
                // Take a look at _Allocate_manually_vector_aligned in xmemory0
                newAlignment = sizeof(void*) * 2;
            }
            return Base::ReAllocate(ptr, newSize, newAlignment);
        }
    };

    using StdLegacyAllocator = AZStdAlloc<LegacyAllocator>;

#ifdef LEGACYALLOCATOR_MODULE_STATIC
    // Specialize for the LegacyAllocator to provide one per module that does not use the
    // environment for its storage
    template <>
    class AllocatorInstance<LegacyAllocator>
    {
    public:
        using Descriptor = LegacyAllocator::Descriptor;

        static LegacyAllocator& Get()
        {
#if AZ_TRAIT_COMPILER_USE_STATIC_STORAGE_FOR_NON_POD_STATICS
            static AZStd::static_storage<LegacyAllocator> s_legacyAllocator;
#else
            static LegacyAllocator s_legacyAllocator;
#endif
            return s_legacyAllocator;
        }

        static void Create(const Descriptor& desc = Descriptor())
        {
            Get().Create(desc);
        }

        static void Destroy()
        {
            Get().Destroy();
        }

        AZ_FORCE_INLINE static bool IsReady()
        {
            return true;
        }        
    };
#endif // LEGACYALLOCATOR_MODULE_STATIC
}
