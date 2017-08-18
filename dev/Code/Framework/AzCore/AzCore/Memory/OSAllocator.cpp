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

#include <AzCore/Memory/OSAllocator.h>

#if defined(AZ_PLATFORM_LINUX)
    #include <malloc.h>
#elif defined(AZ_PLATFORM_APPLE)
    #include <malloc/malloc.h>
#endif

using namespace AZ;

//=========================================================================
// OSAllocator
// [9/2/2009]
//=========================================================================
OSAllocator::OSAllocator()
    : m_custom(NULL)
    , m_numAllocatedBytes(0)
{}

//=========================================================================
// ~OSAllocator
// [9/2/2009]
//=========================================================================
OSAllocator::~OSAllocator()
{
    // Debug allocator can be destroyed even after we exit the application and this is fine.
    if (IsReady())
    {
        OnDestroy();
    }
}

//=========================================================================
// Create
// [9/2/2009]
//=========================================================================
bool
OSAllocator::Create(const Descriptor& desc)
{
    m_custom = desc.m_custom;
    m_numAllocatedBytes = 0;
    OnCreated();
    return true;
}

//=========================================================================
// Destroy
// [9/2/2009]
//=========================================================================
void
OSAllocator::Destroy()
{
    OnDestroy();
}

//=========================================================================
// Allocate
// [9/2/2009]
//=========================================================================
OSAllocator::pointer_type
OSAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    OSAllocator::pointer_type address;
    if (m_custom)
    {
        address =  m_custom->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
    }
    else
    {
        address = AZ_OS_MALLOC(byteSize, alignment);
    }

    if (address == 0)
    {
        AZ_Printf("Memory", "======================================================\n");
        AZ_Printf("Memory", "OSAllocator run out of system memory!\nWe can't track the debug allocator, since it's used for tracking and pipes trought the OS... here are the other allocator status:\n");
        OnOutOfMemory(byteSize, alignment, flags, name, fileName, lineNum);
    }

    m_numAllocatedBytes += byteSize;

    return address;
}

//=========================================================================
// DeAllocate
// [9/2/2009]
//=========================================================================
void
OSAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    (void)alignment;
    if (m_custom)
    {
        m_custom->DeAllocate(ptr);
    }
    else
    {
        AZ_OS_FREE(ptr);
    }

    m_numAllocatedBytes -= byteSize;
}

#endif // #ifndef AZ_UNITY_BUILD