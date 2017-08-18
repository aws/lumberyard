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
//  File name:   SpotManager.h
//  Created:     May/6/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "Core/Polygon.h"

namespace CD
{
    class Model;
};

class SpotManager
{
protected:

    enum ESpotPositionState
    {
        eSpotPosState_InPolygon,
        eSpotPosState_Edge,
        eSpotPosState_CenterOfEdge,
        eSpotPosState_CenterOfPolygon,
        eSpotPosState_EitherPointOfEdge,
        eSpotPosState_FirstPointOfPolygon,
        eSpotPosState_LastPointOfPolygon,
        eSpotPosState_AtFirstSpot,
        eSpotPosState_OnVirtualLine,
        eSpotPosState_OutsideDesigner,
        eSpotPosState_Invalid
    };

    struct SSpot
    {
        SSpot()
            : m_Pos(BrushVec3(0, 0, 0))
        {
            Reset();
        }

        explicit SSpot(const BrushVec3& pos)
            : m_Pos(pos)
        {
            Reset();
        }

        SSpot(const BrushVec3& pos, ESpotPositionState posState, CD::PolygonPtr pPolygon)
            : m_Pos(pos)
            , m_PosState(posState)
            , m_pPolygon(pPolygon)
        {
            if (m_pPolygon)
            {
                m_Plane = m_pPolygon->GetPlane();
            }
            else
            {
                m_Plane = BrushPlane(BrushVec3(0, 0, 0), 0);
            }
            m_bProcessed = false;
        }

        SSpot(const BrushVec3& pos, ESpotPositionState posState, const BrushPlane& plane)
            : m_Pos(pos)
            , m_PosState(posState)
            , m_Plane(plane)
        {
            m_pPolygon = NULL;
            m_bProcessed = false;
        }

        bool IsAtEndPoint() const
        {
            return m_PosState == eSpotPosState_FirstPointOfPolygon || m_PosState == eSpotPosState_LastPointOfPolygon;
        }

        bool IsEquivalentPos(const SSpot& spot) const
        {
            return CD::IsEquivalent(m_Pos, spot.m_Pos);
        }

        bool IsOnEdge() const
        {
            return IsAtEndPoint() || m_PosState == eSpotPosState_Edge || m_PosState == eSpotPosState_CenterOfEdge || m_PosState == eSpotPosState_EitherPointOfEdge;
        }

        bool IsCenterOfEdge() const
        {
            return m_PosState == eSpotPosState_CenterOfEdge;
        }

        bool IsAtEitherPointOnEdge() const
        {
            return IsAtEndPoint() || m_PosState == eSpotPosState_EitherPointOfEdge;
        }

        bool IsInPolygon() const
        {
            return m_PosState == eSpotPosState_InPolygon || m_PosState == eSpotPosState_CenterOfPolygon;
        }

        bool IsSamePos(const SSpot& spot) const
        {
            return CD::IsEquivalent(m_Pos, spot.m_Pos);
        }

        void Reset()
        {
            m_PosState = eSpotPosState_InPolygon;
            m_pPolygon = NULL;
            m_bProcessed = false;
            m_Plane = BrushPlane(BrushVec3(0, 0, 0), 0);
        }

        ESpotPositionState m_PosState;
        bool m_bProcessed;
        BrushVec3 m_Pos;
        BrushPlane m_Plane;
        CD::PolygonPtr m_pPolygon;
    };

    struct SSpotPair
    {
        SSpotPair(){}
        SSpotPair(const SSpot& spot0, const SSpot& spot1)
        {
            m_Spot[0] = spot0;
            m_Spot[1] = spot1;
        }
        SSpot m_Spot[2];
    };

    typedef std::vector<SSpot> SpotList;
    typedef std::vector<SSpotPair> SpotPairList;

protected:

    SpotManager()
    {
        ResetAllSpots();
        m_bBuiltInSnap = false;
        m_BuiltInSnapSize = (BrushFloat)0.5;
        m_bEnableMagnetic = true;
    }
    ~SpotManager(){}

    void DrawPolyline(DisplayContext& dc) const;
    void DrawCurrentSpot(DisplayContext& dc, const BrushMatrix34& worldTM) const;
    void ResetAllSpots()
    {
        m_SpotList.clear();
        m_CurrentSpot.Reset();
        m_StartSpot.Reset();
    }

    virtual void CreatePolygonFromSpots(bool bClosedPolygon, const SpotList& spotList)        {   assert(0); };

    static void GenerateVertexListFromSpotList(const SpotList& spotList, std::vector<BrushVec3>& outVList);

    void RegisterSpotList(CD::Model* pModel, const SpotList& spotList);

    const BrushVec3& GetCurrentSpotPos() const{return m_CurrentSpot.m_Pos; }
    const BrushVec3& GetStartSpotPos() const{return m_StartSpot.m_Pos; }

