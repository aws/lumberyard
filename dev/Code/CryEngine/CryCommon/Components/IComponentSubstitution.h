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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTSUBSTITUTION_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTSUBSTITUTION_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

//! Interface for the entity's substitution component.
//! This component remembers the IRenderNode that this entity substitutes
//! and unhides it upon deletion
struct IComponentSubstitution
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentSubstitution", 0x73EEB283990F4DB2, 0x943916C287108EF0)

    IComponentSubstitution() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Substitution; }

    virtual void SetSubstitute(IRenderNode* pSubstitute) = 0;
    virtual IRenderNode* GetSubstitute() = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentSubstitution);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTSUBSTITUTION_H