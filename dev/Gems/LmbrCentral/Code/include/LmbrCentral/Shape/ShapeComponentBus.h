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

namespace AZ
{
    class BehaviorContext;
    enum class RandomDistributionType : AZ::u32;
}
namespace LmbrCentral
{
    /**
     * Reason shape cache should be recalculated.
     */
    enum class InvalidateShapeCacheReason
    {
        TransformChange, ///< The cache is invalid because the transform of the entity changed.
        ShapeChange ///< The cache is invalid because the shape configuration/properties changed.
    };

    /**
     * Wrapper for cache of data used for intersection tests
     */
    template <typename ShapeConfiguration>
    class IntersectionTestDataCache
    {
    public:
        virtual ~IntersectionTestDataCache() = default;

        /**
         * @brief Updates the intersection data cache to reflect the current state of the shape.
         * @param currentTransform The current Transform of the entity.
         * @param configuration The specific configuration of a shape.
         */
        void UpdateIntersectionParams(
            const AZ::Transform& currentTransform, const ShapeConfiguration& configuration)
        {
            // does the cache need updating
            if (m_cacheStatus > ShapeCacheStatus::Current)
            {
                UpdateIntersectionParamsImpl(currentTransform, configuration); // shape specifc cache update
                m_cacheStatus = ShapeCacheStatus::Current; // mark cache as up to date
            }
        }

        /**
         * Mark the cache as needing to be updated.
         */
        void InvalidateCache(const InvalidateShapeCacheReason reason)
        {
            switch (reason)
            {
            case InvalidateShapeCacheReason::TransformChange:
                if (m_cacheStatus < ShapeCacheStatus::ObsoleteTransformChange)
                {
                    m_cacheStatus = ShapeCacheStatus::ObsoleteTransformChange;
                }
                break;
            case InvalidateShapeCacheReason::ShapeChange:
                if (m_cacheStatus < ShapeCacheStatus::ObsoleteShapeChange)
                {
                    m_cacheStatus = ShapeCacheStatus::ObsoleteShapeChange;
                }
                break;
            default:
                break;
            }
        }

    protected:
        /**
         * Derived shape specific implementation of cache update (called from UpdateIntersectionParams).
         */
        virtual void UpdateIntersectionParamsImpl(
            const AZ::Transform& currentTransform, const ShapeConfiguration& configuration) = 0;

        /**
         * State of shape cache - should the internal shape cache be recalculated, or is it up to date.
         */
        enum class ShapeCacheStatus
        {
            Current, ///< Cache is up to date.
            ObsoleteTransformChange, ///< The cache is invalid because the transform of the entity changed.
            ObsoleteShapeChange ///< The cache is invalid because the shape configuration/properties changed.
        };

        /**
         * Expose read only cache status to derived IntersectionTestDataCache if different
         * logic want to hapoen based on the cache status (shape/transform).
         */
        ShapeCacheStatus CacheStatus() const
        {
            return m_cacheStatus;
        }

    private:
        ShapeCacheStatus m_cacheStatus = ShapeCacheStatus::Current; ///< The current state of the shape cache.
    };

    struct ShapeComponentGeneric
    {
        static void Reflect(AZ::ReflectContext* context);
    };

    /**
     * Services provided by the Shape Component
     */
    class ShapeComponentRequests : public AZ::ComponentBus
    {
    public:
        /**
         * @brief Returns the type of shape that this component holds
         * @return Crc32 indicating the type of shape
         */
        virtual AZ::Crc32 GetShapeType() = 0;

        /**
         * @brief Returns an AABB that encompasses this entire shape
         * @return AABB that encompasses the shape
         */
        virtual AZ::Aabb GetEncompassingAabb() = 0;

        /**
         * @brief Checks if a given point is inside a shape or outside it
         * @param point Vector3 indicating the point to be tested
         * @return bool indicating whether the point is inside or out
         */
        virtual bool IsPointInside(const AZ::Vector3& point) = 0;

        /**
         * @brief Returns the min distance a given point is from the shape
         * @param point Vector3 indicating point to calculate distance from
         * @return float indicating distance point is from shape
         */
        virtual float DistanceFromPoint(const AZ::Vector3& point)
        {
            return sqrtf(DistanceSquaredFromPoint(point));
        }

        /**
         * @brief Returns the min squared distance a given point is from the shape
         * @param point Vector3 indicating point to calculate square distance from
         * @return float indicating square distance point is from shape
         */
        virtual float DistanceSquaredFromPoint(const AZ::Vector3& point) = 0;

        /**
         * @brief Returns a random position inside the volume.
         * @param randomDistribution An enum representing the different random distributions to use.
         */
        virtual AZ::Vector3 GenerateRandomPointInside(AZ::RandomDistributionType /*randomDistribution*/)
        {
            AZ_Warning("ShapeComponentRequests", false, "GenerateRandomPointInside not implemented");
            return AZ::Vector3::CreateZero();
        }

        /**
         * @brief Returns if a ray is intersecting the shape.
         */
        virtual bool IntersectRay(const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, AZ::VectorFloat& /*distance*/)
        {
            AZ_Warning("ShapeComponentRequests", false, "IntersectRay not implemented");
            return false;
        }

        virtual ~ShapeComponentRequests() = default;
    };

    // Bus to service the Shape component requests event group
    using ShapeComponentRequestsBus = AZ::EBus<ShapeComponentRequests>;

    /**
     * Notifications sent by the shape component.
     */
    class ShapeComponentNotifications : public AZ::ComponentBus
    {
    public:
        enum class ShapeChangeReasons
        {
            TransformChanged,
            ShapeChanged
        };

        /**
         * @brief Informs listeners that the shape component has been updated (The shape was modified)
         * @param changeReason Informs listeners of the reason for this shape change (transform change, the shape dimensions being altered)
         */
        virtual void OnShapeChanged(ShapeChangeReasons changeReason) = 0;
    };

    // Bus to service Shape component notifications event group
    using ShapeComponentNotificationsBus = AZ::EBus<ShapeComponentNotifications>;
} // namespace LmbrCentral