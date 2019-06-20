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
#include <Source/OceanSurfaceDataComponent.h>
#include <Water/WaterSystemComponent.h>

#include <AZCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Math/MathUtils.h>
#include <MathConversion.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <I3DEngine.h>
#include <ISystem.h>

namespace Water
{
    namespace OceanConstants
    {
        static const AZ::Crc32 s_oceanTagCrc = AZ::Crc32(Constants::s_oceanTagName);
        static const AZ::Crc32 s_underWaterTagCrc = AZ::Crc32(Constants::s_underWaterTagName);
        static const AZ::Crc32 s_waterTagCrc = AZ::Crc32(Constants::s_waterTagName);
    }

    void OceanSurfaceDataConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<OceanSurfaceDataConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ProviderTags", &OceanSurfaceDataConfig::m_providerTags)
                ->Field("ModifierTags", &OceanSurfaceDataConfig::m_modifierTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<OceanSurfaceDataConfig>(
                    "Ocean Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &OceanSurfaceDataConfig::m_providerTags, "Generated Tags", "Surface tags to add to created points")
                    ->DataElement(0, &OceanSurfaceDataConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ;
            }
        }
    }

    void OceanSurfaceDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void OceanSurfaceDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void OceanSurfaceDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("WaterOceanService", 0x12a06661));
    }

    void OceanSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        OceanSurfaceDataConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<OceanSurfaceDataComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &OceanSurfaceDataComponent::m_configuration)
                ;
        }
    }

    OceanSurfaceDataComponent::OceanSurfaceDataComponent(const OceanSurfaceDataConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void OceanSurfaceDataComponent::Activate()
    {
        m_system = GetISystem();
        AddItemIfNotFound(m_configuration.m_providerTags, OceanConstants::s_oceanTagCrc);
        AddItemIfNotFound(m_configuration.m_providerTags, OceanConstants::s_waterTagCrc);
        AddItemIfNotFound(m_configuration.m_modifierTags, OceanConstants::s_underWaterTagCrc);

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
        LmbrCentral::WaterNotificationBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();

        UpdateShapeData();
        m_refresh = false;
    }

    void OceanSurfaceDataComponent::Deactivate()
    {
        m_system = nullptr;
        m_refresh = false;
        CrySystemEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::WaterNotificationBus::Handler::BusDisconnect();

        SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
        m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
    }

    bool OceanSurfaceDataComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const OceanSurfaceDataConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool OceanSurfaceDataComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<OceanSurfaceDataConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void OceanSurfaceDataComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        auto engine = m_system ? m_system->GetI3DEngine() : nullptr;
        if (engine)
        {
            const float waterHeight = engine->GetWaterLevel();
            const AZ::Vector3 resultPosition = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), waterHeight);
            const AZ::Vector3 resultNormal = AZ::Vector3::CreateAxisZ();
            const float terrainHeight = engine->GetTerrainElevation(resultPosition.GetX(), resultPosition.GetY());
            if (waterHeight != WATER_LEVEL_UNKNOWN && (!engine->GetShowTerrainSurface() || !engine->GetTerrainAabb().Contains(inPosition) || waterHeight >= terrainHeight))
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

    void OceanSurfaceDataComponent::ModifySurfacePoints(SurfaceData::SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        auto engine = m_system ? m_system->GetI3DEngine() : nullptr;
        if (engine && !m_configuration.m_modifierTags.empty())
        {
            const float waterHeight = engine->GetWaterLevel();
            const AZ::EntityId entityId = GetEntityId();
            for (auto& point : surfacePointList)
            {
                if (point.m_entityId != entityId && waterHeight != WATER_LEVEL_UNKNOWN && waterHeight > point.m_position.GetZ())
                {
                    AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, 1.0f);
                }
            }
        }
    }

    void OceanSurfaceDataComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        OnCompositionChanged();
    }

    void OceanSurfaceDataComponent::OceanHeightChanged(float height)
    {
        OnCompositionChanged();
    }

    void OceanSurfaceDataComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateShapeData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void OceanSurfaceDataComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
    }

    void OceanSurfaceDataComponent::OnCrySystemShutdown(ISystem& system)
    {
        m_system = nullptr;
    }

    void OceanSurfaceDataComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void OceanSurfaceDataComponent::UpdateShapeData()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            m_shapeWorldTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(m_shapeWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            m_shapeBounds = AZ::Aabb::CreateNull();
        }

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = m_shapeBounds;
        registryEntry.m_tags = m_configuration.m_providerTags;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, registryEntry, AZ::Aabb::CreateNull());

        registryEntry.m_tags = m_configuration.m_modifierTags;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier, m_modifierHandle, registryEntry, AZ::Aabb::CreateNull());
    }
}