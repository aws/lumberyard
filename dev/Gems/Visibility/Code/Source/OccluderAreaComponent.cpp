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
#include "OccluderAreaComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>

namespace Visibility
{
    void OccluderAreaComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("OccluderAreaService"));
    }
    void OccluderAreaComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService"));
    }

    void OccluderAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<OccluderAreaConfiguration>()
                ->Version(1)
                ->Field("DisplayFilled", &OccluderAreaConfiguration::m_displayFilled)
                ->Field("CullDistRatio", &OccluderAreaConfiguration::m_cullDistRatio)
                ->Field("UseInIndoors", &OccluderAreaConfiguration::m_useInIndoors)
                ->Field("DoubleSide", &OccluderAreaConfiguration::m_doubleSide)
                ->Field("vertices", &OccluderAreaConfiguration::m_vertices)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<OccluderAreaRequestBus>("OccluderAreaRequestBus")
                
                ->Event("SetDisplayFilled", &OccluderAreaRequestBus::Events::SetDisplayFilled)
                ->Event("GetDisplayFilled", &OccluderAreaRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", "SetDisplayFilled")

                ->Event("SetCullDistRatio", &OccluderAreaRequestBus::Events::SetCullDistRatio)
                ->Event("GetCullDistRatio", &OccluderAreaRequestBus::Events::GetCullDistRatio)
                ->VirtualProperty("CullDistRatio", "GetCullDistRatio", "SetCullDistRatio")

                ->Event("SetUseInIndoors", &OccluderAreaRequestBus::Events::SetUseInIndoors)
                ->Event("GetUseInIndoors", &OccluderAreaRequestBus::Events::GetUseInIndoors)
                ->VirtualProperty("UseInIndoors", "GetUseInIndoors", "SetUseInIndoors")

                ->Event("SetDoubleSide", &OccluderAreaRequestBus::Events::SetDoubleSide)
                ->Event("GetDoubleSide", &OccluderAreaRequestBus::Events::GetDoubleSide)
                ->VirtualProperty("DoubleSide", "GetDoubleSide", "SetDoubleSide")
            ;

            behaviorContext->Class<OccluderAreaComponent>()->RequestBus("OccluderAreaRequestBus");
        }
    }

    void OccluderAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<OccluderAreaComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &OccluderAreaComponent::m_config)
            ;
        }

        OccluderAreaConfiguration::Reflect(context);
    }

    void OccluderAreaComponent::Activate()
    {
        Update();
        OccluderAreaRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void OccluderAreaComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        OccluderAreaRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void OccluderAreaComponent::SetDisplayFilled(const bool value)
    {
        m_config.m_displayFilled = value;
        Update();
    }
    bool OccluderAreaComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    void OccluderAreaComponent::SetCullDistRatio(const float value)
    {
        m_config.m_cullDistRatio = value;
        Update();
    }
    float OccluderAreaComponent::GetCullDistRatio()
    {
        return m_config.m_cullDistRatio;
    }

    void OccluderAreaComponent::SetUseInIndoors(const bool value)
    {
        m_config.m_useInIndoors = value;
        Update();
    }
    bool OccluderAreaComponent::GetUseInIndoors()
    {
        return m_config.m_useInIndoors;
    }

    void OccluderAreaComponent::SetDoubleSide(const bool value)
    {
        m_config.m_doubleSide = value;
        Update();
    }
    bool OccluderAreaComponent::GetDoubleSide()
    {
        return m_config.m_doubleSide;
    }

    void OccluderAreaComponent::Update()
    {
    }
} //namespace Visibility