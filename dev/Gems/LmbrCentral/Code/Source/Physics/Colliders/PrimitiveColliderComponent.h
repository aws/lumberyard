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

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/Component.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Physics/ColliderComponentBus.h>

namespace primitives
{
    struct primitive;
}

namespace LmbrCentral
{
    /**
     * Configuration data for the PrimitiveColliderComponent.
     */
    struct PrimitiveColliderConfiguration
    {
        AZ_CLASS_ALLOCATOR(PrimitiveColliderConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PrimitiveColliderConfiguration, "{85AA27D6-E019-469F-8472-89862323DBF7}");
        static void Reflect(AZ::ReflectContext* context);

        PrimitiveColliderConfiguration();

        /// Get all available surface types.
        AZStd::vector<AZStd::string> GetSurfaceTypeNames() const;

        /// Physical surface type (\ref ISurfaceType) to use on this collider.
        AZStd::string m_surfaceTypeName;
    };

    /**
     * Base class for components that provide collision
     * in the form of primitive shapes.
     */
    class PrimitiveColliderComponent
        : public AZ::Component
        , public ColliderComponentRequestBus::Handler
        , private ShapeComponentNotificationsBus::Handler
        , public AZ::EntityBus::MultiHandler
    {
    public:
        AZ_COMPONENT(PrimitiveColliderComponent, "{9CB3707A-73B3-4EE5-84EA-3CF86E0E3722}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        PrimitiveColliderComponent() = default;
        explicit PrimitiveColliderComponent(const PrimitiveColliderConfiguration& configuration);
        ~PrimitiveColliderComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ColliderComponentRequests interface implementation
        int AddColliderToPhysicalEntity(IPhysicalEntity& physicalEntity, int nextPartId) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus::Handler
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons) override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EntityEvents
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        ////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);
    private:
        // Adds the shape from a given Entity Id to this physical entity
        int AddEntityShapeToPhysicalEntity(IPhysicalEntity& physicalEntity, int nextPartId, const AZ::EntityId&);
    protected:
        //! Adds geometry for primitive shape to the IPhysicalEntity.
        int AddPrimitiveFromEntityToPhysicalEntity(
            const AZ::EntityId& entityId, 
            IPhysicalEntity& physicalEntity,
            int nextPartId,
            int primitiveType,
            const primitives::primitive& primitive);

        /// Serialized configuration.
        PrimitiveColliderConfiguration m_configuration;

        /// If connecting to an active entity while harvesting colliders
        /// for a compound shape, add the colliders to this entity.
        IPhysicalEntity* m_recipientOfNewlyActivatedShapes = nullptr;

        /// Next partId to use when adding parts to m_recipientOfNewlyActivatedShapes.
        int m_recipientOfNewlyActivatedShapesNextPartId;

        /// Last partId added to m_recipientOfNewlyActivatedShapes.
        int m_recipientOfNewlyActivatedShapesFinalPartId;
    };
} // namespace LmbrCentral
