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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTTRIGGER_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTTRIGGER_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

//! Interface for the entity's trigger component.
//! This component will detect collisions of entities with a bounding box,
//! sending events to interested parties.
struct IComponentTrigger
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentTrigger", 0xEB87BF3317B94E89, 0x838244AC116F5F12);

    IComponentTrigger() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Trigger; }

    // Description:
    //    Creates a trigger bounding box.
    //    When physics will detect collision with this bounding box it will send an events to the entity.
    //    If entity have script OnEnterArea and OnLeaveArea events will be called.
    // Arguments:
    //    bbox - Axis aligned bounding box of the trigger in entity local space (Rotation and scale of the entity is ignored).
    //           Set empty bounding box to disable trigger.
    virtual void SetTriggerBounds(const AABB& bbox) = 0;

    // Description:
    //    Retrieve trigger bounding box in local space.
    // Return:
    //    Axis aligned bounding box of the trigger in the local space.
    virtual void GetTriggerBounds(AABB& bbox) const = 0;

    // Description:
    //    Forward enter/leave events to this entity
    virtual void ForwardEventsTo(EntityId id) = 0;

    // Description:
    //    Invalidate the trigger, so it gets recalculated and catches things which are already inside
    //      when it gets enabled.
    virtual void InvalidateTrigger() = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentTrigger);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTTRIGGER_H