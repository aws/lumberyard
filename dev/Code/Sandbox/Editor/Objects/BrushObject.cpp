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

// Description : CBrushObject implementation.


#include "StdAfx.h"
#include "BrushObject.h"

#include "../Viewport.h"

#include "../Dialogs/BrushPanel.h"
#include "PanelTreeBrowser.h"
#include "AssetBrowser/AssetBrowserMetaTaggingDlg.h"
#include "AssetBrowser/AssetBrowserManager.h"

#include "EntityObject.h"
#include "Geometry/EdMesh.h"
#include "Material/Material.h"
#include "Material/MaterialManager.h"
#include "ISubObjectSelectionReferenceFrameCalculator.h"

#include <I3DEngine.h>
#include <IEntitySystem.h>
#include <IEntityRenderState.h>
#include <IPhysics.h>
#include "IBreakableManager.h"
#include "IGamePhysicsSettings.h"

#include <QMessageBox>

#define MIN_BOUNDS_SIZE 0.01f

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

namespace
{
    CBrushPanel* s_brushPanel = NULL;
    int s_brushPanelId = 0;

    CPanelTreeBrowser* s_treePanelPtr = NULL;
    int s_treePanelId = 0;
}


//////////////////////////////////////////////////////////////////////////
CBrushObject::CBrushObject()
{
    m_pGeometry = 0;
    m_pRenderNode = 0;

    m_renderFlags = 0;
    m_bNotSharedGeom = false;
    m_bbox.Reset();

    AddVariable(mv_geometryFile, "Prefab", "Geometry", functor(*this, &CBrushObject::OnGeometryChange), IVariable::DT_OBJECT);

    // Init Variables.
    mv_outdoor = false;
    mv_castShadowMaps = true;
    mv_rainOccluder = true;
    mv_registerByBBox = false;
    mv_dynamicDistanceShadows = false;
    mv_noIndirLight = false;
    //  mv_recvLightmap = false;
    mv_hideable = 0;
    mv_ratioLOD = 100;
    mv_viewDistanceMultiplier = 1.0f;
    mv_excludeFromTriangulation = false;
    mv_noDynWater = false;
    mv_aiRadius = -1.0f;
    mv_noDecals = false;
    mv_lightmapQuality = 1;
    mv_lightmapQuality.SetLimits(0, 100);
    mv_recvWind = false;
    mv_integrQuality = 0;
    mv_Occluder = false;
    mv_drawLast = false;
    mv_shadowLodBias = 0;

    static QString sVarName_OutdoorOnly = "OutdoorOnly";
    //  static CString sVarName_CastShadows = "CastShadows";
    //  static CString sVarName_CastShadows2 = _T("CastShadowVolume");
    //static CString sVarName_SelfShadowing = "SelfShadowing";
    static QString sVarName_CastShadowMaps = "CastShadowMaps";
    static QString sVarName_RainOccluder = "RainOccluder";
    static QString sVarName_RegisterByBBox = "SupportSecondVisarea";
    static QString sVarName_DynamicDistanceShadows = "DynamicDistanceShadows";
    static QString sVarName_ReceiveLightmap = "ReceiveRAMmap";
    static QString sVarName_Hideable = "Hideable";
    static QString sVarName_HideableSecondary = "HideableSecondary";
    static QString sVarName_LodRatio = "LodRatio";
    static QString sVarName_ViewDistanceMultiplier = "ViewDistanceMultiplier";
    static QString sVarName_NotTriangulate = "NotTriangulate";
    static QString sVarName_NoDynWater = "NoDynamicWater";
    static QString sVarName_AIRadius = "AIRadius";
    static QString sVarName_LightmapQuality = "RAMmapQuality";
    static QString sVarName_NoDecals = "NoStaticDecals";
    static QString sVarName_Frozen = "Frozen";
    static QString sVarName_NoAmbShadowCaster = "NoAmnbShadowCaster";
    static QString sVarName_RecvWind = "RecvWind";
    static QString sVarName_IntegrQuality = "IntegrationQuality";
    static QString sVarName_Occluder = "Occluder";
    static QString sVarName_DrawLast = "DrawLast";
    static QString sVarName_ShadowLodBias = "ShadowLodBias";

    CVarEnumList<int>* pHideModeList = new CVarEnumList<int>();
    pHideModeList->AddItem("None", 0);
    pHideModeList->AddItem("Hideable", 1);
    pHideModeList->AddItem("Secondary", 2);
    mv_hideable.SetEnumList(pHideModeList);

    ReserveNumVariables(16);

    // Add Collision filtering variables
    m_collisionFiltering.AddToObject(this, functor(*this, &CBrushObject::OnRenderVarChange));

    AddVariable(mv_outdoor, sVarName_OutdoorOnly, functor(*this, &CBrushObject::OnRenderVarChange));
    //  AddVariable( mv_castShadows,sVarName_CastShadows,sVarName_CastShadows2,functor(*this,&CBrushObject::OnRenderVarChange) );
    //AddVariable( mv_selfShadowing,sVarName_SelfShadowing,functor(*this,&CBrushObject::OnRenderVarChange) );
    AddVariable(mv_castShadowMaps, sVarName_CastShadowMaps, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_rainOccluder, sVarName_RainOccluder, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_registerByBBox, sVarName_RegisterByBBox, functor(*this, &CBrushObject::OnRenderVarChange));

    AddVariable(mv_dynamicDistanceShadows, sVarName_DynamicDistanceShadows, functor(*this, &CBrushObject::OnRenderVarChange));
    //AddVariable( mv_recvLightmap,sVarName_ReceiveLightmap,functor(*this,&CBrushObject::OnRenderVarChange) );
    AddVariable(mv_hideable, sVarName_Hideable, functor(*this, &CBrushObject::OnRenderVarChange));
    //  AddVariable( mv_hideableSecondary,sVarName_HideableSecondary,functor(*this,&CBrushObject::OnRenderVarChange) );
    AddVariable(mv_ratioLOD, sVarName_LodRatio, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_viewDistanceMultiplier, sVarName_ViewDistanceMultiplier, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_excludeFromTriangulation, sVarName_NotTriangulate, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_noDynWater, sVarName_NoDynWater, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_aiRadius, sVarName_AIRadius, functor(*this, &CBrushObject::OnAIRadiusVarChange));
    AddVariable(mv_noDecals, sVarName_NoDecals, functor(*this, &CBrushObject::OnRenderVarChange));
    //AddVariable( mv_lightmapQuality,sVarName_LightmapQuality );
    AddVariable(mv_noIndirLight, sVarName_NoAmbShadowCaster);
    AddVariable(mv_recvWind, sVarName_RecvWind, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_Occluder, sVarName_Occluder, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_drawLast, sVarName_DrawLast, functor(*this, &CBrushObject::OnRenderVarChange));
    AddVariable(mv_shadowLodBias, sVarName_ShadowLodBias, functor(*this, &CBrushObject::OnRenderVarChange));

    mv_ratioLOD.SetLimits(0, 255);
    mv_viewDistanceMultiplier.SetLimits(0.0f, IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX);
    mv_integrQuality.SetLimits(0, 16.f);

    m_bIgnoreNodeUpdate = false;

    SetFlags(OBJFLAG_SHOW_ICONONTOP);
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::Done()
{
    FreeGameData();

    CBaseObject::Done();
}

bool CBrushObject::IncludeForGI()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::FreeGameData()
{
    if (m_pRenderNode)
    {
        GetIEditor()->Get3DEngine()->DeleteRenderNode(m_pRenderNode);
        m_pRenderNode = 0;
    }

    StatObjEventBus::Handler::BusDisconnect();

    // Release Mesh.
    if (m_pGeometry)
    {
        m_pGeometry->RemoveUser();
    }
    m_pGeometry = 0;
}

//////////////////////////////////////////////////////////////////////////
bool CBrushObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    SetColor(QColor(255, 255, 255));

    if (IsCreateGameObjects())
    {
        if (prev)
        {
            CBrushObject* brushObj = (CBrushObject*)prev;
        }
        else if (!file.isEmpty())
        {
            // Create brush from geometry.
            mv_geometryFile = file;

            QString name = Path::GetFileName(file);
            SetUniqueName(name);
        }
        //m_indoor->AddBrush( this );
    }

    // Must be after SetBrush call.
    bool res = CBaseObject::Init(ie, prev, file);

    if (prev)
    {
        CBrushObject* brushObj = (CBrushObject*)prev;
        m_bbox = brushObj->m_bbox;
    }

    return res;
}

