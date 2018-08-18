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
#include "CryLegacy_precompiled.h"
#include "ComponentRender.h"
#include "Components/IComponentSerialization.h"

#include "Entity.h"
#include "IEntity.h"
#include "EntityObject.h"
#include "EntityTimeoutList.h"
#include "ISerialize.h"
#include "ParticleParams.h"

#include <IRenderer.h>
#include <IEntityRenderState.h>
#include <ISystem.h>
#include <ICryAnimation.h>
#include <IShaderParamCallback.h>

#include <CryExtension/CryCreateClassInstance.h>

#include <AzCore/Jobs/LegacyJobExecutor.h>
#include <AzCore/std/sort.h>

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentRender, IComponentRender)

#if defined(PLATFORM_64BIT) // the parameters for the particle geometry render job are too large for the infoblock on 32 bit systems, there only use it on 64 bit
#   define SUPPORT_COMPONENTRENDER_RENDER_JOB
#endif // PLATFORM_64BIT

float CComponentRender::s_fViewDistMin          = 0.0f;
float CComponentRender::s_fViewDistRatio        = 60.0f;
float CComponentRender::s_fViewDistRatioCustom  = 60.0f;
float CComponentRender::s_fViewDistRatioDetail  = 30.0f;
std::vector<CComponentRender*> CComponentRender::s_arrCharactersToRegisterForRendering;

CEntityTimeoutList* CComponentRender::s_pTimeoutList = 0;

