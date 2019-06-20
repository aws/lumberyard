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

#include "Water_precompiled.h"

#include <Water/WaterOceanComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <LmbrCentral/Physics/WaterNotificationBus.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AZ::OceanEnvironmentRequests::ReflectionFlags, "{1A340FFB-5458-46B3-8421-D5E0B2BAD6F5}");
}

namespace Water
{
    // static
    void WaterOceanComponent::Reflect(AZ::ReflectContext* context)
    {
        Water::WaterOceanComponentData::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<WaterOceanComponent, AZ::Component>()
                ->Version(2)
                ->Field("data", &WaterOceanComponent::m_data)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<AZ::OceanEnvironmentBus>("OceanEnvironmentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("GetOceanLevel", &AZ::OceanEnvironmentBus::Events::GetOceanLevel)

                ->Event("GetFogColor", &AZ::OceanEnvironmentBus::Events::GetFogColor)
                ->Event("SetFogColor", &AZ::OceanEnvironmentBus::Events::SetFogColor)
                ->VirtualProperty("FogColor", "GetFogColor", "SetFogColor")

                ->Event("GetFogColorMulitplier", &AZ::OceanEnvironmentBus::Events::GetFogColorMulitplier)
                ->Event("SetFogColorMulitplier", &AZ::OceanEnvironmentBus::Events::SetFogColorMulitplier)
                ->VirtualProperty("FogColorMulitplier", "GetFogColorMulitplier", "SetFogColorMulitplier")

                ->Event("GetFogColorPremultiplied", &AZ::OceanEnvironmentBus::Events::GetFogColorPremultiplied)

                ->Event("SetFogDensity", &AZ::OceanEnvironmentBus::Events::SetFogDensity)
                ->Event("GetFogDensity", &AZ::OceanEnvironmentBus::Events::GetFogDensity)
                ->VirtualProperty("FogDensity", "GetFogDensity", "SetFogDensity")

                ->Event("GetNearFogColor", &AZ::OceanEnvironmentBus::Events::GetNearFogColor)
                ->Event("SetNearFogColor", &AZ::OceanEnvironmentBus::Events::SetNearFogColor)
                ->VirtualProperty("NearFogColor", "GetNearFogColor", "SetNearFogColor")

                ->Event("GetWaterTessellationAmount", &AZ::OceanEnvironmentBus::Events::GetWaterTessellationAmount)
                ->Event("SetWaterTessellationAmount", &AZ::OceanEnvironmentBus::Events::SetWaterTessellationAmount)

                ->Event("GetAnimationWindDirection", &AZ::OceanEnvironmentBus::Events::GetAnimationWindDirection)
                ->Event("SetAnimationWindDirection", &AZ::OceanEnvironmentBus::Events::SetAnimationWindDirection)
                ->VirtualProperty("WindDirection", "GetAnimationWindDirection", "SetAnimationWindDirection")

                ->Event("GetAnimationWindSpeed", &AZ::OceanEnvironmentBus::Events::GetAnimationWindSpeed)
                ->Event("SetAnimationWindSpeed", &AZ::OceanEnvironmentBus::Events::SetAnimationWindSpeed)
                ->VirtualProperty("WindSpeed", "GetAnimationWindSpeed", "SetAnimationWindSpeed")

                ->Event("GetAnimationWavesSpeed", &AZ::OceanEnvironmentBus::Events::GetAnimationWavesSpeed)
                ->Event("SetAnimationWavesSpeed", &AZ::OceanEnvironmentBus::Events::SetAnimationWavesSpeed)
                ->VirtualProperty("WavesSpeed", "GetAnimationWavesSpeed", "SetAnimationWavesSpeed")

                ->Event("GetAnimationWavesSize", &AZ::OceanEnvironmentBus::Events::GetAnimationWavesSize)
                ->Event("SetAnimationWavesSize", &AZ::OceanEnvironmentBus::Events::SetAnimationWavesSize)
                ->VirtualProperty("WavesSize", "GetAnimationWavesSize", "SetAnimationWavesSize")

                ->Event("GetAnimationWavesAmount", &AZ::OceanEnvironmentBus::Events::GetAnimationWavesAmount)
                ->Event("SetAnimationWavesAmount", &AZ::OceanEnvironmentBus::Events::SetAnimationWavesAmount)
                ->VirtualProperty("WavesAmount", "GetAnimationWavesAmount", "SetAnimationWavesAmount")

                ->Event("GetReflectResolutionScale", &AZ::OceanEnvironmentBus::Events::GetReflectResolutionScale)
                ->Event("SetReflectResolutionScale", &AZ::OceanEnvironmentBus::Events::SetReflectResolutionScale)
                ->VirtualProperty("ReflectResolutionScale", "GetReflectResolutionScale", "SetReflectResolutionScale")

                ->Event("GetReflectionAnisotropic", &AZ::OceanEnvironmentBus::Events::GetReflectionAnisotropic)
                ->Event("SetReflectionAnisotropic", &AZ::OceanEnvironmentBus::Events::SetReflectionAnisotropic)
                ->VirtualProperty("ReflectionAnisotropic", "GetReflectionAnisotropic", "SetReflectionAnisotropic")

                ->Event("GetUseOceanBottom", &AZ::OceanEnvironmentBus::Events::GetUseOceanBottom)
                ->Event("SetUseOceanBottom", &AZ::OceanEnvironmentBus::Events::SetUseOceanBottom)
                ->VirtualProperty("OceanBottom", "GetUseOceanBottom", "SetUseOceanBottom")

                ->Event("GetGodRaysEnabled", &AZ::OceanEnvironmentBus::Events::GetGodRaysEnabled)
                ->Event("SetGodRaysEnabled", &AZ::OceanEnvironmentBus::Events::SetGodRaysEnabled)
                ->VirtualProperty("GodRays", "GetGodRaysEnabled", "SetGodRaysEnabled")

                ->Event("GetUnderwaterDistortion", &AZ::OceanEnvironmentBus::Events::GetUnderwaterDistortion)
                ->Event("SetUnderwaterDistortion", &AZ::OceanEnvironmentBus::Events::SetUnderwaterDistortion)
                ->VirtualProperty("UnderwaterDistortion", "GetUnderwaterDistortion", "SetUnderwaterDistortion")

                ->Event("GetReflectRenderFlag", &AZ::OceanEnvironmentBus::Events::GetReflectRenderFlag)
                ->Event("SetReflectRenderFlag", &AZ::OceanEnvironmentBus::Events::SetReflectRenderFlag)

                ->Event("GetCausticsEnabled", &AZ::OceanEnvironmentBus::Events::GetCausticsEnabled)
                ->Event("SetCausticsEnabled", &AZ::OceanEnvironmentBus::Events::SetCausticsEnabled)
                ->VirtualProperty("CausticsEnabled", "GetCausticsEnabled", "SetCausticsEnabled")

                ->Event("GetCausticsDepth", &AZ::OceanEnvironmentBus::Events::GetCausticsDepth)
                ->Event("SetCausticsDepth", &AZ::OceanEnvironmentBus::Events::SetCausticsDepth)
                ->VirtualProperty("CausticsDepth", "GetCausticsDepth", "SetCausticsDepth")

                ->Event("GetCausticsIntensity", &AZ::OceanEnvironmentBus::Events::GetCausticsIntensity)
                ->Event("SetCausticsIntensity", &AZ::OceanEnvironmentBus::Events::SetCausticsIntensity)
                ->VirtualProperty("CausticsIntensity", "GetCausticsIntensity", "SetCausticsIntensity")

                ->Event("GetCausticsTiling", &AZ::OceanEnvironmentBus::Events::GetCausticsTiling)
                ->Event("SetCausticsTiling", &AZ::OceanEnvironmentBus::Events::SetCausticsTiling)
                ->VirtualProperty("CausticsTiling", "GetCausticsTiling", "SetCausticsTiling")

                ->Event("GetCausticsDistanceAttenuation", &AZ::OceanEnvironmentBus::Events::GetCausticsDistanceAttenuation)
                ->Event("SetCausticsDistanceAttenuation", &AZ::OceanEnvironmentBus::Events::SetCausticsDistanceAttenuation)
                ->VirtualProperty("CausticsDistanceAttenuation", "GetCausticsDistanceAttenuation", "SetCausticsDistanceAttenuation")

                ->Event("GetOceanMaterialName", &AZ::OceanEnvironmentBus::Events::GetOceanMaterialName)
                ->Event("SetOceanMaterialName", &AZ::OceanEnvironmentBus::Events::SetOceanMaterialName)
                ;

            behaviorContext->Class<AZ::OceanEnvironmentRequests::ReflectionFlags>("ReflectionFlags")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Enum<int(AZ::OceanEnvironmentRequests::ReflectionFlags::StaticObjects)>("StaticObjects")
                ->Enum<int(AZ::OceanEnvironmentRequests::ReflectionFlags::Particles)>("Particles")
                ->Enum<int(AZ::OceanEnvironmentRequests::ReflectionFlags::Entities)>("Entities")
                ->Enum<int(AZ::OceanEnvironmentRequests::ReflectionFlags::TerrainDetailMaterials)>("TerrainDetailMaterials")
                ;

            behaviorContext->Class<WaterOceanComponent>()->RequestBus("OceanEnvironmentRequestBus");
        }
    }

