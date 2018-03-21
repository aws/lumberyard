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

#include "Visibility_precompiled.h"
#include "PortalComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>

namespace Visibility
{
    void PortalComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ::Crc32("PortalService"));
    }
    void PortalComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ::Crc32("TransformService"));
    }

    void PortalConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PortalConfiguration>()
                ->Version(1)
                ->Field("Height", &PortalConfiguration::m_height)
                ->Field("DisplayFilled", &PortalConfiguration::m_displayFilled)
                ->Field("AffectedBySun", &PortalConfiguration::m_affectedBySun)
                ->Field("ViewDistRatio", &PortalConfiguration::m_viewDistRatio)
                ->Field("SkyOnly", &PortalConfiguration::m_skyOnly)
                ->Field("OceanIsVisible", &PortalConfiguration::m_oceanIsVisible)
                ->Field("UseDeepness", &PortalConfiguration::m_useDeepness)
                ->Field("DoubleSide", &PortalConfiguration::m_doubleSide)
                ->Field("LightBlending", &PortalConfiguration::m_lightBlending)
                ->Field("LightBlendValue", &PortalConfiguration::m_lightBlendValue)
                ->Field("vertices", &PortalConfiguration::m_vertices)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PortalRequestBus>("PortalRequestBus")

                ->Event("SetHeight", &PortalRequestBus::Events::SetHeight)
                ->Event("GetHeight", &PortalRequestBus::Events::GetHeight)
                ->VirtualProperty("Height", "GetHeight", "SetHeight")

                ->Event("SetDisplayFilled", &PortalRequestBus::Events::SetDisplayFilled)
                ->Event("GetDisplayFilled", &PortalRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", "SetDisplayFilled")

                ->Event("SetAffectedBySun", &PortalRequestBus::Events::SetAffectedBySun)
                ->Event("GetAffectedBySun", &PortalRequestBus::Events::GetAffectedBySun)
                ->VirtualProperty("AffectedBySun", "GetAffectedBySun", "SetAffectedBySun")

                ->Event("SetViewDistRatio", &PortalRequestBus::Events::SetViewDistRatio)
                ->Event("GetViewDistRatio", &PortalRequestBus::Events::GetViewDistRatio)
                ->VirtualProperty("ViewDistRatio", "GetViewDistRatio", "SetViewDistRatio")

                ->Event("SetSkyOnly", &PortalRequestBus::Events::SetSkyOnly)
                ->Event("GetSkyOnly", &PortalRequestBus::Events::GetSkyOnly)
                ->VirtualProperty("SkyOnly", "GetSkyOnly", "SetSkyOnly")

                ->Event("SetOceanIsVisible", &PortalRequestBus::Events::SetOceanIsVisible)
                ->Event("GetOceanIsVisible", &PortalRequestBus::Events::GetOceanIsVisible)
                ->VirtualProperty("OceanIsVisible", "GetOceanIsVisible", "SetOceanIsVisible")

                ->Event("SetUseDeepness", &PortalRequestBus::Events::SetUseDeepness)
                ->Event("GetUseDeepness", &PortalRequestBus::Events::GetUseDeepness)
                ->VirtualProperty("UseDeepness", "GetUseDeepness", "SetUseDeepness")

                ->Event("SetDoubleSide", &PortalRequestBus::Events::SetDoubleSide)
                ->Event("GetDoubleSide", &PortalRequestBus::Events::GetDoubleSide)
                ->VirtualProperty("DoubleSide", "GetDoubleSide", "SetDoubleSide")

                ->Event("SetLightBlending", &PortalRequestBus::Events::SetLightBlending)
                ->Event("GetLightBlending", &PortalRequestBus::Events::GetLightBlending)
                ->VirtualProperty("LightBlending", "GetLightBlending", "SetLightBlending")

                ->Event("SetLightBlendValue", &PortalRequestBus::Events::SetLightBlendValue)
                ->Event("GetLightBlendValue", &PortalRequestBus::Events::GetLightBlendValue)
                ->VirtualProperty("LightBlendValue", "GetLightBlendValue", "SetLightBlendValue")
                ;

            behaviorContext->Class<PortalComponent>()->RequestBus("PortalRequestBus");
        }
    }

    bool PortalConfiguration::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - Remove IgnoreSkyColor
        // - Remove IgnoreGI
        if (classElement.GetVersion() <= 1)
        {
            classElement.RemoveElementByName(AZ_CRC("IgnoreSkyColor"));
            classElement.RemoveElementByName(AZ_CRC("IgnoreGI"));
        }

        return true;
    }

    void PortalComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PortalComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &PortalComponent::m_config)
                ;
        }

        PortalConfiguration::Reflect(context);
    }

    void PortalComponent::Activate()
    {
        Update();
        PortalRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void PortalComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        PortalRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void PortalComponent::SetHeight(const float value)
    {
        m_config.m_height = value;
        Update();
    }
    float PortalComponent::GetHeight()
    {
        return m_config.m_height;
    }

    void PortalComponent::SetDisplayFilled(const bool value)
    {
        m_config.m_displayFilled = value;
        Update();
    }
    bool PortalComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    void PortalComponent::SetAffectedBySun(const bool value)
    {
        m_config.m_affectedBySun = value;
        Update();
    }
    bool PortalComponent::GetAffectedBySun()
    {
        return m_config.m_affectedBySun;
    }

    void PortalComponent::SetViewDistRatio(const float value)
    {
        m_config.m_viewDistRatio = value;
        Update();
    }
    float PortalComponent::GetViewDistRatio()
    {
        return m_config.m_viewDistRatio;
    }

    void PortalComponent::SetSkyOnly(const bool value)
    {
        m_config.m_skyOnly = value;
        Update();
    }
    bool PortalComponent::GetSkyOnly()
    {
        return m_config.m_skyOnly;
    }

    void PortalComponent::SetOceanIsVisible(const bool value)
    {
        m_config.m_oceanIsVisible = value;
        Update();
    }
    bool PortalComponent::GetOceanIsVisible()
    {
        return m_config.m_oceanIsVisible;
    }

    void PortalComponent::SetUseDeepness(const bool value)
    {
        m_config.m_useDeepness = value;
        Update();
    }
    bool PortalComponent::GetUseDeepness()
    {
        return m_config.m_useDeepness;
    }

    void PortalComponent::SetDoubleSide(const bool value)
    {
        m_config.m_doubleSide = value;
        Update();
    }
    bool PortalComponent::GetDoubleSide()
    {
        return m_config.m_doubleSide;
    }

    void PortalComponent::SetLightBlending(const bool value)
    {
        m_config.m_lightBlending = value;
        Update();
    }
    bool PortalComponent::GetLightBlending()
    {
        return m_config.m_lightBlending;
    }

    void PortalComponent::SetLightBlendValue(const float value)
    {
        m_config.m_lightBlendValue = value;
        Update();
    }
    float PortalComponent::GetLightBlendValue()
    {
        return m_config.m_lightBlendValue;
    }

    void PortalComponent::Update()
    {
    }
} //namespace Visibility