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

#include "StdAfx.h"

#include <CrySizer.h>
#include "Vegetation.h"
#include "VegetationPoolManager.h"

namespace LegacyProceduralVegetation
{
    ///////////////////////////////////////////////////////////////////////////
    // VegetationChunk START
    
    void VegetationChunk::GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));

        if (m_pInstances)
        {
            const int maxVegetationInstancesPerChunk = GetCVars()->e_ProcVegetationMaxObjectsInChunk;
            pSizer->AddObject(m_pInstances, sizeof(CVegetation) * maxVegetationInstancesPerChunk);
        }
    }

    // VegetationChunk END
    ///////////////////////////////////////////////////////////////////////////



    ///////////////////////////////////////////////////////////////////////////
    // VegetationSector START

    VegetationSector::~VegetationSector()
    {
        FUNCTION_PROFILER_3DENGINE;
        AZ_Assert(VegetationPoolManager::s_poolManagerPtr, "VegetationPoolManager static pointer not initialized");
        ReleaseAllVegetationInstances(*(VegetationPoolManager::s_poolManagerPtr->m_vegetationChunksPool));
    }

    CVegetation* VegetationSector::AllocateVegetationInstance(VegetationChunksPool& vegetationChunksPool)
    {
        FUNCTION_PROFILER_3DENGINE;

        // find pool id
        const int maxVegetationInstancesPerChunk = GetCVars()->e_ProcVegetationMaxObjectsInChunk;
        int nLastPoolId = m_allocatedVegetationObjectsCount / maxVegetationInstancesPerChunk;
        if (nLastPoolId >= m_vegetationChunks.Count())
        {
            m_vegetationChunks.PreAllocate(nLastPoolId + 1, nLastPoolId + 1);
            VegetationChunk* pChunk = m_vegetationChunks[nLastPoolId] = vegetationChunksPool.GetObject();

            // init objects
            for (int o = 0; o < maxVegetationInstancesPerChunk; o++)
            {
                pChunk->m_pInstances[o].Init();
            }
        }

        // find empty slot id and return pointer to it
        int nNextSlotInPool = m_allocatedVegetationObjectsCount - nLastPoolId * maxVegetationInstancesPerChunk;
        CVegetation* pObj = &(m_vegetationChunks[nLastPoolId]->m_pInstances)[nNextSlotInPool];
        m_allocatedVegetationObjectsCount++;
        return pObj;
    }

    void VegetationSector::ReleaseAllVegetationInstances(VegetationChunksPool& vegetationChunksPool)
    {
        const int maxVegetationInstancesPerChunk = GetCVars()->e_ProcVegetationMaxObjectsInChunk;
        for (int i = 0; i < m_vegetationChunks.Count(); i++)
        {
            VegetationChunk* pChunk = m_vegetationChunks[i];
            for (int o = 0; o < maxVegetationInstancesPerChunk; o++)
            {
                pChunk->m_pInstances[o].ShutDown();
            }
            vegetationChunksPool.ReleaseObject(m_vegetationChunks[i]);
        }
        m_vegetationChunks.Clear();
        m_allocatedVegetationObjectsCount = 0;
    }

    void VegetationSector::GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));

        pSizer->AddObject(m_vegetationChunks);

        const int maxVegetationInstancesPerChunk = GetCVars()->e_ProcVegetationMaxObjectsInChunk;
        for (int i = 0; i < m_vegetationChunks.Count(); i++)
        {
            pSizer->AddObject(m_vegetationChunks[i], sizeof(CVegetation) * maxVegetationInstancesPerChunk);
        }
    }

    // VegetationSector END
    ///////////////////////////////////////////////////////////////////////////
    
    VegetationPoolManager* VegetationPoolManager::s_poolManagerPtr = nullptr;

    VegetationPoolManager::VegetationPoolManager()
    {
        m_vegetationChunksPool = new VegetationChunksPool(MAX_PROC_OBJ_CHUNKS_NUM);
        m_vegetationSectorsPool = new VegetationSectorsPool(GetCVars()->e_ProcVegetationMaxSectorsInCache);
        s_poolManagerPtr = this;
    }

    bool VegetationPoolManager::AddVegetationInstanceToSector(VegetationSector* sector, float scale, Vec3 worldPosition, int nGroupId, byte instanceAngle)
    {
        AZ_Assert(sector, "Invalid sector handle");

        CVegetation* vegetationInstance = sector->AllocateVegetationInstance(*m_vegetationChunksPool);
        if (!vegetationInstance)
        {
            return false;
        }

        vegetationInstance->SetScale(scale);
        vegetationInstance->m_vPos = worldPosition;
        vegetationInstance->SetStatObjGroupIndex(nGroupId);

        AABB aabb = vegetationInstance->CalcBBox();

        vegetationInstance->SetRndFlags(ERF_PROCEDURAL, true);
        vegetationInstance->m_fWSMaxViewDist = vegetationInstance->GetMaxViewDist(); // note: duplicated
        vegetationInstance->UpdateRndFlags();
        vegetationInstance->Physicalize(true);

        float fObjRadius = aabb.GetRadius();
        if (fObjRadius > MAX_VALID_OBJECT_VOLUME || !_finite(fObjRadius) || fObjRadius <= 0)
        {
            AZ_Warning("LegacyProceduralVegetation", false, "Vegetation Instance has invalid bbox: %s,%s, GetRadius() = %.2f",
                vegetationInstance->GetName(), vegetationInstance->GetEntityClassName(), fObjRadius);
        }
        if (Get3DEngine()->IsObjectTreeReady())
        {
            Get3DEngine()->GetObjectTree()->InsertObject(vegetationInstance, aabb, fObjRadius, aabb.GetCenter());
        }
        return true;
    }

    void VegetationPoolManager::GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_vegetationSectorsPool);
        pSizer->AddObject(m_vegetationChunksPool);
    }

    VegetationPoolManager::~VegetationPoolManager()
    {
        SAFE_DELETE(m_vegetationChunksPool);
        SAFE_DELETE(m_vegetationSectorsPool);
    }

}//namespace LegacyProceduralVegetation
