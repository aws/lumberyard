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

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /*
    * Wrapper for cache of data used for intersection tests
    */
    template <typename ShapeConfiguration>
    class IntersectionTestDataCache
    {
    public:

        enum CacheStatus
        {
            Current, ///< No cache invalidating operations have happened since the last time the cache was updated and the values therein are ready to use
            Obsolete_TransformChange, ///< The cache is invalid because the transform of the entity changed, this will result in a partial recalculation of some intersection parameters
            Obsolete_ShapeChange ///< The cache is invalid because the transform of the entity changed, this will result in a full recalculation of intersection parameters
        };

        /**
        * \brief Updates the intersection Data cache to reflect intersection data for the current state of the shape
        * \param currentTransform The current Transform of the entity
        * \param configuration The shape configuration of a specific shape type
        */
        virtual void UpdateIntersectionParams(const AZ::Transform& currentTransform, const ShapeConfiguration& configuration) = 0;

        AZ_INLINE CacheStatus GetCacheStatus() const
        {
            return m_cacheStatus;
        }

        AZ_INLINE void SetCacheStatus(CacheStatus newStatus)
        {
            m_cacheStatus = newStatus;
        }

    protected:

        CacheStatus m_cacheStatus = CacheStatus::Obsolete_ShapeChange;
    };

    struct ShapeComponentGeneric
    {
        static void Reflect(AZ::ReflectContext* context);
    };

    /*!
    * Services provided by the Shape Component
    */
    class ShapeComponentRequests : public AZ::ComponentBus
    {
    public:

        /**
        * \brief Returns the type of shape that this component holds
        * \return Crc32 indicating the type of shape
        */
        virtual AZ::Crc32 GetShapeType() { return AZ::Crc32(); }

        /**
        * \brief Returns an AABB that encompasses this entire shape
        * \return AABB that encompasses the shape
        */
        virtual AZ::Aabb GetEncompassingAabb() { return AZ::Aabb(); }

        /**
        * \brief Checks if a given point is inside a shape or outside it
        * \param point Vector3 indicating the point to be tested
        * \return bool indicating whether the point is inside or out
        */
        virtual bool IsPointInside(const AZ::Vector3& point) = 0;

        /**
        * \brief Returns the min distance a given point is from the shape
        * \param point Vector3 indicating point to calculate distance from
        * \return float indicating distance point is from shape
        */
        virtual float DistanceFromPoint(const AZ::Vector3& point)
        {
            return sqrtf(DistanceSquaredFromPoint(point));
        }

        /**
        * \brief Returns the min squared distance a given point is from the shape
        * \param point Vector3 indicating point to calculate square distance from
        * \return float indicating square distance point is from shape
        */
        virtual float DistanceSquaredFromPoint(const AZ::Vector3& point) = 0;

        virtual ~ShapeComponentRequests() = default;
    };

    // Bus to service the Shape component requests event group
    using ShapeComponentRequestsBus = AZ::EBus<ShapeComponentRequests>;

    // Notifications sent by the shape component.
    class ShapeComponentNotifications : public AZ::ComponentBus
    {
        public:

            enum class ShapeChangeReasons
            {
                TransformChanged,
                ShapeChanged
            };

            /**
            * Informs listeners that the shape component has been updated (The shape was modified)
            * @param changeReason Informs listeners of the reason for this shape change (transform change, the shape dimensions being altered)
            */
            virtual void OnShapeChanged(ShapeChangeReasons /*changeReason*/) = 0;
    };

    // Bus to service Shape component notifications event group
    using ShapeComponentNotificationsBus = AZ::EBus<ShapeComponentNotifications>;
} // namespace LmbrCentral