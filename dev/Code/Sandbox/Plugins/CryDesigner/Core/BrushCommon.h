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
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushUtil.h
//  Created:     6/7/2010 by Jaesik
////////////////////////////////////////////////////////////////////////////
#include "Plane.h"
#include "Line.h"
#include "IIndexedMesh.h"
#include "Serialization/Serializer.h"
#include "Serialization/IArchiveHost.h"
#include "Serialization/Decorators/Range.h"
#include "FlexibleMesh.h"

#ifdef DEBUG
//#define ENABLE_DESIGNERASSERTION
//#define ENABLE_DRAWDEBUGHELPER
#endif

#ifdef ENABLE_DESIGNERASSERTION
#define DESIGNER_ASSERT(condition)  assert(condition)
#else
#define DESIGNER_ASSERT(condition)  assert(1)
#endif

class ElementManager;

namespace CD
{
    class Model;
    class ModelCompiler;
    class Polygon;

    struct STexInfo
    {
        float shift[2];
        float scale[2];
        float rotate;

        STexInfo()
        {
            shift[0] = shift[1] = 0;
            scale[0] = scale[1] = 1.0f;
            rotate = 0;
        }

        STexInfo(const STexInfo& ti)
        {
            shift[0] = ti.shift[0];
            shift[1] = ti.shift[1];

            scale[0] = ti.scale[0];
            scale[1] = ti.scale[1];

            rotate = ti.rotate;
        }

        void Load(XmlNodeRef& xmlNode);
        void Save(XmlNodeRef& xmlNode);
    };

    enum EBooleanOperationEnum
    {
        eBOE_Union,
        eBOE_Intersection,
        eBOE_Difference,
    };

    enum EPointPosEnum
    {
        ePP_BORDER,
        ePP_INSIDE,
        ePP_OUTSIDE
    };

    enum EClipType
    {
        eCT_Positive,
        eCT_Negative,
    };

    enum ECompilerFlags
    {
        BRF_MESH_VALID = 0x40,
        BRF_MODIFIED = 0x80,
        BRF_SUB_OBJ_SEL = 0x100,
        BRF_SELECTED    = 0x200,
    };

    static const char* BrushDesignerDefaultMaterial("EngineAssets/TextureMsg/DefaultSolids");
    static const char* BrushSolidDefaultMaterial("EngineAssets/TextureMsg/DefaultSolids");
    static const int kLineThickness(2);
    static const int kChosenLineThickness(7);
    static const ColorB kSelectedColor(214, 150, 0, 255);
    static const ColorB kResizedPolygonColor(128, 128, 240, 255);
    static const ColorB PolygonLineColor(100, 100, 200, 255);
    static const ColorB PolygonInvalidLineColor(255, 100, 0, 255);
    static const ColorB PolygonParallelToAxis(100, 200, 200, 255);
    static uint32 s_nGlobalBrushFileId = 0;
    static uint32 s_nGlobalBrushDesignerFileId = 0;
    static const BrushFloat PI = 3.14159265358979323846;
    static const BrushFloat PI2 = CD::PI * 2.0;
    static const BrushFloat kNormalGap(0.01);
    static const BrushFloat kEnoughBigNumber(3e10);
    static const char* BrushDesignerCameraName("Crydesigner_camera");
    static const int kDefaultSubdivisionNum = 16;
    static const int kDefaultCurveEdgeNum = 18;
    static const float kDefaultViewDist = 1.f;
    static const ColorB kElementBoxColor(0xFFFFAAAA);
    static const BrushFloat kElementEdgeThickness = 6;
    static const int kSmoothingGroupIDNumber = 32;
    static const int kMaximumSubdivisionLevel = 5;
    static const char* sessionName = "Settings\\Designer";

    typedef int ShelfID;
    static const ShelfID kMaxShelfCount = 2;

    // BrushEdge3D - A picked Edge, BrushVec3 - A picked point in the middle of the edge
    typedef std::vector< std::pair<BrushEdge3D, BrushVec3> > EdgeQueryResult;

    enum EPickingFlag
    {
        ePF_Vertex = BIT(0),
        ePF_Edge = BIT(1),
        ePF_Face = BIT(2),
        ePF_All = ePF_Vertex | ePF_Edge | ePF_Face
    };

