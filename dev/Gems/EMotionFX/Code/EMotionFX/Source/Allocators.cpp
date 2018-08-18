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

#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    // Implementation of the GetDescription methods. They are put here so they don't generate multiple
    // strings in the binaries across compilation units
#define EMOTION_FX_ALLOCATOR_IMPL(ALLOCATOR_SEQUENCE) \
    const char* EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)::GetDescription() const         { return EMOTIONFX_ALLOCATOR_SEQ_GET_DESCRIPTION(ALLOCATOR_SEQUENCE); }
    
    // Here we create the "GetDescription" method implementation for all the items in the header's table (Step 3)
    AZ_SEQ_FOR_EACH(EMOTION_FX_ALLOCATOR_IMPL, EMOTIONFX_ALLOCATORS)

#undef EMOTION_FX_ALLOCATOR_IMPL

    template <typename TAllocator, typename TSubAllocator>
    class AllocatorCreator
    {
    public:
        static void Create()
        {
            typename TAllocator::Descriptor descriptor;
            descriptor.m_custom = &AZ::AllocatorInstance<TSubAllocator>::Get();
            AZ::AllocatorInstance<TAllocator>::Create(descriptor);
        }
    };

    template <typename TAllocator>
    class AllocatorCreator<TAllocator, NoSubAllocator>
    {
    public:
        static void Create()
        {
            AZ::AllocatorInstance<TAllocator>::Create();
        }
    };

    // Create all allocators
    void Allocators::Create()
    {
#define EMOTION_FX_ALLOCATOR_CREATE(ALLOCATOR_SEQUENCE) \
    AllocatorCreator<EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE), EMOTIONFX_ALLOCATOR_SEQ_GET_SUBALLOCATOR(ALLOCATOR_SEQUENCE)>::Create();

        // Here we call the "Create" to create the allocator for all the items in the header's table (Step 4)
        AZ_SEQ_FOR_EACH(EMOTION_FX_ALLOCATOR_CREATE, EMOTIONFX_ALLOCATORS)

#undef EMOTION_FX_ALLOCATOR_CREATE
    }

    // Destroy all allocators
    void Allocators::Destroy()
    {
#define EMOTION_FX_ALLOCATOR_DESTROY(ALLOCATOR_SEQUENCE) \
    AZ::AllocatorInstance<EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)>::Destroy();

        // Here we call the "Destroy" to destroy the allocator for all the items in the header's table (Step 5)
        AZ_SEQ_FOR_EACH(EMOTION_FX_ALLOCATOR_DESTROY, EMOTIONFX_ALLOCATORS)

#undef EMOTION_FX_ALLOCATOR_DESTROY
    }

    void Allocators::ShrinkPools()
    {
        AZ::AllocatorInstance<AnimGraphObjectDataAllocator>::Get().GarbageCollect();
        AZ::AllocatorInstance<AnimGraphObjectUniqueDataAllocator>::Get().GarbageCollect();
        AZ::AllocatorInstance<AnimGraphConditionAllocator>::Get().GarbageCollect();
    }

} // EMotionFX namespace