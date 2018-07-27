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

// Description : StaticObject implementation.


#include "StdAfx.h"
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

#include "EntityObject.h"
#include "EntityPanel.h"
#include "EntityLinksPanel.h"
#include "PanelTreeBrowser.h"
#include "Viewport.h"
#include "LineGizmo.h"
#include "Group.h"

#include <I3DEngine.h>
#include <IAgent.h>
#include <IMovieSystem.h>
#include <IEntitySystem.h>
#include <ICryAnimation.h>
#include <IVertexAnimation.h>
#include <IEntityRenderState.h>

#include "IEntityPoolManager.h"
#include "IIndexedMesh.h"
#include "IBreakableManager.h"

#include "EntityPrototype.h"
#include "Material/MaterialManager.h"

#include "StringDlg.h"
#include "GenericSelectItemDialog.h"

#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraph.h"
#include "HyperGraph/HyperGraphDialog.h"
#include "HyperGraph/FlowGraphSearchCtrl.h"
#include "HyperGraph/FlowGraphHelpers.h"

#include "BrushObject.h"
#include "GameEngine.h"

#include "IAIObject.h"
#include "IAIActor.h"

#include "Viewport.h"
#include "ViewManager.h"
#include "Include/IIconManager.h"
#include "QtViewPaneManager.h"

#include <Serialization/Serializer.h>
#include <Serialization/IArchive.h>
#include <Serialization/STL.h>
#include <Serialization/DynArray.h>
#include <Serialization/IArchiveHost.h>

#include "../Controls/RollupBar.h"
#include "Util/BoostPythonHelpers.h"
#include "Objects/ObjectLayer.h"

#include "IFlares.h"
#include "LensFlareEditor/LensFlareManager.h"
#include "LensFlareEditor/LensFlareUtil.h"
#include "LensFlareEditor/LensFlareItem.h"
#include "LensFlareEditor/LensFlareLibrary.h"

#include "AI/AIManager.h"

#include "Prefabs/PrefabManager.h"
#include "Prefabs/PrefabLibrary.h"
#include "Prefabs/PrefabItem.h"
#include "AnimationContext.h"
#include "TrackView/TrackViewSequenceManager.h"
#include "TrackView/TrackViewAnimNode.h"

#include "UndoEntityProperty.h"
#include "UndoEntityParam.h"

#include "IGeomCache.h"

#include <QWidget> // To use the view pane
#include <QCoreApplication> // To message the view pane
#include <IEntityHelper.h>
#include "Components/IComponentFlowGraph.h"
#include "Components/IComponentRender.h"
#include "Components/IComponentEntityAttributes.h"
#include <boost/make_shared.hpp>

#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>

const char* CEntityObject::s_LensFlarePropertyName("flare_Flare");
const char* CEntityObject::s_LensFlareMaterialName("EngineAssets/Materials/lens_optics");


//////////////////////////////////////////////////////////////////////////
//! Undo Entity Link
class CUndoEntityLink
    : public IUndoObject
{
public:
    CUndoEntityLink(CSelectionGroup* pSelection)
    {
        int nObjectSize = pSelection->GetCount();
        m_Links.reserve(nObjectSize);
        for (int i = 0; i < nObjectSize; ++i)
        {
            CBaseObject* pObj = pSelection->GetObject(i);
            if (qobject_cast<CEntityObject*>(pObj))
            {
                SLink link;
                link.entityID = pObj->GetId();
                link.linkXmlNode = XmlHelpers::CreateXmlNode("undo");
                ((CEntityObject*)pObj)->SaveLink(link.linkXmlNode);
                m_Links.push_back(link);
            }
        }
    }

protected:
    virtual void Release() { delete this; };
    virtual int GetSize() { return sizeof(*this); }; // Return size of xml state.
    virtual QString GetDescription() { return "Entity Link"; };
    virtual QString GetObjectName(){ return ""; };

    virtual void Undo(bool bUndo)
    {
        for (int i = 0, iLinkSize(m_Links.size()); i < iLinkSize; ++i)
        {
            SLink& link = m_Links[i];
            CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(link.entityID);
            if (pObj == NULL)
            {
                continue;
            }
            if (!qobject_cast<CEntityObject*>(pObj))
            {
                continue;
            }
            CEntityObject* pEntity = (CEntityObject*)pObj;
            if (link.linkXmlNode->getChildCount() == 0)
            {
                continue;
            }
            pEntity->LoadLink(link.linkXmlNode->getChild(0));
        }
    }
    virtual void Redo(){}

private:

    struct SLink
    {
        GUID entityID;
        XmlNodeRef linkXmlNode;
    };

    std::vector<SLink> m_Links;
};

//////////////////////////////////////////////////////////////////////////
//! Undo object for attach/detach changes
class CUndoAttachEntity
    : public IUndoObject
{
public:
    CUndoAttachEntity(CEntityObject* pAttachedObject, bool bAttach)
        : m_attachedEntityGUID(pAttachedObject->GetId())
        , m_attachmentTarget(pAttachedObject->GetAttachTarget())
        , m_attachmentType(pAttachedObject->GetAttachType())
        , m_bAttach(bAttach)
    {}

    virtual void Undo(bool bUndo) override
    {
        if (!m_bAttach)
        {
            SetAttachmentTypeAndTarget();
        }
    }

    virtual void Redo() override
    {
        if (m_bAttach)
        {
            SetAttachmentTypeAndTarget();
        }
    }

private:
    void SetAttachmentTypeAndTarget()
    {
        CObjectManager* pObjectManager = static_cast<CObjectManager*>(GetIEditor()->GetObjectManager());
        CEntityObject* pEntity = static_cast<CEntityObject*>(pObjectManager->FindObject(m_attachedEntityGUID));

        if (pEntity)
        {
            pEntity->SetAttachType(m_attachmentType);
            pEntity->SetAttachTarget(m_attachmentTarget.toUtf8().data());
        }
    }

    virtual int GetSize() { return sizeof(CUndoAttachEntity); }
    virtual QString GetDescription() { return "Attachment Changed"; }

    GUID m_attachedEntityGUID;
    CEntityObject::EAttachmentType m_attachmentType;
    QString m_attachmentTarget;
    bool m_bAttach;
};

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

int CEntityObject::m_rollupId = 0;
CEntityPanel* CEntityObject::m_panel = 0;
float CEntityObject::m_helperScale = 1;
CPanelTreeBrowser* CEntityObject::ms_pTreePanel = nullptr;
int CEntityObject::ms_treePanelId = 0;

namespace
{
    int s_entityEventsPanelIndex = 0;
    CEntityEventsPanel* s_entityEventsPanel = 0;

    int s_entityLinksPanelIndex = 0;
    CEntityLinksPanel* s_entityLinksPanel = 0;

    int s_propertiesPanelIndex = 0;
    ReflectedPropertiesPanel* s_pPropertiesPanel = nullptr;

    int s_propertiesPanelIndex2 = 0;
    ReflectedPropertiesPanel* s_pPropertiesPanel2 = nullptr;

    CEntityObject* s_pPropertyPanelEntityObject = nullptr;

    // Prevent OnPropertyChange to be executed when loading many properties at one time.
    static bool s_ignorePropertiesUpdate = false;

    std::map<EntityId, CEntityObject*> s_entityIdMap;
};

void CEntityObject::DeleteUIPanels()
{
    delete s_pPropertiesPanel;
    delete s_pPropertiesPanel2;
    delete ms_pTreePanel;
}

//////////////////////////////////////////////////////////////////////////
CEntityObject::CEntityObject()
    : m_listeners(1)
{
    m_pEntity = 0;
    m_bLoadFailed = false;

    m_pEntityScript = 0;

    m_visualObject = 0;

    m_box.min.Set(0, 0, 0);
    m_box.max.Set(0, 0, 0);

    m_proximityRadius = 0;
    m_innerRadius = 0;
    m_outerRadius = 0;
    m_boxSizeX = 1;
    m_boxSizeY = 1;
    m_boxSizeZ = 1;
    m_bProjectInAllDirs = false;
    m_bProjectorHasTexture = false;

    m_bDisplayBBox = true;
    m_bBBoxSelection = false;
    m_bDisplaySolidBBox = false;
    m_bDisplayAbsoluteRadius = false;
    m_bIconOnTop = false;
    m_bDisplayArrow = false;
    m_entityId = 0;
    m_bVisible = true;
    m_bCalcPhysics = true;
    m_pFlowGraph = 0;
    m_bLight = false;
    m_bAreaLight = false;
    m_fAreaWidth = 1;
    m_fAreaHeight = 1;
    m_fAreaLightSize = 0.05f;
    m_bBoxProjectedCM = false;
    m_fBoxWidth = 1;
    m_fBoxHeight = 1;
    m_fBoxLength = 1;

    m_bEnableReload = true;

    m_lightColor = Vec3(1, 1, 1);

    m_bEntityXfromValid = false;

    SetColor(QColor(255, 255, 0));

    // Init Variables.
    mv_castShadow = true;
    mv_castShadowMinSpec = CONFIG_LOW_SPEC;
    mv_outdoor          =   false;
    mv_recvWind = false;
    mv_renderNearest = false;
    mv_noDecals = false;

    mv_createdThroughPool = false;

    mv_obstructionMultiplier = 1.f;
    mv_obstructionMultiplier.SetLimits(0.f, 1.f, 0.01f);

    mv_hiddenInGame = false;
    mv_ratioLOD = 100;
    mv_viewDistanceMultiplier = 1.0f;
    mv_ratioLOD.SetLimits(0, 255);
    mv_viewDistanceMultiplier.SetLimits(0.0f, IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX);

    m_physicsState = 0;

    m_bForceScale = false;

    m_attachmentType = eAT_Pivot;
}

CEntityObject::~CEntityObject()
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::InitVariables()
{
    mv_castShadowMinSpec.AddEnumItem("Never",          END_CONFIG_SPEC_ENUM);
    mv_castShadowMinSpec.AddEnumItem("Low",                CONFIG_LOW_SPEC);
    mv_castShadowMinSpec.AddEnumItem("Medium",     CONFIG_MEDIUM_SPEC);
    mv_castShadowMinSpec.AddEnumItem("High",               CONFIG_HIGH_SPEC);
    mv_castShadowMinSpec.AddEnumItem("VeryHigh",       CONFIG_VERYHIGH_SPEC);

    mv_castShadow.SetFlags(mv_castShadow.GetFlags() | IVariable::UI_INVISIBLE);
    mv_castShadowMinSpec->SetFlags(mv_castShadowMinSpec->GetFlags() | IVariable::UI_UNSORTED);

    AddVariable(mv_outdoor, "OutdoorOnly", tr("Outdoor Only"), functor(*this, &CEntityObject::OnEntityFlagsChange));
    AddVariable(mv_castShadow, "CastShadow", tr("Cast Shadow"), functor(*this, &CEntityObject::OnEntityFlagsChange));
    AddVariable(mv_castShadowMinSpec, "CastShadowMinspec", tr("Cast Shadow MinSpec"), functor(*this, &CEntityObject::OnEntityFlagsChange));

    AddVariable(mv_ratioLOD, "LodRatio", functor(*this, &CEntityObject::OnRenderFlagsChange));
    AddVariable(mv_viewDistanceMultiplier, "ViewDistanceMultiplier", functor(*this, &CEntityObject::OnRenderFlagsChange));
    AddVariable(mv_hiddenInGame, "HiddenInGame");
    AddVariable(mv_recvWind, "RecvWind", tr("Receive Wind"), functor(*this, &CEntityObject::OnEntityFlagsChange));

    // [artemk]: Add RenderNearest entity param because of animator request.
    // This will cause that slot zero is rendered with ENTITY_SLOT_RENDER_NEAREST flag raised.
    // Used mostly in TrackView editor.
    AddVariable(mv_renderNearest, "RenderNearest", functor(*this, &CEntityObject::OnRenderFlagsChange));
    mv_renderNearest.SetDescription("Used to eliminate z-buffer artifacts when rendering from first person view");
    AddVariable(mv_noDecals, "NoStaticDecals", functor(*this, &CEntityObject::OnRenderFlagsChange));

    AddVariable(mv_createdThroughPool, "CreatedThroughPool", tr("Created Through Pool"));

    AddVariable(mv_obstructionMultiplier, "ObstructionMultiplier", tr("Obstruction Multiplier"), functor(*this, &CEntityObject::OnEntityObstructionMultiplierChange));
}