    void SetCurrentSpotPos(const BrushVec3& vPos){m_CurrentSpot.m_Pos = vPos; }
    void SetStartSpotPos(const BrushVec3& vPos){m_StartSpot.m_Pos = vPos; }

    void SetCurrentSpotPosState(ESpotPositionState state){m_CurrentSpot.m_PosState = state; }
    ESpotPositionState GetCurrentSpotPosState() const{return m_CurrentSpot.m_PosState; }
    void ResetCurrentSpot()
    {
        m_CurrentSpot.Reset();
    }
    void ResetCurrentSpotWeakly()
    {
        m_CurrentSpot.m_PosState = eSpotPosState_InPolygon;
    }

    const SSpot& GetCurrentSpot() const{return m_CurrentSpot; }
    const SSpot& GetStartSpot() const{return m_StartSpot; }

    void SetCurrentSpot(const SSpot& spot){m_CurrentSpot = spot; }
    void SetStartSpot(const SSpot& spot){m_StartSpot = spot; }
    void SetSpotProcessed(int nPos, bool bProcessed){ m_SpotList[nPos].m_bProcessed = bProcessed; }
    void SetPosState(int nPos, ESpotPositionState state){m_SpotList[nPos].m_PosState = state; }

    void SwapCurrentAndStartSpots(){std::swap(m_CurrentSpot, m_StartSpot); }

    int GetSpotListCount() const{return m_SpotList.size(); }
    const BrushVec3& GetSpotPos(int nPos) const {return m_SpotList[nPos].m_Pos; }
    void AddSpotToSpotList(const SSpot& spot);
    void ReplaceSpotList(const SpotList& spotList);

    void ClearSpotList(){m_SpotList.clear(); }

    const SSpot& GetSpot(int nIndex) const { return m_SpotList[nIndex]; }
    const SpotList& GetSpotList() const { return m_SpotList; }

    void SplitSpotList(CD::Model* pModel, const SpotList& spotList, std::vector<SpotList>& outSpotLists);
    void SplitSpot(CD::Model* pModel, const SSpotPair& spotPair, SpotPairList& outSpotPairs);

    bool UpdateCurrentSpotPosition(CD::Model* pModel, const BrushMatrix34& worldTM, const BrushPlane& plane, IDisplayViewport* view, const QPoint& point, bool bKeepInitialPlane, bool bSearchAllShelves = false);
    SSpot Convert2Spot(CD::Model* pModel, const BrushVec3& pos) const;

    bool GetPlaneBeginEndPoints(const BrushPlane& plane, BrushVec2& outProjectedStartPt, BrushVec2& outProjectedEndPt) const;

    bool GetPosAndPlaneBasedOnWorld(IDisplayViewport* view, const QPoint& point, const BrushMatrix34& worldTM, BrushVec3& outPos, BrushPlane& outPlane);

    bool IsSnapEnabled() const;
    BrushVec3 Snap(const BrushVec3& vPos) const;

    void EnableBuiltInSnap(bool bEnabled) { m_bBuiltInSnap = bEnabled; }
    void SetBuiltInSnapSize(BrushFloat fSnapSize) { m_BuiltInSnapSize = fSnapSize; }

    void EnableMagnetic(bool bEnabled) {  m_bEnableMagnetic = bEnabled; }

private:

    struct SCandidateInfo
    {
        SCandidateInfo(const BrushVec3& pos, ESpotPositionState spotPosState)
            : m_Pos(pos)
            , m_SpotPosState(spotPosState){}
        BrushVec3 m_Pos;
        ESpotPositionState m_SpotPosState;
    };

    bool FindBestPlane(CD::Model* pModel, const SSpot& s0, const SSpot& s1, BrushPlane& outPlane);
    bool AddPolygonToDesignerFromSpotList(CD::Model* pModel, const SpotList& spotList);
    bool FindSnappedSpot(const BrushMatrix34& worldTM, CD::PolygonPtr pPickedPolygon, const BrushVec3& pickedPos, SSpot& outSpot) const;
    bool FindNicestSpot(IDisplayViewport* pViewport, const std::vector<SCandidateInfo>& candidates, const CD::Model* pModel, const BrushMatrix34& worldTM, const BrushVec3& pickedPos, CD::PolygonPtr pPickedPolygon, const BrushPlane& plane, SSpot& outSpot) const;
    bool FindSpotNearAxisAlignedLine(IDisplayViewport* pViewport, CD::PolygonPtr pPolygon, const BrushMatrix34& worldTM, SSpot& outSpot);

    struct SIntersectionInfo
    {
        int nPolygonIndex;
        BrushVec3 vIntersection;
    };

    SSpot m_CurrentSpot;
    SSpot m_StartSpot;
    SpotList m_SpotList;

    bool m_bEnableMagnetic;
    bool m_bBuiltInSnap;
    BrushFloat m_BuiltInSnapSize;
};