    WaterOceanComponent::WaterOceanComponent()
        : m_data()
    {
    }

    WaterOceanComponent::WaterOceanComponent(const WaterOceanComponentData& data)
        : m_data(data)
    {
    }

    // static
    void WaterOceanComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("WaterOceanService", 0x12a06661));
    }

    // static
    void WaterOceanComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("WaterOceanService", 0x12a06661));
    }

    // static
    void WaterOceanComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void WaterOceanComponent::Activate()
    {
        m_data.Activate();
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

        AZ::Transform worldTransform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(worldTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        m_data.SetOceanLevel(worldTransform.GetPosition().GetZ());

        LmbrCentral::WaterNotificationBus::Broadcast(&LmbrCentral::WaterNotificationBus::Events::OceanHeightChanged, worldTransform.GetPosition().GetZ());
    }

    void WaterOceanComponent::Deactivate()
    {
        m_data.Deactivate();
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());

        LmbrCentral::WaterNotificationBus::Broadcast(&LmbrCentral::WaterNotificationBus::Events::OceanHeightChanged, AZ::OceanConstants::s_HeightUnknown);
    }

    void WaterOceanComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_data.SetOceanLevel(world.GetPosition().GetZ());
        LmbrCentral::WaterNotificationBus::Broadcast(&LmbrCentral::WaterNotificationBus::Events::OceanHeightChanged, world.GetPosition().GetZ());
    }
}

