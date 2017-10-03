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
#include "BoxShapeComponent.h"
#include <Cry_GeoOverlap.h>
#include <MathConversion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace LmbrCentral
{
    void BoxShape::Activate(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.SetCacheStatus(BoxIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        BoxShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void BoxShape::Deactivate()
    {
        BoxShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void BoxShape::InvalidateCache(BoxIntersectionDataCache::CacheStatus reason)
    {
        m_intersectionDataCache.SetCacheStatus(reason);
    }

    void BoxShape::SetBoxDimensions(AZ::Vector3 newDimensions)
    {        
        GetConfiguration().SetDimensions(newDimensions);
        m_intersectionDataCache.SetCacheStatus(BoxShapeComponent::BoxIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }    

    AZ::Aabb BoxShape::GetEncompassingAabb()
    {   
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, GetConfiguration());
        if (m_intersectionDataCache.IsAxisAligned())
        {
            return LyAABBToAZAabb(m_intersectionDataCache.GetAABB());
        }
        else
        {
            AZ::Vector3 points[8] =
            {
                AZ::Vector3(-1,-1, 1),
                AZ::Vector3( 1,-1, 1),
                AZ::Vector3(-1, 1, 1),
                AZ::Vector3( 1, 1, 1),
                AZ::Vector3(-1,-1,-1),
                AZ::Vector3( 1,-1,-1),
                AZ::Vector3(-1, 1,-1),
                AZ::Vector3( 1, 1,-1)
            };

            OBB currentOBB = m_intersectionDataCache.GetOBB();
            AZ::Vector3 halfLength = LYVec3ToAZVec3(currentOBB.h);

            for (int i = 0; i < 8; i++)
            {
                AZ::Vector3 unrotatedPosition(halfLength.GetX() * points[i].GetX(),
                    halfLength.GetY() * points[i].GetY(),
                    halfLength.GetZ() * points[i].GetZ());
                AZ::Vector3 rotatedPosition = unrotatedPosition * LyMatrix3x3ToAzMatrix3x3(currentOBB.m33);
                AZ::Vector3 finalPosition = rotatedPosition + m_intersectionDataCache.GetCurrentPosition();
                points[i] = finalPosition;
            }
            
            AZ::Aabb encompassingAabb(AZ::Aabb::CreatePoints(points, 8));
            return encompassingAabb;
        }
    }

    bool BoxShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, GetConfiguration());

        bool result = false;
        if (m_intersectionDataCache.IsAxisAligned())
        {
            result = Overlap::Point_AABB(AZVec3ToLYVec3(point), m_intersectionDataCache.GetAABB());
        }
        else
        {
            result = Overlap::Point_OBB(AZVec3ToLYVec3(point), AZVec3ToLYVec3(m_intersectionDataCache.GetCurrentPosition()), m_intersectionDataCache.GetOBB());
        }

        return result;
    }

    float BoxShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, GetConfiguration());

        float distanceSquared = 0.f;
        if (m_intersectionDataCache.IsAxisAligned())
        {
            distanceSquared = Distance::Point_AABBSq(AZVec3ToLYVec3(point), m_intersectionDataCache.GetAABB());
        }
        else
        {
            // The cached OBB has center point at Zero (object space).
            // create a copy and set the center point from the transform before calculating distance.
            OBB obb(m_intersectionDataCache.GetOBB());
            obb.c = AZVec3ToLYVec3(m_currentTransform.GetPosition());
            distanceSquared = Distance::Point_OBBSq(AZVec3ToLYVec3(point), obb);
        }
        return distanceSquared;
    }
    
    void BoxShape::BoxIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform,
        const BoxShapeConfig& configuration)
    {
        if (m_cacheStatus != CacheStatus::Current)
        {
            m_currentPosition = currentTransform.GetPosition();
            AZ::Quaternion quaternionForTransform = AZ::Quaternion::CreateFromTransform(currentTransform);
            if(quaternionForTransform.IsClose(AZ::Quaternion::CreateIdentity()))
            {
                AZ::Vector3 minPos = configuration.GetDimensions() * -0.5;
                minPos = currentTransform * minPos;

                AZ::Vector3 maxPos = configuration.GetDimensions() * 0.5;
                maxPos = currentTransform * maxPos;

                Vec3 boxMin = AZVec3ToLYVec3(minPos);
                Vec3 boxMax = AZVec3ToLYVec3(maxPos);
                m_aabb = AABB(boxMin, boxMax);
                m_isAxisAligned = true;
            }
            else
            {
                AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateFromTransform(currentTransform);
                AZ::Vector3 halfLengthVector = configuration.GetDimensions() * 0.5;
                m_obb = OBB::CreateOBB(AZMatrix3x3ToLYMatrix3x3(rotationMatrix), AZVec3ToLYVec3(halfLengthVector), Vec3(0));
                m_isAxisAligned = false;
            }

            SetCacheStatus(CacheStatus::Current);
        }
    }

    void BoxShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.SetCacheStatus(BoxShapeComponent::BoxIntersectionDataCache::CacheStatus::Obsolete_TransformChange);
        ShapeComponentNotificationsBus::Event(m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

} // namespace LmbrCentral
