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
#include "Pool.h"
#include "Memory.h"

namespace Memory {
    /*
    CContext
    */

    CContext::CContext()
        : m_pPools(NULL)
    {
    }

    //

    void CContext::AddPool(CPool& pool)
    {
        if (HasPool(pool))
        {
            return;
        }

        pool.m_pContext = this;
        pool.m_pNext = m_pPools;
        m_pPools = &pool;
    }

    void CContext::RemovePool(CPool& pool)
    {
        if (m_pPools == &pool)
        {
            pool.m_pContext = NULL;
            m_pPools = m_pPools->m_pNext;
            return;
        }

        CPool* pPool = m_pPools;
        while (pPool)
        {
            if (pPool->m_pNext != &pool)
            {
                continue;
            }

            pool.m_pContext = NULL;
            pPool->m_pNext = pPool->m_pNext->m_pNext;
            break;
        }
    }

    bool CContext::HasPool(CPool& pool)
    {
        return pool.m_pContext == this;
    }

    void CContext::Update()
    {
        CPool* pPool = m_pPools;
        while (pPool)
        {
            pPool->Update();

            pPool = pPool->m_pNext;
        }
    }

    void CContext::GetMemoryUsage(ICrySizer* pSizer) const
    {
        CPool* pPool = m_pPools;
        while (pPool)
        {
            pSizer->AddObject(pPool);
            pPool = pPool->m_pNext;
        }

        pSizer->AddObject(this, sizeof(*this));
    }
} // namespace Memory

/*
CAnimationContext
*/

void CAnimationContext::Update()
{
    m_memoryContext.Update();
}

void CAnimationContext::GetMemoryUsage(ICrySizer* pSizer) const
{
    m_memoryContext.GetMemoryUsage(pSizer);
}
