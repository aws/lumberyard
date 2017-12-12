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
#include "Core/ModelCompiler.h"
#include "Core/Model.h"

namespace CD
{
    class ModelDB;
};

class ArgumentModel
    : public CRefCountBase
{
public:

    ArgumentModel(CD::Polygon* pPolygon, BrushFloat fScale, CBaseObject* pObject, std::vector<CD::PolygonPtr>* perpendicularPolygons, CD::ModelDB* pDB);
    ~ArgumentModel();

    enum EBrushArgumentUpdate
    {
        eBAU_None = 0,
        eBAU_UpdateBrush = 1,
    };

    void Update(EBrushArgumentUpdate updateOp);

    void SetHeight(BrushFloat fHeight);
    BrushFloat GetHeight() const { return m_fHeight; }
    const BrushPlane& GetCurrentCapPlane() { return m_CapPlane; }
    const BrushPlane& GetBasePlane() { return m_BasePlane; }
    CD::PolygonPtr GetCapPolygon() const;
    bool GetSidePolygonList(std::vector<CD::PolygonPtr>& outPolygons) const;
    bool GetPolygonList(std::vector<CD::PolygonPtr>& outPolygons) const;

    AABB GetBoundBox() const { return m_pModel->GetBoundBox(); }

protected:

    void AddCapPolygons();
    void AddSidePolygons();

    void MoveVertices2Plane(const BrushPlane& targetPlane);
    void CompileModel();
    bool FindPlaneWithEdge(const BrushEdge3D& edge, const BrushPlane& hintPlane, BrushPlane& outPlane);

    void UpdateWithScale(EBrushArgumentUpdate updateOp);

protected:

    _smart_ptr<CBaseObject> m_pBaseObject;
    _smart_ptr<CD::ModelCompiler> m_pCompiler;
    _smart_ptr<CD::Model> m_pModel;

    CD::PolygonPtr m_pPolygon;

    CD::PolygonPtr m_pInitialPolygon;
    CD::PolygonPtr m_pInitialOutsidePolygonWithoutBridgeEdges;
    std::vector<CD::PolygonPtr> m_InitialInsidePolygonsWithoutBrideEdges;

    f64 m_fHeight;
    BrushPlane m_BasePlane;
    BrushPlane m_CapPlane;
    BrushFloat m_fScale;

    typedef std::pair<BrushEdge3D, BrushPlane> EdgeBrushPlanePair;
    std::vector<EdgeBrushPlanePair> m_EdgePlanePairs;

    CD::ModelDB* m_pDB;
};

namespace CD
{
    typedef _smart_ptr<ArgumentModel> BrushArgumentPtr;
}