    enum EClipObjective
    {
        eCO_JustClip,
        eCO_Union0,
        eCO_Union1,
        eCO_Subtract,
        eCO_Intersection0,
        eCO_Intersection0IncludingCoSame,
        eCO_Intersection1,
        eCO_Intersection1IncludingCoDiff,
    };

    enum EIntersectionType
    {
        eIT_None,
        eIT_Intersection,
        eIT_JustTouch,
    };

    enum EResetXForm
    {
        eResetXForm_Rotation = BIT(0),
        eResetXForm_Scale = BIT(1),
        eResetXForm_Position = BIT(2),
        eResetXForm_All = eResetXForm_Rotation | eResetXForm_Scale | eResetXForm_Position
    };

    enum EDBResetFlag
    {
        eDBRF_Plane  = BIT(0),
        eDBRF_Vertex = BIT(1),
        eDBRF_ALL    = 0xFFFFFFFF
    };

    static const float kDefaultStepRise = 0.33f;
    static const int kDefaultStepNumber = 16;

    BrushVec3 WorldPos2ScreenPos(const BrushVec3& worldPos);
    BrushVec3 ScreenPos2WorldPos(const BrushVec3& screenPos);
    void DrawSpot(DisplayContext& dc, const BrushMatrix34& worldTM, const BrushVec3& pos, const ColorB& color, float fSize = 5.0f);

    EOperationResult SubtractEdge3D(const BrushEdge3D&inEdge0, const BrushEdge3D&inEdge1, BrushEdge3D outEdge[2]);
    EOperationResult IntersectEdge3D(const BrushEdge3D& inEdge0, const BrushEdge3D& inEdge1, BrushEdge3D& outEdge);
    ESplitResult Split(
        const BrushPlane& plane,
        const BrushLine& splitLine,
        const std::vector<BrushEdge3D>& splitEdges,
        const BrushEdge3D& inEdge,
        BrushEdge3D& positiveEdge,
        BrushEdge3D& negativeEdge);
    void CalcTextureBasis(const SBrushPlane<float>& p, const CD::STexInfo& ti, Vec3& u, Vec3& v);
    void CalcTexCoords(const SBrushPlane<float>& p, const CD::STexInfo& ti, const Vec3& pos, float& tu, float& tv);
    void FitTexture(const SBrushPlane<float>& plane, const Vec3& vMin, const Vec3& vMax, float tileU, float tileV, STexInfo& outTexInfo);

    void FillMesh(const FlexibleMesh& mesh, IIndexedMesh* pMesh);
    void JoinTwoMeshes(FlexibleMesh& mesh0, const FlexibleMesh& mesh1);
    void MakeSectorOfCircle(BrushFloat fRadius, const BrushVec2& vCenter, BrushFloat startRadian, BrushFloat diffRadian, int nSegmentCount, std::vector<BrushVec2>& outVertexList);
    float ComputeAnglePointedByPos(const BrushVec2& vCenter, const BrushVec2& vPointedPos);
    template<class T>
    bool FindVertexWithMinimum(std::vector<T> vertexList, int nElementIndex, int& outMinimumIndex)
    {
        int nMinimumIndex = -1;
        BrushFloat minimumValue = 3e10f;
        for (int i = 0, iVertexSize(vertexList.size()); i < iVertexSize; ++i)
        {
            if (vertexList[i][nElementIndex] < minimumValue)
            {
                nMinimumIndex = i;
                minimumValue = vertexList[i][nElementIndex];
            }
        }
        if (nMinimumIndex == -1)
        {
            return false;
        }
        outMinimumIndex = nMinimumIndex;
        return true;
    }

    template<class T>
    T GetAdjustedFloatToAvoidZero(T x)
    {
        T _x = std::abs(x) < kDesignerEpsilon ? kDesignerEpsilon : x;
        if (x < 0)
        {
            _x *= -1;
        }
        return _x;
    }

