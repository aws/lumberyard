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
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

namespace LmbrCentral
{
    class BoxShapeComponent
        : public AZ::Component
        , private ShapeComponentRequestsBus::Handler
        , private BoxShapeComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:

        friend class EditorBoxShapeComponent;

        AZ_COMPONENT(BoxShapeComponent, "{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override
        {
            return AZ::Crc32("Box");
        }

        AZ::Aabb GetEncompassingAabb() override;

        bool IsPointInside(const AZ::Vector3& point) override;

        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ShapeBoxComponentRequestBus::Handler implementation
        BoxShapeConfiguration GetBoxConfiguration() override
        {
            return m_configuration;
        }

        void SetBoxDimensions(AZ::Vector3) override;
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
            provided.push_back(AZ_CRC("BoxShapeService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService"));
            incompatible.push_back(AZ_CRC("BoxShapeService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:

        //! Stores configuration of a box for this component
        BoxShapeConfiguration m_configuration;

        //////////////////////////////////////////////////////////////////////////
        // Runtime data 
        class BoxIntersectionDataCache : public IntersectionTestDataCache<BoxShapeConfiguration>
        {
        public:
            void UpdateIntersectionParams(const AZ::Transform& currentTransform,
                                          const BoxShapeConfiguration& configuration) override;

            AZ_INLINE const AABB& GetAABB() const
            {
                AZ_Warning("Box Shape Component", m_isAxisAligned, "Fetching AABB of a non axis aligned box, Use GetOBB instead");
                return m_aabb;
            }

            AZ_INLINE const OBB& GetOBB() const
            {
                AZ_Warning("Box Shape Component", !m_isAxisAligned, "Fetching OBB of an axis aligned box, Use GetAABB instead");
                return m_obb;
            }

            AZ_INLINE const AZ::Vector3& GetCurrentPosition() const
            {
                return m_currentPosition;
            }

            AZ_INLINE bool IsAxisAligned() const
            {
                return m_isAxisAligned;
            }

        private:

            //! Indicates whether the box is axis or object aligned
            bool m_isAxisAligned = true;

            //! AABB representing this Box
            AABB m_aabb;
            OBB m_obb;

            // Position of the Box
            AZ::Vector3 m_currentPosition;
        };

        // Caches transient intersection data
        BoxIntersectionDataCache m_intersectionDataCache;

        //! Caches the current transform for the entity on which this component lives
        AZ::Transform m_currentTransform;
    };
} // namespace LmbrCentral
