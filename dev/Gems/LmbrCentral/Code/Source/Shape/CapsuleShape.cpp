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
#include "CapsuleShapeComponent.h"
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>

namespace LmbrCentral
{
    void CapsuleShape::Activate(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;
        AZ::TransformBus::EventResult(m_currentWorldTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        CapsuleShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        m_intersectionDataCache.SetCacheStatus(CapsuleIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }

    void CapsuleShape::Deactivate()
    {
        CapsuleShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void CapsuleShape::InvalidateCache(CapsuleIntersectionDataCache::CacheStatus reason)
    {
        m_intersectionDataCache.SetCacheStatus(reason);
    }

    void CapsuleShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        m_intersectionDataCache.SetCacheStatus(CapsuleShapeComponent::CapsuleIntersectionDataCache::CacheStatus::Obsolete_TransformChange);    
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);        
    }
    
    void CapsuleShape::SetHeight(float newHeight)
    {
        GetConfiguration().SetHeight(newHeight);
        m_intersectionDataCache.SetCacheStatus(CapsuleShapeComponent::CapsuleIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);        
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void CapsuleShape::SetRadius(float newRadius)
    {
        GetConfiguration().SetRadius(newRadius);
        m_intersectionDataCache.SetCacheStatus(CapsuleShapeComponent::CapsuleIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }    

    AZ::Aabb CapsuleShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, GetConfiguration());
        AZ::Aabb topAabb(AZ::Aabb::CreateCenterRadius(m_intersectionDataCache.m_topPlaneCenterPoint, GetConfiguration().GetRadius()));
        AZ::Aabb baseAabb(AZ::Aabb::CreateCenterRadius(m_intersectionDataCache.m_basePlaneCenterPoint, GetConfiguration().GetRadius()));
        baseAabb.AddAabb(topAabb);
        return baseAabb;
    }

    bool CapsuleShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, GetConfiguration());

        AZ::Vector3 currentPosition = m_currentWorldTransform.GetPosition();

        // Check Bottom sphere
        if (AZ::Intersect::PointSphere(m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_radiusSquared, point))
        {
            return true;
        }

        // If the capsule is infact just a sphere then just stop (because the height of the cylinder <= 2 * radius of the cylinder)
        if (m_intersectionDataCache.m_isSphere)
        {
            return false;
        }

        // Check Top sphere
        if (AZ::Intersect::PointSphere(m_intersectionDataCache.m_topPlaneCenterPoint, m_intersectionDataCache.m_radiusSquared, point))
        {
            return true;
        }

        // If its not in either sphere check the cylinder
        return AZ::Intersect::PointCylinder(m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_axisVector,
            m_intersectionDataCache.m_axisLengthSquared, m_intersectionDataCache.m_radiusSquared, point);
    }

    float CapsuleShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, GetConfiguration());

        Lineseg lineSeg(
            AZVec3ToLYVec3(m_intersectionDataCache.m_basePlaneCenterPoint),
            AZVec3ToLYVec3(m_intersectionDataCache.m_topPlaneCenterPoint)
            );

        float tValue = 0.f;
        float distance = Distance::Point_Lineseg(AZVec3ToLYVec3(point), lineSeg, tValue);
        distance -= GetConfiguration().GetRadius();
        return sqr(AZStd::max(distance, 0.f));
    }    

    void CapsuleShape::CapsuleIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform, const CapsuleShapeConfig& configuration)
    {
        if (m_cacheStatus > CacheStatus::Current)
        {
            m_axisVector = currentTransform.GetBasisZ();

            float internalCylinderHeight = configuration.GetHeight() - 2 * configuration.GetRadius();

            if (internalCylinderHeight > std::numeric_limits<float>::epsilon())
            {
                AZ::Vector3 CurrentPositionToPlanesVector = m_axisVector * (internalCylinderHeight / 2);
                m_topPlaneCenterPoint = currentTransform.GetPosition() + CurrentPositionToPlanesVector;
                m_basePlaneCenterPoint = currentTransform.GetPosition() - CurrentPositionToPlanesVector;
                m_axisVector = m_axisVector * internalCylinderHeight;
                m_isSphere = false;
            }
            else
            {
                m_basePlaneCenterPoint = currentTransform.GetPosition();
                m_isSphere = true;
            }

            if (m_cacheStatus == CacheStatus::Obsolete_ShapeChange)
            {
                m_radiusSquared = pow(configuration.GetRadius(), 2);
                m_axisLengthSquared = pow(internalCylinderHeight, 2);
            }

            SetCacheStatus(CacheStatus::Current);
        }
    }

} // namespace LmbrCentral
