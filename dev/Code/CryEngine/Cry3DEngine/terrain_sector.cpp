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

// Description : sector initialiazilation, objects rendering


#include "StdAfx.h"
#include "terrain_sector.h"
#include "terrain.h"
#include "ObjMan.h"

void CTerrainNode::SetLOD(const SRenderingPassInfo& passInfo)
{
    const float distanceToCamera = m_DistanceToCamera[passInfo.GetRecursiveLevel()];
    const float sectorSizeAndAHalf = static_cast<float>(CTerrain::GetSectorSize() + (CTerrain::GetSectorSize() >> 2));

    // Force highest detail when camera is in sector.
    if (distanceToCamera < sectorSizeAndAHalf)
    {
        m_QueuedLOD = 0;
    }
    else
    {
        // Ad-hoc error calculation.
        float allowedError = (passInfo.GetZoomFactor() * GetCVars()->e_TerrainLodRatio * distanceToCamera) / 180.f * 2.5f;

        //
        // The LOD range spans the entire sector quad, so we can't go higher than that. The unit-to-sector bitshift encodes this maximum LOD.
        // Each sector has a min LOD that it has encoded in the height data (e.g. some have only a 3x3 heightmap). We can't go lower than this.
        //
        const int maxLOD = GetTerrain()->m_UnitToSectorBitShift - 1;
        const int minLOD = 0;

        int lod = maxLOD;
        while (lod > minLOD)
        {
            bool bErrorIsAcceptable = m_ZErrorFromBaseLOD[lod] < allowedError;
            if (bErrorIsAcceptable)
            {
                break;
            }
            --lod;
        }
        m_QueuedLOD = min(lod, int(distanceToCamera / 32)); // This seems like a completely ad-hoc clamp.
    }

    if (passInfo.IsGeneralPass())
    {
        m_TextureLOD = GetTextureLOD(distanceToCamera, passInfo);
    }
}

int CTerrainNode::GetSecIndex()
{
    int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;
    int nSectorsTableSize = CTerrain::GetSectorsTableSize() >> m_nTreeLevel;
    return (m_nOriginX / nSectorSize) * nSectorsTableSize + (m_nOriginY / nSectorSize);
}

void CTerrainNode::GetMaterials(AZStd::vector<_smart_ptr<IMaterial>>& materials)
{
    const int projectionAxisCount = 3;
    const uint8 projectionAxis[projectionAxisCount] = { 'X', 'Y', 'Z' };

    for (int i = 0; i < m_DetailLayers.Count(); i++)
    {
        if (!m_DetailLayers[i].surfaceType || !m_DetailLayers[i].surfaceType->HasMaterial() || !m_DetailLayers[i].HasRM())
        {
            continue;
        }

        for (int p = 0; p < projectionAxisCount; p++)
        {
            _smart_ptr<IMaterial> pMat = m_DetailLayers[i].surfaceType->GetMaterialOfProjection(projectionAxis[p]);
            if (pMat)
            {
                materials.push_back(pMat);
            }
        }
    }
}