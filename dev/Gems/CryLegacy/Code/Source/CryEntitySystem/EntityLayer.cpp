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
#include "EntityLayer.h"
#include "Entity.h"
#include "EntitySystem.h"
#include "ISerialize.h"


CEntityLayer::CEntityLayer(const char* name, uint16 id, bool havePhysics, int specs, bool defaultLoaded, TGarbageHeaps& garbageHeaps)
    : m_name(name)
    , m_id(id)
    , m_isEnabled(true)
    , m_isEnabledBrush(false)
    , m_isSerialized(true)
    , m_wasReEnabled(false)
    , m_havePhysics(havePhysics)
    , m_specs(specs)
    , m_defaultLoaded(defaultLoaded)
    , m_pGarbageHeaps(&garbageHeaps)
    , m_pHeap(NULL)
{
}


CEntityLayer::~CEntityLayer()
{
    for (TEntityProps::iterator it = m_entities.begin(); it != m_entities.end(); ++it)
    {
        if (IEntity* pEntity = g_pIEntitySystem->GetEntity(it->first))
        {
            if (it->second.m_bIsNoAwake)
            {
                if (IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>())
                {
                    if (IPhysicalEntity* pPhysicalEntity = pPhysicsComponent->GetPhysicalEntity())
                    {
                        pe_action_awake aa;
                        aa.bAwake = false;
                        pPhysicalEntity->Action(&aa);
                    }
                }
            }
            it->second.m_bIsNoAwake = false;
        }
    }

    if (m_pHeap)
    {
        m_pGarbageHeaps->push_back(SEntityLayerGarbage(m_pHeap, m_name));
    }
}

void CEntityLayer::AddObject(EntityId id)
{
    IEntity* pEntity = g_pIEntitySystem->GetEntity(id);
    if (pEntity)
    {
        m_entities[id] = EntityProp(id, false, pEntity->IsHidden(), pEntity->IsActive());
    }
    m_wasReEnabled = false;
}


void CEntityLayer::RemoveObject(EntityId id)
{
    TEntityProps::iterator it = m_entities.find(id);
    if (it != m_entities.end())
    {
        m_entities.erase(it);
    }
}

bool CEntityLayer::IsSkippedBySpec() const
{
    if (m_specs == eSpecType_All)
    {
        return false;
    }

    switch (gEnv->pRenderer->GetRenderType())
    {
    case eRT_PS4: // ACCEPTED_USE
        if (m_specs & eSpecType_PS4) // ACCEPTED_USE
        {
            return false;
        }
        break;

    case eRT_XboxOne: // ACCEPTED_USE
        if (m_specs & eSpecType_XBoxOne) // ACCEPTED_USE
        {
            return false;
        }
        break;

    case eRT_OpenGL:
    case eRT_DX11:
    default:
        if (m_specs & eSpecType_PC)
        {
            return false;
        }
        break;
    }

    return true;
}

void CEntityLayer::Enable(bool isEnable, bool isSerialized /*=true*/, bool bAllowRecursive /*=true*/)
{
    // Wait for the physics thread to avoid massive amounts of ChangeRequest queuing
    gEnv->pSystem->SetThreadState(ESubsys_Physics, false);

#ifdef ENABLE_PROFILING_CODE
    bool bChanged = (m_isEnabledBrush != isEnable) || (m_isEnabled != isEnable);
    float fStartTime = gEnv->pTimer->GetAsyncCurTime();
#endif //ENABLE_PROFILING_CODE

    if (isEnable && IsSkippedBySpec())
    {
        return;
    }

    MEMSTAT_LABEL_FMT("Layer '%s' %s", m_name.c_str(), (isEnable ? "Activating" : "Deactivating"));
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Entity, 0, "Layer '%s' %s", m_name.c_str(), (isEnable ? "Activating" : "Deactivating"));

    if (isEnable)
    {
        if (m_pHeap)
        {
            m_pGarbageHeaps->push_back(SEntityLayerGarbage(m_pHeap, m_name));
        }
    }

    if (!isEnable) // make sure that children are hidden before parents and unhidden after them
    {
        if (bAllowRecursive && !gEnv->pSystem->IsSerializingFile()) // this check should not be needed now, because Enable() is not used in serialization anymore, but im keeping it for extra sanity check
        {
            for (std::vector<CEntityLayer*>::iterator it = m_childs.begin(); it != m_childs.end(); ++it)
            {
                (*it)->Enable(isEnable);
            }
        }
    }

    EnableBrushes(isEnable);
    EnableEntities(isEnable);

    if (isEnable) // make sure that children are hidden before parents and unhidden after them
    {
        if (bAllowRecursive && !gEnv->pSystem->IsSerializingFile()) // this check should not be needed now, because Enable() is not used in serialization anymore, but im keeping it for extra sanity check
        {
            for (std::vector<CEntityLayer*>::iterator it = m_childs.begin(); it != m_childs.end(); ++it)
            {
                (*it)->Enable(isEnable);
            }
        }
    }

    m_isSerialized = isSerialized;

