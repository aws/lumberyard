/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensor's.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "SurfaceData_precompiled.h"
#include "SurfaceDataShapeComponent.h"

#include <AZCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

namespace SurfaceData
{
    void SurfaceDataShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataShapeConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ProviderTags", &SurfaceDataShapeConfig::m_providerTags)
                ->Field("ModifierTags", &SurfaceDataShapeConfig::m_modifierTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceDataShapeConfig>(
                    "Shape Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceDataShapeConfig::m_providerTags, "Generated Tags", "Surface tags to add to created points")
                    ->DataElement(0, &SurfaceDataShapeConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ;
            }
        }
    }

    void SurfaceDataShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void SurfaceDataShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void SurfaceDataShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void SurfaceDataShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceDataShapeConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataShapeComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceDataShapeComponent::m_configuration)
                ;
        }
    }

    SurfaceDataShapeComponent::SurfaceDataShapeComponent(const SurfaceDataShapeConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceDataShapeComponent::Activate()
    {
        SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_shapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;

        m_providerHandle = InvalidSurfaceDataRegistryHandle;
        SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);
        SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);

        registryEntry.m_tags = m_configuration.m_modifierTags;

        m_modifierHandle = InvalidSurfaceDataRegistryHandle;
        SurfaceDataSystemRequestBus::BroadcastResult(m_modifierHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, registryEntry);
        SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

        UpdateShapeData();
        m_refresh = false;
    }

    void SurfaceDataShapeComponent::Deactivate()
    {
        m_refresh = false;
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();

        SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
        m_providerHandle = InvalidSurfaceDataRegistryHandle;

        SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
        m_modifierHandle = InvalidSurfaceDataRegistryHandle;
    }

    bool SurfaceDataShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceDataShapeConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceDataShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceDataShapeConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void SurfaceDataShapeComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (m_shapeBoundsIsValid)
        {
            const AZ::Vector3 rayOrigin = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_shapeBounds.GetMax().GetZ());
            const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();
            AZ::VectorFloat intersectionDistance = 0.0f;
            bool hitShape = false;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(hitShape, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, rayOrigin, rayDirection, intersectionDistance);
            if (hitShape)
            {
                SurfacePoint point;
                point.m_entityId = GetEntityId();
                point.m_position = rayOrigin + intersectionDistance * rayDirection;
                point.m_normal = AZ::Vector3::CreateAxisZ();
                AddMaxValueForMasks(point.m_masks, m_configuration.m_providerTags, 1.0f);
                surfacePointList.push_back(point);
            }
        }
    }

    void SurfaceDataShapeComponent::ModifySurfacePoints(SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (m_shapeBoundsIsValid && !m_configuration.m_modifierTags.empty())
        {
            const AZ::EntityId entityId = GetEntityId();
            for (auto& point : surfacePointList)
            {
                if (point.m_entityId != entityId && m_shapeBounds.Contains(point.m_position))
                {
                    bool inside = false;
                    LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point.m_position);
                    if (inside)
                    {
                        AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, 1.0f);
                    }
                }
            }
        }
    }

    void SurfaceDataShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        OnCompositionChanged();
    }

    void SurfaceDataShapeComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        OnCompositionChanged();
    }

    void SurfaceDataShapeComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateShapeData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SurfaceDataShapeComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SurfaceDataShapeComponent::UpdateShapeData()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZ::Aabb dirtyVolume = AZ::Aabb::CreateNull();

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            // To make sure we update our surface data even if our surface shrinks or grows, keep track of a bounding box
            // that contains the old surface volume.  At the end, we'll add in our new surface volume.
            if (m_shapeBoundsIsValid)
            {
                dirtyVolume.AddAabb(m_shapeBounds);
            }

            m_shapeWorldTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(m_shapeWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            m_shapeBounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(m_shapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            m_shapeBoundsIsValid = m_shapeBounds.IsValid();

            // Add our new surface volume to our dirty volume, so that we have a bounding box that encompasses everything that's been added / changed / removed
            if (m_shapeBoundsIsValid)
            {
                dirtyVolume.AddAabb(m_shapeBounds);
            }
        }

        SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_shapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;
        SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry, dirtyVolume);

        registryEntry.m_tags = m_configuration.m_modifierTags;
        SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier, m_modifierHandle, registryEntry, dirtyVolume);
    }

    const float SurfaceDataShapeComponent::s_rayAABBHeightPadding = 0.1f;
}
