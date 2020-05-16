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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
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

        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        m_refresh = false;

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        CrySystemEventBus::Handler::BusConnect();

        // Update the cached shape data and bounds, then register the surface data provider / modifier
        UpdateShapeData();
    }

    void WaterVolumeSurfaceDataComponent::Deactivate()
    {
        if (m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle)
        {
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        }
        if (m_modifierHandle != SurfaceData::InvalidSurfaceDataRegistryHandle)
        {
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
            m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        }

        m_system = nullptr;
        m_refresh = false;
        CrySystemEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();

        // Clear the cached shape data
        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);
            m_shapeBounds = AZ::Aabb::CreateNull();
            m_shapeBoundsIsValid = false;
        }
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

        if (m_shapeBoundsIsValid)
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

                bool isTerrainActive = false;
                float terrainHeight = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();
                bool isPointInTerrain = false;
                auto enumerationCallback = [&](AzFramework::Terrain::TerrainDataRequests* terrain) -> bool
                {
                    isTerrainActive = true;
                    terrainHeight = terrain->GetHeightFromFloats(resultPosition.GetX(), resultPosition.GetY());
                    isPointInTerrain = SurfaceData::AabbContains2D(terrain->GetTerrainAabb(), resultPosition);
                    // Only one handler should exist.
                    return false;
                };
                AzFramework::Terrain::TerrainDataRequestBus::EnumerateHandlers(enumerationCallback);

                if (!isTerrainActive || !isPointInTerrain || resultPosition.GetZ() >= terrainHeight)
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

        if (m_shapeBoundsIsValid && !m_configuration.m_modifierTags.empty())
        {
            bool isTerrainActive = false;
            const AZ::EntityId entityId = GetEntityId();
            auto enumerationCallback = [&](AzFramework::Terrain::TerrainDataRequests* terrain) -> bool
            {
                isTerrainActive = true;
                for (auto& point : surfacePointList)
                {
                    if (point.m_entityId != entityId && m_shapeBounds.Contains(point.m_position))
                    {
                        bool inside = false;
                        LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point.m_position);
                        if (inside)
                        {
                            const AZ::Vector3& resultPosition = point.m_position;
                            const float terrainHeight = terrain->GetHeightFromFloats(resultPosition.GetX(), resultPosition.GetY());
                            if (!terrain->GetTerrainAabb().Contains(resultPosition) || resultPosition.GetZ() >= terrainHeight)
                            {
                                AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, 1.0f);
                            }
                        }
                    }
                }

                // Only one handler should exist.
                return false;
            };
            AzFramework::Terrain::TerrainDataRequestBus::EnumerateHandlers(enumerationCallback);

            if (isTerrainActive)
            {
                return;
            }

            for (auto& point : surfacePointList)
            {
                if (point.m_entityId != entityId && m_shapeBounds.Contains(point.m_position))
                {
                    bool inside = false;
                    LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point.m_position);
                    if (inside)
                    {
                        const AZ::Vector3& resultPosition = point.m_position;
                        AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, 1.0f);
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

        bool shapeValidBeforeUpdate = false;
        bool shapeValidAfterUpdate = false;

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            shapeValidBeforeUpdate = m_shapeBoundsIsValid;

            m_shapeBounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(m_shapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            m_shapeBoundsIsValid = m_shapeBounds.IsValid();

            shapeValidAfterUpdate = m_shapeBoundsIsValid;
        }

        SurfaceData::SurfaceDataRegistryEntry providerRegistryEntry;
        providerRegistryEntry.m_entityId = GetEntityId();
        providerRegistryEntry.m_bounds = m_shapeBounds;
        providerRegistryEntry.m_tags = m_configuration.m_providerTags;

        SurfaceData::SurfaceDataRegistryEntry modifierRegistryEntry(providerRegistryEntry);
        modifierRegistryEntry.m_tags = m_configuration.m_modifierTags;

        if (shapeValidBeforeUpdate && shapeValidAfterUpdate)
        {
            // Our shape was valid before and after, it just changed in some way, so update our registry entries
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid modifier data handle");
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, providerRegistryEntry);
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier, m_modifierHandle, modifierRegistryEntry);
        }
        else if (!shapeValidBeforeUpdate && shapeValidAfterUpdate)
        {
            // Our shape has become valid, so register as a provider and save off the registry handles
            AZ_Assert((m_providerHandle == SurfaceData::InvalidSurfaceDataRegistryHandle), "Surface Provider data handle is initialized before our shape became valid");
            AZ_Assert((m_modifierHandle == SurfaceData::InvalidSurfaceDataRegistryHandle), "Surface Modifier data handle is initialized before our shape became valid");
            SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, providerRegistryEntry);
            SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_modifierHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, modifierRegistryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
            SurfaceData::SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);
        }
        else if (shapeValidBeforeUpdate && !shapeValidAfterUpdate)
        {
            // Our shape has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
            m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
            SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        }
        else
        {
            // We didn't have a valid shape before or after running this, so do nothing.
        }
    }
}
