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

// Description : Surface type class implementation.


#include "StdAfx.h"
#include "SurfaceType.h"
#include "GameEngine.h"
#include "VegetationMap.h"
#include "VegetationObject.h"
#include "Material/MaterialManager.h"

#include "Terrain/TerrainManager.h"
#include "Terrain/Layer.h"

#include "ITerrain.h"

//////////////////////////////////////////////////////////////////////////
CSurfaceType::CSurfaceType()
{
    m_nLayerReferences = 0;
    m_detailScale[0] = 1;
    m_detailScale[1] = 1;
    m_projAxis = ESFT_Z;
    m_nSurfaceTypeID = CLayer::e_undefined;
}

//////////////////////////////////////////////////////////////////////////
CSurfaceType::~CSurfaceType()
{
    m_detailScale[0] = 1;
    m_detailScale[1] = 1;
}

//////////////////////////////////////////////////////////////////////////
void CSurfaceType::Serialize(CXmlArchive& xmlAr)
{
    Serialize(xmlAr.root, xmlAr.bLoading);
}
//////////////////////////////////////////////////////////////////////////
void CSurfaceType::Serialize(XmlNodeRef xmlRootNode, bool boLoading)
{
    if (boLoading)
    {
        XmlNodeRef sfType = xmlRootNode;

        // Name
        sfType->getAttr("Name", m_name);
        sfType->getAttr("SurfaceTypeID", m_nSurfaceTypeID);
        sfType->getAttr("DetailTexture", m_detailTexture);
        sfType->getAttr("DetailScaleX", m_detailScale[0]);
        sfType->getAttr("DetailScaleY", m_detailScale[1]);
        sfType->getAttr("DetailMaterial", m_material);
        sfType->getAttr("ProjectAxis", m_projAxis);
        sfType->getAttr("Bumpmap", m_bumpmap);

        if (!m_detailTexture.isEmpty() && m_material.isEmpty())
        {
            // For backward compatibility create a material for this detail texture.
            CMaterial* pMtl = GetIEditor()->GetMaterialManager()->CreateMaterial(Path::RemoveExtension(m_detailTexture), XmlNodeRef(), 0);
            pMtl->AddRef();
            pMtl->SetShaderName("Terrain.Layer");
            pMtl->GetShaderResources().m_TexturesResourcesMap[EFTT_DIFFUSE].m_Name = m_detailTexture.toUtf8().data();  // populate the diffuse slot
            pMtl->Update();
            m_material = pMtl->GetName();
        }
    }
    else
    {
        ////////////////////////////////////////////////////////////////////////
        // Storing
        ////////////////////////////////////////////////////////////////////////
        XmlNodeRef sfType = xmlRootNode;

        // Name
        sfType->setAttr("Name", m_name.toUtf8().data());
        sfType->setAttr("SurfaceTypeID", m_nSurfaceTypeID);
        sfType->setAttr("DetailTexture", m_detailTexture.toUtf8().data());
        sfType->setAttr("DetailScaleX", m_detailScale[0]);
        sfType->setAttr("DetailScaleY", m_detailScale[1]);
        sfType->setAttr("DetailMaterial", m_material.toUtf8().data());
        sfType->setAttr("ProjectAxis", m_projAxis);
        sfType->setAttr("Bumpmap", m_bumpmap.toUtf8().data());

        switch (m_projAxis)
        {
        case 0:
            sfType->setAttr("ProjAxis", "X");
            break;
        case 1:
            sfType->setAttr("ProjAxis", "Y");
            break;
        case 2:
        default:
            sfType->setAttr("ProjAxis", "Z");
        }
        ;

        SaveVegetationIds(sfType);
    }
}
//////////////////////////////////////////////////////////////////////////
void CSurfaceType::SaveVegetationIds(XmlNodeRef& node)
{
    CVegetationMap* pVegMap = GetIEditor()->GetVegetationMap();
    // Go thru all vegetation groups, and see who uses us.

    if (!pVegMap)
    {
        return;
    }
    bool bExport = false;
    if (node && node->getParent() && node->getParent()->getTag())
    {
        QString pTag = node->getParent()->getTag();
        bExport = (pTag == "SurfaceTypes");
    }

    if (bExport)
    {
        DynArray<struct IStatInstGroup*> statInstGroupTable;

        if (ITerrain* pTerrain = GetIEditor()->Get3DEngine()->GetITerrain())
        {
            uint32 nObjTypeMask = 1 << eERType_Vegetation | 1 << eERType_MergedMesh;

            pTerrain->GetStatObjAndMatTables(NULL, NULL, &statInstGroupTable, nObjTypeMask);

            for (int i = 0; i < statInstGroupTable.size(); ++i)
            {
                IStatInstGroup* p = statInstGroupTable[i];
                for (int j = 0; j < pVegMap->GetObjectCount(); ++j)
                {
                    CVegetationObject* pObject = pVegMap->GetObject(j);
                    if (pObject->GetId() == i)
                    {
                        if (pObject->IsUsedOnTerrainLayer(m_name))
                        {
                            XmlNodeRef nodeVeg = node->newChild("VegetationGroup");
                            nodeVeg->setAttr("GUID", pObject->GetGUID());
                            nodeVeg->setAttr("Id", i);
                            break;
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < pVegMap->GetObjectCount(); i++)
        {
            CVegetationObject* pObject = pVegMap->GetObject(i);
            if (pObject->IsUsedOnTerrainLayer(m_name))
            {
                XmlNodeRef nodeVeg = node->newChild("VegetationGroup");
                nodeVeg->setAttr("Id", pObject->GetId());
                nodeVeg->setAttr("GUID", pObject->GetGUID());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSurfaceType::SetMaterial(const QString& mtl)
{
    m_material = mtl;
}

//////////////////////////////////////////////////////////////////////////
void CSurfaceType::AssignUnusedSurfaceTypeID()
{
    // Inefficient function to find a free unused surface type id.
    int nID = CLayer::e_undefined;
    std::vector<int> ids;

    const CTerrainManager* tManager = GetIEditor()->GetTerrainManager();

    for (int i = 0; i < tManager->GetSurfaceTypeCount(); i++)
    {
        CSurfaceType* pSrfType = tManager->GetSurfaceTypePtr(i);
        ids.push_back(pSrfType->GetSurfaceTypeID());
    }
    std::sort(ids.begin(), ids.end());

    int numIds = ids.size();
    for (int i = 0; i < numIds; i++)
    {
        int j;
        for (j = 0; j < numIds; j++)
        {
            if (i == ids[j])
            {
                break;
            }
        }
        if (j == numIds)
        {
            nID = i;
            break;
        }
    }
    if (nID >= CLayer::e_undefined)
    {
        nID = numIds < CLayer::e_undefined ? numIds : CLayer::e_undefined;
    }

    m_nSurfaceTypeID = nID;
}
