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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>

namespace LmbrCentral
{
    class CapsuleShape
        : public ShapeComponentRequestsBus::Handler
        , public CapsuleShapeComponentRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        
        // Runtime data
        class CapsuleIntersectionDataCache : public IntersectionTestDataCache<CapsuleShapeConfig>
        {
        public:
            friend class CapsuleShape;
            friend class EditorCapsuleShapeComponent;

            void UpdateIntersectionParams(const AZ::Transform& currentTransform, const CapsuleShapeConfig& configuration) override;

        private:

            // Values that are used for capsule intersection tests
            // The end points of the cylinder inside the capsule
            AZ::Vector3 m_basePlaneCenterPoint;
            AZ::Vector3 m_topPlaneCenterPoint;

            // A unit vector along the axis of this capsule
            AZ::Vector3 m_axisVector;

            // Length of the cylinder axis squared
            float m_axisLengthSquared;

            // Radius of the capsule squared
            float m_radiusSquared;

            // indicates whether the cylinder is actually just a sphere
            bool m_isSphere = false;
        };        

        void Activate(const AZ::EntityId& entityId);
        void Deactivate();
        void InvalidateCache(CapsuleIntersectionDataCache::CacheStatus reason);

        // Get a reference to the underlying capsule shape configuration
        virtual CapsuleShapeConfig& GetConfiguration() = 0;

        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override { return AZ::Crc32("Capsule"); }
        AZ::Aabb GetEncompassingAabb() override;
        bool IsPointInside(const AZ::Vector3& point) override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        
        // ShapeBoxComponentRequestBus::Handler implementation
        CapsuleShapeConfig GetCapsuleConfiguration() override { return GetConfiguration(); }
        void SetHeight(float newHeight) override;
        void SetRadius(float newRadius) override;    
        
        // Transform notification bus listener        
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;

    private:
        // Caches transient intersection data
        CapsuleIntersectionDataCache m_intersectionDataCache;   

        //! Caches the current World transform
        AZ::Transform m_currentWorldTransform;

        // Id of the entity the shape is attached to
        AZ::EntityId m_entityId;
    };

} // namespace LmbrCentral
