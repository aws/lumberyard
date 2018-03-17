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

#include "LmbrCentral_precompiled.h"
#include "CylinderShapeComponent.h"
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Random.h>

#include <LmbrCentral/Scripting/RandomTimedSpawnerComponentBus.h>
#include <random>

namespace LmbrCentral
{   
    void CylinderShape::InvalidateCache(CylinderIntersectionDataCache::CacheStatus reason)
    {
        m_intersectionDataCache.SetCacheStatus(reason);
    }

    void CylinderShape::Activate(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;
                
        AZ::TransformBus::EventResult(m_currentWorldTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.SetCacheStatus(CylinderShape::CylinderIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        CylinderShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void CylinderShape::Deactivate()
    {
        CylinderShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void CylinderShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        m_intersectionDataCache.SetCacheStatus(CylinderShapeComponent::CylinderIntersectionDataCache::CacheStatus::Obsolete_TransformChange);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);        
    }

    void CylinderShape::SetHeight(float newHeight)
    {
        GetConfiguration().SetHeight(newHeight);
        m_intersectionDataCache.SetCacheStatus(CylinderShapeComponent::CylinderIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);        
    }

    void CylinderShape::SetRadius(float newRadius)
    {
        GetConfiguration().SetRadius(newRadius);
        m_intersectionDataCache.SetCacheStatus(CylinderShapeComponent::CylinderIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    AZ::Aabb CylinderShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, GetConfiguration());
        AZ::Vector3 axisUnitVector = m_currentWorldTransform.GetBasisZ();
        AZ::Vector3 lowerAABBCenter = m_intersectionDataCache.m_baseCenterPoint + axisUnitVector * GetConfiguration().GetRadius();
        AZ::Aabb baseAabb(AZ::Aabb::CreateCenterRadius(lowerAABBCenter, GetConfiguration().GetRadius()));

        AZ::Vector3 topAABBCenter = m_intersectionDataCache.m_baseCenterPoint + axisUnitVector * (GetConfiguration().GetHeight() - GetConfiguration().GetRadius());
        AZ::Aabb topAabb(AZ::Aabb::CreateCenterRadius(topAABBCenter, GetConfiguration().GetRadius()));

        baseAabb.AddAabb(topAabb);
        return baseAabb;
    }

    AZ::Vector3 CylinderShape::GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution)
    {
        float halfHeight = GetConfiguration().GetHeight() * 0.5f;
        float maxRadius = GetConfiguration().GetRadius();
        const float minAngle = 0.0f;
        const float maxAngle = AZ::Constants::TwoPi;

        std::default_random_engine generator;
        generator.seed(static_cast<unsigned int>(time(0)));

        float randomRadius = 0.0f;
        float randomZ = 0.0f;
        float randomAngle = 0.0f;

        switch (randomDistribution)
        {
        case AZ::RandomDistributionType::Normal:
        {
            // Points should be generated just inside the shape boundary
            halfHeight *= 0.999f;
            maxRadius *= 0.999f;

            std::normal_distribution<float> normalDist;
            float meanRadius = 0.0f; //Mean for the radius should be 0. Negative radius is still valid
            float meanZ = 0.0f; //We want the average height of generated points to be between the min height and the max height
            float meanAngle = 0.0f; //There really isn't a good mean angle
            float stdDevRadius = sqrtf(maxRadius); //StdDev of the radius will be the sqrt of the radius (the radius is the total variation)
            float stdDevZ = sqrtf(halfHeight); //Same principle applied to the stdDev of the height
            float stdDevAngle = sqrtf(maxAngle); //And the angle as well

            //Generate a random radius 
            normalDist = std::normal_distribution<float>(meanRadius, stdDevRadius);
            randomRadius = normalDist(generator);
            //Normal distributions can produce values higher than the desired max 
            //This is very unlikely but we clamp anyway
            randomRadius = AZStd::clamp(randomRadius, -maxRadius, maxRadius);

            //Generate a random height
            normalDist = std::normal_distribution<float>(meanZ, stdDevZ);
            randomZ = normalDist(generator);
            randomZ = AZStd::clamp(randomZ, -halfHeight, halfHeight);

            //Generate a random angle along the circle
            normalDist = std::normal_distribution<float>(meanAngle, stdDevAngle);
            randomAngle = normalDist(generator);
            //Don't bother to clamp the angle because it doesn't matter if the angle is above 360 deg or below 0 deg

            break;
        }
        case AZ::RandomDistributionType::UniformReal:
        {

            std::uniform_real_distribution<float> uniformRealDist;

            uniformRealDist = std::uniform_real_distribution<float>(-maxRadius, maxRadius);
            randomRadius = uniformRealDist(generator);

            uniformRealDist = std::uniform_real_distribution<float>(-halfHeight, halfHeight);
            randomZ = uniformRealDist(generator);

            uniformRealDist = std::uniform_real_distribution<float>(minAngle, maxAngle);
            randomAngle = uniformRealDist(generator);

            break;
        }
        default:
            AZ_Warning("CylinderShape", false, "Unsupported random distribution type. Returning default vector (0,0,0)");
        }

        float x = randomRadius * cosf(randomAngle);
        float y = randomRadius * sinf(randomAngle);
        float z = randomZ;

        //Transform into world space
        AZ::Vector3 position = AZ::Vector3(x, y, z);

        position = m_currentWorldTransform * position;

        return position;
    }

    bool CylinderShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, GetConfiguration());

        return AZ::Intersect::PointCylinder(
            m_intersectionDataCache.m_baseCenterPoint,
            m_intersectionDataCache.m_axisVector,
            m_intersectionDataCache.m_axisLengthSquared,
            m_intersectionDataCache.m_radiusSquared,
            point
            );
    }

    float CylinderShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, GetConfiguration());

        float distanceSquared = Distance::Point_CylinderSq(
            point,
            m_intersectionDataCache.m_baseCenterPoint,
            m_intersectionDataCache.m_baseCenterPoint + m_intersectionDataCache.m_axisVector,
            GetConfiguration().GetRadius()
            );
        return distanceSquared;
    }    

    void CylinderShape::CylinderIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform, const CylinderShapeConfig& configuration)
    {
        if (m_cacheStatus > CacheStatus::Current)
        {
            m_axisVector = currentTransform.GetBasisZ();

            AZ::Vector3 CurrentPositionToBaseVector = -1 * m_axisVector * (configuration.GetHeight() / 2);
            m_baseCenterPoint = currentTransform.GetPosition() + CurrentPositionToBaseVector;

            m_axisVector = m_axisVector * configuration.GetHeight();

            if (m_cacheStatus == CacheStatus::Obsolete_ShapeChange)
            {
                m_radiusSquared = pow(configuration.GetRadius(), 2);
                m_axisLengthSquared = pow(configuration.GetHeight(), 2);
            }

            SetCacheStatus(CacheStatus::Current);
        }
    }
} // namespace LmbrCentral
