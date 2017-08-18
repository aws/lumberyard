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
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include "Cry_Vector3.h"

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void BoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        BoxShapeConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: BoxColliderComponent -> BoxShapeComponent
            serializeContext->ClassDeprecate(
                "BoxColliderComponent",
                "{C215EB2A-1803-4EDC-B032-F7C92C142337}",
                &ClassConverters::DeprecateBoxColliderComponent
                );

            serializeContext->Class<BoxShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &BoxShapeComponent::m_configuration)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<BoxShapeComponentRequestsBus>("BoxShapeComponentRequestsBus")
                ->Event("GetBoxConfiguration", &BoxShapeComponentRequestsBus::Events::GetBoxConfiguration)
                ->Event("SetBoxDimensions", &BoxShapeComponentRequestsBus::Events::SetBoxDimensions)
                ;
        }
    }

    void BoxShapeComponent::Activate()
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(m_currentTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        m_intersectionDataCache.SetCacheStatus(BoxShapeComponent::BoxIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        BoxShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void BoxShapeComponent::Deactivate()
    {
        BoxShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void BoxShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.SetCacheStatus(BoxShapeComponent::BoxIntersectionDataCache::CacheStatus::Obsolete_TransformChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    //////////////////////////////////////////////////////////////////////////

    void BoxShapeComponent::SetBoxDimensions(AZ::Vector3 newDimensions)
    {
        m_configuration.SetDimensions(newDimensions);
        m_intersectionDataCache.SetCacheStatus(BoxShapeComponent::BoxIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }
    
    //////////////////////////////////////////////////////////////////////////

    AZ::Aabb BoxShapeComponent::GetEncompassingAabb()
    {   
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_configuration);
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

    bool BoxShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_configuration);

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

    float BoxShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_configuration);

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

    //////////////////////////////////////////////////////////////////////////
    void BoxShapeComponent::BoxIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform,
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


    //////////////////////////////////////////////////////////////////////////
    namespace ClassConverters
    {
        static bool DeprecateBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="BoxColliderComponent" version="1" type="{C215EB2A-1803-4EDC-B032-F7C92C142337}">
             <Class name="BoxColliderConfiguration" field="Configuration" version="1" type="{282E47CB-9F6D-47AE-A210-4CE879527EFD}">
              <Class name="Vector3" field="Size" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
             </Class>
            </Class>

            New:
            <Class name="BoxShapeComponent" version="1" type="{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}">
             <Class name="BoxShapeConfiguration" field="Configuration" version="1" type="{F034FBA2-AC2F-4E66-8152-14DFB90D6283}">
              <Class name="Vector3" field="Dimensions" value="1.0000000 2.0000000 3.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            BoxShapeConfiguration configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<BoxShapeConfiguration>(configuration);
            }

            // Convert to BoxShapeComponent
            bool result = classElement.Convert(context, "{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}");
            if (result)
            {
                configIndex = classElement.AddElement<BoxShapeConfiguration>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<BoxShapeConfiguration>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
