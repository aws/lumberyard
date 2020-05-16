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
#include "BrushHelper.h"
#include "Objects/DesignerObject.h"
#include "Objects/PrefabObject.h"
#include "Model.h"
#include "Polygon.h"
#include "BSPTree2D.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/ClipVolumeObject.h"
#include "Tools/DesignerTool.h"
#include "Viewport.h"
#include "Core/PolygonDecomposer.h"
#include "Util/ElementManager.h"

namespace CD
{
    bool UpdateStatObjWithoutBackFaces(CBaseObject* pObj)
    {
        CD::Model* pModel = NULL;
        CD::ModelCompiler* pCompiler = NULL;

        if (GetModel(pObj, pModel) && GetCompiler(pObj, pCompiler))
        {
            bool bDisplayBackFace = pModel->CheckModeFlag(CD::eDesignerMode_DisplayBackFace);
            if (bDisplayBackFace)
            {
                pModel->SetModeFlag(pModel->GetModeFlag() & (~CD::eDesignerMode_DisplayBackFace));
                pCompiler->Compile(pObj, pModel);
                pModel->SetModeFlag(pModel->GetModeFlag() | CD::eDesignerMode_DisplayBackFace);
            }
            return true;
        }
        return false;
    }

    bool UpdateStatObj(CBaseObject* pObj)
    {
        CD::Model* pModel = NULL;
        CD::ModelCompiler* pCompiler = NULL;

        if (GetModel(pObj, pModel) && GetCompiler(pObj, pCompiler))
        {
            pCompiler->Compile(pObj, pModel);
            return true;
        }
        return false;
    }

    bool UpdateGameResource(CBaseObject* pObj)
    {
        if (pObj == NULL)
        {
            return false;
        }

        if (qobject_cast<AreaSolidObject*>(pObj))
        {
            AreaSolidObject* pArea = (AreaSolidObject*)pObj;
            pArea->UpdateGameResource();
            return true;
        }
        else if (qobject_cast<ClipVolumeObject*>(pObj))
        {
            ClipVolumeObject* pVolume = (ClipVolumeObject*)pObj;
            pVolume->UpdateGameResource();
            return true;
        }

        return false;
    }

    bool GetIStatObj(CBaseObject* pObj, _smart_ptr<IStatObj>* pOutStatObj)
    {
        if (pObj == NULL)
        {
            return false;
        }

        CD::ModelCompiler* pCompiler = NULL;
        if (GetCompiler(pObj, pCompiler))
        {
            pCompiler->GetIStatObj(pOutStatObj);
            return true;
        }

        return false;
    }

    bool GenerateGameFilename(CBaseObject* pObj, QString& outFileName)
    {
        if (pObj == NULL)
        {
            return false;
        }

        if (qobject_cast<DesignerObject*>(pObj))
        {
            DesignerObject* pModel = (DesignerObject*)pObj;
            pModel->GenerateGameFilename(outFileName);
            return true;
        }
        else if (qobject_cast<AreaSolidObject*>(pObj))
        {
            AreaSolidObject* pArea = (AreaSolidObject*)pObj;
            pArea->GenerateGameFilename(outFileName);
            return true;
        }
        else if (qobject_cast<ClipVolumeObject*>(pObj))
        {
            ClipVolumeObject* pVolume = (ClipVolumeObject*)pObj;
            outFileName = pVolume->GenerateGameFilename();
            return true;
        }

        return false;
    }

    bool GetRenderFlag(CBaseObject* pObj, int& outRenderFlag)
    {
        if (pObj == NULL)
        {
            return false;
        }

        CD::ModelCompiler* pCompiler = NULL;
        if (GetCompiler(pObj, pCompiler))
        {
            outRenderFlag = pCompiler->GetRenderFlags();
            return true;
        }

        return false;
    }

