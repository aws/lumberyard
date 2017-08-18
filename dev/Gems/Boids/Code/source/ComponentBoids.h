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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_BOIDSCOMPONENT_H
#define CRYINCLUDE_GAMEDLL_BOIDS_BOIDSCOMPONENT_H
#pragma once

#include <IComponent.h>

class CFlock;
class CBoidObject;

//! Implementation of the entity's boids component.
//! The Boids component allows an entity to host flocks.
struct CComponentBoids
    : public IComponent
{
    DECLARE_COMPONENT_TYPE("ComponentBoids", 0xD4FDFD0B5F4C497A, 0xA46A082F38D5D37D);

    CComponentBoids();
    ~CComponentBoids() override;
    IEntity* GetEntity() const { return m_pEntity; };

    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    void Initialize(const SComponentInitializer& init) override;
    void Done() override {};
    void UpdateComponent(SEntityUpdateContext& ctx) override;
    void ProcessEvent(SEntityEvent& event) override;
    bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params) override { return true; }
    void Reload(IEntity* pEntity, SEntitySpawnParams& params) override;
    void SerializeXML(XmlNodeRef& entityNode, bool bLoading){};
    void Serialize(TSerialize ser);
    bool NeedSerialize() { return false; };
    bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void SetFlock(CFlock* pFlock);
    CFlock* GetFlock() { return m_pFlock; }
    void OnTrigger(bool bEnter, SEntityEvent& event);

    void GetMemoryUsage(ICrySizer* pSizer) const override
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pFlock);
    }

private:
    void OnMove();

private:
    //////////////////////////////////////////////////////////////////////////
    // Private member variables.
    //////////////////////////////////////////////////////////////////////////
    // Host entity.
    IEntity* m_pEntity;

    // Flock of items.
    CFlock* m_pFlock;

    int m_playersInCount;
};

DECLARE_COMPONENT_POINTERS(CComponentBoids);

struct CComponentBoidObject
    : public IComponent
{
    DECLARE_COMPONENT_TYPE("ComponentBoidObject", 0x85F2F92FA7C54F7F, 0x9BBE1A6B9D472798);

    CComponentBoidObject();
    ~CComponentBoidObject() override;
    IEntity* GetEntity() const { return m_pEntity; };

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    void Initialize(const SComponentInitializer& init) override;
    void Done() override {};
    void UpdateComponent(SEntityUpdateContext& ctx) override {};
    void ProcessEvent(SEntityEvent& event) override;
    bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params) override { return true; }
    void Reload(IEntity* pEntity, SEntitySpawnParams& params) override {};
    void SerializeXML(XmlNodeRef& entityNode, bool bLoading){};
    void Serialize(TSerialize ser);
    bool NeedSerialize() { return false; };
    bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void SetBoid(CBoidObject* pBoid) { m_pBoid = pBoid; };
    CBoidObject* GetBoid() { return m_pBoid; }

    void GetMemoryUsage(ICrySizer* pSizer) const override
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pBoid);
    }

private:
    //////////////////////////////////////////////////////////////////////////
    // Private member variables.
    //////////////////////////////////////////////////////////////////////////
    // Host entity.
    IEntity* m_pEntity;
    // Host Flock.
    CBoidObject* m_pBoid;
};

DECLARE_COMPONENT_POINTERS(CComponentBoidObject);

#endif // CRYINCLUDE_GAMEDLL_BOIDS_BOIDSCOMPONENT_H