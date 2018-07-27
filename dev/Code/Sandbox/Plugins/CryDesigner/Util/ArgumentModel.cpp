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
#include "ArgumentModel.h"
#include "Core/Polygon.h"
#include "Core/ModelDB.h"

using namespace CD;

ArgumentModel::ArgumentModel(CD::Polygon* pPolygon, BrushFloat fScale, CBaseObject* pObject, std::vector<PolygonPtr>* perpendicularPolygons, ModelDB* pDB)
    : m_fHeight(0)
    , m_fScale(fScale)
{
    DESIGNER_ASSERT(pPolygon);

    if (pPolygon)
    {
        m_pPolygon = pPolygon->Clone();
        m_pInitialPolygon = pPolygon->Clone();
        m_BasePlane = pPolygon->GetPlane();

        if (m_pInitialPolygon->HasBridgeEdges())
        {
            m_pInitialOutsidePolygonWithoutBridgeEdges = m_pInitialPolygon->Clone();
            m_pInitialOutsidePolygonWithoutBridgeEdges->RemoveBridgeEdges();
        }
        else
        {
            m_pInitialOutsidePolygonWithoutBridgeEdges = m_pInitialPolygon;
        }

        std::vector<PolygonPtr> oustsidePolygons;
        m_pInitialOutsidePolygonWithoutBridgeEdges->GetSeparatedPolygons(oustsidePolygons, eSP_OuterHull);
        m_pInitialOutsidePolygonWithoutBridgeEdges->GetSeparatedPolygons(m_InitialInsidePolygonsWithoutBrideEdges, eSP_InnerHull);
        if (oustsidePolygons.size() == 1 && !m_InitialInsidePolygonsWithoutBrideEdges.empty())
        {
            m_pInitialOutsidePolygonWithoutBridgeEdges = oustsidePolygons[0];
        }
    }

    if (perpendicularPolygons)
    {
        for (int i = 0, iPerpendicularSize(perpendicularPolygons->size()); i < iPerpendicularSize; ++i)
        {
            PolygonPtr pPerpendicularPolygon = (*perpendicularPolygons)[i];
            if (pPerpendicularPolygon->CheckFlags(ePolyFlag_Mirrored))
            {
                continue;
            }
            for (int k = 0, iEdgeSize(pPolygon->GetEdgeCount()); k < iEdgeSize; ++k)
            {
                BrushEdge3D edge = pPolygon->GetEdge(k);
                if (pPerpendicularPolygon->IsEdgeOnCrust(edge))
                {
                    m_EdgePlanePairs.push_back(EdgeBrushPlanePair(edge, pPerpendicularPolygon->GetPlane()));
                }
            }
        }
    }

    m_pBaseObject = pObject;
    m_pModel = new Model;
    m_pCompiler = new CD::ModelCompiler(CD::eCompiler_CastShadow);
    m_pDB = pDB;
    if (m_pDB)
    {
        m_pDB->AddRef();
    }
}

ArgumentModel::~ArgumentModel()
{
    if (m_pDB)
    {
        m_pDB->Release();
    }
}

bool ArgumentModel::FindPlaneWithEdge(const BrushEdge3D& edge, const BrushPlane& hintPlane, BrushPlane& outPlane)
{
    for (int i = 0, iSize(m_EdgePlanePairs.size()); i < iSize; ++i)
    {
        if (m_EdgePlanePairs[i].first.IsEquivalent(edge))
        {
            if (m_EdgePlanePairs[i].second.IsSameFacing(hintPlane))
            {
                outPlane = m_EdgePlanePairs[i].second;
                return true;
            }
        }
    }
    return false;
}

void ArgumentModel::CompileModel()
{
    m_pCompiler->Compile(m_pBaseObject, m_pModel);
}

