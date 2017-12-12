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
    class CylinderShapeComponent
        : public AZ::Component
        , private ShapeComponentRequestsBus::Handler
        , private CylinderShapeComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:

        friend class EditorCylinderShapeComponent;

        AZ_COMPONENT(CylinderShapeComponent, "{B0C6AA97-E754-4E33-8D32-33E267DB622F}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override
        {
            return AZ::Crc32("Cylinder");
        }

        bool IsPointInside(const AZ::Vector3& point) override;

        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;

        AZ::Aabb GetEncompassingAabb() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeBoxComponentRequestBus::Handler implementation
        CylinderShapeConfiguration GetCylinderConfiguration() override
        {
            return m_configuration;
        }

        void SetHeight(float newHeight) override;
        void SetRadius(float newRadius) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // Transform notification bus listener
        /// Called when the local transform of the entity has changed. Local transform update always implies world transform change too.
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////////////

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService"));
            provided.push_back(AZ_CRC("CylinderShapeService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService"));
            incompatible.push_back(AZ_CRC("CylinderShapeService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        //////////////////////////////////////////////////////////////////////////
        // Serialized data

        //! Stores configuration of a cylinder for this component
        CylinderShapeConfiguration m_configuration;

        //////////////////////////////////////////////////////////////////////////
        // Runtime data
        class CylinderIntersectionDataCache : public IntersectionTestDataCache<CylinderShapeConfiguration>
        {
        public:
            friend class CylinderShapeComponent;

            void UpdateIntersectionParams(const AZ::Transform& currentTransform, 
                                          const CylinderShapeConfiguration& configuration) override;

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

        // Caches transient intersection data
        CylinderIntersectionDataCache m_intersectionDataCache;
        
        //! Caches the current World transform
        AZ::Transform m_currentWorldTransform;
    };
} // namespace LmbrCentral
