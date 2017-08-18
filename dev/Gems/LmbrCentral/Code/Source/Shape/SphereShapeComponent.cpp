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
#include "SphereShapeComponent.h"
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Cry_GeoOverlap.h>
#include <Cry_GeoDistance.h>


namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void SphereShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        SphereShapeConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: SphereColliderComponent -> SphereShapeComponent
            serializeContext->ClassDeprecate(
                "SphereColliderComponent",
                "{99F33E4A-4EFB-403C-8918-9171D47A03A4}",
                &ClassConverters::DeprecateSphereColliderComponent
                );

            serializeContext->Class<SphereShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &SphereShapeComponent::m_configuration)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<SphereShapeComponentRequestsBus>("SphereShapeComponentRequestsBus")
                ->Event("GetSphereConfiguration", &SphereShapeComponentRequestsBus::Events::GetSphereConfiguration)
                ->Event("SetRadius", &SphereShapeComponentRequestsBus::Events::SetRadius)
                ;
    }
    }

    void SphereShapeComponent::Activate()
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(m_currentTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        m_intersectionDataCache.SetCacheStatus(SphereShapeComponent::SphereIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        SphereShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void SphereShapeComponent::Deactivate()
    {
        SphereShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void SphereShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.SetCacheStatus(SphereShapeComponent::SphereIntersectionDataCache::CacheStatus::Obsolete_TransformChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    //////////////////////////////////////////////////////////////////////////
    void SphereShapeComponent::SetRadius(float newRadius)
    {
        m_configuration.SetRadius(newRadius);
        m_intersectionDataCache.SetCacheStatus(SphereShapeComponent::SphereIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }


    //////////////////////////////////////////////////////////////////////////

    AZ::Aabb SphereShapeComponent::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_configuration);
        return AZ::Aabb::CreateCenterRadius(m_currentTransform.GetPosition(), m_configuration.GetRadius());
    }

    bool SphereShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_configuration);
        return Overlap::Point_Sphere(m_intersectionDataCache.GetCenterPosition(), m_intersectionDataCache.GetRadiusSquared(), point);
    }

    float SphereShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_configuration);
        const AZ::Vector3 pointToSphereCenter = m_intersectionDataCache.GetCenterPosition() - point;
        float distance = pointToSphereCenter.GetLength() - m_configuration.GetRadius();
        return sqr(AZStd::max(distance, 0.f));
    }

    //////////////////////////////////////////////////////////////////////////
    void SphereShapeComponent::SphereIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform,
        const SphereShapeConfiguration& configuration)
    {
        if (m_cacheStatus > CacheStatus::Current)
        {
            m_currentCenterPosition = currentTransform.GetPosition();

            if (m_cacheStatus == CacheStatus::Obsolete_ShapeChange)
            {
                m_radiusSquared = configuration.GetRadius() * configuration.GetRadius();
                SetCacheStatus(CacheStatus::Current);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    namespace ClassConverters
    {
        static bool DeprecateSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="SphereColliderComponent" version="1" type="{99F33E4A-4EFB-403C-8918-9171D47A03A4}">
             <Class name="SphereColliderConfiguration" field="Configuration" version="1" type="{0319AE62-3355-4C98-873D-3139D0427A53}">
              <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="SphereShapeComponent" version="1" type="{E24CBFF0-2531-4F8D-A8AB-47AF4D54BCD2}">
             <Class name="SphereShapeConfiguration" field="Configuration" version="1" type="{4AADFD75-48A7-4F31-8F30-FE4505F09E35}">
              <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            SphereShapeConfiguration configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<SphereShapeConfiguration>(configuration);
            }

            // Convert to SphereShapeComponent
            bool result = classElement.Convert(context, "{E24CBFF0-2531-4F8D-A8AB-47AF4D54BCD2}");
            if (result)
            {
                configIndex = classElement.AddElement<SphereShapeConfiguration>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<SphereShapeConfiguration>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
