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
#include "SmoothingGroup.h"
#include "Model.h"
#include "ModelDB.h"
#include "BrushHelper.h"

SmoothingGroup::SmoothingGroup(const std::vector<CD::PolygonPtr>& polygons)
    : m_pStorage(new CD::Model)
{
    Invalidate();
    m_pStorage->AddRef();
    SetPolygons(polygons);
}

SmoothingGroup::~SmoothingGroup()
{
    m_pStorage->Release();
}

void SmoothingGroup::SetPolygons(const std::vector<CD::PolygonPtr>& polygons)
{
    m_pStorage->Clear();

    for (int i = 0, iPolygonCount(polygons.size()); i < iPolygonCount; ++i)
    {
        m_PolygonSet.insert(polygons[i]);
        m_pStorage->AddPolygon(polygons[i], CD::eOpType_Add);
    }

    Invalidate();
}

void SmoothingGroup::AddPolygon(CD::PolygonPtr pPolygon)
{
    if (m_PolygonSet.find(pPolygon) != m_PolygonSet.end())
    {
        return;
    }
    m_PolygonSet.insert(pPolygon);
    m_pStorage->AddPolygon(pPolygon, CD::eOpType_Add);
    Invalidate();
}

bool SmoothingGroup::HasPolygon(CD::PolygonPtr pPolygon) const
{
    return m_PolygonSet.find(pPolygon) != m_PolygonSet.end();
}

bool SmoothingGroup::CalculateNormal(const BrushVec3& vPos, BrushVec3& vOutNormal) const
{
    BrushVec3 vNormal(0, 0, 0);
    CD::ModelDB::QueryResult qResult;
    if (m_pStorage->GetDB()->QueryAsVertex(vPos, qResult) && qResult.size() == 1 && !qResult[0].m_MarkList.empty())
    {
        for (int i = 0, iCount(qResult[0].m_MarkList.size()); i < iCount; ++i)
        {
            vNormal += qResult[0].m_MarkList[i].m_pPolygon->GetPlane().Normal();
        }
        vOutNormal = vNormal.GetNormalized();
        return true;
    }
    return false;
}

int SmoothingGroup::GetPolygonCount() const
{
    return m_pStorage->GetPolygonCount();
}

CD::PolygonPtr SmoothingGroup::GetPolygon(int nIndex) const
{
    return m_pStorage->GetPolygon(nIndex);
}

std::vector<CD::PolygonPtr> SmoothingGroup::GetAll() const
{
    std::vector<CD::PolygonPtr> polygons;
    m_pStorage->GetPolygonList(polygons);
    return polygons;
}

void SmoothingGroup::Updatemesh(bool bGenerateBackFaces)
{
    m_pStorage->ResetDB(CD::eDBRF_Vertex);
    m_FlexibleMesh.Clear();

    for (int i = 0, iPolygonCount(GetPolygonCount()); i < iPolygonCount; ++i)
    {
        int nVertexOffset = m_FlexibleMesh.vertexList.size();
        if (GetPolygon(i)->CheckFlags(CD::ePolyFlag_Hidden))
        {
            continue;
        }
        CD::CreateMeshFacesFromPolygon(GetPolygon(i), m_FlexibleMesh, bGenerateBackFaces);
        for (int k = nVertexOffset, iVertexCount(m_FlexibleMesh.vertexList.size()); k < iVertexCount; ++k)
        {
            CalculateNormal(m_FlexibleMesh.vertexList[k].pos, m_FlexibleMesh.normalList[k]);
        }
    }
}

void SmoothingGroup::RemovePolygon(CD::PolygonPtr pPolygon)
{
    m_PolygonSet.erase(pPolygon);
    m_pStorage->RemovePolygon(pPolygon);
    Invalidate();
}

const CD::FlexibleMesh& SmoothingGroup::GetFlexibleMesh(bool bGenerateBackFaces)
{
    if (!bGenerateBackFaces && !m_bValidmesh[0] || bGenerateBackFaces && !m_bValidmesh[1])
    {
        if (bGenerateBackFaces)
        {
            m_bValidmesh[1] = true;
        }
        else
        {
            m_bValidmesh[0] = true;
        }
        Updatemesh(bGenerateBackFaces);
    }
    return m_FlexibleMesh;
}