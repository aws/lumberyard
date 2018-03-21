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
#include "Converter.h"
#include "Objects/DesignerObject.h"
#include "Core/ModelCompiler.h"
#include "Core/Model.h"
#include "Core/BrushHelper.h"
#include "Core/BSPTree2D.h"
#include "Objects/BrushObject.h"
#include "Geometry/EdGeometry.h"
#include "Objects/EntityObject.h"
#include "CGFContent.h"
#include "Material/MaterialManager.h"
#include "Tools/ToolCommon.h"

namespace CD
{
    bool Converter::CreateNewDesignerObject()
    {
        std::vector<SSelectedMesh> pSelectedMeshes;
        GetSelectedObjects(pSelectedMeshes);

        if (pSelectedMeshes.empty())
        {
            return false;
        }

        CUndo undo("Create Designer Objects");
        GetIEditor()->ClearSelection();

        std::vector<DesignerObject*> pDesignerObjects;
        CreateDesignerObjects(pSelectedMeshes, pDesignerObjects);

        int iSizeOfDesignerObjects(pDesignerObjects.size());

        if (iSizeOfDesignerObjects <= 0)
        {
            return false;
        }

        const float kMarginBetweenOldAndNew = 1.2f;

        for (int i = 0; i < iSizeOfDesignerObjects; ++i)
        {
            AABB aabb;
            pDesignerObjects[i]->GetBoundBox(aabb);
            float gapX = (aabb.max.x - aabb.min.x) * kMarginBetweenOldAndNew;
            Matrix34 newTM(pDesignerObjects[i]->GetWorldTM());
            newTM.SetTranslation(newTM.GetTranslation() + Vec3(gapX, 0, 0));
            pDesignerObjects[i]->SetWorldTM(newTM);
            GetIEditor()->SelectObject(pDesignerObjects[i]);
        }

        return true;
    }

    bool Converter::ConvertToDesignerObject()
    {
        std::vector<SSelectedMesh> pSelectedMeshes;
        GetSelectedObjects(pSelectedMeshes);

        if (pSelectedMeshes.empty())
        {
            return false;
        }

        CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
        if (pSelection == NULL)
        {
            return false;
        }

        CUndo undo("Convert to Designer Objects");

        GetIEditor()->GetObjectManager()->ClearSelection();

        std::vector<DesignerObject*> pDesignerObjects;
        CreateDesignerObjects(pSelectedMeshes, pDesignerObjects);

        int iSizeOfDesignerObjects(pDesignerObjects.size());
        if (iSizeOfDesignerObjects <= 0)
        {
            return false;
        }

        for (int i = 0; i < iSizeOfDesignerObjects; ++i)
        {
            GetIEditor()->SelectObject(pDesignerObjects[i]);
        }

        for (int i = 0, iSizeOfSelectedObjects(pSelectedMeshes.size()); i < iSizeOfSelectedObjects; ++i)
        {
            GetIEditor()->DeleteObject(pSelectedMeshes[i].m_pOriginalObject);
        }

        return true;
    }

    void Converter::CreateDesignerObjects(std::vector<SSelectedMesh>& pSelectedMeshes, std::vector<DesignerObject*>& pOutDesignerObjects)
    {
        for (int i = 0, iSizeOfSelectedObjs(pSelectedMeshes.size()); i < iSizeOfSelectedObjs; ++i)
        {
            SSelectedMesh& obj = pSelectedMeshes[i];

            if (obj.m_pIndexedMesh == NULL)
            {
                continue;
            }

            DesignerObject* pDesignerObj = CreateDesignerObject(obj.m_pIndexedMesh);

            if (obj.m_bLoadedIndexedMeshFromFile)
            {
                delete obj.m_pIndexedMesh;
            }

            if (pDesignerObj == NULL)
            {
                continue;
            }

            if (obj.m_pMaterial)
            {
                pDesignerObj->SetMaterial(obj.m_pMaterial);
            }

            pDesignerObj->SetWorldTM(obj.m_worldTM);
            pOutDesignerObjects.push_back(pDesignerObj);
        }
    }

