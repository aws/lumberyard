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
#include <Cry_GeoOverlap.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void CylinderShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        CylinderShapeConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate(
                "CylinderColliderComponent",
                "{A43F684B-07B6-4CD7-8D59-643709DF9486}",
                &ClassConverters::DeprecateCylinderColliderComponent
                );

            serializeContext->Class<CylinderShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &CylinderShapeComponent::m_configuration)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<CylinderShapeComponentRequestsBus>("CylinderShapeComponentRequestsBus")
                ->Event("GetCylinderConfiguration", &CylinderShapeComponentRequestsBus::Events::GetCylinderConfiguration)
                ->Event("SetHeight", &CylinderShapeComponentRequestsBus::Events::SetHeight)
                ->Event("SetRadius", &CylinderShapeComponentRequestsBus::Events::SetRadius)
                ;
        }
    }

    void CylinderShapeComponent::Activate()
    {
        AZ::Transform currentEntityTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(currentEntityTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        m_currentWorldTransform = currentEntityTransform;
        m_intersectionDataCache.SetCacheStatus(CylinderShapeComponent::CylinderIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        CylinderShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void CylinderShapeComponent::Deactivate()
    {
        CylinderShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void CylinderShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentWorldTransform = world;
        m_intersectionDataCache.SetCacheStatus(CylinderShapeComponent::CylinderIntersectionDataCache::CacheStatus::Obsolete_TransformChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    //////////////////////////////////////////////////////////////////////////

    void CylinderShapeComponent::SetHeight(float newHeight)
    {
        m_configuration.SetHeight(newHeight);
        m_intersectionDataCache.SetCacheStatus(CylinderShapeComponent::CylinderIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void CylinderShapeComponent::SetRadius(float newRadius)
    {
        m_configuration.SetRadius(newRadius);
        m_intersectionDataCache.SetCacheStatus(CylinderShapeComponent::CylinderIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    //////////////////////////////////////////////////////////////////////////

    AZ::Aabb CylinderShapeComponent::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, m_configuration);
        AZ::Vector3 axisUnitVector = m_currentWorldTransform.GetBasisZ();
        AZ::Vector3 lowerAABBCenter = m_intersectionDataCache.m_baseCenterPoint + axisUnitVector * m_configuration.GetRadius();
        AZ::Aabb baseAabb(AZ::Aabb::CreateCenterRadius(lowerAABBCenter, m_configuration.GetRadius()));

        AZ::Vector3 topAABBCenter = m_intersectionDataCache.m_baseCenterPoint + axisUnitVector * (m_configuration.GetHeight() - m_configuration.GetRadius());
        AZ::Aabb topAabb(AZ::Aabb::CreateCenterRadius(topAABBCenter, m_configuration.GetRadius()));

        baseAabb.AddAabb(topAabb);
        return baseAabb;
    }


    bool CylinderShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, m_configuration);

        return Overlap::Point_Cylinder(
            m_intersectionDataCache.m_baseCenterPoint,
            m_intersectionDataCache.m_axisVector,
            m_intersectionDataCache.m_axisLengthSquared,
            m_intersectionDataCache.m_radiusSquared,
            point
            );
    }

    float CylinderShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentWorldTransform, m_configuration);

        float distanceSquared = Distance::Point_CylinderSq(
            point,
            m_intersectionDataCache.m_baseCenterPoint,
            m_intersectionDataCache.m_baseCenterPoint + m_intersectionDataCache.m_axisVector,
            m_configuration.GetRadius()
            );
        return distanceSquared;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    void CylinderShapeComponent::CylinderIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform, const CylinderShapeConfiguration& configuration)
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


    //////////////////////////////////////////////////////////////////////////
    namespace ClassConverters
    {
        static bool DeprecateCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CylinderColliderComponent" version="1" type="{A43F684B-07B6-4CD7-8D59-643709DF9486}">
             <Class name="CylinderColliderConfiguration" field="Configuration" version="1" type="{E1DCB833-EFC4-43AC-97B0-4E07AA0DFAD9}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="CylinderShapeComponent" version="1" type="{B0C6AA97-E754-4E33-8D32-33E267DB622F}">
             <Class name="CylinderShapeConfiguration" field="Configuration" version="1" type="{53254779-82F1-441E-9116-81E1FACFECF4}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            CylinderShapeConfiguration configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CylinderShapeConfiguration>(configuration);
            }

            // Convert to CylinderShapeComponent
            bool result = classElement.Convert(context, "{B0C6AA97-E754-4E33-8D32-33E267DB622F}");
            if (result)
            {
                configIndex = classElement.AddElement<CylinderShapeConfiguration>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CylinderShapeConfiguration>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
