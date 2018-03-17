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
#include "VisAreaComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>

namespace Visibility
{

    void VisAreaComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ::Crc32("VisAreaService"));
    }
    void VisAreaComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ::Crc32("TransformService"));
    }

    void VisAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<VisAreaConfiguration>()
                ->Version(2, &VersionConverter)
                ->Field("m_Height", &VisAreaConfiguration::m_Height)
                ->Field("m_DisplayFilled", &VisAreaConfiguration::m_DisplayFilled)
                ->Field("m_AffectedBySun", &VisAreaConfiguration::m_AffectedBySun)
                ->Field("m_ViewDistRatio", &VisAreaConfiguration::m_ViewDistRatio)
                ->Field("m_OceanIsVisible", &VisAreaConfiguration::m_OceanIsVisible)
                ->Field("m_vertexContainer", &VisAreaConfiguration::m_vertexContainer)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<VisAreaComponentRequestBus>("VisAreaComponentRequestBus")


                ->Event("SetHeight", &VisAreaComponentRequestBus::Events::SetHeight)
                ->Event("GetHeight", &VisAreaComponentRequestBus::Events::GetHeight)
                ->VirtualProperty("Height", "GetHeight", "SetHeight")

                ->Event("SetDisplayFilled", &VisAreaComponentRequestBus::Events::SetDisplayFilled)
                ->Event("GetDisplayFilled", &VisAreaComponentRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", "SetDisplayFilled")

                ->Event("SetAffectedBySun", &VisAreaComponentRequestBus::Events::SetAffectedBySun)
                ->Event("GetAffectedBySun", &VisAreaComponentRequestBus::Events::GetAffectedBySun)
                ->VirtualProperty("AffectedBySun", "GetAffectedBySun", "SetAffectedBySun")

                ->Event("SetViewDistRatio", &VisAreaComponentRequestBus::Events::SetViewDistRatio)
                ->Event("GetViewDistRatio", &VisAreaComponentRequestBus::Events::GetViewDistRatio)
                ->VirtualProperty("ViewDistRatio", "GetViewDistRatio", "SetViewDistRatio")

                ->Event("SetOceanIsVisible", &VisAreaComponentRequestBus::Events::SetOceanIsVisible)
                ->Event("GetOceanIsVisible", &VisAreaComponentRequestBus::Events::GetOceanIsVisible)
                ->VirtualProperty("OceanIsVisible", "GetOceanIsVisible", "SetOceanIsVisible")
                ;

            behaviorContext->Class<VisAreaComponent>()->RequestBus("VisAreaComponentRequestBus");
        }
    }

    bool VisAreaConfiguration::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - Remove IgnoreSkyColor
        // - Remove IgnoreGI
        // - Remove SkyOnly
        if (classElement.GetVersion() <= 1)
        {
            classElement.RemoveElementByName(AZ_CRC("IgnoreSkyColor"));
            classElement.RemoveElementByName(AZ_CRC("IgnoreGI"));
            classElement.RemoveElementByName(AZ_CRC("SkyOnly"));
        }

        return true;
    }

    void VisAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<VisAreaComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &VisAreaComponent::m_config)
                ;
        }

        VisAreaConfiguration::Reflect(context);
    }
    
    void VisAreaComponent::Activate()
    {
        VisAreaComponentRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void VisAreaComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        VisAreaComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void VisAreaComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {

    }

    void VisAreaComponent::SetHeight(const float value)
    {
        m_config.m_Height = value;
        
    }
    float VisAreaComponent::GetHeight()
    {
        return m_config.m_Height;
    }

    void VisAreaComponent::SetDisplayFilled(const bool value)
    {
        m_config.m_DisplayFilled = value;
        
    }
    bool VisAreaComponent::GetDisplayFilled()
    {
        return m_config.m_DisplayFilled;
    }

    void VisAreaComponent::SetAffectedBySun(const bool value)
    {
        m_config.m_AffectedBySun = value;
        
    }
    bool VisAreaComponent::GetAffectedBySun()
    {
        return m_config.m_AffectedBySun;
    }

    void VisAreaComponent::SetViewDistRatio(const float value)
    {
        m_config.m_ViewDistRatio = value;
    }
    float VisAreaComponent::GetViewDistRatio()
    {
        return m_config.m_ViewDistRatio;
    }

    void VisAreaComponent::SetOceanIsVisible(const bool value)
    {
        m_config.m_OceanIsVisible = value;
    }
    bool VisAreaComponent::GetOceanIsVisible()
    {
        return m_config.m_OceanIsVisible;
    }

    void VisAreaComponent::SetVertices(const AZStd::vector<AZ::Vector3>& value)
    {
        m_config.m_vertexContainer.SetVertices(value);
    }
    const AZStd::vector<AZ::Vector3>& VisAreaComponent::GetVertices()
    {
        return m_config.m_vertexContainer.GetVertices();
    }

} //namespace Visibility