    bool GetCompiler(CBaseObject* pObj, CD::ModelCompiler*& pCompiler)
    {
        if (pObj == NULL)
        {
            return false;
        }

        pCompiler = NULL;

        if (qobject_cast<DesignerObject*>(pObj))
        {
            DesignerObject* pModel = (DesignerObject*)pObj;
            pCompiler = pModel->GetCompiler();
        }
        else if (qobject_cast<AreaSolidObject*>(pObj))
        {
            AreaSolidObject* pArea = (AreaSolidObject*)pObj;
            pCompiler = pArea->GetCompiler();
        }
        else if (qobject_cast<ClipVolumeObject*>(pObj))
        {
            ClipVolumeObject* pVolume = (ClipVolumeObject*)pObj;
            pCompiler = pVolume->GetCompiler();
        }

        return pCompiler ? true : false;
    }

    bool GetModel(CBaseObject* pObj, CD::Model*& pModel)
    {
        if (pObj == NULL)
        {
            return false;
        }

        pModel = NULL;

        if (qobject_cast<DesignerObject*>(pObj))
        {
            DesignerObject* pModelObj = (DesignerObject*)pObj;
            pModel = pModelObj->GetModel();
        }
        else if (qobject_cast<AreaSolidObject*>(pObj))
        {
            AreaSolidObject* pArea = (AreaSolidObject*)pObj;
            pModel = pArea->GetModel();
        }
        else if (qobject_cast<ClipVolumeObject*>(pObj))
        {
            ClipVolumeObject* pVolume = (ClipVolumeObject*)pObj;
            pModel = pVolume->GetModel();
        }

        return pModel != NULL;
    }

    void AddMirroredPolygon(CD::Model* pModel, CD::PolygonPtr pPolygon, int opDesignerType)
    {
        if (!pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            return;
        }

        CD::EOperationType opType = (CD::EOperationType)opDesignerType;
        CD::PolygonPtr pMirroredPolygon = pPolygon->Clone()->Mirror(pModel->GetMirrorPlane());
        pModel->AddPolygon(pMirroredPolygon, opType);
    }

    void AddMirroredOpenPolygon(CD::Model* pModel, CD::PolygonPtr pPolygon, bool bOnlyAdd)
    {
        if (!pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            return;
        }

        CD::PolygonPtr pMirroredPolygon = pPolygon->Clone()->Mirror(pModel->GetMirrorPlane());
        pModel->AddOpenPolygon(pMirroredPolygon, bOnlyAdd);
    }

    void RemoveMirroredPolygon(CD::Model* pModel, CD::PolygonPtr pPolygon)
    {
        if (!pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            return;
        }

        CD::PolygonPtr pMirroredPolygon = pModel->QueryEquivalentPolygon(pPolygon->Clone()->Mirror(pModel->GetMirrorPlane()));

        if (pMirroredPolygon == NULL)
        {
            return;
        }

        pModel->RemovePolygon(pMirroredPolygon);
    }

    void DrillMirroredPolygon(CD::Model* pModel, CD::PolygonPtr pPolygon, bool bRemainFrame)
    {
        if (!pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            return;
        }

        CD::PolygonPtr pMirroredPolygon = pPolygon->Clone()->Mirror(pModel->GetMirrorPlane());
        pModel->DrillPolygon(pModel->QueryEquivalentPolygon(pMirroredPolygon), bRemainFrame);
    }

    void RemovePolygonWithSpecificFlagsFromList(CD::Model* pModel, std::vector<CD::PolygonPtr>& polygonList, int nFlags)
    {
        std::vector<CD::PolygonPtr>::iterator ii = polygonList.begin();
        for (; ii != polygonList.end(); )
        {
            if ((*ii)->CheckFlags(nFlags))
            {
                ii = polygonList.erase(ii);
            }
            else
            {
                ++ii;
            }
        }
    }

    void RemovePolygonWithoutSpecificFlagsFromList(CD::Model* pModel, std::vector<CD::PolygonPtr>& polygonList, int nFlags)
    {
        std::vector<CD::PolygonPtr>::iterator ii = polygonList.begin();
        for (; ii != polygonList.end(); )
        {
            if (!(*ii)->CheckFlags(nFlags))
            {
                ii = polygonList.erase(ii);
            }
            else
            {
                ++ii;
            }
        }
    }

