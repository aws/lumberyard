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
#include "HeightManipulator.h"
#include "ExtrusionSnappingHelper.h"
#include "Viewport.h"

namespace CD
{
    HeightManipulator s_HeightManipulator;
}

BrushFloat HeightManipulator::UpdateHeight(const BrushMatrix34& worldTM, CViewport* view, const QPoint& point)
{
    const CCamera& camera = GetIEditor()->GetRenderer()->GetCamera();
    BrushMatrix34 invWorldTM = worldTM.GetInverted();
    BrushVec3 vThirdDirs[] = {
        invWorldTM.TransformVector(camera.GetMatrix().GetColumn0()),
        invWorldTM.TransformVector(camera.GetMatrix().GetColumn2())
    };
    int nThirdIdx = 0;

    BrushFloat fDotNearestZero = (BrushFloat)1.0f;
    for (int i = 0, iCount(sizeof(vThirdDirs) / sizeof(*vThirdDirs)); i < iCount; ++i)
    {
        BrushFloat fDot = m_FloorPlane.Normal().Dot(vThirdDirs[i]);
        if (std::abs(fDot) < fDotNearestZero)
        {
            fDotNearestZero = fDot;
            nThirdIdx = i;
        }
    }

    BrushVec3 v0 = m_vPivot;
    BrushVec3 v1 = m_vPivot + m_FloorPlane.Normal();
    BrushVec3 v2 = m_vPivot + vThirdDirs[nThirdIdx];
    m_HelperPlane = BrushPlane(v0, v1, v2);

    BrushVec3 vLocalSrc;
    BrushVec3 vLocalDir;
    CD::GetLocalViewRay(worldTM, view, point, vLocalSrc, vLocalDir);
    BrushVec3 vHit;
    BrushFloat t;
    if (!m_HelperPlane.HitTest(vLocalSrc, vLocalSrc + vLocalDir, &t, &vHit) || t < 0)
    {
        return 0;
    }

    m_bDisplayable = true;

    return CD::SnapGrid(m_FloorPlane.Distance(vHit));
}

void HeightManipulator::Display(DisplayContext& dc)
{
#ifdef ENABLE_DRAWDEBUGHELPER
    if (m_bDisplayable)
    {
        CD::DrawPlane(dc, m_vPivot, m_HelperPlane, 1000.0f);
    }
#endif
}