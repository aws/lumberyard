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

#ifndef CRYINCLUDE_CRYANIMATION_VERTEX_VERTEXCOMMANDBUFFER_H
#define CRYINCLUDE_CRYANIMATION_VERTEX_VERTEXCOMMANDBUFFER_H
#pragma once

namespace AZ
{
    class LegacyJobExecutor;
}

class CVertexData;

//

class CVertexCommandBufferAllocator
{
public:
    virtual ~CVertexCommandBufferAllocator() { }

public:
    virtual void* Allocate(const uint length) = 0;
};

//

class CVertexCommandBufferAllocationCounter
    : public CVertexCommandBufferAllocator
{
public:
    CVertexCommandBufferAllocationCounter();
    virtual ~CVertexCommandBufferAllocationCounter();

public:
    ILINE uint GetCount() const { return m_count; }

    // CVertexCommandBufferAllocator
public:
    virtual void* Allocate(const uint length);

private:
    uint m_count;
};

//

class CVertexCommandBufferAllocatorStatic
    : public CVertexCommandBufferAllocator
{
public:
    CVertexCommandBufferAllocatorStatic(const void* pMemory, const uint length);
    ~CVertexCommandBufferAllocatorStatic();

    // CVertexCommandBufferAllocator
public:
    virtual void* Allocate(const uint length);

private:
    uint8* m_pMemory;
    uint m_memoryLeft;
};

//

class CVertexCommandBuffer
{
public:
    bool Initialize(CVertexCommandBufferAllocator& allocator)
    {
        m_pAllocator = &allocator;
        m_pCommands = NULL;
        m_commandsLength = 0;
        return true;
    }

    template<class Type>
    Type* AddCommand();

    void Process(CVertexData& vertexData);

private:
    CVertexCommandBufferAllocator* m_pAllocator;
    const uint8* m_pCommands;
    uint m_commandsLength;
};

//

template<class Type>
Type* CVertexCommandBuffer::AddCommand()
{
    uint length = sizeof(Type);
    void* pAllocation = m_pAllocator->Allocate(length);
    if (!pAllocation)
    {
        return NULL;
    }

    if (!m_pCommands)
    {
        m_pCommands = (const uint8*)pAllocation;
    }

    m_commandsLength += length;

    Type* pCommand = (Type*)pAllocation;
    new (pCommand) Type();
    pCommand->length = length;
    return pCommand;
}

struct SVertexAnimationJob
{
    CVertexData vertexData;
    CVertexCommandBuffer commandBuffer;
    uint commandBufferLength;

    volatile int* pRenderMeshSyncVariable;
    _smart_ptr<IRenderMesh> m_previousRenderMesh;

public:
    SVertexAnimationJob() {}

    void Begin(AZ::LegacyJobExecutor* pJobExecutor);
    void Execute(int);
};

#endif // CRYINCLUDE_CRYANIMATION_VERTEX_VERTEXCOMMANDBUFFER_H