    bool DoesEquivalentExist(std::vector<BrushEdge3D>& edgeList, const BrushEdge3D& edge, int* pOutIndex = NULL);
    bool DoesEquivalentExist(std::vector<BrushVec3>& vertexList, const BrushVec3& vertex);
    bool ComputePlane(const std::vector<SVertex>& vList, BrushPlane& outPlane);
    BrushVec3 ComputeNormal(const BrushVec3& v0, const BrushVec3& v1, const BrushVec3& v2);
    void GetLocalViewRay(const BrushMatrix34& worldTM, IDisplayViewport* view, const QPoint& point, BrushVec3& outRaySrc, BrushVec3& outRayDir);
    void DrawPlane(DisplayContext& dc, const BrushVec3& vPivot, const BrushPlane& plane, float size = 12.0f);
    BrushFloat SnapGrid(BrushFloat fValue);
    BrushFloat Snap(BrushFloat pos);

    void Write2Buffer(std::vector<char>& buffer, const void* pData, int nDataSize);
    int ReadFromBuffer(std::vector<char>& buffer, int nPos, void* pData, int nDataSize);

    bool GetIntersectionOfRayAndAABB(const BrushVec3& vPoint, const BrushVec3& vDir, const AABB& aabb, BrushFloat* pOutDistance);

    int FindShortestEdge(std::vector<BrushEdge3D>& edges);
    std::vector<BrushEdge3D> FindNearestEdges(CViewport* pViewport, const BrushMatrix34& worldTM, std::vector< std::pair<BrushEdge3D, BrushVec3> >& edges);

    BrushVec3 GetElementBoxSize(IDisplayViewport* pView, bool b2DViewport, const BrushVec3& vWorldPos);
    bool AreTwoPositionsNear(const BrushVec3& modelPos0, const BrushVec3& modelPos1, const BrushMatrix34& worldTM, IDisplayViewport* pViewport, BrushFloat fBoundRadiuse, BrushFloat* pOutDistance = NULL);
    bool HitTestEdge(const BrushVec3& raySrc, const BrushVec3& rayDir, const BrushVec3& vNormal, const BrushEdge3D& edge, IDisplayViewport* pView = NULL);
    bool PickPosFromWorld(IDisplayViewport* view, const QPoint& point, Vec3& outPickedPos);
    BrushVec3 CorrectVec3(const BrushVec3& v);
    bool IsConvex(std::vector<BrushVec2>& vList);
    bool HasEdgeInEdgeList(const std::vector<BrushEdge3D>& edgeList, const BrushEdge3D& edge);
    bool HasVertexInVertexList(const std::vector<BrushVec3>& vertexList, const BrushVec3& v);

    struct SMainContext
    {
        SMainContext()
            : pObject(NULL)
            , pCompiler(NULL)
            , pModel(NULL) {}
        SMainContext(CBaseObject* _pObject, CD::ModelCompiler* _pBrush, CD::Model* _pDesigner)
            : pObject(_pObject)
            , pCompiler(_pBrush)
            , pModel(_pDesigner) {}
        bool IsValid() const { return pObject && pCompiler && pModel; }
        CBaseObject* pObject;
        CD::ModelCompiler* pCompiler;
        CD::Model* pModel;
        ElementManager* pSelected;
    };

    struct LessConstCharStrCmp
    {
        bool operator()(const char* a, const char* b) const
        {
            return strcmp(a, b) < 0;
        }
    };

    stack_string GetSaveStateFilePath(const char* name);
    bool LoadSettings(const Serialization::SStruct& outObj, const char* name);
    bool SaveSettings(const Serialization::SStruct& outObj, const char* name);

    inline Serialization::RangeDecorator<float> LENGTH_RANGE(float& value)  { return Serialization::Range(value, 0.01f, 100.0f); }
    inline Serialization::RangeDecorator<int> SUBDIVISION_RANGE(int& value) { return Serialization::Range(value, 3, 256); }
    inline Serialization::RangeDecorator<float> STEPRISE_RANGE(float& value){ return Serialization::Range(value, 0.1f, 10.0f); }
};

#define CRYDESIGNER_USER_DIRECTORY      "CryDesigner"
#define SERIALIZATION_ENUM_LABEL(value, label) \
    SERIALIZATION_ENUM(value, label, label);