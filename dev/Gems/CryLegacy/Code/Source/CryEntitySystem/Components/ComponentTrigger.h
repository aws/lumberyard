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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTTRIGGER_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTTRIGGER_H
#pragma once

#include "EntitySystem.h"
#include "Components/IComponentTrigger.h"

struct SProximityElement;

//! Implementation of the entity's trigger component.
//! This component will detect collisions of entities with a bounding box,
//! sending events to interested parties.
class CComponentTrigger
    : public IComponentTrigger
{
public:
    CComponentTrigger();
    ~CComponentTrigger() override;

    IEntity* GetEntity() const { return m_pEntity; };

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void Initialize(const SComponentInitializer& init);
    virtual void ProcessEvent(SEntityEvent& event);
    virtual void Done() {};
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading) {};
    virtual void Serialize(TSerialize ser);
    virtual bool NeedSerialize();
    virtual bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IComponentTrigger
    //////////////////////////////////////////////////////////////////////////
    virtual void SetTriggerBounds(const AABB& bbox) { SetAABB(bbox); };
    virtual void GetTriggerBounds(AABB& bbox) const { bbox = m_aabb; };
    virtual void ForwardEventsTo(EntityId id) { m_forwardingEntity = id; }
    virtual void InvalidateTrigger();
    //////////////////////////////////////////////////////////////////////////

    const AABB& GetAABB() const { return m_aabb; }
    void SetAABB(const AABB& aabb);

    CProximityTriggerSystem* GetTriggerSystem() { return GetIEntitySystem()->GetProximityTriggerSystem(); }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:
    void OnMove(bool invalidateAABB = false);
    void Reset();

private:
    //////////////////////////////////////////////////////////////////////////
    // Private member variables.
    //////////////////////////////////////////////////////////////////////////
    // Host entity.
    IEntity* m_pEntity;
    AABB m_aabb;
    SProximityElement* m_pProximityTrigger;
    EntityId m_forwardingEntity;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTTRIGGER_H