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
#include "StdAfx.h"
#include <IEntitySystem.h>
#include <IEntityHelper.h>
#include "ComponentBoids.h"
#include "Flock.h"
#include "ISerialize.h"
#include "Components/IComponentRender.h"
#include "Components/IComponentArea.h"
#include "Components/IComponentSerialization.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentBoids, CComponentBoids)
DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentBoidObject, CComponentBoidObject)

//////////////////////////////////////////////////////////////////////////
CComponentBoids::CComponentBoids()
    : m_pEntity(NULL)
    , m_pFlock(NULL)
    , m_playersInCount(0)
{
}

//////////////////////////////////////////////////////////////////////////
CComponentBoids::~CComponentBoids()
{
    if (m_pFlock)
    {
        delete m_pFlock;
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoids::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;

    m_pEntity->GetOrCreateComponent<IComponentSerialization>()
        ->Register<CComponentBoids>(SerializationOrder::Boids, *this, &CComponentBoids::Serialize,
        &CComponentBoids::SerializeXML, &CComponentBoids::NeedSerialize,
        &CComponentBoids::GetSignature);

    // Make sure render and trigger component also exist.
    m_pEntity->GetOrCreateComponent<IComponentRender>();
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoids::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    m_pEntity = pEntity;

    if (m_pFlock)
    {
        m_pFlock->SetPos(pEntity->GetWorldPos());
    }

    // Make sure render and trigger component also exist.
    pEntity->GetOrCreateComponent<IComponentRender>();
    m_playersInCount = 0;
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoids::UpdateComponent(SEntityUpdateContext& ctx)
{
    if (m_pFlock)
    {
        m_pFlock->Update(ctx.pCamera);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoids::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_XFORM:
        if (m_pFlock)
        {
            m_pFlock->SetPos(m_pEntity->GetWorldPos());
        }
        break;
    case ENTITY_EVENT_PRE_SERIALIZE:
        if (m_pFlock)
        {
            m_pFlock->DeleteEntities(true);
        }
        break;
    case ENTITY_EVENT_ENTERAREA:
        OnTrigger(true, event);
        break;
    case ENTITY_EVENT_LEAVEAREA:
        OnTrigger(false, event);
        break;
    case ENTITY_EVENT_RESET:
        if (m_pFlock)
        {
            m_pFlock->Reset();
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentBoids::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentBoids");
    if (m_pFlock)
    {
        uint32 type = (uint32)m_pFlock->GetType();
        signature.Value("type", type);
    }
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoids::Serialize(TSerialize ser)
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoids::SetFlock(CFlock* pFlock)
{
    m_pFlock = pFlock;

    if (!pFlock)
    {
        return;
    }

    // Update trigger based on new visibility distance settings.
    float fMaxDist = pFlock->GetMaxVisibilityDistance();

    /*
    CComponentTriggerPtr pTrigger = m_pEntity->GetOrCreateComponent<IComponentTrigger>();
    if (!pTrigger)
            return;

    AABB bbox;
    bbox.min = -Vec3(fMaxDist,fMaxDist,fMaxDist);
    bbox.max = Vec3(fMaxDist,fMaxDist,fMaxDist);
    pTrigger->SetTriggerBounds( bbox );
    */

    IComponentAreaPtr pArea = m_pEntity->GetOrCreateComponent<IComponentArea>();
    if (!pArea)
    {
        return;
    }

    pArea->SetFlags(pArea->GetFlags() & IComponentArea::FLAG_NOT_SERIALIZE);
    pArea->SetSphere(Vec3(0, 0, 0), fMaxDist);
    pArea->AddEntity(m_pEntity->GetId()); // add itself.
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoids::OnTrigger(bool bEnter, SEntityEvent& event)
{
    EntityId whoId = (EntityId)event.nParam[0];
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(whoId);

    if (pEntity)
    {
        if (pEntity->GetFlags() & ENTITY_FLAG_LOCAL_PLAYER)
        {
            if (bEnter)
            {
                m_playersInCount++;
            }
            else
            {
                m_playersInCount--;
            }

            if (m_playersInCount == 1)
            {
                // Activates entity when player is nearby.
                m_pEntity->Activate(true);
                if (m_pFlock)
                {
                    m_pFlock->SetEnabled(true);
                }
            }
            if (m_playersInCount <= 0)
            {
                // Activates entity when player is nearby.
                m_playersInCount = 0;
                m_pEntity->Activate(false);
                if (m_pFlock)
                {
                    m_pFlock->SetEnabled(false);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CComponentBoidObject::CComponentBoidObject()
    : m_pBoid(NULL)
    , m_pEntity(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
CComponentBoidObject::~CComponentBoidObject()
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoidObject::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;

    m_pEntity->GetOrCreateComponent<IComponentSerialization>()
        ->Register<CComponentBoidObject>(SerializationOrder::BoidObject, *this, &CComponentBoidObject::Serialize,
        &CComponentBoidObject::SerializeXML, &CComponentBoidObject::NeedSerialize,
        &CComponentBoidObject::GetSignature);
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoidObject::ProcessEvent(SEntityEvent& event)
{
    if (m_pBoid)
    {
        m_pBoid->OnEntityEvent(event);
    }
}

///////////////////////////////////////////////////////////78//////////////
bool CComponentBoidObject::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentBoidObject");
    if (m_pBoid && m_pBoid->m_flock)
    {
        uint32 type = (uint32)m_pBoid->m_flock->GetType();
        signature.Value("type", type);
    }
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentBoidObject::Serialize(TSerialize ser)
{
}