void ArgumentModel::Update(EBrushArgumentUpdate updateOp)
{
    if (m_pModel == NULL)
    {
        return;
    }

    if (std::abs(m_fScale) >= kDesignerEpsilon && !m_pPolygon->CheckFlags(ePolyFlag_NonplanarQuad))
    {
        UpdateWithScale(updateOp);
        return;
    }

    m_pModel->Clear();

    m_CapPlane = BrushPlane(m_BasePlane.Normal(), m_BasePlane.Distance() - m_fHeight);

    AddCapPolygons();
    AddSidePolygons();

    if (updateOp == eBAU_UpdateBrush)
    {
        CompileModel();
    }
}

void ArgumentModel::UpdateWithScale(EBrushArgumentUpdate updateOp)
{
    m_pModel->Clear();

    m_CapPlane = BrushPlane(m_BasePlane.Normal(), m_BasePlane.Distance() - m_fHeight);
    AddCapPolygons();

    for (int i = 0, iEdgeSize(m_pInitialPolygon->GetEdgeCount()); i < iEdgeSize; ++i)
    {
        BrushEdge3D initEdge = m_pInitialPolygon->GetEdge(i);
        BrushEdge3D capEdge = m_pPolygon->GetEdge(i);

        std::vector<BrushVec3> v;
        v.resize(4);

        v[0] = capEdge.m_v[1];
        v[1] = capEdge.m_v[0];
        v[2] = initEdge.m_v[0];
        v[3] = initEdge.m_v[1];

        if (m_fHeight < 0.0f)
        {
            std::swap(v[0], v[3]);
            std::swap(v[1], v[2]);
        }

        BrushPlane sidePlane(v[0], v[1], v[2]);

        if (m_pDB)
        {
            m_pDB->FindPlane(sidePlane, sidePlane);
        }

        PolygonPtr pSidePolygon = new CD::Polygon(v, sidePlane, m_pInitialPolygon->GetMaterialID(), &m_pInitialPolygon->GetTexInfo(), true);
        pSidePolygon->SetFlag(m_pInitialPolygon->GetFlag());
        if (m_pDB)
        {
            m_pDB->UpdatePolygonVertices(pSidePolygon);
        }

        m_pModel->AddPolygon(pSidePolygon->Clone(), eOpType_Add);
    }

    if (updateOp == eBAU_UpdateBrush)
    {
        CompileModel();
    }
}

void ArgumentModel::AddCapPolygons()
{
    MoveVertices2Plane(m_CapPlane);
    m_pPolygon->Scale(m_fScale);
    m_pModel->AddPolygon(m_pPolygon->Clone(), eOpType_Add);
}

