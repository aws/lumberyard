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

#include "StdAfx.h"
#include "PolygonMesh.h"
#include "PolygonDecomposer.h"
#include "Material/Material.h"
#include "Material/MaterialManager.h"

namespace CD
{
    PolygonMesh::PolygonMesh()
    {
        m_pStatObj = NULL;
        m_pRenderNode = NULL;
        m_MaterialName = "Editor/Materials/crydesigner_selection";
    }

    PolygonMesh::~PolygonMesh()
    {
        ReleaseResources();
    }

    void PolygonMesh::ReleaseResources()
    {
        if (m_pStatObj)
        {
            m_pStatObj->Release();
            m_pStatObj = NULL;
        }

        ReleaseRenderNode();
    }

    void PolygonMesh::ReleaseRenderNode()
    {
        if (m_pRenderNode)
        {
            GetIEditor()->Get3DEngine()->DeleteRenderNode(m_pRenderNode);
            m_pRenderNode = NULL;
        }
    }

    void PolygonMesh::CreateRenderNode()
    {
        ReleaseRenderNode();
        m_pRenderNode = GetIEditor()->Get3DEngine()->CreateRenderNode(eERType_Brush);
    }

    void PolygonMesh::SetPolygon(PolygonPtr pPolygon, bool bForce, const Matrix34& worldTM, int dwRndFlags, float viewDistMultiplier, int nMinSpec, uint8 materialLayerMask)
    {
        if (m_pPolygons.size() == 1 && m_pPolygons[0] == pPolygon && !bForce)
        {
            return;
        }

        std::vector<PolygonPtr> polygons;
        polygons.push_back(pPolygon);

        SetPolygons(polygons, bForce, worldTM, dwRndFlags, viewDistMultiplier, nMinSpec, materialLayerMask);
    }

    void PolygonMesh::SetPolygons(const std::vector<PolygonPtr>& polygonList, bool bForce, const Matrix34& worldTM, int dwRndFlags, float viewDistMultiplier, int nMinSpec, uint8 materialLayerMask)
    {
        m_pPolygons = polygonList;
        ReleaseResources();

        FlexibleMesh mesh;

        for (int i = 0, iPolygonSize(polygonList.size()); i < iPolygonSize; ++i)
        {
            PolygonPtr pPolygon = polygonList[i];
            if (!pPolygon || pPolygon->IsOpen())
            {
                continue;
            }

            int vertexOffset = mesh.vertexList.size();
            int faceOffset = mesh.faceList.size();

            PolygonDecomposer decomposer;
            if (!decomposer.TriangulatePolygon(pPolygon, mesh, vertexOffset, faceOffset))
            {
                continue;
            }
        }

        if (mesh.IsValid())
        {
            UpdateStatObjAndRenderNode(mesh, worldTM, dwRndFlags, viewDistMultiplier, nMinSpec, materialLayerMask);
        }
    }

    //void PolygonMesh::UpdateStatObjAndRenderNode( const SMeshInfo& mesh, const Matrix34& worldTM, int dwRndFlags, float viewDistanceMultiplier, int nMinSpec, uint8 materialLayerMask )
    //void PolygonMesh::UpdateStatObjAndRenderNode( const FlexibleMesh& mesh, const Matrix34& worldTM, int dwRndFlags, int nViewDistRatio, int nMinSpec, uint8 materialLayerMask )

    void PolygonMesh::UpdateStatObjAndRenderNode(const FlexibleMesh& mesh, const Matrix34& worldTM, int dwRndFlags, float viewDistanceMultiplier, int nMinSpec, uint8 materialLayerMask)
    {
        CreateRenderNode();

        if (!m_pStatObj)
        {
            m_pStatObj = GetIEditor()->Get3DEngine()->CreateStatObj();
            m_pStatObj->AddRef();
        }

        IIndexedMesh* pMesh = m_pStatObj->GetIndexedMesh();
        if (!pMesh)
        {
            return;
        }

        STexInfo texInfo;
        FillMesh(mesh, pMesh);

        m_pStatObj->SetBBoxMin(pMesh->GetBBox().min);
        m_pStatObj->SetBBoxMax(pMesh->GetBBox().max);

        pMesh->Optimize();
        pMesh->RestoreFacesFromIndices();

        Matrix34 identityTM = Matrix34::CreateIdentity();
        m_pRenderNode->SetEntityStatObj(0, m_pStatObj, &identityTM);

        m_pStatObj->Invalidate(false);

        m_pRenderNode->SetMatrix(worldTM);
        m_pRenderNode->SetRndFlags(dwRndFlags | ERF_RENDER_ALWAYS);
        m_pRenderNode->SetViewDistanceMultiplier(viewDistanceMultiplier);
        m_pRenderNode->SetMinSpec(nMinSpec);
        m_pRenderNode->SetMaterialLayers(materialLayerMask);

        ApplyMaterial();
    }

    void PolygonMesh::SetWorldTM(const Matrix34& worldTM)
    {
        SetPolygons(m_pPolygons, true, worldTM);
    }

    void PolygonMesh::SetMaterialName(const string& name)
    {
        m_MaterialName = name;
        ApplyMaterial();
        if (m_pStatObj)
        {
            m_pStatObj->Invalidate(true);
        }
    }

    void PolygonMesh::ApplyMaterial()
    {
        if (!m_pStatObj || !m_pRenderNode)
        {
            return;
        }

        _smart_ptr<CMaterial> pMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(m_MaterialName);
        if (pMaterial == NULL)
        {
            return;
        }

        m_pStatObj->SetMaterial(pMaterial->GetMatInfo());
        m_pRenderNode->SetMaterial(pMaterial->GetMatInfo());
    }
};