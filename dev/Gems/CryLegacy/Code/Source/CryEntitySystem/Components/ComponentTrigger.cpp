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
#include "ComponentTrigger.h"
#include "ISerialize.h"
#include "ProximityTriggerSystem.h"
#include "Components/IComponentSerialization.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentTrigger, IComponentTrigger)

//////////////////////////////////////////////////////////////////////////
CComponentTrigger::CComponentTrigger()
    : m_pEntity(NULL)
    , m_forwardingEntity(0)
    , m_pProximityTrigger(NULL)
    , m_aabb(AABB::RESET)
{
}

//////////////////////////////////////////////////////////////////////////
CComponentTrigger::~CComponentTrigger()
{
    GetTriggerSystem()->RemoveTrigger(m_pProximityTrigger);
}

//////////////////////////////////////////////////////////////////////////
void CComponentTrigger::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;
    m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentTrigger>(SerializationOrder::Trigger, *this, &CComponentTrigger::Serialize, &CComponentTrigger::SerializeXML, &CComponentTrigger::NeedSerialize, &CComponentTrigger::GetSignature);

    m_pProximityTrigger = GetTriggerSystem()->CreateTrigger();
    m_pProximityTrigger->id = AZ::EntityId(m_pEntity->GetId());

    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CComponentTrigger::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    assert(pEntity);

    m_pEntity = pEntity;

    assert(m_pProximityTrigger);
    if (m_pProximityTrigger)
    {
        m_pProximityTrigger->id = AZ::EntityId(m_pEntity->GetId());
    }

    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CComponentTrigger::Reset()
{
    m_pEntity->SetTrigger(true);
    m_forwardingEntity = 0;

    // Release existing proximity entity if present, triggers should not trigger themself.
    SProximityElement* pElement = m_pEntity->GetProximityElement();
    if (pElement)
    {
        GetTriggerSystem()->RemoveEntity(pElement);
        m_pEntity->SetProximityElement(nullptr);
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentTrigger::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_XFORM:
        OnMove();
        break;
    case ENTITY_EVENT_ENTERAREA:
    case ENTITY_EVENT_LEAVEAREA:
        if (m_forwardingEntity)
        {
            IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_forwardingEntity);
            if (pEntity && (pEntity != this->GetEntity()))
            {
                pEntity->SendEvent(event);
            }
        }
        break;
    case ENTITY_EVENT_PRE_SERIALIZE:
        break;
    case ENTITY_EVENT_POST_SERIALIZE:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentTrigger::NeedSerialize()
{
    return true;
};

//////////////////////////////////////////////////////////////////////////
bool CComponentTrigger::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentTrigger");
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentTrigger::Serialize(TSerialize ser)
{
    if (ser.GetSerializationTarget() != eST_Network)
    {
        if (ser.BeginOptionalGroup("ComponentTrigger", true))
        {
            ser.Value("BoxMin", m_aabb.min);
            ser.Value("BoxMax", m_aabb.max);
            ser.EndGroup();
        }

        if (ser.IsReading())
        {
            OnMove();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentTrigger::OnMove(bool invalidateAABB)
{
    Vec3 pos = m_pEntity->GetWorldPos();
    AABB box = m_aabb;
    box.min += pos;
    box.max += pos;
    GetTriggerSystem()->MoveTrigger(m_pProximityTrigger, box, invalidateAABB);
}

//////////////////////////////////////////////////////////////////////////
void CComponentTrigger::InvalidateTrigger()
{
    OnMove(true);
}

//////////////////////////////////////////////////////////////////////////
void CComponentTrigger::SetAABB(const AABB& aabb)
{
    m_aabb = aabb;
    OnMove();
}


