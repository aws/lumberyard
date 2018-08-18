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

// Description : terrain detail textures


#include "StdAfx.h"
#include "terrain.h"
#include "terrain_sector.h"
#include "3dEngine.h"

void CTerrain::SetDetailLayerProperties(int nId, float fScaleX, float fScaleY,
    uint8 ucProjAxis, const char* szSurfName,
    const PodArray<int>& lstnVegetationGroups, _smart_ptr<IMaterial> pMat)
{
    if (nId >= 0 && nId < SurfaceTile::MaxSurfaceCount)
    {
        const int nNameLength = strlen(szSurfName);
        const int nDestNameLength = sizeof(m_SurfaceTypes[nId].szName) - 1;
        if (nNameLength > nDestNameLength)
        {
            Error("CTerrain::SetDetailLayerProperties: attempt to assign too long surface type name (%s)", szSurfName);
        }
        cry_strcpy(m_SurfaceTypes[nId].szName, szSurfName);
        m_SurfaceTypes[nId].fScale = fScaleX;
        m_SurfaceTypes[nId].ucDefProjAxis = ucProjAxis;
        m_SurfaceTypes[nId].ucThisSurfaceTypeId = nId;
        m_SurfaceTypes[nId].lstnVegetationGroups.Reset();
        m_SurfaceTypes[nId].lstnVegetationGroups.AddList(lstnVegetationGroups);
        m_SurfaceTypes[nId].pLayerMat = (CMatInfo*)pMat.get();
        if (m_SurfaceTypes[nId].pLayerMat && !m_bEditor)
        {
            CTerrain::Get3DEngine()->PrintMessage("  Layer %d - %s has material %s", nId, szSurfName, pMat->GetName());
        }
    }
    else
    {
        Warning("CTerrain::SetDetailTextures: LayerId is out of range: %d: %s", nId, szSurfName);
        assert(!"CTerrain::SetDetailTextures: LayerId is out of range");
    }
}

void CTerrain::LoadSurfaceTypesFromXML(XmlNodeRef pDoc)
{
    LOADING_TIME_PROFILE_SECTION;

    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Surface types");

    if (!pDoc)
    {
        return;
    }

    IMaterialManager*       pMatMan = Get3DEngine()->GetMaterialManager();
    for (int nId = 0; nId < pDoc->getChildCount() && nId < SurfaceTile::MaxSurfaceCount; nId++)
    {
        XmlNodeRef pDetLayer = pDoc->getChild(nId);
        const char* pMatName = pDetLayer->getAttr("DetailMaterial");
        _smart_ptr<IMaterial> pMat = pMatMan->FindMaterial(pMatName);
        if (!pMat)
        {
            pMat = pMatMan->LoadMaterial(pMatName, true, false, MTL_FLAG_IS_TERRAIN);
        }

        float fScaleX = 1.f;
        pDetLayer->getAttr("DetailScaleX", fScaleX);
        float fScaleY = 1.f;
        pDetLayer->getAttr("DetailScaleY", fScaleY);
        uint8 projAxis = pDetLayer->getAttr("ProjAxis")[0];

        if (!pMat || pMat == pMatMan->GetDefaultMaterial())
        {
            Error("CTerrain::LoadSurfaceTypesFromXML: Error loading material: %s", pMatName);
            pMat = pMatMan->GetDefaultTerrainLayerMaterial();
        }

        if (CMatInfo* pMatInfo = (CMatInfo*)pMat.get())
        {
            pMatInfo->m_fDefautMappingScale = fScaleX;
            Get3DEngine()->InitMaterialDefautMappingAxis(pMatInfo);
        }

        PodArray<int> lstnVegetationGroups;
        for (int nGroup = 0; nGroup < pDetLayer->getChildCount(); nGroup++)
        {
            XmlNodeRef pVegetationGroup = pDetLayer->getChild(nGroup);
            int nVegetationGroupId = -1;
            if (pVegetationGroup->isTag("VegetationGroup") && pVegetationGroup->getAttr("Id", nVegetationGroupId))
            {
                lstnVegetationGroups.Add(nVegetationGroupId);
            }
        }

        SetDetailLayerProperties(nId, fScaleX, fScaleY, projAxis, pDetLayer->getAttr("Name"), lstnVegetationGroups, pMat);
    }
}

void CTerrain::UpdateSurfaceTypes()
{
    if (m_RootNode)
    {
        m_RootNode->UpdateDetailLayersInfo(true);
    }
}
