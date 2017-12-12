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

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/AllocatorManager.h>

using namespace AZ;

//=========================================================================
// IAllocator
// [9/11/2009]
//=========================================================================
IAllocator::IAllocator()
    : m_records(NULL)
    , m_isReady(false)
{
}

//=========================================================================
// IAllocator
// [9/11/2009]
//=========================================================================
IAllocator::~IAllocator()
{
    if (!AllocatorManager::Instance().m_isAllocatorLeaking)
    {
        AZ_Assert(!m_isReady, "You forgot to destroy an allocator!");
    }
}

//=========================================================================
// OnCreated
// [9/11/2009]
//=========================================================================
void
IAllocator::OnCreated()
{
    AllocatorManager::Instance().RegisterAllocator(this);
    m_isReady = true;
}

//=========================================================================
// OnDestroy
// [9/11/2009]
//=========================================================================
void
IAllocator::OnDestroy()
{
    AllocatorManager::Instance().UnRegisterAllocator(this);
    m_isReady = false;
}

//=========================================================================
// OnOutOfMemory
// [12/2/2010]
//=========================================================================
bool
IAllocator::OnOutOfMemory(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum)
{
    if (AllocatorManager::Instance().m_outOfMemoryListener)
    {
        AllocatorManager::Instance().m_outOfMemoryListener(this, byteSize, alignment, flags, name, fileName, lineNum);
        return true;
    }
    return false;
}

#endif // #ifndef AZ_UNITY_BUILD