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
#include "SphereShapeComponent.h"
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/IntersectPoint.h>
#include <Cry_GeoDistance.h>


namespace LmbrCentral
{
    void SphereShape::Activate(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        SphereShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        m_intersectionDataCache.SetCacheStatus(SphereIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }

    void SphereShape::Deactivate()
    {
        SphereShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void SphereShape::InvalidateCache(SphereIntersectionDataCache::CacheStatus reason)
    {
        m_intersectionDataCache.SetCacheStatus(reason);
    }

    void SphereShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.SetCacheStatus(SphereIntersectionDataCache::CacheStatus::Obsolete_TransformChange);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    void SphereShape::SetRadius(float newRadius)
    {
        GetConfiguration().SetRadius(newRadius);
        m_intersectionDataCache.SetCacheStatus(SphereIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);        
    }

    AZ::Aabb SphereShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, GetConfiguration());
        return AZ::Aabb::CreateCenterRadius(m_currentTransform.GetPosition(), GetConfiguration().GetRadius());
    }

    bool SphereShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, GetConfiguration());
        return AZ::Intersect::PointSphere(m_intersectionDataCache.GetCenterPosition(), m_intersectionDataCache.GetRadiusSquared(), point);
    }

    float SphereShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, GetConfiguration());
        const AZ::Vector3 pointToSphereCenter = m_intersectionDataCache.GetCenterPosition() - point;
        float distance = pointToSphereCenter.GetLength() - GetConfiguration().GetRadius();
        return sqr(AZStd::max(distance, 0.f));
    }

    void SphereShape::SphereIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform,
        const SphereShapeConfig& configuration)
    {
        if (m_cacheStatus > CacheStatus::Current)
        {
            m_currentCenterPosition = currentTransform.GetPosition();

            if (m_cacheStatus == CacheStatus::Obsolete_ShapeChange)
            {
                m_radiusSquared = configuration.GetRadius() * configuration.GetRadius();
                m_cacheStatus = CacheStatus::Current;
            }
        }
    }

} // namespace LmbrCentral
