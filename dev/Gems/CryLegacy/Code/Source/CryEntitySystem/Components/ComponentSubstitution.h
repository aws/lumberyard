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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTSUBSTITUTION_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTSUBSTITUTION_H
#pragma once

#include "Components/IComponentSubstitution.h"

//! Implementation of the entity's substitution component.
//! This component remembers the IRenderNode that this entity substitutes
//! and unhides it upon deletion
struct CComponentSubstitution
    : IComponentSubstitution
{
public:
    CComponentSubstitution() { m_pSubstitute = 0; }
    ~CComponentSubstitution()
    {
        if (m_pSubstitute)
        {
            m_pSubstitute->ReleaseNode();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void ProcessEvent(SEntityEvent& event) {}
    virtual void Done();
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading) {}
    virtual void Serialize(TSerialize ser);
    virtual bool NeedSerialize();
    virtual bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IComponentSubstitution interface.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetSubstitute(IRenderNode* pSubstitute);
    virtual IRenderNode* GetSubstitute() { return m_pSubstitute; }
    //////////////////////////////////////////////////////////////////////////

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
protected:
    IRenderNode* m_pSubstitute;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTSUBSTITUTION_H

