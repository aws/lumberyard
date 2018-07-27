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

#include "CryLegacy_precompiled.h"
#include "PhysicsEventListener.h"
#include "IBreakableManager.h"
#include "EntityObject.h"
#include "Entity.h"
#include "EntitySystem.h"
#include "EntityCVars.h"
#include "BreakableManager.h"

#include <IPhysics.h>
#include <IParticles.h>
#include <ParticleParams.h>
#include "IAISystem.h"

#include <ICodeCheckpointMgr.h>

#include "../Cry3DEngine/Environment/OceanEnvironmentBus.h"

std::vector<CPhysicsEventListener::PhysVisAreaUpdate> CPhysicsEventListener::m_physVisAreaUpdateVector;

//////////////////////////////////////////////////////////////////////////
CPhysicsEventListener::CPhysicsEventListener(CEntitySystem* pEntitySystem, IPhysicalWorld* pPhysics)
{
    assert(pEntitySystem);
    assert(pPhysics);
    m_pEntitySystem = pEntitySystem;

    m_pPhysics = pPhysics;

    RegisterPhysicCallbacks();
}

//////////////////////////////////////////////////////////////////////////
CPhysicsEventListener::~CPhysicsEventListener()
{
    UnregisterPhysicCallbacks();

    m_pPhysics->SetPhysicsEventClient(NULL);
}

//////////////////////////////////////////////////////////////////////////
IEntity* CPhysicsEventListener::GetEntity(IPhysicalEntity* pPhysEntity)
{
    assert(pPhysEntity);
    return static_cast<IEntity*>(pPhysEntity->GetForeignData(PHYS_FOREIGN_ID_ENTITY));
}

