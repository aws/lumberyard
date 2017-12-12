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

#include "StdAfx.h"
#include "CylinderShapeComponent.h"
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>

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