#ifdef ENABLE_PROFILING_CODE
    if (CVar::es_LayerDebugInfo == 5 && bChanged)
    {
        float fTimeMS = (gEnv->pTimer->GetAsyncCurTime() - fStartTime) * 1000.0f;
        CEntitySystem::SLayerProfile layerProfile;
        layerProfile.pLayer = this;
        layerProfile.fTimeOn = gEnv->pTimer->GetCurrTime();
        layerProfile.isEnable = isEnable;
        layerProfile.fTimeMS = fTimeMS;
        g_pIEntitySystem->m_layerProfiles.insert(g_pIEntitySystem->m_layerProfiles.begin(), layerProfile);
    }
#endif //ENABLE_PROFILING_CODE

    if (m_pHeap && m_pHeap->Cleanup())
    {
        m_pHeap->Release();
        m_pHeap = NULL;
    }

    MEMSTAT_LABEL_FMT("Layer '%s' %s", m_name.c_str(), (isEnable ? "Activated" : "Deactivated"));
}


void CEntityLayer::EnableBrushes(bool isEnable)
{
    if (m_isEnabledBrush != isEnable)
    {
        if (m_id)
        {
            // activate brushes but not static lights
            gEnv->p3DEngine->ActivateObjectsLayer(m_id, isEnable, m_havePhysics, true, false, m_name.c_str(), m_pHeap);
        }
        m_isEnabledBrush = isEnable;
    }

    ReEvalNeedForHeap();
}


void CEntityLayer::EnableEntities(bool isEnable)
{
    if (m_isEnabled != isEnable)
    {
        m_isEnabled = isEnable;

        // activate static lights but not brushes
        gEnv->p3DEngine->ActivateObjectsLayer(m_id, isEnable, m_havePhysics, false, true, m_name.c_str(), m_pHeap);

        pe_action_awake noAwake;
        noAwake.bAwake = false;

        for (TEntityProps::iterator it = m_entities.begin(); it != m_entities.end(); ++it)
        {
            EntityProp& prop = it->second;

            IEntity* pEntity = g_pIEntitySystem->GetEntity(prop.m_id);

            if (!pEntity)
            {
                continue;
            }

            // when is serializing (reading, as we never call this on writing), we dont want to change those values. we just use the values that come directly from serialization.
            if (!isEnable && !gEnv->pSystem->IsSerializingFile())
            {
                prop.m_bIsHidden = pEntity->IsHidden();
                prop.m_bIsActive = pEntity->IsActive();
            }

            if (prop.m_bIsHidden)
            {
                continue;
            }

            if (isEnable)
            {
                pEntity->Hide(!isEnable);
                pEntity->Activate(prop.m_bIsActive);

                if (prop.m_bIsNoAwake)
                {
                    if (IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>())
                    {
                        if (IPhysicalEntity* pPhysicalEntity = pPhysicsComponent->GetPhysicalEntity())
                        {
                            pPhysicalEntity->Action(&noAwake);
                        }
                    }
                }
                prop.m_bIsNoAwake = false;

                SEntityEvent event;
                event.nParam[0] = 1;
                static IEntityClass* pConstraintClass = g_pIEntitySystem->GetClassRegistry()->FindClass("Constraint");
                if (pConstraintClass && pEntity->GetClass() == pConstraintClass)
                {
                    event.event = ENTITY_EVENT_RESET;
                    pEntity->SendEvent(event);
                    event.event = ENTITY_EVENT_LEVEL_LOADED;
                    pEntity->SendEvent(event);
                }
                if (!m_wasReEnabled && pEntity->GetPhysics() && pEntity->GetPhysics()->GetType() == PE_ROPE)
                {
                    event.event = ENTITY_EVENT_LEVEL_LOADED;
                    pEntity->SendEvent(event);
                }
            }
            else
            {
                prop.m_bIsNoAwake = false;
                if (IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>())
                {
                    pe_status_awake isawake;
                    if (IPhysicalEntity* pPhysicalEntity = pPhysicsComponent->GetPhysicalEntity())
                    {
                        if (pPhysicalEntity->GetStatus(&isawake) == 0)
                        {
                            prop.m_bIsNoAwake = true;
                        }
                    }
                }
                pEntity->Hide(!isEnable);
                pEntity->Activate(isEnable);
                if (prop.m_bIsNoAwake)
                {
                    if (IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>())
                    {
                        if (IPhysicalEntity* pPhysicalEntity = pPhysicsComponent->GetPhysicalEntity())
                        {
                            pPhysicalEntity->Action(&noAwake);
                        }
                    }
                }
            }
        }

        if (isEnable)
        {
            m_wasReEnabled = true;
        }
    }

    ReEvalNeedForHeap();
}

