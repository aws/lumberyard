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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTROPE_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTROPE_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

//! Interface for the entity's rope component.
//! This component allows the entity to be a rope.
//! A rope has its own render and physics data;
//! it does not make use of the normal physics or render components.
struct IComponentRope
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentRope", 0x713068A4F0E8484F, 0x8D5F37A0E43347E0)

    IComponentRope() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Rope; }

    virtual struct IRopeRenderNode* GetRopeRenderNode() const = 0;
    virtual void PreserveParams() = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentRope);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTROPE_H