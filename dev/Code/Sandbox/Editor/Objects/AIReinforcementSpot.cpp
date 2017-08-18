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
#include "AIReinforcementSpot.h"

#include "Include/IIconManager.h"
#include "IAgent.h"

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
float CAIReinforcementSpot::m_helperScale = 1;

//////////////////////////////////////////////////////////////////////////
CAIReinforcementSpot::CAIReinforcementSpot()
{
    m_entityClass = "AIReinforcementSpot";
}

//////////////////////////////////////////////////////////////////////////
void CAIReinforcementSpot::Done()
{
    CEntityObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CAIReinforcementSpot::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    SetColor(QColor(255, 196, 0));
    bool res = CEntityObject::Init(ie, prev, file);

    return res;
}

//////////////////////////////////////////////////////////////////////////
float CAIReinforcementSpot::GetRadius()
{
    return 0.5f * m_helperScale * gSettings.gizmo.helpersScale;
}

//////////////////////////////////////////////////////////////////////////
void CAIReinforcementSpot::Display(DisplayContext& dc)
{
    const Matrix34& wtm = GetWorldTM();

    Vec3 wp = wtm.GetTranslation();

    if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(GetColor());
    }

    Matrix34 tm(wtm);
    dc.RenderObject(eStatObject_ReinforcementSpot, tm);

    if (IsSelected())
    {
        dc.SetColor(GetColor());

        dc.PushMatrix(wtm);
        float r = GetRadius();
        dc.DrawWireBox(-Vec3(r, r, r), Vec3(r, r, r));

        float callRadius = 0.0f;
        float avoidRadius = 0.0f;

        if (GetIEntity())
        {
            IScriptTable* pTable = GetIEntity()->GetScriptTable();
            if (pTable)
            {
                SmartScriptTable props;
                if (pTable->GetValue("Properties", props))
                {
                    props->GetValue("AvoidWhenTargetInRadius", avoidRadius);
                    props->GetValue("radius", callRadius);
                }
            }
        }

        if (callRadius > 0.0f)
        {
            dc.SetColor(GetColor());
            dc.DrawWireSphere(Vec3(0, 0, 0), callRadius);
        }

        if (avoidRadius > 0.0f)
        {
            ColorB col;
            // Blend between red and the selected color
            col.r = (255 + GetColor().red()) / 2;
            col.g = (0 + GetColor().green()) / 2;
            col.b = (0 + GetColor().blue()) / 2;
            col.a = 255;
            dc.SetColor(col);
            dc.DrawWireSphere(Vec3(0, 0, 0), avoidRadius);
        }

        dc.PopMatrix();
    }
    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
bool CAIReinforcementSpot::HitTest(HitContext& hc)
{
    Vec3 origin = GetWorldPos();
    float radius = GetRadius();

    Vec3 w = origin - hc.raySrc;
    w = hc.rayDir.Cross(w);
    float d = w.GetLength();

    if (d < radius + hc.distanceTolerance)
    {
        hc.dist = hc.raySrc.GetDistance(origin);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAIReinforcementSpot::GetLocalBounds(AABB& box)
{
    float r = GetRadius();
    box.min = -Vec3(r, r, r);
    box.max = Vec3(r, r, r);
}

#include <Objects/AIReinforcementSpot.moc>