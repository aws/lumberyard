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

#include "GradientSignal_precompiled.h"
#include "GradientSurfaceDataComponent.h"
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

namespace GradientSignal
{
    void GradientSurfaceDataConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientSurfaceDataConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("ThresholdMin", &GradientSurfaceDataConfig::m_thresholdMin)
                ->Field("ThresholdMax", &GradientSurfaceDataConfig::m_thresholdMax)
                ->Field("ModifierTags", &GradientSurfaceDataConfig::m_modifierTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<GradientSurfaceDataConfig>(
                    "Gradient Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSurfaceDataConfig::m_thresholdMin, "Threshold Min", "Minimum value accepted from input gradient that allows tags to be applied.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSurfaceDataConfig::m_thresholdMax, "Threshold Max", "Maximum value accepted from input gradient that allows tags to be applied.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &GradientSurfaceDataConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<GradientSurfaceDataConfig>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Method("GetNumTags", &GradientSurfaceDataConfig::GetNumTags)
                ->Method("GetTag", &GradientSurfaceDataConfig::GetTag)
                ->Method("RemoveTag", &GradientSurfaceDataConfig::RemoveTag)
                ->Method("AddTag", &GradientSurfaceDataConfig::AddTag)
                ;
        }
    }

    size_t GradientSurfaceDataConfig::GetNumTags() const
    {
        return m_modifierTags.size();
    }

    AZ::Crc32 GradientSurfaceDataConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_modifierTags.size() && tagIndex >= 0)
        {
            return m_modifierTags[tagIndex];
        }

        return AZ::Crc32();
    }

    void GradientSurfaceDataConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_modifierTags.size() && tagIndex >= 0)
        {
            m_modifierTags.erase(m_modifierTags.begin() + tagIndex);
        }
    }

    void GradientSurfaceDataConfig::AddTag(AZStd::string tag)
    {
        m_modifierTags.push_back(SurfaceData::SurfaceTag(tag));
    }

    void GradientSurfaceDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void GradientSurfaceDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void GradientSurfaceDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void GradientSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientSurfaceDataConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientSurfaceDataComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &GradientSurfaceDataComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("GradientSurfaceDataComponentTypeId", BehaviorConstant(GradientSurfaceDataComponentTypeId));

            behaviorContext->Class<GradientSurfaceDataComponent>()->RequestBus("GradientSurfaceDataRequestBus");

            behaviorContext->EBus<GradientSurfaceDataRequestBus>("GradientSurfaceDataRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("SetThresholdMin", &GradientSurfaceDataRequestBus::Events::SetThresholdMin)
                ->Event("GetThresholdMin", &GradientSurfaceDataRequestBus::Events::GetThresholdMin)
                ->VirtualProperty("ThresholdMin", "GetThresholdMin", "SetThresholdMin")
                ->Event("SetThresholdMax", &GradientSurfaceDataRequestBus::Events::SetThresholdMax)
                ->Event("GetThresholdMax", &GradientSurfaceDataRequestBus::Events::GetThresholdMax)
                ->VirtualProperty("ThresholdMax", "GetThresholdMax", "SetThresholdMax")
                ->Event("GetNumTags", &GradientSurfaceDataRequestBus::Events::GetNumTags)
                ->Event("GetTag", &GradientSurfaceDataRequestBus::Events::GetTag)
                ->Event("RemoveTag", &GradientSurfaceDataRequestBus::Events::RemoveTag)
                ->Event("AddTag", &GradientSurfaceDataRequestBus::Events::AddTag)
                ;
        }
    }

    GradientSurfaceDataComponent::GradientSurfaceDataComponent(const GradientSurfaceDataConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void GradientSurfaceDataComponent::Activate()
    {
        m_gradientSampler.m_gradientId = GetEntityId();
        m_gradientSampler.m_ownerEntityId = GetEntityId();

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = AZ::Aabb::CreateNull();
        registryEntry.m_tags = m_configuration.m_modifierTags;

        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_modifierHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, registryEntry);
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        GradientSurfaceDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void GradientSurfaceDataComponent::Deactivate()
    {
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        GradientSurfaceDataRequestBus::Handler::BusDisconnect();
    }

    bool GradientSurfaceDataComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const GradientSurfaceDataConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool GradientSurfaceDataComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<GradientSurfaceDataConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void GradientSurfaceDataComponent::ModifySurfacePoints(SurfaceData::SurfacePointList& surfacePointList) const
    {
        if (!m_configuration.m_modifierTags.empty())
        {
            const AZ::EntityId entityId = GetEntityId();
            for (auto& point : surfacePointList)
            {
                if (point.m_entityId != entityId)
                {
                    const GradientSampleParams sampleParams = { point.m_position };
                    const float value = m_gradientSampler.GetValue(sampleParams);
                    if (value >= m_configuration.m_thresholdMin &&
                        value <= m_configuration.m_thresholdMax)
                    {
                        SurfaceData::AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, value);
                    }
                }
            }
        }
    }

    void GradientSurfaceDataComponent::OnCompositionChanged()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_bounds = AZ::Aabb::CreateNull();
        registryEntry.m_tags = m_configuration.m_modifierTags;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier, m_modifierHandle, registryEntry, AZ::Aabb::CreateNull());
    }

    void GradientSurfaceDataComponent::SetThresholdMin(float thresholdMin)
    {
        m_configuration.m_thresholdMin = thresholdMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float GradientSurfaceDataComponent::GetThresholdMin() const
    {
        return m_configuration.m_thresholdMin;
    }

    void GradientSurfaceDataComponent::SetThresholdMax(float thresholdMax)
    {
        m_configuration.m_thresholdMax = thresholdMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float GradientSurfaceDataComponent::GetThresholdMax() const
    {
        return m_configuration.m_thresholdMax;
    }

    size_t GradientSurfaceDataComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 GradientSurfaceDataComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void GradientSurfaceDataComponent::RemoveTag(int tagIndex)
    {
        m_configuration.RemoveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void GradientSurfaceDataComponent::AddTag(AZStd::string tag)
    {
        m_configuration.AddTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}