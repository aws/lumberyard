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
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BevelTool.h
//  Created:     Oct/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Tools/Select/SelectTool.h"

class BevelTool
    : public SelectTool
{
public:

    BevelTool(CD::EDesignerTool tool)
        : SelectTool(tool)
        , m_nMousePrevY(0)
        , m_fDelta(0)
    {
    }

    void OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point) override;
    void OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point) override;
    bool OnKeyDown(CViewport* view, uint32 nKeycode, uint32 nRepCnt, uint32 nFlags) override;
    void Display(DisplayContext& dc) override;

    void Enter() override;
    void Leave() override;

private:

    typedef std::vector< std::pair<BrushVec3, BrushVec3> > MapBewteenSpreadedVertexAndApex;
    typedef std::map<int, std::vector<BrushEdge3D> > MapBetweenElementIndexAndEdges;
    typedef std::map<int, CD::PolygonPtr > MapBetweenElementIndexAndOrignialPolygon;
    struct SMappingInfo
    {
        void Reset()
        {
            mapSpreadedVertex2Apex.clear();
            mapElementIdx2Edges.clear();
            mapElementIdx2OriginalPolygon.clear();
            vertexSetToMakePolygon.clear();
        }
        MapBewteenSpreadedVertexAndApex mapSpreadedVertex2Apex;
        MapBetweenElementIndexAndEdges mapElementIdx2Edges;
        MapBetweenElementIndexAndOrignialPolygon mapElementIdx2OriginalPolygon;
        std::set<BrushVec3> vertexSetToMakePolygon;
    };

    // First - Polygon, Second - EdgeIndex
    typedef std::pair<CD::PolygonPtr, int> EdgeIdentifier;
    struct SResultForNextPhase
    {
        void Reset()
        {
            mapBetweenEdgeIdToApex.clear();
            mapBetweenEdgeIdToVertex.clear();
            middlePhaseEdgePolygons.clear();
            middlePhaseSidePolygons.clear();
            middlePhaseBottomPolygons.clear();
            middlePhaseApexPolygons.clear();
        }
        std::map<EdgeIdentifier, BrushVec3> mapBetweenEdgeIdToApex;
        std::map<EdgeIdentifier, BrushVec3> mapBetweenEdgeIdToVertex;
        std::vector<CD::PolygonPtr> middlePhaseEdgePolygons;
        std::vector<CD::PolygonPtr> middlePhaseSidePolygons;
        std::vector<CD::PolygonPtr> middlePhaseBottomPolygons;
        std::vector<CD::PolygonPtr> middlePhaseApexPolygons;
    };
    SResultForNextPhase m_ResultForSecondPhase;

    bool PP0_Initialize(bool bSpreadEdge = false);

    void PP0_SpreadEdges(int offset, bool bSpreadEdge = true);
    bool PP1_PushEdgesAndVerticesOut(SResultForNextPhase& outResultForNextPhase, SMappingInfo& outMappingInfo);
    void PP1_MakeEdgePolygons(const SMappingInfo& mappingInfo, SResultForNextPhase& outResultForNextPhase);
    void PP2_MapBetweenEdgeIdToApexPos(
        const SMappingInfo& mappingInfo,
        CD::PolygonPtr pEdgePolygon,
        const BrushEdge3D& sideEdge0,
        const BrushEdge3D& sideEdge1,
        SResultForNextPhase& outResultForNextPhase);
    void PP1_MakeApexPolygons(const SMappingInfo& mappingInfo, SResultForNextPhase& outResultForNextPhase);
    void PP0_SubdivideSpreadedEdge(int nSubdivideNum);

    struct SInfoForSubdivingApexPolygon
    {
        BrushEdge3D edge;
        std::vector< std::pair<BrushVec3, BrushVec3> > vIntermediate;
    };
    void PP1_SubdivideApexPolygon(int nSubdivideNum, const std::vector<SInfoForSubdivingApexPolygon>& infoForSubdividingApexPolygonList);

private:

    int GetEdgeCountHavingVertexInElementList(const BrushVec3& vertex, const ElementManager& elementList) const;
    int FindCorrespondingEdge(const BrushEdge3D& e, const std::vector<SInfoForSubdivingApexPolygon>& infoForSubdividingApexPolygonList) const;

    std::vector<CD::PolygonPtr> CreateFirstOddSubdividedApexPolygons(const std::vector<const SInfoForSubdivingApexPolygon*>& subdividedEdges);
    std::vector<CD::PolygonPtr> CreateFirstEvenSubdividedApexPolygons(const std::vector<const SInfoForSubdivingApexPolygon*>& subdividedEdges);

    enum EBevelMode
    {
        eBevelMode_Nothing,
        eBevelMode_Spread,
        eBevelMode_Divide,
    };
    EBevelMode m_BevelMode;

    _smart_ptr<CD::Model> m_pOriginalModel;
    ElementManager m_OriginalSelectedElements;

    std::vector<CD::PolygonPtr> m_OriginalPolygons;

    int m_nMousePrevY;
    BrushFloat m_fDelta;
    int m_nDividedNumber;
};