    void Converter::GetSelectedObjects(std::vector<SSelectedMesh>& pObjects) const
    {
        CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
        if (pSelection == NULL)
        {
            return;
        }

        for (int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i)
        {
            SSelectedMesh s;
            CBaseObject* pObj = pSelection->GetObject(i);

            if (pObj->GetMaterial())
            {
                s.m_pMaterial = pObj->GetMaterial();
            }

            if (qobject_cast<CBrushObject*>(pObj))
            {
                CBrushObject* pBrushObj = (CBrushObject*)pObj;
                if (pBrushObj->GetGeometry())
                {
                    s.m_pIndexedMesh = pBrushObj->GetGeometry()->GetIndexedMesh();
                    IStatObj* pStatObj = pBrushObj->GetGeometry()->GetIStatObj();
                    if (pStatObj)
                    {
                        s.m_pMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(pStatObj->GetMaterial());
                    }
                }
            }
            else if (qobject_cast<CEntityObject*>(pObj))
            {
                CEntityObject* pEntityObject = (CEntityObject*)pObj;
                IEntity* pEntity = pEntityObject->GetIEntity();
                if (pEntity)
                {
                    int nSlotCount = pEntity->GetSlotCount();
                    int nCurSlot = 0;
                    while (nCurSlot < nSlotCount)
                    {
                        SEntitySlotInfo slotInfo;
                        if (pEntity->GetSlotInfo(nCurSlot++, slotInfo) == false)
                        {
                            continue;
                        }
                        if (!slotInfo.pStatObj)
                        {
                            continue;
                        }
                        s.m_pIndexedMesh = slotInfo.pStatObj->GetIndexedMesh();
                        s.m_pMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(slotInfo.pStatObj->GetMaterial());
                        if (s.m_pIndexedMesh == NULL)
                        {
                            QString sFilename(slotInfo.pStatObj->GetFilePath());
                            CContentCGF cgf(sFilename.toUtf8().data());
                            if (!GetIEditor()->Get3DEngine()->LoadChunkFileContent(&cgf, sFilename.toUtf8().data()))
                            {
                                continue;
                            }
                            for (int i = 0; i < cgf.GetNodeCount(); i++)
                            {
                                if (cgf.GetNode(i)->type != CNodeCGF::NODE_MESH)
                                {
                                    continue;
                                }
                                CMesh* pMesh = cgf.GetNode(i)->pMesh;
                                if (!pMesh)
                                {
                                    continue;
                                }
                                s.m_pIndexedMesh = GetIEditor()->Get3DEngine()->CreateIndexedMesh();
                                s.m_pIndexedMesh->SetMesh(*pMesh);
                                s.m_bLoadedIndexedMeshFromFile = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (s.m_pIndexedMesh == NULL)
            {
                continue;
            }
            s.m_worldTM = pObj->GetWorldTM();
            s.m_pOriginalObject = pObj;
            pObjects.push_back(s);
        }
    }

    DesignerObject* Converter::CreateDesignerObject(IIndexedMesh* pMesh)
    {
        if (pMesh == NULL)
        {
            return NULL;
        }

        DesignerObject* pDesignerObj = (DesignerObject*)GetIEditor()->NewObject("Designer", "");

        if (!ConvertMeshToDesignerObject(pDesignerObj, pMesh))
        {
            GetIEditor()->DeleteObject(pDesignerObj);
            return NULL;
        }

        return pDesignerObj;
    }

    bool Converter::ConvertMeshToDesignerObject(DesignerObject* pDesignerObject, IIndexedMesh* pMesh)
    {
        if (ConvertMeshToBrushDesigner(pMesh, pDesignerObject->GetModel()))
        {
            CD::UpdateAll(pDesignerObject->MainContext(), CD::eUT_Mesh);
            return true;
        }

        return false;
    }

    bool Converter::ConvertMeshToBrushDesigner(IIndexedMesh* pMesh, CD::Model* pModel)
    {
        if (pMesh == NULL || pModel == NULL)
        {
            return false;
        }

        int numVerts = pMesh->GetVertexCount();
        int numFaces = pMesh->GetIndexCount() / 3;

        IIndexedMesh::SMeshDescription md;
        pMesh->GetMeshDescription(md);

        Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
        Vec3f16* const positionsf16 = pMesh->GetMesh()->GetStreamPtr<Vec3f16>(CMesh::POSITIONSF16);
        SMeshTexCoord* const texCoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
        vtx_idx* const indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);

        if ((positions == NULL && positionsf16 == NULL) || indices == NULL)
        {
            return false;
        }

        int nSubsetCount = pMesh->GetMesh()->m_subsets.size();
        for (int i = 0; i < nSubsetCount; ++i)
        {
            const SMeshSubset& subset = pMesh->GetMesh()->m_subsets[i];
            for (int k = 0; k < subset.nNumIndices; k += 3)
            {
                int i0 = indices[k + 0 + subset.nFirstIndexId];
                int i1 = indices[k + 1 + subset.nFirstIndexId];
                int i2 = indices[k + 2 + subset.nFirstIndexId];

                if (i0 >= numVerts || i0 < 0)
                {
                    continue;
                }
                if (i1 >= numVerts || i1 < 0)
                {
                    continue;
                }
                if (i2 >= numVerts || i2 < 0)
                {
                    continue;
                }

                std::vector<CD::SVertex> vertices(3);

                if (positions)
                {
                    vertices[0].pos = ToBrushVec3(positions[i0]);
                    vertices[1].pos = ToBrushVec3(positions[i1]);
                    vertices[2].pos = ToBrushVec3(positions[i2]);
                }
                else if (positionsf16)
                {
                    vertices[0].pos = ToBrushVec3(positionsf16[i0].ToVec3());
                    vertices[1].pos = ToBrushVec3(positionsf16[i1].ToVec3());
                    vertices[2].pos = ToBrushVec3(positionsf16[i2].ToVec3());
                }

                // To avoid registering degenerated triangles
                BrushVec3 d0 = (vertices[2].pos - vertices[1].pos).GetNormalized();
                BrushVec3 d1 = (vertices[0].pos - vertices[1].pos).GetNormalized();
                if (std::abs(d0.Dot(d1)) >= 1 - kDesignerEpsilon)
                {
                    continue;
                }

                BrushPlane plane(vertices[0].pos, vertices[1].pos, vertices[2].pos);

                if (texCoords)
                {
                    vertices[0].uv = texCoords[i0].GetUV();
                    vertices[1].uv = texCoords[i1].GetUV();
                    vertices[2].uv = texCoords[i2].GetUV();
                }

                CD::PolygonPtr pPolygon = new CD::Polygon(vertices, plane, subset.nMatID, NULL, true);
                pModel->AddPolygon(pPolygon, CD::eOpType_Add);
            }
        }

        return true;
    }

    bool Converter::ConvertSolidXMLToDesignerObject(XmlNodeRef pSolidNode, DesignerObject* pDesignerObject)
    {
        if (pSolidNode->getChildCount() == 0)
        {
            DESIGNER_ASSERT(0);
            return false;
        }

        const char* tag = pSolidNode->getChild(0)->getTag();

        if (!stricmp(tag, "Face"))
        {
            int numFaces = pSolidNode->getChildCount();
            std::vector<SSolidPolygon> polygonlist;
            std::vector<BrushVec3> vertexlist;
            for (int i = 0; i < numFaces; ++i)
            {
                XmlNodeRef faceNode = pSolidNode->getChild(i);
                if (faceNode == NULL)
                {
                    continue;
                }
                if (faceNode->haveAttr("NumberOfPoints"))
                {
                    SSolidPolygon polygon;
                    LoadPolygon(&polygon, faceNode);
                    polygonlist.push_back(polygon);
                    if (i == 0)
                    {
                        LoadVertexList(vertexlist, faceNode);
                    }
                }
                else
                {
                    DESIGNER_ASSERT(!"Can't convert an old type of solid");
                    return true;
                }
            }
            AddPolygonsToDesigner(polygonlist, vertexlist, pDesignerObject);
        }
        else if (!stricmp(tag, "Polygon"))
        {
            std::vector<SSolidPolygon> polylist;
            int numberOfChildren = pSolidNode->getChildCount();
            for (int i = 0; i < numberOfChildren; ++i)
            {
                XmlNodeRef polygonNode = pSolidNode->getChild(i);
                if (polygonNode && !strcmp(polygonNode->getTag(), "Polygon"))
                {
                    SSolidPolygon polygon;
                    LoadPolygon(&polygon, polygonNode);
                    polylist.push_back(polygon);
                }
            }
            std::vector<BrushVec3> vertexlist;
            LoadVertexList(vertexlist, pSolidNode);

            AddPolygonsToDesigner(polylist, vertexlist, pDesignerObject);
        }

        return true;
    }

    void Converter::AddPolygonsToDesigner(const std::vector<SSolidPolygon>& polygonList, const std::vector<BrushVec3>& vList, DesignerObject* pDesignerObject)
    {
        for (int i = 0, iPolySize(polygonList.size()); i < iPolySize; ++i)
        {
            const SSolidPolygon& solidPolygon = polygonList[i];
            std::vector<BrushVec3> reorderedVertexList;
            for (int k = 0, iVIndexCount(solidPolygon.vIndexList.size()); k < iVIndexCount; ++k)
            {
                reorderedVertexList.push_back(vList[solidPolygon.vIndexList[k]]);
            }
            if (reorderedVertexList.size() < 3)
            {
                continue;
            }
            BrushPlane plane(reorderedVertexList[0], reorderedVertexList[1], reorderedVertexList[2]);
            CD::PolygonPtr pPolygon = new CD::Polygon(reorderedVertexList, plane, solidPolygon.matID, &solidPolygon.texinfo, true);

            if (!pPolygon->IsOpen())
            {
                if (pPolygon->GetBSPTree() && !pPolygon->GetBSPTree()->HasNegativeNode())
                {
                    pPolygon->SetPlane(pPolygon->GetPlane().GetInverted());
                }
                pDesignerObject->GetModel()->AddPolygon(pPolygon, CD::eOpType_Add);
            }
        }
    }

    void Converter::LoadTexInfo(CD::STexInfo*  texinfo, const XmlNodeRef& node)
    {
        Vec3 texScale(1, 1, 1);
        Vec3 texShift(0, 0, 0);
        node->getAttr("TexScale", texScale);
        node->getAttr("TexShift", texShift);
        node->getAttr("TexRotate", texinfo->rotate);
        texinfo->scale[0] = texScale.x;
        texinfo->scale[1] = texScale.y;
        texinfo->shift[0] = texShift.x;
        texinfo->shift[1] = texShift.y;
    }

    void Converter::LoadPolygon(SSolidPolygon* polygon, const XmlNodeRef& polygonNode)
    {
        int numberOfPoints = 0;
        polygonNode->getAttr("NumberOfPoints", numberOfPoints);

        int nCount = 0;
        while (1)
        {
            QString attribute;
            attribute = QStringLiteral("v%1").arg(nCount++);
            int16 vertexindex;
            bool ok = polygonNode->getAttr(attribute.toUtf8().data(), vertexindex);
            if (ok == false)
            {
                break;
            }
            polygon->vIndexList.push_back(vertexindex);
        }

        int nMatId;
        polygonNode->getAttr("MatId", nMatId);
        polygon->matID = nMatId;

        LoadTexInfo(&polygon->texinfo, polygonNode);
    }

    void Converter::LoadVertexList(std::vector<BrushVec3>& vertexlist, const XmlNodeRef& node)
    {
        XmlNodeRef vertexNode = node->findChild("Vertex");
        int numberOfVertices;
        vertexNode->getAttr("NumberOfVertices", numberOfVertices);
        int nCount = 0;
        while (1)
        {
            Vec3 position;
            QString attribute;
            attribute = QStringLiteral("p%1").arg(nCount++);
            bool ok = vertexNode->getAttr(attribute.toUtf8().data(), position);
            if (ok == false)
            {
                break;
            }
            vertexlist.push_back(CD::ToBrushVec3(position));
        }
    }
};