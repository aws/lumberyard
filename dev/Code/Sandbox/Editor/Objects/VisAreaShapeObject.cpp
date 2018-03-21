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
#include "VisAreaShapeObject.h"

#include <I3DEngine.h>

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
C3DEngineAreaObjectBase::C3DEngineAreaObjectBase()
{
    m_area = 0;
    mv_closed = true;
    m_bPerVertexHeight = true;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef C3DEngineAreaObjectBase::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = CBaseObject::Export(levelPath, xmlNode);

    // Export Points
    if (!m_points.empty())
    {
        const Matrix34& wtm = GetWorldTM();
        XmlNodeRef points = objNode->newChild("Points");
        for (int i = 0; i < m_points.size(); i++)
        {
            XmlNodeRef pnt = points->newChild("Point");
            pnt->setAttr("Pos", wtm.TransformPoint(m_points[i]));
        }
    }

    return objNode;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngineAreaObjectBase::Done()
{
    if (m_area)
    {
        // reset the listener vis area in the unlucky case that we are deleting the
        // vis area where the listener is currently in
        // Audio: do we still need this?
        GetIEditor()->Get3DEngine()->DeleteVisArea(m_area);
        m_area = 0;
    }
    CShapeObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool C3DEngineAreaObjectBase::CreateGameObject()
{
    if (!m_area)
    {
        uint64 visGUID = *((uint64*)&GetId());
        m_area = GetIEditor()->Get3DEngine()->CreateVisArea(visGUID);
        m_bAreaModified = true;
        UpdateGameArea(false);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
// CVisAreaShapeObject
//////////////////////////////////////////////////////////////////////////
CVisAreaShapeObject::CVisAreaShapeObject()
{
    mv_height = 5;
    m_bDisplayFilledWhenSelected = true;
    // Ambient color zeroed out and hidden in Editor, no longer supported in VisAreas/Portals with PBR
    mv_vAmbientColor = Vec3(ZERO);
    mv_bAffectedBySun = false;
    mv_bIgnoreSkyColor = false;
    mv_fViewDistRatio = 100.f;
    mv_bSkyOnly = false;
    mv_bOceanIsVisible = false;
    mv_bIgnoreGI = false;

    SetColor(QColor(255, 128, 0));
}

//////////////////////////////////////////////////////////////////////////
void CVisAreaShapeObject::InitVariables()
{
    AddVariable(mv_height, "Height", functor(*this, &CVisAreaShapeObject::OnShapeChange));
    AddVariable(mv_displayFilled, "DisplayFilled");

    AddVariable(mv_bAffectedBySun, "AffectedBySun", functor(*this, &CVisAreaShapeObject::OnShapeChange));
    AddVariable(mv_bIgnoreSkyColor, "IgnoreSkyColor", functor(*this, &CVisAreaShapeObject::OnShapeChange));
    AddVariable(mv_bIgnoreGI, "IgnoreGI", functor(*this, &CVisAreaShapeObject::OnShapeChange));
    AddVariable(mv_fViewDistRatio, "ViewDistRatio", functor(*this, &CVisAreaShapeObject::OnShapeChange));
    AddVariable(mv_bSkyOnly, "SkyOnly", functor(*this, &CVisAreaShapeObject::OnShapeChange));
    AddVariable(mv_bOceanIsVisible, "OceanIsVisible", functor(*this, &CVisAreaShapeObject::OnShapeChange));
}

//////////////////////////////////////////////////////////////////////////
void CVisAreaShapeObject::UpdateGameArea(bool bRemove)
{
    if (bRemove)
    {
        return;
    }
    if (!m_bAreaModified)
    {
        return;
    }

    if (m_area)
    {
        std::vector<Vec3> points;
        if (GetPointCount() > 3)
        {
            const Matrix34& wtm = GetWorldTM();
            points.resize(GetPointCount());
            for (int i = 0; i < GetPointCount(); i++)
            {
                points[i] = wtm.TransformPoint(GetPoint(i));
            }

            Vec3 vAmbClr = mv_vAmbientColor;

            SVisAreaInfo info;
            info.fHeight = GetHeight();
            info.vAmbientColor = vAmbClr;
            info.bAffectedByOutLights = mv_bAffectedBySun;
            info.bIgnoreSkyColor = mv_bIgnoreSkyColor;
            info.bSkyOnly = mv_bSkyOnly;
            info.fViewDistRatio = mv_fViewDistRatio;
            info.bDoubleSide = true;
            info.bUseDeepness = false;
            info.bUseInIndoors = false;
            info.bOceanIsVisible = mv_bOceanIsVisible;
            info.bIgnoreGI = mv_bIgnoreGI;
            info.fPortalBlending = -1;

            GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &points[0], points.size(), GetName().toUtf8().data(), info, true);
        }
    }
    m_bAreaModified = false;
}

//////////////////////////////////////////////////////////////////////////
// CPortalShapeObject
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CPortalShapeObject::CPortalShapeObject()
{
    m_bDisplayFilledWhenSelected = true;
    SetColor(QColor(100, 250, 60));

    mv_bUseDeepness = true;
    mv_bDoubleSide = true;
    mv_bPortalBlending = true;
    mv_fPortalBlendValue = 0.5f;
}

//////////////////////////////////////////////////////////////////////////
void CPortalShapeObject::InitVariables()
{
    CVisAreaShapeObject::InitVariables();

    AddVariable(mv_bUseDeepness, "UseDeepness", functor(*this, &CPortalShapeObject::OnShapeChange));
    AddVariable(mv_bDoubleSide, "DoubleSide", functor(*this, &CPortalShapeObject::OnShapeChange));
    AddVariable(mv_bPortalBlending, "LightBlending", functor(*this, &CPortalShapeObject::OnShapeChange));
    AddVariable(mv_fPortalBlendValue, "LightBlendValue", functor(*this, &CPortalShapeObject::OnShapeChange));

    mv_fPortalBlendValue.SetLimits(0.0f, 1.0f);
}

//////////////////////////////////////////////////////////////////////////
void CPortalShapeObject::UpdateGameArea(bool bRemove)
{
    if (bRemove)
    {
        return;
    }

    if (!m_bAreaModified)
    {
        return;
    }

    if (m_area)
    {
        std::vector<Vec3> points;
        if (GetPointCount() > 3)
        {
            const Matrix34& wtm = GetWorldTM();
            points.resize(GetPointCount());
            for (int i = 0; i < GetPointCount(); i++)
            {
                points[i] = wtm.TransformPoint(GetPoint(i));
            }

            QString name = QString("Portal_") + GetName();

            Vec3 vAmbClr = mv_vAmbientColor;

            SVisAreaInfo info;
            info.fHeight = GetHeight();
            info.vAmbientColor = vAmbClr;
            info.bAffectedByOutLights = mv_bAffectedBySun;
            info.bIgnoreSkyColor = mv_bIgnoreSkyColor;
            info.bSkyOnly = mv_bSkyOnly;
            info.fViewDistRatio = mv_fViewDistRatio;
            info.bDoubleSide = true;
            info.bUseDeepness = false;
            info.bUseInIndoors = false;
            info.bOceanIsVisible = mv_bOceanIsVisible;
            info.bIgnoreGI = mv_bIgnoreGI;
            info.fPortalBlending = -1;
            if (mv_bPortalBlending)
            {
                info.fPortalBlending = mv_fPortalBlendValue;
            }

            GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &points[0], points.size(), name.toUtf8().data(), info, true);
        }
    }
    m_bAreaModified = false;
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
COccluderPlaneObject::COccluderPlaneObject()
{
    m_bDisplayFilledWhenSelected = true;
    SetColor(QColor(200, 128, 60));

    mv_height = 5;
    mv_displayFilled = true;

    mv_bUseInIndoors = false;
    mv_bDoubleSide = true;
    mv_fViewDistRatio = 100.f;
}

//////////////////////////////////////////////////////////////////////////
void COccluderPlaneObject::InitVariables()
{
    AddVariable(mv_height, "Height", functor(*this, &COccluderPlaneObject::OnShapeChange));
    AddVariable(mv_displayFilled, "DisplayFilled");

    AddVariable(mv_fViewDistRatio, "CullDistRatio", functor(*this, &COccluderPlaneObject::OnShapeChange));
    AddVariable(mv_bUseInIndoors, "UseInIndoors", functor(*this, &COccluderPlaneObject::OnShapeChange));
    AddVariable(mv_bDoubleSide, "DoubleSide", functor(*this, &COccluderPlaneObject::OnShapeChange));
}

//////////////////////////////////////////////////////////////////////////
void COccluderPlaneObject::UpdateGameArea(bool bRemove)
{
    if (bRemove)
    {
        return;
    }
    if (!m_bAreaModified)
    {
        return;
    }

    if (m_area)
    {
        std::vector<Vec3> points;
        if (GetPointCount() > 1)
        {
            const Matrix34& wtm = GetWorldTM();
            points.resize(GetPointCount());
            for (int i = 0; i < GetPointCount(); i++)
            {
                points[i] = wtm.TransformPoint(GetPoint(i));
            }
            if (!m_points[0].IsEquivalent(m_points[1]))
            {
                QString name = QString("OcclArea_") + GetName();

                SVisAreaInfo info;
                info.fHeight = GetHeight();
                info.vAmbientColor = Vec3(0, 0, 0);
                info.bAffectedByOutLights = false;
                info.bSkyOnly = false;
                info.fViewDistRatio = mv_fViewDistRatio;
                info.bDoubleSide = true;
                info.bUseDeepness = false;
                info.bUseInIndoors = mv_bUseInIndoors;
                info.bOceanIsVisible = false;
                info.fPortalBlending = -1;

                GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &points[0], points.size(), name.toUtf8().data(), info, false);
            }
        }
    }
    m_bAreaModified = false;
}

//////////////////////////////////////////////////////////////////////////
void COccluderPlaneObject::PostLoad(CObjectArchive& ar)
{
    C3DEngineAreaObjectBase::PostLoad(ar);

    // July 2013: This needs for compatibility with old saved levels. Can be removed later.
    if (m_points.size() == 4)
    {
        GetIEditor()->GetObjectManager()->ConvertToType(this, "OccluderArea");
    }
}


//////////////////////////////////////////////////////////////////////////
// COccluderAreaObject
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
COccluderAreaObject::COccluderAreaObject()
{
    m_bDisplayFilledWhenSelected = true;
    m_bForce2D = true;
    m_bNoCulling = true;
    SetColor(QColor(200, 128, 60));

    mv_displayFilled = true;

    mv_bUseInIndoors = false;
    mv_fViewDistRatio = 100.f;
}

//////////////////////////////////////////////////////////////////////////
void COccluderAreaObject::InitVariables()
{
    AddVariable(mv_displayFilled, "DisplayFilled");

    AddVariable(mv_fViewDistRatio, "CullDistRatio", functor(*this, &COccluderAreaObject::OnShapeChange));
    AddVariable(mv_bUseInIndoors, "UseInIndoors", functor(*this, &COccluderAreaObject::OnShapeChange));
}

//////////////////////////////////////////////////////////////////////////
void COccluderAreaObject::UpdateGameArea(bool bRemove)
{
    if (bRemove)
    {
        return;
    }
    if (!m_bAreaModified)
    {
        return;
    }

    if (m_area)
    {
        std::vector<Vec3> points;
        if (GetPointCount() > 3)
        {
            const Matrix34& wtm = GetWorldTM();
            points.resize(GetPointCount());
            for (int i = 0; i < GetPointCount(); i++)
            {
                points[i] = wtm.TransformPoint(GetPoint(i));
            }
            {
                QString name = QString("OcclArea_") + GetName();

                SVisAreaInfo info;
                info.fHeight = 0;
                info.vAmbientColor = Vec3(0, 0, 0);
                info.bAffectedByOutLights = false;
                info.bSkyOnly = false;
                info.fViewDistRatio = mv_fViewDistRatio;
                info.bDoubleSide = true;
                info.bUseDeepness = false;
                info.bUseInIndoors = mv_bUseInIndoors;
                info.bOceanIsVisible = false;
                info.fPortalBlending = -1;

                GetIEditor()->Get3DEngine()->UpdateVisArea(m_area, &points[0], points.size(), name.toUtf8().data(), info, false);
            }
        }
    }
    m_bAreaModified = false;
}

//////////////////////////////////////////////////////////////////////////
void COccluderAreaObject::PostLoad(CObjectArchive& ar)
{
    C3DEngineAreaObjectBase::PostLoad(ar);

    // July 2013: This needs for compatibility with old saved levels. Can be removed later.
    if (m_points.size() == 2)
    {
        GetIEditor()->GetObjectManager()->ConvertToType(this, "OccluderPlane");
    }
}

#include <Objects/VisAreaShapeObject.moc>