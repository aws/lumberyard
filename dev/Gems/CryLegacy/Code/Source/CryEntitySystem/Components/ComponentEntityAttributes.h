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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTENTITYATTRIBUTES_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTENTITYATTRIBUTES_H
#pragma once

#include "EntitySystem.h"
#include "Components/IComponentEntityAttributes.h"
#include "IEntityClass.h"
#include "ISerialize.h"

//! Implementation of the attribute storage component.
//! This component exists to store IEntityAttributes for its entity.
class CComponentEntityAttributes
    : public IComponentEntityAttributes
{
public:
    // IComponent
    void ProcessEvent(SEntityEvent& event) override;
    void Initialize(SComponentInitializer const& inititializer) override;
    void Done() override;
    void UpdateComponent(SEntityUpdateContext& context) override;
    bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params) override;
    void Reload(IEntity* pEntity, SEntitySpawnParams& params) override;
    void SerializeXML(XmlNodeRef& entityNodeXML, bool loading);
    void Serialize(TSerialize serialize);
    bool NeedSerialize();
    bool GetSignature(TSerialize signature);
    void GetMemoryUsage(ICrySizer* pSizer) const override;
    // ~IComponent

    // IComponentEntityAttributes
    virtual void SetAttributes(const TEntityAttributeArray& attributes) override;
    virtual TEntityAttributeArray& GetAttributes() override;
    virtual const TEntityAttributeArray& GetAttributes() const override;
    // ~IComponentEntityAttributes

private:

    TEntityAttributeArray   m_attributes;
};

DECLARE_SMART_POINTERS(CComponentEntityAttributes)

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTENTITYATTRIBUTES_H