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

#pragma once

#include "Core/Polygon.h"

namespace CD
{
    class Model;
    class ModelCompiler;

    bool UpdateStatObjWithoutBackFaces(CBaseObject* pObj);
    bool UpdateStatObj(CBaseObject* pObj);
    bool UpdateGameResource(CBaseObject* pObj);
    bool GetIStatObj(CBaseObject* pObj, _smart_ptr<IStatObj>* pOutStatObj);
    bool GenerateGameFilename(CBaseObject* pObj, QString& outFileName);
    bool GetRenderFlag(CBaseObject* pObj, int& outRenderFlag);
    bool GetCompiler(CBaseObject* pObj, CD::ModelCompiler*& pCompiler);
    bool GetModel(CBaseObject* pObj, CD::Model*& pModel);
    bool HitTest(SMainContext& mc, HitContext& hit);
    void AddMirroredPolygon(CD::Model* pModel, CD::PolygonPtr pPolygon, int opDesignerType);
    void AddMirroredOpenPolygon(CD::Model* pModel, CD::PolygonPtr pPolygon, bool bOnlyAdd);
    void RemoveMirroredPolygon(CD::Model* pModel, CD::PolygonPtr pPolygon);
    void DrillMirroredPolygon(CD::Model* pModel, CD::PolygonPtr pPolygon, bool bRemainFrame = false);
    void RemovePolygonWithSpecificFlagsFromList(CD::Model* pModel, std::vector<CD::PolygonPtr>& polygonList, int nFlags);
    void RemovePolygonWithoutSpecificFlagsFromList(CD::Model* pModel, std::vector<CD::PolygonPtr>& polygonList, int nFlags);
    void CreateMirroredPolygons(CD::Model* pModel);
    void UpdateMirroredPartWithPlane(CD::Model* pModel, const BrushPlane& plane);
    void EraseMirroredEdge(CD::Model* pModel, const BrushEdge3D& edge);
    bool MakeListConsistingOfArc(const BrushVec2& vOutsideVertex, const BrushVec2& vBaseVertex0, const BrushVec2& vBaseVertex1, int nSegmentCount, std::vector<BrushVec2>& outVertexList);
    template<class T>
    bool ComputeCircumradiusAndCircumcenter(const T& v0, const T& v1, const T& v2, BrushFloat* outCircumradius, T* outCircumcenter);
    BrushVec3 GetWorldBottomCenter(CBaseObject* pObject);
    BrushVec3 GetOffsetToAnother(CBaseObject* pObject, const BrushVec3& vTargetPos);
    void PivotToCenter(CBaseObject* pObject, CD::Model* pModel);
    void PivotToPos(CBaseObject* pObject, CD::Model* pModel, const BrushVec3& vPivot);
    bool DoesEdgeIntersectRect(const QRect& rect, CViewport* pView, const Matrix34& worldTM, const BrushEdge3D& edge);
    bool DoesSelectionBoxIntersectRect(CViewport* view, const Matrix34& worldTM, const QRect& rect, CD::PolygonPtr pPolygon);
    CD::PolygonPtr MakePolygonFromRectangle(const QRect& rectangle);
    void CopyPolygons(std::vector<CD::PolygonPtr>& sourcePolygons, std::vector<CD::PolygonPtr>& destPolygons);
    CD::PolygonPtr Convert2ViewPolygon(IDisplayViewport* pView, const BrushMatrix34& worldTM, CD::PolygonPtr pPolygon, CD::PolygonPtr* pOutPolygon);
    bool IsFrameRemainInRemovingFace(CBaseObject* pObject);
    bool BinarySearchForScale(BrushFloat fValidScale, BrushFloat fInvalidScale, int nCount, Polygon& polygon, BrushFloat& fOutScale);
    void ResetXForm(CBaseObject* pBaseObject, CD::Model* pModel, int nResetFlag = CD::eResetXForm_All);
    void AddPolygonWithSubMatID(CD::PolygonPtr pPolygon);
    void CreateMeshFacesFromPolygon(PolygonPtr pPolygon, FlexibleMesh& mesh, bool bGenerateBackFaces);
    void AssignMatIDToSelectedPolygons(int matID);
    std::vector<DesignerObject*> GetSelectedDesignerObjects();
    bool IsDesignerRelated(CBaseObject* pObject);
    bool CheckIfHasValidModel(CBaseObject* pObject);
    void RemoveAllEmptyDesignerObjects();
    void MakeRectangle(const BrushPlane& plane, const BrushVec2& startPos, const BrushVec2& endPos, std::vector<BrushVec3>& outVertices);
    void ConvertMeshFacesToFaces(const std::vector<SVertex>& vertices, std::vector<SMeshFace>& meshFaces, std::vector<std::vector<Vec3> >& outFaces);
};