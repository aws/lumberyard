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
#include "ModelCompiler.h"
#include "Model.h"
#include "Objects/DesignerObject.h"
#include "Tools/BaseTool.h"
#include "SmoothingGroupManager.h"
#include "SubdivisionModifier.h"
#include "Material/Material.h"
#include "BrushHelper.h"

namespace CD
{
    ModelCompiler::ModelCompiler(int nCompilerFlag)
    {
        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            m_pStatObj[shelfID] = NULL;
            m_pRenderNode[shelfID] = NULL;
        }

        m_nCompilerFlag = nCompilerFlag;
        m_RenderFlags = CheckFlags(eCompiler_CastShadow) ? ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS : 0;
        m_viewDistanceMultiplier = 1.0f;
    }

    ModelCompiler::ModelCompiler(const ModelCompiler& compiler)
    {
        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            m_pStatObj[shelfID] = NULL;
            m_pRenderNode[shelfID] = NULL;
        }
        m_RenderFlags = compiler.m_RenderFlags;
        m_viewDistanceMultiplier = compiler.m_viewDistanceMultiplier;
        m_nCompilerFlag = compiler.m_nCompilerFlag;
    }

    ModelCompiler::~ModelCompiler()
    {
        RemoveStatObj();
        DeleteAllRenderNodes();
    }

    _smart_ptr<IMaterial>  ModelCompiler::GetMaterialFromBaseObj(CBaseObject* pObj) const
    {
        _smart_ptr<IMaterial>  pMaterial = NULL;
        if (pObj)
        {
            QString materialName(pObj->GetMaterialName());
            CMaterial* pObjMaterial = pObj->GetMaterial();
            if (pObjMaterial)
            {
                pMaterial = pObjMaterial->GetMatInfo();
            }
        }
        return pMaterial;
    }

    void ModelCompiler::RemoveStatObj()
    {
        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            if (m_pStatObj[shelfID])
            {
                m_pStatObj[shelfID]->Release();
                m_pStatObj[shelfID] = NULL;
            }
        }
    }

    void ModelCompiler::CreateStatObj(int nShelfID) const
    {
        if (!m_pStatObj[nShelfID])
        {
            m_pStatObj[nShelfID] = GetIEditor()->Get3DEngine()->CreateStatObj();
            m_pStatObj[nShelfID]->AddRef();
        }
    }

    void ModelCompiler::InvalidateStatObj(IStatObj* pStatObj, bool bPhysics)
    {
        if (!pStatObj)
        {
            return;
        }
        IIndexedMesh* pIndexedMesh = pStatObj->GetIndexedMesh();
        if (pIndexedMesh)
        {
            CMesh* pMesh = pIndexedMesh->GetMesh();
            if (!pMesh->m_pIndices)
            {
                bPhysics = false;
            }
        }
        pStatObj->Invalidate(bPhysics);
        int nSubObjCount = pStatObj->GetSubObjectCount();
        for (int i = 0; i < nSubObjCount; ++i)
        {
            IStatObj::SSubObject* pSubObject = pStatObj->GetSubObject(i);
            if (pSubObject && pSubObject->pStatObj)
            {
                pSubObject->pStatObj->Invalidate(bPhysics);
            }
        }
    }

    void ModelCompiler::Compile(CBaseObject* pBaseObject, Model* pModel, ShelfID shelfID, bool bUpdateOnlyRenderNode)
    {
        MODEL_SHELF_RECONSTRUCTOR(pModel);

        int nSubdivisionLevel = pModel->GetSubdivisionLevel();
        if (nSubdivisionLevel == 0)
        {
            if (shelfID == -1)
            {
                for (ShelfID id = 0; id < kMaxShelfCount; ++id)
                {
                    pModel->SetShelf(id);
                    if (!bUpdateOnlyRenderNode)
                    {
                        UpdateMesh(pBaseObject, pModel);
                    }
                    UpdateRenderNode(pBaseObject, pModel);
                }
            }
            else
            {
                pModel->SetShelf(shelfID);
                if (!bUpdateOnlyRenderNode)
                {
                    UpdateMesh(pBaseObject, pModel);
                }
                UpdateRenderNode(pBaseObject, pModel);
            }
            pModel->SetSubdivisionResult(NULL);
        }
        else
        {
            if (!bUpdateOnlyRenderNode)
            {
                RemoveStatObj();

                if (!m_pStatObj[0])
                {
                    CreateStatObj(0);
                }

                int nTessFactor = pModel->GetTessFactor();

                SubdivisionModifier subdivisionModifier;
                SSubdivisionContext sc = subdivisionModifier.CreateSubdividedMesh(pModel, nSubdivisionLevel, nTessFactor);

                bool bDisplayBackface = pModel->GetModeFlag() & eDesignerMode_DisplayBackFace;
                _smart_ptr<IMaterial>  pMaterial = GetMaterialFromBaseObj(pBaseObject);

                std::vector<FlexibleMeshPtr> meshes;
                sc.fullPatches->CreateMeshFaces(meshes, bDisplayBackface, pModel->IsSmoothingSurfaceForSubdividedMeshEnabled());

                pModel->SetSubdivisionResult(sc.fullPatches);

                if (meshes.size() == 1)
                {
                    IIndexedMesh* pMesh = m_pStatObj[0]->GetIndexedMesh();
                    FillMesh(*meshes[0], pMesh);
                    m_pStatObj[0]->SetMaterial(pMaterial);
#if defined(AZ_PLATFORM_WINDOWS)
                    pMesh->Optimize();
#endif
                    InvalidateStatObj(m_pStatObj[0], CheckFlags(eCompiler_Physicalize));
                }
                else
                {
                    for (int i = 0, iSubObjCount(meshes.size()); i < iSubObjCount; ++i)
                    {
                        m_pStatObj[0]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
                    }

                    for (int i = 0, iSubObjCount(m_pStatObj[0]->GetSubObjectCount()); i < iSubObjCount; ++i)
                    {
                        IStatObj* pSubObj = m_pStatObj[0]->GetSubObject(i)->pStatObj;
                        pSubObj->SetMaterial(pMaterial);
                        IIndexedMesh* pMesh = pSubObj->GetIndexedMesh();
                        FillMesh(*meshes[i], pMesh);
#if defined(AZ_PLATFORM_WINDOWS)
                        pMesh->Optimize();
#endif
                        InvalidateStatObj(pSubObj, CheckFlags(eCompiler_Physicalize));
                        pSubObj->m_eStreamingStatus = ecss_Ready;
                    }

                    AABB aabb = pModel->GetBoundBox();
                    m_pStatObj[0]->SetBBoxMin(aabb.min);
                    m_pStatObj[0]->SetBBoxMax(aabb.max);
                }
            }
            UpdateRenderNode(pBaseObject, pModel);
        }
    }

    bool ModelCompiler::UpdateMesh(CBaseObject* pBaseObject, Model* pModel)
    {
        if (pModel == NULL)
        {
            return false;
        }

        ShelfID shelfID = pModel->GetShelf();
        MODEL_SHELF_RECONSTRUCTOR(pModel);

        pModel->SetShelf(shelfID);
        if (!pModel->HasClosedPolygon(shelfID))
        {
            DeleteRenderNode(shelfID);
            if (m_pStatObj[shelfID])
            {
                m_pStatObj[shelfID]->Release();
                m_pStatObj[shelfID] = NULL;
            }
            return true;
        }

        RemoveStatObj();

        if (!m_pStatObj[shelfID])
        {
            CreateStatObj(shelfID);
        }

        _smart_ptr<IMaterial>  pMaterial = GetMaterialFromBaseObj(pBaseObject);

        int prevSubObjCount = m_pStatObj[shelfID]->GetSubObjectCount();
        bool bDisplayBackface = pModel->GetModeFlag() & eDesignerMode_DisplayBackFace;

        std::vector<IStatObj*> statObjList;
        std::vector<bool> generateBackFaces;
        if (bDisplayBackface)
        {
            if (prevSubObjCount == 0)
            {
                m_pStatObj[shelfID]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
                m_pStatObj[shelfID]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
            }
            statObjList.push_back(m_pStatObj[shelfID]->GetSubObject(0)->pStatObj);
            generateBackFaces.push_back(false);
            statObjList.push_back(m_pStatObj[shelfID]->GetSubObject(1)->pStatObj);
            generateBackFaces.push_back(true);
        }
        else
        {
            if (prevSubObjCount > 0)
            {
                m_pStatObj[shelfID]->GetIndexedMesh()->SetSubSetCount(0);
                m_pStatObj[shelfID]->SetSubObjectCount(0);
            }
            statObjList.push_back(m_pStatObj[shelfID]);
            generateBackFaces.push_back(false);
        }

        for (int i = 0, iSize(statObjList.size()); i < iSize; ++i)
        {
            IIndexedMesh* pMesh = statObjList[i]->GetIndexedMesh();
            assert(pMesh);

            statObjList[i]->SetMaterial(pMaterial);

            bool bSuccessGenerateMesh = GenerateIndexMesh(pModel, pMesh, generateBackFaces[i]);
            if (!bSuccessGenerateMesh)
            {
                continue;
            }

#if defined(AZ_PLATFORM_WINDOWS)
            pMesh->Optimize();
#endif
            InvalidateStatObj(statObjList[i], CheckFlags(eCompiler_Physicalize));
            statObjList[i]->m_eStreamingStatus = ecss_Ready;
        }

        int nStatObjFlag = m_pStatObj[shelfID]->GetFlags();
        if (m_pStatObj[shelfID]->GetSubObjectCount() > 0)
        {
            AABB aabb = pModel->GetBoundBox();
            m_pStatObj[shelfID]->SetBBoxMin(aabb.min);
            m_pStatObj[shelfID]->SetBBoxMax(aabb.max);
            m_pStatObj[shelfID]->SetFlags(nStatObjFlag | STATIC_OBJECT_COMPOUND);
            m_pStatObj[shelfID]->m_eStreamingStatus = ecss_Ready;
        }
        else
        {
            m_pStatObj[shelfID]->SetFlags(nStatObjFlag & (~STATIC_OBJECT_COMPOUND));
        }

        return true;
    }

    bool ModelCompiler::GenerateIndexMesh(Model* pModel, IIndexedMesh* pMesh, bool bGenerateBackFaces)
    {
        if (!pModel)
        {
            return false;
        }

        ShelfID shelfID = pModel->GetShelf();
        MODEL_SHELF_RECONSTRUCTOR(pModel);

        SmoothingGroupManager* pSmoothingGroupMgr = pModel->GetSmoothingGroupMgr();

        pModel->SetShelf(shelfID);
        std::vector<PolygonPtr> polygonList;
        for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
        {
            PolygonPtr pPolygon = pModel->GetPolygon(i);
            if (pSmoothingGroupMgr->GetSmoothingGroupID(pPolygon) == NULL)
            {
                polygonList.push_back(pPolygon);
            }
        }

        FlexibleMesh mesh;
        mesh.Reserve(1000);

        for (int i = 0, iSize(polygonList.size()); i < iSize; ++i)
        {
            if (polygonList[i]->CheckFlags(ePolyFlag_Hidden))
            {
                continue;
            }
            JoinTwoMeshes(mesh, *(polygonList[i]->GetTriangles(bGenerateBackFaces)));
        }

        std::vector<std::pair<string, DesignerSmoothingGroupPtr> > smoothingGroupList = pSmoothingGroupMgr->GetSmoothingGroupList();
        for (int i = 0, iSmoothingGroupCount(smoothingGroupList.size()); i < iSmoothingGroupCount; ++i)
        {
            DesignerSmoothingGroupPtr pSmoothingGroup = smoothingGroupList[i].second;
            JoinTwoMeshes(mesh, pSmoothingGroup->GetFlexibleMesh(bGenerateBackFaces));
        }

        if (mesh.IsValid())
        {
            FillMesh(mesh, pMesh);
        }
        else
        {
            pMesh->FreeStreams();
            return false;
        }

        return true;
    }

    void ModelCompiler::SetRenderFlags(int nRenderFlag)
    {
        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            m_RenderFlags = nRenderFlag;
            if (m_pRenderNode[shelfID])
            {
                m_pRenderNode[shelfID]->SetRndFlags(m_RenderFlags);
            }
        }
    }

    void ModelCompiler::SetStaticObjFlags(int nStaticObjFlag)
    {
        if (!m_pStatObj[0])
        {
            CreateStatObj(0);
        }
        m_pStatObj[0]->SetFlags(nStaticObjFlag);
    }

    int ModelCompiler::GetStaticObjFlags() const
    {
        if (!m_pStatObj[0])
        {
            CreateStatObj(0);
        }
        return m_pStatObj[0]->GetFlags();
    }

    void ModelCompiler::DisplayTriangulation(CBaseObject* pBaseObject, Model* pModel, DisplayContext& dc)
    {
        MODEL_SHELF_RECONSTRUCTOR(pModel);

        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            pModel->SetShelf(shelfID);

            if (!pModel->HasClosedPolygon(shelfID))
            {
                continue;
            }

            if (!m_pStatObj[shelfID])
            {
                if (!UpdateMesh(pBaseObject, pModel))
                {
                    continue;
                }
            }

            IIndexedMesh* pMesh;
            if (m_pStatObj[shelfID]->GetSubObjectCount() == 2)
            {
                pMesh = m_pStatObj[shelfID]->GetSubObject(0)->pStatObj->GetIndexedMesh();
            }
            else
            {
                pMesh = m_pStatObj[shelfID]->GetIndexedMesh();
            }

            if (pMesh == NULL)
            {
                return;
            }

            IIndexedMesh::SMeshDescription meshDesc;
            pMesh->GetMeshDescription(meshDesc);

            dc.SetColor(Vec3(0.5f, 0.5f, 0.5f));
            float oldLineWidth = dc.GetLineWidth();
            dc.SetLineWidth(1.0f);

            if (meshDesc.m_pFaces)
            {
                for (int i = 0, iFaceCount(meshDesc.m_nFaceCount); i < iFaceCount; ++i)
                {
                    Vec3 v[3] = {
                        meshDesc.m_pVerts[meshDesc.m_pFaces[i].v[0]],
                        meshDesc.m_pVerts[meshDesc.m_pFaces[i].v[1]],
                        meshDesc.m_pVerts[meshDesc.m_pFaces[i].v[2]]
                    };
                    dc.DrawPolyLine(v, 3);
                }
            }
            else if (meshDesc.m_pIndices)
            {
                for (int i = 0, iIndexCount(meshDesc.m_nIndexCount); i < iIndexCount; i += 3)
                {
                    Vec3 v[3] = {
                        meshDesc.m_pVerts[meshDesc.m_pIndices[i]],
                        meshDesc.m_pVerts[meshDesc.m_pIndices[i + 1]],
                        meshDesc.m_pVerts[meshDesc.m_pIndices[i + 2]]
                    };
                    dc.DrawPolyLine(v, 3);
                }
            }

            dc.SetLineWidth(oldLineWidth);
        }
    }

    void ModelCompiler::DeleteAllRenderNodes()
    {
        for (ShelfID shelfID = 0; shelfID < kMaxShelfCount; ++shelfID)
        {
            DeleteRenderNode(shelfID);
        }
    }

    void ModelCompiler::DeleteRenderNode(ShelfID shelfID)
    {
        if (m_pRenderNode[shelfID])
        {
            GetIEditor()->Get3DEngine()->DeleteRenderNode(m_pRenderNode[shelfID]);
            m_pRenderNode[shelfID] = NULL;
        }
    }

    void ModelCompiler::UpdateRenderNode(CBaseObject* pBaseObject, Model* pModel)
    {
        ShelfID shelfID = pModel->GetShelf();

        if (m_pStatObj[shelfID] == NULL)
        {
            DeleteRenderNode(shelfID);
            return;
        }

        if (!pModel->GetPolygonCount())
        {
            return;
        }

        if (!m_pRenderNode[shelfID])
        {
            m_pRenderNode[shelfID] = GetIEditor()->Get3DEngine()->CreateRenderNode(eERType_Brush);
            if (pBaseObject)
            {
                m_pRenderNode[shelfID]->SetEditorObjectId(pBaseObject->GetId().Data1);
            }
        }

        if (gSettings.viewports.bHighlightSelectedGeometry)
        {
            m_RenderFlags |= ERF_SELECTED;
        }
        else
        {
            m_RenderFlags &= ~ERF_SELECTED;
        }

        m_pRenderNode[shelfID]->SetRndFlags(m_RenderFlags);
        m_pRenderNode[shelfID]->SetViewDistanceMultiplier(m_viewDistanceMultiplier);
        m_pRenderNode[shelfID]->SetMinSpec(pBaseObject->GetMinSpec());
        m_pRenderNode[shelfID]->SetMaterialLayers(pBaseObject->GetMaterialLayersMask());

        if (qobject_cast<DesignerObject*>(pBaseObject))
        {
            QString generatedFilename;
            static_cast<DesignerObject*>(pBaseObject)->GenerateGameFilename(generatedFilename);
            m_pStatObj[shelfID]->SetFilePath(generatedFilename.toUtf8().constData());
        }

        Matrix34A mtx = pBaseObject->GetWorldTM();

        m_pStatObj[shelfID]->SetMaterial(GetMaterialFromBaseObj(pBaseObject));
        m_pRenderNode[shelfID]->SetEntityStatObj(0, m_pStatObj[shelfID], &mtx);
        m_pRenderNode[shelfID]->SetMaterial(m_pStatObj[shelfID]->GetMaterial());

        m_RenderFlags = m_pRenderNode[shelfID]->GetRndFlags();

        if (m_pRenderNode[shelfID])
        {
            if (CheckFlags(eCompiler_Physicalize))
            {
                m_pRenderNode[shelfID]->Physicalize();
            }
            else
            {
                m_pRenderNode[shelfID]->Dephysicalize();
            }
        }
    }

    bool ModelCompiler::GetIStatObj(_smart_ptr<IStatObj>* pOutStatObj)
    {
        if (!m_pStatObj[0])
        {
            return false;
        }

        if (pOutStatObj)
        {
            *pOutStatObj = m_pStatObj[0];
        }

        return IsValid();
    }

    void ModelCompiler::SaveToCgf(const char* filename)
    {
        _smart_ptr<IStatObj> pObj;
        if (GetIStatObj(&pObj))
        {
            pObj->SaveToCGF(filename, NULL, true);
        }
    }

    void ModelCompiler::SaveMesh(CArchive& ar, CBaseObject* pObj, Model* pModel)
    {
        if (!m_pStatObj[0])
        {
            return;
        }

        int nSubObjectCount = m_pStatObj[0]->GetSubObjectCount();

        std::vector<IStatObj*> pStatObjs;
        if (nSubObjectCount == 0)
        {
            pStatObjs.push_back(m_pStatObj[0]);
        }
        else if (nSubObjectCount > 0)
        {
            for (int i = 0; i < nSubObjectCount; ++i)
            {
                pStatObjs.push_back(m_pStatObj[0]->GetSubObject(i)->pStatObj);
            }
        }

        ar.Write(&nSubObjectCount, sizeof(nSubObjectCount));

        for (int k = 0, iStatObjCount(pStatObjs.size()); k < iStatObjCount; ++k)
        {
            IIndexedMesh* pMesh = pStatObjs[k]->GetIndexedMesh();
            if (!pMesh)
            {
                return;
            }

            int nPositionCount = pMesh->GetVertexCount();
            Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
            Vec3* const normals = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);

            int nTexCoordCount = pMesh->GetTexCoordCount();
            SMeshTexCoord* const texcoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);

            int nFaceCount = pMesh->GetFaceCount();
            SMeshFace* faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
            if (nFaceCount == 0 || faces == NULL)
            {
                pMesh->RestoreFacesFromIndices();
                faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
                nFaceCount = pMesh->GetFaceCount();
            }

            int nIndexCount = pMesh->GetIndexCount();
            vtx_idx* const indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);

            int nSubsetCount = pMesh->GetSubSetCount();

            ar.Write(&nPositionCount, sizeof(int));
            ar.Write(&nTexCoordCount, sizeof(int));
            ar.Write(&nFaceCount, sizeof(int));
            ar.Write(&nSubsetCount, sizeof(int));

            ar.Write(positions, sizeof(Vec3) * nPositionCount);
            ar.Write(normals, sizeof(Vec3) * nPositionCount);
            ar.Write(texcoords, sizeof(SMeshTexCoord) * nTexCoordCount);

            for (int i = 0; i < nSubsetCount; ++i)
            {
                const SMeshSubset& subset = pMesh->GetSubSet(i);
                for (int k = 0; k < subset.nNumIndices / 3; ++k)
                {
                    faces[(subset.nFirstIndexId / 3) + k].nSubset = i;
                }
            }
            ar.Write(faces, sizeof(SMeshFace) * nFaceCount);

            for (int i = 0; i < nSubsetCount; ++i)
            {
                SMeshSubset& subset = const_cast<SMeshSubset&>(pMesh->GetSubSet(i));
                ar.Write(&subset.vCenter, sizeof(Vec3));
                ar.Write(&subset.fRadius, sizeof(float));
                ar.Write(&subset.fTexelDensity, sizeof(float));
                ar.Write(&subset.nFirstIndexId, sizeof(int));
                ar.Write(&subset.nNumIndices, sizeof(int));
                ar.Write(&subset.nFirstVertId, sizeof(int));
                ar.Write(&subset.nNumVerts, sizeof(int));
                ar.Write(&subset.nMatID, sizeof(int));
                ar.Write(&subset.nMatFlags, sizeof(int));
                ar.Write(&subset.nPhysicalizeType, sizeof(int));
            }
        }
    }

    bool ModelCompiler::LoadMesh(CArchive& ar, CBaseObject* pObj, Model* pModel)
    {
        int nSubObjectCount = 0;
        ar.Read(&nSubObjectCount, sizeof(nSubObjectCount));

        if (!m_pStatObj[0])
        {
            m_pStatObj[0] = GetIEditor()->Get3DEngine()->CreateStatObj();
            m_pStatObj[0]->AddRef();
        }

        std::vector<IStatObj*> statObjList;
        if (nSubObjectCount == 2)
        {
            m_pStatObj[0]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
            m_pStatObj[0]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
            m_pStatObj[0]->GetIndexedMesh()->FreeStreams();
            statObjList.push_back(m_pStatObj[0]->GetSubObject(0)->pStatObj);
            statObjList.push_back(m_pStatObj[0]->GetSubObject(1)->pStatObj);
        }
        else
        {
            statObjList.push_back(m_pStatObj[0]);
        }

        _smart_ptr<IMaterial>  pMaterial = GetMaterialFromBaseObj(pObj);
        m_pStatObj[0]->SetMaterial(pMaterial);

        for (int k = 0, iCount(statObjList.size()); k < iCount; ++k)
        {
            int nPositionCount = 0;
            int nTexCoordCount = 0;
            int nFaceCount = 0;
            ar.Read(&nPositionCount, sizeof(int));
            ar.Read(&nTexCoordCount, sizeof(int));
            ar.Read(&nFaceCount, sizeof(int));
            if (nPositionCount <= 0 || nTexCoordCount <= 0 || nFaceCount <= 0)
            {
                assert(0);
                return false;
            }

            int nSubsetCount = 0;
            ar.Read(&nSubsetCount, sizeof(int));

            IIndexedMesh* pMesh = statObjList[k]->GetIndexedMesh();
            if (!pMesh)
            {
                assert(0);
                return false;
            }

            pMesh->FreeStreams();
            pMesh->SetVertexCount(nPositionCount);
            pMesh->SetFaceCount(nFaceCount);
            pMesh->SetIndexCount(0);
            pMesh->SetTexCoordCount(nTexCoordCount);

            Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
            Vec3* const normals = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);
            SMeshTexCoord* const texcoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
            SMeshFace* const faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);

            ar.Read(positions, sizeof(Vec3) * nPositionCount);
            ar.Read(normals, sizeof(Vec3) * nPositionCount);
            ar.Read(texcoords, sizeof(SMeshTexCoord) * nTexCoordCount);
            ar.Read(faces, sizeof(SMeshFace) * nFaceCount);

            pMesh->SetSubSetCount(nSubsetCount);
            for (int i = 0; i < nSubsetCount; ++i)
            {
                SMeshSubset subset;
                ar.Read(&subset.vCenter, sizeof(Vec3));
                ar.Read(&subset.fRadius, sizeof(float));
                ar.Read(&subset.fTexelDensity, sizeof(float));
                ar.Read(&subset.nFirstIndexId, sizeof(int));
                ar.Read(&subset.nNumIndices, sizeof(int));
                ar.Read(&subset.nFirstVertId, sizeof(int));
                ar.Read(&subset.nNumVerts, sizeof(int));
                ar.Read(&subset.nMatID, sizeof(int));
                ar.Read(&subset.nMatFlags, sizeof(int));
                ar.Read(&subset.nPhysicalizeType, sizeof(int));

                pMesh->SetSubsetBounds(i, subset.vCenter, subset.fRadius);
                pMesh->SetSubsetIndexVertexRanges(i, subset.nFirstIndexId, subset.nNumIndices, subset.nFirstVertId, subset.nNumVerts);
                pMesh->SetSubsetMaterialId(i, subset.nMatID);
                pMesh->SetSubsetMaterialProperties(i, subset.nMatFlags, subset.nPhysicalizeType, eVF_P3F_C4B_T2F);
            }

