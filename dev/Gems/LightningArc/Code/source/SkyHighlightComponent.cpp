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

#include "LightningArc_precompiled.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/std/algorithm.h>
#include <MathConversion.h>

#include "SkyHighlightComponent.h"

namespace Lightning
{
	const float SkyHighlightComponent::MinSkyHighlightSize = 0.000001f;

    void SkyHighlightConfiguration::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SkyHighlightConfiguration>()
                ->Version(1)
                ->Field("Enabled", &SkyHighlightConfiguration::m_enabled)
                ->Field("Color", &SkyHighlightConfiguration::m_color)
                ->Field("ColorMultiplier", &SkyHighlightConfiguration::m_colorMultiplier)
                ->Field("VerticalOffset", &SkyHighlightConfiguration::m_verticalOffset)
                ->Field("Size", &SkyHighlightConfiguration::m_size)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SkyHighlightComponentRequestBus>("SkyHighlightComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)

                ->Event("Enable", &SkyHighlightComponentRequestBus::Events::Enable)

                ->Event("Disable", &SkyHighlightComponentRequestBus::Events::Disable)

                ->Event("Toggle", &SkyHighlightComponentRequestBus::Events::Toggle)

                ->Event("IsEnabled", &SkyHighlightComponentRequestBus::Events::IsEnabled)

                ->Event("SetColor", &SkyHighlightComponentRequestBus::Events::SetColor)
                ->Event("GetColor", &SkyHighlightComponentRequestBus::Events::GetColor)
                ->VirtualProperty("Color", "GetColor", "SetColor")

                ->Event("SetColorMultiplier", &SkyHighlightComponentRequestBus::Events::SetColorMultiplier)
                ->Event("GetColorMultiplier", &SkyHighlightComponentRequestBus::Events::GetColorMultiplier)
                ->VirtualProperty("ColorMultiplier", "GetColorMultiplier", "SetColorMultiplier")

                ->Event("SetVerticalOffset", &SkyHighlightComponentRequestBus::Events::SetVerticalOffset)
                ->Event("GetVerticalOffset", &SkyHighlightComponentRequestBus::Events::GetVerticalOffset)
                ->VirtualProperty("VerticalOffset", "GetVerticalOffset", "SetVerticalOffset")

                ->Event("SetSize", &SkyHighlightComponentRequestBus::Events::SetSize)
                ->Event("GetSize", &SkyHighlightComponentRequestBus::Events::GetSize)
                ->VirtualProperty("Size", "GetSize", "SetSize")
                ;

            behaviorContext->Class<SkyHighlightComponent>()->RequestBus("SkyHighlightComponentRequestBus");
        }
    }

    void SkyHighlightComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("SkyHighlightService"));
    }

    void SkyHighlightComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType & requires)
    {
        requires.push_back(AZ_CRC("TransformService"));
    }

    void SkyHighlightComponent::Reflect(AZ::ReflectContext * context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SkyHighlightComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &SkyHighlightComponent::m_config)
                ;
        }

        SkyHighlightConfiguration::Reflect(context);
    }

    void SkyHighlightComponent::Activate()
    {
        SkyHighlightComponentRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

        //Retrieve the transform on startup
        //We can rely on the transform component listening to this bus call because we depend on the TransformService
        AZ::Transform world = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(world, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        //Call OnTransformChanged because it will set the transform and update the sky highlight effect
        //Identity transform for "local" transform because our impl doesn't need local transform info
        OnTransformChanged(AZ::Transform::CreateIdentity(), world);
    }

    void SkyHighlightComponent::Deactivate()
    {
        TurnOffHighlight();

        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        SkyHighlightComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void SkyHighlightComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_worldPos = world.GetPosition();

        UpdateSkyHighlight();
    }

    void SkyHighlightComponent::Enable() 
    {
        m_config.m_enabled = true;
        UpdateSkyHighlight();
    }

    void SkyHighlightComponent::Disable() 
    {
        m_config.m_enabled = false;
        TurnOffHighlight();
    }
    
    void SkyHighlightComponent::Toggle() 
    {
        m_config.m_enabled = !m_config.m_enabled;
        if (m_config.m_enabled)
        {
            UpdateSkyHighlight();
        }
        else
        {
            TurnOffHighlight();
        }
    }

    void SkyHighlightComponent::SetColor(const AZ::Color& color)
    {
        m_config.m_color = color;
        UpdateSkyHighlight();
    }

    void SkyHighlightComponent::SetColorMultiplier(float colorMultiplier)
    {
        m_config.m_colorMultiplier = colorMultiplier;
        UpdateSkyHighlight();
    }

    void SkyHighlightComponent::SetVerticalOffset(float verticalOffset)
    {
        m_config.m_verticalOffset = verticalOffset;
        UpdateSkyHighlight();
    }

    void SkyHighlightComponent::SetSize(float lightAttenuation)
    {
        m_config.m_size = lightAttenuation;
        UpdateSkyHighlight();
    }

    void SkyHighlightComponent::UpdateSkyHighlight()
    {
        if (m_config.m_enabled == false)
        {
            return;
        }

        AZ::Vector3 colorVec = m_config.m_color.GetAsVector3() * m_config.m_colorMultiplier;
        AZ::Vector3 offsetPos = m_worldPos;
        offsetPos.SetZ(offsetPos.GetZ() + m_config.m_verticalOffset);

        float size = AZStd::max<float>(m_config.m_size, SkyHighlightComponent::MinSkyHighlightSize);
        Vec3 color = AZVec3ToLYVec3(colorVec);
        Vec3 pos = AZVec3ToLYVec3(offsetPos);

        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_SIZE, size);
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, color);
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_POS, pos);
    }

    void SkyHighlightComponent::TurnOffHighlight()
    {
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_SIZE, SkyHighlightComponent::MinSkyHighlightSize);
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, Vec3());
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_POS, Vec3());
    }

} //namespace Lightning