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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Color.h>
#include <LmbrCentral/Physics/ForceVolumeRequestBus.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace AzFramework 
{
    class EntityDebugDisplayRequests;
}

namespace LmbrCentral
{
    /**
     * Manages a list of forces attached to a
     * force volume component.
     */
    class ForceVolume
        : private AZ::TransformNotificationBus::MultiHandler
        , private LmbrCentral::SplineComponentNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceVolume, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ForceVolume, "{18DF175C-01D1-4501-A73D-4A9370EBD9D5}");
        static void Reflect(AZ::ReflectContext* context);

        ForceVolume() = default;
        ForceVolume(const ForceVolume& forceVolume);
        ~ForceVolume();

        void Activate(AZ::EntityId entityId);
        void Deactivate();

        /**
         * Calculates the net force acting on an entity in the force volume.
         */
        AZ::Vector3 CalculateNetForce(const EntityParams& entity) const;

        /**
         * Displays the force volume.
         */
        void Display(AzFramework::EntityDebugDisplayRequests& displayContext);

    private:
        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // SplineComponentNotificationBus
        void OnSplineChanged() override;

        // ShapeComponentNotificationBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        AZ::EntityId m_entityId; ///< Entity id of the volume.
        AZ::Transform m_worldTransform; ///< The world transform of the volume.
        AZStd::vector<Force*> m_forces; ///< List of forces attached to the volume.
        VolumeParams m_volumeParams;
        static const AZ::Color s_arrowColor;
    };

    namespace ForceVolumeUtil
    {
        using PointList = AZStd::vector<AZ::Vector3>;

        /**
         * Creates a structure with params about the force volume used.
         * to calculate a resulting force.
         */
        VolumeParams CreateVolumeParams(const AZ::EntityId& entityId);

        /**
         * Creates a structure with params about en entity used.
         * to calculate a resulting force.
         */
        EntityParams CreateEntityParams(const AZ::EntityId& entityId);

        /**
         * Gets maximum aabb for an entity
         */
        AZ::Aabb GetMaxAabb(const AZ::EntityId& entityId);

        /**
         * Generates a list of points on a box.
         */
        PointList GenerateBoxPoints(const AZ::Vector3& min, const AZ::Vector3& max);

        /**
         * Generates a list of points on the surface of a sphere.
         */
        PointList GenerateSpherePoints(float radius);

        /**
         * Generates a list of points on the surface of a tube.
         */
        PointList GenerateTubePoints(const AZ::Spline& spline, float radius);

        /**
         * Generates a list of points on the surface of a cylinder.
         */
        PointList GenerateCylinderPoints(float height, float radius);

        /**
         * Generates a list of points in a circle
         */
        void GenerateCirclePoints(int sides, float radius, const AZ::Vector3& position, const AZ::Vector3& normal, const AZ::Vector3& tangent, PointList& points);
        
        /**
         * Draws an arrow in the direction.
         */
        void DisplayForceDirection(const ForceVolume& forceVolume, AzFramework::EntityDebugDisplayRequests& displayContext, const AZ::Vector3& worldPoint);
    }
}
