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
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MemoryDrillerBus.h>

using namespace AZ;

AllocatorBase::AllocatorBase(IAllocatorAllocate* allocationSource, const char* name, const char* desc) :
    IAllocator(allocationSource),
    m_name(name),
    m_desc(desc)
{
}

AllocatorBase::~AllocatorBase()
{
    AZ_Assert(!m_isReady, "Allocator %s (%s) is being destructed without first having gone through proper calls to PreDestroy() and Destroy(). Use AllocatorInstance<> for global allocators or AllocatorWrapper<> for local allocators.", m_name, m_desc);
}

const char* AllocatorBase::GetName() const
{
    return m_name;
}

const char* AllocatorBase::GetDescription() const
{
    return m_desc;
}

IAllocatorAllocate* AllocatorBase::GetSchema()
{
    return nullptr;
}

Debug::AllocationRecords* AllocatorBase::GetRecords()
{
    return m_records;
}

void AllocatorBase::SetRecords(Debug::AllocationRecords* records)
{
    m_records = records;
    m_memoryGuardSize = records ? records->MemoryGuardSize() : 0;
}

bool AllocatorBase::IsReady() const
{
    return m_isReady;
}

bool AllocatorBase::CanBeOverridden() const
{
    return m_canBeOverridden;
}

void AllocatorBase::PostCreate()
{
    if (m_registrationEnabled)
    {
        if (AZ::Environment::IsReady())
        {
            AllocatorManager::Instance().RegisterAllocator(this);
        }
        else
        {
            AllocatorManager::PreRegisterAllocator(this);
        }
    }

    m_isReady = true;
}

void AllocatorBase::PreDestroy()
{
    if (m_registrationEnabled && AZ::AllocatorManager::IsReady())
    {
        AllocatorManager::Instance().UnRegisterAllocator(this);
    }

    m_isReady = false;
}

void AllocatorBase::SetLazilyCreated(bool lazy)
{
    m_isLazilyCreated = lazy;
}

bool AllocatorBase::IsLazilyCreated() const
{
    return m_isLazilyCreated;
}

void AllocatorBase::SetProfilingActive(bool active)
{
    m_isProfilingActive = active;
}

bool AllocatorBase::IsProfilingActive() const
{
    return m_isProfilingActive;
}

void AllocatorBase::DisableOverriding()
{
    m_canBeOverridden = false;
}

void AllocatorBase::DisableRegistration()
{
    m_registrationEnabled = false;
}

void AllocatorBase::ProfileAllocation(void* ptr, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, int suppressStackRecord)
{
#if defined(AZ_HAS_VARIADIC_TEMPLATES) && defined(AZ_DEBUG_BUILD)
    ++suppressStackRecord; // one more for the fact the ebus is a function
#endif // AZ_HAS_VARIADIC_TEMPLATES

    if (m_isProfilingActive)
    {
        EBUS_EVENT(AZ::Debug::MemoryDrillerBus, RegisterAllocation, this, ptr, byteSize, alignment, name, fileName, lineNum, suppressStackRecord);
    }
}

void AllocatorBase::ProfileDeallocation(void* ptr, size_t byteSize, size_t alignment, Debug::AllocationInfo* info)
{
    if (m_isProfilingActive)
    {
        EBUS_EVENT(AZ::Debug::MemoryDrillerBus, UnregisterAllocation, this, ptr, byteSize, alignment, info);
    }
}

void AllocatorBase::ProfileReallocation(void* ptr, void* newPtr, size_t newSize, size_t newAlignment)
{
    if (m_isProfilingActive)
    {
        EBUS_EVENT(AZ::Debug::MemoryDrillerBus, ReallocateAllocation, this, ptr, newPtr, newSize, newAlignment);
    }
}

void AllocatorBase::ProfileResize(void* ptr, size_t newSize)
{
    if (newSize && m_isProfilingActive)
    {
        EBUS_EVENT(AZ::Debug::MemoryDrillerBus, ResizeAllocation, this, ptr, newSize);
    }
}

bool AllocatorBase::OnOutOfMemory(size_t byteSize, size_t alignment, int flags, const char* name, const char* fileName, int lineNum)
{
    if (AllocatorManager::IsReady() && AllocatorManager::Instance().m_outOfMemoryListener)
    {
        AllocatorManager::Instance().m_outOfMemoryListener(this, byteSize, alignment, flags, name, fileName, lineNum);
        return true;
    }
    return false;
}
