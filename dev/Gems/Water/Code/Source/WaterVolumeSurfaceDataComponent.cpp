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

#include "Water_precompiled.h"
#include <Source/WaterVolumeSurfaceDataComponent.h>
#include <Water/WaterSystemComponent.h>

#include <AZCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <I3DEngine.h>
#include <ISystem.h>

namespace Water
{
    namespace WaterVolumeConstants
    {
        static const AZ::Crc32 s_waterVolumeTagCrc = AZ::Crc32(Constants::s_waterVolumeTagName);
        static const AZ::Crc32 s_underWaterTagCrc = AZ::Crc32(Constants::s_underWaterTagName);
        static const AZ::Crc32 s_waterTagCrc = AZ::Crc32(Constants::s_waterTagName);
    }

    void WaterVolumeSurfaceDataConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<WaterVolumeSurfaceDataConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ProviderTags", &WaterVolumeSurfaceDataConfig::m_providerTags)
                ->Field("ModifierTags", &WaterVolumeSurfaceDataConfig::m_modifierTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<WaterVolumeSurfaceDataConfig>(
                    "Water Volume Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &WaterVolumeSurfaceDataConfig::m_providerTags, "Generated Tags", "Surface tags to add to created points")
                    ->DataElement(0, &WaterVolumeSurfaceDataConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ;
            }
        }
    }

    void WaterVolumeSurfaceDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void WaterVolumeSurfaceDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void WaterVolumeSurfaceDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        services.push_back(AZ_CRC("WaterVolumeService", 0x895e29b1));
    }

    void WaterVolumeSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        WaterVolumeSurfaceDataConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<WaterVolumeSurfaceDataComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &WaterVolumeSurfaceDataComponent::m_configuration)
                ;
        }
    }

    WaterVolumeSurfaceDataComponent::WaterVolumeSurfaceDataComponent(const WaterVolumeSurfaceDataConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void WaterVolumeSurfaceDataComponent::Activate()
    {
        m_system = GetISystem();
        AddItemIfNotFound(m_configuration.m_providerTags, WaterVolumeConstants::s_waterVolumeTagCrc);
        AddItemIfNotFound(m_configuration.m_providerTags, WaterVolumeConstants::s_waterTagCrc);
        AddItemIfNotFound(m_configuration.m_modifierTags, WaterVolumeConstants::s_underWaterTagCrc);

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_shapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;

        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);
        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);

        registryEntry.m_tags = m_configuration.m_modifierTags;

        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_modifierHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, registryEntry);
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        CrySystemEventBus::Handler::BusConnect();

        UpdateShapeData();
        m_refresh = false;
    }

    void WaterVolumeSurfaceDataComponent::Deactivate()
    {
        m_system = nullptr;
        m_refresh = false;
        CrySystemEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();

        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
    }

    bool WaterVolumeSurfaceDataComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const WaterVolumeSurfaceDataConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool WaterVolumeSurfaceDataComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<WaterVolumeSurfaceDataConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void WaterVolumeSurfaceDataComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        auto engine = m_system ? m_system->GetI3DEngine() : nullptr;
        if (engine && m_shapeBoundsIsValid)
        {
            const AZ::Vector3 rayOrigin = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_shapeBounds.GetMax().GetZ());
            const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();
            AZ::VectorFloat intersectionDistance = 0.0f;
            bool hitShape = false;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(hitShape, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, rayOrigin, rayDirection, intersectionDistance);
            if (hitShape)
            {
                const AZ::Vector3 resultPosition = rayOrigin + rayDirection * intersectionDistance;
                const AZ::Vector3 resultNormal = AZ::Vector3::CreateAxisZ();
                const float terrainHeight = engine->GetTerrainElevation(resultPosition.GetX(), resultPosition.GetY());
                if (!engine->GetShowTerrainSurface() || !engine->GetTerrainAabb().Contains(resultPosition) || resultPosition.GetZ() >= terrainHeight)
                {
                    SurfaceData::SurfacePoint point;
                    point.m_entityId = GetEntityId();
                    point.m_position = resultPosition;
                    point.m_normal = resultNormal;
                    AddMaxValueForMasks(point.m_masks, m_configuration.m_providerTags, 1.0f);
                    surfacePointList.push_back(point);
                }
            }
        }
    }

    void WaterVolumeSurfaceDataComponent::ModifySurfacePoints(SurfaceData::SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        auto engine = m_system ? m_system->GetI3DEngine() : nullptr;
        if (engine && m_shapeBoundsIsValid && !m_configuration.m_modifierTags.empty())
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
                        const AZ::Vector3& resultPosition = point.m_position;
                        const float terrainHeight = engine->GetTerrainElevation(resultPosition.GetX(), resultPosition.GetY());
                        if (!engine->GetShowTerrainSurface() || !engine->GetTerrainAabb().Contains(resultPosition) || resultPosition.GetZ() >= terrainHeight)
                        {
                            AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, 1.0f);
                        }
                    }
                }
            }
        }
    }

    void WaterVolumeSurfaceDataComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        OnCompositionChanged();
    }

    void WaterVolumeSurfaceDataComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        OnCompositionChanged();
    }

    void WaterVolumeSurfaceDataComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateShapeData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void WaterVolumeSurfaceDataComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
    }

    void WaterVolumeSurfaceDataComponent::OnCrySystemShutdown(ISystem& system)
    {
        m_system = nullptr;
    }

    void WaterVolumeSurfaceDataComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void WaterVolumeSurfaceDataComponent::UpdateShapeData()
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

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_shapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry, dirtyVolume);

        registryEntry.m_tags = m_configuration.m_modifierTags;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier, m_modifierHandle, registryEntry, dirtyVolume);
    }
}