//////////////////////////////////////////////////////////////////////////&
void CEntityObject::Done()
{
    if (m_pFlowGraph)
    {
        CFlowGraphManager* pFGMGR = GetIEditor()->GetFlowGraphManager();
        if (pFGMGR)
        {
            pFGMGR->UnregisterAndResetView(m_pFlowGraph);
        }
    }
    SAFE_RELEASE(m_pFlowGraph);

    DeleteEntity();
    UnloadScript();

    ReleaseEventTargets();
    RemoveAllEntityLinks();

    for (CListenerSet<IEntityObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnDone();
    }

    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::FreeGameData()
{
    DeleteEntity();
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::Init(IEditor* pEditor, CBaseObject* pPrev, const QString& file)
{
    CBaseObject::Init(pEditor, pPrev, file);

    if (pPrev)
    {
        CEntityObject* pPreviousEntity = ( CEntityObject* )pPrev;

        // Clone Properties.
        if (pPreviousEntity->m_pProperties)
        {
            m_pProperties = CloneProperties(pPreviousEntity->m_pProperties);
        }
        if (pPreviousEntity->m_pProperties2)
        {
            m_pProperties2 = CloneProperties(pPreviousEntity->m_pProperties2);
        }

        // When cloning entity, do not get properties from script.
        SetClass(pPreviousEntity->GetEntityClass(), false, false);
        SpawnEntity();

        if (pPreviousEntity->m_pFlowGraph)
        {
            SetFlowGraph(( CFlowGraph* )pPreviousEntity->m_pFlowGraph->Clone());
        }

        mv_createdThroughPool = pPreviousEntity->mv_createdThroughPool;

        UpdatePropertyPanels(true);
    }
    else if (!file.isEmpty())
    {
        SetUniqueName(file);
        m_entityClass = file;

        IEntityPoolManager* pPoolManager = gEnv->pEntitySystem ? gEnv->pEntitySystem->GetIEntityPoolManager() : nullptr;
        if (pPoolManager && pPoolManager->IsClassDefaultBookmarked(m_entityClass.toUtf8().data()))
        {
            mv_createdThroughPool = true;
        }
    }

    ResetCallbacks();

    return true;
}

//////////////////////////////////////////////////////////////////////////
/*static*/ CEntityObject* CEntityObject::FindFromEntityId(EntityId id)
{
    CEntityObject* pEntity = stl::find_in_map(s_entityIdMap, id, 0);
    return pEntity;
}

//////////////////////////////////////////////////////////////////////////
/*static*/ CEntityObject* CEntityObject::FindFromEntityId(const AZ::EntityId& id)
{
    CEntityObject* retEntity = nullptr;
    if (IsLegacyEntityId(id))
    {
        retEntity = FindFromEntityId(GetLegacyEntityId(id));
    }
    else
    {
        AzToolsFramework::ComponentEntityEditorRequestBus::EventResult(retEntity, id, &AzToolsFramework::ComponentEntityEditorRequestBus::Events::GetSandboxObject);
    }
    return retEntity;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetTransformDelegate(ITransformDelegate* pTransformDelegate)
{
    CBaseObject::SetTransformDelegate(pTransformDelegate);

    if (this == s_pPropertyPanelEntityObject)
    {
        return;
    }

    UpdatePropertyPanels(true);
    s_ignorePropertiesUpdate = true;
    ForceVariableUpdate();
    s_ignorePropertiesUpdate = false;
    SetPropertyPanelsState();
    ResetCallbacks();
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IsSameClass(CBaseObject* obj)
{
    if (GetClassDesc() == obj->GetClassDesc())
    {
        CEntityObject* ent = ( CEntityObject* )obj;
        return GetScript() == ent->GetScript();
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::ConvertFromObject(CBaseObject* object)
{
    CBaseObject::ConvertFromObject(object);

    if (qobject_cast<CEntityObject*>(object))
    {
        CEntityObject* pObject = ( CEntityObject* )object;

        mv_outdoor = pObject->mv_outdoor;
        mv_castShadowMinSpec = pObject->mv_castShadowMinSpec;
        mv_ratioLOD = pObject->mv_ratioLOD;
        mv_viewDistanceMultiplier = pObject->mv_viewDistanceMultiplier;
        mv_hiddenInGame = pObject->mv_hiddenInGame;
        mv_recvWind = pObject->mv_recvWind;
        mv_renderNearest = pObject->mv_renderNearest;
        mv_noDecals = pObject->mv_noDecals;

        mv_createdThroughPool = pObject->mv_createdThroughPool;

        mv_obstructionMultiplier = pObject->mv_obstructionMultiplier;
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetLookAt(CBaseObject* target)
{
    CBaseObject::SetLookAt(target);
}

//////////////////////////////////////////////////////////////////////////
IPhysicalEntity* CEntityObject::GetCollisionEntity() const
{
    // Returns physical object of entity.
    if (m_pEntity)
    {
        return m_pEntity->GetPhysics();
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::GetLocalBounds(AABB& box)
{
    box = m_box;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetModified(bool boModifiedTransformOnly)
{
    CBaseObject::SetModified(boModifiedTransformOnly);

    if (m_pEntity)
    {
        if (IAIObject* ai = m_pEntity->GetAI())
        {
            if (ai->GetAIType() == AIOBJECT_ACTOR)
            {
                GetIEditor()->GetAI()->CalculateNavigationAccessibility();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::HitTestEntity(HitContext& hc, bool& bHavePhysics)
{
    bHavePhysics = true;
    IPhysicalWorld* pPhysWorld = GetIEditor()->GetSystem()->GetIPhysicalWorld();
    // Test 3D viewport.
    IPhysicalEntity* physic = 0;

    ICharacterInstance* pCharacter = m_pEntity->GetCharacter(0);
    if (pCharacter)
    {
        physic = pCharacter->GetISkeletonPose()->GetCharacterPhysics();
        auto pe_status = pe_status_nparts();
        if (physic)
        {
            int type = physic->GetType();
            if (type == PE_NONE || type == PE_PARTICLE || type == PE_ROPE || type == PE_SOFT)
            {
                physic = 0;
            }
            else if (physic->GetStatus(&pe_status) == 0)
            {
                physic = 0;
            }
        }
        if (physic)
        {
            ray_hit hit;
            int col = pPhysWorld->RayTraceEntity(physic, hc.raySrc, hc.rayDir * 10000.0f, &hit);
            if (col <= 0)
            {
                return false;
            }
            hc.dist = hit.dist;
            hc.object = this;
            return true;
        }
    }

    physic = m_pEntity->GetPhysics();
    if (physic)
    {
        auto pe_status = pe_status_nparts();
        int type = physic->GetType();
        if (type == PE_NONE || type == PE_PARTICLE || type == PE_ROPE || type == PE_SOFT)
        {
            physic = 0;
        }
        else if (physic->GetStatus(&pe_status) == 0)
        {
            physic = 0;
        }
    }
    // Now if box intersected try real geometry ray test.
    if (physic)
    {
        ray_hit hit;
        int col = pPhysWorld->RayTraceEntity(physic, hc.raySrc, hc.rayDir * 10000.0f, &hit);
        if (col <= 0)
        {
            return false;
        }
        hc.dist = hit.dist;
        hc.object = this;
        return true;
    }
    else
    {
        bHavePhysics = false;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::HitTest(HitContext& hc)
{
    if (!hc.b2DViewport)
    {
        // Test 3D viewport.
        if (m_pEntity)
        {
            bool bHavePhysics = false;
            if (HitTestEntity(hc, bHavePhysics))
            {
                hc.object = this;
                return true;
            }
            if (bHavePhysics)
            {
                return false;
            }
        }
        if (m_visualObject && !gSettings.viewports.bShowIcons && !gSettings.viewports.bShowSizeBasedIcons)
        {
            Matrix34 tm = GetWorldTM();
            float sz = m_helperScale * gSettings.gizmo.helpersScale;
            tm.ScaleColumn(Vec3(sz, sz, sz));
            primitives::ray aray;
            aray.origin = hc.raySrc;
            aray.dir = hc.rayDir * 10000.0f;

            IGeomManager* pGeomMgr = GetIEditor()->GetSystem()->GetIPhysicalWorld()->GetGeomManager();
            IGeometry* pRay = pGeomMgr->CreatePrimitive(primitives::ray::type, &aray);
            geom_world_data gwd;
            gwd.offset = tm.GetTranslation();
            gwd.scale = tm.GetColumn0().GetLength();
            gwd.R = Matrix33(tm);
            geom_contact* pcontacts = 0;
            WriteLockCond lock;

            int col = (m_visualObject->GetPhysGeom() && m_visualObject->GetPhysGeom()->pGeom)
                ? m_visualObject->GetPhysGeom()->pGeom->IntersectLocked(pRay, &gwd, 0, 0, pcontacts, lock)
                : 0;

            pGeomMgr->DestroyGeometry(pRay);
            if (col > 0)
            {
                if (pcontacts)
                {
                    hc.dist = pcontacts[col - 1].t;
                }
                hc.object = this;
                return true;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    if ((m_bDisplayBBox && gSettings.viewports.bShowTriggerBounds) || hc.b2DViewport || (m_bDisplayBBox && m_bBBoxSelection))
    {
        float hitEpsilon = hc.view->GetScreenScaleFactor(GetWorldPos()) * 0.01f;
        float hitDist;

        float fScale = GetScale().x;
        AABB boxScaled;
        boxScaled.min = m_box.min * fScale;
        boxScaled.max = m_box.max * fScale;

        Matrix34 invertWTM = GetWorldTM();
        invertWTM.Invert();

        Vec3 xformedRaySrc = invertWTM.TransformPoint(hc.raySrc);
        Vec3 xformedRayDir = invertWTM.TransformVector(hc.rayDir);
        xformedRayDir.Normalize();

        {
            Vec3 intPnt;
            if (m_bBBoxSelection)
            {
                // Check intersection with bbox.
                if (Intersect::Ray_AABB(xformedRaySrc, xformedRayDir, boxScaled, intPnt))
                {
                    hc.dist = xformedRaySrc.GetDistance(intPnt);
                    hc.object = this;
                    return true;
                }
            }
            else
            {
                // Check intersection with bbox edges.
                if (Intersect::Ray_AABBEdge(xformedRaySrc, xformedRayDir, boxScaled, hitEpsilon, hitDist, intPnt))
                {
                    hc.dist = xformedRaySrc.GetDistance(intPnt);
                    hc.object = this;
                    return true;
                }
            }
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::HitHelperTest(HitContext& hc)
{
    bool bResult = CBaseObject::HitHelperTest(hc);

    if (!bResult && m_pEntity && GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        bResult = HitHelperAtTest(hc, m_pEntity->GetWorldPos());
    }

    if (bResult)
    {
        hc.object = this;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::HitTestRect(HitContext& hc)
{
    bool bResult = false;

    if (m_visualObject && !gSettings.viewports.bShowIcons && !gSettings.viewports.bShowSizeBasedIcons)
    {
        AABB box;
        box.SetTransformedAABB(GetWorldTM(), m_visualObject->GetAABB());
        bResult = HitTestRectBounds(hc, box);
    }
    else
    {
        bResult = CBaseObject::HitTestRect(hc);
        if (!bResult && m_pEntity && GetIEditor()->GetGameEngine()->GetSimulationMode())
        {
            AABB box;
            if (hc.bUseSelectionHelpers)
            {
                box.max = box.min = m_pEntity->GetWorldPos();
            }
            else
            {
                // Retrieve world space bound box.
                m_pEntity->GetWorldBounds(box);
            }

            bResult = HitTestRectBounds(hc, box);
        }
    }

    if (bResult)
    {
        hc.object = this;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
int CEntityObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown)
    {
        Vec3 pos;
        // Rise Entity above ground on Bounding box amount.
        if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
        {
            pos = view->MapViewToCP(point);
        }
        else
        {
            // Snap to terrain.
            bool hitTerrain;
            pos = view->ViewToWorld(point, &hitTerrain);
            if (hitTerrain)
            {
                pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);
                pos.z = pos.z - m_box.min.z;
            }
            pos = view->SnapToGrid(pos);
        }
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
bool CEntityObject::CreateGameObject()
{
    if (!m_pEntityScript)
    {
        if (!m_entityClass.isEmpty())
        {
            SetClass(m_entityClass, true);

            SetCommonEntityParamsFromProperties();
        }
    }
    if (!m_pEntity)
    {
        if (!m_entityClass.isEmpty())
        {
            SpawnEntity();

            //We check whether the Editor is in game or simulation mode during object creation,if true
            //than we are sending a reset event to the entity with true parameter true
            CGameEngine* gameEngine = GetIEditor()->GetGameEngine();
            if (gameEngine->IsInGameMode() || gameEngine->GetSimulationMode())
            {
                SEntityEvent event;
                event.event = ENTITY_EVENT_RESET;
                event.nParam[0] = (UINT_PTR)1;
                m_pEntity->SendEvent(event);
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::SetClass(const QString& entityClass, bool bForceReload, bool bGetScriptProperties, XmlNodeRef xmlProperties, XmlNodeRef xmlProperties2)
{
    // [9/29/2009 evgeny] Added "&& !bGetScriptProperties", because if bGetScriptProperties,
    // then we should reach the SetEntityScript method (at the end of this method)
    if (entityClass == m_entityClass && m_pEntityScript != 0 && !bForceReload && !bGetScriptProperties)
    {
        return true;
    }
    if (entityClass.isEmpty())
    {
        return false;
    }

    m_entityClass = entityClass;
    m_bLoadFailed = false;

    UnloadScript();

    if (!IsCreateGameObjects())
    {
        return false;
    }

    CEntityScript* pEntityScript = CEntityScriptRegistry::Instance()->Find(m_entityClass);
    if (!pEntityScript)
    {
        OnLoadFailed();
        return false;
    }

    SetEntityScript(pEntityScript, bForceReload, bGetScriptProperties, xmlProperties, xmlProperties2);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::SetEntityScript(CEntityScript* pEntityScript, bool bForceReload, bool bGetScriptProperties, XmlNodeRef xmlProperties, XmlNodeRef xmlProperties2)
{
    assert(pEntityScript);

    if (pEntityScript == m_pEntityScript && !bForceReload)
    {
        return true;
    }
    m_pEntityScript = pEntityScript;

    m_bLoadFailed = false;
    // Load script if its not loaded yet.
    if (!m_pEntityScript->IsValid())
    {
        if (!m_pEntityScript->Load())
        {
            OnLoadFailed();
            return false;
        }
    }

    int nTexIcon = pEntityScript->GetTextureIcon();
    if (nTexIcon)
    {
        SetTextureIcon(pEntityScript->GetTextureIcon());
    }

    //////////////////////////////////////////////////////////////////////////
    // Turns of Cast Shadow variable for Light entities.
    if (QString::compare(pEntityScript->GetName(), CLASS_LIGHT, Qt::CaseInsensitive) == 0)
    {
        mv_castShadowMinSpec = END_CONFIG_SPEC_ENUM;
        mv_castShadowMinSpec->SetFlags(mv_castShadowMinSpec->GetFlags() | IVariable::UI_INVISIBLE);

        mv_ratioLOD.SetFlags(mv_ratioLOD.GetFlags() | IVariable::UI_INVISIBLE);
        mv_recvWind.SetFlags(mv_recvWind.GetFlags() | IVariable::UI_INVISIBLE);
        mv_noDecals.SetFlags(mv_noDecals.GetFlags() | IVariable::UI_INVISIBLE);

        mv_obstructionMultiplier.SetFlags(mv_obstructionMultiplier.GetFlags() | IVariable::UI_INVISIBLE);

        Vec3 vNoScale(1.0f, 1.0f, 1.0f);

        if (GetScale().GetSquaredDistance(vNoScale) != 0)
        {
            bool bWasForceScale = GetForceScale();
            SetForceScale(true);
            SetScale(vNoScale);

            if (!bWasForceScale)
            {
                m_bForceScale = false;
            }

            CErrorRecord error;
            error.pObject = this;
            error.count = 1;
            error.severity = CErrorRecord::ESEVERITY_WARNING;
            error.error = tr("Light %1 had scale different than x=1,y=1,z=1. This issue was fixed to avoid light scissors effect").arg(this->GetName());
            error.description = "Scale was set to 1";
            GetIEditor()->GetErrorReport()->ReportError(error);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Make visual editor object for this entity.
    //////////////////////////////////////////////////////////////////////////
    if (!m_pEntityScript->GetVisualObject().isEmpty())
    {
        m_visualObject = GetIEditor()->Get3DEngine()->LoadStatObjUnsafeManualRef(m_pEntityScript->GetVisualObject().toUtf8().data(), NULL, NULL, false);
        if (m_visualObject)
        {
            m_visualObject->AddRef();
        }
    }

    if (bGetScriptProperties)
    {
        GetScriptProperties(pEntityScript, xmlProperties, xmlProperties2);
    }

    // Populate empty events from the script.
    for (int i = 0, num = m_pEntityScript->GetEmptyLinkCount(); i < num; i++)
    {
        const QString& linkName = m_pEntityScript->GetEmptyLink(i);
        int j = 0;
        int numlinks = ( int )m_links.size();
        for (j = 0; j < numlinks; j++)
        {
            if (QString::compare(m_links[j].name, linkName) == 0)
            {
                break;
            }
        }
        if (j >= numlinks)
        {
            AddEntityLink(linkName, GUID_NULL);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::GetScriptProperties(CEntityScript* pEntityScript, XmlNodeRef xmlProperties, XmlNodeRef xmlProperties2)
{
    // Create Entity properties from Script properties..
    if (m_prototype == NULL && m_pEntityScript->GetDefaultProperties())
    {
        CVarBlockPtr oldProperties = m_pProperties;
        m_pProperties = CloneProperties(m_pEntityScript->GetDefaultProperties());

        if (xmlProperties)
        {
            s_ignorePropertiesUpdate = true;
            m_pProperties->Serialize(xmlProperties, true);
            s_ignorePropertiesUpdate = false;
        }
        else if (oldProperties)
        {
            // If we had propertied before copy their values to new script.
            s_ignorePropertiesUpdate = true;
            m_pProperties->CopyValuesByName(oldProperties);
            s_ignorePropertiesUpdate = false;
        }

        // High limit for snow flake count for snow entity.
        if (IVariable* pSubBlockSnowFall = m_pProperties->FindVariable("SnowFall"))
        {
            if (IVariable* pSnowFlakeCount = FindVariableInSubBlock(m_pProperties, pSubBlockSnowFall, "nSnowFlakeCount"))
            {
                pSnowFlakeCount->SetLimits(0, 10000, 0, true, true);
            }
        }

        // For all lights, we set the minimum radius to 0.01, as a radius of
        // 0 would bring serious numerical problems.
        // For old light entities increment version number
        if ((QString::compare(pEntityScript->GetName(), CLASS_LIGHT, Qt::CaseInsensitive) == 0) || (QString::compare(pEntityScript->GetName(), CLASS_ENVIRONMENT_LIGHT, Qt::CaseInsensitive) == 0))
        {
            AdjustLightProperties(m_pProperties, NULL);
        }

        // hide common params table from properties panel
        if (IVariable* pCommonParamsVar = m_pProperties->FindVariable(COMMONPARAMS_TABLE))
        {
            pCommonParamsVar->SetFlags(pCommonParamsVar->GetFlags() | IVariable::UI_INVISIBLE);
        }
    }

    // Create Entity properties from Script properties..
    if (m_pEntityScript->GetDefaultProperties2())
    {
        CVarBlockPtr oldProperties = m_pProperties2;
        m_pProperties2 = CloneProperties(m_pEntityScript->GetDefaultProperties2());

        if (xmlProperties2)
        {
            s_ignorePropertiesUpdate = true;
            m_pProperties2->Serialize(xmlProperties2, true);
            s_ignorePropertiesUpdate = false;
        }
        else if (oldProperties)
        {
            // If we had properties before copy their values to new script.
            s_ignorePropertiesUpdate = true;
            m_pProperties2->CopyValuesByName(oldProperties);
            s_ignorePropertiesUpdate = false;
        }

        if ((QString::compare(pEntityScript->GetName(), CLASS_RIGIDBODY_LIGHT, Qt::CaseInsensitive) == 0 || QString::compare(pEntityScript->GetName(), CLASS_DESTROYABLE_LIGHT, Qt::CaseInsensitive) == 0) && m_pProperties2)
        {
            AdjustLightProperties(m_pProperties2, "LightProperties_Base");
            AdjustLightProperties(m_pProperties2, "LightProperties_Destroyed");
        }
    }

    ResetCallbacks();
}

//////////////////////////////////////////////////////////////////////////
IVariable* CEntityObject::FindVariableInSubBlock(CVarBlockPtr& properties, IVariable* pSubBlockVar, const char* pVarName)
{
    IVariable* pVar = pSubBlockVar ? pSubBlockVar->FindVariable(pVarName) : properties->FindVariable(pVarName);
    return pVar;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::AdjustLightProperties(CVarBlockPtr& properties, const char* pSubBlock)
{
    IVariable* pSubBlockVar = pSubBlock ? properties->FindVariable(pSubBlock) : NULL;

    if (IVariable* pRadius = FindVariableInSubBlock(properties, pSubBlockVar, "Radius"))
    {
        // The value of 0.01 was found through asking Crysis 2 designer
        // team.
        pRadius->SetLimits(0.01f, 100.0f, 0.0f, true, false);
    }

    if (IVariable* pBoxSizeX = FindVariableInSubBlock(properties, pSubBlockVar, "BoxSizeX"))
    {
        pBoxSizeX->SetLimits(0.01f, 100.0f, 0.0f, true, false);
    }

    if (IVariable* pBoxSizeY = FindVariableInSubBlock(properties, pSubBlockVar, "BoxSizeY"))
    {
        pBoxSizeY->SetLimits(0.01f, 100.0f, 0.0f, true, false);
    }

    if (IVariable* pBoxSizeZ = FindVariableInSubBlock(properties, pSubBlockVar, "BoxSizeZ"))
    {
        pBoxSizeZ->SetLimits(0.01f, 100.0f, 0.0f, true, false);
    }

    if (IVariable* pProjectorFov = FindVariableInSubBlock(properties, pSubBlockVar, "fProjectorFov"))
    {
        pProjectorFov->SetLimits(0.01f, 180.0f, 0.0f, true, true);
    }

    if (IVariable* pPlaneWidth = FindVariableInSubBlock(properties, pSubBlockVar, "fPlaneWidth"))
    {
        pPlaneWidth->SetLimits(0.01f, 10.0f, 0.0f, true, false);
        pPlaneWidth->SetHumanName("SourceWidth");
    }

    if (IVariable* pPlaneHeight = FindVariableInSubBlock(properties, pSubBlockVar, "fPlaneHeight"))
    {
        pPlaneHeight->SetLimits(0.01f, 10.0f, 0.0f, true, false);
        pPlaneHeight->SetHumanName("SourceDiameter");
    }

    // For backwards compatibility with old lights (avoids changing settings in Lua which will break loading compatibility).
    // Todo: Change the Lua property names on the next big light refactor.
    if (IVariable* pAreaLight = FindVariableInSubBlock(properties, pSubBlockVar, "bAreaLight"))
    {
        pAreaLight->SetHumanName("PlanarLight");
    }

    bool bCastShadowLegacy = false;  // Backward compatibility for existing shadow casting lights
    if (IVariable* pCastShadowVarLegacy = FindVariableInSubBlock(properties, pSubBlockVar, "bCastShadow"))
    {
        pCastShadowVarLegacy->SetFlags(pCastShadowVarLegacy->GetFlags() | IVariable::UI_INVISIBLE);

        if (pCastShadowVarLegacy->GetDisplayValue()[0] != '0')
        {
            bCastShadowLegacy = true;
            pCastShadowVarLegacy->SetDisplayValue("0");
        }
    }

    if (IVariable* pCastShadowVar = FindVariableInSubBlock(properties, pSubBlockVar, "nCastShadows"))
    {
        if (bCastShadowLegacy)
        {
            pCastShadowVar->SetDisplayValue("1");
        }
        pCastShadowVar->SetDataType(IVariable::DT_UIENUM);
        pCastShadowVar->SetFlags(pCastShadowVar->GetFlags() | IVariable::UI_UNSORTED);
    }

    if (IVariable* pShadowMinRes = FindVariableInSubBlock(properties, pSubBlockVar, "nShadowMinResPercent"))
    {
        pShadowMinRes->SetDataType(IVariable::DT_UIENUM);
        pShadowMinRes->SetFlags(pShadowMinRes->GetFlags() | IVariable::UI_UNSORTED);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsLeft"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsRight"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsNear"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsFar"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsTop"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsBottom"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pSortPriority = FindVariableInSubBlock(properties, pSubBlockVar, "SortPriority"))
    {
        pSortPriority->SetLimits(0, 255, 1, true, true);
    }

    if (IVariable* pAttenFalloffMax = FindVariableInSubBlock(properties, pSubBlockVar, "fAttenuationFalloffMax"))
    {
        pAttenFalloffMax->SetLimits(0.0f, 1.0f, 1.0f / 255.0f, true, true);
    }

    if (IVariable* pVer = FindVariableInSubBlock(properties, pSubBlockVar, "_nVersion"))
    {
        int version = -1;
        pVer->Get(version);
        if (version == -1)
        {
            version++;
            pVer->Set(version);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetCommonEntityParamsFromProperties()
{
    if (m_pProperties)
    {
        // retrieve common params from table
        if (IVariable* pCommonParamsVar = m_pProperties->FindVariable(COMMONPARAMS_TABLE))
        {
            // TODO: additional params to set
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SpawnEntity()
{
    if (!m_pEntityScript)
    {
        return;
    }

    // Do not spawn second time.
    if (m_pEntity)
    {
        return;
    }

    m_bLoadFailed = false;

    IEntitySystem* pEntitySystem = GetIEditor()->GetSystem()->GetIEntitySystem();

    if (m_entityId != 0)
    {
        if (pEntitySystem && pEntitySystem->IsIDUsed(m_entityId))
        {
            m_entityId = 0;
        }
    }

    SEntitySpawnParams params;
    params.pClass = m_pEntityScript->GetClass();
    params.nFlags = 0;
    QByteArray name = GetName().toUtf8();
    params.sName = name.data();
    params.vPosition = GetPos();
    params.qRotation = GetRotation();
    params.id = m_entityId;

    m_bLight = strstr(params.pClass->GetScriptFile(), "/Lights/") ? true : false;

    if (m_prototype)
    {
        params.pArchetype = m_prototype->GetIEntityArchetype();
    }

    if (mv_castShadowMinSpec <= gSettings.editorConfigSpec)
    {
        params.nFlags  |= ENTITY_FLAG_CASTSHADOW;
    }

    if (mv_outdoor)
    {
        params.nFlags |= ENTITY_FLAG_OUTDOORONLY;
    }
    if (mv_recvWind)
    {
        params.nFlags |= ENTITY_FLAG_RECVWIND;
    }
    if (mv_noDecals)
    {
        params.nFlags |= ENTITY_FLAG_NO_DECALNODE_DECALS;
    }

    if (params.id == 0)
    {
        params.bStaticEntityId = true; // Tells to Entity system to generate new static id.
    }

    params.guid = ToEntityGuid(GetId());

    // Spawn Entity but not initialize it.
    m_pEntity = pEntitySystem ? pEntitySystem->SpawnEntity(params, false) : nullptr;
    if (m_pEntity)
    {
        m_entityId = m_pEntity->GetId();

        s_entityIdMap[m_entityId] = this;

        CopyPropertiesToScriptTables();

        gEnv->pEntitySystem->AddEntityEventListener(m_entityId, ENTITY_EVENT_SCRIPT_PROPERTY_ANIMATED, this);

        m_pEntityScript->SetEventsTable(this);

        // Mark this entity non destroyable.
        m_pEntity->AddFlags(ENTITY_FLAG_UNREMOVABLE);

        // Force transformation on entity.
        XFormGameEntity();

        PreInitLightProperty();

        //////////////////////////////////////////////////////////////////////////
        // Now initialize entity.
        //////////////////////////////////////////////////////////////////////////
        if (!pEntitySystem->InitEntity(m_pEntity, params))
        {
            m_pEntity = 0;
            OnLoadFailed();
            return;
        }

        // Bind to parent.
        BindToParent();
        BindIEntityChilds();
        UpdateIEntityLinks(false);

        m_pEntity->Hide(!m_bVisible);

        //////////////////////////////////////////////////////////////////////////
        // If have material, assign it to the entity.
        if (GetMaterial())
        {
            UpdateMaterialInfo();
        }

        // Update render flags of entity (Must be after InitEntity).
        OnRenderFlagsChange(0);

        if (!m_physicsState)
        {
            m_pEntity->SetPhysicsState(m_physicsState);
        }

        //////////////////////////////////////////////////////////////////////////
        // Check if needs to display bbox for this entity.
        //////////////////////////////////////////////////////////////////////////
        m_bCalcPhysics = true;
        if (m_pEntity->GetPhysics() != 0)
        {
            m_bDisplayBBox = false;
            if (m_pEntity->GetPhysics()->GetType() == PE_SOFT)
            {
                m_bCalcPhysics = false;
                //! Ignore entity being updated from physics.
                m_pEntity->SetFlags(m_pEntity->GetFlags() | ENTITY_FLAG_IGNORE_PHYSICS_UPDATE);
            }
        }
        else
        {
            if (m_pEntityScript->GetFlags() & ENTITY_SCRIPT_SHOWBOUNDS)
            {
                m_bDisplayBBox = true;
                m_bDisplaySolidBBox = true;
            }
            else
            {
                m_bDisplayBBox = false;
                m_bDisplaySolidBBox = false;
            }

            m_bDisplayAbsoluteRadius = (m_pEntityScript->GetFlags() & ENTITY_SCRIPT_ABSOLUTERADIUS) ? true : false;
        }

        m_bBBoxSelection = (m_pEntityScript->GetClass()->GetFlags() & ECLF_BBOX_SELECTION) ? true : false;

        m_bIconOnTop = (m_pEntityScript->GetFlags() & ENTITY_SCRIPT_ICONONTOP) != 0;
        if (m_bIconOnTop)
        {
            SetFlags(OBJFLAG_SHOW_ICONONTOP);
        }
        else
        {
            ClearFlags(OBJFLAG_SHOW_ICONONTOP);
        }

        m_bDisplayArrow = (m_pEntityScript->GetFlags() & ENTITY_SCRIPT_DISPLAY_ARROW) ? true : false;

        if (m_pEntityScript->GetTextureIcon())
        {
            m_pEntityScript->UpdateTextureIcon(m_pEntity);
            SetTextureIcon(m_pEntityScript->GetTextureIcon());
        }
        else if (GetClassDesc()->GetTextureIconId())
        {
            SetTextureIcon(GetClassDesc()->GetTextureIconId());
        }

        //////////////////////////////////////////////////////////////////////////
        // Calculate entity bounding box.
        CalcBBox();

        if (m_pFlowGraph)
        {
            // Re-apply entity for flow graph.
            m_pFlowGraph->SetEntity(this, true);

            IComponentFlowGraphPtr pFlowGraphComponent = m_pEntity->GetOrCreateComponent<IComponentFlowGraph>();
            IFlowGraph* pGameFlowGraph = m_pFlowGraph->GetIFlowGraph();
            pFlowGraphComponent->SetFlowGraph(pGameFlowGraph);
            if (pGameFlowGraph)
            {
                pGameFlowGraph->SetActive(true);
            }

            // Check for new entity flownode data
            CFlowGraphManager* pFGManager = GetIEditor()->GetFlowGraphManager();

            if (pFGManager)
            {
                int numFG = pFGManager->GetFlowGraphCount();

                for (int i = 0; i < numFG; ++i)
                {
                    CFlowGraph* pFG = pFGManager->GetFlowGraph(i);

                    if (pFG)
                    {
                        IHyperGraphEnumerator* pEnum = pFG->GetNodesEnumerator();
                        for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
                        {
                            CHyperNode* pNode = (CHyperNode*)pINode;

                            string classname = string("entity:") + GetIEntity()->GetClass()->GetName();
                            TFlowNodeTypeId id = gEnv->pFlowSystem->GetTypeId(classname);
                            IFlowNodeData* pFlowNodeData = pFG->GetIFlowGraph()->GetNodeData(pNode->GetFlowNodeId());

                            if (pFlowNodeData && pFlowNodeData->GetNodeTypeId() == id)
                            {
                                // Reload flownode port data
                                pFGManager->ReloadNodeConfig(pFlowNodeData, (CFlowNode*)pNode);
                                // Check for non existing port connections after reloading data
                                pFG->ValidateEdges(pNode);
                                pFG->SendNotifyEvent(0, EHG_GRAPH_INVALIDATE);
                            }
                        }
                        pEnum->Release();
                    }
                }
            }
        }
        if (m_physicsState)
        {
            m_pEntity->SetPhysicsState(m_physicsState);
        }
    }
    else
    {
        OnLoadFailed();
    }

    UpdateLightProperty();
    UpdatePropertyPanels(true);
    CheckSpecConfig();

    InstallStatObjListeners();
    InstallCharacterBoundsListeners();
   
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::DeleteEntity()
{
    if (m_pEntity)
    {
        UnbindIEntity();
        m_pEntity->ClearFlags(ENTITY_FLAG_UNREMOVABLE);
        if (IEntitySystem* pEntitySystem = gEnv->pEntitySystem)
        {
            pEntitySystem->RemoveEntity(m_pEntity->GetId(), true);
            gEnv->pEntitySystem->RemoveEntityEventListener(m_entityId, ENTITY_EVENT_SCRIPT_PROPERTY_ANIMATED, this);
        }
        s_entityIdMap.erase(m_entityId);
    }
    m_pEntity = 0;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UnloadScript()
{
    ClearCallbacks();

    if (m_pEntity)
    {
        DeleteEntity();
    }
    if (m_visualObject)
    {
        m_visualObject->Release();
    }
    m_visualObject = 0;
    m_pEntityScript = 0;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::XFormGameEntity()
{
    CBaseObject* pBaseObject = GetParent();
    if (qobject_cast<CEntityObject*>(pBaseObject))
    {
        static_cast<CEntityObject*>(pBaseObject)->XFormGameEntity();
    }

    // Make sure components are correctly calculated.
    if (m_pEntity)
    {
        if (m_pEntity->GetParent())
        {
            // If the entity is linked to its parent in game avoid precision issues from inverse/forward
            // transform calculations by setting the local matrix instead of the world matrix
            const Matrix34& tm = GetLocalTM();
            m_pEntity->SetLocalTM(tm, ENTITY_XFORM_EDITOR);
        }
        else
        {
            const Matrix34& tm = GetWorldTM();
            m_pEntity->SetWorldTM(tm, ENTITY_XFORM_EDITOR);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::CalcBBox()
{
    if (m_pEntity)
    {
        // Get Local bounding box of entity.
        m_pEntity->GetLocalBounds(m_box);

        if (m_box.min.x >= m_box.max.x || m_box.min.y >= m_box.max.y || m_box.min.z >= m_box.max.z)
        {
            if (m_visualObject)
            {
                Vec3 minp = m_visualObject->GetBoxMin() * m_helperScale * gSettings.gizmo.helpersScale;
                Vec3 maxp = m_visualObject->GetBoxMax() * m_helperScale * gSettings.gizmo.helpersScale;
                m_box.Add(minp);
                m_box.Add(maxp);
            }
            else
            {
                m_box = AABB(ZERO);
            }
        }
        float minSize = 0.0001f;
        if (fabs(m_box.max.x - m_box.min.x) + fabs(m_box.max.y - m_box.min.y) + fabs(m_box.max.z - m_box.min.z) < minSize)
        {
            m_box.min = -Vec3(minSize, minSize, minSize);
            m_box.max = Vec3(minSize, minSize, minSize);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetName(const QString& name)
{
    if (name == GetName())
    {
        return;
    }

    const QString oldName = GetName();

    CBaseObject::SetName(name);
    if (m_pEntity)
    {
        m_pEntity->SetName(GetName().toUtf8().data());
    }

    CListenerSet<IEntityObjectListener*> listeners = m_listeners;
    for (CListenerSet<IEntityObjectListener*>::Notifier notifier(listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnNameChanged(name.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::BeginEditParams(IEditor* ie, int flags)
{
    CBaseObject::BeginEditParams(ie, flags);

    if (m_pProperties2 != NULL)
    {
        if (!s_pPropertiesPanel2)
        {
            s_pPropertiesPanel2 = new ReflectedPropertiesPanel;
            s_pPropertiesPanel2->Setup(true, 180);
        }
        else
        {
            s_pPropertiesPanel2->DeleteVars();
        }
        s_pPropertiesPanel2->AddVars(m_pProperties2);
        if (!s_propertiesPanelIndex2)
        {
            s_propertiesPanelIndex2 = AddUIPage(QString(GetTypeName() + " Properties2").toUtf8().data(), s_pPropertiesPanel2);
        }
    }

    if (!m_prototype)
    {
        if (m_pProperties != NULL)
        {
            if (!s_pPropertiesPanel)
            {
                s_pPropertiesPanel = new ReflectedPropertiesPanel;
                s_pPropertiesPanel->Setup(true, 180);
            }
            else
            {
                s_pPropertiesPanel->DeleteVars();
            }
            s_pPropertiesPanel->AddVars(m_pProperties);
            if (!s_propertiesPanelIndex)
            {
                s_propertiesPanelIndex = AddUIPage(QString(GetTypeName() + " Properties").toUtf8().data(), s_pPropertiesPanel);
            }

            if (s_pPropertiesPanel)
            {
                s_pPropertiesPanel->SetUpdateCallback(functor(*this, &CBaseObject::OnPropertyChanged));
            }
        }
    }

    if (!m_panel && m_pEntity)
    {
        m_panel = new CEntityPanel;
        m_rollupId = AddUIPage((QString("Entity: ") + m_entityClass).toUtf8().data(), m_panel);
    }
    if (m_panel)
    {
        m_panel->SetEntity(this);
    }

    //////////////////////////////////////////////////////////////////////////
    // Links Panel
    if (!s_entityLinksPanel && m_pEntity)
    {
        s_entityLinksPanel = new CEntityLinksPanel;
        s_entityLinksPanelIndex = AddUIPage("Entity Links", s_entityLinksPanel, -1, false);
    }
    if (s_entityLinksPanel)
    {
        s_entityLinksPanel->SetEntity(this);
    }

    //////////////////////////////////////////////////////////////////////////
    // Events panel
    if (!s_entityEventsPanel && m_pEntity)
    {
        s_entityEventsPanel = new CEntityEventsPanel(nullptr);
        s_entityEventsPanelIndex = AddUIPage("Entity Events", s_entityEventsPanel, -1, false);
        GetIEditor()->ExpandRollUpPage(s_entityEventsPanelIndex, FALSE);
    }
    if (s_entityEventsPanel)
    {
        s_entityEventsPanel->SetEntity(this);
    }

    s_pPropertyPanelEntityObject = this;
    SetPropertyPanelsState();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::EndEditParams(IEditor* ie)
{
    if (ms_treePanelId != 0)
    {
        RemoveUIPage(ms_treePanelId);
        ms_treePanelId = 0;
    }
    ms_pTreePanel = NULL;

    if (s_pPropertiesPanel && s_pPropertiesPanel)
    {
        s_pPropertiesPanel->ClearUpdateCallback();
    }

    if (s_entityEventsPanelIndex != 0)
    {
        RemoveUIPage(s_entityEventsPanelIndex);
    }
    s_entityEventsPanelIndex = 0;
    s_entityEventsPanel = 0;

    if (s_entityLinksPanelIndex != 0)
    {
        RemoveUIPage(s_entityLinksPanelIndex);
    }
    s_entityLinksPanelIndex = 0;
    s_entityLinksPanel = 0;

    if (m_rollupId != 0)
    {
        RemoveUIPage(m_rollupId);
    }
    m_rollupId = 0;
    m_panel = 0;

    if (s_propertiesPanelIndex != 0)
    {
        s_pPropertiesPanel->DeleteVars();
        RemoveUIPage(s_propertiesPanelIndex);
    }
    s_propertiesPanelIndex = 0;
    s_pPropertiesPanel = 0;

    if (s_propertiesPanelIndex2 != 0)
    {
        s_pPropertiesPanel2->DeleteVars();
        RemoveUIPage(s_propertiesPanelIndex2);
    }
    s_propertiesPanelIndex2 = 0;
    s_pPropertiesPanel2 = 0;

    CBaseObject::EndEditParams(GetIEditor());

    s_pPropertyPanelEntityObject = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdatePropertyPanels(const bool bReload)
{
    if (s_pPropertyPanelEntityObject != this)
    {
        return;
    }

    if (bReload)
    {
        if (s_pPropertiesPanel && m_pProperties)
        {
            s_pPropertiesPanel->DeleteVars();
            s_pPropertiesPanel->AddVars(m_pProperties);
        }

        if (s_pPropertiesPanel2 && m_pProperties2)
        {
            s_pPropertiesPanel2->DeleteVars();
            s_pPropertiesPanel2->AddVars(m_pProperties2);
        }
    }
    else
    {
        if (s_pPropertiesPanel && m_pProperties)
        {
            s_pPropertiesPanel->UpdateVarBlock(m_pProperties);
        }

        if (s_pPropertiesPanel2 && m_pProperties2)
        {
            s_pPropertiesPanel2->UpdateVarBlock(m_pProperties2);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CBaseObject::BeginEditMultiSelParams(bAllOfSameType);

    if (!bAllOfSameType)
    {
        return;
    }

    if (m_pProperties2 != NULL)
    {
        if (!s_pPropertiesPanel2)
        {
            s_pPropertiesPanel2 = new ReflectedPropertiesPanel;
            s_pPropertiesPanel2->Setup(true, 180);
        }
        else
        {
            s_pPropertiesPanel2->DeleteVars();
        }

        // Add all selected objects.
        CSelectionGroup* grp = GetIEditor()->GetSelection();
        for (int i = 0; i < grp->GetCount(); i++)
        {
            CEntityObject* ent = ( CEntityObject* )grp->GetObject(i);

            if (ent->m_pProperties2)
            {
                s_pPropertiesPanel2->AddVars(ent->m_pProperties2);
            }
        }
        if (!s_propertiesPanelIndex2)
        {
            s_propertiesPanelIndex2 = AddUIPage(QString(GetTypeName() + " Properties").toUtf8().data(), s_pPropertiesPanel2);
        }

        if (s_pPropertiesPanel2)
        {
            s_pPropertiesPanel2->SetUpdateCallback(functor(*this, &CBaseObject::OnMultiSelPropertyChanged));
        }
    }

    if (m_pProperties && !m_prototype)
    {
        if (!s_pPropertiesPanel)
        {
            s_pPropertiesPanel = new ReflectedPropertiesPanel;
            s_pPropertiesPanel->Setup(true, 180);
        }
        else
        {
            s_pPropertiesPanel->DeleteVars();
        }

        // Add all selected objects.
        CSelectionGroup* grp = GetIEditor()->GetSelection();
        for (int i = 0; i < grp->GetCount(); i++)
        {
            CEntityObject* ent = ( CEntityObject* )grp->GetObject(i);
            if (ent->m_pProperties)
            {
                s_pPropertiesPanel->AddVars(ent->m_pProperties);
            }
        }
        if (!s_propertiesPanelIndex)
        {
            s_propertiesPanelIndex = AddUIPage((GetTypeName() + " Properties").toUtf8().data(), s_pPropertiesPanel);
        }

        if (s_pPropertiesPanel)
        {
            s_pPropertiesPanel->SetUpdateCallback(functor(*this, &CBaseObject::OnMultiSelPropertyChanged));
        }
    }

    s_pPropertyPanelEntityObject = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::EndEditMultiSelParams()
{
    if (s_pPropertiesPanel)
    {
        s_pPropertiesPanel->ClearUpdateCallback();
    }

    if (s_pPropertiesPanel2)
    {
        s_pPropertiesPanel2->ClearUpdateCallback();
    }

    if (s_propertiesPanelIndex != 0)
    {
        s_pPropertiesPanel->DeleteVars();
        RemoveUIPage(s_propertiesPanelIndex);
    }
    s_propertiesPanelIndex = 0;
    s_pPropertiesPanel = 0;

    if (s_propertiesPanelIndex2 != 0)
    {
        s_pPropertiesPanel2->DeleteVars();
        RemoveUIPage(s_propertiesPanelIndex2);
    }
    s_propertiesPanelIndex2 = 0;
    s_pPropertiesPanel2 = 0;

    CBaseObject::EndEditMultiSelParams();

    s_pPropertyPanelEntityObject = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetSelected(bool bSelect)
{
    CBaseObject::SetSelected(bSelect);

    if (m_pEntity)
    {
        IComponentRenderPtr pRenderComponent = m_pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
            if (pRenderNode)
            {
                int flags = pRenderNode->GetRndFlags();
                if (bSelect && gSettings.viewports.bHighlightSelectedGeometry)
                {
                    flags |= ERF_SELECTED;
                }
                else
                {
                    flags &= ~ERF_SELECTED;
                }
                pRenderNode->SetRndFlags(flags);
            }
        }
    }

    if (bSelect)
    {
        UpdateLightProperty();
    }

    for (CListenerSet<IEntityObjectListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnSelectionChanged(bSelect);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAttributeChange(int attributeIndex)
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnPropertyChange(IVariable* var)
{
    if (s_ignorePropertiesUpdate)
    {
        return;
    }

    if (m_pEntityScript != 0 && m_pEntity != 0)
    {
        CopyPropertiesToScriptTables();

        m_pEntityScript->CallOnPropertyChange(m_pEntity);
        if (IsLight() && var && !var->GetName().isNull())
        {
            if (var->GetName().contains("Flare"))
            {
                UpdateLightProperty();
            }
            else if (!QString::compare(var->GetName(), "clrDiffuse", Qt::CaseInsensitive))
            {
                IOpticsElementBasePtr pOptics = GetOpticsElement();
                if (pOptics)
                {
                    pOptics->Invalidate();
                }
            }
        }

        // After change of properties bounding box of entity may change.
        CalcBBox();
        InvalidateTM(0);

        InstallCharacterBoundsListeners();

        // Custom behavior for derived classes
        OnPropertyChangeDone(var);
        // Update prefab data if part of prefab
        UpdatePrefab();
    }
}

template <typename T>
struct IVariableType {};
template <>
struct IVariableType<bool>
{
    enum
    {
        value = IVariable::BOOL
    };
};
template <>
struct IVariableType<int>
{
    enum
    {
        value = IVariable::INT
    };
};
template <>
struct IVariableType<float>
{
    enum
    {
        value = IVariable::FLOAT
    };
};
template <>
struct IVariableType<QString>
{
    enum
    {
        value = IVariable::STRING
    };
};
template <>
struct IVariableType<Vec3>
{
    enum
    {
        value = IVariable::VECTOR
    };
};

void CEntityObject::DrawExtraLightInfo(DisplayContext& dc)
{
    IObjectManager* objMan = GetIEditor()->GetObjectManager();

    if (objMan)
    {
        if (objMan->IsLightClass(this) && GetProperties())
        {
            QString csText("");

            if (GetEntityPropertyBool("bAmbient"))
            {
                csText += "A";
            }

            if (!GetEntityPropertyString("texture_Texture").isEmpty())
            {
                csText += "P";
            }

            int nLightType = GetEntityPropertyInteger("nCastShadows");
            if (nLightType > 0)
            {
                csText += "S";
            }

            float fScale = GetIEditor()->GetViewManager()->GetView(ET_ViewportUnknown)->GetScreenScaleFactor(GetWorldPos());
            Vec3 vDrawPos(GetWorldPos());
            vDrawPos.z += fScale / 25;

            ColorB col(255, 255, 255);
            dc.SetColor(col);
            dc.DrawTextLabel(vDrawPos, 1.3f, csText.toUtf8().data());
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CEntityObject::DrawProjectorPyramid(DisplayContext& dc, float dist)
{
    const int numPoints = 16; // per one arc
    const int numArcs = 6;

    if (m_projectorFOV < FLT_EPSILON)
    {
        return;
    }

    Vec3 points[numPoints * numArcs];
    {
        // generate 4 arcs on intersection of sphere with pyramid
        const float fov = DEG2RAD(m_projectorFOV);

        const Vec3 lightAxis(dist, 0.0f, 0.0f);
        const float tanA = tan(fov * 0.5f);
        const float fovProj = asinf(1.0f / sqrtf(2.0f + 1.0f / (tanA * tanA))) * 2.0f;

        const float halfFov = 0.5f * fov;
        const float halfFovProj = fovProj * 0.5f;
        const float anglePerSegmentOfFovProj = 1.0f / (numPoints - 1) * fovProj;

        const Quat yRot = Quat::CreateRotationY(halfFov);
        Vec3* arcPoints = points;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = i * anglePerSegmentOfFovProj - halfFovProj;
            arcPoints[i] = lightAxis * Quat::CreateRotationZ(angle) * yRot;
        }

        const Quat zRot = Quat::CreateRotationZ(halfFov);
        arcPoints += numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = (numPoints - i - 1) * anglePerSegmentOfFovProj - halfFovProj;
            arcPoints[i] = lightAxis * Quat::CreateRotationY(angle) * zRot;
        }

        const Quat nyRot = Quat::CreateRotationY(-halfFov);
        arcPoints += numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = (numPoints - i - 1) * anglePerSegmentOfFovProj - halfFovProj;
            arcPoints[i] = lightAxis * Quat::CreateRotationZ(angle) * nyRot;
        }

        const Quat nzRot = Quat::CreateRotationZ(-halfFov);
        arcPoints += numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = i * anglePerSegmentOfFovProj - halfFovProj;
            arcPoints[i] = lightAxis * Quat::CreateRotationY(angle) * nzRot;
        }

        // generate cross
        arcPoints += numPoints;
        const float anglePerSegmentOfFov = 1.0f / (numPoints - 1) * fov;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = i * anglePerSegmentOfFov - halfFov;
            arcPoints[i] = lightAxis * Quat::CreateRotationY(angle);
        }

        arcPoints += numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = i * anglePerSegmentOfFov - halfFov;
            arcPoints[i] = lightAxis * Quat::CreateRotationZ(angle);
        }
    }
    // draw pyramid and sphere intersection
    dc.DrawPolyLine(points, numPoints * 4, false);

    // draw cross
    dc.DrawPolyLine(points + numPoints * 4, numPoints, false);
    dc.DrawPolyLine(points + numPoints * 5, numPoints, false);

    Vec3 org(0.0f, 0.0f, 0.0f);
    dc.DrawLine(org, points[numPoints * 0]);
    dc.DrawLine(org, points[numPoints * 1]);
    dc.DrawLine(org, points[numPoints * 2]);
    dc.DrawLine(org, points[numPoints * 3]);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::DrawProjectorFrustum(DisplayContext& dc, Vec2 size, float dist)
{
    static const Vec3 org(0.0f, 0.0f, 0.0f);
    const Vec3 corners[4] =
    {
        Vec3(dist, -size.x, -size.y),
        Vec3(dist, size.x, -size.y),
        Vec3(dist, -size.x, size.y),
        Vec3(dist, size.x, size.y)
    };

    dc.DrawLine(org, corners[0]);
    dc.DrawLine(org, corners[1]);
    dc.DrawLine(org, corners[2]);
    dc.DrawLine(org, corners[3]);

    dc.DrawWireBox(Vec3(dist, -size.x, -size.y), Vec3(dist, size.x, size.y));
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::Display(DisplayContext& dc)
{
    if (!m_pEntity)
    {
        return;
    }

    float fWidth = m_fAreaWidth * 0.5f;
    float fHeight = m_fAreaHeight * 0.5f;

    Matrix34 wtm = GetWorldTM();

    QColor col = GetColor();
    if (IsFrozen())
    {
        col = dc.GetFreezeColor();
    }

    dc.PushMatrix(wtm);

    if (m_bDisplayArrow)
    {
        // Show direction arrow.
        Vec3 dir = FORWARD_DIRECTION;
        if (IsFrozen())
        {
            dc.SetFreezeColor();
        }
        else
        {
            dc.SetColor(1, 1, 0);
        }
        dc.DrawArrow(Vec3(0, 0, 0), FORWARD_DIRECTION * m_helperScale, m_helperScale);
    }

    bool bDisplaySolidBox = (m_bDisplaySolidBBox && gSettings.viewports.bShowTriggerBounds);
    if (IsSelected())
    {
        dc.SetSelectedColor(0.5f);
        if (!m_visualObject || m_bDisplayBBox || (dc.flags & DISPLAY_2D))
        {
            bDisplaySolidBox = m_bDisplaySolidBBox;
            dc.DrawWireBox(m_box.min, m_box.max);
        }
    }
    else
    {
        if ((m_bDisplayBBox && gSettings.viewports.bShowTriggerBounds) || (dc.flags & DISPLAY_2D))
        {
            dc.SetColor(col, 0.3f);
            dc.DrawWireBox(m_box.min, m_box.max);
        }
    }

    // Only display solid BBox if visual object is associated with the entity.
    if (bDisplaySolidBox)
    {
        dc.DepthWriteOff();
        dc.SetColor(col, 0.25f);
        dc.DrawSolidBox(m_box.min, m_box.max);
        dc.DepthWriteOn();
    }

    // Draw area light plane.
    if (IsLight() && IsSelected() && m_bAreaLight)
    {
        Vec3 org(0.0f, 0.0f, 0.0f);

        // Draw plane.
        dc.SetColor(m_lightColor, 0.25f);
        dc.DrawSolidBox(Vec3(0, -fWidth, -fHeight), Vec3(0, fWidth, fHeight));

        // Draw outline and direction line.
        dc.SetColor(m_lightColor, 0.75f);
        dc.DrawWireBox(Vec3(0, -fWidth, -fHeight), Vec3(0, fWidth, fHeight));
        dc.DrawLine(org, Vec3(m_proximityRadius, 0, 0), m_lightColor, m_lightColor);

        // Draw cross.
        dc.DrawLine(org, Vec3(0, fWidth, fHeight));
        dc.DrawLine(org, Vec3(0, -fWidth, fHeight));
        dc.DrawLine(org, Vec3(0, fWidth, -fHeight));
        dc.DrawLine(org, Vec3(0, -fWidth, -fHeight));
    }

    // Draw area light helpers.
    if (IsLight() && IsSelected())
    {
        // Draw light disk
        if (m_bProjectorHasTexture && !m_bAreaLight)
        {
            float fShapeRadius = m_fAreaHeight * 0.5f;
            dc.SetColor(m_lightColor, 0.25f);
            dc.DrawSolidCylinder(Vec3(0, 0, 0), Vec3(1, 0, 0), fShapeRadius, 0.001f);
            dc.SetColor(m_lightColor, 0.75f);
            dc.DrawWireSphere(Vec3(0, 0, 0), Vec3(0, fShapeRadius, fShapeRadius));
            dc.SetColor(0, 1, 0, 0.7f);
            DrawProjectorFrustum(dc, Vec2(fShapeRadius, fShapeRadius), 1.0f);
        }
        else if (m_bAreaLight)  // Draw light rectangle
        {
            static const Vec3 org(0.0f, 0.0f, 0.0f);

            // Draw plane.
            dc.SetColor(m_lightColor, 0.25f);
            dc.DrawSolidBox(Vec3(0, -fWidth, -fHeight), Vec3(0, fWidth, fHeight));

            // Draw outline and frustum.
            dc.SetColor(m_lightColor, 0.75f);
            dc.DrawWireBox(Vec3(0, -fWidth, -fHeight), Vec3(0, fWidth, fHeight));

            // Draw cross.
            dc.DrawLine(org, Vec3(0, fWidth, fHeight));
            dc.DrawLine(org, Vec3(0, -fWidth, fHeight));
            dc.DrawLine(org, Vec3(0, fWidth, -fHeight));
            dc.DrawLine(org, Vec3(0, -fWidth, -fHeight));

            dc.SetColor(0, 1, 0, 0.7f);
            DrawProjectorFrustum(dc, Vec2(fWidth, fHeight), 1.0f);
        }
        else // Draw light sphere.
        {
            float fShapeRadius = m_fAreaHeight * 0.5f;
            dc.SetColor(m_lightColor, 0.25f);
            dc.DrawBall(Vec3(0, 0, 0), fShapeRadius);
            dc.SetColor(m_lightColor, 0.75f);
            dc.DrawWireSphere(Vec3(0, 0, 0), fShapeRadius);
        }
    }

    // Draw radii if present and object selected.
    if (gSettings.viewports.bAlwaysShowRadiuses || IsSelected())
    {
        const Vec3& scale = GetScale();
        float fScale = scale.x; // Ignore matrix scale.
        if (fScale == 0)
        {
            fScale = 1;
        }
        if (m_innerRadius > 0)
        {
            dc.SetColor(0, 1, 0, 0.3f);
            dc.DrawWireSphere(Vec3(0, 0, 0), m_innerRadius / fScale);
        }
        if (m_outerRadius > 0)
        {
            dc.SetColor(1, 1, 0, 0.8f);
            dc.DrawWireSphere(Vec3(0, 0, 0), m_outerRadius / fScale);
        }
        if (m_proximityRadius > 0)
        {
            dc.SetColor(1, 1, 0, 0.8f);

            if (IsLight() && m_bAreaLight)
            {
                float fFOVScale = max(0.001f, m_projectorFOV) / 180.0f;
                float boxWidth = m_proximityRadius + fWidth * fFOVScale;
                float boxHeight = m_proximityRadius + fHeight * fFOVScale;
                dc.DrawWireBox(Vec3(0.0, -boxWidth, -boxHeight), Vec3(m_proximityRadius, boxWidth, boxHeight));
            }

            if (IsLight() && m_bProjectorHasTexture && !m_bAreaLight)
            {
                DrawProjectorPyramid(dc, m_proximityRadius);
            }

            if (!IsLight() || (!m_bProjectorHasTexture && !m_bAreaLight) || m_bProjectInAllDirs)
            {
                if (m_bDisplayAbsoluteRadius)
                {
                    AffineParts ap;
                    //HINT: we need to do this because the entity class does not have a method to get final entity world scale, nice to have one in the future
                    ap.SpectralDecompose(wtm);
                    dc.DrawWireSphere(Vec3(0, 0, 0), Vec3(m_proximityRadius / ap.scale.x, m_proximityRadius / ap.scale.y, m_proximityRadius / ap.scale.z));
                }
                else
                {
                    dc.DrawWireSphere(Vec3(0, 0, 0), m_proximityRadius);
                }
            }
        }
    }

    dc.PopMatrix();

    // Entities themselves are rendered by 3DEngine.

    if (m_visualObject && ((!gSettings.viewports.bShowIcons && !gSettings.viewports.bShowSizeBasedIcons) || !HaveTextureIcon()))
    {
        Matrix34 tm(wtm);
        float sz = m_helperScale * gSettings.gizmo.helpersScale;
        tm.ScaleColumn(Vec3(sz, sz, sz));

        SRendParams rp;
        Vec3 color;
        if (IsSelected())
        {
            color = Vec3(dc.GetSelectedColor().redF(), dc.GetSelectedColor().greenF(), dc.GetSelectedColor().blueF());
        }
        else
        {
            color = Vec3(col.redF(), col.greenF(), col.blueF());
        }
        SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(gEnv->p3DEngine->GetRenderingCamera());

        rp.AmbientColor = ColorF(color[0], color[1], color[2], 1);
        rp.fAlpha = 1;
        rp.nDLightMask = 1;
        rp.pMatrix = &tm;
        rp.pMaterial = GetIEditor()->GetIconManager()->GetHelperMaterial();

        m_visualObject->Render(rp, passInfo);
    }

    if (IsSelected())
    {
        if (m_pEntity)
        {
            IAIObject* pAIObj = m_pEntity->GetAI();
            if (pAIObj)
            {
                DrawAIInfo(dc, pAIObj);
            }
        }
    }

    if (IsSelected())
    {
        DrawExtraLightInfo(dc);
    }

    if ((dc.flags & DISPLAY_HIDENAMES) && gSettings.viewports.bDrawEntityLabels)
    {
        // If labels hidden but we draw entity labels enabled, always draw them.
        CGroup* pGroup = GetGroup();
        if (!pGroup || pGroup->IsOpen())
        {
            DrawLabel(dc, GetWorldPos(), col);
        }
    }

    if (dc.flags & DISPLAY_SELECTION_HELPERS)
    {
        DrawExtraLightInfo(dc);
    }

    if (m_pEntity)
    {
        SGeometryDebugDrawInfo dd;
        dd.tm = wtm;
        dd.bExtrude = true;
        dd.color = ColorB(0, 0, 0, 0);
        dd.lineColor = ColorB(0, 0, 0, 0);

        if (IsHighlighted() && gSettings.viewports.bHighlightMouseOverGeometry)
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
            m_pEntity->DebugDraw(dd);
        }
    }

    DrawDefault(dc, col);

    IObjectManager* pObjectManager = GetObjectManager();
    if (pObjectManager->IsEntityAssignedToSelectedTerritory(this))
    {
        Vec3 wp = GetWorldPos();
        AABB box;
        GetBoundBox(box);
        wp.z = box.max.z;

        dc.DrawTextureLabel(
            wp,
            OBJECT_TEXTURE_ICON_SIZEX,
            OBJECT_TEXTURE_ICON_SIZEY,
            GetIEditor()->GetIconManager()->GetIconTexture("Editor/ObjectIcons/T.bmp"),
            DisplayContext::TEXICON_ON_TOP | DisplayContext::TEXICON_ALIGN_BOTTOM);
    }
    if (pObjectManager->IsEntityAssignedToSelectedWave(this))
    {
        Vec3 wp = GetWorldPos();
        AABB box;
        GetBoundBox(box);
        wp.z = box.max.z;

        dc.DrawTextureLabel(
            wp,
            OBJECT_TEXTURE_ICON_SIZEX,
            OBJECT_TEXTURE_ICON_SIZEY,
            GetIEditor()->GetIconManager()->GetIconTexture("Editor/ObjectIcons/W.bmp"),
            DisplayContext::TEXICON_ON_TOP | DisplayContext::TEXICON_ALIGN_TOP);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::DrawAIInfo(DisplayContext& dc, IAIObject* aiObj)
{
    assert(aiObj);

    IAIActor* pAIActor = aiObj->CastToIAIActor();
    if (!pAIActor)
    {
        return;
    }

    const AgentParameters& ap = pAIActor->GetParameters();

    // Draw ranges.
    bool bTerrainCircle = false;
    Vec3 wp = GetWorldPos();
    float z = GetIEditor()->GetTerrainElevation(wp.x, wp.y);
    if (fabs(wp.z - z) < 5)
    {
        bTerrainCircle = true;
    }

    dc.SetColor(QColor(255 / 2, 0, 0));
    if (bTerrainCircle)
    {
        dc.DrawTerrainCircle(wp, ap.m_PerceptionParams.sightRange / 2, 0.2f);
    }
    else
    {
        dc.DrawCircle(wp, ap.m_PerceptionParams.sightRange / 2);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::Serialize(CObjectArchive& ar)
{
    CBaseObject::Serialize(ar);
    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        // Load
        QString entityClass = m_entityClass;
        m_bLoadFailed = false;

        if (!m_prototype)
        {
            xmlNode->getAttr("EntityClass", entityClass);
        }
        m_physicsState = xmlNode->findChild("PhysicsState");

        Vec3 angles;
        // Backward compatibility, with FarCry levels.
        if (xmlNode->getAttr("Angles", angles))
        {
            angles = DEG2RAD(angles);
            angles.z += gf_PI / 2;
            Quat quat;
            quat.SetRotationXYZ(Ang3(angles));
            SetRotation(quat);
        }

        // Load Event Targets.
        ReleaseEventTargets();
        XmlNodeRef eventTargets = xmlNode->findChild("EventTargets");
        if (eventTargets)
        {
            for (int i = 0; i < eventTargets->getChildCount(); i++)
            {
                XmlNodeRef eventTarget = eventTargets->getChild(i);
                CEntityEventTarget et;
                et.target = 0;
                GUID targetId = GUID_NULL;
                eventTarget->getAttr("TargetId", targetId);
                eventTarget->getAttr("Event", et.event);
                eventTarget->getAttr("SourceEvent", et.sourceEvent);
                m_eventTargets.push_back(et);
                if (targetId != GUID_NULL)
                {
                    ar.SetResolveCallback(this, targetId, functor(*this, &CEntityObject::ResolveEventTarget), i);
                }
            }
        }

        XmlNodeRef propsNode;
        XmlNodeRef props2Node = xmlNode->findChild("Properties2");
        if (!m_prototype)
        {
            propsNode = xmlNode->findChild("Properties");
        }

        QString attachmentType;
        xmlNode->getAttr("AttachmentType", attachmentType);

        if (attachmentType == "GeomCacheNode")
        {
            m_attachmentType = eAT_GeomCacheNode;
        }
        else if (attachmentType == "CharacterBone")
        {
            m_attachmentType = eAT_CharacterBone;
        }
        else
        {
            m_attachmentType = eAT_Pivot;
        }

        xmlNode->getAttr("AttachmentTarget", m_attachmentTarget);

        bool bLoaded = SetClass(entityClass, !ar.bUndo, true, propsNode, props2Node);

        if (ar.bUndo)
        {
            RemoveAllEntityLinks();
            SpawnEntity();
            PostLoad(ar);
        }

        if ((mv_castShadowMinSpec == CONFIG_LOW_SPEC) && !mv_castShadow) // backwards compatibility check
        {
            mv_castShadowMinSpec = END_CONFIG_SPEC_ENUM;
            mv_castShadow = true;
        }
    }
    else
    {
        if (m_attachmentType != eAT_Pivot)
        {
            if (m_attachmentType == eAT_GeomCacheNode)
            {
                xmlNode->setAttr("AttachmentType", "GeomCacheNode");
            }
            else if (m_attachmentType == eAT_CharacterBone)
            {
                xmlNode->setAttr("AttachmentType", "CharacterBone");
            }

            xmlNode->setAttr("AttachmentTarget", m_attachmentTarget.toUtf8().data());
        }

        // Saving.
        if (!m_entityClass.isEmpty() && m_prototype == NULL)
        {
            xmlNode->setAttr("EntityClass", m_entityClass.toUtf8().data());
        }

        if (m_physicsState)
        {
            xmlNode->addChild(m_physicsState);
        }

        //! Save properties.
        if (m_pProperties)
        {
            XmlNodeRef propsNode = xmlNode->newChild("Properties");
            m_pProperties->Serialize(propsNode, ar.bLoading);
        }

        //! Save properties.
        if (m_pProperties2)
        {
            XmlNodeRef propsNode = xmlNode->newChild("Properties2");
            m_pProperties2->Serialize(propsNode, ar.bLoading);
        }

        // Save Event Targets.
        if (!m_eventTargets.empty())
        {
            XmlNodeRef eventTargets = xmlNode->newChild("EventTargets");
            for (int i = 0; i < m_eventTargets.size(); i++)
            {
                CEntityEventTarget& et = m_eventTargets[i];
                GUID targetId = GUID_NULL;
                if (et.target != 0)
                {
                    targetId = et.target->GetId();
                }

                XmlNodeRef eventTarget = eventTargets->newChild("EventTarget");
                eventTarget->setAttr("TargetId", targetId);
                eventTarget->setAttr("Event", et.event.toUtf8().data());
                eventTarget->setAttr("SourceEvent", et.sourceEvent.toUtf8().data());
            }
        }

        // Save Entity Links.
        SaveLink(xmlNode);

        // Save flow graph.
        if (m_pFlowGraph && !m_pFlowGraph->IsEmpty())
        {
            XmlNodeRef graphNode = xmlNode->newChild("FlowGraph");
            m_pFlowGraph->Serialize(graphNode, false, &ar);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::PostLoad(CObjectArchive& ar)
{
    if (m_pEntity)
    {
        // Force entities to register them-self in sectors.
        // force entity to be registered in terrain sectors again.
        XFormGameEntity();
        BindToParent();
        BindIEntityChilds();
        if (m_pEntityScript)
        {
            m_pEntityScript->SetEventsTable(this);
        }
        if (m_physicsState)
        {
            m_pEntity->SetPhysicsState(m_physicsState);
        }

        // Refreshes the Obstruction multiplier on load of level
        m_pEntity->SetObstructionMultiplier(mv_obstructionMultiplier);
    }

    //////////////////////////////////////////////////////////////////////////
    // Load Links.
    XmlNodeRef linksNode = ar.node->findChild("EntityLinks");
    LoadLink(linksNode, &ar);

    //////////////////////////////////////////////////////////////////////////
    // Load flow graph after loading of everything.
    XmlNodeRef graphNode = ar.node->findChild("FlowGraph");
    if (graphNode)
    {
        if (!m_pFlowGraph)
        {
            SetFlowGraph(GetIEditor()->GetFlowGraphManager()->CreateGraphForEntity(this));
        }
        if (m_pFlowGraph && m_pFlowGraph->GetIFlowGraph())
        {
            m_pFlowGraph->ExtractObjectsPrefabIdToGlobalIdMappingFromPrefab(ar);
            m_pFlowGraph->Serialize(graphNode, true, &ar);
        }
    }
    else
    {
        if (m_pFlowGraph)
        {
            SetFlowGraph(nullptr);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::CheckSpecConfig()
{
    if (m_pEntity && m_pEntity->GetAI() && GetMinSpec() != 0)
    {
        CErrorRecord err;
        err.error = tr("AI entity %1 ->> spec dependent").arg(GetName());
        err.pObject = this;
        err.severity = CErrorRecord::ESEVERITY_WARNING;
        GetIEditor()->GetErrorReport()->ReportError(err);
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEntityObject::Export(const QString& levelPath, XmlNodeRef& xmlExportNode)
{
    if (m_bLoadFailed)
    {
        return 0;
    }

    // Do not export entity with bad id.
    if (!m_entityId)
    {
        return XmlHelpers::CreateXmlNode("Temp");
    }

    CheckSpecConfig();

    // Export entities to entities.ini
    XmlNodeRef objNode = xmlExportNode->newChild("Entity");

    objNode->setAttr("Name", GetName().toUtf8().data());

    if (GetMaterial())
    {
        objNode->setAttr("Material", GetMaterial()->GetName().toUtf8().data());
    }

    if (m_prototype)
    {
        objNode->setAttr("Archetype", m_prototype->GetFullName().toUtf8().data());
    }

    Vec3 pos = GetPos(), scale = GetScale();
    Quat rotate = GetRotation();

    if (GetGroup() && qobject_cast<CEntityObject*>(GetGroup()->GetParent()))
    {
        // Store the parent entity id of the group.
        CEntityObject* parentEntity = ( CEntityObject* )(GetGroup()->GetParent());
        if (parentEntity)
        {
            objNode->setAttr("ParentId", parentEntity->GetEntityId());
        }
    }
    else if (GetParent())
    {
        if (qobject_cast<CEntityObject*>(GetParent()))
        {
            // Store parent entity id.
            CEntityObject* parentEntity = ( CEntityObject* )GetParent();
            if (parentEntity)
            {
                objNode->setAttr("ParentId", parentEntity->GetEntityId());
                if (m_attachmentType != eAT_Pivot)
                {
                    if (m_attachmentType == eAT_GeomCacheNode)
                    {
                        objNode->setAttr("AttachmentType", "GeomCacheNode");
                    }
                    else if (m_attachmentType == eAT_CharacterBone)
                    {
                        objNode->setAttr("AttachmentType", "CharacterBone");
                    }

                    objNode->setAttr("AttachmentTarget", m_attachmentTarget.toUtf8().data());
                }
            }
        }
        else
        {
            // Export world coordinates.
            AffineParts ap;
            ap.SpectralDecompose(GetWorldTM());
            pos = ap.pos;
            rotate = ap.rot;
            scale = ap.scale;
        }
    }

    if (!IsEquivalent(pos, Vec3(0, 0, 0), 0))
    {
        objNode->setAttr("Pos", pos);
    }

    if (!rotate.IsIdentity())
    {
        objNode->setAttr("Rotate", rotate);
    }

    if (!IsEquivalent(scale, Vec3(1, 1, 1), 0))
    {
        objNode->setAttr("Scale", scale);
    }

    objNode->setTag("Entity");
    objNode->setAttr("EntityClass", m_entityClass.toUtf8().data());
    objNode->setAttr("EntityId", m_entityId);
    objNode->setAttr("EntityGuid", ToEntityGuid(GetId()));

    if (mv_ratioLOD != 100)
    {
        objNode->setAttr("LodRatio", ( int )mv_ratioLOD);
    }

    if (fabs(mv_viewDistanceMultiplier - 1.f) > FLT_EPSILON)
    {
        objNode->setAttr("ViewDistanceMultiplier", mv_viewDistanceMultiplier);
    }

    objNode->setAttr("CastShadowMinSpec", mv_castShadowMinSpec);

    if (mv_recvWind)
    {
        objNode->setAttr("RecvWind", true);
    }

    if (mv_noDecals)
    {
        objNode->setAttr("NoDecals", true);
    }

    if (mv_outdoor)
    {
        objNode->setAttr("OutdoorOnly", true);
    }

    if (GetMinSpec() != 0)
    {
        objNode->setAttr("MinSpec", ( uint32 )GetMinSpec());
    }

    uint32 nMtlLayersMask = GetMaterialLayersMask();
    if (nMtlLayersMask != 0)
    {
        objNode->setAttr("MatLayersMask", nMtlLayersMask);
    }

    if (mv_hiddenInGame)
    {
        objNode->setAttr("HiddenInGame", true);
    }

    if (mv_createdThroughPool)
    {
        objNode->setAttr("CreatedThroughPool", true);
    }

    if (mv_obstructionMultiplier != 1.f)
    {
        objNode->setAttr("ObstructionMultiplier", (float)mv_obstructionMultiplier);
    }

    if (m_physicsState)
    {
        objNode->addChild(m_physicsState);
    }

    if (!GetLayer()->GetName().isEmpty())
    {
        objNode->setAttr("Layer", GetLayer()->GetName().toUtf8().data());
    }

    // Export Event Targets.
    if (!m_eventTargets.empty())
    {
        XmlNodeRef eventTargets = objNode->newChild("EventTargets");
        for (int i = 0; i < m_eventTargets.size(); i++)
        {
            CEntityEventTarget& et = m_eventTargets[i];

            int entityId = 0;
            EntityGUID entityGuid = 0;
            if (et.target)
            {
                if (qobject_cast<CEntityObject*>(et.target))
                {
                    entityId = (( CEntityObject* )et.target)->GetEntityId();
                    entityGuid = ToEntityGuid(et.target->GetId());
                }
            }

            XmlNodeRef eventTarget = eventTargets->newChild("EventTarget");
            eventTarget->setAttr("Target", entityId);
            eventTarget->setAttr("Event", et.event.toUtf8().data());
            eventTarget->setAttr("SourceEvent", et.sourceEvent.toUtf8().data());
        }
    }

    // Save Entity Links.
    bool bForceDisableCheapLight = false;

    if (!m_links.empty())
    {
        XmlNodeRef linksNode = objNode->newChild("EntityLinks");
        for (int i = 0, num = m_links.size(); i < num; i++)
        {
            if (m_links[i].target)
            {
                XmlNodeRef linkNode = linksNode->newChild("Link");
                linkNode->setAttr("TargetId", m_links[i].target->GetEntityId());
                linkNode->setAttr("Name", m_links[i].name.toUtf8().data());

                if (m_links[i].target->GetType() == OBJTYPE_VOLUMESOLID)
                {
                    bForceDisableCheapLight = true;
                }
            }
        }
    }

    //! Export properties.
    if (m_pProperties)
    {
        XmlNodeRef propsNode = objNode->newChild("Properties");
        m_pProperties->Serialize(propsNode, false);
    }
    //! Export properties.
    if (m_pProperties2)
    {
        XmlNodeRef propsNode = objNode->newChild("Properties2");
        m_pProperties2->Serialize(propsNode, false);
    }

    if (objNode != NULL && m_pEntity != NULL)
    {
        // Export internal entity data.
        m_pEntity->SerializeXML(objNode, false);
    }

    // Save flow graph.
    if (m_pFlowGraph && !m_pFlowGraph->IsEmpty())
    {
        XmlNodeRef graphNode = objNode->newChild("FlowGraph");
        m_pFlowGraph->Serialize(graphNode, false, 0);
    }

    return objNode;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnEvent(ObjectEvent event)
{
    CBaseObject::OnEvent(event);

    switch (event)
    {
    case EVENT_INGAME:
    case EVENT_OUTOFGAME:
        if (m_pEntity)
        {
            if (event == EVENT_INGAME)
            {
                if (!m_bCalcPhysics)
                {
                    m_pEntity->ClearFlags(ENTITY_FLAG_IGNORE_PHYSICS_UPDATE);
                }
                // Entity must be hidden when going to game.
                if (m_bVisible)
                {
                    m_pEntity->Hide(mv_hiddenInGame);
                }
            }
            else if (event == EVENT_OUTOFGAME)
            {
                // Entity must be returned to editor visibility state.
                m_pEntity->Hide(!m_bVisible);
            }
            XFormGameEntity();
            OnRenderFlagsChange(0);

            if (event == EVENT_OUTOFGAME)
            {
                if (!m_bCalcPhysics)
                {
                    m_pEntity->SetFlags(ENTITY_FLAG_IGNORE_PHYSICS_UPDATE);
                }
            }
        }
        break;
    case EVENT_REFRESH:
        if (m_pEntity)
        {
            // force entity to be registered in terrain sectors again.

            //-- little hack to force reregistration of entities
            //<<FIXME>> when issue with registration in editor is resolved
            Vec3 pos = GetPos();
            pos.z += 1.f;
            m_pEntity->SetPos(pos);
            //----------------------------------------------------

            XFormGameEntity();
        }
        break;

    case EVENT_UNLOAD_GEOM:
    case EVENT_UNLOAD_ENTITY:
        if (m_pEntityScript)
        {
            //this is not an error, this is here to ensure the scripts get reloaded on "Reload All Scripts"
            m_pEntityScript->Reload();
            m_pEntityScript = 0;
        }
        if (m_pEntity)
        {
            UnloadScript();
        }
        break;

    case EVENT_RELOAD_ENTITY:
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(this);
        if (m_pEntityScript)
        {
            m_pEntityScript->Reload();
        }
        Reload();
        break;

    case EVENT_RELOAD_GEOM:
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(this);
        Reload();
        break;

    case EVENT_PHYSICS_GETSTATE:
        AcceptPhysicsState();
        break;
    case EVENT_PHYSICS_RESETSTATE:
        ResetPhysicsState();
        break;
    case EVENT_PHYSICS_APPLYSTATE:
        if (m_pEntity && m_physicsState)
        {
            m_pEntity->SetPhysicsState(m_physicsState);
        }
        break;

    case EVENT_FREE_GAME_DATA:
        FreeGameData();
        break;

    case EVENT_CONFIG_SPEC_CHANGE:
    {
        IObjectManager* objMan = GetIEditor()->GetObjectManager();
        if (objMan && objMan->IsLightClass(this))
        {
            OnPropertyChange(NULL);
        }
        OnEntityFlagsChange(NULL);
        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::Reload(bool bReloadScript)
{
    if (!m_bEnableReload)
    {
        return;
    }

    if (!m_pEntityScript || bReloadScript)
    {
        SetClass(m_entityClass, true);
    }
    if (m_pEntity)
    {
        DeleteEntity();
    }

    // check if entityId changed due to reload
    EntityId oldEntityId = m_entityId;

    SpawnEntity();

    //We are checking whether the editor is in game or simulation mode,if true
    //than we are sending a reset event entity with true parameter true
    CGameEngine* gameEngine = GetIEditor()->GetGameEngine();
    if (m_pEntity && (gameEngine->IsInGameMode() || gameEngine->GetSimulationMode()))
    {
        SEntityEvent event;
        event.event = ENTITY_EVENT_RESET;
        event.nParam[0] = (UINT_PTR)1;
        m_pEntity->SendEvent(event);
    }

    // if it changed, kick of a setEntityId event
    EntityId newEntityId = m_entityId;
    if (m_pEntity && newEntityId != oldEntityId)
    {
        gEnv->pFlowSystem->OnEntityIdChanged(FlowEntityId(oldEntityId), FlowEntityId(newEntityId));
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnStatObjReloaded()
{
    // reload the entity.  Too much of it (such as its script) may depend on calls to the bounding box, physics, and other parts
    // of its mesh to do a minimal reload.  We basically need to respawn it here.
    Reload(false);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnCharacterBoundsReset()
{
    if (m_pEntity)
    {
        m_pEntity->InvalidateBounds();
    }
    CalcBBox();
    InvalidateTM(0);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdateVisibility(bool bVisible)
{
    CBaseObject::UpdateVisibility(bVisible);

    bool bVisibleWithSpec = bVisible && !IsHiddenBySpec();
    if (bVisibleWithSpec != m_bVisible)
    {
        m_bVisible = bVisibleWithSpec;
    }

    if (m_pEntity)
    {
        m_pEntity->Hide(!m_bVisible);
    }

    size_t const numChildren = GetChildCount();
    for (size_t i = 0; i < numChildren; ++i)
    {
        CBaseObject* const pChildObject = GetChild(i);
        pChildObject->SetHidden(!m_bVisible);

        if (qobject_cast<CEntityObject*>(pChildObject))
        {
            pChildObject->UpdateVisibility(m_bVisible);
        }
    }
};

//////////////////////////////////////////////////////////////////////////
void CEntityObject::DrawDefault(DisplayContext& dc, const QColor& labelColor)
{
    CBaseObject::DrawDefault(dc, labelColor);

    bool bDisplaySelectionHelper = false;
    if (m_pEntity && CanBeDrawn(dc, bDisplaySelectionHelper))
    {
        const Vec3 wp = m_pEntity->GetWorldPos();

        if (gEnv->pAISystem)
        {
            ISmartObjectManager* pSmartObjectManager = gEnv->pAISystem->GetSmartObjectManager();
            if (!pSmartObjectManager->ValidateSOClassTemplate(m_pEntity))
            {
                DrawLabel(dc, wp, QColor(255, 0, 0), 1.f, 4);
            }
            if (IsSelected() || IsHighlighted())
            {
                pSmartObjectManager->DrawSOClassTemplate(m_pEntity);
            }
        }

        // Draw "ghosted" data around the entity's actual position in simulation mode
        if (GetIEditor()->GetGameEngine()->GetSimulationMode())
        {
            if (bDisplaySelectionHelper)
            {
                DrawSelectionHelper(dc, wp, labelColor, 0.5f);
            }
            else if (!(dc.flags & DISPLAY_HIDENAMES))
            {
                DrawLabel(dc, wp, labelColor, 0.5f);
            }

            DrawTextureIcon(dc, wp, 0.5f);
        }
    }
}

void CEntityObject::DrawTextureIcon(DisplayContext& dc, const Vec3& pos, float alpha)
{
    if (GetTextureIcon() && (gSettings.viewports.bShowSizeBasedIcons || gSettings.viewports.bShowIcons))
    {
        CEntityScript* pEntityScript = GetScript();

        if (pEntityScript)
        {
            SetDrawTextureIconProperties(dc, pos);

            int nIconSizeX = OBJECT_TEXTURE_ICON_SIZEX;
            int nIconSizeY = OBJECT_TEXTURE_ICON_SIZEY;

            if ((QString::compare(pEntityScript->GetName(), "Light", Qt::CaseInsensitive) == 0) && gSettings.viewports.bShowSizeBasedIcons)
            {
                float fRadiusModValue = m_proximityRadius;

                if (m_proximityRadius < 1)
                {
                    fRadiusModValue = 1;
                }

                if (m_proximityRadius > 50)
                {
                    fRadiusModValue = 50;
                }

                // Radius=1 is 0.5x icon size, Radius=50 is 2x icon size.
                fRadiusModValue /= 33.3f;
                fRadiusModValue += 0.5f;

                nIconSizeX *= fRadiusModValue;
                nIconSizeY *= fRadiusModValue;
            }

            dc.DrawTextureLabel(GetTextureIconDrawPos(), nIconSizeX, nIconSizeY, GetTextureIcon(), GetTextureIconFlags());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IsInCameraView(const CCamera& camera)
{
    bool bResult = CBaseObject::IsInCameraView(camera);

    if (!bResult && m_pEntity && GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        AABB bbox;
        m_pEntity->GetWorldBounds(bbox);
        bResult = (camera.IsAABBVisible_F(AABB(bbox.min, bbox.max)));
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
float CEntityObject::GetCameraVisRatio(const CCamera& camera)
{
    float visRatio = CBaseObject::GetCameraVisRatio(camera);

    if (m_pEntity && GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        AABB bbox;
        m_pEntity->GetWorldBounds(bbox);

        float objectHeightSq = max(1.0f, (bbox.max - bbox.min).GetLengthSquared());
        float camdistSq = (bbox.min - camera.GetPosition()).GetLengthSquared();
        if (camdistSq > FLT_EPSILON)
        {
            visRatio = max(visRatio, objectHeightSq / camdistSq);
        }
    }

    return visRatio;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IntersectRectBounds(const AABB& bbox)
{
    bool bResult = CBaseObject::IntersectRectBounds(bbox);

    // Check real entity in simulation mode as well
    if (!bResult && m_pEntity && GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        AABB aabb;
        m_pEntity->GetWorldBounds(aabb);

        bResult = aabb.IsIntersectBox(bbox);
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IntersectRayBounds(const Ray& ray)
{
    bool bResult = CBaseObject::IntersectRayBounds(ray);

    // Check real entity in simulation mode as well
    if (!bResult && m_pEntity && GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        Vec3 tmpPnt;
        AABB aabb;
        m_pEntity->GetWorldBounds(aabb);

        bResult = Intersect::Ray_AABB(ray, aabb, tmpPnt);
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
IVariable* CEntityObject::GetLightVariable(const char* name0) const
{
    if (m_pProperties2)
    {
        IVariable* pLightProperties = m_pProperties2->FindVariable("LightProperties_Base");

        if (pLightProperties)
        {
            for (int i = 0; i < pLightProperties->GetNumVariables(); ++i)
            {
                IVariable* pChild = pLightProperties->GetVariable(i);

                if (pChild == NULL)
                {
                    continue;
                }

                QString name(pChild->GetName());
                if (name == name0)
                {
                    return pChild;
                }
            }
        }
    }

    return m_pProperties ? m_pProperties->FindVariable(name0) : nullptr;
}

//////////////////////////////////////////////////////////////////////////
QString CEntityObject::GetLightAnimation() const
{
    IVariable* pStyleGroup = GetLightVariable("Style");
    if (pStyleGroup)
    {
        for (int i = 0; i < pStyleGroup->GetNumVariables(); ++i)
        {
            IVariable* pChild = pStyleGroup->GetVariable(i);

            if (pChild == NULL)
            {
                continue;
            }

            QString name(pChild->GetName());
            if (name == "lightanimation_LightAnimation")
            {
                QString lightAnimationName;
                pChild->Get(lightAnimationName);
                return lightAnimationName;
            }
        }
    }

    return "";
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::InvalidateTM(int nWhyFlags)
{
    CBaseObject::InvalidateTM(nWhyFlags);

    if (nWhyFlags & eObjectUpdateFlags_RestoreUndo)   // Can skip updating game object when restoring undo.
    {
        return;
    }

    // Make sure components are correctly calculated.
    const Matrix34& tm = GetWorldTM();
    if (m_pEntity)
    {
        int nWhyEntityFlag = 0;
        if (nWhyFlags & eObjectUpdateFlags_Animated)
        {
            nWhyEntityFlag = ENTITY_XFORM_TRACKVIEW;
        }
        else
        {
            nWhyEntityFlag = ENTITY_XFORM_EDITOR;
        }

        if (GetParent() && !m_pEntity->GetParent())
        {
            m_pEntity->SetWorldTM(tm, nWhyEntityFlag);
        }
        else if (GetLookAt())
        {
            m_pEntity->SetWorldTM(tm, nWhyEntityFlag);
        }
        else
        {
            if (nWhyFlags & eObjectUpdateFlags_ParentChanged)
            {
                nWhyEntityFlag |= ENTITY_XFORM_FROM_PARENT;
            }
            m_pEntity->SetPosRotScale(GetPos(), GetRotation(), GetScale(), nWhyEntityFlag);
            // In case scale has changed, we need to recreate the entity to
            // ensure that complex physical setup is correctly re-applied after
            // transformation
            bool bByTrackView = nWhyEntityFlag & ENTITY_XFORM_TRACKVIEW;
            if (bByTrackView == false && nWhyFlags & eObjectUpdateFlags_ScaleChanged)
            {
                Reload(true);
            }
        }
    }

    m_bEntityXfromValid = false;
}

//////////////////////////////////////////////////////////////////////////
//! Attach new child node.
void CEntityObject::OnAttachChild(CBaseObject* pChild)
{
    CUndo::Record(new CUndoAttachEntity(this, true));

    if (qobject_cast<CEntityObject*>(pChild))
    {
        ((CEntityObject*)pChild)->BindToParent();
    }
    else if (qobject_cast<CGroup*>(pChild))
    {
        ((CGroup*)pChild)->BindToParent();
    }
}

// Detach this node from parent.
void CEntityObject::OnDetachThis()
{
    CUndo::Record(new CUndoAttachEntity(this, false));
    UnbindIEntity();
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CEntityObject::GetParentAttachPointWorldTM() const
{
    if (m_attachmentType != eAT_Pivot)
    {
        const IEntity* pEntity = GetIEntity();
        if (pEntity && pEntity->GetParent())
        {
            return pEntity->GetParentAttachPointWorldTM();
        }
    }

    return CBaseObject::GetParentAttachPointWorldTM();
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IsParentAttachmentValid() const
{
    if (m_attachmentType != eAT_Pivot)
    {
        const IEntity* pEntity = GetIEntity();
        if (pEntity)
        {
            return pEntity->IsParentAttachmentValid();
        }
    }

    return CBaseObject::IsParentAttachmentValid();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdateTransform()
{
    if (m_attachmentType == eAT_GeomCacheNode || m_attachmentType == eAT_CharacterBone)
    {
        IEntity* pEntity = GetIEntity();
        if (pEntity && pEntity->IsParentAttachmentValid())
        {
            const Matrix34& entityTM = pEntity->GetLocalTM();
            SetLocalTM(entityTM, eObjectUpdateFlags_Animated);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::BindToParent()
{
    if (!m_pEntity)
    {
        return;
    }

    CBaseObject* parent = GetParent();
    if (parent)
    {
        if (qobject_cast<CEntityObject*>(parent))
        {
            CEntityObject* parentEntity = ( CEntityObject* )parent;

            IEntity* ientParent = parentEntity->GetIEntity();
            if (ientParent)
            {
                XFormGameEntity();
                const int flags = ((m_attachmentType == eAT_GeomCacheNode) ? IEntity::ATTACHMENT_GEOMCACHENODE : 0)
                    | ((m_attachmentType == eAT_CharacterBone) ? IEntity::ATTACHMENT_CHARACTERBONE : 0);
                SChildAttachParams attachParams(flags | IEntity::ATTACHMENT_KEEP_TRANSFORMATION, m_attachmentTarget.toUtf8().data());
                ientParent->AttachChild(m_pEntity, attachParams);
                XFormGameEntity();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::BindIEntityChilds()
{
    if (!m_pEntity)
    {
        return;
    }

    int numChilds = GetChildCount();
    for (int i = 0; i < numChilds; i++)
    {
        CBaseObject* pChild = GetChild(i);
        if (qobject_cast<CEntityObject*>(pChild))
        {
            CEntityObject* pChildEntity = static_cast<CEntityObject*>(pChild);
            IEntity* ientChild = pChildEntity->GetIEntity();

            if (ientChild)
            {
                pChildEntity->XFormGameEntity();
                const int flags = ((pChildEntity->m_attachmentType == eAT_GeomCacheNode) ? IEntity::ATTACHMENT_GEOMCACHENODE : 0)
                    | ((pChildEntity->m_attachmentType == eAT_CharacterBone) ? IEntity::ATTACHMENT_CHARACTERBONE : 0);
                SChildAttachParams attachParams(flags, pChildEntity->m_attachmentTarget.toUtf8().data());
                m_pEntity->AttachChild(ientChild, attachParams);
                pChildEntity->XFormGameEntity();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UnbindIEntity()
{
    if (!m_pEntity)
    {
        return;
    }

    CBaseObject* parent = GetParent();
    if (qobject_cast<CEntityObject*>(parent))
    {
        CEntityObject* parentEntity = ( CEntityObject* )parent;

        IEntity* ientParent = parentEntity->GetIEntity();
        if (ientParent)
        {
            m_pEntity->DetachThis(IEntity::ATTACHMENT_KEEP_TRANSFORMATION);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    CBaseObject::PostClone(pFromObject, ctx);

    CEntityObject* pFromEntity = ( CEntityObject* )pFromObject;
    // Clone event targets.
    if (!pFromEntity->m_eventTargets.empty())
    {
        int numTargets = pFromEntity->m_eventTargets.size();
        for (int i = 0; i < numTargets; i++)
        {
            CEntityEventTarget& et = pFromEntity->m_eventTargets[i];
            CBaseObject* pClonedTarget = ctx.FindClone(et.target);
            if (!pClonedTarget)
            {
                pClonedTarget = et.target;  // If target not cloned, link to original target.
            }

            // Add cloned event.
            AddEventTarget(pClonedTarget, et.event, et.sourceEvent, true);
        }
    }

    // Clone links.
    if (!pFromEntity->m_links.empty())
    {
        int numTargets = pFromEntity->m_links.size();
        for (int i = 0; i < numTargets; i++)
        {
            CEntityLink& et = pFromEntity->m_links[i];
            CBaseObject* pClonedTarget = ctx.FindClone(et.target);
            if (!pClonedTarget)
            {
                pClonedTarget = et.target;  // If target not cloned, link to original target.
            }

            // Add cloned event.
            if (pClonedTarget)
            {
                AddEntityLink(et.name, pClonedTarget->GetId());
            }
            else
            {
                AddEntityLink(et.name, GUID_NULL);
            }
        }
    }


    if (m_pFlowGraph)
    {
        m_pFlowGraph->PostClone(pFromObject, ctx);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ResolveEventTarget(CBaseObject* object, unsigned int index)
{
    // Find target id.
    assert(index >= 0 && index < m_eventTargets.size());
    if (object)
    {
        object->AddEventListener(functor(*this, &CEntityObject::OnEventTargetEvent));
    }
    m_eventTargets[index].target = object;
    if (!m_eventTargets.empty() && m_pEntityScript != 0)
    {
        m_pEntityScript->SetEventsTable(this);
    }

    // Make line gizmo.
    if (!m_eventTargets[index].pLineGizmo && object)
    {
        CLineGizmo* pLineGizmo = new CLineGizmo;
        pLineGizmo->SetObjects(this, object);
        pLineGizmo->SetColor(Vec3(0.8f, 0.4f, 0.4f), Vec3(0.8f, 0.4f, 0.4f));
        pLineGizmo->SetName(m_eventTargets[index].event.toUtf8().data());
        AddGizmo(pLineGizmo);
        m_eventTargets[index].pLineGizmo = pLineGizmo;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RemoveAllEntityLinks()
{
    while (!m_links.empty())
    {
        RemoveEntityLink(m_links.size() - 1);
    }
    m_links.clear();
    SetModified(false);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ReleaseEventTargets()
{
    while (!m_eventTargets.empty())
    {
        RemoveEventTarget(m_eventTargets.size() - 1, false);
    }
    m_eventTargets.clear();
    SetModified(false);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::LoadLink(XmlNodeRef xmlNode, CObjectArchive* pArchive)
{
    RemoveAllEntityLinks();

    if (!xmlNode)
    {
        return;
    }

    QString name;
    GUID targetId;

    for (int i = 0; i < xmlNode->getChildCount(); i++)
    {
        XmlNodeRef linkNode = xmlNode->getChild(i);
        linkNode->getAttr("Name", name);

        if (linkNode->getAttr("TargetId", targetId))
        {
            int version = 0;
            linkNode->getAttr("Version", version);

            GUID newTargetId = pArchive ? pArchive->ResolveID(targetId) : targetId;

            // Backwards compatibility with old bone attachment system
            const char kOldBoneLinkPrefix = '@';
            if (version == 0 && !name.isEmpty() && name[0] == kOldBoneLinkPrefix)
            {
                CBaseObject* pObject = FindObject(newTargetId);
                if (qobject_cast<CEntityObject*>(pObject))
                {
                    CEntityObject* pTargetEntity = static_cast<CEntityObject*>(pObject);

                    Quat relRot(IDENTITY);
                    linkNode->getAttr("RelRot", relRot);
                    Vec3 relPos(IDENTITY);
                    linkNode->getAttr("RelPos", relPos);

                    SetAttachType(eAT_CharacterBone);
                    SetAttachTarget(name.mid(1).toUtf8().data());
                    pTargetEntity->AttachChild(this);

                    SetPos(relPos);
                    SetRotation(relRot);
                }
            }
            else
            {
                AddEntityLink(name, newTargetId);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SaveLink(XmlNodeRef xmlNode)
{
    if (m_links.empty())
    {
        return;
    }

    XmlNodeRef linksNode = xmlNode->newChild("EntityLinks");
    for (int i = 0, num = m_links.size(); i < num; i++)
    {
        XmlNodeRef linkNode = linksNode->newChild("Link");
        linkNode->setAttr("TargetId", m_links[i].targetId);
        linkNode->setAttr("Name", m_links[i].name.toUtf8().data());
        linkNode->setAttr("Version", 1);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnEventTargetEvent(CBaseObject* target, int event)
{
    // When event target is deleted.
    if (event == CBaseObject::ON_DELETE)
    {
        // Find this target in events list and remove.
        int numTargets = m_eventTargets.size();
        for (int i = 0; i < numTargets; i++)
        {
            if (m_eventTargets[i].target == target)
            {
                RemoveEventTarget(i);
                numTargets = m_eventTargets.size();
                i--;
            }
        }
    }
    else if (event == CBaseObject::ON_PREDELETE)
    {
        int numTargets = m_links.size();
        for (int i = 0; i < numTargets; i++)
        {
            if (m_links[i].target == target)
            {
                RemoveEntityLink(i);
                numTargets = m_eventTargets.size();
                i--;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CEntityObject::AddEventTarget(CBaseObject* target, const QString& event, const QString& sourceEvent, bool bUpdateScript)
{
    StoreUndo("Add EventTarget");
    CEntityEventTarget et;
    et.target = target;
    et.event = event;
    et.sourceEvent = sourceEvent;

    // Assign event target.
    if (et.target)
    {
        et.target->AddEventListener(functor(*this, &CEntityObject::OnEventTargetEvent));
    }

    if (target)
    {
        // Make line gizmo.
        CLineGizmo* pLineGizmo = new CLineGizmo;
        pLineGizmo->SetObjects(this, target);
        pLineGizmo->SetColor(Vec3(0.8f, 0.4f, 0.4f), Vec3(0.8f, 0.4f, 0.4f));
        pLineGizmo->SetName(event.toUtf8().data());
        AddGizmo(pLineGizmo);
        et.pLineGizmo = pLineGizmo;
    }

    m_eventTargets.push_back(et);

    // Update event table in script.
    if (bUpdateScript && m_pEntityScript != 0)
    {
        m_pEntityScript->SetEventsTable(this);
    }

    SetModified(false);
    return m_eventTargets.size() - 1;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RemoveEventTarget(int index, bool bUpdateScript)
{
    if (index >= 0 && index < m_eventTargets.size())
    {
        StoreUndo("Remove EventTarget");

        if (m_eventTargets[index].pLineGizmo)
        {
            RemoveGizmo(m_eventTargets[index].pLineGizmo);
        }

        if (m_eventTargets[index].target)
        {
            m_eventTargets[index].target->RemoveEventListener(functor(*this, &CEntityObject::OnEventTargetEvent));
        }
        m_eventTargets.erase(m_eventTargets.begin() + index);

        // Update event table in script.
        if (bUpdateScript && m_pEntityScript != 0 && m_pEntity != 0)
        {
            m_pEntityScript->SetEventsTable(this);
        }

        SetModified(false);
    }
}

//////////////////////////////////////////////////////////////////////////
int CEntityObject::AddEntityLink(const QString& name, GUID targetEntityId)
{
    CEntityObject* target = 0;
    if (targetEntityId != GUID_NULL)
    {
        CBaseObject* pObject = FindObject(targetEntityId);
        if (qobject_cast<CEntityObject*>(pObject))
        {
            target = ( CEntityObject* )pObject;

            // Legacy entities and AZ entities shouldn't be linked.
            if (target->GetType() == OBJTYPE_AZENTITY || GetType() == OBJTYPE_AZENTITY)
            {
                return -1;
            }
        }
    }

    StoreUndo("Add EntityLink");

    CLineGizmo* pLineGizmo = 0;

    // Assign event target.
    if (target)
    {
        target->AddEventListener(functor(*this, &CEntityObject::OnEventTargetEvent));

        // Make line gizmo.
        pLineGizmo = new CLineGizmo;
        pLineGizmo->SetObjects(this, target);
        pLineGizmo->SetColor(Vec3(0.4f, 1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
        pLineGizmo->SetName(name.toUtf8().data());
        AddGizmo(pLineGizmo);
    }

    CEntityLink lnk;
    lnk.targetId = targetEntityId;
    lnk.target = target;
    lnk.name = name;
    lnk.pLineGizmo = pLineGizmo;
    m_links.push_back(lnk);

    if (m_pEntity != NULL && target != NULL)
    {
        // Add link to entity itself.
        m_pEntity->AddEntityLink(name.toUtf8().data(), target->GetEntityId(), 0);
        // tell the target about the linkage
        target->EntityLinked(name, GetId());
    }

    SetModified(false);
    UpdateGroup();

    return m_links.size() - 1;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::EntityLinkExists(const QString& name, GUID targetEntityId)
{
    for (int i = 0, num = m_links.size(); i < num; ++i)
    {
        if (m_links[i].targetId == targetEntityId && name.compare(m_links[i].name, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RemoveEntityLink(int index)
{
    if (index >= 0 && index < m_links.size())
    {
        CEntityLink& link = m_links[index];
        StoreUndo("Remove EntityLink");

        if (link.pLineGizmo)
        {
            RemoveGizmo(link.pLineGizmo);
        }

        if (link.target)
        {
            link.target->RemoveEventListener(functor(*this, &CEntityObject::OnEventTargetEvent));
            link.target->EntityUnlinked(link.name, GetId());
        }
        m_links.erase(m_links.begin() + index);

        UpdateIEntityLinks();

        SetModified(false);
        UpdateGroup();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RenameEntityLink(int index, const QString& newName)
{
    if (index >= 0 && index < m_links.size())
    {
        StoreUndo("Rename EntityLink");

        if (m_links[index].pLineGizmo)
        {
            m_links[index].pLineGizmo->SetName(newName.toUtf8().data());
        }

        m_links[index].name = newName;

        UpdateIEntityLinks();

        SetModified(false);
        UpdateGroup();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdateIEntityLinks(bool bCallOnPropertyChange)
{
    if (m_pEntity)
    {
        m_pEntity->RemoveAllEntityLinks();
        for (int i = 0, num = m_links.size(); i < num; i++)
        {
            if (m_links[i].target)
            {
                m_pEntity->AddEntityLink(m_links[i].name.toUtf8().data(), m_links[i].target->GetEntityId(), 0);
            }
        }

        if (bCallOnPropertyChange)
        {
            m_pEntityScript->CallOnPropertyChange(m_pEntity);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnEntityFlagsChange(IVariable* var)
{
    if (m_pEntity)
    {
        int flags = m_pEntity->GetFlags();

        if (mv_castShadowMinSpec <= gSettings.editorConfigSpec)
        {
            flags |= ENTITY_FLAG_CASTSHADOW;
        }
        else
        {
            flags &= ~ENTITY_FLAG_CASTSHADOW;
        }

        if (mv_outdoor)
        {
            flags |= ENTITY_FLAG_OUTDOORONLY;
        }
        else
        {
            flags &= ~ENTITY_FLAG_OUTDOORONLY;
        }

        if (mv_recvWind)
        {
            flags |= ENTITY_FLAG_RECVWIND;
        }
        else
        {
            flags &= ~ENTITY_FLAG_RECVWIND;
        }

        if (mv_noDecals)
        {
            flags |= ENTITY_FLAG_NO_DECALNODE_DECALS;
        }
        else
        {
            flags &= ~ENTITY_FLAG_NO_DECALNODE_DECALS;
        }

        m_pEntity->SetFlags(flags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnRenderFlagsChange(IVariable* var)
{
    if (m_pEntity)
    {
        IComponentRenderPtr pRenderComponent = m_pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
            if (pRenderNode)
            {
                pRenderNode->SetLodRatio(mv_ratioLOD);

                // With Custom view dist ratio it is set by code not UI.
                if (!(m_pEntity->GetFlags() & ENTITY_FLAG_CUSTOM_VIEWDIST_RATIO))
                {
                    pRenderNode->SetViewDistanceMultiplier(mv_viewDistanceMultiplier);
                }
                else
                {
                    // Disable UI for view distance ratio.
                    mv_viewDistanceMultiplier.SetFlags(mv_viewDistanceMultiplier.GetFlags() | IVariable::UI_DISABLED);
                }

                int nRndFlags = pRenderNode->GetRndFlags();
                if (mv_recvWind)
                {
                    nRndFlags |= ERF_RECVWIND;
                }
                else
                {
                    nRndFlags &= ~ERF_RECVWIND;
                }

                if (mv_noDecals)
                {
                    nRndFlags |= ERF_NO_DECALNODE_DECALS;
                }
                else
                {
                    nRndFlags &= ~ERF_NO_DECALNODE_DECALS;
                }

                pRenderNode->SetRndFlags(nRndFlags);
                pRenderNode->SetMinSpec(GetMinSpec());
            }
            //// Set material layer flags..
            uint8 nMaterialLayersFlags = GetMaterialLayersMask();
            pRenderComponent->SetMaterialLayersMask(nMaterialLayersFlags);
        }

        SetModified(false);
    }
}

void CEntityObject::OnEntityObstructionMultiplierChange(IVariable* var)
{
    if (m_pEntity)
    {
        m_pEntity->SetObstructionMultiplier(mv_obstructionMultiplier);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnRadiusChange(IVariable* var)
{
    var->Get(m_proximityRadius);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnInnerRadiusChange(IVariable* var)
{
    var->Get(m_innerRadius);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnOuterRadiusChange(IVariable* var)
{
    var->Get(m_outerRadius);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxSizeXChange(IVariable* var)
{
    var->Get(m_boxSizeX);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxSizeYChange(IVariable* var)
{
    var->Get(m_boxSizeY);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxSizeZChange(IVariable* var)
{
    var->Get(m_boxSizeZ);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnProjectorFOVChange(IVariable* var)
{
    var->Get(m_projectorFOV);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnProjectInAllDirsChange(IVariable* var)
{
    int value;
    var->Get(value);
    m_bProjectInAllDirs = value;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnProjectorTextureChange(IVariable* var)
{
    QString texture;
    var->Get(texture);
    m_bProjectorHasTexture = !texture.isEmpty();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAreaLightChange(IVariable* var)
{
    int value;
    var->Get(value);
    m_bAreaLight = value;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAreaWidthChange(IVariable* var)
{
    var->Get(m_fAreaWidth);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAreaHeightChange(IVariable* var)
{
    var->Get(m_fAreaHeight);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAreaLightSizeChange(IVariable* var)
{
    var->Get(m_fAreaLightSize);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnColorChange(IVariable* var)
{
    var->Get(m_lightColor);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxProjectionChange(IVariable* var)
{
    int value;
    var->Get(value);
    m_bBoxProjectedCM = value;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxWidthChange(IVariable* var)
{
    var->Get(m_fBoxWidth);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxHeightChange(IVariable* var)
{
    var->Get(m_fBoxHeight);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxLengthChange(IVariable* var)
{
    var->Get(m_fBoxLength);
}

//////////////////////////////////////////////////////////////////////////
CVarBlock* CEntityObject::CloneProperties(CVarBlock* srcProperties)
{
    assert(srcProperties);
    return srcProperties->Clone(true);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnLoadFailed()
{
    m_bLoadFailed = true;

    CErrorRecord err;
    err.error = tr("Entity %1 Failed to Spawn (Script: %2)").arg(GetName(), m_entityClass);
    err.pObject = this;
    GetIEditor()->GetErrorReport()->ReportError(err);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetMaterial(CMaterial* mtl)
{
    CBaseObject::SetMaterial(mtl);
    UpdateMaterialInfo();
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CEntityObject::GetRenderMaterial() const
{
    if (GetMaterial())
    {
        return GetMaterial();
    }
    if (m_pEntity)
    {
        IComponentRenderPtr pRenderComponent = m_pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            return GetIEditor()->GetMaterialManager()->FromIMaterial(pRenderComponent->GetRenderMaterial());
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdateMaterialInfo()
{
    if (m_pEntity)
    {
        if (GetMaterial())
        {
            m_pEntity->SetMaterial(GetMaterial()->GetMatInfo());
        }
        else
        {
            m_pEntity->SetMaterial(0);
        }

        IComponentRenderPtr pRenderComponent = m_pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            // Set material layer flags on entity.
            uint8 nMaterialLayersFlags = GetMaterialLayersMask();
            pRenderComponent->SetMaterialLayersMask(nMaterialLayersFlags);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetMaterialLayersMask(uint32 nLayersMask)
{
    CBaseObject::SetMaterialLayersMask(nLayersMask);
    UpdateMaterialInfo();
}


//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetMinSpec(uint32 nMinSpec, bool bSetChildren)
{
    CBaseObject::SetMinSpec(nMinSpec, bSetChildren);
    OnRenderFlagsChange(0);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::AcceptPhysicsState()
{
    if (m_pEntity)
    {
        StoreUndo("Accept Physics State");

        // [Anton] - StoreUndo sends EVENT_AFTER_LOAD, which forces position and angles to editor's position and
        // angles, which are not updated with the physics value
        SetWorldTM(m_pEntity->GetWorldTM());

        IPhysicalEntity* physic = m_pEntity->GetPhysics();
        if (!physic && m_pEntity->GetCharacter(0))     // for ropes
        {
            physic = m_pEntity->GetCharacter(0)->GetISkeletonPose()->GetCharacterPhysics(0);
        }
        if (physic && (physic->GetType() == PE_ARTICULATED || physic->GetType() == PE_ROPE))
        {
            IXmlSerializer* pSerializer = GetISystem()->GetXmlUtils()->CreateXmlSerializer();
            if (pSerializer)
            {
                m_physicsState = GetISystem()->CreateXmlNode("PhysicsState");
                ISerialize* ser = pSerializer->GetWriter(m_physicsState);
                physic->GetStateSnapshot(ser);
                pSerializer->Release();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ResetPhysicsState()
{
    if (m_pEntity)
    {
        m_physicsState = 0;
        m_pEntity->SetPhysicsState(m_physicsState);
        Reload();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetHelperScale(float scale)
{
    bool bChanged = m_helperScale != scale;
    m_helperScale = scale;
    if (bChanged)
    {
        CalcBBox();
    }
}

//////////////////////////////////////////////////////////////////////////
float CEntityObject::GetHelperScale()
{
    return m_helperScale;
}

//////////////////////////////////////////////////////////////////////////
//! Analyze errors for this object.
void CEntityObject::Validate(IErrorReport* report)
{
    CBaseObject::Validate(report);

    if (!m_pEntity && !m_entityClass.isEmpty())
    {
        CErrorRecord err;
        err.error = tr("Entity %1 Failed to Spawn (Script: %2)").arg(GetName(), m_entityClass);
        err.pObject = this;
        report->ReportError(err);
        return;
    }

    if (!m_pEntity)
    {
        return;
    }

    int slot;

    // Check Entity.
    int numObj = m_pEntity->GetSlotCount();
    for (slot = 0; slot < numObj; slot++)
    {
        IStatObj* pObj = m_pEntity->GetStatObj(slot);
        if (!pObj)
        {
            continue;
        }

        if (pObj->IsDefaultObject())
        {
            // File Not found.
            CErrorRecord err;
            err.error = tr("Geometry File in Slot %1 for Entity %2 not found").arg(slot).arg(GetName());
            err.pObject = this;
            err.flags = CErrorRecord::FLAG_NOFILE;
            report->ReportError(err);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::GatherUsedResources(CUsedResources& resources)
{
    CBaseObject::GatherUsedResources(resources);
    if (m_pProperties)
    {
        m_pProperties->GatherUsedResources(resources);
    }
    if (m_pProperties2)
    {
        m_pProperties2->GatherUsedResources(resources);
    }
    if (m_prototype != NULL && m_prototype->GetProperties())
    {
        m_prototype->GetProperties()->GatherUsedResources(resources);
    }

    if (m_pEntity)
    {
        IComponentRenderPtr pRenderComponent = m_pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial();
            CMaterialManager::GatherResources(pMtl, resources);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IsSimilarObject(CBaseObject* pObject)
{
    if (pObject->GetClassDesc() == GetClassDesc() && pObject->metaObject() == metaObject())
    {
        CEntityObject* pEntity = ( CEntityObject* )pObject;
        if (m_entityClass == pEntity->m_entityClass &&
            m_proximityRadius == pEntity->m_proximityRadius &&
            m_innerRadius == pEntity->m_innerRadius &&
            m_outerRadius == pEntity->m_outerRadius)
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::CreateFlowGraphWithGroupDialog()
{
    if (m_pFlowGraph)
    {
        return false;
    }

    CUndo undo("Create Flow graph");

    CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager();
    std::set<QString> groupsSet;
    pFlowGraphManager->GetAvailableGroups(groupsSet);

    QString groupName;
    bool bDoNewGroup  = true;
    bool bCreateGroup = false;
    bool bDoNewGraph  = false;

    if (groupsSet.size() > 0)
    {
        std::vector<QString> groups;
        groups.push_back("<None>");
        groups.insert (groups.end(), groupsSet.begin(), groupsSet.end());

        CGenericSelectItemDialog gtDlg; // KDAP_PORT need to fix parent
        gtDlg.setWindowTitle(QObject::tr("Choose Group for the Flow Graph"));
        gtDlg.SetItems(groups);
        gtDlg.AllowNew(true);
        switch (gtDlg.exec())
        {
        case QDialog::Accepted:
            groupName = gtDlg.GetSelectedItem();
            bCreateGroup = true;
            bDoNewGroup = false;
            bDoNewGraph = true;
            break;
        case CGenericSelectItemDialog::New:
            bDoNewGroup = true;
            break;
        }
    }

    if (bDoNewGroup)
    {
        StringDlg dlg(QObject::tr("Choose Group for the Flow Graph"));
        dlg.SetString(QObject::tr("<None>"));
        if (dlg.exec() == QDialog::Accepted)
        {
            bCreateGroup = true;
            groupName = dlg.GetString();
        }
    }

    if (bCreateGroup)
    {
        if (groupName == tr("<None>"))
        {
            groupName.clear();
        }
        bDoNewGraph = true;
        OpenFlowGraph(groupName);
        if (m_panel)
        {
            m_panel->SetEntity(this);
        }
    }
    return bDoNewGraph;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetFlowGraph(CFlowGraph* pGraph)
{
    if (pGraph != m_pFlowGraph)
    {
        if (m_pFlowGraph)
        {
            m_pFlowGraph->Release();
        }
        m_pFlowGraph = pGraph;
        if (m_pFlowGraph)
        {
            m_pFlowGraph->SetEntity(this, true);   // true -> re-adjust graph entity nodes
            m_pFlowGraph->AddRef();

            if (m_pEntity)
            {
                IComponentFlowGraphPtr flowGraphComponent = m_pEntity->GetOrCreateComponent<IComponentFlowGraph>();
                flowGraphComponent->SetFlowGraph(m_pFlowGraph->GetIFlowGraph());
            }
        }
        SetModified(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OpenFlowGraph(const QString& groupName)
{
    if (!m_pFlowGraph)
    {
        StoreUndo("Create Flow Graph");
        SetFlowGraph(GetIEditor()->GetFlowGraphManager()->CreateGraphForEntity(this, groupName.toUtf8().data()));
        UpdatePrefab();
    }
    GetIEditor()->GetFlowGraphManager()->OpenView(m_pFlowGraph);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RemoveFlowGraph(bool bInitFlowGraph)
{
    if (m_pFlowGraph)
    {
        StoreUndo("Remove Flow Graph");
        GetIEditor()->GetFlowGraphManager()->UnregisterAndResetView(m_pFlowGraph, bInitFlowGraph);
        m_pFlowGraph->Release();
        m_pFlowGraph = 0;

        if (m_pEntity)
        {
            IComponentFlowGraphPtr pFlowGraphComponent = m_pEntity->GetComponent<IComponentFlowGraph>();
            if (pFlowGraphComponent)
            {
                pFlowGraphComponent->SetFlowGraph(0);
            }
        }

        SetModified(false);
    }
}

//////////////////////////////////////////////////////////////////////////
QString CEntityObject::GetSmartObjectClass() const
{
    QString soClass;
    if (m_pProperties)
    {
        IVariable* pVar = m_pProperties->FindVariable("soclasses_SmartObjectClass");
        if (pVar)
        {
            pVar->Get(soClass);
        }
    }
    return soClass;
}

//////////////////////////////////////////////////////////////////////////
IRenderNode*  CEntityObject::GetEngineNode() const
{
    if (m_pEntity)
    {
        IComponentRenderPtr pRenderComponent = m_pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            return pRenderComponent->GetRenderNode();
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnMenuCreateFlowGraph()
{
    if (CreateFlowGraphWithGroupDialog())
    {
        OpenFlowGraph("");
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnMenuFlowGraphOpen(CFlowGraph* pFlowGraph)
{
    GetIEditor()->GetFlowGraphManager()->OpenView(pFlowGraph);

    CHyperGraphDialog* pHGDlg = CHyperGraphDialog::instance();
    if (pHGDlg)
    {
        CFlowGraphSearchCtrl* pSC = pHGDlg->GetSearchControl();
        if (pSC)
        {
            CFlowGraphSearchOptions* pOpts = CFlowGraphSearchOptions::GetSearchOptions();
            pOpts->m_bIncludeEntities = true;
            pOpts->m_findSpecial = CFlowGraphSearchOptions::eFLS_None;
            pOpts->m_LookinIndex = CFlowGraphSearchOptions::eFL_Current;
            pSC->Find(GetName(), false, true, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnMenuScriptEvent(int eventIndex)
{
    CEntityScript* pScript = GetScript();
    if (pScript && GetIEntity())
    {
        pScript->SendEvent(GetIEntity(), pScript->GetEvent(eventIndex));
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnMenuOpenTrackView(CTrackViewAnimNode* pAnimNode)
{
    QtViewPaneManager::instance()->OpenPane(LyViewPane::TrackView);

    CTrackViewSequence* pSequence = pAnimNode->GetSequence();
    CTrackViewSequenceNotificationContext context(pSequence);
    GetIEditor()->GetAnimation()->SetSequence(pSequence, false, false);
    pSequence->ClearSelection();
    pAnimNode->SetSelected(true);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnMenuReloadScripts()
{
    CEntityScript* pScript = GetScript();
    if (pScript)
    {
        pScript->Reload();
    }
    Reload(true);

    if (gEnv->pEntitySystem)
    {
        SEntityEvent event;
        event.event = ENTITY_EVENT_RELOAD_SCRIPT;
        gEnv->pEntitySystem->SendEventToAll(event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnMenuReloadAllScripts()
{
    CCryEditApp::instance()->OnReloadEntityScripts();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnMenuConvertToPrefab()
{
    const QString& libraryFileName = this->GetEntityPropertyString("filePrefabLibrary");

    IDataBaseLibrary* pLibrary = GetIEditor()->GetPrefabManager()->FindLibrary(libraryFileName);
    if (pLibrary == NULL)
    {
        IDataBaseLibrary* pLibrary = GetIEditor()->GetPrefabManager()->LoadLibrary(libraryFileName);
    }

    if (pLibrary == NULL)
    {
        QString sError = tr("Could not convert procedural object %1 to prefab library %2").arg(this->GetName(), libraryFileName);
        CryMessageBox(sError.toUtf8().data(), "Conversion Failure", MB_OKCANCEL | MB_ICONERROR);
        return;
    }

    GetIEditor()->GetObjectManager()->ClearSelection();

    GetIEditor()->SuspendUndo();
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedEntities);

    QString strFullName = this->GetEntityPropertyString("sPrefabVariation");

    // check if we have the info we need from the script
    IEntity* pEnt = gEnv->pEntitySystem ? gEnv->pEntitySystem->GetEntity(this->GetEntityId()) : nullptr;
    if (pEnt)
    {
        IScriptTable* pScriptTable(pEnt->GetScriptTable());
        if (pScriptTable)
        {
            ScriptAnyValue value;
            if (pScriptTable->GetValueAny("PrefabSourceName", value))
            {
                char* szPrefabName = NULL;
                if (value.CopyTo(szPrefabName))
                {
                    strFullName = QString(szPrefabName);
                }
            }
        }
    }

    // strip the library name if it was added (for example it happens when automatically converting from prefab to procedural object)
    const QString& sLibraryName = pLibrary->GetName();
    int nIdx = 0;
    int nLen = sLibraryName.length();
    int nLen2 = strFullName.length();
    if (nLen2 > nLen && strFullName.startsWith(sLibraryName))
    {
        nIdx = nLen + 1;    // counts the . separating the library names
    }
    // check if the prefab item exists inside the library
    IDataBaseItem* pItem = pLibrary->FindItem(strFullName.mid(nIdx).toUtf8().data());

    if (pItem)
    {
        QString guid = GuidUtil::ToString(pItem->GetGUID());
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->NewObject("Prefab", 0, guid);
        if (!pObject)
        {
            QString sError = tr("Could not convert procedural object to %1").arg(strFullName);
            CryMessageBox(sError.toUtf8().data(), "Conversion Failure", MB_OKCANCEL | MB_ICONERROR);
        }
        else
        {
            pObject->SetLayer(GetLayer());

            GetIEditor()->SelectObject(pObject);

            pObject->SetWorldTM(this->GetWorldTM());
            GetIEditor()->GetObjectManager()->DeleteObject(this);
        }
    }
    else
    {
        QString sError = tr("Library not found %1").arg(sLibraryName);
        CryMessageBox(sError.toUtf8().data(), "Conversion Failure", MB_OKCANCEL | MB_ICONERROR);
    }

    QtViewPaneManager::instance()->OpenPane(LyViewPane::LegacyRollupBar);
    GetIEditor()->SelectRollUpBar(ROLLUP_OBJECTS);

    GetIEditor()->ResumeUndo();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnContextMenu(QMenu* pMenu)
{
    if (!pMenu->isEmpty())
    {
        pMenu->addSeparator();
    }

    std::vector<CFlowGraph*> flowgraphs;
    CFlowGraph* pEntityFG = 0;
    FlowGraphHelpers::FindGraphsForEntity(this, flowgraphs, pEntityFG);

    QAction* action;
    if (GetFlowGraph() == 0)
    {
        action = pMenu->addAction(QObject::tr("Create Flow Graph"));
        QObject::connect(action, &QAction::triggered, this, &CEntityObject::OnMenuCreateFlowGraph);
        pMenu->addSeparator();
    }

    if (!flowgraphs.empty())
    {
        QMenu* fgMenu = pMenu->addMenu(QObject::tr("Flow Graphs"));

        for (auto flowGraph : flowgraphs)
        {
            if (!flowGraph)
            {
                continue;
            }
            QString name;
            FlowGraphHelpers::GetHumanName(flowGraph, name);
            if (flowGraph == pEntityFG)
            {
                name += " <GraphEntity>";

                action = fgMenu->addAction(name);
                QObject::connect(action, &QAction::triggered, this, [this, flowGraph] { OnMenuFlowGraphOpen(flowGraph);
                    });
                if (fgMenu->actions().size() > 1)
                {
                    fgMenu->addSeparator();
                }
            }
            else
            {
                action = fgMenu->addAction(name);
                QObject::connect(action, &QAction::triggered, this, [this, flowGraph] { OnMenuFlowGraphOpen(flowGraph);
                    });
            }
        }
        pMenu->addSeparator();
    }

    // TrackView sequences
    CTrackViewAnimNodeBundle bundle = GetIEditor()->GetSequenceManager()->GetAllRelatedAnimNodes(this);

    if (bundle.GetCount() > 0)
    {
        QMenu* sequenceMenu = pMenu->addMenu(QObject::tr("Track View Sequences"));

        const unsigned int nodeListCount = bundle.GetCount();
        for (unsigned int nodeIndex = 0; nodeIndex < nodeListCount; ++nodeIndex)
        {
            CTrackViewSequence* pSequence = bundle.GetNode(nodeIndex)->GetSequence();

            if (pSequence)
            {
                action = sequenceMenu->addAction(pSequence->GetName());
                auto node = bundle.GetNode(nodeIndex);
                QObject::connect(action, &QAction::triggered, this, [this, node] { OnMenuOpenTrackView(node);
                    });
            }
        }

        pMenu->addSeparator();
    }

    // Events
    CEntityScript* pScript = GetScript();
    if (pScript && pScript->GetEventCount() > 0)
    {
        QMenu* eventMenu = pMenu->addMenu(QObject::tr("Events"));

        int eventCount = pScript->GetEventCount();
        for (int i = 0; i < eventCount; ++i)
        {
            QString sourceEvent = pScript->GetEvent(i);
            action = eventMenu->addAction(sourceEvent);
            QObject::connect(action, &QAction::triggered, this, [this, i] { OnMenuScriptEvent(i);
                });
        }
        pMenu->addSeparator();
    }

    action = pMenu->addAction(QObject::tr("Reload Script"));
    QObject::connect(action, &QAction::triggered, this, &CEntityObject::OnMenuReloadScripts);

    action = pMenu->addAction(QObject::tr("Reload All Scripts"));
    QObject::connect(action, &QAction::triggered, this, &CEntityObject::OnMenuReloadAllScripts);

    if (pScript && pScript->GetClass() && _stricmp(pScript->GetClass()->GetName(), "ProceduralObject") == 0)
    {
        action = pMenu->addAction(QObject::tr("Convert to prefab"));
        QObject::connect(action, &QAction::triggered, this, &CEntityObject::OnMenuConvertToPrefab);
        //pMenu->AddSeparator();
    }
    CBaseObject::OnContextMenu(pMenu);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetFrozen(bool bFrozen)
{
    CBaseObject::SetFrozen(bFrozen);
    if (m_pFlowGraph)
    {
        GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_UPDATE_FROZEN);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IsScalable() const
{
    if (!m_bForceScale)
    {
        return !(m_pEntityScript && (m_pEntityScript->GetFlags() & ENTITY_SCRIPT_ISNOTSCALABLE));
    }
    else
    {
        return true;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IsRotatable() const
{
    return !(m_pEntityScript && (m_pEntityScript->GetFlags() & ENTITY_SCRIPT_ISNOTROTATABLE));
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnMaterialChanged(MaterialChangeFlags change)
{
    if (change & MATERIALCHANGE_SURFACETYPE)
    {
        if (IEntity* pEntity = GetIEntity())
        {
            m_statObjValidator.Validate(pEntity->GetStatObj(ENTITY_SLOT_ACTUAL), GetRenderMaterial(), pEntity->GetPhysics());
        }
        else
        {
            m_statObjValidator.Validate(0, GetRenderMaterial(), 0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QString CEntityObject::GetTooltip() const
{
    return m_statObjValidator.GetDescription();
}

//////////////////////////////////////////////////////////////////////////
IOpticsElementBasePtr CEntityObject::GetOpticsElement()
{
    CDLight* pLight = GetLightProperty();
    if (pLight == NULL)
    {
        return NULL;
    }
    return pLight->GetLensOpticsElement();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetOpticsElement(IOpticsElementBase* pOptics)
{
    CDLight* pLight = GetLightProperty();
    if (pLight == NULL)
    {
        return;
    }
    pLight->SetLensOpticsElement(pOptics);
    if (GetEntityPropertyBool("bFlareEnable") && pOptics)
    {
        CBaseObject::SetMaterial(s_LensFlareMaterialName);
    }
    else
    {
        SetMaterial(NULL);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ApplyOptics(const QString& opticsFullName, IOpticsElementBasePtr pOptics)
{
    if (pOptics == NULL)
    {
        CDLight* pLight = GetLightProperty();
        if (pLight)
        {
            pLight->SetLensOpticsElement(NULL);
        }
        SetFlareName("");
        SetMaterial(NULL);
    }
    else
    {
        int nOpticsIndex(0);
        if (!gEnv->pOpticsManager->Load(opticsFullName.toUtf8().data(), nOpticsIndex))
        {
            IOpticsElementBasePtr pNewOptics = gEnv->pOpticsManager->Create(eFT_Root);
            if (!gEnv->pOpticsManager->AddOptics(pNewOptics, opticsFullName.toUtf8().data(), nOpticsIndex))
            {
                CDLight* pLight = GetLightProperty();
                if (pLight)
                {
                    pLight->SetLensOpticsElement(NULL);
                    SetMaterial(NULL);
                }
                return;
            }
            LensFlareUtil::CopyOptics(pOptics, pNewOptics);
        }
        SetFlareName(opticsFullName);
    }

    UpdatePropertyPanels(false);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetOpticsName(const QString& opticsFullName)
{
    bool bUpdateOpticsProperty = true;

    if (opticsFullName.isEmpty())
    {
        CDLight* pLight = GetLightProperty();
        if (pLight)
        {
            pLight->SetLensOpticsElement(NULL);
        }
        SetFlareName("");
        SetMaterial(NULL);
    }
    else
    {
        if (GetOpticsElement())
        {
            if (gEnv->pOpticsManager->Rename(GetOpticsElement()->GetName(), opticsFullName.toUtf8().data()))
            {
                SetFlareName(opticsFullName);
            }
        }
    }

    UpdatePropertyPanels(false);
}

//////////////////////////////////////////////////////////////////////////
CDLight* CEntityObject::GetLightProperty() const
{
    const PodArray<ILightSource*>* pLightEntities = GetIEditor()->Get3DEngine()->GetLightEntities();
    if (pLightEntities == NULL)
    {
        return NULL;
    }
    for (int i = 0, iLightSize(pLightEntities->Count()); i < iLightSize; ++i)
    {
        ILightSource* pLightSource = pLightEntities->GetAt(i);
        if (pLightSource == NULL)
        {
            continue;
        }
        CDLight& lightProperty = pLightSource->GetLightProperties();
        if (GetName() != lightProperty.m_sName)
        {
            continue;
        }
        return &lightProperty;
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::GetValidFlareName(QString& outFlareName) const
{
    IVariable* pFlareVar(m_pProperties->FindVariable(s_LensFlarePropertyName));
    if (!pFlareVar)
    {
        return false;
    }

    QString flareName;
    pFlareVar->Get(flareName);
    if (flareName.isEmpty() || flareName == "@root")
    {
        return false;
    }

    outFlareName = flareName;

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::PreInitLightProperty()
{
    if (!IsLight() || !m_pProperties)
    {
        return;
    }

    QString flareFullName;
    if (GetValidFlareName(flareFullName))
    {
        bool bEnableOptics = GetEntityPropertyBool("bFlareEnable");
        if (bEnableOptics)
        {
            CLensFlareManager* pLensManager = GetIEditor()->GetLensFlareManager();
            CLensFlareLibrary* pLevelLib = (CLensFlareLibrary*)pLensManager->GetLevelLibrary();
            IOpticsElementBasePtr pLevelOptics = pLevelLib->GetOpticsOfItem(flareFullName.toUtf8().data());
            if (pLevelLib && pLevelOptics)
            {
                int nOpticsIndex(0);
                IOpticsElementBasePtr pNewOptics = GetOpticsElement();
                if (pNewOptics == NULL)
                {
                    pNewOptics = gEnv->pOpticsManager->Create(eFT_Root);
                }

                if (gEnv->pOpticsManager->AddOptics(pNewOptics, flareFullName.toUtf8().data(), nOpticsIndex))
                {
                    LensFlareUtil::CopyOptics(pLevelOptics, pNewOptics);
                    SetOpticsElement(pNewOptics);
                }
                else
                {
                    CDLight* pLight = GetLightProperty();
                    if (pLight)
                    {
                        pLight->SetLensOpticsElement(NULL);
                        SetMaterial(NULL);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdateLightProperty()
{
    if (!IsLight() || !m_pProperties)
    {
        return;
    }

    QString flareName;
    if (GetValidFlareName(flareName))
    {
        IOpticsElementBasePtr pOptics = GetOpticsElement();
        if (pOptics == NULL)
        {
            pOptics = gEnv->pOpticsManager->Create(eFT_Root);
        }
        bool bEnableOptics = GetEntityPropertyBool("bFlareEnable");
        if (bEnableOptics && GetIEditor()->GetLensFlareManager()->LoadFlareItemByName(flareName, pOptics))
        {
            pOptics->SetName(flareName.toUtf8().data());
            SetOpticsElement(pOptics);
        }
        else
        {
            SetOpticsElement(NULL);
        }
    }

    IScriptTable* pScriptTable = m_pEntity->GetScriptTable();
    if (pScriptTable && GetEntityPropertyBool("bActive"))
    {
        HSCRIPTFUNCTION activateLightFunction;
        if (pScriptTable->GetValue("ActivateLight", activateLightFunction))
        {
            Script::CallMethod(pScriptTable, activateLightFunction, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetPropertyPanelsState()
{
    if (s_pPropertyPanelEntityObject != this)
    {
        return;
    }

    if (s_pPropertiesPanel)
    {
        s_pPropertiesPanel->InvalidateCtrl();

        RenameUIPage(s_propertiesPanelIndex, (GetTypeName() + " Properties").toUtf8().data());
    }

    if (s_pPropertiesPanel2)
    {
        s_pPropertiesPanel2->InvalidateCtrl();

        RenameUIPage(s_propertiesPanelIndex2, (GetTypeName() + " Properties2").toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::CopyPropertiesToScriptTables()
{
    if (m_pProperties && !m_prototype)
    {
        m_pEntityScript->CopyPropertiesToScriptTable(m_pEntity, m_pProperties, false);
    }
    if (m_pProperties2)
    {
        m_pEntityScript->CopyProperties2ToScriptTable(m_pEntity, m_pProperties2, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ForceVariableUpdate()
{
    if (m_pProperties && !m_prototype)
    {
        m_pProperties->OnSetValues();
    }
    if (m_pProperties2)
    {
        m_pProperties2->OnSetValues();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ResetCallbacks()
{
    ClearCallbacks();

    CVarBlock* pProperties = m_pProperties;
    CVarBlock* pProperties2 = m_pProperties2;

    if (pProperties)
    {
        m_callbacks.reserve(6);

        //@FIXME Hack to display radii of properties.
        // wires properties from param block, to this entity internal variables.
        IVariable* var = 0;
        var = pProperties->FindVariable("Radius", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_proximityRadius);
            SetVariableCallback(var, functor(*this, &CEntityObject::OnRadiusChange));
        }
        else
        {
            var = pProperties->FindVariable("radius", false);
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_proximityRadius);
                SetVariableCallback(var, functor(*this, &CEntityObject::OnRadiusChange));
            }
        }

        var = pProperties->FindVariable("InnerRadius", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_innerRadius);
            SetVariableCallback(var, functor(*this, &CEntityObject::OnInnerRadiusChange));
        }
        var = pProperties->FindVariable("OuterRadius", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_outerRadius);
            SetVariableCallback(var, functor(*this, &CEntityObject::OnOuterRadiusChange));
        }

        var = pProperties->FindVariable("BoxSizeX", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_boxSizeX);
            SetVariableCallback(var, functor(*this, &CEntityObject::OnBoxSizeXChange));
        }
        var = pProperties->FindVariable("BoxSizeY", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_boxSizeY);
            SetVariableCallback(var, functor(*this, &CEntityObject::OnBoxSizeYChange));
        }
        var = pProperties->FindVariable("BoxSizeZ", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_boxSizeZ);
            SetVariableCallback(var, functor(*this, &CEntityObject::OnBoxSizeZChange));
        }

        var = pProperties->FindVariable("fAttenuationBulbSize");
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_fAreaLightSize);
            SetVariableCallback(var, functor(*this, &CEntityObject::OnAreaLightSizeChange));
        }

        IVariable* pProjector = pProperties->FindVariable("Projector");
        if (pProjector)
        {
            var = pProjector->FindVariable("fProjectorFov");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_projectorFOV);
                SetVariableCallback(var, functor(*this, &CEntityObject::OnProjectorFOVChange));
            }
            var = pProjector->FindVariable("bProjectInAllDirs");
            if (var && var->GetType() == IVariable::BOOL)
            {
                int value;
                var->Get(value);
                m_bProjectInAllDirs = value;
                SetVariableCallback(var, functor(*this, &CEntityObject::OnProjectInAllDirsChange));
            }
            var = pProjector->FindVariable("texture_Texture");
            if (var && var->GetType() == IVariable::STRING)
            {
                QString projectorTexture;
                var->Get(projectorTexture);
                m_bProjectorHasTexture = !projectorTexture.isEmpty();
                SetVariableCallback(var, functor(*this, &CEntityObject::OnProjectorTextureChange));
            }
        }

        IVariable* pColorGroup = pProperties->FindVariable("Color", false);
        if (pColorGroup)
        {
            const int nChildCount = pColorGroup->GetNumVariables();
            for (int i = 0; i < nChildCount; ++i)
            {
                IVariable* pChild = pColorGroup->GetVariable(i);
                if (!pChild)
                {
                    continue;
                }

                QString name(pChild->GetName());
                if (name == "clrDiffuse")
                {
                    pChild->Get(m_lightColor);
                    SetVariableCallback(pChild, functor(*this, &CEntityObject::OnColorChange));
                    break;
                }
            }
        }

        IVariable* pType = pProperties->FindVariable("Shape");
        if (pType)
        {
            var = pType->FindVariable("bAreaLight");
            if (var && var->GetType() == IVariable::BOOL)
            {
                int value;
                var->Get(value);
                m_bAreaLight = value;
                SetVariableCallback(var, functor(*this, &CEntityObject::OnAreaLightChange));
            }
            var = pType->FindVariable("fPlaneWidth");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fAreaWidth);
                SetVariableCallback(var, functor(*this, &CEntityObject::OnAreaWidthChange));
            }
            var = pType->FindVariable("fPlaneHeight");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fAreaHeight);
                SetVariableCallback(var, functor(*this, &CEntityObject::OnAreaHeightChange));
            }
        }

        IVariable* pProjection = pProperties->FindVariable("Projection");
        if (pProjection)
        {
            var = pProjection->FindVariable("bBoxProject");
            if (var && var->GetType() == IVariable::BOOL)
            {
                int value;
                var->Get(value);
                m_bBoxProjectedCM = value;
                SetVariableCallback(var, functor(*this, &CEntityObject::OnBoxProjectionChange));
            }
            var = pProjection->FindVariable("fBoxWidth");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fBoxWidth);
                SetVariableCallback(var, functor(*this, &CEntityObject::OnBoxWidthChange));
            }
            var = pProjection->FindVariable("fBoxHeight");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fBoxHeight);
                SetVariableCallback(var, functor(*this, &CEntityObject::OnBoxHeightChange));
            }
            var = pProjection->FindVariable("fBoxLength");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fBoxLength);
                SetVariableCallback(var, functor(*this, &CEntityObject::OnBoxLengthChange));
            }
        }

        // Each property must have callback to our OnPropertyChange.
        pProperties->AddOnSetCallback(functor(*this, &CEntityObject::OnPropertyChange));
    }

    if (pProperties2)
    {
        pProperties2->AddOnSetCallback(functor(*this, &CEntityObject::OnPropertyChange));
    }
}

void CEntityObject::InstallStatObjListeners()
{
    StatObjEventBus::MultiHandler::BusDisconnect();
    if (m_pEntity)
    {
        int numObj = m_pEntity->GetSlotCount();
        for (int slot = 0; slot < numObj; slot++)
        {
            IStatObj* pObj = m_pEntity->GetStatObj(slot);
            if (!pObj)
            {
                continue;
            }
            StatObjEventBus::MultiHandler::BusConnect(pObj);
        }
    }
}

void CEntityObject::InstallCharacterBoundsListeners()
{
    AZ::CharacterBoundsNotificationBus::Handler::BusDisconnect();
    if (m_pEntity)
    {
        int count = m_pEntity->GetSlotCount();
        for (int i = 0; i < count; i++)
        {
            ICharacterInstance* character = m_pEntity->GetCharacter(i);
            AZ::CharacterBoundsNotificationBus::Handler::BusConnect(character);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetVariableCallback(IVariable* pVar, IVariable::OnSetCallback func)
{
    pVar->AddOnSetCallback(func);
    m_callbacks.push_back(std::make_pair(pVar, func));
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ClearCallbacks()
{
    if (m_pProperties)
    {
        m_pProperties->RemoveOnSetCallback(functor(*this, &CEntityObject::OnPropertyChange));
    }

    if (m_pProperties2)
    {
        m_pProperties2->RemoveOnSetCallback(functor(*this, &CEntityObject::OnPropertyChange));
    }

    for (auto iter = m_callbacks.begin(); iter != m_callbacks.end(); ++iter)
    {
        iter->first->RemoveOnSetCallback(iter->second);
    }

    m_callbacks.clear();
}

void CEntityObject::StoreUndoEntityLink(CSelectionGroup* pGroup)
{
    if (!pGroup)
    {
        return;
    }

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoEntityLink(pGroup));
    }
}

IStatObj* CEntityObject::GetIStatObj()
{
    if (m_pEntity == NULL)
    {
        return NULL;
    }

    for (int i = 0, iSlotCount(m_pEntity->GetSlotCount()); i < iSlotCount; ++i)
    {
        if (!m_pEntity->IsSlotValid(i))
        {
            continue;
        }
        IStatObj* pStatObj = m_pEntity->GetStatObj(i);
        if (pStatObj == NULL)
        {
            continue;
        }
        return pStatObj;
    }

    return NULL;
}

void CEntityObject::RegisterListener(IEntityObjectListener* pListener)
{
    m_listeners.Add(pListener);
}

void CEntityObject::UnregisterListener(IEntityObjectListener* pListener)
{
    m_listeners.Remove(pListener);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnEntityEvent(IEntity* pEntity, SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_SCRIPT_PROPERTY_ANIMATED:
        // update UI
        // optimization: only sync the var block if this entity is shown in the entity propery panel -

        if (m_panel && m_panel->IsSetTo(this))
        {
            if (m_pProperties && !m_prototype)
            {
                m_pEntityScript->CopyPropertiesFromScriptTable(m_pEntity, m_pProperties);
            }
            if (m_pProperties2)
            {
                m_pEntityScript->CopyProperties2FromScriptTable(m_pEntity, m_pProperties2);
            }
            UpdatePropertyPanels(false);
        }
        break;
    default:
        break;
    }
}

template <typename T>
T CEntityObject::GetEntityProperty(const char* pName, T defaultvalue) const
{
    CVarBlock* pProperties = GetProperties2();
    IVariable* pVariable = NULL;
    if (pProperties)
    {
        pVariable = pProperties->FindVariable(pName);
    }

    if (!pVariable)
    {
        pProperties = GetProperties();
        if (pProperties)
        {
            pVariable = pProperties->FindVariable(pName);
        }

        if (!pVariable)
        {
            return defaultvalue;
        }
    }

    if (pVariable->GetType() != IVariableType<T>::value)
    {
        return defaultvalue;
    }

    T value;
    pVariable->Get(value);
    return value;
}

template <typename T>
void CEntityObject::SetEntityProperty(const char* pName, T value)
{
    CVarBlock* pProperties = GetProperties2();
    IVariable* pVariable = NULL;
    if (pProperties)
    {
        pVariable = pProperties->FindVariable(pName);
    }

    if (!pVariable)
    {
        pProperties = GetProperties();
        if (pProperties)
        {
            pVariable = pProperties->FindVariable(pName);
        }

        if (!pVariable)
        {
            throw std::runtime_error((QString("\"") + pName + "\" is an invalid property.").toUtf8().data());
        }
    }

    if (pVariable->GetType() != IVariableType<T>::value)
    {
        throw std::logic_error("Data type is invalid.");
    }
    pVariable->Set(value);
}

bool CEntityObject::GetEntityPropertyBool(const char* name) const
{
    return GetEntityProperty<bool>(name, false);
}

int CEntityObject::GetEntityPropertyInteger(const char* name) const
{
    return GetEntityProperty<int>(name, 0);
}

float CEntityObject::GetEntityPropertyFloat(const char* name) const
{
    return GetEntityProperty<float>(name, 0.0f);
}

//////////////////////////////////////////////////////////////////////////
QString CEntityObject::GetMouseOverStatisticsText() const
{
    if (m_pEntity)
    {
        QString statsText;
        QString lodText;

        const unsigned int slotCount = m_pEntity->GetSlotCount();
        for (unsigned int slot = 0; slot < slotCount; ++slot)
        {
            IStatObj* pStatObj = m_pEntity->GetStatObj(slot);

            if (pStatObj)
            {
                IStatObj::SStatistics stats;
                pStatObj->GetStatistics(stats);

                for (int i = 0; i < MAX_STATOBJ_LODS_NUM; ++i)
                {
                    if (stats.nIndicesPerLod[i] > 0)
                    {
                        lodText = tr("\n  Slot%1, LOD%2:   ").arg(slot).arg(i);
                        statsText = statsText + lodText +
                            FormatWithThousandsSeperator(stats.nIndicesPerLod[i] / 3) + " Tris,   "
                            + FormatWithThousandsSeperator(stats.nVerticesPerLod[i]) + " Verts";
                    }
                }
            }

            IGeomCacheRenderNode* pGeomCacheRenderNode = m_pEntity->GetGeomCacheRenderNode(slot);

            if (pGeomCacheRenderNode)
            {
                IGeomCache* pGeomCache = pGeomCacheRenderNode->GetGeomCache();
                if (pGeomCache)
                {
                    IGeomCache::SStatistics stats = pGeomCache->GetStatistics();

                    QString averageDataRate;
                    FormatFloatForUI(averageDataRate, 2, stats.m_averageAnimationDataRate);

                    statsText += "\n  " + (stats.m_bPlaybackFromMemory ? QString("Playback from Memory") : QString("Playback from Disk"));
                    statsText += "\n  " + averageDataRate + " MiB/s Average Animation Data Rate";
                    statsText += "\n  " + FormatWithThousandsSeperator(stats.m_numStaticTriangles) + " Static Tris, "
                        + FormatWithThousandsSeperator(stats.m_numStaticVertices) + " Static Verts";
                    statsText += "\n  " + FormatWithThousandsSeperator(stats.m_numAnimatedTriangles) + " Animated Tris, "
                        + FormatWithThousandsSeperator(stats.m_numAnimatedVertices) + " Animated Verts";
                    statsText += "\n  " + FormatWithThousandsSeperator(stats.m_numMaterials) + " Materials";
                    statsText += "\n  " + FormatWithThousandsSeperator(stats.m_numAnimatedMeshes) + " Animated Meshes";
                    statsText += "\n  " + FormatWithThousandsSeperator(stats.m_numStaticMeshes) + " Static Meshes";
                    statsText += "\n  " + FormatWithThousandsSeperator(stats.m_staticDataSize) + " Bytes Memory Static Data";
                    statsText += "\n  " + FormatWithThousandsSeperator(stats.m_diskAnimationDataSize) + " Bytes Disk Animation Data";
                    statsText += "\n  " + FormatWithThousandsSeperator(stats.m_memoryAnimationDataSize) + " Bytes Memory Animation Data";
                }
            }

            ICharacterInstance* pCharacter = m_pEntity->GetCharacter(slot);
            if (pCharacter)
            {
                IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager();
                if (pAttachmentManager)
                {
                    const int32 count = pAttachmentManager->GetAttachmentCount();
                    for (int32 i = 0; i < count; ++i)
                    {
                        if (IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(i))
                        {
                            IAttachmentSkin* pAttachmentSkin = pAttachment->GetIAttachmentSkin();
                            if (pAttachmentSkin)
                            {
                                ISkin* pSkin = pAttachmentSkin->GetISkin();
                                if (pSkin)
                                {
                                    const uint32 numLods = pSkin->GetNumLODs();
                                    const char* szFilename = CryStringUtils::FindFileNameInPath(pSkin->GetModelFilePath());

                                    statsText += "\n  " + QString(szFilename);
                                    for (int j = 0; j < numLods; ++j)
                                    {
                                        IRenderMesh* pRenderMesh = pSkin->GetIRenderMesh(j);
                                        if (pRenderMesh)
                                        {
                                            const int32 vertCount = pRenderMesh->GetVerticesCount();
                                            statsText += ", LOD" + FormatWithThousandsSeperator(j) + ": " + FormatWithThousandsSeperator(vertCount) + " Vertices";
                                        }
                                    }

                                    const uint32 vertexFrameCount = pSkin->GetVertexFrames()->GetCount();
                                    statsText += ", " + FormatWithThousandsSeperator(vertexFrameCount) + " Vertex Frames";
                                }
                            }
                        }
                    }
                }
            }
        }

        return statsText;
    }

    return "";
}

QString CEntityObject::GetEntityPropertyString(const char* name) const
{
    return GetEntityProperty<QString>(name, "");
}

void CEntityObject::SetEntityPropertyBool(const char* name, bool value)
{
    SetEntityProperty<bool>(name, value);
}

void CEntityObject::SetEntityPropertyInteger(const char* name, int value)
{
    SetEntityProperty<int>(name, value);
}

void CEntityObject::SetEntityPropertyFloat(const char* name, float value)
{
    SetEntityProperty<float>(name, value);
}

void CEntityObject::SetEntityPropertyString(const char* name, const QString& value)
{
    SetEntityProperty<QString>(name, value);
}

SPyWrappedProperty CEntityObject::PyGetEntityProperty(const char* pName) const
{
    CVarBlock* pProperties = GetProperties2();
    IVariable* pVariable = NULL;
    if (pProperties)
    {
        pVariable = pProperties->FindVariable(pName, true, true);
    }

    if (!pVariable)
    {
        pProperties = GetProperties();
        if (pProperties)
        {
            pVariable = pProperties->FindVariable(pName, true, true);
        }

        if (!pVariable)
        {
            throw std::runtime_error((QString("\"") + pName + "\" is an invalid property.").toUtf8().data());
        }
    }

    SPyWrappedProperty value;
    if (pVariable->GetType() == IVariable::BOOL)
    {
        value.type =  SPyWrappedProperty::eType_Bool;
        pVariable->Get(value.property.boolValue);
        return value;
    }
    else if (pVariable->GetType() == IVariable::INT)
    {
        value.type = SPyWrappedProperty::eType_Int;
        pVariable->Get(value.property.intValue);
        return value;
    }
    else if (pVariable->GetType() == IVariable::FLOAT)
    {
        value.type = SPyWrappedProperty::eType_Float;
        pVariable->Get(value.property.floatValue);
        return value;
    }
    else if (pVariable->GetType() == IVariable::STRING)
    {
        value.type = SPyWrappedProperty::eType_String;
        pVariable->Get(value.stringValue);
        return value;
    }
    else if (pVariable->GetType() == IVariable::VECTOR)
    {
        Vec3 tempVec3;
        pVariable->Get(tempVec3);

        if (pVariable->GetDataType() == IVariable::DT_COLOR)
        {
            value.type = SPyWrappedProperty::eType_Color;
            QColor col = ColorLinearToGamma(ColorF(
                        tempVec3[0],
                        tempVec3[1],
                        tempVec3[2]));
            value.property.colorValue.r = col.red();
            value.property.colorValue.g = col.green();
            value.property.colorValue.b = col.blue();
        }
        else
        {
            value.type = SPyWrappedProperty::eType_Vec3;
            value.property.vecValue.x = tempVec3[0];
            value.property.vecValue.y = tempVec3[1];
            value.property.vecValue.z = tempVec3[2];
        }

        return value;
    }

    throw std::runtime_error("Data type is invalid.");
}

void CEntityObject::PySetEntityProperty(const char* pName, const SPyWrappedProperty& value)
{
    CVarBlock* pProperties = GetProperties2();
    IVariable* pVariable = NULL;
    if (pProperties)
    {
        pVariable = pProperties->FindVariable(pName, true, true);
    }

    if (!pVariable)
    {
        pProperties = GetProperties();
        if (pProperties)
        {
            pVariable = pProperties->FindVariable(pName, true, true);
        }

        if (!pVariable)
        {
            throw std::runtime_error((QString("\"") + pName + "\" is an invalid property.").toUtf8().data());
        }
    }

    if (value.type == SPyWrappedProperty::eType_Bool)
    {
        pVariable->Set(value.property.boolValue);
    }
    else if (value.type == SPyWrappedProperty::eType_Int)
    {
        pVariable->Set(value.property.intValue);
    }
    else if (value.type == SPyWrappedProperty::eType_Float)
    {
        pVariable->Set(value.property.floatValue);
    }
    else if (value.type == SPyWrappedProperty::eType_String)
    {
        pVariable->Set(value.stringValue);
    }
    else if (value.type == SPyWrappedProperty::eType_Vec3)
    {
        pVariable->Set(Vec3(value.property.vecValue.x, value.property.vecValue.y, value.property.vecValue.z));
    }
    else if (value.type == SPyWrappedProperty::eType_Color)
    {
        pVariable->Set(Vec3(value.property.colorValue.r, value.property.colorValue.g, value.property.colorValue.b));
    }
    else
    {
        throw std::runtime_error("Data type is invalid.");
    }

    UpdatePropertyPanels(true);
}

SPyWrappedProperty PyGetEntityProperty(const char* pObjName, const char* pPropName)
{
    CBaseObject* pObject;
    if (GetIEditor()->GetObjectManager()->FindObject(pObjName))
    {
        pObject = GetIEditor()->GetObjectManager()->FindObject(pObjName);
    }
    else if (GetIEditor()->GetObjectManager()->FindObject(GuidUtil::FromString(pObjName)))
    {
        pObject = GetIEditor()->GetObjectManager()->FindObject(GuidUtil::FromString(pObjName));
    }
    else
    {
        throw std::logic_error((QString("\"") + pObjName + "\" is an invalid object.").toUtf8().data());
    }

    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntityObject = static_cast<CEntityObject*>(pObject);
        return pEntityObject->PyGetEntityProperty(pPropName);
    }
    else
    {
        throw std::logic_error((QString("\"") + pObjName + "\" is an invalid entity.").toUtf8().data());
    }
}

pSPyWrappedProperty PyGetEntityPropertyUsingSharedPtr(const char* pObjName, const char* pPropName)
{
    SPyWrappedProperty property = PyGetEntityProperty(pObjName, pPropName);
    return boost::make_shared<SPyWrappedProperty>(property);
}


void PySetEntityProperty(const char* entityName, const char* propName, SPyWrappedProperty value)
{
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(entityName);
    if (!pObject || !qobject_cast<CEntityObject*>(pObject))
    {
        throw std::logic_error((QString("\"") + entityName + "\" is an invalid entity.").toUtf8().data());
    }

    CUndo undo("Set Entity Property");
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoEntityProperty(entityName, propName));
    }

    CEntityObject* pEntityObject = static_cast<CEntityObject*>(pObject);
    pEntityObject->PySetEntityProperty(propName, value);
}

void PySetEntityPropertyUsingSharedPtr(const char* entityName, const char* propName, pSPyWrappedProperty sharedPtr)
{
    PySetEntityProperty(entityName, propName, *sharedPtr);
}

SPyWrappedProperty PyGetObjectVariableRec(IVariableContainer* pVariableContainer, std::deque<QString>& path)
{
    SPyWrappedProperty value;
    QString currentName = path.back();
    QString currentPath = path.front();

    IVariableContainer* pSubVariableContainer = pVariableContainer;
    if (path.size() != 1)
    {
        pSubVariableContainer = pSubVariableContainer->FindVariable(currentPath.toUtf8().data(), false, true);
        if (!pSubVariableContainer)
        {
            throw std::logic_error("Path is invalid.");
        }
        path.pop_front();
        PyGetObjectVariableRec(pSubVariableContainer, path);
    }

    IVariable* pVariable = pSubVariableContainer->FindVariable(currentName.toUtf8().data(), false, true);

    if (pVariable)
    {
        if (pVariable->GetType() == IVariable::BOOL)
        {
            value.type = SPyWrappedProperty::eType_Bool;
            pVariable->Get(value.property.boolValue);
            return value;
        }
        else if (pVariable->GetType() == IVariable::INT)
        {
            value.type = SPyWrappedProperty::eType_Int;
            pVariable->Get(value.property.intValue);
            return value;
        }
        else if (pVariable->GetType() == IVariable::FLOAT)
        {
            value.type = SPyWrappedProperty::eType_Float;
            pVariable->Get(value.property.floatValue);
            return value;
        }
        else if (pVariable->GetType() == IVariable::STRING)
        {
            value.type = SPyWrappedProperty::eType_String;
            pVariable->Get(value.stringValue);
            return value;
        }
        else if (pVariable->GetType() == IVariable::VECTOR)
        {
            value.type = SPyWrappedProperty::eType_Vec3;
            Vec3 tempVec3;
            pVariable->Get(tempVec3);
            value.property.vecValue.x = tempVec3[0];
            value.property.vecValue.y = tempVec3[1];
            value.property.vecValue.z = tempVec3[2];
            return value;
        }

        throw std::logic_error("Data type is invalid.");
    }

    throw std::logic_error((QString("\"") + currentName + "\" is an invalid parameter.").toUtf8().data());
}

SPyWrappedProperty PyGetEntityParam(const char* pObjName, const char* pVarPath)
{
    SPyWrappedProperty result;
    QString varPath = pVarPath;
    std::deque<QString> splittedPath;
    for (auto token : varPath.split(QRegularExpression(QStringLiteral(R"([\\/])")), QString::SkipEmptyParts))
    {
        splittedPath.push_back(token);
    }

    CBaseObject* pObject;
    if (GetIEditor()->GetObjectManager()->FindObject(pObjName))
    {
        pObject = GetIEditor()->GetObjectManager()->FindObject(pObjName);
    }
    else if (GetIEditor()->GetObjectManager()->FindObject(GuidUtil::FromString(pObjName)))
    {
        pObject = GetIEditor()->GetObjectManager()->FindObject(GuidUtil::FromString(pObjName));
    }
    else
    {
        throw std::logic_error((QString("\"") + pObjName + "\" is an invalid object.").toUtf8().data());
        return result;
    }

    if (qobject_cast<CEntityObject*>(pObject))
    {
        CVarBlock* pVarBlock = pObject->GetVarBlock();
        if (pVarBlock)
        {
            return PyGetObjectVariableRec(pVarBlock, splittedPath);
        }
    }
    else
    {
        throw std::logic_error((QString("\"") + pObjName + "\" is an invalid entity.").toUtf8().data());
    }

    return result;
}

pSPyWrappedProperty PyGetEntityParamUsingSharedPtr(const char* pObjName, const char* pVarPath)
{
    SPyWrappedProperty property = PyGetEntityParam(pObjName, pVarPath);
    return  boost::make_shared<SPyWrappedProperty>(property);
}

void PySetEntityParam(const char* pObjectName, const char* pVarPath, SPyWrappedProperty value)
{
    QString varPath = pVarPath;
    std::deque<QString> splittedPath;
    for (auto token : varPath.split(QRegularExpression(QStringLiteral(R"([\\/])")), QString::SkipEmptyParts))
    {
        splittedPath.push_back(token);
    }

    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pObjectName);
    if (pObject)
    {
        CVarBlock* pVarBlock = pObject->GetVarBlock();
        if (pVarBlock)
        {
            IVariable* pVariable = pVarBlock->FindVariable(splittedPath.back().toUtf8().data(), false, true);

            CUndo undo("Set Entity Param");
            if (CUndo::IsRecording())
            {
                CUndo::Record(new CUndoEntityParam(pObjectName, pVarPath));
            }

            if (pVariable)
            {
                if (value.type == SPyWrappedProperty::eType_Bool)
                {
                    pVariable->Set(value.property.boolValue);
                }
                else if (value.type == SPyWrappedProperty::eType_Int)
                {
                    pVariable->Set(value.property.intValue);
                }
                else if (value.type == SPyWrappedProperty::eType_Float)
                {
                    pVariable->Set(value.property.floatValue);
                }
                else if (value.type == SPyWrappedProperty::eType_String)
                {
                    pVariable->Set(value.stringValue);
                }
                else if (value.type == SPyWrappedProperty::eType_Vec3)
                {
                    Vec3 tempVec3;
                    tempVec3[0] = value.property.vecValue.x;
                    tempVec3[1] = value.property.vecValue.y;
                    tempVec3[2] = value.property.vecValue.z;
                    pVariable->Set(tempVec3);
                }
                else
                {
                    throw std::logic_error("Data type is invalid.");
                }
            }
            else
            {
                throw std::logic_error((QString("\"") + pVarPath + "\"" + " is an invalid parameter.").toUtf8().data());
            }
        }
    }
    else
    {
        throw std::logic_error((QString("\"") + pObjectName + "\" is an invalid entity.").toUtf8().data());
    }
}

void PySetEntityParamUsingSharedPtr(const char* pObjectName, const char* pVarPath, pSPyWrappedProperty sharedPtr)
{
    PySetEntityParam(pObjectName, pVarPath, *sharedPtr);
}

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetEntityParamUsingSharedPtr, general, get_entity_param,
    "Gets an object param",
    "general.get_entity_param(str entityName, str paramName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetEntityParamUsingSharedPtr, general, set_entity_param,
    "Sets an object param",
    "general.set_entity_param(str entityName, str paramName, [ bool || int || float || str || (float, float, float) ] paramValue)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetEntityPropertyUsingSharedPtr, general, get_entity_property,
    "Gets an entity property or property2",
    "general.get_entity_property(str entityName, str propertyName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetEntityPropertyUsingSharedPtr, general, set_entity_property,
    "Sets an entity property or property2",
    "general.set_entity_property(str entityName, str propertyName, [ bool || int || float || str || (float, float, float) ] propertyValue)");


#include <Objects/EntityObject.moc>
