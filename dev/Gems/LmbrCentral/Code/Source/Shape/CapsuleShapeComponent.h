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
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>

namespace LmbrCentral
{
    class CapsuleShapeComponent
        : public AZ::Component
        , private ShapeComponentRequestsBus::Handler
        , private CapsuleShapeComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:

        friend class EditorCapsuleShapeComponent;

        AZ_COMPONENT(CapsuleShapeComponent, "{967EC13D-364D-4696-AB5C-C00CC05A2305}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override
        {
            return AZ::Crc32("Capsule");
        }
        AZ::Aabb GetEncompassingAabb() override;


        bool IsPointInside(const AZ::Vector3& point) override;

        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeBoxComponentRequestBus::Handler implementation
        CapsuleShapeConfiguration GetCapsuleConfiguration() override
        {
            return m_configuration;
        }

        void SetHeight(float newHeight) override;
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
            provided.push_back(AZ_CRC("CapsuleShapeService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService"));
            incompatible.push_back(AZ_CRC("CapsuleShapeService"));
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
        CapsuleShapeConfiguration m_configuration;

        //////////////////////////////////////////////////////////////////////////
        // Runtime data
        class CapsuleIntersectionDataCache : public IntersectionTestDataCache<CapsuleShapeConfiguration>
        {
        public:
            friend class CapsuleShapeComponent;

            void UpdateIntersectionParams(const AZ::Transform& currentTransform,
                const CapsuleShapeConfiguration& configuration) override;

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

        // Caches transient intersection data
        CapsuleIntersectionDataCache m_intersectionDataCache;

        //! Caches the current World transform
        AZ::Transform m_currentWorldTransform;
    };
} // namespace LmbrCentral
