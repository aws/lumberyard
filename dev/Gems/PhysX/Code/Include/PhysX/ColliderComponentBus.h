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

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace PhysX
{
    /**
    * Messages serviced by a PhysX collider component.
    * A PhysX collider component allows collision geometry to be attached to bodies in PhysX.
    */
    class PhysXColliderComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /**
        * Creates a ShapeConfiguration based on shape information from this entity's shape bus.
        * @return A smart pointer to the created ShapeConfiguration.
        */
        virtual Physics::Ptr<Physics::ShapeConfiguration> GetShapeConfigFromEntity() = 0;
    };
    using PhysXColliderComponentRequestBus = AZ::EBus<PhysXColliderComponentRequests>;

    /**
    * Events dispatched by a PhysX collider component.
    * A PhysX collider component allows collision geometry to be attached to bodies in PhysX.
    */
    class PhysXColliderComponentEvents
        : public AZ::ComponentBus
    {
    public:
        /**
        * Event fired when the collider is updated.
        */
        virtual void OnColliderChanged() {}
    };
    using PhysXColliderComponentEventBus = AZ::EBus<PhysXColliderComponentEvents>;
} // namespace PhysX