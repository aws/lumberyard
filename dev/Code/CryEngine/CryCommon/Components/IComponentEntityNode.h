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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTENTITYNODE_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTENTITYNODE_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

//! Interface for the component that coordinates with the CAnimEntityNode
//! by reacting to specific animation events.
//! For example, a 'footstep' animation event could result in a sound
//! that emanates from the location of this entity's foot bone.
struct IComponentEntityNode
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentEntityNode", 0x290AE744A0A549CB, 0x8510D806A89DE91D)

    IComponentEntityNode() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::EntityNode; }

    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentEntityNode);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTENTITYNODE_H