    void UpdateMirroredPartWithPlane(CD::Model* pModel, const BrushPlane& plane)
    {
        if (!pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            return;
        }

        const BrushPlane& mirroredPlane = pModel->GetMirrorPlane().MirrorPlane(plane);
        pModel->RemovePolygonsWithSpecificFlagsPlane(CD::ePolyFlag_Mirrored, &mirroredPlane);

        std::vector<CD::PolygonPtr> polygons;
        if (!pModel->QueryPolygons(plane, polygons))
        {
            return;
        }

        RemovePolygonWithSpecificFlagsFromList(pModel, polygons, CD::ePolyFlag_Mirrored);

        for (int i = 0, iPolygonSize(polygons.size()); i < iPolygonSize; ++i)
        {
            if (polygons[i]->IsOpen())
            {
                AddMirroredOpenPolygon(pModel, polygons[i], true);
            }
            else
            {
                AddMirroredPolygon(pModel, polygons[i], CD::eOpType_Add);
            }
        }
    }

    void EraseMirroredEdge(CD::Model* pModel, const BrushEdge3D& edge)
    {
        if (!pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            return;
        }

        const BrushPlane& mirrorPlane = pModel->GetMirrorPlane();
        BrushEdge3D mirroredEdge(mirrorPlane.MirrorVertex(edge.m_v[0]), mirrorPlane.MirrorVertex(edge.m_v[1]));

        pModel->EraseEdge(mirroredEdge);
    }

    bool MakeListConsistingOfArc(const BrushVec2& vOutsideVertex, const BrushVec2& vBaseVertex0, const BrushVec2& vBaseVertex1, int nSegmentCount, std::vector<BrushVec2>& outVertexList)
    {
        BrushFloat distToCrossPoint;
        BrushEdge::GetSquaredDistance(BrushEdge(vBaseVertex0, vBaseVertex1), vOutsideVertex, distToCrossPoint);
        if (distToCrossPoint == 0)
        {
            DESIGNER_ASSERT(0);
            return false;
        }

        BrushFloat circumCircleRadius;
        BrushVec2 circumCenter;
        if (!ComputeCircumradiusAndCircumcenter(vBaseVertex0, vBaseVertex1, vOutsideVertex, &circumCircleRadius, &circumCenter))
        {
            DESIGNER_ASSERT(0);
            return false;
        }

        const BrushVec2 vXAxis(1.0f, 0.0f);

        BrushVec2 circumCenterToFirstPoint = (vBaseVertex0 - circumCenter).GetNormalized();
        BrushVec2 circumCenterToLastPoint = (vBaseVertex1 - circumCenter).GetNormalized();

        BrushFloat startRadian = acos(vXAxis.Dot(circumCenterToFirstPoint));
        BrushFloat endRadian = acos(vXAxis.Dot(circumCenterToLastPoint));

        BrushLine vXAxisLine(circumCenter, circumCenter + vXAxis);

        bool bFirstPointBetween180To360 = vXAxisLine.Distance(vBaseVertex0) < 0;
        if (bFirstPointBetween180To360)
        {
            startRadian = -startRadian;
        }

        bool bSecondPointBetween180To360 = vXAxisLine.Distance(vBaseVertex1) < 0;
        if (bSecondPointBetween180To360)
        {
            endRadian =  -endRadian;
        }

        BrushFloat diffRadian = endRadian - startRadian;
        BrushFloat diffOppositeRadian = diffRadian > 0 ? -(CD::PI2 - diffRadian) : -(-CD::PI2 - diffRadian);

        BrushFloat diffTryRadian[2] = { diffRadian, diffOppositeRadian };
        std::vector<BrushVec2> arcTryVertexList[2];
        const int nTryEdgeCount = 3;
        BrushFloat fNearestDistance = 3e10f;
        int nNearerSide(0);

        for (int k = 0; k < 2; ++k)
        {
            CD::MakeSectorOfCircle(circumCircleRadius, circumCenter, startRadian, diffTryRadian[k], nTryEdgeCount, arcTryVertexList[k]);

            for (int i = 0, iArcVertexSize(arcTryVertexList[k].size()); i < iArcVertexSize; ++i)
            {
                const BrushVec2& v0 = arcTryVertexList[k][i];
                const BrushVec2& v1 = arcTryVertexList[k][(i + 1) % iArcVertexSize];

                BrushFloat fDistance = 3e10f;
                BrushEdge::GetSquaredDistance(BrushEdge(v0, v1), vOutsideVertex, fDistance);
                fDistance = std::abs(fDistance);
                if (fDistance < fNearestDistance)
                {
                    nNearerSide = k;
                    fNearestDistance = fDistance;
                }
            }
        }

        CD::MakeSectorOfCircle(circumCircleRadius, circumCenter, startRadian, diffTryRadian[nNearerSide], nSegmentCount, outVertexList);

        for (int i = 0, iCount(outVertexList.size()); i < iCount; ++i)
        {
            if (!outVertexList[i].IsValid())
            {
                DESIGNER_ASSERT(0);
                return false;
            }
        }

        return true;
    }

