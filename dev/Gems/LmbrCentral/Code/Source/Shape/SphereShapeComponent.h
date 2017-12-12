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
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>

namespace LmbrCentral
{
    class SphereShapeComponent
        : public AZ::Component
        , private ShapeComponentRequestsBus::Handler
        , private SphereShapeComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:

        friend class EditorSphereShapeComponent;

        AZ_COMPONENT(SphereShapeComponent, "{E24CBFF0-2531-4F8D-A8AB-47AF4D54BCD2}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override
        {
            return AZ::Crc32("Sphere");
        }

        AZ::Aabb GetEncompassingAabb() override;

        bool IsPointInside(const AZ::Vector3& point)  override;

        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SphereShapeComponentRequestsBus::Handler implementation
        SphereShapeConfiguration GetSphereConfiguration() override
        {
            return m_configuration;
        }

        void SetRadius(float newRadius) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // Transform notification bus listener
        /// Called when the local transform of the entity has changed. Local transform update always implies world transform change too.
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;
        //////////////////////////////////////////////////////////////////////////////////
    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService"));
            provided.push_back(AZ_CRC("SphereShapeService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService"));
            incompatible.push_back(AZ_CRC("SphereShapeService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        //! Stores configuration of a sphere for this component
        SphereShapeConfiguration m_configuration;

        //////////////////////////////////////////////////////////////////////////
        // Runtime data 
        class SphereIntersectionDataCache : public IntersectionTestDataCache<SphereShapeConfiguration>
        {
        public:
            void UpdateIntersectionParams(const AZ::Transform& currentTransform,
                                          const SphereShapeConfiguration& configuration) override;

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

        // Caches transient intersection data
        SphereIntersectionDataCache m_intersectionDataCache;

        //! Caches the current transform for the entity on which this component lives
        AZ::Transform m_currentTransform;
    };
} // namespace LmbrCentral
