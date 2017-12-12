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

    class BoxShape
        : public ShapeComponentRequestsBus::Handler
        , public BoxShapeComponentRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        // Runtime data 
        class BoxIntersectionDataCache : public IntersectionTestDataCache<BoxShapeConfig>
        {
        public:           

            void UpdateIntersectionParams(const AZ::Transform& currentTransform, const BoxShapeConfig& configuration) override;

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

        void Activate(const AZ::EntityId& entityId);
        void Deactivate();
        void InvalidateCache(BoxIntersectionDataCache::CacheStatus reason);

        // Get a reference to the underlying box configuration
        virtual BoxShapeConfig& GetConfiguration() = 0;

        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override { return AZ::Crc32("Box"); }
        AZ::Aabb GetEncompassingAabb() override;
        bool IsPointInside(const AZ::Vector3& point) override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;

        // BoxShapeComponentRequestBus::Handler implementation
        inline BoxShapeConfig GetBoxConfiguration() override { return GetConfiguration(); }
        inline AZ::Vector3 GetBoxDimensions() override { return GetConfiguration().GetDimensions(); }
        void SetBoxDimensions(AZ::Vector3) override;

        // Transform notification bus listener        
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;

    private:
        // Caches transient intersection data
        BoxIntersectionDataCache m_intersectionDataCache;

        //! Caches the current transform for the entity on which this component lives
        AZ::Transform m_currentTransform;

        // Id of the entity the box shape is attached to
        AZ::EntityId m_entityId;
    };
} // namespace LmbrCentral
