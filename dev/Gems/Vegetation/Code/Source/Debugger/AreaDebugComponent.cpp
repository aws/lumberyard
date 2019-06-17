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

#include "Vegetation_precompiled.h"
#include <Debugger/AreaDebugComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <AzCore/Debug/Profiler.h>

namespace Vegetation
{
    void AreaDebugConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaDebugConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("DebugColor", &AreaDebugConfig::m_debugColor)
                ->Field("CubeSize", &AreaDebugConfig::m_debugCubeSize)
                ->Field("PropagateDebug", &AreaDebugConfig::m_propagateDebug)
                ->Field("InheritDebug", &AreaDebugConfig::m_inheritDebug)
                ->Field("HideInDebug", &AreaDebugConfig::m_hideDebug)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<AreaDebugConfig>(
                    "Vegetation Layer Debugger Config", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Color, &AreaDebugConfig::m_debugColor, "Debug Visualization Color", "")
                    ->DataElement(AZ::Edit::UIHandlers::Color, &AreaDebugConfig::m_debugCubeSize, "Debug Visualization Cube Size", "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AreaDebugConfig::m_propagateDebug, "Propagate debug flags to child areas", "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AreaDebugConfig::m_inheritDebug, "Inherit debug flags from parent areas", "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AreaDebugConfig::m_hideDebug, "Hide created instance in the Debug Visualization", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AreaDebugConfig>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("DebugColor", BehaviorValueProperty(&AreaDebugConfig::m_debugColor))
                ->Property("DebugCubeSize", BehaviorValueProperty(&AreaDebugConfig::m_debugCubeSize))
                ->Property("PropagateDebug", BehaviorValueProperty(&AreaDebugConfig::m_propagateDebug))
                ->Property("InheritDebug", BehaviorValueProperty(&AreaDebugConfig::m_inheritDebug))
                ->Property("HideInDebug", BehaviorValueProperty(&AreaDebugConfig::m_hideDebug))
                ;
        }
    }

    void AreaDebugComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaDebugService", 0x2c6f3c5c));
    }

    void AreaDebugComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaDebugService", 0x2c6f3c5c));
    }

    void AreaDebugComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void AreaDebugComponent::Reflect(AZ::ReflectContext* context)
    {
        AreaDebugConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaDebugComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &AreaDebugComponent::m_configuration)
                ;
        }
    }

    AreaDebugComponent::AreaDebugComponent(const AreaDebugConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void AreaDebugComponent::Activate()
    {
        AreaDebugBus::Handler::BusConnect(GetEntityId());
    }

    void AreaDebugComponent::Deactivate()
    {
        AreaDebugBus::Handler::BusDisconnect();
    }

    bool AreaDebugComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AreaDebugConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool AreaDebugComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AreaDebugConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void AreaDebugComponent::SetDebugColor(const AZ::Color& color)
    {
        m_configuration.m_debugColor = color;
    }

    void AreaDebugComponent::SetDebugPropagate(bool propagate)
    {
        m_configuration.m_propagateDebug = propagate;
    }

    void AreaDebugComponent::SetDebugInherit(bool inherit)
    {
        m_configuration.m_inheritDebug = inherit;
    }

    void AreaDebugComponent::SetDebugHide(bool hide)
    {
        m_configuration.m_hideDebug = hide;
    }

    void AreaDebugComponent::GetDebugColorOverride(bool& inherit, bool& propagate, bool& hide, AZ::Color& color) const
    {
        inherit = m_configuration.m_inheritDebug;
        propagate = m_configuration.m_propagateDebug;
        hide = m_configuration.m_hideDebug;
        color = m_configuration.m_debugColor;
        color.SetA(m_configuration.m_debugCubeSize);
    }
}