    template<class T>
    bool ComputeCircumradiusAndCircumcenter(const T& v0, const T& v1, const T& v2, BrushFloat* outCircumradius, T* outCircumcenter)
    {
        BrushFloat dca = (v2 - v0).Dot(v1 - v0);
        BrushFloat dba = (v2 - v1).Dot(v0 - v1);
        BrushFloat dcb = (v0 - v2).Dot(v1 - v2);

        BrushFloat n1 = dba * dcb;
        BrushFloat n2 = dcb * dca;
        BrushFloat n3 = dca * dba;

        BrushFloat n123 = n1 + n2 + n3;

        if (n123 == 0)
        {
            return false;
        }

        if (outCircumradius)
        {
            *outCircumradius = sqrt(((dca + dba) * (dba + dcb) * (dcb + dca)) / n123) * 0.5f;
        }

        if (outCircumcenter)
        {
            *outCircumcenter = (v0 * (n2 + n3) + v1 * (n3 + n1) + v2 * (n1 + n2)) / (2 * n123);
        }

        return true;
    }

    BrushVec3 GetWorldBottomCenter(CBaseObject* pObject)
    {
        AABB boundbox;
        pObject->GetBoundBox(boundbox);
        return BrushVec3(boundbox.GetCenter().x, boundbox.GetCenter().y, boundbox.min.z);
    }

    BrushVec3 GetOffsetToAnother(CBaseObject* pObject, const BrushVec3& vTargetPos)
    {
        BrushVec3 vPos = pObject->GetWorldPos();
        return pObject->GetWorldTM().GetInverted().TransformVector(vPos - vTargetPos);
    }

    void PivotToCenter(CBaseObject* pObject, CD::Model* pModel)
    {
        BrushVec3 vBottomCenter = GetWorldBottomCenter(pObject);
        BrushVec3 vWorldPos = pObject->GetWorldPos();
        BrushVec3 vDiff = GetOffsetToAnother(pObject, vBottomCenter);
        pModel->Move(vDiff);
        pObject->SetWorldPos(vWorldPos - pObject->GetWorldTM().TransformVector(vDiff));
    }

    void PivotToPos(CBaseObject* pObject, CD::Model* pModel, const BrushVec3& vPivot)
    {
        BrushVec3 vWorldNewPivot = pObject->GetWorldTM().TransformPoint(vPivot);
        BrushVec3 vWorldPos = pObject->GetWorldPos();
        BrushVec3 vDiff = GetOffsetToAnother(pObject, vWorldNewPivot);
        pModel->Move(vDiff);
        pObject->SetWorldPos(vWorldPos - pObject->GetWorldTM().TransformVector(vDiff));
    }

