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
#include "CapsuleShapeComponent.h"
#include <Cry_GeoOverlap.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void CapsuleShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        CapsuleShapeConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: CapsuleColliderComponent -> CapsuleShapeComponent
            serializeContext->ClassDeprecate(
                "CapsuleColliderComponent",
                "{D1F746A9-FC24-48E4-88DE-5B3122CB6DE7}",
                &ClassConverters::DeprecateCapsuleColliderComponent
                );

            serializeContext->Class<CapsuleShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &CapsuleShapeComponent::m_configuration)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<CapsuleShapeComponentRequestsBus>("CapsuleShapeComponentRequestsBus")
                ->Event("GetCapsuleConfiguration", &CapsuleShapeComponentRequestsBus::Events::GetCapsuleConfiguration)
                ->Event("SetHeight", &CapsuleShapeComponentRequestsBus::Events::SetHeight)
                ->Event("SetRadius", &CapsuleShapeComponentRequestsBus::Events::SetRadius)
                ;
        }

    }

    void CapsuleShapeComponent::Activate()
    {
        AZ::Transform currentEntityTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(currentEntityTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        m_currentWorldTransform = currentEntityTransform;

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        CapsuleShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void CapsuleShapeComponent::Deactivate()
    {
        CapsuleShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void CapsuleShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        m_intersectionDataCache.SetCacheStatus(CapsuleShapeComponent::CapsuleIntersectionDataCache::CacheStatus::Obsolete_TransformChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    //////////////////////////////////////////////////////////////////////////
    void CapsuleShapeComponent::SetHeight(float newHeight)
    {
        m_configuration.SetHeight(newHeight);
        m_intersectionDataCache.SetCacheStatus(CapsuleShapeComponent::CapsuleIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void CapsuleShapeComponent::SetRadius(float newRadius)
    {
        m_configuration.SetRadius(newRadius);
        m_intersectionDataCache.SetCacheStatus(CapsuleShapeComponent::CapsuleIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    //////////////////////////////////////////////////////////////////////////

    AZ::Aabb CapsuleShapeComponent::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, m_configuration);
        AZ::Aabb topAabb(AZ::Aabb::CreateCenterRadius(m_intersectionDataCache.m_topPlaneCenterPoint, m_configuration.GetRadius()));
        AZ::Aabb baseAabb(AZ::Aabb::CreateCenterRadius(m_intersectionDataCache.m_basePlaneCenterPoint, m_configuration.GetRadius()));
        baseAabb.AddAabb(topAabb);
        return baseAabb;
    }


    bool CapsuleShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, m_configuration);

        AZ::Vector3 currentPosition = m_currentWorldTransform.GetPosition();

        // Check Bottom sphere
        if (Overlap::Point_Sphere(m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_radiusSquared, point))
        {
            return true;
        }

        // If the capsule is infact just a sphere then just stop (because the height of the cylinder <= 2 * radius of the cylinder)
        if (m_intersectionDataCache.m_isSphere)
        {
            return false;
        }

        // Check Top sphere
        if (Overlap::Point_Sphere(m_intersectionDataCache.m_topPlaneCenterPoint, m_intersectionDataCache.m_radiusSquared, point))
        {
            return true;
        }

        // If its not in either sphere check the cylinder
        return Overlap::Point_Cylinder(m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_axisVector,
            m_intersectionDataCache.m_axisLengthSquared, m_intersectionDataCache.m_radiusSquared, point);
    }

    float CapsuleShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, m_configuration);

        Lineseg lineSeg(
            AZVec3ToLYVec3(m_intersectionDataCache.m_basePlaneCenterPoint),
            AZVec3ToLYVec3(m_intersectionDataCache.m_topPlaneCenterPoint)
            );

        float tValue = 0.f;
        float distance = Distance::Point_Lineseg(AZVec3ToLYVec3(point), lineSeg, tValue);
        distance -= m_configuration.GetRadius();
        return sqr(AZStd::max(distance, 0.f));
    }

    /////////////////////////////////////////////////////////////////////////////////////////////

    void CapsuleShapeComponent::CapsuleIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform, const CapsuleShapeConfiguration& configuration)
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


    //////////////////////////////////////////////////////////////////////////
    namespace ClassConverters
    {
        static bool DeprecateCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CapsuleColliderComponent" version="1" type="{D1F746A9-FC24-48E4-88DE-5B3122CB6DE7}">
             <Class name="CapsuleColliderConfiguration" field="Configuration" version="1" type="{902BCDA9-C9E5-429C-991B-74C241ED2889}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="CapsuleShapeComponent" version="1" type="{967EC13D-364D-4696-AB5C-C00CC05A2305}">
             <Class name="CapsuleShapeConfiguration" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            CapsuleShapeConfiguration configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CapsuleShapeConfiguration>(configuration);
            }

            // Convert to CapsuleShapeComponent
            bool result = classElement.Convert(context, "{967EC13D-364D-4696-AB5C-C00CC05A2305}");
            if (result)
            {
                configIndex = classElement.AddElement<CapsuleShapeConfiguration>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CapsuleShapeConfiguration>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