void ArgumentModel::AddSidePolygons()
{
    if (std::abs(m_fHeight) <= kDesignerEpsilon)
    {
        return;
    }

    std::vector<PolygonPtr> polygons;

    if (!m_pInitialPolygon->CheckFlags(ePolyFlag_NonplanarQuad))
    {
        polygons.push_back(m_pInitialOutsidePolygonWithoutBridgeEdges);
        if (!m_InitialInsidePolygonsWithoutBrideEdges.empty())
        {
            polygons.insert(polygons.end(), m_InitialInsidePolygonsWithoutBrideEdges.begin(), m_InitialInsidePolygonsWithoutBrideEdges.end());
        }
    }
    else
    {
        polygons.push_back(m_pInitialPolygon);
    }

    for (int a = 0, iPolygonCount(polygons.size()); a < iPolygonCount; ++a)
    {
        PolygonPtr pPolygon = polygons[a];

        if (m_pInitialPolygon->CheckFlags(ePolyFlag_NonplanarQuad) && m_pInitialPolygon != pPolygon)
        {
            continue;
        }

        for (int i = 0, iEdgeSize(pPolygon->GetEdgeCount()); i < iEdgeSize; ++i)
        {
            BrushEdge3D e = pPolygon->GetEdge(i);
            std::vector<SVertex> vSideList(4);

            if (m_pInitialPolygon->CheckFlags(ePolyFlag_NonplanarQuad))
            {
                BrushEdge3D cap_e = m_pPolygon->GetEdge(i);
                vSideList[0].pos = cap_e.m_v[1];
                vSideList[1].pos = cap_e.m_v[0];
            }
            else
            {
                m_CapPlane.HitTest(e.m_v[1], e.m_v[1] - m_CapPlane.Normal(), NULL, &vSideList[0].pos);
                m_CapPlane.HitTest(e.m_v[0], e.m_v[0] - m_CapPlane.Normal(), NULL, &vSideList[1].pos);
            }

            vSideList[2].pos = e.m_v[0];
            vSideList[3].pos = e.m_v[1];

            if (m_fHeight < 0.0f)
            {
                std::swap(vSideList[0], vSideList[3]);
                std::swap(vSideList[1], vSideList[2]);
            }

            BrushPlane sidePlane(vSideList[0].pos, vSideList[1].pos, vSideList[2].pos);

            for (int k = 0; k < 4; ++k)
            {
                CD::CalcTexCoords(SBrushPlane<float>(ToVec3(sidePlane.Normal()), 0), pPolygon->GetTexInfo(), ToVec3(vSideList[k].pos), vSideList[k].uv.x, vSideList[k].uv.y);
            }

            if (!FindPlaneWithEdge(e, sidePlane, sidePlane))
            {
                if (m_pDB)
                {
                    m_pDB->FindPlane(sidePlane, sidePlane);
                }
            }

            PolygonPtr pSidePolygon = new CD::Polygon(vSideList, sidePlane, m_pInitialPolygon->GetMaterialID(), &m_pInitialPolygon->GetTexInfo(), true);
            if (!pSidePolygon->IsOpen())
            {
                if (m_pDB)
                {
                    m_pDB->UpdatePolygonVertices(pSidePolygon);
                }
                m_pModel->AddPolygon(pSidePolygon, eOpType_Add);
            }
        }
    }
}

void ArgumentModel::MoveVertices2Plane(const BrushPlane& targetPlane)
{
    m_pPolygon = m_pInitialPolygon->Clone();

    if (m_pPolygon->CheckFlags(ePolyFlag_NonplanarQuad))
    {
        m_pPolygon->Move(targetPlane.Normal() * m_fHeight);
    }
    else
    {
        for (int i = 0, iVertexSize(m_pPolygon->GetVertexCount()); i < iVertexSize; ++i)
        {
            const BrushVec3& in_v = m_pPolygon->GetPos(i);
            BrushVec3 out_v;
            if (targetPlane.HitTest(in_v, in_v - targetPlane.Normal(), NULL, &out_v))
            {
                m_pPolygon->SetPos(i, out_v);
            }
        }
    }

    m_pPolygon->SetPlane(targetPlane);
}

bool ArgumentModel::GetPolygonList(std::vector<PolygonPtr>& outPolygons) const
{
    if (m_pModel->GetPolygonCount() < 2)
    {
        return false;
    }

    outPolygons.push_back(m_pModel->GetPolygon(0));
    GetSidePolygonList(outPolygons);
    return true;
}

bool ArgumentModel::GetSidePolygonList(std::vector<PolygonPtr>& outPolygons) const
{
    int iPolygonSize(m_pModel->GetPolygonCount());

    for (int i = 1; i < iPolygonSize; ++i)
    {
        outPolygons.push_back(m_pModel->GetPolygon(i));
    }

    return true;
}

PolygonPtr ArgumentModel::GetCapPolygon() const
{
    if (m_pModel->GetPolygonCount() <= 0)
    {
        return NULL;
    }
    return m_pModel->GetPolygon(0);
}

void ArgumentModel::SetHeight(BrushFloat fHeight)
{
    if (std::abs(fHeight) < kDesignerEpsilon)
    {
        m_fHeight = 0;
    }
    else
    {
        m_fHeight = fHeight;
    }
}