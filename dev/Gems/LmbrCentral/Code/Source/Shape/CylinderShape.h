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

#include <Cry_Geo.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>

namespace LmbrCentral
{
    class CylinderShape        
        : public ShapeComponentRequestsBus::Handler
        , public CylinderShapeComponentRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        
        // Runtime data
        class CylinderIntersectionDataCache : public IntersectionTestDataCache<CylinderShapeConfig>
        {
        public:
            friend class CylinderShape;

            void UpdateIntersectionParams(const AZ::Transform& currentTransform, const CylinderShapeConfig& configuration) override;

        private:

            // Values that are used for cylinder intersection tests
            // The center point of the cylinder
            AZ::Vector3 m_baseCenterPoint;

            // A vector along the axis of this cylinder scaled to the height of the cylinder
            AZ::Vector3 m_axisVector;

            // Length of the axis squared
            float m_axisLengthSquared;

            // Radius of the cylinder squared
            float m_radiusSquared;
        };

        void Activate(const AZ::EntityId& entityId);
        void Deactivate();
        void InvalidateCache(CylinderIntersectionDataCache::CacheStatus reason);

        // Get a reference to the underlying cylinder configuration
        virtual CylinderShapeConfig& GetConfiguration() = 0;

        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override { return AZ::Crc32("Cylinder"); }
        bool IsPointInside(const AZ::Vector3& point) override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        AZ::Aabb GetEncompassingAabb() override;
                
        // ShapeBoxComponentRequestBus::Handler implementation
        CylinderShapeConfig GetCylinderConfiguration() override { return GetConfiguration(); }
        void SetHeight(float newHeight) override;
        void SetRadius(float newRadius) override;        
        
        // Transform notification bus listener        
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;   

    private:

        //! Caches transient intersection data
        CylinderIntersectionDataCache m_intersectionDataCache;

        //! Caches the current World transform
        AZ::Transform m_currentWorldTransform;

        //! The Id of the entity the shape is attached to
        AZ::EntityId m_entityId;
    };    
} // namespace LmbrCentral
