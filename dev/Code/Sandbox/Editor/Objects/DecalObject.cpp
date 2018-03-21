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

#include "Viewport.h"
#include "DecalObjectPanel.h"

#include "EntityObject.h"
#include "Geometry/EdMesh.h"
#include "Material/MaterialManager.h"

#include <I3DEngine.h>

#include "DecalObject.h"

#include <AzCore/Casting/numeric_cast.h>

namespace
{
    CDecalObjectPanel* s_pDecalPanel(0);
    int s_pDecalPanelId(0);
}


//////////////////////////////////////////////////////////////////////////
// CDecalObject implementation.
//////////////////////////////////////////////////////////////////////////

CDecalObject::CDecalObject()
{
    m_renderFlags = 0;
    m_projectionType = 0;
    m_deferredDecal = false;

    m_projectionType.AddEnumItem("Planar", SDecalProperties::ePlanar);
    m_projectionType.AddEnumItem("ProjectOnTerrain", SDecalProperties::eProjectOnTerrain);
    m_projectionType.AddEnumItem("ProjectOnTerrainAndStaticObjects", SDecalProperties::eProjectOnTerrainAndStaticObjects);

    AddVariable(m_projectionType, "ProjectionType", functor(*this, &CDecalObject::OnParamChanged));
    m_projectionType.SetDescription("0-Planar\n1-ProjectOnStaticObjects\n2-ProjectOnTerrain\n3-ProjectOnTerrainAndStaticObjects");

    AddVariable(m_deferredDecal, "Deferred", functor(*this, &CDecalObject::OnParamChanged));

    m_viewDistanceMultiplier = 1.0f;
    AddVariable(m_viewDistanceMultiplier, "ViewDistanceMultiplier", "View Distance Multiplier", functor(*this, &CDecalObject::OnViewDistRatioChanged));
    m_viewDistanceMultiplier.SetLimits(0.0f, IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX);

    m_sortPrio = 16;
    AddVariable(m_sortPrio, "SortPriority", functor(*this, &CDecalObject::OnParamChanged));
    m_sortPrio.SetLimits(0, 255);

    m_depth = 1.0f;
    AddVariable(m_depth, "ProjectionDepth", "Projection Depth (Deferred)", functor(*this, &CDecalObject::OnParamChanged));
    m_depth.SetLimits(0.0f, 10.0f, 1.0f / 255.0f, true, true);

    m_pRenderNode = 0;
    m_pPrevRenderNode = 0;

    m_boWasModified = false;
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::BeginEditParams(IEditor* ie, int flags)
{
    CBaseObject::BeginEditParams(ie, flags);
    if (!s_pDecalPanel)
    {
        s_pDecalPanel = new CDecalObjectPanel;
        s_pDecalPanelId = ie->AddRollUpPage(ROLLUP_OBJECTS, tr("Decal"), s_pDecalPanel);
    }

    if (s_pDecalPanel)
    {
        s_pDecalPanel->SetDecalObject(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::EndEditParams(IEditor* ie)
{
    CBaseObject::EndEditParams(ie);
    if (s_pDecalPanel)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, s_pDecalPanelId);
        s_pDecalPanel = 0;
        s_pDecalPanelId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::OnParamChanged(IVariable* pVar)
{
    UpdateEngineNode();
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::OnViewDistRatioChanged(IVariable* pVar)
{
    if (m_pRenderNode)
    {
        m_pRenderNode->SetViewDistanceMultiplier(m_viewDistanceMultiplier);

        // set matrix again to for update of view distance ratio in engine
        const Matrix34& wtm(GetWorldTM());
        m_pRenderNode->SetMatrix(wtm);

        m_boWasModified = true;
    }
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CDecalObject::GetDefaultMaterial() const
{
    CMaterial* pDefMat(GetIEditor()->GetMaterialManager()->LoadMaterial("EngineAssets/Materials/Decals/Default"));
    pDefMat->AddRef();
    return pDefMat;
}

//////////////////////////////////////////////////////////////////////////
bool CDecalObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    CDecalObject* pSrcDecalObj((CDecalObject*) prev);

    SetColor(QColor(127, 127, 255));

    if (IsCreateGameObjects())
    {
        if (pSrcDecalObj)
        {
            m_pPrevRenderNode = pSrcDecalObj->m_pRenderNode;
        }
    }

    // Must be after SetDecal call.
    bool res = CBaseObject::Init(ie, prev, file);

    if (pSrcDecalObj)
    {
        // clone custom properties
        m_projectionType = pSrcDecalObj->m_projectionType;
        m_deferredDecal = pSrcDecalObj->m_deferredDecal;
        m_renderFlags = pSrcDecalObj->m_renderFlags;
    }
    else
    {
        // new decal, set default material
        SetMaterial(GetDefaultMaterial());
    }

    return res;
}

//////////////////////////////////////////////////////////////////////////
int CDecalObject::GetProjectionType() const
{
    return m_projectionType;
}

//////////////////////////////////////////////////////////////////////////
bool CDecalObject::CreateGameObject()
{
    if (!m_pRenderNode)
    {
        m_pRenderNode = static_cast< IDecalRenderNode* >(GetIEditor()->Get3DEngine()->CreateRenderNode(eERType_Decal));

        if (m_pRenderNode && m_pPrevRenderNode)
        {
            m_pPrevRenderNode = 0;
        }

        UpdateEngineNode();
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::Done()
{
    if (m_pRenderNode)
    {
        GetIEditor()->Get3DEngine()->DeleteRenderNode(m_pRenderNode);
        m_pRenderNode = 0;
    }

    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::UpdateEngineNode()
{
    if (!m_pRenderNode)
    {
        return;
    }

    bool    boIsSelected(IsSelected());

    // update basic entity render flags
    m_renderFlags = 0;

    if (boIsSelected && gSettings.viewports.bHighlightSelectedGeometry)
    {
        m_renderFlags |= ERF_SELECTED;
    }

    if (IsHidden() || IsHiddenBySpec())
    {
        m_renderFlags |= ERF_HIDDEN;
    }

    m_pRenderNode->SetRndFlags(m_renderFlags);

    // set properties
    SDecalProperties decalProperties;
    switch (m_projectionType)
    {
    case SDecalProperties::eProjectOnTerrainAndStaticObjects:
    {
        decalProperties.m_projectionType = SDecalProperties::eProjectOnTerrainAndStaticObjects;
        break;
    }
    case SDecalProperties::eProjectOnTerrain:
    {
        decalProperties.m_projectionType = SDecalProperties::eProjectOnTerrain;
        break;
    }
    case SDecalProperties::ePlanar:
    default:
    {
        decalProperties.m_projectionType = SDecalProperties::ePlanar;
        break;
    }
    }

    // update world transform
    const Matrix34& wtm(GetWorldTM());

    // get normalized rotation (remove scaling)
    Matrix33 rotation(wtm);
    if (m_projectionType != SDecalProperties::ePlanar)
    {
        rotation.SetRow(0, rotation.GetRow(0).GetNormalized());
        rotation.SetRow(1, rotation.GetRow(1).GetNormalized());
        rotation.SetRow(2, rotation.GetRow(2).GetNormalized());
    }

    decalProperties.m_pos = wtm.TransformPoint(Vec3(0, 0, 0));
    decalProperties.m_normal = wtm.TransformVector(Vec3(0, 0, 1));
    QByteArray ba = GetMaterialName().toUtf8();
    decalProperties.m_pMaterialName = ba.data();
    decalProperties.m_radius = m_projectionType != SDecalProperties::ePlanar ? decalProperties.m_normal.GetLength() : 1;
    decalProperties.m_explicitRightUpFront = rotation;
    decalProperties.m_sortPrio = m_sortPrio;
    decalProperties.m_deferred = m_deferredDecal;
    decalProperties.m_depth = m_depth;
    m_pRenderNode->SetDecalProperties(decalProperties);

    m_pRenderNode->SetMatrix(wtm);
    m_pRenderNode->SetMinSpec(aznumeric_caster(decalProperties.m_minSpec));
    m_pRenderNode->SetMaterialLayers(GetMaterialLayersMask());
    m_pRenderNode->SetViewDistanceMultiplier(m_viewDistanceMultiplier);

    m_boWasModified = false;
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::SetHidden(bool bHidden)
{
    m_boWasModified = true;
    CBaseObject::SetHidden(bHidden);
    UpdateEngineNode();
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::UpdateVisibility(bool visible)
{
    CBaseObject::UpdateVisibility(visible);
    UpdateEngineNode();
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::SetMinSpec(uint32 nSpec, bool bSetChildren)
{
    m_boWasModified = true;
    CBaseObject::SetMinSpec(nSpec, bSetChildren);
    UpdateEngineNode();
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::SetMaterialLayersMask(uint32 nLayersMask)
{
    m_boWasModified = true;
    CBaseObject::SetMaterialLayersMask(nLayersMask);
    UpdateEngineNode();
}

//////////////////////////////////////////////////////////////////////////
void    CDecalObject::SetMaterial(CMaterial* mtl)
{
    m_boWasModified = true;
    if (!mtl)
    {
        mtl = GetDefaultMaterial();
    }
    CBaseObject::SetMaterial(mtl);
    UpdateEngineNode();
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::InvalidateTM(int nWhyFlags)
{
    m_boWasModified = true;
    CBaseObject::InvalidateTM(nWhyFlags);
    UpdateEngineNode();
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::GetLocalBounds(AABB& box)
{
    box = AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1));
}

//////////////////////////////////////////////////////////////////////////
bool CDecalObject::MouseCallbackImpl(CViewport* view, EMouseEvent event, QPoint& point, int flags, bool callerIsMouseCreateCallback)
{
    static bool s_mousePosTracked(false);

    if ((callerIsMouseCreateCallback && !flags) || (callerIsMouseCreateCallback && (flags & MK_LBUTTON)))
    {
        Vec3 pos(view->MapViewToCP(point));
        SetPos(pos);
        s_mousePosTracked = false;
    }
    else
    {
        static QPoint s_prevMousePos;

        enum EInputMode
        {
            eNone,

            eAlign,
            eScale,
            eRotate
        };

        const bool altKeyPressed(Qt::AltModifier & QApplication::queryKeyboardModifiers());

        EInputMode eInputMode(eNone);
        if ((flags & MK_CONTROL) && !altKeyPressed)
        {
            eInputMode = eAlign;
        }
        else if ((flags & MK_CONTROL) && altKeyPressed)
        {
            eInputMode = eRotate;
        }
        else if (altKeyPressed)
        {
            eInputMode = eScale;
        }

        switch (eInputMode)
        {
        case eAlign:
        {
            Vec3 raySrc, rayDir;
            GetIEditor()->GetActiveView()->ViewToWorldRay(point, raySrc, rayDir);
            rayDir = rayDir.GetNormalized() * 1000.0f;

            ray_hit hit;
            if (gEnv->pPhysicalWorld->RayWorldIntersection(raySrc, rayDir, ent_all, rwi_stop_at_pierceable | rwi_ignore_terrain_holes, &hit, 1) > 0)
            {
                const Matrix34& wtm(GetWorldTM());
                Vec3 x(wtm.GetColumn0().GetNormalized());
                Vec3 y(wtm.GetColumn1().GetNormalized());
                Vec3 z(hit.n.GetNormalized());

                y = z.Cross(x);
                if (y.GetLengthSquared() < 1e-4f)
                {
                    y = z.GetOrthogonal();
                }
                y.Normalize();
                x = y.Cross(z);

                Matrix33 newOrient;
                newOrient.SetColumn(0, x);
                newOrient.SetColumn(1, y);
                newOrient.SetColumn(2, z);
                Quat q(newOrient);

                SetPos(hit.pt, eObjectUpdateFlags_DoNotInvalidate);
                SetRotation(q, eObjectUpdateFlags_DoNotInvalidate);
                InvalidateTM(eObjectUpdateFlags_PositionChanged | eObjectUpdateFlags_RotationChanged);

                m_boWasModified = true;
            }
            break;
        }
        case eRotate:
        {
            if (!s_mousePosTracked)
            {
                break;
            }

            float angle(-DEG2RAD(point.x() - s_prevMousePos.x()));
            SetRotation(GetRotation() * Quat(Matrix33::CreateRotationZ(angle)));

            m_boWasModified = true;
            break;
        }
        case eScale:
        {
            if (!s_mousePosTracked)
            {
                break;
            }

            float scaleDelta(0.01f * (s_prevMousePos.y() - point.y()));
            Vec3 newScale(GetScale() + Vec3(scaleDelta, scaleDelta, scaleDelta));
            newScale.x = max(newScale.x, 0.1f);
            newScale.y = max(newScale.y, 0.1f);
            newScale.z = max(newScale.z, 0.1f);
            SetScale(newScale);

            m_boWasModified = true;
            break;
        }
        default:
        {
            if (event == eMouseLDown)
            {
                return false;
            }
            break;
        }
        }

        s_prevMousePos = point;
        s_mousePosTracked = true;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
int CDecalObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown || event == eMouseLUp)
    {
        MouseCallbackImpl(view, event, point, flags, true);

        if (event == eMouseLDown)
        {
            return MOUSECREATE_OK;
        }

        return MOUSECREATE_CONTINUE;
    }
    return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::Display(DisplayContext& dc)
{
    if (IsSelected())
    {
        const Matrix34& wtm(GetWorldTM());

        Vec3 x1(wtm.TransformPoint(Vec3(-1,  0, 0)));
        Vec3 x2(wtm.TransformPoint(Vec3(1,  0, 0)));
        Vec3 y1(wtm.TransformPoint(Vec3(0, -1, 0)));
        Vec3 y2(wtm.TransformPoint(Vec3(0,  1, 0)));
        Vec3  p(wtm.TransformPoint(Vec3(0,  0, 0)));
        Vec3  n(wtm.TransformPoint(Vec3(0,  0, 1)));

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

        dc.DrawLine(p, n);
        dc.DrawLine(x1, x2);
        dc.DrawLine(y1, y2);

        if (IsSelected())
        {
            Vec3 p0 = wtm.TransformPoint(Vec3(-1, -1, 0));
            Vec3 p1 = wtm.TransformPoint(Vec3(-1,  1, 0));
            Vec3 p2 = wtm.TransformPoint(Vec3(1,  1, 0));
            Vec3 p3 = wtm.TransformPoint(Vec3(1, -1, 0));
            dc.DrawLine(p0, p1);
            dc.DrawLine(p1, p2);
            dc.DrawLine(p2, p3);
            dc.DrawLine(p3, p0);
            dc.DrawLine(p0, p2);
            dc.DrawLine(p1, p3);
        }
    }

    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
void CDecalObject::Serialize(CObjectArchive& ar)
{
    CBaseObject::Serialize(ar);

    XmlNodeRef xmlNode(ar.node);

    if (ar.bLoading)
    {
        // load attributes
        int projectionType(SDecalProperties::ePlanar);
        xmlNode->getAttr("ProjectionType", projectionType);
        m_projectionType = projectionType;

        xmlNode->getAttr("RndFlags", m_renderFlags);

        // update render node
        if (!m_pRenderNode)
        {
            CreateGameObject();
        }

        UpdateEngineNode();
    }
    else
    {
        // save attributes
        xmlNode->setAttr("ProjectionType", m_projectionType);
        xmlNode->setAttr("RndFlags", ERF_GET_WRITABLE(m_renderFlags));
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CDecalObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef decalNode(CBaseObject::Export(levelPath, xmlNode));
    decalNode->setAttr("ProjectionType", m_projectionType);
    return decalNode;
}

//////////////////////////////////////////////////////////////////////////
bool CDecalObject::RayToLineDistance(const Vec3& rayLineP1, const Vec3& rayLineP2, const Vec3& pi, const Vec3& pj, float& distance, Vec3& intPnt)
{
    Vec3 pa, pb;
    float ua, ub;
    if (!LineLineIntersect(pi, pj, rayLineP1, rayLineP2, pa, pb, ua, ub))
    {
        return false;
    }

    // If before ray origin.
    if (ub < 0)
    {
        return false;
    }

    float d(0);
    if (ua < 0)
    {
        d = PointToLineDistance(rayLineP1, rayLineP2, pi, intPnt);
    }
    else if (ua > 1)
    {
        d = PointToLineDistance(rayLineP1, rayLineP2, pj, intPnt);
    }
    else
    {
        intPnt = rayLineP1 + ub * (rayLineP2 - rayLineP1);
        d = (pb - pa).GetLength();
    }
    distance = d;

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CDecalObject::HitTest(HitContext& hc)
{
    // Selectable by icon.
    /*
    const Matrix34& wtm( GetWorldTM() );

    Vec3 x1( wtm.TransformPoint( Vec3( -1,  0, 0 ) ) );
    Vec3 x2( wtm.TransformPoint( Vec3(  1,  0, 0 ) ) );
    Vec3 y1( wtm.TransformPoint( Vec3(  0, -1, 0 ) ) );
    Vec3 y2( wtm.TransformPoint( Vec3(  0,  1, 0 ) ) );
    Vec3  p( wtm.TransformPoint( Vec3(  0,  0, 0 ) ) );
    Vec3  n( wtm.TransformPoint( Vec3(  0,  0, 1 ) ) );

    float minDist( FLT_MAX );
    Vec3 ip;
    Vec3 intPnt;
    Vec3 rayLineP1( hc.raySrc );
    Vec3 rayLineP2( hc.raySrc + hc.rayDir * 100000.0f );

    bool bWasIntersection( false );

    float d( 0 );
    if( RayToLineDistance( rayLineP1, rayLineP2, p, n, d, ip ) && d < minDist )
        { minDist = d; intPnt = ip; bWasIntersection = true; }
    if( RayToLineDistance( rayLineP1, rayLineP2, x1, x2, d, ip ) && d < minDist )
        { minDist = d; intPnt = ip; bWasIntersection = true; }
    if( RayToLineDistance( rayLineP1, rayLineP2, y1, y2, d, ip ) && d < minDist )
        { minDist = d; intPnt = ip; bWasIntersection = true; }

    float fRoadCloseDistance( 0.8f * hc.view->GetScreenScaleFactor( intPnt ) * 0.01f );
    if( bWasIntersection && minDist < fRoadCloseDistance + hc.distanceTollerance )
    {
        hc.dist = hc.raySrc.GetDistance( intPnt );
        return true;
    }
    */

    return false;
}

#include <Objects/DecalObject.moc>