    bool DoesEdgeIntersectRect(const QRect& rect, CViewport* pView, const Matrix34& worldTM, const BrushEdge3D& edge)
    {
        const CCamera& camera = GetIEditor()->GetRenderer()->GetCamera();
        const Plane* pNearPlane = camera.GetFrustumPlane(FR_PLANE_NEAR);
        BrushPlane nearPlane(pNearPlane->n, pNearPlane->d);

        BrushEdge3D clippedEdge(edge);
        clippedEdge.m_v[0] = ToBrushVec3(worldTM.TransformPoint(ToVec3(clippedEdge.m_v[0])));
        clippedEdge.m_v[1] = ToBrushVec3(worldTM.TransformPoint(ToVec3(clippedEdge.m_v[1])));

        BrushVec3 intersectedVertex;
        BrushFloat fDist = 0;
        if (nearPlane.HitTest(edge.m_v[0], edge.m_v[1], &fDist, &intersectedVertex) && fDist >= 0 && fDist <= edge.GetLength())
        {
            if (nearPlane.Distance(edge.m_v[0]) < 0)
            {
                clippedEdge.m_v[1] = intersectedVertex;
            }
            else if (nearPlane.Distance(edge.m_v[1]) < 0)
            {
                clippedEdge.m_v[0] = intersectedVertex;
            }
        }

        QPoint p0 = pView->WorldToView(ToVec3(clippedEdge.m_v[0]));
        QPoint p1 = pView->WorldToView(ToVec3(clippedEdge.m_v[1]));
        BrushEdge3D clippedEdgeOnView(BrushVec3(p0.x(), p0.y(), 0), BrushVec3(p1.x(), p1.y(), 0));

        CD::PolygonPtr pRectPolygon = CD::MakePolygonFromRectangle(rect);
        return pRectPolygon->GetBSPTree()->HasIntersection(clippedEdgeOnView);
    }

    bool DoesSelectionBoxIntersectRect(CViewport* view, const Matrix34& worldTM, const QRect& rect, CD::PolygonPtr pPolygon)
    {
        BrushVec3 vBoxSize = CD::GetElementBoxSize(view, view->GetType() != ET_ViewportCamera, worldTM.GetTranslation());
        BrushVec3 vRepresentativePos = pPolygon->GetRepresentativePosition();
        AABB aabb(ToVec3(vRepresentativePos - vBoxSize), ToVec3(vRepresentativePos + vBoxSize));
        QPoint center2D = view->WorldToView(worldTM.TransformPoint(aabb.GetCenter()));
        return rect.contains(center2D) && !pPolygon->CheckFlags(CD::ePolyFlag_Hidden);
    }

    CD::PolygonPtr MakePolygonFromRectangle(const QRect& rectangle)
    {
        std::vector<BrushVec3> vRectangleList;
        vRectangleList.push_back(BrushVec3(rectangle.right() + 1, rectangle.top(), 0));
        vRectangleList.push_back(BrushVec3(rectangle.right() + 1, rectangle.bottom() + 1, 0));
        vRectangleList.push_back(BrushVec3(rectangle.left(), rectangle.bottom() + 1, 0));
        vRectangleList.push_back(BrushVec3(rectangle.left(), rectangle.top(), 0));
        return new CD::Polygon(vRectangleList);
    }

    void CopyPolygons(std::vector<CD::PolygonPtr>& sourcePolygons, std::vector<CD::PolygonPtr>& destPolygons)
    {
        for (int i = 0, iSourcePolygonCount(sourcePolygons.size()); i < iSourcePolygonCount; ++i)
        {
            destPolygons.push_back(sourcePolygons[i]->Clone());
        }
    }

