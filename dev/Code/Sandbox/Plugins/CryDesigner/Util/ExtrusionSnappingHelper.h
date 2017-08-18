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
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   ExtrusionSnappingHelper.h
//  Created:     20/Feb/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Core/Polygon.h"
#include "Core/Model.h"
#include "Tools/ToolCommon.h"

class CViewport;

class ExtrusionSnappingHelper
{
public:

    void Init(CD::Model* pModel)
    {
        m_pModel = pModel;
    }

    void SearchForOppositePolygons(CD::PolygonPtr pCapPolygon);
    bool IsOverOppositePolygon(const CD::PolygonPtr& pCapPolygon, CD::EPushPull pushpull, bool bIgnoreSimilarDirection = false);
    void ApplyOppositePolygons(CD::PolygonPtr pCapPolygon, CD::EPushPull pushpull, bool bHandleTouch = false);
    BrushFloat GetNearestDistanceToOpposite(CD::EPushPull pushpull) const;
    CD::PolygonPtr FindAlignedPolygon(CD::PolygonPtr pCapPolygon, const BrushMatrix34& worldTM, CViewport* view, const QPoint& point) const;

    struct SOpposite
    {
        SOpposite()
        {
            Init();
        }
        void Init()
        {
            polygon = NULL;
            polygonRelation = CD::eER_None;
            nearestDistanceToPolygon = 0;
            bTouchAdjacent = false;
        }
        CD::PolygonPtr polygon;
        CD::EPolygonRelation polygonRelation;
        BrushFloat nearestDistanceToPolygon;
        bool bTouchAdjacent;
    };

private:

    CD::Model* GetModel() const { return m_pModel; }

    _smart_ptr<CD::Model> m_pModel;
    SOpposite m_Opposites[2];
};

namespace CD
{
    extern ExtrusionSnappingHelper s_SnappingHelper;
}
using CD::s_SnappingHelper;