//////////////////////////////////////////////////////////////////////////
bool CBrushObject::CreateGameObject()
{
    if (!m_pRenderNode)
    {
        uint32 ForceID =    GetObjectManager()->ForceID();
        m_pRenderNode = GetIEditor()->Get3DEngine()->CreateRenderNode(eERType_Brush);
        if (ForceID)
        {
            m_pRenderNode->SetEditorObjectId(ForceID++);
            GetObjectManager()->ForceID(ForceID);
        }
        else
        {
            m_pRenderNode->SetEditorObjectId(GetId().Data1);
        }
        UpdateEngineNode();

        m_statObjValidator.Validate(GetIStatObj(), GetRenderMaterial(), m_pRenderNode->GetPhysics());
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::BeginEditParams(IEditor* ie, int flags)
{
    CBaseObject::BeginEditParams(ie, flags);

    if (!s_brushPanel)
    {
        s_brushPanel = new CBrushPanel;
        s_brushPanelId = AddUIPage(tr("Brush Parameters").toUtf8().data(), s_brushPanel);
    }

    if (gSettings.bGeometryBrowserPanel)
    {
        QString geometryName = mv_geometryFile;
        if (!geometryName.isEmpty())
        {
            if (!s_treePanelPtr)
            {
                auto flags = CPanelTreeBrowser::NO_DRAGDROP | CPanelTreeBrowser::NO_PREVIEW | CPanelTreeBrowser::SELECT_ONCLICK;
                s_treePanelPtr = new CPanelTreeBrowser(functor(*this, &CBrushObject::OnFileChange), GetClassDesc()->GetFileSpec(), flags);
            }
            if (s_treePanelId == 0)
            {
                s_treePanelId = AddUIPage(tr("Prefab").toUtf8().data(), s_treePanelPtr);
            }
        }

        if (s_treePanelPtr)
        {
            s_treePanelPtr->SetSelectCallback(functor(*this, &CBrushObject::OnFileChange));
            s_treePanelPtr->SelectFile(geometryName);
        }
    }

    if (s_brushPanel)
    {
        s_brushPanel->SetBrush(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::EndEditParams(IEditor* ie)
{
    CBaseObject::EndEditParams(ie);

    if (s_treePanelId != 0)
    {
        RemoveUIPage(s_treePanelId);
        s_treePanelId = 0;
    }

    s_treePanelPtr = NULL;

    if (s_brushPanelId != 0)
    {
        RemoveUIPage(s_brushPanelId);
        s_brushPanel = 0;
        s_brushPanelId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CBaseObject::BeginEditMultiSelParams(bAllOfSameType);
    if (bAllOfSameType)
    {
        if (!s_brushPanel)
        {
            s_brushPanel = new CBrushPanel;
            s_brushPanelId = AddUIPage(tr("Brush Parameters").toUtf8().data(), s_brushPanel);
        }
        if (s_brushPanel)
        {
            s_brushPanel->SetBrush(0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::EndEditMultiSelParams()
{
    CBaseObject::EndEditMultiSelParams();

    if (s_brushPanel)
    {
        RemoveUIPage(s_brushPanelId);
        s_brushPanel = 0;
        s_brushPanelId = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::OnFileChange(QString filename)
{
    CUndo undo("Brush Prefab Modify");
    StoreUndo("Brush Prefab Modify");
    mv_geometryFile = filename;

    // Update variables in UI.
    UpdateUIVars();
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::SetSelected(bool bSelect)
{
    CBaseObject::SetSelected(bSelect);

    if (m_pRenderNode)
    {
        if (bSelect && gSettings.viewports.bHighlightSelectedGeometry)
        {
            m_renderFlags |= ERF_SELECTED;
        }
        else
        {
            m_renderFlags &= ~ERF_SELECTED;
        }

        m_pRenderNode->SetRndFlags(m_renderFlags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::GetLocalBounds(AABB& box)
{
    box = m_bbox;
}

//////////////////////////////////////////////////////////////////////////
int CBrushObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown)
    {
        Vec3 pos = view->MapViewToCP(point);
        SetPos(pos);

        if (event == eMouseLDown)
        {
            return MOUSECREATE_OK;
        }
        return MOUSECREATE_CONTINUE;
    }
    return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::Display(DisplayContext& dc)
{
    if (!m_pGeometry)
    {
        static int  nagCount = 0;
        if (nagCount < 100)
        {
            const QString&  geomName = mv_geometryFile;
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Brush '%s' (%s) does not have geometry!", GetName().toUtf8().data(), geomName.toUtf8().data());
            nagCount++;
        }
    }

    if (dc.flags & DISPLAY_2D)
    {
        int flags = 0;

        if (IsSelected())
        {
            dc.SetLineWidth(2);

            flags = 1;
            dc.SetColor(QColor(225, 0, 0));
        }
        else
        {
            flags = 0;
            dc.SetColor(GetColor());
        }

        dc.PushMatrix(GetWorldTM());
        dc.DrawWireBox(m_bbox.min, m_bbox.max);

        dc.PopMatrix();

        if (IsSelected())
        {
            dc.SetLineWidth(0);
        }

        if (m_pGeometry)
        {
            m_pGeometry->Display(dc);
        }

        return;
    }

    if (m_pGeometry)
    {
        SGeometryDebugDrawInfo dd;
        dd.tm = GetWorldTM();
        dd.bExtrude = true;
        dd.color = ColorB(0, 0, 0, 0);
        dd.lineColor = ColorB(0, 0, 0, 0);

        if (!IsHighlighted())
        {
            if (m_statObjValidator.IsValid())
            {
                m_pGeometry->Display(dc);
            }
        }
        else if (gSettings.viewports.bHighlightMouseOverGeometry)
        {
            dd.color = ColorB(250, 0, 250, 30);
            dd.lineColor = ColorB(255, 255, 0, 160);
        }

        if (!m_statObjValidator.IsValid())
        {
            float t = GetTickCount() * 0.001f;
            const float flashPeriodSeconds = 0.8f;
            float alpha = sinf(t * g_PI2 / flashPeriodSeconds) * 0.5f + 0.5f;
            if (dd.color.a == 0)
            {
                dd.color = ColorB(0, 0, 0, 1);
            }
            dd.color.lerpFloat(dd.color, ColorB(255, 0, 0, 255), alpha);
            if (dd.lineColor.a == 0)
            {
                dd.lineColor = ColorB(255, 0, 0, 160);
            }
        }

        if (dd.color.a != 0)
        {
            m_pGeometry->DebugDraw(dd);
        }
    }


    if (IsSelected())
    {
        dc.SetSelectedColor();
        dc.PushMatrix(GetWorldTM());
        dc.DrawWireBox(m_bbox.min, m_bbox.max);
        //dc.SetColor( QColor(255,255,0),0.1f ); // Yellow selected color.
        //dc.DrawSolidBox( m_bbox.min,m_bbox.max );
        dc.PopMatrix();
    }


    if (IsSelected() && m_pRenderNode)
    {
        Vec3 scaleVec = GetScale();
        float scale = 0.5f * (scaleVec.x + scaleVec.y);
        float aiRadius = mv_aiRadius * scale;
        if (aiRadius > 0.0f)
        {
            dc.SetColor(1, 0, 1.0f, 0.7f);
            dc.DrawTerrainCircle(GetPos(), aiRadius, 0.1f);
        }
    }

    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CBrushObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::Serialize(CObjectArchive& ar)
{
    XmlNodeRef xmlNode = ar.node;
    m_bIgnoreNodeUpdate = true;
    CBaseObject::Serialize(ar);
    m_bIgnoreNodeUpdate = false;
    if (ar.bLoading)
    {
        ar.node->getAttr("NotSharedGeometry", m_bNotSharedGeom);
        if (!m_pGeometry)
        {
            QString mesh = mv_geometryFile;
            if (!mesh.isEmpty())
            {
                CreateBrushFromMesh(mesh.toUtf8().data());
            }
        }

        UpdateEngineNode();
    }
    else
    {
        ar.node->setAttr("RndFlags", ERF_GET_WRITABLE(m_renderFlags));
        if (m_bNotSharedGeom)
        {
            ar.node->setAttr("NotSharedGeometry", m_bNotSharedGeom);
        }

        if (!ar.bUndo)
        {
            if (m_pGeometry)
            {
                m_pGeometry->Serialize(ar);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CBrushObject::HitTest(HitContext& hc)
{
    if (CheckFlags(OBJFLAG_SUBOBJ_EDITING))
    {
        if (m_pGeometry)
        {
            bool bHit = m_pGeometry->HitTest(hc);
            if (bHit)
            {
                hc.object = this;
            }
            return bHit;
        }
        return false;
    }

    Vec3 pnt;

    Vec3 raySrc = hc.raySrc;
    Vec3 rayDir = hc.rayDir;
    WorldToLocalRay(raySrc, rayDir);

    if (Intersect::Ray_AABB(raySrc, rayDir, m_bbox, pnt))
    {
        if (hc.b2DViewport)
        {
            // World space distance.
            hc.dist = hc.raySrc.GetDistance(GetWorldTM().TransformPoint(pnt));
            hc.object = this;
            return true;
        }

        IPhysicalEntity* physics = 0;

        if (m_pRenderNode)
        {
            IStatObj* pStatObj = m_pRenderNode->GetEntityStatObj();
            if (pStatObj)
            {
                SRayHitInfo hi;
                hi.inReferencePoint = raySrc;
                hi.inRay = Ray(raySrc, rayDir);
                if (pStatObj->RayIntersection(hi))
                {
                    // World space distance.
                    Vec3 worldHitPos = GetWorldTM().TransformPoint(hi.vHitPos);
                    hc.dist = hc.raySrc.GetDistance(worldHitPos);
                    hc.object = this;
                    return true;
                }
                return false;
            }

            physics = m_pRenderNode->GetPhysics();
            if (physics)
            {
                auto status = pe_status_nparts();
                if (physics->GetStatus(&status) == 0)
                {
                    physics = 0;
                }
            }
        }

        if (physics)
        {
            Vec3r origin = hc.raySrc;
            Vec3r dir = hc.rayDir * 10000.0f;
            ray_hit hit;
            int col = GetIEditor()->GetSystem()->GetIPhysicalWorld()->RayTraceEntity(physics, origin, dir, &hit, 0, geom_collides);
            if (col > 0)
            {
                // World space distance.
                hc.dist = hit.dist;
                hc.object = this;
                return true;
            }
        }

        hc.dist = hc.raySrc.GetDistance(GetWorldTM().TransformPoint(pnt));
        hc.object = this;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
int CBrushObject::HitTestAxis(HitContext& hc)
{
    //@HACK Temporary hack.
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//! Invalidates cached transformation matrix.
void CBrushObject::InvalidateTM(int nWhyFlags)
{
    CBaseObject::InvalidateTM(nWhyFlags);

    if (!(nWhyFlags & eObjectUpdateFlags_RestoreUndo)) // Can skip updating game object when restoring undo.
    {
        if (m_pRenderNode)
        {
            UpdateEngineNode(true);
        }
    }

    m_invertTM = GetWorldTM();
    m_invertTM.Invert();
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::OnEvent(ObjectEvent event)
{
    switch (event)
    {
    case EVENT_FREE_GAME_DATA:
        FreeGameData();
        return;
    case EVENT_PRE_EXPORT:
    {
        if (m_pRenderNode)
        {
            m_pRenderNode->SetCollisionClassIndex(m_collisionFiltering.GetCollisionClassExportId());
        }
    }
        return;
    }
    CBaseObject::OnEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::WorldToLocalRay(Vec3& raySrc, Vec3& rayDir)
{
    raySrc = m_invertTM.TransformPoint(raySrc);
    rayDir = m_invertTM.TransformVector(rayDir).GetNormalized();
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::OnGeometryChange(IVariable* var)
{
    // Load new prefab model.
    QString objName = mv_geometryFile;

    CreateBrushFromMesh(objName.toUtf8().data());
    InvalidateTM(0);

    m_statObjValidator.Validate(GetIStatObj(), GetRenderMaterial(), m_pRenderNode ? m_pRenderNode->GetPhysics() : 0);

    if (NULL != s_treePanelPtr)
    {
        s_treePanelPtr->SelectFile(mv_geometryFile);
    }

    SetModified(false);
}

//////////////////////////////////////////////////////////////////////////
static bool IsBrushFilename(const char* filename)
{
    if ((filename == 0) || (filename[0] == 0))
    {
        return false;
    }

    const char* const ext = CryGetExt(filename);

    if ((_stricmp(ext, CRY_GEOMETRY_FILE_EXT) == 0) ||
        (_stricmp(ext, CRY_ANIM_GEOMETRY_FILE_EXT) == 0))
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::CreateBrushFromMesh(const char* meshFilename)
{
    if (m_pGeometry)
    {
        if (m_pGeometry->IsSameObject(meshFilename))
        {
            return;
        }
        StatObjEventBus::Handler::BusDisconnect(m_pGeometry->GetIStatObj());

        m_pGeometry->RemoveUser();
    }

    GetIEditor()->GetErrorReport()->SetCurrentFile(meshFilename);

    if (meshFilename && meshFilename[0] && (!IsBrushFilename(meshFilename)))
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "File %s cannot be used for brush geometry (unsupported file extension)", meshFilename);
        m_pGeometry = 0;
    }
    else
    {
        m_pGeometry = CEdMesh::LoadMesh(meshFilename);
    }

    if (m_pGeometry)
    {
        m_pGeometry->AddUser();

        if (m_pGeometry->GetIStatObj())
        {
            StatObjEventBus::Handler::BusConnect(m_pGeometry->GetIStatObj());
        }
    }

    GetIEditor()->GetErrorReport()->SetCurrentFile("");

    if (m_pGeometry)
    {
        UpdateEngineNode();
    }
    else if (m_pRenderNode)
    {
        // Remove this object from engine node.
        m_pRenderNode->SetEntityStatObj(0, 0, 0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::ReloadGeometry()
{
    if (m_pGeometry)
    {
        m_pGeometry->ReloadGeometry();
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::OnAIRadiusVarChange(IVariable* var)
{
    if (m_bIgnoreNodeUpdate)
    {
        return;
    }

    if (m_pRenderNode)
    {
        IStatObj* pStatObj = m_pRenderNode->GetEntityStatObj();
        if (pStatObj)
        {
            // update all other objects that share pStatObj
            CBaseObjectsArray objects;
            GetIEditor()->GetObjectManager()->GetObjects(objects);
            for (unsigned i = 0; i < objects.size(); ++i)
            {
                CBaseObject* pBase = objects[i];
                if (pBase->GetType() == OBJTYPE_BRUSH)
                {
                    CBrushObject* obj = (CBrushObject*)pBase;
                    if (obj->m_pRenderNode && obj->m_pRenderNode->GetEntityStatObj() == pStatObj)
                    {
                        obj->mv_aiRadius = mv_aiRadius;
                    }
                }
            }
        }
    }
    UpdateEngineNode();
    UpdatePrefab();
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::OnRenderVarChange(IVariable* var)
{
    UpdateEngineNode();
    UpdatePrefab();

    // cached shadows if dynamic distance shadow flag has changed
    if (var == &mv_dynamicDistanceShadows)
    {
        gEnv->p3DEngine->SetRecomputeCachedShadows();
    }
}

//////////////////////////////////////////////////////////////////////////
IPhysicalEntity* CBrushObject::GetCollisionEntity() const
{
    // Returns physical object of entity.
    if (m_pRenderNode)
    {
        return m_pRenderNode->GetPhysics();
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CBrushObject::ConvertFromObject(CBaseObject* object)
{
    CBaseObject::ConvertFromObject(object);
    if (qobject_cast<CEntityObject*>(object))
    {
        CEntityObject* entity = (CEntityObject*)object;
        IEntity* pIEntity = entity->GetIEntity();
        if (!pIEntity)
        {
            return false;
        }

        IStatObj* pStatObj = pIEntity->GetStatObj(0);
        if (!pStatObj)
        {
            return false;
        }

        // Copy entity shadow parameters.
        //mv_selfShadowing = entity->IsSelfShadowing();
        mv_castShadowMaps = (entity->GetCastShadowMinSpec() < END_CONFIG_SPEC_ENUM) ? true : false;
        mv_rainOccluder = true;
        mv_registerByBBox = false;
        //mv_castLightmap = entity->IsCastLightmap();
        //mv_recvLightmap = entity->IsRecvLightmap();
        mv_ratioLOD = entity->GetRatioLod();
        mv_viewDistanceMultiplier = entity->GetViewDistanceMultiplier();
        mv_noIndirLight = false;
        mv_recvWind = false;
        mv_integrQuality = 0;

        mv_geometryFile = pStatObj->GetFilePath();
        return true;
    }
    /*
    if (object->IsKindOf(RUNTIME_CLASS(CStaticObject)))
    {
        CStaticObject *pStatObject = (CStaticObject*)object;
        mv_hideable = pStatObject->IsHideable();
    }
    */
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::OnStatObjReloaded()
{
    // Minimal set of update to ensure correct object presentation (bounding box, culling)
    InvalidateTM(0);
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::UpdateEngineNode(bool bOnlyTransform)
{
    if (m_bIgnoreNodeUpdate)
    {
        return;
    }

    if (!m_pRenderNode)
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    // Set brush render flags.
    //////////////////////////////////////////////////////////////////////////
    m_renderFlags = 0;
    if (mv_outdoor)
    {
        m_renderFlags |= ERF_OUTDOORONLY;
    }
    //  if (mv_castShadows)
    //  m_renderFlags |= ERF_CASTSHADOWVOLUME;
    //if (mv_selfShadowing)
    //m_renderFlags |= ERF_SELFSHADOW;
    if (mv_castShadowMaps)
    {
        m_renderFlags |= ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS;
    }
    if (mv_rainOccluder)
    {
        m_renderFlags |= ERF_RAIN_OCCLUDER;
    }
    if (mv_registerByBBox)
    {
        m_renderFlags |= ERF_REGISTER_BY_BBOX;
    }
    if (mv_dynamicDistanceShadows)
    {
        m_renderFlags |= ERF_DYNAMIC_DISTANCESHADOWS;
    }
    //  if (mv_recvLightmap)
    //  m_renderFlags |= ERF_USERAMMAPS;
    if (IsHidden() || IsHiddenBySpec())
    {
        m_renderFlags |= ERF_HIDDEN;
    }
    if (mv_hideable == 1)
    {
        m_renderFlags |= ERF_HIDABLE;
    }
    if (mv_hideable == 2)
    {
        m_renderFlags |= ERF_HIDABLE_SECONDARY;
    }
    //  if (mv_hideableSecondary)
    //      m_renderFlags |= ERF_HIDABLE_SECONDARY;
    if (mv_excludeFromTriangulation)
    {
        m_renderFlags |= ERF_EXCLUDE_FROM_TRIANGULATION;
    }
    if (mv_noDynWater)
    {
        m_renderFlags |= ERF_NODYNWATER;
    }
    if (mv_noDecals)
    {
        m_renderFlags |= ERF_NO_DECALNODE_DECALS;
    }
    if (mv_recvWind)
    {
        m_renderFlags |= ERF_RECVWIND;
    }
    if (mv_Occluder)
    {
        m_renderFlags |= ERF_GOOD_OCCLUDER;
    }
    ((IBrush*)m_pRenderNode)->SetDrawLast(mv_drawLast);
    if (m_pRenderNode->GetRndFlags() & ERF_COLLISION_PROXY)
    {
        m_renderFlags |= ERF_COLLISION_PROXY;
    }

    if (m_pRenderNode->GetRndFlags() & ERF_RAYCAST_PROXY)
    {
        m_renderFlags |= ERF_RAYCAST_PROXY;
    }

    if (IsSelected() && gSettings.viewports.bHighlightSelectedGeometry)
    {
        m_renderFlags |= ERF_SELECTED;
    }

    int flags = GetRenderFlags();

    m_pRenderNode->SetRndFlags(m_renderFlags);

    m_pRenderNode->SetMinSpec(GetMinSpec());
    m_pRenderNode->SetViewDistanceMultiplier(mv_viewDistanceMultiplier);
    m_pRenderNode->SetLodRatio(mv_ratioLOD);
    m_pRenderNode->SetShadowLodBias(mv_shadowLodBias);

    m_renderFlags = m_pRenderNode->GetRndFlags();

    m_pRenderNode->SetMaterialLayers(GetMaterialLayersMask());

    if (m_pGeometry)
    {
        m_pGeometry->GetBounds(m_bbox);

        Matrix34A tm = GetWorldTM();
        m_pRenderNode->SetEntityStatObj(0, m_pGeometry->GetIStatObj(), &tm);
    }

    IStatObj* pStatObj = m_pRenderNode->GetEntityStatObj();
    if (pStatObj)
    {
        float r = mv_aiRadius;
        pStatObj->SetAIVegetationRadius(r);
    }

    // Fast exit if only transformation needs to be changed.
    if (bOnlyTransform)
    {
        return;
    }


    if (GetMaterial())
    {
        GetMaterial()->AssignToEntity(m_pRenderNode);
    }
    else
    {
        // Reset all material settings for this node.
        m_pRenderNode->SetMaterial(0);
    }

    m_pRenderNode->SetLayerId(0);

    // Apply collision class filtering
    m_collisionFiltering.ApplyToPhysicalEntity(m_pRenderNode ? m_pRenderNode->GetPhysics() : 0);

    // Setting the objects layer modified, this is to ensure that if this object is embedded in a prefab that it is properly saved.
    this->SetLayerModified();

    return;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
IStatObj* CBrushObject::GetIStatObj()
{
    if (m_pRenderNode)
    {
        IStatObj* pStatObj = m_pRenderNode->GetEntityStatObj();
        if (pStatObj)
        {
            if (pStatObj->GetIndexedMesh(true))
            {
                return pStatObj;
            }
        }
    }
    if (m_pGeometry == NULL)
    {
        return NULL;
    }
    return m_pGeometry->GetIStatObj();
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::SetMinSpec(uint32 nSpec, bool bSetChildren)
{
    CBaseObject::SetMinSpec(nSpec, bSetChildren);
    if (m_pRenderNode)
    {
        m_pRenderNode->SetMinSpec(GetMinSpec());
        m_renderFlags = m_pRenderNode->GetRndFlags();
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::UpdateVisibility(bool visible)
{
    if (visible == CheckFlags(OBJFLAG_INVISIBLE) ||
        m_pRenderNode && (bool(m_renderFlags & ERF_HIDDEN) == (visible && !IsHiddenBySpec())) // force update if spec changed
        )
    {
        CBaseObject::UpdateVisibility(visible);
        if (m_pRenderNode)
        {
            if (!visible || IsHiddenBySpec())
            {
                m_renderFlags |= ERF_HIDDEN;
            }
            else
            {
                m_renderFlags &= ~ERF_HIDDEN;
            }

            m_pRenderNode->SetRndFlags(m_renderFlags);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::SetMaterial(CMaterial* mtl)
{
    CBaseObject::SetMaterial(mtl);
    if (m_pRenderNode)
    {
        UpdateEngineNode();
    }
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CBrushObject::GetRenderMaterial() const
{
    if (GetMaterial())
    {
        return GetMaterial();
    }
    if (m_pGeometry && m_pGeometry->GetIStatObj())
    {
        return GetIEditor()->GetMaterialManager()->FromIMaterial(m_pGeometry->GetIStatObj()->GetMaterial());
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::SetMaterialLayersMask(uint32 nLayersMask)
{
    CBaseObject::SetMaterialLayersMask(nLayersMask);
    UpdateEngineNode(false);
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::Validate(IErrorReport* report)
{
    CBaseObject::Validate(report);

    if (!m_pGeometry)
    {
        QString file = mv_geometryFile;
        CErrorRecord err;
        err.error = QStringLiteral("No Geometry %1 for Brush %2").arg(file, GetName());
        err.file = file;
        err.pObject = this;
        report->ReportError(err);
    }
    else if (m_pGeometry->IsDefaultObject())
    {
        QString file = mv_geometryFile;
        CErrorRecord err;
        err.error = QStringLiteral("Geometry file %1 for Brush %2 Failed to Load").arg(file, GetName());
        err.file = file;
        err.pObject = this;
        report->ReportError(err);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::GatherUsedResources(CUsedResources& resources)
{
    QString geomFile = mv_geometryFile;
    if (!geomFile.isEmpty())
    {
        resources.Add(geomFile.toUtf8().data());
    }
    if (m_pGeometry && m_pGeometry->GetIStatObj())
    {
        CMaterialManager::GatherResources(m_pGeometry->GetIStatObj()->GetMaterial(), resources);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CBrushObject::IsSimilarObject(CBaseObject* pObject)
{
    if (pObject->GetClassDesc() == GetClassDesc() && metaObject() == pObject->metaObject())
    {
        CBrushObject* pBrush = (CBrushObject*)pObject;
        if (mv_geometryFile == pBrush->mv_geometryFile)
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBrushObject::StartSubObjSelection(int elemType)
{
    bool bStarted = false;
    if (m_pGeometry)
    {
        if (m_pGeometry->GetUserCount() > 1)
        {
            const QString str = tr("Geometry File %1 is used by multiple objects.\r\nDo you want to make an unique copy of this geometry?").arg(m_pGeometry->GetFilename());
            int res = QMessageBox::question(QApplication::activeWindow(), tr("Warning"), str, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (res == QMessageBox::Cancel)
            {
                return false;
            }
            if (res == QMessageBox::Yes)
            {
                // Must make a copy of the geometry.
                m_pGeometry = (CEdMesh*)m_pGeometry->Clone();
                QString levelPath = Path::AddPathSlash(GetIEditor()->GetLevelFolder());
                QString filename = levelPath + "Objects" + QDir::separator() + GetName() + "." + CRY_GEOMETRY_FILE_EXT;
                filename = Path::MakeGamePath(filename);
                m_pGeometry->SetFilename(filename);
                mv_geometryFile = filename;

                UpdateEngineNode();
                m_bNotSharedGeom = true;
            }
        }
        bStarted = m_pGeometry->StartSubObjSelection(GetWorldTM(), elemType, 0);
    }
    if (bStarted)
    {
        SetFlags(OBJFLAG_SUBOBJ_EDITING);
    }
    return bStarted;
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::EndSubObjectSelection()
{
    ClearFlags(OBJFLAG_SUBOBJ_EDITING);
    if (m_pGeometry)
    {
        m_pGeometry->EndSubObjSelection();
        UpdateEngineNode(true);
        if (m_pRenderNode)
        {
            m_pRenderNode->Physicalize();
        }
        m_pGeometry->GetBounds(m_bbox);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::CalculateSubObjectSelectionReferenceFrame(ISubObjectSelectionReferenceFrameCalculator* pCalculator)
{
    if (m_pGeometry)
    {
        Matrix34 refFrame;
        bool bAnySelected = m_pGeometry->GetSelectionReferenceFrame(refFrame);
        refFrame = this->GetWorldTM() * refFrame;
        pCalculator->SetExplicitFrame(bAnySelected, refFrame);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::ModifySubObjSelection(SSubObjSelectionModifyContext& modCtx)
{
    if (m_pGeometry)
    {
        m_pGeometry->ModifySelection(modCtx);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::AcceptSubObjectModify()
{
    if (m_pGeometry)
    {
        m_pGeometry->AcceptModifySelection();
    }
}

//////////////////////////////////////////////////////////////////////////
CEdGeometry* CBrushObject::GetGeometry()
{
    // Return our geometry.
    return m_pGeometry;
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::SaveToCGF(const QString& filename)
{
    if (m_pGeometry)
    {
        if (GetMaterial())
        {
            m_pGeometry->SaveToCGF(filename.toUtf8().data(), NULL, GetMaterial()->GetMatInfo());
        }
        else
        {
            m_pGeometry->SaveToCGF(filename.toUtf8().data());
        }
    }
    mv_geometryFile = Path::MakeGamePath(filename);
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::GetVerticesInWorld(std::vector<Vec3>& vertices) const
{
    vertices.clear();
    if (m_pGeometry == NULL)
    {
        return;
    }
    IIndexedMesh* pIndexedMesh = m_pGeometry->GetIndexedMesh();
    if (pIndexedMesh == NULL)
    {
        return;
    }

    IIndexedMesh::SMeshDescription meshDesc;
    pIndexedMesh->GetMeshDescription(meshDesc);

    vertices.reserve(meshDesc.m_nVertCount);

    const Matrix34 tm = GetWorldTM();

    if (meshDesc.m_pVerts)
    {
        for (int v = 0; v < meshDesc.m_nVertCount; ++v)
        {
            vertices.push_back(tm.TransformPoint(meshDesc.m_pVerts[v]));
        }
    }
    else
    {
        for (int v = 0; v < meshDesc.m_nVertCount; ++v)
        {
            vertices.push_back(tm.TransformPoint(meshDesc.m_pVertsF16[v].ToVec3()));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::EditTags(bool alwaysTag)
{
    bool toTag = true;

    if (alwaysTag == false)
    {
        CAssetBrowserManager::StrVector tags;
        QString asset(GetGeometryFile());

        int numTags = CAssetBrowserManager::Instance()->GetTagsForAsset(tags, asset);

        if (numTags != 0)
        {
            toTag = false;
        }
    }

    if (toTag == false)
    {
        return;
    }

    CAssetBrowserMetaTaggingDlg taggingDialog(GetGeometryFile());
    taggingDialog.exec();
}

//////////////////////////////////////////////////////////////////////////
void CBrushObject::OnMaterialChanged(MaterialChangeFlags change)
{
    if (change & MATERIALCHANGE_SURFACETYPE)
    {
        m_statObjValidator.Validate(GetIStatObj(), GetRenderMaterial(), m_pRenderNode ? m_pRenderNode->GetPhysics() : 0);
    }
}

//////////////////////////////////////////////////////////////////////////
QString CBrushObject::GetTooltip() const
{
    return m_statObjValidator.GetDescription();
}

//////////////////////////////////////////////////////////////////////////
QString CBrushObject::GetMouseOverStatisticsText() const
{
    QString triangleCountText;
    QString lodText;

    if (!m_pGeometry)
    {
        return triangleCountText;
    }

    IStatObj* pStatObj = m_pGeometry->GetIStatObj();

    if (pStatObj)
    {
        IStatObj::SStatistics stats;
        pStatObj->GetStatistics(stats);

        for (int i = 0; i < MAX_STATOBJ_LODS_NUM; ++i)
        {
            if (stats.nIndicesPerLod[i] > 0)
            {
                lodText = QStringLiteral("\n  LOD%1:   ").arg(i);
                triangleCountText = triangleCountText + lodText +
                    FormatWithThousandsSeperator(stats.nIndicesPerLod[i] / 3) + " Tris   "
                    + FormatWithThousandsSeperator(stats.nVerticesPerLod[i]) + " Verts";
            }
        }
    }

    return triangleCountText;
}

#include <Objects/BrushObject.moc>