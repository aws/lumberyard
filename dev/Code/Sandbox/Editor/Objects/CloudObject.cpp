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

// Description : CCloudObject implementation.


#include "StdAfx.h"
#include "CloudObject.h"
#include "Viewport.h"

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CCloudObject::CCloudObject()
{
    m_bNotSharedGeom = false;
    m_bIgnoreNodeUpdate = false;

    m_width = 1;
    m_height = 1;
    m_length = 1;
    mv_spriteRow = -1;
    m_angleVariations = 0.0f;

    mv_sizeSprites = 0;
    mv_randomSizeValue = 0;

    AddVariable(mv_spriteRow, "SpritesRow", "Sprites Row");
    mv_spriteRow.SetLimits(-1, 64);
    AddVariable(m_width, "Width", functor(*this, &CCloudObject::OnSizeChange));
    m_width.SetLimits(0, 99999);
    AddVariable(m_height, "Height", functor(*this, &CCloudObject::OnSizeChange));
    m_height.SetLimits(0, 99999);
    AddVariable(m_length, "Length", functor(*this, &CCloudObject::OnSizeChange));
    m_length.SetLimits(0, 99999);
    AddVariable(m_angleVariations, "AngleVariations", "Angle Variations", functor(*this, &CCloudObject::OnSizeChange), IVariable::DT_ANGLE);
    m_angleVariations.SetLimits(0, 99999);

    AddVariable(mv_sizeSprites, "SizeofSprites", "Size of Sprites", functor(*this, &CCloudObject::OnSizeChange));
    mv_sizeSprites.SetLimits(0, 99999);
    AddVariable(mv_randomSizeValue, "SizeVariation", "Size Variation", functor(*this, &CCloudObject::OnSizeChange));
    mv_randomSizeValue.SetLimits(0, 99999);

    SetColor(QColor(127, 127, 255));
}

//////////////////////////////////////////////////////////////////////////
void CCloudObject::OnSizeChange(IVariable* pVar)
{
    InvalidateTM(eObjectUpdateFlags_PositionChanged | eObjectUpdateFlags_RotationChanged | eObjectUpdateFlags_ScaleChanged);
}

//////////////////////////////////////////////////////////////////////////
void CCloudObject::GetLocalBounds(AABB& box)
{
    Vec3 diag(m_width, m_length, m_height);
    box = AABB(-diag / 2, diag / 2);
}

//////////////////////////////////////////////////////////////////////////
int CCloudObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown || event == eMouseLUp)
    {
        SetPos(view->MapViewToCP(point));

        if (event == eMouseLDown)
        {
            return MOUSECREATE_OK;
        }

        return MOUSECREATE_CONTINUE;
    }
    return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

//////////////////////////////////////////////////////////////////////////
void CCloudObject::Display(DisplayContext& dc)
{
    const Matrix34& wtm = GetWorldTM();
    Vec3 wp = wtm.GetTranslation();

    if (IsSelected())
    {
        dc.SetSelectedColor();
    }
    else if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(GetColor());
    }

    dc.PushMatrix(wtm);
    AABB box;
    GetLocalBounds(box);
    dc.DrawWireBox(box.min, box.max);
    dc.PopMatrix();

    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
bool CCloudObject::HitTest(HitContext& hc)
{
    Vec3 origin = GetWorldPos();

    AABB box;
    GetBoundBox(box);

    float radius = sqrt((box.max.x - box.min.x) * (box.max.x - box.min.x) + (box.max.y - box.min.y) * (box.max.y - box.min.y) + (box.max.z - box.min.z) * (box.max.z - box.min.z)) / 2;

    Vec3 w = origin - hc.raySrc;
    w = hc.rayDir.Cross(w);
    float d = w.GetLength();

    if (d < radius + hc.distanceTolerance)
    {
        hc.dist = hc.raySrc.GetDistance(origin);
        hc.object = this;
        return true;
    }
    return false;
}

#include <Objects/CloudObject.moc>