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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTBOIDS_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTBOIDS_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

//! Interface for the entity's boids component.
//! The Boids component allows an entity to host flocks.
struct IComponentBoids
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentBoids", 0x91E7912824E4452F, 0xB996340364A55E6D)

    IComponentBoids() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Boids; }

    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentBoids);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTBOIDS_H