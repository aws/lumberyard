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
#include <AzFramework/Physics/ColliderComponentBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Shape.h>

namespace PhysX
{
    class Shape;
    class RigidBodyStatic;

    /// Messages serviced by a PhysX collider component.
    /// A PhysX collider component allows collision geometry to be attached to bodies in PhysX.
    class ColliderComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // LUMBERYARD_DEPRECATED(LY-101785)
        /// @deprecated Please use GetShapeConfigurations instead.
        AZ_DEPRECATED(virtual AZStd::shared_ptr<Physics::ShapeConfiguration> GetShapeConfigFromEntity() = 0;,
            "Colliders now support multiple shapes.  Please use GetShapeConfigurations instead to get a "
            "container of ColliderConfiguration, ShapeConfiguration pairs.")

        // LUMBERYARD_DEPRECATED(LY-101785)
        /// @deprecated Please use GetShapeConfigurations instead.
        AZ_DEPRECATED(virtual const Physics::ColliderConfiguration& GetColliderConfig() = 0;,
            "Colliders now support multiple shapes.  Please use GetShapeConfigurations instead to get a "
            "container of ColliderConfiguration, ShapeConfiguration pairs.")

        // LUMBERYARD_DEPRECATED(LY-101785)
        /// @deprecated Please use GetShapes instead.
        AZ_DEPRECATED(virtual AZStd::shared_ptr<Physics::Shape> GetShape() = 0;,
            "Colliders now support multiple shapes.  Please use GetShapes instead.")

        // LUMBERYARD_DEPRECATED(LY-101785)
        /// @deprecated Please use GetShapes instead.
        AZ_DEPRECATED(virtual void* GetNativePointer() = 0;,
            "Colliders now support multiple shapes.  Please use GetShapes instead.  "
            "GetNativePointer may be called on each Shape in the container returned by GetShapes.")

        /// Gets the collection of collider configuration / shape configuration pairs used to define the collider's shapes.
        virtual Physics::ShapeConfigurationList GetShapeConfigurations() = 0;

        /// Gets the collection of physics shapes associated with the collider.
        virtual AZStd::vector<AZStd::shared_ptr<Physics::Shape>> GetShapes() = 0;

        // LUMBERYARD_DEPRECATED(LY-108535)
        /// @deprecated Please use StaticRigidBodyComponent directly instead.
        /// Checks if this collider component is associated with a static rigid body.
        /// Checks whether this collider component exists on an entity without a rigid body component,
        /// in which case a static rigid body will automatically be created.
        /// @return true if static rigid body is created for this collider component
        virtual bool IsStaticRigidBody() = 0;

        // LUMBERYARD_DEPRECATED(LY-108535)
        /// @deprecated Please use StaticRigidBodyComponent directly instead.
        /// Gets the static rigid body associated with the collider if one was created.
        /// @return the static rigid body pointer
        virtual PhysX::RigidBodyStatic* GetStaticRigidBody() = 0;
    };
    using ColliderComponentRequestBus = AZ::EBus<ColliderComponentRequests>;

    /// Events dispatched by a PhysX collider component.
    /// A PhysX collider component allows collision geometry to be attached to bodies in PhysX.
    //! LUMBERYARD_DEPRECATED(LY-114678) Use Physics::ColliderComponentEvents instead
    using ColliderComponentEvents = Physics::ColliderComponentEvents;
    using ColliderComponentEventBus = AZ::EBus<Physics::ColliderComponentEvents>;
} // namespace PhysX