#if defined(AZ_PLATFORM_WINDOWS)
            pMesh->Optimize();
#endif

            statObjList[k]->SetMaterial(pMaterial);
            InvalidateStatObj(statObjList[k], CheckFlags(eCompiler_Physicalize));
        }

        AABB aabb = pModel->GetBoundBox(0);
        m_pStatObj[0]->SetBBoxMin(aabb.min);
        m_pStatObj[0]->SetBBoxMax(aabb.max);

        UpdateRenderNode(pObj, pModel);

        return true;
    }

    bool ModelCompiler::SaveMesh(int nVersion, std::vector<char>& buffer, CBaseObject* pObj, Model* pModel)
    {
        if (!m_pStatObj[0])
        {
            return false;
        }

        int32 nSubObjectCount = (int32)m_pStatObj[0]->GetSubObjectCount();

        std::vector<IStatObj*> pStatObjs;
        if (nSubObjectCount == 0)
        {
            pStatObjs.push_back(m_pStatObj[0]);
        }
        else if (nSubObjectCount > 0)
        {
            for (int i = 0; i < nSubObjectCount; ++i)
            {
                pStatObjs.push_back(m_pStatObj[0]->GetSubObject(i)->pStatObj);
            }
        }

        Write2Buffer(buffer, &nSubObjectCount, sizeof(int32));

        if (nVersion == 2)
        {
            int32 nStaticObjFlags = (int32)m_pStatObj[0]->GetFlags();
            Write2Buffer(buffer, &nStaticObjFlags, sizeof(int32));
        }

        for (int k = 0, iStatObjCount(pStatObjs.size()); k < iStatObjCount; ++k)
        {
            IIndexedMesh* pMesh = pStatObjs[k]->GetIndexedMesh();
            if (!pMesh)
            {
                return false;
            }
            int nIndexCount = pMesh->GetIndexCount();
            vtx_idx* const indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);
            if (nIndexCount == 0 || indices == 0)
            {
#if defined(AZ_PLATFORM_WINDOWS)
                pMesh->Optimize();
#endif
                break;
            }
        }

        for (int k = 0, iStatObjCount(pStatObjs.size()); k < iStatObjCount; ++k)
        {
            IIndexedMesh* pMesh = pStatObjs[k]->GetIndexedMesh();
            if (!pMesh)
            {
                return false;
            }

            int32 nPositionCount = (int32)pMesh->GetVertexCount();
            int32 nTexCoordCount = (int32)pMesh->GetTexCoordCount();
            int32 nFaceCount = (int32)pMesh->GetFaceCount();
            int32 nTangentCount = (int32)pMesh->GetTangentCount();
            int32 nSubsetCount = (int32)pMesh->GetSubSetCount();
            int32 nIndexCount = (int32)pMesh->GetIndexCount();

            Write2Buffer(buffer, &nPositionCount, sizeof(int32));
            Write2Buffer(buffer, &nTexCoordCount, sizeof(int32));
            if (nVersion == 0)
            {
                Write2Buffer(buffer, &nFaceCount, sizeof(int32));
            }
            else if (nVersion >= 1)
            {
                if (sizeof(vtx_idx) == sizeof(uint16) && nIndexCount >= 0xFFFFFFFF)
                {
                    return false;
                }
                Write2Buffer(buffer, &nIndexCount, sizeof(int32));
                Write2Buffer(buffer, &nTangentCount, sizeof(int32));
            }
            Write2Buffer(buffer, &nSubsetCount, sizeof(int32));

            Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
            Vec3* const normals = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);
            SMeshTexCoord* const texcoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
            SMeshFace* faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
            if (nVersion == 0 && (nFaceCount == 0 || faces == NULL))
            {
                pMesh->RestoreFacesFromIndices();
                faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
                nFaceCount = pMesh->GetFaceCount();
            }
            vtx_idx* const indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);
            uint16* const indices16 = nVersion >= 1 ? (sizeof(vtx_idx) == sizeof(uint16) ? (uint16*)indices : new uint16[nIndexCount]) : NULL;
            if (nVersion >= 1 && (uint16*)indices != indices16)
            {
                for (int i = 0; i < nIndexCount; ++i)
                {
                    indices16[i] = (uint16)indices[i];
                }
            }

            Write2Buffer(buffer, positions, sizeof(Vec3) * nPositionCount);
            Write2Buffer(buffer, normals, sizeof(Vec3) * nPositionCount);
            Write2Buffer(buffer, texcoords, sizeof(SMeshTexCoord) * nTexCoordCount);
            if (nVersion == 0)
            {
                for (int i = 0; i < nSubsetCount; ++i)
                {
                    const SMeshSubset& subset = pMesh->GetSubSet(i);
                    for (int k = 0; k < subset.nNumIndices / 3; ++k)
                    {
                        faces[(subset.nFirstIndexId / 3) + k].nSubset = i;
                    }
                }
                Write2Buffer(buffer, faces, sizeof(SMeshFace) * nFaceCount);
            }
            else if (nVersion >= 1)
            {
                Write2Buffer(buffer, indices16, sizeof(uint16) * nIndexCount);
                if (nTangentCount > 0)
                {
                    SMeshTangents* tangents = pMesh->GetMesh()->GetStreamPtr<SMeshTangents>(CMesh::TANGENTS);
                    Write2Buffer(buffer, tangents, sizeof(SMeshTangents) * nTangentCount);
                }
            }

            for (int i = 0; i < nSubsetCount; ++i)
            {
                const SMeshSubset& subset = pMesh->GetSubSet(i);
                Write2Buffer(buffer, &subset.vCenter, sizeof(Vec3));
                Write2Buffer(buffer, &subset.fRadius, sizeof(float));
                Write2Buffer(buffer, &subset.fTexelDensity, sizeof(float));

                int32 nFirstIndexId = (int32)subset.nFirstIndexId;
                int32 nNumIndices = (int32)subset.nNumIndices;
                int32 nFirstVertId = (int32)subset.nFirstVertId;
                int32 nNumVerts = (int32)subset.nNumVerts;
                int32 nMatID = (int32)subset.nMatID;
                int32 nMatFlags = (int32)subset.nMatFlags;
                int32 nPhysicalizeType = (int32)subset.nPhysicalizeType;

                Write2Buffer(buffer, &nFirstIndexId, sizeof(int32));
                Write2Buffer(buffer, &nNumIndices, sizeof(int32));
                Write2Buffer(buffer, &nFirstVertId, sizeof(int32));
                Write2Buffer(buffer, &nNumVerts, sizeof(int32));
                Write2Buffer(buffer, &nMatID, sizeof(int32));
                Write2Buffer(buffer, &nMatFlags, sizeof(int32));
                Write2Buffer(buffer, &nPhysicalizeType, sizeof(int32));
            }

            if (indices16 && (uint16*)indices != indices16)
            {
                delete [] indices16;
            }
        }

        return true;
    }

    bool ModelCompiler::LoadMesh(int nVersion, std::vector<char>& buffer, CBaseObject* pObj, Model* pModel)
    {
        IStatObj* pStatObj = GetIEditor()->Get3DEngine()->LoadDesignerObject(nVersion, &buffer[0], buffer.size());
        if (pStatObj)
        {
            RemoveStatObj();
            pStatObj->AddRef();
            m_pStatObj[0] = pStatObj;

            _smart_ptr<IMaterial>  pMaterial = GetMaterialFromBaseObj(pObj);
            m_pStatObj[0]->SetMaterial(pMaterial);

            AABB aabb = pModel->GetBoundBox(0);
            m_pStatObj[0]->SetBBoxMin(aabb.min);
            m_pStatObj[0]->SetBBoxMax(aabb.max);
            m_pStatObj[0]->m_eStreamingStatus = ecss_Ready;
            UpdateRenderNode(pObj, pModel);
            return true;
        }
        return false;
    }

    bool ModelCompiler::IsValid() const
    {
        return m_pStatObj[0] || m_pStatObj[1];
    }

    int ModelCompiler::GetPolygonCount() const
    {
        if (!m_pStatObj[0] && !m_pStatObj[1])
        {
            return 0;
        }

        int nPositionCount = 0;
        for (int i = 0; i < 2; ++i)
        {
            if (m_pStatObj[i])
            {
                int nSubObjectCount = m_pStatObj[i]->GetSubObjectCount();
                for (int k = 0; k < nSubObjectCount; ++k)
                {
                    if (m_pStatObj[i]->GetSubObject(k) && m_pStatObj[i]->GetSubObject(k)->pStatObj)
                    {
                        IIndexedMesh* pMesh = m_pStatObj[i]->GetSubObject(k)->pStatObj->GetIndexedMesh();
                        nPositionCount += pMesh->GetVertexCount();
                    }
                }

                IIndexedMesh* pMesh = m_pStatObj[i]->GetIndexedMesh();
                if (!pMesh)
                {
                    continue;
                }

                nPositionCount += pMesh->GetVertexCount();
            }
        }

        return nPositionCount;
    }
};