    CD::PolygonPtr Convert2ViewPolygon(IDisplayViewport* pView, const BrushMatrix34& worldTM, CD::PolygonPtr pPolygon, CD::PolygonPtr* pOutPolygon)
    {
        BrushMatrix34 vInvMatrix = worldTM.GetInverted();

        const BrushVec3 vCameraPos = vInvMatrix.TransformPoint(ToBrushVec3(pView->GetViewTM().GetTranslation()));
        const BrushVec3 vDir = vInvMatrix.TransformVector(ToBrushVec3(pView->GetViewTM().GetColumn1()));

        std::set<int> verticesBehindCamera;
        for (int i = 0, iVertexCount(pPolygon->GetVertexCount()); i < iVertexCount; ++i)
        {
            if (vDir.Dot((pPolygon->GetPos(i) - vCameraPos).GetNormalized()) <= 0)
            {
                verticesBehindCamera.insert(i);
            }
        }

        if (verticesBehindCamera.size() == pPolygon->GetVertexCount())
        {
            return NULL;
        }

        CD::PolygonPtr pViewPolygon = new CD::Polygon(*pPolygon);
        pViewPolygon->Transform(worldTM);

        if (!verticesBehindCamera.empty())
        {
            const CCamera& camera = GetIEditor()->GetRenderer()->GetCamera();
            const Plane* pNearPlane = camera.GetFrustumPlane(FR_PLANE_NEAR);
            BrushPlane nearPlane(pNearPlane->n, pNearPlane->d);

            std::vector<CD::PolygonPtr> frontPolygons;
            std::vector<CD::PolygonPtr> backPolygons;
            pViewPolygon->ClipByPlane(nearPlane, frontPolygons, backPolygons);

            int nBackPolygonCount = backPolygons.size();
            if (nBackPolygonCount == 1)
            {
                pViewPolygon = backPolygons[0];
            }
            else if (nBackPolygonCount > 1)
            {
                pViewPolygon->Clear();
                for (int i = 0; i < nBackPolygonCount; ++i)
                {
                    pViewPolygon->Union(backPolygons[i]);
                }
            }

            for (int i = 0, iVertexCount(pViewPolygon->GetVertexCount()); i < iVertexCount; ++i)
            {
                const BrushVec3& v = pViewPolygon->GetPos(i);
                BrushFloat fDistance = nearPlane.Distance(v);
                if (fDistance > -kDesignerEpsilon)
                {
                    pViewPolygon->SetPos(i, v - nearPlane.Normal() * (BrushFloat)0.001);
                }
            }

            if (pOutPolygon)
            {
                *pOutPolygon = pViewPolygon->Clone();
                (*pOutPolygon)->Transform(worldTM.GetInverted());
            }
        }
        else
        {
            if (pOutPolygon)
            {
                *pOutPolygon = NULL;
            }
        }

        for (int i = 0, iVertexCount(pViewPolygon->GetVertexCount()); i < iVertexCount; ++i)
        {
            QPoint pt = pView->WorldToView(pViewPolygon->GetPos(i));
            pViewPolygon->SetPos(i, BrushVec3((BrushFloat)pt.x(), (BrushFloat)pt.y(), 0));
        }

        BrushPlane plane(BrushVec3(0, 0, 1), 0);
        pViewPolygon->GetComputedPlane(plane);
        pViewPolygon->SetPlane(plane);

        return pViewPolygon;
    }

    bool IsFrameRemainInRemovingFace(CBaseObject* pObject)
    {
        if (pObject == NULL)
        {
            return false;
        }

        return pObject->GetType() != OBJTYPE_SOLID ? true : false;
    }

    bool BinarySearchForScale(BrushFloat fValidScale, BrushFloat fInvalidScale, int nCount, Polygon& polygon, BrushFloat& fOutScale)
    {
        static const int kMaxCount(10);
        static const int kMaxCountToAvoidInfiniteLoop(100);

        BrushFloat fMiddleScale = (fValidScale + fInvalidScale) * 0.5f;

        Polygon tempPolygon(polygon);
        if (tempPolygon.Scale(fMiddleScale, true))
        {
            if (nCount >= kMaxCount || std::abs(fValidScale - fMiddleScale) <= (BrushFloat)0.001)
            {
                fOutScale = fMiddleScale;
                return true;
            }
            return BinarySearchForScale(fMiddleScale, fInvalidScale, nCount + 1, polygon, fOutScale);
        }

        if (nCount >= kMaxCountToAvoidInfiniteLoop)
        {
            return false;
        }

        return BinarySearchForScale(fValidScale, fMiddleScale, nCount + 1, polygon, fOutScale);
    }

