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

#ifndef CRYINCLUDE_CRYANIMATION_MEMORY_MEMORY_H
#define CRYINCLUDE_CRYANIMATION_MEMORY_MEMORY_H
#pragma once

namespace Memory {
    class CPool;

    class CContext
    {
    public:
        CContext();

    public:
        void AddPool(CPool& pool);
        void RemovePool(CPool& pool);

        bool HasPool(CPool& pool);

        void Update();

        void GetMemoryUsage(ICrySizer* pSizer) const;

    private:
        CPool* m_pPools;
    };
} // namespace Memory

class CAnimationContext
{
public:
    static CAnimationContext& Instance()
    {
        static CAnimationContext instance;
        return instance;
    }

public:
    Memory::CContext& GetMemoryContext() { return m_memoryContext; }

    void Update();

    void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    Memory::CContext m_memoryContext;
};

#endif // CRYINCLUDE_CRYANIMATION_MEMORY_MEMORY_H