void CEntityLayer::ReEvalNeedForHeap()
{
    if (!IsEnabled() && m_pHeap)
    {
        m_pGarbageHeaps->push_back(SEntityLayerGarbage(m_pHeap, m_name));
        m_pHeap = NULL;
    }
}


void CEntityLayer::GetMemoryUsage(ICrySizer* pSizer, int* pOutNumEntities)
{
    if (pOutNumEntities)
    {
        *pOutNumEntities = 0;
    }
    for (TEntityProps::iterator it = m_entities.begin(); it != m_entities.end(); ++it)
    {
        EntityProp& prop = it->second;
        IEntity* pEntity = g_pIEntitySystem->GetEntity(prop.m_id);
        if (pEntity)
        {
            pEntity->GetMemoryUsage(pSizer);
            if (pOutNumEntities)
            {
                (*pOutNumEntities)++;
            }
        }
    }
}


void CEntityLayer::Serialize(TSerialize ser, TLayerActivationOpVec& deferredOps)
{
    bool temp_isEnabledBrush = m_isEnabledBrush;
    bool temp_isEnabled = m_isEnabled;
    ser.BeginGroup("Layer");
    ser.Value("name", m_name);
    ser.Value("enabled", temp_isEnabled);
    ser.Value("enabledBrush", temp_isEnabledBrush);
    ser.Value("m_isSerialized", m_isSerialized);

    if (ser.IsReading())
    {
        int count = 0;
        ser.Value("count", count);

        m_entities.clear();

        for (int i = 0; i < count; ++i)
        {
            ser.BeginGroup("LayerEntities");

            EntityId id = 0;
            bool hidden = false;
            bool noAwake = false;
            bool active = false;
            ser.Value("entityId", id);
            ser.Value("hidden", hidden);
            ser.Value("noAwake", noAwake);
            ser.Value("active", active);

            EntityProp& prop = m_entities[i];
            prop.m_id = id;
            prop.m_bIsHidden = hidden;
            prop.m_bIsNoAwake = noAwake;
            prop.m_bIsActive = active;

            ser.EndGroup();
        }

        SPostSerializeLayerActivation brushActivation;
        brushActivation.m_layer = this;
        brushActivation.m_func = &CEntityLayer::EnableBrushes;
        brushActivation.enable = temp_isEnabledBrush;
        deferredOps.push_back(brushActivation);

        SPostSerializeLayerActivation entityActivation;
        entityActivation.m_layer = this;
        entityActivation.m_func = &CEntityLayer::EnableEntities;
        entityActivation.enable = temp_isEnabled;
        deferredOps.push_back(entityActivation);
    }
    else
    {
        int count = (int)m_entities.size();
        ser.Value("count", count);
        for (TEntityProps::const_iterator it = m_entities.begin(); it != m_entities.end(); ++it)
        {
            ser.BeginGroup("LayerEntities");
            const EntityProp& prop = it->second;
            EntityId id = prop.m_id;
            bool hidden = prop.m_bIsHidden;
            bool noAwake = prop.m_bIsNoAwake;
            bool active = prop.m_bIsActive;
            ser.Value("entityId", id);
            ser.Value("hidden", hidden);
            ser.Value("noAwake", noAwake);
            ser.Value("active", active);
            ser.EndGroup();
        }
    }

    ser.EndGroup();
}
