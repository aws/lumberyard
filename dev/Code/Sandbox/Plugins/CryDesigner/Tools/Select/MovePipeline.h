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
#include "Tools/Select/SelectTool.h"
#include "Core/ModelDB.h"

class MovePipeline
{
public:

    MovePipeline(){}
    ~MovePipeline(){}

    void TransformSelections(CD::SMainContext& mc, const BrushMatrix34& offsetTM);

    void SetQueryResultsFromSelectedElements(const ElementManager& selectedElements);
    bool ExcutedAdditionPass() const { return m_bExecutedAdditionPass; }
    void SetExcutedAdditionPass(bool bExcuted){ m_bExecutedAdditionPass = bExcuted; }
    void CreateOrganizedResultsAroundPolygonFromQueryResults();
    void ComputeIntermediatePositionsBasedOnInitQueryResults(const BrushMatrix34& offsetTM);
    CD::ModelDB::Mark& GetMark(const SelectTool::QueryInput& qInput){ return m_QueryResult[qInput.first].m_MarkList[qInput.second]; }
    bool VertexAdditionFirstPass();
    bool VertexAdditionSecondPass();
    bool SubdivisionPass();
    void TransformationPass();
    bool MergeCoplanarPass();
    void AssignIntermediatedPosToSelectedElements(ElementManager& selectedElements);
    CD::PolygonPtr FindAdjacentPolygon(CD::PolygonPtr pPolygon, const BrushVec3& vPos, int& outAdjacentPolygonIndex);
    void Initialize(const ElementManager& elements);
    void InitializeIndependently(ElementManager& elements);
    void End();
    bool GetAveragePos(BrushVec3& outAveragePos) const;
    void SetModel(CD::Model* pModel) { m_pModel = pModel; }

private:

    void SnappedToMirrorPlane();
    CD::Model* GetModel(){return m_pModel; }

    CD::Model* m_pModel;
    std::set<CD::PolygonPtr> m_UnsubdividedPolygons;
    std::map<CD::PolygonPtr, CD::PolygonList> m_SubdividedPolygons;
    CD::ModelDB::QueryResult m_QueryResult;
    CD::ModelDB::QueryResult m_InitQueryResult;
    std::vector<BrushVec3> m_IntermediateTransQueryPos;
    SelectTool::OrganizedQueryResults m_OrganizedQueryResult;
    bool m_bExecutedAdditionPass;
};