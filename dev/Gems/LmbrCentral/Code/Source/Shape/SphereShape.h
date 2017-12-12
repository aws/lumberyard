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

#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>

namespace LmbrCentral
{
    class SphereShape
        : public ShapeComponentRequestsBus::Handler
        , public SphereShapeComponentRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {

    public:
        // Runtime data 
        class SphereIntersectionDataCache : public IntersectionTestDataCache<SphereShapeConfig>
        {
        public:
            void UpdateIntersectionParams(const AZ::Transform& currentTransform,
                const SphereShapeConfig& configuration) override;

            AZ_INLINE float GetRadiusSquared() const
            {
                return m_radiusSquared;
            }

            AZ_INLINE const AZ::Vector3& GetCenterPosition() const
            {
                return m_currentCenterPosition;
            }

        private:
            // Radius of the sphere squared
            float m_radiusSquared;

            // Position of the center of the sphere
            AZ::Vector3 m_currentCenterPosition;
        };

        void Activate(const AZ::EntityId& entityId);
        void Deactivate();        
        void InvalidateCache(SphereIntersectionDataCache::CacheStatus reason);

        // Get a reference to the underlying sphere configuration
        virtual SphereShapeConfig& GetConfiguration() = 0;

        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override { return AZ::Crc32("Sphere"); }
        AZ::Aabb GetEncompassingAabb() override;
        bool IsPointInside(const AZ::Vector3& point)  override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;        
        
        // SphereShapeComponentRequestsBus::Handler implementation
        void SetRadius(float newRadius) override;
        SphereShapeConfig GetSphereConfiguration() override { return GetConfiguration(); }

        // Transform notification bus listener        
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;        

    private:
       
        //! Caches transient intersection data
        SphereIntersectionDataCache m_intersectionDataCache;        

        //! Caches the current World transform
        AZ::Transform m_currentTransform;

        //! The Id of the entity the shape is attached to
        AZ::EntityId m_entityId;
        
    };
} // namespace LmbrCentral
