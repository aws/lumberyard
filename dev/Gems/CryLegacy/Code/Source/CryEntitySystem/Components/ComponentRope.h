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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTROPE_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTROPE_H
#pragma once

#include "Components/IComponentRope.h"

// forward declarations.
struct SEntityEvent;
struct IPhysicalEntity;
struct IPhysicalWorld;

//! Implementation of the entity's rope component.
//! This component allows the entity to be a rope.
//! A rope has its own render and physics data;
//! it does not make use of the normal physics or render components.
class CComponentRope
    : public IComponentRope
{
public:
    CComponentRope();
    ~CComponentRope() override;
    IEntity* GetEntity() const { return m_pEntity; };

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void Initialize(const SComponentInitializer& init);
    virtual void ProcessEvent(SEntityEvent& event);
    virtual void Done() {};
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading);
    virtual bool NeedSerialize();
    virtual void Serialize(TSerialize ser);
    virtual bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    /// IComponentRope
    //////////////////////////////////////////////////////////////////////////
    virtual IRopeRenderNode* GetRopeRenderNode() const { return m_pRopeRenderNode; };
    //////////////////////////////////////////////////////////////////////////

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
    void PreserveParams() override;
protected:
    IEntity* m_pEntity;

    IRopeRenderNode* m_pRopeRenderNode;
    int m_nSegmentsOrg;
    float m_texTileVOrg;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTROPE_H

