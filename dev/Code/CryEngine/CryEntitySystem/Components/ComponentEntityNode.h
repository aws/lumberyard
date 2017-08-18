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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTENTITYNODE_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTENTITYNODE_H
#pragma once

#include "EntitySystem.h"
#include "ISerialize.h"
#include "Components/IComponentEntityNode.h"

//! Implementation of the component that coordinates with the CAnimEntityNode
//! by reacting to specific animation events.
//! For example, a 'footstep' animation event could result in a sound
//! that emanates from the location of this entity's foot bone.
class CComponentEntityNode
    : public IComponentEntityNode
{
public:

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void Initialize(const SComponentInitializer& init);
    virtual void ProcessEvent(SEntityEvent& event);
    //////////////////////////////////////////////////////////////////////////

    void GetMemoryUsage(ICrySizer* pSizer) const{};

    bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params) { m_pEntity = pEntity; return true; };
    void Reload(IEntity* pEntity, SEntitySpawnParams& params) { m_pEntity = pEntity; };

    void Done() {};
    void UpdateComponent(SEntityUpdateContext& ctx) {};

    void SerializeXML(XmlNodeRef& entityNode, bool bLoading) {};
    void Serialize(TSerialize ser) {};
    bool NeedSerialize() { return false; };
    bool GetSignature(TSerialize signature) { return true; };

private:
    IEntity* m_pEntity;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTENTITYNODE_H