inline void CheckIfBBoxValid(const AABB& box, IEntity* pEntity)
{
#ifndef _RELEASE
    float v = box.min.x + box.min.y + box.min.z + box.max.x + box.max.y + box.max.z;
    if (!_finite(v))
    {
        CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, "BBox contain invalid floating values for Entity: %s", pEntity->GetEntityTextDescription());
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
CComponentRender::CComponentRender()
{
    m_pEntity = 0;
    m_entityId = -1;
    m_nFlags = 0;

    m_localBBox.min.Set(0, 0, 0);
    m_localBBox.max.Set(0, 0, 0);
    m_WSBBox.min = Vec3(ZERO);
    m_WSBBox.max = Vec3(ZERO);

    m_pMaterial = NULL;

    m_nHUDSilhouettesParams = 0;
    m_nMaterialLayers = 0;
    m_nMaterialLayersBlend = 0;
    m_nOpacity = 255;
    m_nCustomData = 0;
    memset(m_fCustomData, 0, sizeof(m_fCustomData));
    m_fLodDistance = 0.0f;
    m_fLastSeenTime = gEnv->pTimer->GetCurrTime();
}

//////////////////////////////////////////////////////////////////////////
CComponentRender::~CComponentRender()
{
    GetI3DEngine()->FreeRenderNodeState(this); // internally calls UnRegisterEntity(this)

    ClearShaderParamCallbacks();

    // Free all slots.
    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        if (m_slots[i])
        {
            delete m_slots[i];
        }
    }
    m_slots.clear();

    // use a loop, to prevent crashes due duplicates if some client code decided to
    // call register for rendering multiple times
    while (1)
    {
        std::vector<CComponentRender*>::iterator it = std::find(s_arrCharactersToRegisterForRendering.begin(), s_arrCharactersToRegisterForRendering.end(), this);
        if (it == s_arrCharactersToRegisterForRendering.end())
        {
            break;
        }
        s_arrCharactersToRegisterForRendering.erase(it);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;
    m_entityId = m_pEntity->GetId(); // Store so we can use it during Render() (when we shouldn't touch the entity itself).
    m_pMaterial = m_pEntity->GetMaterial();

    m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentRender>(SerializationOrder::Render, *this, &CComponentRender::Serialize, &CComponentRender::SerializeXML, &CComponentRender::NeedSerialize, &CComponentRender::GetSignature);

    UpdateEntityFlags();

    if (m_pEntity->IsInitialized())
    {
        AddFlags(FLAG_POST_INIT);
        RegisterForRendering(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::PostInit()
{
    AddFlags(FLAG_POST_INIT);

    if (m_pEntity->IsHidden() || m_pEntity->IsInvisible())
    {
        OnHide(true);
    }
    else
    {
        RegisterForRendering(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    assert(pEntity == m_pEntity);

    m_entityId = pEntity->GetId(); // Store so we can use it during Render() (when we shouldn't touch the entity itself).

    m_nFlags = 0;

    m_localBBox.min.Set(0, 0, 0);
    m_localBBox.max.Set(0, 0, 0);
    m_WSBBox.min = Vec3(ZERO);
    m_WSBBox.max = Vec3(ZERO);

    m_pMaterial = pEntity->GetMaterial();

    m_nHUDSilhouettesParams = 0;
    m_nMaterialLayers = 0;
    m_nMaterialLayersBlend = 0;
    m_nOpacity = 255;

    m_Callbacks.clear();

    UpdateEntityFlags();

    ClearSlots();

    if (pEntity->IsInitialized())
    {
        AddFlags(FLAG_POST_INIT);
        RegisterForRendering(true);
    }

    m_fLastSeenTime = gEnv->pTimer->GetCurrTime();
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::UpdateEntityFlags()
{
    int nEntityFlags = m_pEntity->GetFlags();
    m_nEntityFlags = nEntityFlags;

    if (nEntityFlags & ENTITY_FLAG_CASTSHADOW)
    {
        m_dwRndFlags |= ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS;
    }
    else
    {
        m_dwRndFlags &= ~ERF_CASTSHADOWMAPS;
    }

    if (nEntityFlags & ENTITY_FLAG_GOOD_OCCLUDER)
    {
        m_dwRndFlags |= ERF_GOOD_OCCLUDER;
    }
    else
    {
        m_dwRndFlags &= ~ERF_GOOD_OCCLUDER;
    }

    if (nEntityFlags & ENTITY_FLAG_OUTDOORONLY)
    {
        m_dwRndFlags |= ERF_OUTDOORONLY;
    }
    else
    {
        m_dwRndFlags &= ~ERF_OUTDOORONLY;
    }

    if (nEntityFlags & ENTITY_FLAG_RECVWIND)
    {
        m_dwRndFlags |= ERF_RECVWIND;
    }
    else
    {
        m_dwRndFlags &= ~ERF_RECVWIND;
    }

    if (nEntityFlags & ENTITY_FLAG_NO_DECALNODE_DECALS)
    {
        m_dwRndFlags |= ERF_NO_DECALNODE_DECALS;
    }
    else
    {
        m_dwRndFlags &= ~ERF_NO_DECALNODE_DECALS;
    }

    if (m_nEntityFlags & ENTITY_FLAG_SEND_RENDER_EVENT)
    {
        ClearFlags(FLAG_EXECUTE_AS_JOB_FLAG);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentRender::CheckCharacterForMorphs(ICharacterInstance* pCharacter)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::AnimEventCallback(ICharacterInstance* pCharacter, void* userdata)
{
    CComponentRender* pInstance = static_cast< CComponentRender* >(userdata);
    if (pInstance)
    {
        pInstance->AnimationEvent(pCharacter, pCharacter->GetISkeletonAnim()->GetLastAnimEvent());
    }
    return 1;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::AnimationEvent(ICharacterInstance* pCharacter, const AnimEventInstance& animEvent)
{
    // Send an entity event.
    SEntityEvent event(ENTITY_EVENT_ANIM_EVENT);
    event.nParam[0] = (INT_PTR)&animEvent;
    event.nParam[1] = (INT_PTR)pCharacter;
    m_pEntity->SendEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::ExpandCompoundSlot0()
{
    AddFlags(FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG);

    int i, nSlots = (int)m_slots.size();
    IStatObj::SSubObject* pSubObj;
    IStatObj* pStatObj;
    Matrix34 mtx;

    if (nSlots == 1 && m_slots[0] && m_slots[0]->pStatObj && m_slots[0]->pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND)
    {
        // if we currently have a compound cgf in slot 0, expand the this compound cgf into subobjects
        pStatObj = m_slots[0]->pStatObj;
        pStatObj->AddRef();
        mtx = GetSlotLocalTM(0, false);
        FreeSlot(0);
        nSlots = 0;
        for (i = 0; i < pStatObj->GetSubObjectCount(); i++)
        {
            if (pSubObj = pStatObj->GetSubObject(i))
            {
                m_slots.push_back(new CEntityObject);
                m_slots[nSlots]->SetLocalTM(mtx * pSubObj->localTM);
                m_slots[nSlots]->pStatObj = pSubObj->pStatObj;
                if (pSubObj->pStatObj)
                {
                    pSubObj->pStatObj->AddRef();
                    if (!pSubObj->bHidden)
                    {
                        m_slots[nSlots]->flags |= ENTITY_SLOT_RENDER;
                    }
                    if (strstr(pSubObj->properties, "entity"))
                    {
                        m_slots[nSlots]->flags |= ENTITY_SLOT_BREAK_AS_ENTITY;
                    }
                }
                nSlots++;
            }
        }
        pStatObj->Release();
        m_pEntity->AddFlags(ENTITY_FLAG_SLOTS_CHANGED);
    }
}

//////////////////////////////////////////////////////////////////////////
CEntityObject* CComponentRender::AllocSlot(int nIndex)
{
    AddFlags(FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG);

    if (nIndex >= (int)m_slots.size())
    {
        m_slots.resize(nIndex + 1);
        m_slots[nIndex] = new CEntityObject;
    }
    else
    {
        if (m_slots[nIndex] == NULL)
        {
            m_slots[nIndex] = new CEntityObject;
        }
    }

    return m_slots[nIndex];
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::FreeSlot(int nIndex)
{
    AddFlags(FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG);
    //  assert(nIndex>=0 && nIndex < (int)m_slots.size());
    if ((nIndex >= 0 && nIndex < (int)m_slots.size()) && (m_slots[nIndex] != NULL))
    {
        CEntityObject* pSlot = m_slots[nIndex];

        // clear shaderparam callbacks in case a slot which is used by it is deleted
        if (!m_Callbacks.empty() && pSlot && pSlot->pCharacter)
        {
            for (TCallbackVector::iterator it = m_Callbacks.begin(); it != m_Callbacks.end(); )
            {
                if ((*it)->GetCharacterInstance() == pSlot->pCharacter)
                {
                    it = m_Callbacks.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        m_slots[nIndex] = NULL;

        if (nIndex == m_slots.size() - 1)
        {
            // If Last slot, then cut away all empty slots at the end of the array.
            for (int i = nIndex; i >= 0 && m_slots[i] == NULL; i--)
            {
                m_slots.pop_back();
            }
        }
        // Check if any other slot references deleted slot as a parent, if yes reset it to NULL.
        for (uint32 i = 0; i < m_slots.size(); i++)
        {
            if (m_slots[i] && m_slots[i]->pParent == pSlot)
            {
                m_slots[i]->pParent = NULL;
            }
        }

        delete pSlot;
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::FreeAllSlots()
{
    AddFlags(FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG);

    // Free all slots.
    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        if (m_slots[i])
        {
            delete m_slots[i];
        }
    }
    m_slots.clear();
    ClearShaderParamCallbacks();
}

//////////////////////////////////////////////////////////////////////////
CEntityObject* CComponentRender::GetOrMakeSlot(int& nSlot)
{
    if (nSlot < 0)
    {
        nSlot = m_slots.size();
    }
    return AllocSlot(nSlot);
}

//////////////////////////////////////////////////////////////////////////
CEntityObject* CComponentRender::GetParentSlot(int nIndex) const
{
    if (IsSlotValid(nIndex))
    {
        return Slot(nIndex)->pParent;
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentRender::SetParentSlot(int nParentIndex, int nChildIndex)
{
    if (Slot(nParentIndex) && Slot(nChildIndex))
    {
        CEntityObject* pChildSlot = Slot(nChildIndex);
        pChildSlot->SetParent(Slot(nParentIndex));
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
IStatObj* CComponentRender::GetCompoundObj() const
{
    return m_slots.size() == 1 && Slot(0)->pStatObj && Slot(0)->pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND ? Slot(0)->pStatObj : 0;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetSlotLocalTM(int nIndex, const Matrix34& localTM, int nWhyFlags)
{
    if (IsSlotValid(nIndex))
    {
        CEntityObject* pSlot = Slot(nIndex);
        Matrix34 identTM;
        identTM.SetIdentity();
        if (pSlot->m_pXForm || !Matrix34::IsEquivalent(localTM, identTM, 0.001f) || pSlot->flags & ENTITY_SLOT_RENDER_NEAREST)
        {
            Slot(nIndex)->SetLocalTM(localTM);
            Slot(nIndex)->OnXForm(m_pEntity);

            if (!(nWhyFlags & ENTITY_XFORM_NOT_REREGISTER)) // A special optimization for characters
            {
                // Invalidate bounding box.
                InvalidateBounds(true, true);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetSlotCameraSpacePos(int nIndex, const Vec3& cameraSpacePos)
{
    if (IsSlotValid(nIndex))
    {
        CEntityObject* pSlot = Slot(nIndex);
        if (pSlot)
        {
            pSlot->SetCameraSpacePos(cameraSpacePos);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::GetSlotCameraSpacePos(int nIndex, Vec3& cameraSpacePos) const
{
    if (IsSlotValid(nIndex))
    {
        CEntityObject* pSlot = Slot(nIndex);
        if (pSlot)
        {
            pSlot->GetCameraSpacePos(cameraSpacePos);
            return;
        }
    }
    cameraSpacePos = ZERO;
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& CComponentRender::GetSlotWorldTM(int nIndex) const
{
    static Matrix34 temp;
    if ((nIndex & ~ENTITY_SLOT_ACTUAL) < PARTID_CGA)
    {
        IStatObj* pCompObj;
        IStatObj::SSubObject* pSubObj;
        if (!(nIndex & ENTITY_SLOT_ACTUAL) && (pCompObj = GetCompoundObj()))
        {
            return (pSubObj = pCompObj->GetSubObject(nIndex)) ?
                   (temp = Slot(0)->GetWorldTM(m_pEntity) * pSubObj->tm) : Slot(0)->GetWorldTM(m_pEntity);
        }
        nIndex &= ~ENTITY_SLOT_ACTUAL;
        if (IsSlotValid(nIndex))
        {
            return Slot(nIndex)->GetWorldTM(m_pEntity);
        }
    }
    else
    {
        int nSlot, nSubSlot = nIndex % PARTID_CGA;
        nSlot = nIndex / PARTID_CGA - 1;
        if (IsSlotValid(nSlot) && Slot(nSlot)->pCharacter)
        {
            return temp = Slot(nSlot)->GetWorldTM(m_pEntity) *  Matrix34(Slot(nSlot)->pCharacter->GetISkeletonPose()->GetAbsJointByID(nSubSlot));
        }
    }
    temp.SetIdentity();
    return temp;
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& CComponentRender::GetSlotLocalTM(int nIndex, bool bRelativeToParent) const
{
    static Matrix34 temp;
    if ((nIndex & ~ENTITY_SLOT_ACTUAL) >= PARTID_CGA)
    {
        int nSlot, nSubSlot = nIndex % PARTID_CGA;
        nSlot = nIndex / PARTID_CGA - 1;
        if (IsSlotValid(nSlot) && Slot(nSlot)->pCharacter)
        {
            if (Slot(nSlot)->pCharacter)
            {
                return temp = Slot(nSlot)->GetLocalTM() *   Matrix34(Slot(nSlot)->pCharacter->GetISkeletonPose()->GetAbsJointByID(nSubSlot));
            }
        }
    }
    else
    {
        IStatObj* pCompObj;
        IStatObj::SSubObject* pSubObj;

        if (!(nIndex & ENTITY_SLOT_ACTUAL) && (pCompObj = GetCompoundObj()))
        {
            return (pSubObj = pCompObj->GetSubObject(nIndex)) ? (temp = Slot(0)->GetLocalTM() * pSubObj->tm) : Slot(0)->GetLocalTM();
        }
        else if (IsSlotValid(nIndex &= ~ENTITY_SLOT_ACTUAL))
        {// Check if this slot has a parent
            if (Slot(nIndex)->pParent && !bRelativeToParent)
            {
                // Combine all local transforms hierarchy.
                temp = Slot(nIndex)->GetLocalTM();
                CEntityObject* pParentSlot = Slot(nIndex);
                while (pParentSlot = pParentSlot->pParent)
                {
                    temp = pParentSlot->GetLocalTM() * temp;
                }
                return temp;
            }
            return Slot(nIndex)->GetLocalTM();
        }
    }
    temp.SetIdentity();
    return temp;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetLocalBounds(const AABB& bounds, bool bDoNotRecalculate)
{
    m_localBBox = bounds;
    if (bDoNotRecalculate)
    {
        AddFlags(FLAG_BBOX_FORCED);
        ClearFlags(FLAG_BBOX_INVALID);
    }
    else
    {
        ClearFlags(FLAG_BBOX_FORCED);
    }
    AddFlags(FLAG_BBOX_VALID_LOCAL);
    InvalidateBounds(true, false);
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::InvalidateLocalBounds()
{
    InvalidateBounds(true, true);
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::CalcLocalBounds()
{
    // If local bounding box is forced from outside, do not calculate it automatically.
    if (CheckFlags(FLAG_BBOX_FORCED))
    {
        return;
    }

    bool bBBoxInitialized = false;
    AABB box;
    m_localBBox.Reset();
    int nSlots = GetSlotCount();
    for (int i = 0; i < nSlots; i++)
    {
        CEntityObject* pSlot = Slot(i);
        if (pSlot && (pSlot->flags & ENTITY_SLOT_RENDER)) // Only renderable slots contribute to the bounding box.
        {
            box.Reset();
            if (pSlot->GetLocalBounds(box))
            {
                CheckIfBBoxValid(box, m_pEntity);

                // Check if slot have transformation.
                if (pSlot->HaveLocalMatrix())
                {
                    // If offset matrix is used transform slot bounds by slot local offset matrix.
                    box.SetTransformedAABB(GetSlotLocalTM(i, false), box);
                }

                CheckIfBBoxValid(box, m_pEntity);

                // Add local slot bounds to local rendering bounds.
                m_localBBox.Add(box.min);
                m_localBBox.Add(box.max);
                bBBoxInitialized = true;
            }
        }
    }

    if (!bBBoxInitialized)
    {
        // If bounding box not initialized, this entity should not be rendered.
        // @TODO: unregister from sector?
        AddFlags(FLAG_BBOX_INVALID);
        m_localBBox.min.Set(0, 0, 0);
        m_localBBox.max.Set(0, 0, 0);
    }
    else
    {
        ClearFlags(FLAG_BBOX_INVALID);
    }
    AddFlags(FLAG_BBOX_VALID_LOCAL);
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::GetLocalBounds(AABB& bbox)
{
    if (!CheckFlags(FLAG_BBOX_VALID_LOCAL))
    {
        CalcLocalBounds();
    }
    bbox = m_localBBox;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::GetWorldBounds(AABB& bbox)
{
    if (!CheckFlags(FLAG_BBOX_VALID_LOCAL))
    {
        CalcLocalBounds();
    }
    bbox = m_localBBox;
    bbox.SetTransformedAABB(m_pEntity->GetWorldTM(), bbox);

    CheckIfBBoxValid(bbox, m_pEntity);
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::ProcessEvent(SEntityEvent& event)
{
    // Pass events to slots.
    for (uint32 i = 0, num = m_slots.size(); i < num; i++)
    {
        if (m_slots[i])
        {
            m_slots[i]->OnEntityEvent(m_pEntity, event);
        }
    }

    switch (event.event)
    {
    case ENTITY_EVENT_XFORM:
        OnEntityXForm((int)event.nParam[0]);
        break;
    case ENTITY_EVENT_HIDE:
    case ENTITY_EVENT_INVISIBLE:
        OnHide(true);
        break;
    case ENTITY_EVENT_UNHIDE:
    case ENTITY_EVENT_VISIBLE:
        OnHide(false);
        break;
    case ENTITY_EVENT_NOT_SEEN_TIMEOUT:
    {
        if (CVar::pDebugNotSeenTimeout->GetIVal())
        {
            CryLogAlways("Render component not seen: id = \"0x%X\" entity = \"%s\" class = \"%s\"", m_pEntity->GetId(), m_pEntity->GetName(), m_pEntity->GetClass()->GetName());
        }

        for (Slots::iterator it = m_slots.begin(), endit = m_slots.end(); it != endit; ++it)
        {
            CEntityObject* pSlot = *it;
            if (pSlot && (pSlot->flags & ENTITY_SLOT_RENDER))
            {
                pSlot->OnNotSeenTimeout();
            }
            if (pSlot && pSlot->pCharacter)
            {
                pSlot->pCharacter->KillAllSkeletonEffects();
            }
        }
    }
    break;
    case ENTITY_EVENT_MATERIAL:
        SetCustomMaterial((_smart_ptr<IMaterial>) reinterpret_cast<IMaterial*>(event.nParam[0]));
        break;
    case ENTITY_EVENT_ANIM_EVENT:
    {
        const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
        if (pAnimEvent)
        {
            ICharacterInstance* pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);
            //              const char* eventName = (pAnimEvent ? pAnimEvent->m_EventName : 0);

            // If the event is an effect event, play the event.
            ISkeletonAnim* pSkeletonAnim = (pCharacter ? pCharacter->GetISkeletonAnim() : 0);
            ISkeletonPose* pSkeletonPose = (pCharacter ? pCharacter->GetISkeletonPose() : 0);
            if (!CheckFlags(FLAG_HIDDEN) && pSkeletonAnim && pAnimEvent->m_EventName && _stricmp(pAnimEvent->m_EventName, "effect") == 0 && pAnimEvent->m_CustomParameter)
            {
                for (Slots::iterator it = m_slots.begin(), endit = m_slots.end(); it != endit; ++it)
                {
                    CEntityObject* pSlot = *it;
                    if (pSlot && pSlot->pCharacter == pCharacter)
                    {
                        pSlot->pCharacter->SpawnSkeletonEffect(pAnimEvent->m_AnimID, pAnimEvent->m_AnimPathName, pAnimEvent->m_CustomParameter, pAnimEvent->m_BonePathName, pAnimEvent->m_SecondBonePathName, pAnimEvent->m_vOffset, pAnimEvent->m_vDir, QuatTS(pSlot->m_worldTM));
                    }
                }
            }
        }
    }
    break;
    case ENTITY_EVENT_RESET:
        OnReset();
        break;
    case ENTITY_EVENT_PREPHYSICSUPDATE:
        for (size_t i = 0, end = m_queuedGeometryChanges.size(); i < end; ++i)
        {
            SetSlotGeometry(m_queuedGeometryChanges[i].first, m_queuedGeometryChanges[i].second);
        }
        m_queuedGeometryChanges.clear();

        // Note: at some point we might want disable this controllably.
        // m_pEntity->PrePhysicsActivate( false );

        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::OnEntityXForm(int nWhyFlags)
{
    if (!(nWhyFlags & ENTITY_XFORM_NOT_REREGISTER)) // A special optimization for characters
    {
        InvalidateBounds(false, true);
    }

    // Invalidate cached world matrices for all slots.
    for (uint32 i = 0, num = m_slots.size(); i < num; i++)
    {
        if (m_slots[i])
        {
            m_slots[i]->OnXForm(m_pEntity);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::OnHide(bool bHide)
{
    if (CheckFlags(FLAG_HIDDEN) != bHide)
    {
        //@TODO: uncomment this.
        //SetRndFlags(ERF_HIDDEN,bHide);
        if (bHide)
        {
            AddFlags(FLAG_HIDDEN);
            RegisterForRendering(false);
        }
        else
        {
            ClearFlags(FLAG_HIDDEN);
            RegisterForRendering(true);
        }
        OnEntityXForm(0);

        // update hidden flag for all slots rendering their stuff explicitly
        for (uint32 i = 0; i < m_slots.size(); i++)
        {
            if (m_slots[i])
            {
                IRenderNode* pRenderNode = m_slots[i]->pChildRenderNode;
                const uint32 mask = (1 << eERType_FogVolume) | (1 << eERType_ParticleEmitter) | (1 << eERType_Cloud) | (1 << eERType_VolumeObject);
                if (pRenderNode && (1 << pRenderNode->GetRenderNodeType()) & mask)
                {
                    pRenderNode->Hide(bHide);
                }

                if (m_slots[i]->pLight)
                {
                    CDLight& cdLight = m_slots[i]->pLight->GetLightProperties();
                    if (bHide)
                    {
                        cdLight.m_Flags |= DLF_DISABLED;
                    }
                    else
                    {
                        cdLight.m_Flags &= ~DLF_DISABLED;
                    }
                }
            }
        }
    }
}


void CComponentRender::SetLayerId(uint16 nLayerId)
{
    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        if (CEntityObject* obj = m_slots[i])
        {
            IRenderNode* pRenderNode = obj->pChildRenderNode;
            const uint32 mask = (1 << eERType_FogVolume) | (1 << eERType_ParticleEmitter) | (1 << eERType_Cloud) | (1 << eERType_VolumeObject);
            if (pRenderNode && (1 << pRenderNode->GetRenderNodeType()) & mask)
            {
                pRenderNode->SetLayerId(nLayerId);
            }

            if (obj->pLight)
            {
                obj->pLight->SetLayerId(nLayerId);
            }
        }
    }
}
//////////////////////////////////////////////////////////////////

void CComponentRender::OnReset()
{
    m_nHUDSilhouettesParams = 0;
}

//////////////////////////////////////////////////////////////////

void CComponentRender::UpdateMaterialLayersRendParams(SRendParams& pRenderParams, const SRenderingPassInfo& passInfo)
{
    if (passInfo.IsShadowPass() && !(m_nFlags & FLAG_SHADOW_DISSOLVE))
    {
        // No update during shadow rendering
        pRenderParams.nMaterialLayers = 0;
        pRenderParams.nMaterialLayersBlend = 0;
        return;
    }

    uint8 nMaterialLayers = m_nMaterialLayers;
    if (nMaterialLayers || m_nMaterialLayersBlend)
    {
        // Disable rain layer if inside vis area
        if (nMaterialLayers & MTL_LAYER_WET || m_nMaterialLayersBlend & MTL_LAYER_BLEND_WET)
        {
            IVisArea* pEntityVisArea = GetEntityVisArea();
            if (pEntityVisArea && !pEntityVisArea->IsPortal())
            {
                nMaterialLayers &= ~MTL_LAYER_WET;
            }
        }

        float fFrameTime = 255.0f * gEnv->pTimer->GetFrameTime();

        float nSrcBlend = 0.0f, nDstBlend = 0.0f;
        float nFinalBlendF = (nMaterialLayers & MTL_LAYER_FROZEN) ? 255.0f : 0.0f;

        nSrcBlend = (float)((m_nMaterialLayersBlend & MTL_LAYER_BLEND_WET) >> 16);
        // account for low end bit missing from wet blend
        nSrcBlend *= (255.0f / 254.0f);
        nDstBlend = (nMaterialLayers & MTL_LAYER_WET) ? 255.f : 0.f;
        float nFinalBlendW = nDstBlend;
        if (nSrcBlend != nDstBlend)
        {
            nFinalBlendW = (nSrcBlend < nDstBlend) ? nSrcBlend + fFrameTime : nSrcBlend - fFrameTime;
            nFinalBlendW = clamp_tpl<float>(nFinalBlendW, 0.0f, 255.0f);
        }

        nSrcBlend = (float)((m_nMaterialLayersBlend & MTL_LAYER_BLEND_DYNAMICFROZEN));
        nDstBlend = (nMaterialLayers & MTL_LAYER_DYNAMICFROZEN) ? 255.0f : 0.0f;
        float nFinalBlendDynF = nDstBlend;
        if (nSrcBlend != nDstBlend)
        {
            nFinalBlendDynF = (nSrcBlend < nDstBlend) ? nSrcBlend + fFrameTime : nSrcBlend - fFrameTime;
            nFinalBlendDynF = clamp_tpl<float>(nFinalBlendDynF, 0.0f, 255.0f);
        }

        m_nMaterialLayersBlend = (uint32) ((int_round(nFinalBlendF)) << 24) | (((int_round(nFinalBlendW)) << 16) & MTL_LAYER_BLEND_WET) | ((int_round(0.0f)) << 8) | (int_round(nFinalBlendDynF));

        pRenderParams.nMaterialLayers = nMaterialLayers;
        pRenderParams.nMaterialLayersBlend = m_nMaterialLayersBlend;
    }
}

void CComponentRender::Render(const SRendParams& inRenderParams, const SRenderingPassInfo& passInfo)
{
    if (CanExecuteRenderAsJob())
    {
#if defined(SUPPORT_COMPONENTRENDER_RENDER_JOB)
        // legacy job priority: "passInfo.IsGeneralPass() ? JobManager::eRegularPriority : JobManager::eLowPriority"
        AZ::LegacyJobExecutor *pJobExecutor = passInfo.IsShadowPass() ? gEnv->pRenderer->GetGenerateShadowRendItemJobExecutor(passInfo.ThreadID()) : gEnv->pRenderer->GetGenerateRendItemJobExecutor(passInfo.ThreadID());
        pJobExecutor->StartJob(
            [this, inRenderParams, passInfo]
            {
                this->Render_JobEntry(inRenderParams, passInfo);
            }
        );
#else
        __debugbreak(); // unsupported code path
#endif // SUPPORT_COMPONENTRENDER_RENDER_JOB
    }
    else
    {
        Render_JobEntry(inRenderParams, passInfo);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::Render_JobEntry(const SRendParams& inRenderParams, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    // Not draw if invalid bounding box.
    if (m_nFlags & (FLAG_HIDDEN | FLAG_BBOX_INVALID))
    {
        return; //false;
    }
    // Make a local copy of render params as some of the parameters will be modified.
    SRendParams rParams(inRenderParams);

    if (m_Callbacks.size() > 0)
    {
        // if we have callbacks be sure the shader params are properly initialized
        IShaderPublicParams* pParams = GetShaderPublicParams(true);

        // get the current active material
        _smart_ptr<IMaterial> pMaterial = m_pEntity->GetMaterial();
        if (pMaterial == NULL)
        {
            pMaterial = GetSlotMaterial(0);
        }

        TCallbackVector::iterator itEnd = m_Callbacks.end();
        for (TCallbackVector::iterator it = m_Callbacks.begin(); it != itEnd; ++it)
        {
            (*it)->SetupShaderParams(pParams, pMaterial);
        }
    }

    if (m_pShaderPublicParams)
    {
        m_pShaderPublicParams->AssignToRenderParams(rParams);
    }

    if (m_pMaterial)
    {
        rParams.pMaterial = m_pMaterial;
    }

    rParams.nHUDSilhouettesParams = m_nHUDSilhouettesParams;

    UpdateMaterialLayersRendParams(rParams, passInfo);

    rParams.fAlpha = GetOpacity();

    if (m_nEntityFlags & ENTITY_FLAG_SEND_RENDER_EVENT)
    {
        SEntityEvent event(ENTITY_EVENT_RENDER);
        event.nParam[0] = (INT_PTR)&inRenderParams;
        m_pEntity->SendEvent(event);
        m_fLastSeenTime = gEnv->pTimer->GetCurrTime();
    }
    if (s_pTimeoutList && (m_nEntityFlags & ENTITY_FLAG_SEND_NOT_SEEN_TIMEOUT))
    {
        s_pTimeoutList->ResetTimeout(m_entityId);
    }

    if (GetEntityVisArea())
    {
        rParams.dwFObjFlags |= FOB_IN_DOORS;
    }

    //////////////////////////////////////////////////////////////////////////
    // Draw all slots.
    for (Slots::iterator it = m_slots.begin(), endit = m_slots.end(); it != endit; ++it)
    {
        CEntityObject* pSlot = *it;
        if (pSlot && (pSlot->flags & ENTITY_SLOT_RENDER))
        {
            rParams.lodValue = pSlot->ComputeLod(inRenderParams.lodValue.LodA(), passInfo);
            pSlot->Render(m_pEntity, rParams, m_dwRndFlags, this, passInfo);
        }
    }
}


IStatObj* CComponentRender::GetEntityStatObj(unsigned int nPartId, unsigned int nSubPartId, Matrix34A* pMatrix /* = NULL */, bool bReturnOnlyVisible /* = false */)
{
    if (!IsSlotValid(nPartId))
    {
        return NULL;
    }

    if (pMatrix)
    {
        *pMatrix = GetSlotWorldTM(nPartId | ENTITY_SLOT_ACTUAL);
    }

    if (!bReturnOnlyVisible || Slot(nPartId)->flags & ENTITY_SLOT_RENDER)
    {
        return Slot(nPartId)->pStatObj;
    }

    return NULL;
}

ICharacterInstance* CComponentRender::GetEntityCharacter(unsigned int nSlot, Matrix34A* pMatrix /* = NULL */, bool bReturnOnlyVisible /* = false */)
{
    if (!IsSlotValid(nSlot))
    {
        return NULL;
    }
    if (pMatrix)
    {
        *pMatrix = GetSlotWorldTM(nSlot);
    }

    if (!bReturnOnlyVisible || Slot(nSlot)->flags & ENTITY_SLOT_RENDER)
    {
        return Slot(nSlot)->pCharacter;
    }

    return NULL;
}

#if defined(USE_GEOM_CACHES)
IGeomCacheRenderNode* CComponentRender::GetGeomCacheRenderNode(unsigned int nSlot, Matrix34A* pMatrix, bool bReturnOnlyVisible)
{
#if defined(USE_GEOM_CACHES)
    if (!IsSlotValid(nSlot))
    {
        return NULL;
    }
    if (pMatrix)
    {
        *pMatrix = GetSlotWorldTM(nSlot);
    }

    if (!bReturnOnlyVisible || Slot(nSlot)->flags & ENTITY_SLOT_RENDER)
    {
        return Slot(nSlot)->GetGeomCacheRenderNode();
    }
#endif
    return NULL;
}
#endif

_smart_ptr<IMaterial> CComponentRender::GetEntitySlotMaterial(unsigned int nPartId, bool bReturnOnlyVisible, bool* pbDrawNear)
{
    if (!IsSlotValid(nPartId))
    {
        return NULL;
    }

    if (pbDrawNear)
    {
        *pbDrawNear = (Slot(nPartId)->flags & (ENTITY_SLOT_RENDER_NEAREST | ENTITY_SLOT_RENDER_AFTER_POSTPROCESSING)) != 0;
    }

    if (!bReturnOnlyVisible || Slot(nPartId)->flags & ENTITY_SLOT_RENDER)
    {
        return Slot(nPartId)->pMaterial;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::UpdateComponent(SEntityUpdateContext& ctx)
{
    if (!CheckFlags(FLAG_UPDATE))
    {
        return;
    }

    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    bool bBoundsChanged = false;
    //////////////////////////////////////////////////////////////////////////
    // Update all slots.
    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        if (m_slots[i] && m_slots[i]->bUpdate)
        {
            bool bSlotBoundsChanged = false;

            m_slots[i]->Update(m_pEntity, CheckFlags(FLAG_NOW_VISIBLE), bSlotBoundsChanged);

            if (bSlotBoundsChanged)
            {
                bBoundsChanged = true;
            }
        }
    }
    if (bBoundsChanged)
    {
        InvalidateBounds(true, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetCustomMaterial(_smart_ptr<IMaterial> pMaterial)
{
    m_pMaterial = pMaterial;

    if (m_nFlags & FLAG_HAS_LIGHTS)
    {
        for (uint32 i = 0; i < m_slots.size(); i++)
        {
            CEntityObject* pSlot = m_slots[i];
            if (pSlot && pSlot->pLight)
            {
                if (pSlot->pMaterial)
                {
                    pSlot->pLight->SetMaterial(pSlot->pMaterial);
                }
                else
                {
                    pSlot->pLight->SetMaterial(m_pMaterial);
                }
            }
        }
    }
    else if (m_nFlags & FLAG_HAS_CHILDRENDERNODES)
    {
        for (uint32 i = 0; i < m_slots.size(); i++)
        {
            CEntityObject* pSlot(m_slots[i]);
            IRenderNode* pChildRenderNode(pSlot ? pSlot->pChildRenderNode : 0);
            if (pChildRenderNode)
            {
                pChildRenderNode->SetMaterial(m_pMaterial);
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////
void CComponentRender::InvalidateBounds(bool bLocal, bool bWorld)
{
    if (bLocal)
    {
        ClearFlags(FLAG_BBOX_VALID_LOCAL);
    }

    // Check if initialization of entity already finished.
    if (CheckFlags(FLAG_POST_INIT))
    {
        // World bounding box changed.
        RegisterForRendering(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::RegisterCharactersForRendering()
{
    // remove potential duplicates
    AZStd::sort(s_arrCharactersToRegisterForRendering.begin(), s_arrCharactersToRegisterForRendering.end());
    std::vector<CComponentRender*>::iterator itEnd = std::unique(s_arrCharactersToRegisterForRendering.begin(), s_arrCharactersToRegisterForRendering.end());

    for (std::vector<CComponentRender*>::iterator it = s_arrCharactersToRegisterForRendering.begin(); it != itEnd; ++it)
    {
        CComponentRender* pComponent = *it;

        pComponent->CalcLocalBounds();
        bool bBoxEmpty = pComponent->m_localBBox.IsEmpty();

        if (!pComponent->CheckFlags(FLAG_BBOX_INVALID) && !bBoxEmpty)
        {
            pComponent->m_WSBBox.SetTransformedAABB(pComponent->m_pEntity->GetWorldTM(), pComponent->m_localBBox);

            pComponent->AddFlags(FLAG_REGISTERED_IN_3DENGINE);

            // register this entity for rendering
            float fObjRadiusSqr = pComponent->m_WSBBox.GetRadiusSqr();
            if (fObjRadiusSqr < 100000000.f && _finite(fObjRadiusSqr))
            {
                pComponent->GetI3DEngine()->RegisterEntity(pComponent);
            }
            else
            {
                EntityWarning("CComponentRender::RegisterForRendering: Object has invalid bbox: name: %s, class name: %s, GetRadius() = %.2f", pComponent->GetName(), pComponent->GetEntityClassName(), fObjRadiusSqr);
            }
        }
    }
    s_arrCharactersToRegisterForRendering.clear();
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::RegisterForRendering(bool bEnable)
{
    if (bEnable)
    {
        // If entity is hidden, it should not register in 3d engine.
        if (CheckFlags(FLAG_HIDDEN))
        {
            // Register for 3D engine rendering.
            if (CheckFlags(FLAG_REGISTERED_IN_3DENGINE))
            {
                GetI3DEngine()->FreeRenderNodeState(this); // internally calls UnRegisterEntity(this)
                ClearFlags(FLAG_REGISTERED_IN_3DENGINE);
            }
            return;
        }

        int nSlotCount = GetSlotCount();
        for (int i = 0; i < nSlotCount; ++i)
        {
            if (GetCharacter(i))
            {
                s_arrCharactersToRegisterForRendering.push_back(this);
                return;
            }
        }

        if (!CheckFlags(FLAG_BBOX_VALID_LOCAL))
        {
            CalcLocalBounds();
        }
        bool bBoxEmpty = m_localBBox.IsEmpty();

        if (!CheckFlags(FLAG_BBOX_INVALID) && !bBoxEmpty)
        {
            m_WSBBox.SetTransformedAABB(m_pEntity->GetWorldTM(), m_localBBox);

            AddFlags(FLAG_REGISTERED_IN_3DENGINE);

            // register this entity for rendering
            float fObjRadiusSqr = m_WSBBox.GetRadiusSqr();
            if (fObjRadiusSqr < 100000000.f && _finite(fObjRadiusSqr))
            {
                GetI3DEngine()->RegisterEntity(this);
            }
            else
            {
                EntityWarning("CComponentRender::RegisterForRendering: Object has invalid bbox: name: %s, class name: %s, GetRadius() = %.2f", GetName(), GetEntityClassName(), fObjRadiusSqr);
            }
        }
    }
    else if (CheckFlags(FLAG_REGISTERED_IN_3DENGINE))
    {
        // Unregister from 3D engine rendering.
        GetI3DEngine()->FreeRenderNodeState(this); // internally calls UnRegisterEntity(this)
        ClearFlags(FLAG_REGISTERED_IN_3DENGINE);
    }
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::GetSlotId(CEntityObject* pSlot) const
{
    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        if (m_slots[i] == pSlot)
        {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentRender::GetSlotInfo(int nIndex, SEntitySlotInfo& slotInfo) const
{
    if (!IsSlotValid(nIndex))
    {
        return false;
    }
    CEntityObject* pSlot = Slot(nIndex);
    slotInfo.nFlags = pSlot->flags;
    if (pSlot->pParent)
    {
        slotInfo.nParentSlot = GetSlotId(pSlot->pParent);
    }
    else
    {
        slotInfo.nParentSlot = -1;
    }

    slotInfo.pStatObj = pSlot->pStatObj;
    slotInfo.pCharacter = pSlot->pCharacter;
    slotInfo.pLight = pSlot->pLight;
    slotInfo.pMaterial = pSlot->pMaterial;
    slotInfo.pLocalTM = &pSlot->GetLocalTM();
    slotInfo.pWorldTM = &pSlot->GetWorldTM(m_pEntity);
    slotInfo.pChildRenderNode = pSlot->pChildRenderNode;
    slotInfo.pParticleEmitter = pSlot->GetParticleEmitter();
#if defined(USE_GEOM_CACHES)
    slotInfo.pGeomCacheRenderNode =
        (pSlot->pChildRenderNode && pSlot->pChildRenderNode->GetRenderNodeType() == eERType_GeomCache)
        ? static_cast<IGeomCacheRenderNode*>(pSlot->pChildRenderNode) : NULL;
#endif
    slotInfo.nSubObjHideMask = pSlot->nSubObjHideMask;
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetSlotFlags(int nSlot, uint32 nFlags)
{
    if (nSlot < 0)
    {
        // All slots.
        for (Slots::iterator it = m_slots.begin(), endit = m_slots.end(); it != endit; ++it)
        {
            CEntityObject* pSlot = *it;
            if (pSlot)
            {
                pSlot->SetFlags(nFlags);
            }
        }
    }
    else if (IsSlotValid(nSlot))
    {
        CEntityObject* pSlot = Slot(nSlot);
        if (nFlags & ENTITY_SLOT_RENDER_NEAREST)
        {
            SetLodRatio(0); // Use LOD 0 on nearest objects
        }
        else if (pSlot->flags & ENTITY_SLOT_RENDER_NEAREST) // If prev slot flags has this set we must be turning it off
        {
            SetLodRatio(100); // Set default lod distance settings back if turning off ENTITY_SLOT_RENDER_NEAREST

            // Delete m_pCameraSpacePos here so that if this flag is turned on again and we don't have a camera space
            // position then it will default to calculate the camera space pos from the WS pos
            pSlot->FreeCameraSpacePos();
        }

        pSlot->SetFlags(nFlags);
    }
    //if (nFlags & (ENTITY_SLOT_RENDER|ENTITY_SLOT_RENDER_NEAREST))
    {
        // If any slot to be rendered, make sure it is registered in 3d engine.
        //if (!CheckFlags(FLAG_REGISTERED_IN_3DENGINE))
        {
            InvalidateBounds(true, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
uint32 CComponentRender::GetSlotFlags(int nSlot) const
{
    if (IsSlotValid(nSlot))
    {
        return Slot(nSlot)->flags;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetSlotMaterial(int nSlot, _smart_ptr<IMaterial> pMaterial)
{
    if (IsSlotValid(nSlot))
    {
        CEntityObject* pSlot = Slot(nSlot);
        pSlot->pMaterial = pMaterial;

        if (pSlot->pLight)
        {
            pSlot->pLight->SetMaterial(pMaterial);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CComponentRender::GetSlotMaterial(int nSlot)
{
    if (IsSlotValid(nSlot))
    {
        return Slot(nSlot)->pMaterial;
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::SetSlotGeometry(int nSlot, IStatObj* pStatObj)
{
    if (m_nInternalFlags & DECAL_OWNER)
    {
        m_nInternalFlags |= UPDATE_DECALS;
    }

    if ((nSlot & ~ENTITY_SLOT_ACTUAL) < PARTID_CGA)
    {
        IStatObj* pCompObj = 0;
        if (!(nSlot & ENTITY_SLOT_ACTUAL))
        {
            pCompObj = GetCompoundObj();
        }
        else
        {
            nSlot &= ~ENTITY_SLOT_ACTUAL;
        }
        if (pCompObj && (!pStatObj || !(pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND)))
        {
            if (nSlot < 0)
            {
                nSlot = pCompObj->GetSubObjectCount();
            }
            if (!(pCompObj->GetFlags() & STATIC_OBJECT_GENERATED))
            {
                IStatObj* pNewCompObj = pCompObj->Clone(false, false, true);
                pNewCompObj->SetFlags(pNewCompObj->GetFlags() | STATIC_OBJECT_GENERATED);
                pNewCompObj->AddRef();
                Slot(0)->ReleaseObjects();
                Slot(0)->pStatObj = pCompObj = pNewCompObj;
            }
            IStatObj::SSubObject* pSubObj;
            if (!(pSubObj = pCompObj->GetSubObject(nSlot)))
            {
                pCompObj->SetSubObjectCount(nSlot + 1);
                pSubObj = pCompObj->GetSubObject(nSlot);
            }
            else if (pSubObj->pStatObj == pStatObj)
            {
                pCompObj->Invalidate(false);
                return nSlot;
            }
            if (!(pSubObj->bHidden = pStatObj == 0))
            {
                pStatObj->AddRef();
            }
            if (pSubObj->pStatObj)
            {
                pSubObj->pStatObj->Release();
            }
            pSubObj->pStatObj = pStatObj;
            pCompObj->Invalidate(false);
        }
        else
        {
            CEntityObject* pSlot = GetOrMakeSlot(nSlot);

            // Check if loading the same object.
            if (pSlot->pStatObj == pStatObj)
            {
                InvalidateBounds(true, true);
                return nSlot;
            }

            if (pStatObj)
            {
                pStatObj->AddRef();
            }

            pSlot->ReleaseObjects();

            if (pStatObj)
            {
                pSlot->flags |= ENTITY_SLOT_RENDER;
            }
            else
            {
                pSlot->flags &= ~ENTITY_SLOT_RENDER;
            }

            pSlot->pStatObj = pStatObj;
            pSlot->bUpdate = false;
        }
    }
    else
    {
        int nCGASlot = nSlot / PARTID_CGA - 1, nSubSlot = nSlot % PARTID_CGA;
        CEntityObject* pSlot = GetOrMakeSlot(nCGASlot);

        if (!pSlot->pCharacter || pSlot->pCharacter->GetISkeletonPose()->GetStatObjOnJoint(nSubSlot) == pStatObj)
        {
            return nSlot;
        }
        pSlot->pCharacter->GetISkeletonPose()->SetStatObjOnJoint(nSubSlot, pStatObj);
    }

    InvalidateBounds(true, true);
    UpdateLodDistance(gEnv->p3DEngine->GetFrameLodInfo());

    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::SetSlotCharacter(int nSlot, ICharacterInstance* pCharacter)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);

    // Check if loading the same character
    if (pSlot->pCharacter == pCharacter)
    {
        return nSlot;
    }

    if (pCharacter)
    {
        pCharacter->AddRef();
    }

    pSlot->ReleaseObjects();

    // The character instance was destroyed, the ShaderParamCallbacks are not valid.
    ClearShaderParamCallbacks();

    if (pCharacter)
    {
        pSlot->pCharacter = pCharacter;
        pSlot->flags |= ENTITY_SLOT_RENDER;
        pSlot->bUpdate = true;
        m_nFlags |= FLAG_UPDATE; // For Characters we need render component to be updated.

        pCharacter->SetFlags(pCharacter->GetFlags() | CS_FLAG_UPDATE);

        // Register ourselves to listen for animation events coming from the character.
        if (ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim())
        {
            pSkeletonAnim->SetEventCallback(AnimEventCallback, this);
        }

        // Characters with morphs have buffers that should be cleaned up after a period of time, so set this flag to receive
        // callbacks when we are not rendered for a certain period of time.
        if (m_pEntity && CheckCharacterForMorphs(pCharacter))
        {
            m_pEntity->SetFlags(m_pEntity->GetFlags() | ENTITY_FLAG_SEND_NOT_SEEN_TIMEOUT);
            if (CVar::pDebugNotSeenTimeout->GetIVal())
            {
                CryLogAlways("CComponentRender::SetSlotCharacter(): Setting ENTITY_FLAG_SEND_NOT_SEEN_TIMEOUT. id = \"0x%X\" entity = \"%s\" class = \"%s\"", m_pEntity->GetId(), m_pEntity->GetName(), m_pEntity->GetClass()->GetName());
            }
        }

        InvalidateBounds(true, true);

        // As we destroyed the previous character instance and created a new one, we must recreate all the ShaderParamCallback
        CheckShaderParamCallbacks();
    }
    else
    {
        pSlot->flags &= ~ENTITY_SLOT_RENDER;
        pSlot->bUpdate = false;
        RegisterForRendering(false);
    }

    UpdateLodDistance(gEnv->p3DEngine->GetFrameLodInfo());

    return (pCharacter) ? nSlot : -1;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::LoadGeometry(int nSlot, const char* sFilename, const char* sGeomName, int nLoadFlags)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);

    if (!sFilename || (sFilename[0] == 0))
    {
        EntityWarning("[ComponentRender::LoadGeometry] Called with empty filename, Entity: %s", m_pEntity->GetEntityTextDescription());
        return -1;
    }

    // Check if loading the same object.
    if ((pSlot->pStatObj) && !(pSlot->pStatObj->GetFlags() & (STATIC_OBJECT_CLONE | STATIC_OBJECT_GENERATED)) && (pSlot->pStatObj->IsSameObject(sFilename, sGeomName)))
    {
        return nSlot;
    }

    IStatObj* pStatObj;
    if (sGeomName && sGeomName[0])
    {
        IStatObj::SSubObject* pSubObject = 0;
        pStatObj = GetI3DEngine()->LoadStatObjUnsafeManualRef(sFilename, sGeomName, &pSubObject);
        if (pStatObj)
        {
            // Will keep a reference to the stat obj.
            pStatObj->AddRef();
        }
        else
        {
            //EntityFileWarning( sFilename,"Failed to load sub-object geometry (%s)", sGeomName );
            return -1;
        }
        if (pSubObject && !pSubObject->bIdentityMatrix)
        {
            // Set sub object matrix into the slot transformation matrix.
            pSlot->SetLocalTM(pSubObject->tm);
        }
    }
    else
    {
        pStatObj = GetI3DEngine()->LoadStatObjUnsafeManualRef(sFilename, NULL, NULL, (nLoadFlags& IEntity::EF_NO_STREAMING) == 0);
        if (pStatObj)
        {
            pStatObj->AddRef();
        }
    }

    pSlot->ReleaseObjects();
    pSlot->pStatObj = pStatObj;
    pSlot->flags |= ENTITY_SLOT_RENDER;
    pSlot->bUpdate = false;

    InvalidateBounds(true, true);
    UpdateLodDistance(gEnv->p3DEngine->GetFrameLodInfo());

    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::LoadCharacter(int nSlot, const char* sFilename, int nLoadFlags)
{
    if (!sFilename || (sFilename[0] == 0))
    {
        EntityWarning("[ComponentRender::LoadCharacter] Called with empty filename, Entity: %s", m_pEntity->GetEntityTextDescription());
        return -1;
    }

    //check the file extension to use the correct function: MakeCharacter for cgf/chr, LoadCharacterDefinition for cdf, AttachParts for cpt
    ICharacterInstance* pCharacter = gEnv->pCharacterManager->CreateInstance(sFilename);
    if (!pCharacter)
    {
        EntityWarning("[ComponentRender::LoadCharacter] Failed to create character instance Filename: %s, Entity: %s", sFilename, m_pEntity->GetEntityTextDescription());
        return -1;
    }

    return SetSlotCharacter(nSlot, pCharacter);
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::SetParticleEmitter(int nSlot, IParticleEmitter* pEmitter, bool bSerialize)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);
    pSlot->ReleaseObjects();
    pSlot->bSerialize = bSerialize;
    pEmitter->AddRef();
    pSlot->pChildRenderNode = pEmitter;

    pEmitter->SetEntity(m_pEntity, nSlot);

    if (pSlot->pMaterial)
    {
        pSlot->pChildRenderNode->SetMaterial(pSlot->pMaterial);
    }
    m_nFlags |= FLAG_UPDATE; // For Particles we need render component to be updated.
    m_nFlags |= FLAG_HAS_PARTICLES; //Mark as particles emitter

    pEmitter->SetViewDistanceMultiplier(GetViewDistanceMultiplier());
    pEmitter->Hide((GetFlags() & FLAG_HIDDEN) != 0);  // hide the emitter if the render component itself is hidden
    InvalidateBounds(true, true);

    UpdateEntityFlags();

    // If the entity is already hidden, be sure to reflect those changes while creating an emitter,
    // since we've already hidden our other ones.
    pEmitter->Hide(CheckFlags(FLAG_HIDDEN));

    return nSlot;
}

int CComponentRender::LoadParticleEmitter(int nSlot, IParticleEffect* pEffect, SpawnParams const* pParams, bool bPrime, bool bSerialize)
{
    assert(pEffect);

    if (pEffect)
    {
        // Spawn/attach params.
        SpawnParams params(pEffect->GetParticleParams().eAttachType, pEffect->GetParticleParams().eAttachForm);
        if (pParams)
        {
            params = *pParams;
        }

        Matrix34 const& TM = nSlot < 0 ? m_pEntity->GetWorldTM() : GetOrMakeSlot(nSlot)->GetWorldTM(m_pEntity);

        // Create emitter, or update existing.
        IParticleEmitter* pEmitter = GetParticleEmitter(nSlot);
        if (!pEmitter)
        {
            pEmitter = pEffect->Spawn(QuatTS(TM), 0, &params);
            if (!pEmitter)
            {
                return -1;
            }
            if (bPrime)
            {
                // Set emitter as though it's been running indefinitely.
                pEmitter->Prime();
            }
            nSlot = SetParticleEmitter(nSlot, pEmitter, bSerialize);
        }
        else
        {
            pEmitter->SetMatrix(TM);
            pEmitter->SetEffect(pEffect);
            pEmitter->SetSpawnParams(params);
        }
        return nSlot;
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::LoadLight(int nSlot, CDLight* pLight, uint16 layerId)
{
    assert(pLight);
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);

    if (pSlot->pMaterial)
    {
        //pLight->pMaterial = pSlot->pMaterial;
    }

    if (!pSlot->pLight)
    {
        pSlot->ReleaseObjects();
        pSlot->pLight = GetI3DEngine()->CreateLightSource();
    }

    if (pSlot->pLight)
    {
        pSlot->pLight->SetLightProperties(*pLight);
        if (pSlot->pMaterial)
        {
            pSlot->pLight->SetMaterial(pSlot->pMaterial);
        }
        else
        {
            pSlot->pLight->SetMaterial(m_pMaterial);
        }

        pSlot->pLight->SetViewDistanceMultiplier(GetViewDistanceMultiplier());
        pSlot->pLight->SetLodRatio(GetLodRatio());
        pSlot->pLight->SetMinSpec(GetMinSpec());
        pSlot->pLight->SetLayerId(layerId);
        pSlot->pLight->SetName(m_pEntity->GetName());
    }

    m_nFlags |= FLAG_HAS_LIGHTS;

    // Update light positions.
    pSlot->OnXForm(m_pEntity);

    pSlot->flags &= (~ENTITY_SLOT_RENDER);
    pSlot->bUpdate = false;
    InvalidateBounds(true, true);

    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::LoadCloud(int nSlot, const char* sFilename)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);

    pSlot->ReleaseObjects();
    ICloudRenderNode* pCloud = (ICloudRenderNode*)GetI3DEngine()->CreateRenderNode(eERType_Cloud);

    pSlot->pChildRenderNode = pCloud;

    pCloud->LoadCloud(sFilename);
    pCloud->SetMaterial(pCloud->GetMaterial());

    // Update slot position.
    pSlot->OnXForm(m_pEntity);

    m_nFlags |= FLAG_HAS_CHILDRENDERNODES;

    //pSlot->flags |= ENTITY_SLOT_RENDER;
    pSlot->bUpdate = false;
    InvalidateBounds(true, true);
    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::SetCloudMovementProperties(int nSlot, const SCloudMovementProperties& properties)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);
    IRenderNode* pRenderNode(pSlot->pChildRenderNode);
    if (pRenderNode && pRenderNode->GetRenderNodeType() == eERType_Cloud)
    {
        ICloudRenderNode* pCloud((ICloudRenderNode*)pRenderNode);
        pCloud->SetMovementProperties(properties);
    }
    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::LoadFogVolume(int nSlot, const SFogVolumeProperties& properties)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);

    IFogVolumeRenderNode* pFogVolume((IFogVolumeRenderNode*)pSlot->pChildRenderNode);
    if (0 == pFogVolume)
    {
        pSlot->ReleaseObjects();

        pFogVolume = (IFogVolumeRenderNode*) GetI3DEngine()->CreateRenderNode(eERType_FogVolume);
        pSlot->pChildRenderNode = pFogVolume;
    }

    assert(0 != pFogVolume);
    pFogVolume->SetFogVolumeProperties(properties);

    // Update slot position.
    pSlot->OnXForm(m_pEntity);

    //pSlot->flags |= ENTITY_SLOT_RENDER;
    pSlot->bUpdate = false;

    SetLocalBounds(AABB(-properties.m_size * 0.5f, properties.m_size * 0.5f), true);

    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::FadeGlobalDensity(int nSlot, float fadeTime, float newGlobalDensity)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);
    IRenderNode* pRenderNode(pSlot->pChildRenderNode);
    if (pRenderNode && pRenderNode->GetRenderNodeType() == eERType_FogVolume)
    {
        IFogVolumeRenderNode* pFogVolume((IFogVolumeRenderNode*)pRenderNode);
        pFogVolume->FadeGlobalDensity(fadeTime, newGlobalDensity);
    }
    return nSlot;
}

#if defined(USE_GEOM_CACHES)
//////////////////////////////////////////////////////////////////////////
int CComponentRender::LoadGeomCache(int nSlot, const char* sFilename)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);

    pSlot->ReleaseObjects();

    IGeomCacheRenderNode* pGeomCacheRenderNode = (IGeomCacheRenderNode*)GetI3DEngine()->CreateRenderNode(eERType_GeomCache);
    pGeomCacheRenderNode->LoadGeomCache(sFilename);

    pSlot->pChildRenderNode = pGeomCacheRenderNode;

    // Update slot position.
    pSlot->OnXForm(m_pEntity);
    m_nFlags |= FLAG_HAS_CHILDRENDERNODES;
    m_nFlags |= FLAG_UPDATE; // For geom caches we need render component to be updated.
    pSlot->flags = ENTITY_SLOT_RENDER;
    pSlot->bUpdate = true;
    InvalidateBounds(true, true);
    return nSlot;
}
#endif

//////////////////////////////////////////////////////////////////////////
int CComponentRender::LoadVolumeObject(int nSlot, const char* sFilename)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);

    pSlot->ReleaseObjects();
    IVolumeObjectRenderNode* pVolObj = (IVolumeObjectRenderNode*) GetI3DEngine()->CreateRenderNode(eERType_VolumeObject);

    pSlot->pChildRenderNode = pVolObj;

    pVolObj->LoadVolumeData(sFilename);
    if (m_pMaterial)
    {
        pVolObj->SetMaterial(m_pMaterial);
    }

    // Update slot position
    pSlot->OnXForm(m_pEntity);

    m_nFlags |= FLAG_HAS_CHILDRENDERNODES;

    //pSlot->flags |= ENTITY_SLOT_RENDER;
    pSlot->bUpdate = false;
    InvalidateBounds(true, true);

    return nSlot;
}

//////////////////////////////////////////////////////////////////////////
int CComponentRender::SetVolumeObjectMovementProperties(int nSlot, const SVolumeObjectMovementProperties& properties)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);
    IRenderNode* pRenderNode(pSlot->pChildRenderNode);
    if (pRenderNode && pRenderNode->GetRenderNodeType() == eERType_VolumeObject)
    {
        IVolumeObjectRenderNode* pVolObj((IVolumeObjectRenderNode*)pRenderNode);
        pVolObj->SetMovementProperties(properties);
    }
    return nSlot;
}

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
int CComponentRender::LoadPrismObject(int nSlot)
{
    CEntityObject* pSlot = GetOrMakeSlot(nSlot);

    pSlot->ReleaseObjects();
    IPrismRenderNode* pVolObj = (IPrismRenderNode*) GetI3DEngine()->CreateRenderNode(eERType_PrismObject);

    pSlot->pChildRenderNode = pVolObj;

    //  pVolObj->LoadVolumeData(sFilename);
    if (m_pMaterial)
    {
        pVolObj->SetMaterial(m_pMaterial);
    }

    // Update slot position
    pSlot->OnXForm(m_pEntity);

    m_nFlags |= FLAG_HAS_CHILDRENDERNODES;

    //pSlot->flags |= ENTITY_SLOT_RENDER;
    pSlot->bUpdate = false;
    InvalidateBounds(true, true);

    return nSlot;
}
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

//////////////////////////////////////////////////////////////////////////
ICharacterInstance* CComponentRender::GetCharacter(int nSlot)
{
    nSlot = nSlot & (~ENTITY_SLOT_ACTUAL);
    if (!IsSlotValid(nSlot))
    {
        return NULL;
    }
    return Slot(nSlot)->pCharacter;
}

//////////////////////////////////////////////////////////////////////////
IStatObj* CComponentRender::GetStatObj(int nSlot)
{
    if ((nSlot & ~ENTITY_SLOT_ACTUAL) < PARTID_CGA)
    {
        IStatObj* pCompObj;
        if (!(nSlot & ENTITY_SLOT_ACTUAL) && (pCompObj = GetCompoundObj()))
        {
            return (unsigned int)pCompObj->GetSubObjectCount() > (unsigned int)nSlot ?
                   pCompObj->GetSubObject(nSlot)->pStatObj : 0;
        }
        nSlot &= ~ENTITY_SLOT_ACTUAL;
        if (!IsSlotValid(nSlot))
        {
            return NULL;
        }
        return Slot(nSlot)->pStatObj;
    }
    else
    {
        int nSubSlot = nSlot % PARTID_CGA;
        nSlot = nSlot / PARTID_CGA - 1;
        if (!IsSlotValid(nSlot) || !Slot(nSlot)->pCharacter)
        {
            return NULL;
        }
        return Slot(nSlot)->pCharacter->GetISkeletonPose()->GetStatObjOnJoint(nSubSlot);
    }
}

//////////////////////////////////////////////////////////////////////////
IParticleEmitter* CComponentRender::GetParticleEmitter(int nSlot)
{
    if (!IsSlotValid(nSlot))
    {
        return NULL;
    }

    return Slot(nSlot)->GetParticleEmitter();
}

#if defined(USE_GEOM_CACHES)
//////////////////////////////////////////////////////////////////////////
IGeomCacheRenderNode* CComponentRender::GetGeomCacheRenderNode(int nSlot)
{
    if (!IsSlotValid(nSlot))
    {
        return NULL;
    }

    return Slot(nSlot)->GetGeomCacheRenderNode();
}
#endif

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CComponentRender::GetRenderMaterial(int nSlot)
{
    CEntityObject* pSlot = NULL;
    if (nSlot >= PARTID_CGA)
    {
        nSlot = nSlot / PARTID_CGA - 1;
    }
    if (nSlot >= 0)
    {
        pSlot = GetSlot(nSlot);
    }
    else
    {
        for (uint32 i = 0; i < m_slots.size(); i++)
        {
            if (m_slots[i] && (m_slots[i]->flags & ENTITY_SLOT_RENDER))
            {
                pSlot = m_slots[i];
                break;
            }
        }
    }

    if (!pSlot)
    {
        return NULL;
    }

    if (pSlot->pMaterial)
    {
        return pSlot->pMaterial;
    }

    if (m_pMaterial)
    {
        return m_pMaterial;
    }

    if (pSlot->pStatObj)
    {
        return pSlot->pStatObj->GetMaterial();
    }

    if (pSlot->pCharacter)
    {
        return pSlot->pCharacter->GetIMaterial();
    }

    if (pSlot->pChildRenderNode)
    {
        return pSlot->pChildRenderNode->GetMaterial(NULL);
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SerializeXML(XmlNodeRef& entityNode, bool bLoading)
{
    if (bLoading)
    {
        int nLodRatio;
        float fViewDistanceMultiplier;
        if (entityNode->getAttr("LodRatio", nLodRatio))
        {
            // Change LOD ratio.
            SetLodRatio(nLodRatio);
        }
        if (entityNode->getAttr("ViewDistanceMultiplier", fViewDistanceMultiplier))
        {
            // Change ViewDistRatio ratio.
            SetViewDistanceMultiplier(fViewDistanceMultiplier);
        }
        uint32 nMaterialLayers = m_nMaterialLayers;
        if (entityNode->getAttr("MatLayersMask", nMaterialLayers))
        {
            m_nMaterialLayers = nMaterialLayers;
        }
    }
}

#define DEBUG_PARTICLES 0

//////////////////////////////////////////////////////////////////////////
bool CComponentRender::NeedSerialize()
{
    return (m_nFlags & FLAG_HAS_PARTICLES) != 0 && GetMaterialLayers() != 0;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentRender::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentRender");
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::Serialize(TSerialize ser)
{
    // Particles.
    if (ser.GetSerializationTarget() != eST_Network)
    {
        if (ser.BeginOptionalGroup("ComponentRender", true))
        {
            if (ser.IsWriting())
            {
                //////////////////////////////////////////////////////////////////////////
                // Saving.
                //////////////////////////////////////////////////////////////////////////
                {
                    int nLodRatio = GetLodRatio();
                    float fViewDistanceMultiplier = GetViewDistanceMultiplier();
                    uint32 nMaterialLayers = GetMaterialLayers();
                    //ser.Value("LodR",nLodRatio);
                    //ser.Value("ViewDistanceMultiplier",fViewDistanceMultiplier);
                    ser.Value("MatLayers", nMaterialLayers);
                }

                if (ser.BeginOptionalGroup("Particles", (m_nFlags & FLAG_HAS_PARTICLES) != 0))
                {
                    // Count serializable
                    int nSerSlots = 0;
                    for (uint32 i = 0; i < m_slots.size(); ++i)
                    {
                        IParticleEmitter* pEmitter = GetParticleEmitter(i);
                        if (pEmitter && pEmitter->IsAlive())
                        {
                            if (m_slots[i]->bSerialize)
                            {
                                ser.BeginGroup("Slot");
                                ++nSerSlots;
                                ser.Value("slot", i);
                                gEnv->pParticleManager->SerializeEmitter(ser, pEmitter);
                                ser.EndGroup();
                            }
                        }
                    }
                    ser.Value("count", nSerSlots);
                    ser.EndGroup(); //Particles
                }
            }
            else
            {
                //////////////////////////////////////////////////////////////////////////
                // Loading.
                //////////////////////////////////////////////////////////////////////////
                {
                    //                  int nLodRatio=0;
                    //                  float fViewDistanceMultiplier=0.0f;
                    uint32 nMaterialLayers = 0;
                    //ser.Value("LodR",nLodRatio);
                    //ser.Value("ViewDistanceMultiplier",fViewDistanceMultiplier);
                    ser.Value("MatLayers", nMaterialLayers);

                    // Change lod ratio.
                    //SetLodRatio(nLodRatio);
                    //SetViewDistanceMultiplier(fViewDistanceMultiplier);
                    SetMaterialLayers(nMaterialLayers);
                }

                if (ser.BeginOptionalGroup("Particles", true))
                {
                    int nSerSlots = 0;
                    ser.Value("count", nSerSlots);
                    //                  int nCurSlot = -1;
                    for (int i = 0; i < nSerSlots; ++i)
                    {
                        ser.BeginGroup("Slot");
                        int nSlot = -1;
                        ser.Value("slot", nSlot);
                        if (nSlot >= 0)
                        {
                            if (IParticleEmitter* pSerEmitter = gEnv->pParticleManager->SerializeEmitter(ser))
                            {
                                CEntityObject* pSlot = GetOrMakeSlot(nSlot);
                                pSerEmitter->SetMatrix(pSlot->GetWorldTM(m_pEntity));
                                pSerEmitter->Activate(1);
                                SetParticleEmitter(nSlot, pSerEmitter, true);
                            }
                        }
                        ser.EndGroup();
                    }
                    ser.EndGroup();//Particles
                }
            }

            ser.EndGroup(); // ComponentRender
        }
    }

#if DEBUG_PARTICLES
    for (int i = 0; i < m_slots.size(); ++i)
    {
        IParticleEmitter* pEmitter = GetParticleEmitter(i);
        if (pEmitter && pEmitter->IsAlive())
        {
            CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_COMMENT,
                "Entity Particle: %s, slot %d, serialize %d: %s",
                m_pEntity->GetName(), i, m_slots[i]->bSerialize, pEmitter->GetDebugString('s').c_str());
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////////

IShaderPublicParams* CComponentRender::GetShaderPublicParams(bool bCreate)
{
    if (m_pShaderPublicParams == NULL && bCreate)
    {
        m_pShaderPublicParams = gEnv->pRenderer->CreateShaderPublicParams();
    }

    return m_pShaderPublicParams;
}

void CComponentRender::AddShaderParamCallback(IShaderParamCallbackPtr pCallback)
{
    TCallbackVector::iterator itEnd = m_Callbacks.end();
    for (TCallbackVector::iterator it = m_Callbacks.begin(); it != itEnd; ++it)
    {
        if (*it == pCallback)
        {
            return;
        }
    }
    m_Callbacks.push_back(pCallback);
}

bool CComponentRender::RemoveShaderParamCallback(IShaderParamCallbackPtr pCallback)
{
    TCallbackVector::iterator itEnd = m_Callbacks.end();
    for (TCallbackVector::iterator it = m_Callbacks.begin(); it != itEnd; ++it)
    {
        if (*it == pCallback)
        {
            m_Callbacks.erase(it);
            return true;
        }
    }

    return false;
}

void CComponentRender::SetMaterialLayersMask(uint8 nMtlLayers)
{
    uint8 old = GetMaterialLayersMask();
    SetMaterialLayers(nMtlLayers);

    if (nMtlLayers != old)
    {
        SEntityEvent event(ENTITY_EVENT_MATERIAL_LAYER);
        event.nParam[0] = nMtlLayers;
        event.nParam[1] = old;
        m_pEntity->SendEvent(event);
    }
}

void CComponentRender::ClearShaderParamCallbacks()
{
    IShaderPublicParams* pParams = GetShaderPublicParams(false);
    if (pParams != NULL)
    {
        TCallbackVector::iterator itEnd = m_Callbacks.end();
        for (TCallbackVector::iterator it = m_Callbacks.begin(); it != itEnd; ++it)
        {
            (*it)->Disable(pParams);
        }
    }

    m_Callbacks.clear();
}

void CComponentRender::CheckShaderParamCallbacks()
{
    if (IScriptTable* pScriptTable = m_pEntity->GetScriptTable())
    {
        SmartScriptTable    properties;

        if (pScriptTable->GetValue("Properties", properties))
        {
            SmartScriptTable renderingProperties;

            if (properties->GetValue("Rendering", renderingProperties))
            {
                ICharacterInstance* pCharacter = GetCharacter(0);

                IScriptTable::Iterator iter = renderingProperties->BeginIteration();
                while (renderingProperties->MoveNext(iter))
                {
                    bool bValue = false;
                    if ((strlen(iter.sKey) > 1) && renderingProperties->GetValue(iter.sKey, bValue) && bValue)
                    {
                        IShaderParamCallbackPtr spShaderParamCallback;
                        if (CryCreateClassInstance(iter.sKey, spShaderParamCallback))
                        {
                            // We should probably initialize spShaderParamCallback with "this" instead of "pCharacter" as we don't know what kind of entity it works on
                            // As we only use CWrinkleMapShaderParamCallback at the moment, this is not a problem.
                            spShaderParamCallback->SetCharacterInstance(pCharacter);
                            if (spShaderParamCallback->Init())
                            {
                                AddShaderParamCallback(spShaderParamCallback);
                            }
                        }
                    }
                }
                renderingProperties->EndIteration(iter);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetMaterial(_smart_ptr<IMaterial> pMat)
{
    m_pEntity->SetMaterial(pMat);
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CComponentRender::GetMaterial(Vec3* pHitPos)
{
    return m_pMaterial;
};

//////////////////////////////////////////////////////////////////////////
int  CComponentRender::GetSlotCount() const
{
    return m_slots.size();
}

//////////////////////////////////////////////////////////////////////////
Vec3 CComponentRender::GetPos(bool bWorldOnly) const
{
    return bWorldOnly ? m_pEntity->GetWorldPos() : m_pEntity->GetPos();
}

//////////////////////////////////////////////////////////////////////////
const char* CComponentRender::GetEntityClassName() const { return m_pEntity->GetClass()->GetName(); };
const Ang3& CComponentRender::GetAngles(int realA) const { static Ang3 angles(m_pEntity->GetWorldAngles()); return angles; };
float CComponentRender::GetScale() const { return m_pEntity->GetScale().x; };
const char* CComponentRender::GetName() const { return m_pEntity->GetName(); };
void CComponentRender::GetRenderBBox(Vec3& mins, Vec3& maxs) { mins = m_WSBBox.min; maxs = m_WSBBox.max; }
void CComponentRender::GetBBox(Vec3& mins, Vec3& maxs) { mins = m_WSBBox.min; maxs = m_WSBBox.max; }

float CComponentRender::GetMaxViewDist()
{
    if (!(m_nEntityFlags & ENTITY_FLAG_CUSTOM_VIEWDIST_RATIO))
    {
        uint32 nMinSpec = (m_dwRndFlags & ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT;
        return max(s_fViewDistMin, m_WSBBox.GetRadius() * s_fViewDistRatio * GetViewDistanceMultiplier());
    }
    else
    {
        float s = max(max((m_WSBBox.max.x - m_WSBBox.min.x), (m_WSBBox.max.y - m_WSBBox.min.y)), (m_WSBBox.max.z - m_WSBBox.min.z));
        return max(s_fViewDistMin, s * s_fViewDistRatioCustom * GetViewDistanceMultiplier());
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetCustomPostEffect(const char* pPostEffectName)
{
    int32 postEffectID = gEnv->p3DEngine->GetPostEffectID(pPostEffectName);
    if (postEffectID >= 0 && postEffectID <= 255)
    {
        AddFlags(FLAG_CUSTOM_POST_EFFECT);
        m_nCustomData = postEffectID;
    }
    else
    {
        ClearFlags(FLAG_CUSTOM_POST_EFFECT);
    }
}

//////////////////////////////////////////////////////////////////////////

void CComponentRender::SetViewDistanceMultiplier(float fViewDistanceMultiplier)
{
    IRenderNode::SetViewDistanceMultiplier(fViewDistanceMultiplier);

    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        if (m_slots[i] && m_slots[i]->pChildRenderNode)
        {
            m_slots[i]->pChildRenderNode->SetViewDistanceMultiplier(fViewDistanceMultiplier);
        }

        if (m_slots[i] && m_slots[i]->pLight)
        {
            m_slots[i]->pLight->SetViewDistanceMultiplier(fViewDistanceMultiplier);
            m_slots[i]->pLight->m_fWSMaxViewDist = m_slots[i]->pLight->GetMaxViewDist();
        }
    }
}

void CComponentRender::SetMinSpec(int nMinSpec)
{
    IRenderNode::SetMinSpec(nMinSpec);

    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        if (m_slots[i] && m_slots[i]->pChildRenderNode)
        {
            m_slots[i]->pChildRenderNode->SetMinSpec(nMinSpec);
        }

        if (m_slots[i] && m_slots[i]->pLight)
        {
            m_slots[i]->pLight->SetMinSpec(nMinSpec);
        }
    }
}

void CComponentRender::SetLodRatio(int nLodRatio)
{
    IRenderNode::SetLodRatio(nLodRatio);

    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        if (m_slots[i] && m_slots[i]->pChildRenderNode)
        {
            m_slots[i]->pChildRenderNode->SetLodRatio(nLodRatio);
        }
        if (m_slots[i] && m_slots[i]->pLight)
        {
            m_slots[i]->pLight->SetLodRatio(nLodRatio);
        }
    }
}

bool CComponentRender::GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const
{
    const float fLodRatio = GetLodRatioNormalized();
    if (fLodRatio > 0.0f)
    {
        // fallback to old system in case the geometric mean information is not available
        if (m_fLodDistance <= 0.0f)
        {
            return false;
        }

        const float fDistMultiplier = 1.0f / (fLodRatio * frameLodInfo.fTargetSize);

        for (uint i = 0; i < SMeshLodInfo::s_nMaxLodCount; ++i)
        {
            distances[i] = m_fLodDistance * (i + 1) * fDistMultiplier;
        }
    }
    else
    {
        for (uint i = 0; i < SMeshLodInfo::s_nMaxLodCount; ++i)
        {
            distances[i] = FLT_MAX;
        }
    }

    return true;
}

void CComponentRender::UpdateLodDistance(const SFrameLodInfo& frameLodInfo)
{
    SMeshLodInfo lodInfo;

    for (Slots::iterator it = m_slots.begin(), endit = m_slots.end(); it != endit; ++it)
    {
        SMeshLodInfo subLodInfo;

        CEntityObject* pSlot = *it;
        if (pSlot && (pSlot->flags & ENTITY_SLOT_RENDER))
        {
            if (pSlot->pStatObj)
            {
                pSlot->pStatObj->ComputeGeometricMean(subLodInfo);
            }
            else if (pSlot->pCharacter)
            {
                pSlot->pCharacter->ComputeGeometricMean(subLodInfo);
            }
        }

        lodInfo.Merge(subLodInfo);
    }

    m_fLodDistance = sqrt(lodInfo.fGeometricMean);
}

//////////////////////////////////////////////////////////////////////////
bool CComponentRender::IsRenderComponentVisAreaVisible() const
{
    // test last render frame id
    if (GetEntityVisArea())
    {
        if (abs(GetEntityVisArea()->GetVisFrameId() - gEnv->pRenderer->GetFrameID()) <= MAX_FRAME_ID_STEP_PER_FRAME)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::GetMemoryUsage(ICrySizer* pSizer) const
{
    {
        SIZER_COMPONENT_NAME(pSizer, "ComponentRender Allocator");
        pSizer->AddObject(g_Alloc_ComponentRender);
    }

    pSizer->AddContainer(m_slots);
}

bool CComponentRender::IsMovableByGame() const
{
    if (IPhysicalEntity* pPhysEnt = m_pEntity->GetPhysics())
    {
        if (pe_type eType = pPhysEnt->GetType())
        {
            return !(eType == PE_NONE || eType == PE_STATIC || eType == PE_AREA);
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::DebugDraw(const SGeometryDebugDrawInfo& info)
{
    uint32 nIndex = 0;
    IStatObj* pCompObj = nullptr;
    for (uint32 i = 0; i < m_slots.size(); i++)
    {
        nIndex = i;
        if (m_slots[i])
        {
            if (m_slots[i]->pStatObj)
            {
                //Can't just get the slots world transform because if there are subobjects we will double
                //multiply the transform matrices on the subobjects when we draw.
                SGeometryDebugDrawInfo subInfo = info;
                pCompObj = GetCompoundObj();
                if (!(nIndex & ENTITY_SLOT_ACTUAL) && (pCompObj))
                {
                    subInfo.tm = Slot(0)->GetWorldTM(m_pEntity);
                }
                else
                {
                    nIndex &= ~ENTITY_SLOT_ACTUAL;
                    if (IsSlotValid(nIndex))
                    {
                        subInfo.tm = Slot(nIndex)->GetWorldTM(m_pEntity);
                    }
                }
                m_slots[i]->pStatObj->DebugDraw(subInfo);
            }
#if defined(USE_GEOM_CACHES)
            else if (IGeomCacheRenderNode* pGeomCacheRenderNode = GetGeomCacheRenderNode(i))
            {
                SGeometryDebugDrawInfo subInfo = info;
                subInfo.tm = GetSlotWorldTM(i);
                pGeomCacheRenderNode->DebugDraw(info);
            }
#endif
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentRender::PhysicalizeFoliage(bool bPhysicalize, int iSource, int nSlot)
{
    CEntityObject* pSlot = GetSlot(0);
    IStatObj::SSubObject* pSubObj;
    static ICVar* g_pLifetime = gEnv->pConsole->GetCVar("e_FoliageBranchesTimeout");
    IPhysicalEntity* pPhysEnt = m_pEntity->GetPhysics();
    float lifetime = g_pLifetime && pPhysEnt && pPhysEnt->GetType() == PE_STATIC ? g_pLifetime->GetFVal() : 0.0f;

    if (pSlot && pSlot->pStatObj && (pSubObj = pSlot->pStatObj->GetSubObject(nSlot)) && pSubObj->pStatObj)
    {
        if (bPhysicalize)
        {
            pSubObj->pStatObj->PhysicalizeFoliage(pPhysEnt, GetSlotWorldTM(ENTITY_SLOT_ACTUAL) * pSubObj->tm, pSubObj->pFoliage, lifetime);
            return pSubObj->pFoliage != 0;
        }
        else if (!pSubObj->pFoliage)
        {
            return false;
        }
        pSubObj->pFoliage->Release();
        return true;
    }
    else if ((pSlot = GetSlot(nSlot)) && pSlot->pStatObj)
    {
        if (bPhysicalize)
        {
            pSlot->pStatObj->PhysicalizeFoliage(pPhysEnt, GetSlotWorldTM(ENTITY_SLOT_ACTUAL | nSlot), pSlot->pFoliage, lifetime);
            return pSlot->pFoliage != 0;
        }
        else if (pSlot->pFoliage)
        {
            pSlot->pFoliage->Release();
            return true;
        }
    }

    return false;
}

IFoliage* CComponentRender::GetFoliage(int nSlot)
{
    CEntityObject* pSlot = GetSlot(0);
    IStatObj::SSubObject* pSubObj;

    if (pSlot && pSlot->pStatObj && (pSubObj = pSlot->pStatObj->GetSubObject(nSlot)) && pSubObj->pStatObj)
    {
        return pSubObj->pFoliage;
    }
    else if (pSlot = GetSlot(nSlot))
    {
        return pSlot->pFoliage;
    }

    return 0;
}

IPhysicalEntity* CComponentRender::GetPhysics() const
{
    return m_pEntity->GetPhysics();
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::SetSubObjHideMask(int nSlot, uint64 nSubObjHideMask)
{
    if (IsSlotValid(nSlot))
    {
        CEntityObject* pSlot = Slot(nSlot);
        pSlot->nSubObjHideMask = nSubObjHideMask;
    }
}

//////////////////////////////////////////////////////////////////////////
uint64 CComponentRender::GetSubObjHideMask(int nSlot) const
{
    if (IsSlotValid(nSlot))
    {
        CEntityObject* pSlot = Slot(nSlot);
        return pSlot->nSubObjHideMask;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::FillBBox(AABB& aabb)
{
    aabb = CComponentRender::GetBBox();
}

//////////////////////////////////////////////////////////////////////////
EERType CComponentRender::GetRenderNodeType()
{
    return eERType_RenderComponent;
}


//////////////////////////////////////////////////////////////////////////
bool CComponentRender::CanExecuteRenderAsJob()
{
#if defined(SUPPORT_COMPONENTRENDER_RENDER_JOB)
    if (CheckFlags(FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG))
    {
        AddFlags(FLAG_EXECUTE_AS_JOB_FLAG);
        int nSlotCount = GetSlotCount();
        // check for configurations not supported by job execution
        for (int i = 0; i < nSlotCount; ++i)
        {
            if (GetCharacter(i))
            {
                ClearFlags(FLAG_EXECUTE_AS_JOB_FLAG);
                break;
            }
            if (GetParticleEmitter(i))
            {
                ClearFlags(FLAG_EXECUTE_AS_JOB_FLAG);
                break;
            }
        }
        ClearFlags(FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG);

        // check some addional states not supported for jobs as not threadsafe
        if (m_nEntityFlags & ENTITY_FLAG_SEND_RENDER_EVENT)
        {
            ClearFlags(FLAG_EXECUTE_AS_JOB_FLAG);
        }
        if (m_Callbacks.size() > 0)
        {
            ClearFlags(FLAG_EXECUTE_AS_JOB_FLAG);
        }
    }

    return CheckFlags(FLAG_EXECUTE_AS_JOB_FLAG);
#else
    return false;
#endif // SUPPORT_COMPONENTRENDER_RENDER_JOB
}

//////////////////////////////////////////////////////////////////////////
void CComponentRender::QueueSlotGeometryChange(int nSlot, IStatObj* pStatObj)
{
    m_queuedGeometryChanges.push_back(std::make_pair(nSlot, pStatObj));
    RegisterEvent(ENTITY_EVENT_PREPHYSICSUPDATE);
    m_pEntity->PrePhysicsActivate(true);
}