//////////////////////////////////////////////////////////////////////////
IEntity* CPhysicsEventListener::GetEntity(void* pForeignData, int iForeignData)
{
    if (PHYS_FOREIGN_ID_ENTITY == iForeignData)
    {
        return static_cast<IEntity*>(pForeignData);
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
int CPhysicsEventListener::OnPostStep(const EventPhys* pEvent)
{
    EventPhysPostStep* pPostStep = (EventPhysPostStep*)pEvent;
    IEntity* pEntity = GetEntity(pPostStep->pForeignData, pPostStep->iForeignData);
    IRenderNode* pRndNode = 0;
    if (pEntity)
    {
        if (IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>())
        {
            pPhysicsComponent->OnPhysicsPostStep(pPostStep);
        }
        if (IComponentRenderPtr renderComponent = pEntity->GetComponent<IComponentRender>())
        {
            pRndNode = pEntity->GetComponent<IComponentRender>()->GetRenderNode();
        }
    }
    else if (pPostStep->iForeignData == PHYS_FOREIGN_ID_ROPE)
    {
        IRopeRenderNode* pRenderNode = (IRopeRenderNode*)pPostStep->pForeignData;
        if (pRenderNode)
        {
            pRenderNode->OnPhysicsPostStep();
        }
        pRndNode = pRenderNode;
    }

    if (pRndNode)
    {
        pe_params_flags pf;
        int bInvisible = 0, bFaraway = 0;
        int bEnableOpt = CVar::es_UsePhysVisibilityChecks;
        float dist = (pRndNode->GetBBox().GetCenter() - GetISystem()->GetViewCamera().GetPosition()).len2();
        float maxDist = pPostStep->pEntity->GetType() != PE_SOFT ? CVar::es_MaxPhysDist : CVar::es_MaxPhysDistCloth;
        bInvisible = bEnableOpt & (isneg(pRndNode->GetDrawFrame() + 10 - gEnv->pRenderer->GetFrameID()) | isneg(sqr(maxDist) - dist));
        if (pRndNode->m_nInternalFlags & IRenderNode::WAS_INVISIBLE ^ (-bInvisible & IRenderNode::WAS_INVISIBLE))
        {
            pf.flagsAND = ~pef_invisible;
            pf.flagsOR = -bInvisible & pef_invisible;
            pPostStep->pEntity->SetParams(&pf);
            (pRndNode->m_nInternalFlags &= ~IRenderNode::WAS_INVISIBLE) |= -bInvisible & IRenderNode::WAS_INVISIBLE;
        }
        if (OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : gEnv->p3DEngine->GetWaterLevel() != WATER_LEVEL_UNKNOWN)
        {
            // Deferred updating ignore ocean flag as the Jobs are busy updating the octree at this point
            m_physVisAreaUpdateVector.push_back(PhysVisAreaUpdate(pRndNode, pPostStep->pEntity));
        }
        bFaraway = bEnableOpt & isneg(sqr(maxDist) +
                bInvisible * (sqr(CVar::es_MaxPhysDistInvisible) - sqr(maxDist)) - dist);
        if (bFaraway && !(pRndNode->m_nInternalFlags & IRenderNode::WAS_FARAWAY))
        {
            pe_params_foreign_data pfd;
            pPostStep->pEntity->GetParams(&pfd);
            bFaraway &= -(pfd.iForeignFlags & PFF_UNIMPORTANT) >> 31;
        }
        if ((-bFaraway & IRenderNode::WAS_FARAWAY) != (pRndNode->m_nInternalFlags & IRenderNode::WAS_FARAWAY))
        {
            pe_params_timeout pto;
            pto.timeIdle = 0;
            pto.maxTimeIdle = bFaraway * CVar::es_FarPhysTimeout;
            pPostStep->pEntity->SetParams(&pto);
            (pRndNode->m_nInternalFlags &= ~IRenderNode::WAS_FARAWAY) |= -bFaraway & IRenderNode::WAS_FARAWAY;
        }
    }

    return 1;
}

int CPhysicsEventListener::OnPostPump(const EventPhys* pEvent)
{
    if (OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : gEnv->p3DEngine->GetWaterLevel() != WATER_LEVEL_UNKNOWN)
    {
        for (std::vector<PhysVisAreaUpdate>::iterator it = m_physVisAreaUpdateVector.begin(), end = m_physVisAreaUpdateVector.end(); it != end; ++it)
        {
            IRenderNode* pRndNode = it->m_pRndNode;
            int bInsideVisarea = pRndNode->GetEntityVisArea() != 0;
            if (pRndNode->m_nInternalFlags & IRenderNode::WAS_IN_VISAREA ^ (-bInsideVisarea & IRenderNode::WAS_IN_VISAREA))
            {
                pe_params_flags pf;
                pf.flagsAND = ~pef_ignore_ocean;
                pf.flagsOR = -bInsideVisarea & pef_ignore_ocean;
                it->m_pEntity->SetParams(&pf);
                (pRndNode->m_nInternalFlags &= ~IRenderNode::WAS_IN_VISAREA) |= -bInsideVisarea & IRenderNode::WAS_IN_VISAREA;
            }
        }
        m_physVisAreaUpdateVector.clear();
    }
    return 1;
}

//////////////////////////////////////////////////////////////////////////
int CPhysicsEventListener::OnBBoxOverlap(const EventPhys* pEvent)
{
    EventPhysBBoxOverlap* pOverlap = (EventPhysBBoxOverlap*)pEvent;

    IEntity* pEntity = GetEntity(pOverlap->pForeignData[0], pOverlap->iForeignData[0]);
    IEntity* pEntityTrg = GetEntity(pOverlap->pForeignData[1], pOverlap->iForeignData[1]);
    if (pEntity && pEntityTrg)
    {
        IComponentPhysicsPtr pPhysicsSrc = pEntity->GetComponent<IComponentPhysics>();
        if (pPhysicsSrc)
        {
            pPhysicsSrc->OnContactWithEntity(pEntityTrg);
        }

        IComponentPhysicsPtr pPhysicsTrg = pEntityTrg->GetComponent<IComponentPhysics>();
        if (pPhysicsTrg)
        {
            pPhysicsTrg->OnContactWithEntity(pEntity);
        }
    }
    return 1;
}

//////////////////////////////////////////////////////////////////////////
int CPhysicsEventListener::OnStateChange(const EventPhys* pEvent)
{
    EventPhysStateChange* pStateChange = (EventPhysStateChange*)pEvent;
    IEntity* pEntity = GetEntity(pStateChange->pForeignData, pStateChange->iForeignData);
    if (pEntity)
    {
        EEntityUpdatePolicy policy = pEntity->GetUpdatePolicy();
        // If its update depends on physics, physics state defines if this entity is to be updated.
        if (policy == ENTITY_UPDATE_PHYSICS || policy == ENTITY_UPDATE_PHYSICS_VISIBLE)
        {
            int nNewSymClass = pStateChange->iSimClass[1];
            //          int nOldSymClass = pStateChange->iSimClass[0];
            if (nNewSymClass == SC_ACTIVE_RIGID)
            {
                // Should activate entity if physics is awaken.
                pEntity->Activate(true);
            }
            else if (nNewSymClass == SC_SLEEPING_RIGID)
            {
                // Entity must go to sleep.
                pEntity->Activate(false);
                //CallStateFunction(ScriptState_OnStopRollSlideContact, "roll");
                //CallStateFunction(ScriptState_OnStopRollSlideContact, "slide");
            }
        }
        int nOldSymClass = pStateChange->iSimClass[0];
        if (nOldSymClass == SC_ACTIVE_RIGID)
        {
            SEntityEvent event(ENTITY_EVENT_PHYSICS_CHANGE_STATE);
            event.nParam[0] = 1;
            pEntity->SendEvent(event);
            if (pStateChange->timeIdle >= CVar::es_FarPhysTimeout)
            {
                pe_status_dynamics sd;
                if (pStateChange->pEntity->GetStatus(&sd) && sd.submergedFraction > 0)
                {
                    pEntity->SetFlags(pEntity->GetFlags() | ENTITY_FLAG_SEND_RENDER_EVENT);
                    IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
                    pPhysicsComponent->SetFlags(pPhysicsComponent->GetFlags() | CComponentPhysics::FLAG_PHYS_AWAKE_WHEN_VISIBLE);
                }
            }
        }
        else if (nOldSymClass == SC_SLEEPING_RIGID)
        {
            SEntityEvent event(ENTITY_EVENT_PHYSICS_CHANGE_STATE);
            event.nParam[0] = 0;
            pEntity->SendEvent(event);
        }
    }
    return 1;
}

//////////////////////////////////////////////////////////////////////////
#include <IDeferredCollisionEvent.h>

class CDeferredMeshUpdatePrep
    : public IDeferredPhysicsEvent
{
public:
    CDeferredMeshUpdatePrep() { m_status = 0; m_pStatObj = 0; m_id = -1; m_pMesh = 0; }
    IStatObj* m_pStatObj;
    int m_id;
    IGeometry* m_pMesh, * m_pMeshSkel;
    Matrix34 m_mtxSkelToMesh;
    volatile int m_status;

    virtual void Start() {}
    virtual int Result(EventPhys*) { return 0; }
    virtual void Sync() {}
    virtual bool HasFinished() { return m_status > 1; }
    virtual DeferredEventType GetType() const { return PhysCallBack_OnCollision; }
    virtual EventPhys* PhysicsEvent() { return 0; }

    virtual void OnUpdate()
    {
        if (m_id)
        {
            m_pStatObj->GetIndexedMesh(true);
        }
        else
        {
            mesh_data* md = (mesh_data*)m_pMeshSkel->GetData();
            IStatObj* pDeformedStatObj = m_pStatObj->SkinVertices(md->pVertices, m_mtxSkelToMesh);
            m_pMesh->SetForeignData(pDeformedStatObj, 0);
        }
        m_pStatObj->Release();
        if (m_threadTaskInfo.m_pThread)
        {
            gEnv->pSystem->GetIThreadTaskManager()->UnregisterTask(this);
        }
        m_status = 2;
    }
    virtual void Stop() {}

    virtual SThreadTaskInfo* GetTaskInfo() { return &m_threadTaskInfo; }
    SThreadTaskInfo m_threadTaskInfo;
};

int g_lastReceivedEventId = 1, g_lastExecutedEventId = 1;
CDeferredMeshUpdatePrep g_meshPreps[16];

int CPhysicsEventListener::OnPreUpdateMesh(const EventPhys* pEvent)
{
    EventPhysUpdateMesh* pepum = (EventPhysUpdateMesh*)pEvent;
    IStatObj* pStatObj;
    if (pepum->iReason == EventPhysUpdateMesh::ReasonDeform || !(pStatObj = (IStatObj*)pepum->pMesh->GetForeignData()))
    {
        return 1;
    }

    if (pepum->idx < 0)
    {
        pepum->idx = pepum->iReason == EventPhysUpdateMesh::ReasonDeform ? 0 : ++g_lastReceivedEventId;
    }

    bool bDefer = false;
    if (g_lastExecutedEventId < pepum->idx - 1)
    {
        bDefer = true;
    }
    else
    {
        int i, j;
        for (i = sizeof(g_meshPreps) / sizeof(g_meshPreps[0]) - 1, j = -1;
             i >= 0 && (!g_meshPreps[i].m_status || (pepum->idx ? (g_meshPreps[i].m_id != pepum->idx) : (g_meshPreps[i].m_pMesh != pepum->pMesh))); i--)
        {
            j += i + 1 & (g_meshPreps[i].m_status - 1 & j) >> 31;
        }
        if (i >= 0)
        {
            if (g_meshPreps[i].m_status == 2)
            {
                g_meshPreps[i].m_status = 0;
            }
            else
            {
                bDefer = true;
            }
        }
        else if (pepum->iReason == EventPhysUpdateMesh::ReasonDeform || !pStatObj->GetIndexedMesh(false))
        {
            if (j >= 0)
            {
                (g_meshPreps[j].m_pStatObj = pStatObj)->AddRef();
                g_meshPreps[j].m_pMesh = pepum->pMesh;
                g_meshPreps[j].m_pMeshSkel = pepum->pMeshSkel;
                g_meshPreps[j].m_mtxSkelToMesh = pepum->mtxSkelToMesh;
                g_meshPreps[j].m_id = pepum->idx;
                g_meshPreps[j].m_status = 1;
                gEnv->p3DEngine->GetDeferredPhysicsEventManager()->DispatchDeferredEvent(&g_meshPreps[j]);
            }
            bDefer = true;
        }
    }
    if (bDefer)
    {
        gEnv->pPhysicalWorld->AddDeferredEvent(EventPhysUpdateMesh::id, pepum);
        return 0;
    }

    if (pepum->idx > 0)
    {
        g_lastExecutedEventId = pepum->idx;
    }
    return 1;
};

int CPhysicsEventListener::OnPreCreatePhysEntityPart(const EventPhys* pEvent)
{
    EventPhysCreateEntityPart* pepcep = (EventPhysCreateEntityPart*)pEvent;
    if (!pepcep->pMeshNew)
    {
        return 1;
    }
    if (pepcep->idx < 0)
    {
        pepcep->idx = ++g_lastReceivedEventId;
    }
    if (g_lastExecutedEventId < pepcep->idx - 1)
    {
        gEnv->pPhysicalWorld->AddDeferredEvent(EventPhysCreateEntityPart::id, pepcep);
        return 0;
    }
    g_lastExecutedEventId = pepcep->idx;
    return 1;
}


//////////////////////////////////////////////////////////////////////////
int CPhysicsEventListener::OnUpdateMesh(const EventPhys* pEvent)
{
    ((CBreakableManager*)GetIEntitySystem()->GetBreakableManager())->HandlePhysics_UpdateMeshEvent((EventPhysUpdateMesh*)pEvent);
    return 1;
}

//////////////////////////////////////////////////////////////////////////
int CPhysicsEventListener::OnCreatePhysEntityPart(const EventPhys* pEvent)
{
    EventPhysCreateEntityPart* pCreateEvent = (EventPhysCreateEntityPart*)pEvent;

    //////////////////////////////////////////////////////////////////////////
    // Let Breakable manager handle creation of the new entity part.
    //////////////////////////////////////////////////////////////////////////
    CBreakableManager* pBreakMgr = (CBreakableManager*)GetIEntitySystem()->GetBreakableManager();
    pBreakMgr->HandlePhysicsCreateEntityPartEvent(pCreateEvent);
    return 1;
}

//////////////////////////////////////////////////////////////////////////
int CPhysicsEventListener::OnRemovePhysEntityParts(const EventPhys* pEvent)
{
    EventPhysRemoveEntityParts* pRemoveEvent = (EventPhysRemoveEntityParts*)pEvent;

    CBreakableManager* pBreakMgr = (CBreakableManager*)GetIEntitySystem()->GetBreakableManager();
    pBreakMgr->HandlePhysicsRemoveSubPartsEvent(pRemoveEvent);

    return 1;
}

//////////////////////////////////////////////////////////////////////////
int CPhysicsEventListener::OnRevealPhysEntityPart(const EventPhys* pEvent)
{
    EventPhysRevealEntityPart* pRevealEvent = (EventPhysRevealEntityPart*)pEvent;

    CBreakableManager* pBreakMgr = (CBreakableManager*)GetIEntitySystem()->GetBreakableManager();
    pBreakMgr->HandlePhysicsRevealSubPartEvent(pRevealEvent);

    return 1;
}

//////////////////////////////////////////////////////////////////////////
IEntity* MatchPartId(IEntity* pent, int partid)
{
    IComponentPhysicsPtr pPhysicsComponent = pent->GetComponent<IComponentPhysics>();
    if (pPhysicsComponent && (unsigned int)(partid - pPhysicsComponent->GetPartId0()) < 1000u)
    {
        return pent;
    }
    IEntity* pMatch;
    for (int i = pent->GetChildCount() - 1; i >= 0; i--)
    {
        if (pMatch = MatchPartId(pent->GetChild(i), partid))
        {
            return pMatch;
        }
    }
    return 0;
}

IEntity* CEntity::UnmapAttachedChild(int& partId)
{
    CEntity* pChild;
    if (partId >= PARTID_LINKED && (pChild = static_cast<CEntity*>(MatchPartId(this, partId))))
    {
        partId -= pChild->GetComponent<IComponentPhysics>()->GetPartId0();
        return pChild;
    }
    return this;
}

int CPhysicsEventListener::OnCollision(const EventPhys* pEvent)
{
    EventPhysCollision* pCollision = (EventPhysCollision*)pEvent;
    SEntityEvent event;
    IEntity* pEntitySrc = GetEntity(pCollision->pForeignData[0], pCollision->iForeignData[0]);
    IEntity* pEntityTrg = GetEntity(pCollision->pForeignData[1], pCollision->iForeignData[1]);
    IEntity* pChild;

    if (pEntitySrc && pCollision->partid[0] >= PARTID_LINKED && (pChild = MatchPartId(pEntitySrc, pCollision->partid[0])))
    {
        pEntitySrc = pChild;
        pCollision->pForeignData[0] = pEntitySrc;
        pCollision->iForeignData[0] = PHYS_FOREIGN_ID_ENTITY;
        pCollision->partid[0] -= pEntitySrc->GetComponent<IComponentPhysics>()->GetPartId0();
    }

    if (pEntitySrc)
    {
        if (pEntityTrg && pCollision->partid[1] >= PARTID_LINKED && (pChild = MatchPartId(pEntityTrg, pCollision->partid[1])))
        {
            pEntityTrg = pChild;
            pCollision->pForeignData[1] = pEntityTrg;
            pCollision->iForeignData[1] = PHYS_FOREIGN_ID_ENTITY;
            pCollision->partid[1] -= pEntityTrg->GetComponent<IComponentPhysics>()->GetPartId0();
        }

        IComponentPhysicsPtr pPhysicsSrc = pEntitySrc->GetComponent<IComponentPhysics>();
        if (pPhysicsSrc)
        {
            pPhysicsSrc->OnCollision(pEntityTrg, pCollision->idmat[1], pCollision->pt, pCollision->n, pCollision->vloc[0], pCollision->vloc[1], pCollision->partid[1], pCollision->mass[1]);
        }

        if (pEntityTrg)
        {
            IComponentPhysicsPtr pPhysicsTrg = pEntityTrg->GetComponent<IComponentPhysics>();
            if (pPhysicsTrg)
            {
                pPhysicsTrg->OnCollision(pEntitySrc, pCollision->idmat[0], pCollision->pt, -pCollision->n, pCollision->vloc[1], pCollision->vloc[0], pCollision->partid[0], pCollision->mass[0]);
            }
        }
    }

    event.event = ENTITY_EVENT_COLLISION;
    event.nParam[0] = (INT_PTR)pEvent;
    event.nParam[1] = 0;
    if (pEntitySrc)
    {
        pEntitySrc->SendEvent(event);
    }
    event.nParam[1] = 1;
    if (pEntityTrg)
    {
        pEntityTrg->SendEvent(event);
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////
int CPhysicsEventListener::OnJointBreak(const EventPhys* pEvent)
{
    EventPhysJointBroken* pBreakEvent = (EventPhysJointBroken*)pEvent;
    IEntity* pEntity = 0;
    IStatObj* pStatObj = 0;
    IRenderNode* pRenderNode = 0;
    Matrix34A nodeTM;

    // Counter for feature test setup
    CODECHECKPOINT(physics_on_joint_break);

    bool bShatter = false;
    switch (pBreakEvent->iForeignData[0])
    {
    case PHYS_FOREIGN_ID_ROPE:
    {
        IRopeRenderNode* pRopeRenderNode = (IRopeRenderNode*)pBreakEvent->pForeignData[0];
        if (pRopeRenderNode)
        {
            EntityId id = (EntityId)pRopeRenderNode->GetEntityOwner();
            pEntity = g_pIEntitySystem->GetEntity(id);
        }
    }
    break;
    case PHYS_FOREIGN_ID_ENTITY:
        pEntity = static_cast<IEntity*>(pBreakEvent->pForeignData[0]);
        break;
    case PHYS_FOREIGN_ID_STATIC:
    {
        pRenderNode = ((IRenderNode*)pBreakEvent->pForeignData[0]);
        pStatObj = pRenderNode->GetEntityStatObj(0, 0, &nodeTM);
        bShatter = pRenderNode->GetMaterialLayers() & MTL_LAYER_FROZEN;
    }
    }
    //GetEntity(pBreakEvent->pForeignData[0],pBreakEvent->iForeignData[0]);
    if (pEntity)
    {
        SEntityEvent event;
        event.event = ENTITY_EVENT_PHYS_BREAK;
        event.nParam[0] = (INT_PTR)pEvent;
        event.nParam[1] = 0;
        pEntity->SendEvent(event);
        pStatObj = pEntity->GetStatObj(ENTITY_SLOT_ACTUAL);

        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            bShatter = pRenderComponent->GetMaterialLayersMask() & MTL_LAYER_FROZEN;
        }
    }

    IStatObj* pStatObjEnt = pStatObj;
    if (pStatObj)
    {
        while (pStatObj->GetCloneSourceObject())
        {
            pStatObj = pStatObj->GetCloneSourceObject();
        }
    }

    if (pStatObj && pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND)
    {
        Matrix34 tm = Matrix34::CreateTranslationMat(pBreakEvent->pt);
        IStatObj* pObj1 = 0;
        //      IStatObj *pObj2=0;
        Vec3 axisx = pBreakEvent->n.GetOrthogonal().normalized();
        tm.SetRotation33(Matrix33(axisx, pBreakEvent->n ^ axisx, pBreakEvent->n).T());

        IStatObj::SSubObject* pSubObject1 = pStatObj->GetSubObject(pBreakEvent->partid[0]);
        if (pSubObject1)
        {
            pObj1 = pSubObject1->pStatObj;
        }
        //IStatObj::SSubObject *pSubObject2 = pStatObj->GetSubObject(pBreakEvent->partid[1]);
        //if (pSubObject2)
        //pObj2 = pSubObject2->pStatObj;

        const char* sEffectType = (!bShatter) ? SURFACE_BREAKAGE_TYPE("joint_break") : SURFACE_BREAKAGE_TYPE("joint_shatter");
        CBreakableManager* pBreakMgr = (CBreakableManager*)GetIEntitySystem()->GetBreakableManager();
        if (pObj1)
        {
            pBreakMgr->CreateSurfaceEffect(pObj1, tm, sEffectType);
        }
        //if (pObj2)
        //pBreakMgr->CreateSurfaceEffect( pObj2,tm,sEffectType );
    }

    IStatObj::SSubObject* pSubObj;

    if (pStatObj && (pSubObj = pStatObj->GetSubObject(pBreakEvent->idJoint)) &&
        pSubObj->nType == STATIC_SUB_OBJECT_DUMMY && !strncmp(pSubObj->name, "$joint", 6))
    {
        const char* ptr;

        if (ptr = strstr(pSubObj->properties, "effect"))
        {
            for (ptr += 6; *ptr && (*ptr == ' ' || *ptr == '='); ptr++)
            {
                ;
            }
            if (*ptr)
            {
                char strEff[256];
                const char* const peff = ptr;
                while (*ptr && *ptr != '\n')
                {
                    ++ptr;
                }
                cry_strcpy(strEff, peff, (size_t)(ptr - peff));
                IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(strEff);
                pEffect->Spawn(true, IParticleEffect::ParticleLoc(pBreakEvent->pt, pBreakEvent->n));
            }
        }

        if (ptr = strstr(pSubObj->properties, "breaker"))
        {
            if (!(pStatObjEnt->GetFlags() & STATIC_OBJECT_GENERATED))
            {
                pStatObjEnt = pStatObjEnt->Clone(false, false, true);
                pStatObjEnt->SetFlags(pStatObjEnt->GetFlags() | STATIC_OBJECT_GENERATED);
                if (pEntity)
                {
                    pEntity->SetStatObj(pStatObjEnt, ENTITY_SLOT_ACTUAL, false);
                }
                else if (pRenderNode)
                {
                    IBreakableManager::SCreateParams createParams;
                    createParams.pSrcStaticRenderNode = pRenderNode;
                    createParams.fScale = nodeTM.GetColumn(0).len();
                    createParams.nSlotIndex = 0;
                    createParams.worldTM = nodeTM;
                    createParams.nMatLayers = pRenderNode->GetMaterialLayers();
                    createParams.nRenderNodeFlags = pRenderNode->GetRndFlags();
                    createParams.pCustomMtl = pRenderNode->GetMaterial();
                    createParams.nEntityFlagsAdd = ENTITY_FLAG_MODIFIED_BY_PHYSICS;
                    createParams.pName = pRenderNode->GetName();
                    ((CBreakableManager*)GetIEntitySystem()->GetBreakableManager())->CreateObjectAsEntity(
                        pStatObjEnt, pBreakEvent->pEntity[0], pBreakEvent->pEntity[0], createParams, true);
                }
            }

            IStatObj::SSubObject* pSubObj1;
            const char* piecesStr;
            for (int i = 0; i < 2; i++)
            {
                if ((pSubObj = pStatObjEnt->GetSubObject(pBreakEvent->partid[i])) && pSubObj->pStatObj &&
                    (piecesStr = strstr(pSubObj->properties, "pieces=")) && (pSubObj1 = pStatObj->FindSubObject(piecesStr + 7)) &&
                    pSubObj1->pStatObj)
                {
                    pSubObj->pStatObj->Release();
                    (pSubObj->pStatObj = pSubObj1->pStatObj)->AddRef();
                }
            }
            pStatObjEnt->Invalidate(false);
        }
    }

    return 1;
}

void CPhysicsEventListener::RegisterPhysicCallbacks()
{
    if (m_pPhysics)
    {
        m_pPhysics->AddEventClient(EventPhysStateChange::id, OnStateChange, 1);
        m_pPhysics->AddEventClient(EventPhysBBoxOverlap::id, OnBBoxOverlap, 1);
        m_pPhysics->AddEventClient(EventPhysPostStep::id, OnPostStep, 1);
        m_pPhysics->AddEventClient(EventPhysUpdateMesh::id, OnUpdateMesh, 1);
        m_pPhysics->AddEventClient(EventPhysUpdateMesh::id, OnUpdateMesh, 0);
        m_pPhysics->AddEventClient(EventPhysCreateEntityPart::id, OnCreatePhysEntityPart, 1);
        m_pPhysics->AddEventClient(EventPhysCreateEntityPart::id, OnCreatePhysEntityPart, 0);
        m_pPhysics->AddEventClient(EventPhysRemoveEntityParts::id, OnRemovePhysEntityParts, 1);
        m_pPhysics->AddEventClient(EventPhysRevealEntityPart::id, OnRevealPhysEntityPart, 1);
        m_pPhysics->AddEventClient(EventPhysCollision::id, OnCollision, 1, 1000.0f);
        m_pPhysics->AddEventClient(EventPhysJointBroken::id, OnJointBreak, 1);
        m_pPhysics->AddEventClient(EventPhysPostPump::id, OnPostPump, 1);
    }
}

void CPhysicsEventListener::UnregisterPhysicCallbacks()
{
    if (m_pPhysics)
    {
        m_pPhysics->RemoveEventClient(EventPhysStateChange::id, OnStateChange, 1);
        m_pPhysics->RemoveEventClient(EventPhysBBoxOverlap::id, OnBBoxOverlap, 1);
        m_pPhysics->RemoveEventClient(EventPhysPostStep::id, OnPostStep, 1);
        m_pPhysics->RemoveEventClient(EventPhysUpdateMesh::id, OnUpdateMesh, 1);
        m_pPhysics->RemoveEventClient(EventPhysUpdateMesh::id, OnUpdateMesh, 0);
        m_pPhysics->RemoveEventClient(EventPhysCreateEntityPart::id, OnCreatePhysEntityPart, 1);
        m_pPhysics->RemoveEventClient(EventPhysCreateEntityPart::id, OnCreatePhysEntityPart, 0);
        m_pPhysics->RemoveEventClient(EventPhysRemoveEntityParts::id, OnRemovePhysEntityParts, 1);
        m_pPhysics->RemoveEventClient(EventPhysRevealEntityPart::id, OnRevealPhysEntityPart, 1);
        m_pPhysics->RemoveEventClient(EventPhysCollision::id, OnCollision, 1);
        m_pPhysics->RemoveEventClient(EventPhysJointBroken::id, OnJointBreak, 1);
        m_pPhysics->RemoveEventClient(EventPhysPostPump::id, OnPostPump, 1);
        stl::free_container(m_physVisAreaUpdateVector);
    }
}