    void ResetXForm(CBaseObject* pBaseObject, CD::Model* pModel, int nResetFlag)
    {
        if (pModel == NULL || pBaseObject == NULL)
        {
            return;
        }

        Matrix34 worldTM;
        worldTM.SetIdentity();

        if ((nResetFlag& CD::eResetXForm_Rotation) && (nResetFlag & CD::eResetXForm_Scale))
        {
            Matrix34 brushTM = pBaseObject->GetWorldTM();
            brushTM.SetTranslation(BrushVec3(0, 0, 0));
            pModel->Transform(ToBrushMatrix34(brushTM));
        }
        else if (nResetFlag & CD::eResetXForm_Rotation)
        {
            Matrix34 rotationTM = Matrix34(pBaseObject->GetRotation());
            worldTM = Matrix34::CreateScale(pBaseObject->GetScale());
            pModel->Transform(ToBrushMatrix34(rotationTM));
        }
        else if (nResetFlag & CD::eResetXForm_Scale)
        {
            Matrix34 scaleTM = Matrix34::CreateScale(pBaseObject->GetScale());
            worldTM = Matrix34(pBaseObject->GetRotation());
            pModel->Transform(ToBrushMatrix34(scaleTM));
        }

        worldTM.SetTranslation(pBaseObject->GetWorldPos());
        pBaseObject->SetWorldTM(worldTM);

        if (nResetFlag & CD::eResetXForm_Position)
        {
            CD::PivotToCenter(pBaseObject, pModel);
        }
    }

    void AddPolygonWithSubMatID(CD::PolygonPtr pPolygon)
    {
        if (!CD::GetDesigner())
        {
            return;
        }
        pPolygon->SetMaterialID(CD::GetDesigner()->GetCurrentSubMatID());
        CD::GetDesigner()->GetModel()->AddPolygon(pPolygon, CD::eOpType_Add);
    }

    void CreateMeshFacesFromPolygon(CD::PolygonPtr pPolygon, CD::FlexibleMesh& mesh, bool bGenerateBackFaces)
    {
        int vertexOffset = mesh.vertexList.size();
        int faceOffset = mesh.faceList.size();

        //! Create vertices, normals and faces
        PolygonDecomposer decomposer;
        if (!bGenerateBackFaces)
        {
            decomposer.TriangulatePolygon(pPolygon, mesh, vertexOffset, faceOffset);
        }
        else
        {
            decomposer.TriangulatePolygon(pPolygon->Clone()->Flip(), mesh, vertexOffset, faceOffset);
        }

        //! Assign a MatID to each face
        int nMatID = pPolygon->GetMaterialID();
        int nSubsetNumber = mesh.AddMatID(nMatID);
        for (int k = faceOffset, iFaceSize(mesh.faceList.size()); k < iFaceSize; ++k)
        {
            mesh.faceList[k].nSubset = nSubsetNumber;
        }
    }

    bool HitTest(const SMainContext& mc, HitContext& hit)
    {
        if (!mc.pModel || !mc.pObject)
        {
            return false;
        }

        MODEL_SHELF_RECONSTRUCTOR(mc.pModel);
        mc.pModel->SetShelf(0);

        BrushVec3 outPosition;
        BrushVec3 localRaySrc, localRayDir;
        const Matrix34& worldTM(mc.pObject->GetWorldTM());
        GetLocalViewRay(worldTM, hit.view, hit.point2d, localRaySrc, localRayDir);
        if (mc.pModel->QueryPosition(localRaySrc, localRayDir, outPosition))
        {
            BrushVec3 worldRaySrc = worldTM.TransformPoint(localRaySrc);
            BrushVec3 worldHitPos = worldTM.TransformPoint(outPosition);
            hit.dist = (float)worldRaySrc.GetDistance(worldHitPos);
            return true;
        }

        return true;
    }

