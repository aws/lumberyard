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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include <AzCore/Debug/Profiler.h>

#include <AzCore/Jobs/LegacyJobExecutor.h>

#include "../ModelMesh.h"

#include "VertexData.h"
#include "VertexCommand.h"
#include "VertexCommandBuffer.h"

/*
CVertexCommandBufferAllocationCounter
*/

CVertexCommandBufferAllocationCounter::CVertexCommandBufferAllocationCounter()
    : m_count(0)
{
}

CVertexCommandBufferAllocationCounter::~CVertexCommandBufferAllocationCounter()
{
}

// CVertexCommandBufferAllocator

void* CVertexCommandBufferAllocationCounter::Allocate(const uint length)
{
    m_count += length;
    return NULL;
}

/*
CVertexCommandBufferAllocatorStatic
*/

CVertexCommandBufferAllocatorStatic::CVertexCommandBufferAllocatorStatic(const void* pMemory, const uint length)
    : m_pMemory((uint8*)pMemory)
    , m_memoryLeft(length)
{
}

CVertexCommandBufferAllocatorStatic::~CVertexCommandBufferAllocatorStatic()
{
}

// CVertexCommandBufferAllocator

void* CVertexCommandBufferAllocatorStatic::Allocate(const uint length)
{
    if (length > m_memoryLeft)
    {
        return NULL;
    }

    void* pAllocation = m_pMemory;
    m_pMemory += length;
    m_memoryLeft -= length;
    return pAllocation;
}

/*
CVertexCommandBuffer
*/

void CVertexCommandBuffer::Process(CVertexData& vertexData)
{
    int length = m_commandsLength;
    if (!length)
    {
        return;
    }

    const uint8* pCommands = m_pCommands;
    while (length > sizeof(VertexCommand))
    {
        VertexCommand* pCommand = (VertexCommand*)pCommands;
        pCommands += pCommand->length;
        length -= int(pCommand->length);
        pCommand->Execute(*pCommand, vertexData);
    }
}

/*
SVertexAnimationJobData
*/

void SVertexAnimationJob::Execute(int)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

    if (commandBufferLength)
    {
        commandBuffer.Process(vertexData);
    }

    if (m_previousRenderMesh)
    {
        m_previousRenderMesh->UnlockStream(VSF_GENERAL);
        m_previousRenderMesh->UnLockForThreadAccess();
    }

    m_previousRenderMesh = NULL;

    CryInterlockedDecrement(pRenderMeshSyncVariable);
}

void SVertexAnimationJob::Begin(AZ::LegacyJobExecutor* pJobExecutor)
{
    pJobExecutor->StartJob([this]()
    {
        this->Execute(0);
    }); // Legacy JobManager priority: eRegularPriority
}
