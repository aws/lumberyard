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

// Description : CAreaSphere implementation.


#include "StdAfx.h"
#include "AreaSphere.h"
#include "../Viewport.h"
#include <IEntitySystem.h>
#include <IEntityHelper.h>
#include "Components/IComponentArea.h"

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CAreaSphere::CAreaSphere()
{
    m_areaId = -1;
    m_edgeWidth = 0;
    m_radius = 3;
    mv_groupId = 0;
    mv_priority = 0;
    m_entityClass = "AreaSphere";

    SetColor(QColor(0, 0, 255));
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::InitVariables()
{
    AddVariable(m_areaId, "AreaId", functor(*this, &CAreaSphere::OnAreaChange));
    AddVariable(m_edgeWidth, "FadeInZone", functor(*this, &CAreaSphere::OnSizeChange));
    AddVariable(m_radius, "Radius", functor(*this, &CAreaSphere::OnSizeChange));
    AddVariable(mv_groupId, "GroupId", functor(*this, &CAreaSphere::OnAreaChange));
    AddVariable(mv_priority, "Priority", functor(*this, &CAreaSphere::OnAreaChange));
    AddVariable(mv_filled, "Filled", functor(*this, &CAreaSphere::OnAreaChange));
}

//////////////////////////////////////////////////////////////////////////
bool CAreaSphere::CreateGameObject()
{
    bool bRes = CAreaObjectBase::CreateGameObject();
    if (bRes)
    {
        UpdateGameArea();
    }
    return bRes;
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::GetLocalBounds(AABB& box)
{
    box.min = Vec3(-m_radius, -m_radius, -m_radius);
    box.max = Vec3(m_radius, m_radius, m_radius);
}

//////////////////////////////////////////////////////////////////////////
bool CAreaSphere::HitTest(HitContext& hc)
{
    Vec3 origin = GetWorldPos();
    Vec3 w = origin - hc.raySrc;
    w = hc.rayDir.Cross(w);
    float d = w.GetLength();

    Matrix34 invertWTM = GetWorldTM();
    Vec3 worldPos = invertWTM.GetTranslation();
    float epsilonDist = hc.view->GetScreenScaleFactor(worldPos) * 0.01f;
    if ((d < m_radius + epsilonDist) &&
        (d > m_radius - epsilonDist))
    {
        hc.dist = hc.raySrc.GetDistance(origin);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::OnAreaChange(IVariable* pVar)
{
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::OnSizeChange(IVariable* pVar)
{
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::Display(DisplayContext& dc)
{
    QColor wireColor, solidColor;
    float wireOffset = 0;
    float alpha = 0.3f;
    if (IsSelected())
    {
        wireColor = dc.GetSelectedColor();
        solidColor = GetColor();
        wireOffset = -0.1f;
    }
    else
    {
        wireColor = GetColor();
        solidColor = GetColor();
    }


    const Matrix34& tm = GetWorldTM();
    Vec3 pos = GetWorldPos();

    bool bFrozen = IsFrozen();

    if (bFrozen)
    {
        dc.SetFreezeColor();
    }
    //////////////////////////////////////////////////////////////////////////
    if (!bFrozen)
    {
        dc.SetColor(solidColor, alpha);
    }

    if (IsSelected() || (bool)mv_filled)
    {
        dc.CullOff();
        dc.DepthWriteOff();
        //int rstate = dc.ClearStateFlag( GS_DEPTHWRITE );
        dc.DrawBall(pos, m_radius);
        //dc.SetState( rstate );
        dc.DepthWriteOn();
        dc.CullOn();
    }

    if (!bFrozen)
    {
        dc.SetColor(wireColor, 1);
    }
    dc.DrawWireSphere(pos, m_radius);
    if (m_edgeWidth)
    {
        dc.DrawWireSphere(pos, m_radius - m_edgeWidth);
    }
    //////////////////////////////////////////////////////////////////////////

    if (!m_entities.empty())
    {
        Vec3 vcol = Vec3(1, 1, 1);
        for (int i = 0; i < m_entities.size(); i++)
        {
            CBaseObject* obj = GetEntity(i);
            if (!obj)
            {
                continue;
            }
            dc.DrawLine(GetWorldPos(), obj->GetWorldPos(), ColorF(vcol.x, vcol.y, vcol.z, 0.7f), ColorF(1, 1, 1, 0.7f));
        }
    }

    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::Serialize(CObjectArchive& ar)
{
    CAreaObjectBase::Serialize(ar);
    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        SetAreaId(m_areaId);
    }
    else
    {
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CAreaSphere::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = CAreaObjectBase::Export(levelPath, xmlNode);
    return objNode;
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::SetAreaId(int nAreaId)
{
    m_areaId = nAreaId;
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
int CAreaSphere::GetAreaId()
{
    return m_areaId;
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::SetRadius(float fRadius)
{
    m_radius = fRadius;
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
float CAreaSphere::GetRadius()
{
    return m_radius;
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::PostLoad(CObjectArchive& ar)
{
    // After loading Update game structure.
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
void CAreaSphere::UpdateGameArea()
{
    if (!m_pEntity)
    {
        return;
    }

    IComponentAreaPtr pArea = m_pEntity->GetOrCreateComponent<IComponentArea>();
    if (!pArea)
    {
        return;
    }

    pArea->SetSphere(Vec3(0, 0, 0), m_radius);
    pArea->SetProximity(m_edgeWidth);
    pArea->SetID(m_areaId);
    pArea->SetGroup(mv_groupId);
    pArea->SetPriority(mv_priority);

    pArea->ClearEntities();
    for (int i = 0; i < GetEntityCount(); i++)
    {
        CEntityObject* pEntity = GetEntity(i);
        if (pEntity && pEntity->GetIEntity())
        {
            pArea->AddEntity(pEntity->GetIEntity()->GetId());
        }
    }
}

#include <Objects/AreaSphere.moc>