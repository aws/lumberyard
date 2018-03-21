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
#include "BoxShapeComponent.h"
#include <Cry_GeoOverlap.h>
#include <MathConversion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/algorithm.h>
#include "Cry_Vector3.h"

#include <LmbrCentral/Scripting/RandomTimedSpawnerComponentBus.h>

#include <random>

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
            /* 
                We use the Point_AABB_MaxInclusive function here because, when working with a box
                shape with the dimensions (5, 5, 5) we want a point at, say, (1, 5, 1) to be a valid
                point. If we use Point_AABB (which is max exclusive), then the point (1, 5, 1) would fail
                because the Y component is at the max bound of the box. 
                
                When dealing with randomly generated points that might end up at the max value
                we want to be max inclusive. However other parts of the engine may be relying on 
                the max exclusive property of Point_AABB so that method has been left alone.
            */

            result = Overlap::Point_AABB_MaxInclusive(AZVec3ToLYVec3(point), m_intersectionDataCache.GetAABB());
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
    
    AZ::Vector3 BoxShape::GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution)
    {
        float x = 0;
        float y = 0;
        float z = 0;
        AZ::Vector3 minPos = GetConfiguration().GetDimensions() * -0.5f;
        AZ::Vector3 maxPos = GetConfiguration().GetDimensions() * +0.5f;

        std::default_random_engine generator;
        generator.seed(static_cast<unsigned int>(time(0)));

        switch(randomDistribution)
        {
        case AZ::RandomDistributionType::Normal:
        {
            // Points should be generated just inside the shape boundary
            minPos *= 0.999f;
            maxPos *= 0.999f;

            std::normal_distribution<float> normalDist;
            float mean = 0.0f; //Mean will always be 0
            float stdDev = 0.0f; //stdDev will be the sqrt of the max value (which is the total variation)

            stdDev = sqrtf(maxPos.GetX());
            normalDist = std::normal_distribution<float>(mean, stdDev);
            x = normalDist(generator);
            //Normal distributions can sometimes produce values outside of our desired range
            //We just need to clamp
            x = AZStd::clamp<float>(x, minPos.GetX(), maxPos.GetX());

            stdDev = sqrtf(maxPos.GetY());
            normalDist = std::normal_distribution<float>(mean, stdDev);
            y = normalDist(generator);

            y = AZStd::clamp<float>(y, minPos.GetY(), maxPos.GetY());

            stdDev = sqrtf(maxPos.GetZ());
            normalDist = std::normal_distribution<float>(mean, stdDev);
            z = normalDist(generator);

            z = AZStd::clamp<float>(z, minPos.GetZ(), maxPos.GetZ());

            break;
        }
        case AZ::RandomDistributionType::UniformReal:
        {
            std::uniform_real_distribution<float> uniformRealDist;

            uniformRealDist = std::uniform_real_distribution<float>(minPos.GetX(), maxPos.GetX());
            x = uniformRealDist(generator);

            uniformRealDist = std::uniform_real_distribution<float>(minPos.GetY(), maxPos.GetY());
            y = uniformRealDist(generator);

            uniformRealDist = std::uniform_real_distribution<float>(minPos.GetZ(), maxPos.GetZ());
            z = uniformRealDist(generator);

            break;
        }
        default:
            AZ_Warning("BoxShape", false, "Unsupported random distribution type. Returning default vector (0,0,0)");
        }

        //Transform into world space
        AZ::Vector3 position = AZ::Vector3(x, y, z);

        position = m_currentTransform * position;

        return position;
    }

    void BoxShape::BoxIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform,
        const BoxShapeConfiguration& configuration)
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