    void AssignMatIDToSelectedPolygons(int matID)
    {
        ElementManager* pSelect = CD::GetDesigner()->GetSelectedElements();
        for (int i = 0, iCount(pSelect->GetCount()); i < iCount; ++i)
        {
            if (!(*pSelect)[i].m_pPolygon)
            {
                continue;
            }
            (*pSelect)[i].m_pPolygon->SetMaterialID(matID);
        }
    }

    std::vector<DesignerObject*> GetSelectedDesignerObjects()
    {
        std::vector<DesignerObject*> designerObjectSet;
        CSelectionGroup* pSelection = GetIEditor()->GetSelection();
        for (int i = 0, iSelectionCount(pSelection->GetCount()); i < iSelectionCount; ++i)
        {
            CBaseObject* pObj = pSelection->GetObject(i);
            if (!qobject_cast<DesignerObject*>(pObj))
            {
                continue;
            }
            designerObjectSet.push_back((DesignerObject*)pObj);
        }
        return designerObjectSet;
    }

    bool IsDesignerRelated(CBaseObject* pObject)
    {
        return qobject_cast<DesignerObject*>(pObject) || qobject_cast<AreaSolidObject*>(pObject) || qobject_cast<ClipVolumeObject*>(pObject);
    }

    void RemoveAllEmptyDesignerObjects()
    {
        DynArray<CBaseObject*> objects;
        GetIEditor()->GetObjectManager()->GetObjects(objects);
        for (int i = 0, iCount(objects.size()); i < iCount; ++i)
        {
            if (!qobject_cast<DesignerObject*>(objects[i]))
            {
                continue;
            }
            DesignerObject* pDesignerObj = (DesignerObject*)objects[i];
            if (pDesignerObj->IsEmpty())
            {
                GetIEditor()->GetObjectManager()->DeleteObject(pDesignerObj);
            }
        }
    }

    bool CheckIfHasValidModel(CBaseObject* pObject)
    {
        if (pObject == NULL)
        {
            return false;
        }

        CD::Model* pModel = NULL;
        if (!GetModel(pObject, pModel))
        {
            return false;
        }

        if (pModel->IsEmpty(0))
        {
            return false;
        }

        bool bValidModel = false;
        for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
        {
            PolygonPtr polygon = pModel->GetPolygon(i);
            if (!polygon->IsOpen())
            {
                bValidModel = true;
                break;
            }
        }

        return bValidModel;
    }

    void MakeRectangle(const BrushPlane& plane, const BrushVec2& startPos, const BrushVec2& endPos, std::vector<BrushVec3>& outVertices)
    {
        outVertices.reserve(4);
        outVertices.push_back(plane.P2W(BrushVec2(startPos.x, startPos.y)));
        outVertices.push_back(plane.P2W(BrushVec2(startPos.x, endPos.y)));
        outVertices.push_back(plane.P2W(BrushVec2(endPos.x, endPos.y)));
        outVertices.push_back(plane.P2W(BrushVec2(endPos.x, startPos.y)));
    }

    void ConvertMeshFacesToFaces(const std::vector<SVertex>& vertices, std::vector<SMeshFace>& meshFaces, std::vector<std::vector<Vec3> >& outFaces)
    {
        for (int i = 0, iFaceCount(meshFaces.size()); i < iFaceCount; ++i)
        {
            std::vector<Vec3> f;
            f.push_back(vertices[meshFaces[i].v[0]].pos);
            f.push_back(vertices[meshFaces[i].v[1]].pos);
            f.push_back(vertices[meshFaces[i].v[2]].pos);
            outFaces.push_back(f);
        }
    }
}