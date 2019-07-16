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
#include <Vegetation/AreaComponentBase.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <AzCore/Debug/Profiler.h>

namespace Vegetation
{
    namespace AreaUtil
    {
        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 1)
            {
                AZ::u8 areaType = 0; //default to cluster
                if (classElement.GetChildData(AZ_CRC("AreaType", 0x5365f66f), areaType))
                {
                    classElement.RemoveElementByName(AZ_CRC("AreaType", 0x5365f66f));
                    switch (areaType)
                    {
                    default:
                    case 0: //cluster
                        classElement.AddElementWithData(context, "Layer", AreaLayer::Foreground);
                        break;
                    case 1: //coverage
                        classElement.AddElementWithData(context, "Layer", AreaLayer::Background);
                        break;
                    }
                }

                int priority = 1;
                if (classElement.GetChildData(AZ_CRC("Priority", 0x62a6dc27), priority))
                {
                    classElement.RemoveElementByName(AZ_CRC("Priority", 0x62a6dc27));
                    classElement.AddElementWithData(context, "Priority", (float)(priority - 1) / (float)std::numeric_limits<int>::max());
                }
            }
            return true;
        }
    }

    void AreaConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaConfig, AZ::ComponentConfig>()
                ->Version(1, &AreaUtil::UpdateVersion)
                ->Field("Layer", &AreaConfig::m_layer)
                ->Field("Priority", &AreaConfig::m_priority)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<AreaConfig>(
                    "Vegetation Area", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AreaConfig::m_layer, "Layer Priority", "Defines a high level order vegetation areas are applied")
                    ->EnumAttribute(AreaLayer::Background, "Background")
                    ->EnumAttribute(AreaLayer::Foreground, "Foreground")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &AreaConfig::m_priority, "Sub Priority", "Defines order vegetation areas are applied within a layer.  Larger numbers = higher priority")
                    ->Attribute(AZ::Edit::Attributes::Min, AreaConfig::s_priorityMin)
                    ->Attribute(AZ::Edit::Attributes::Max, AreaConfig::s_priorityMax)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AreaConfig>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("areaPriority", BehaviorValueProperty(&AreaConfig::m_priority))
                ->Property("areaLayer",
                    [](AreaConfig* config) { return (AZ::u8&)(config->m_layer); },
                    [](AreaConfig* config, const AZ::u8& i) { config->m_layer = (AreaLayer)i; })
                ;
        }
    }

    void AreaComponentBase::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaService", 0x6a859504));
    }

    void AreaComponentBase::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaService", 0x6a859504));
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void AreaComponentBase::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void AreaComponentBase::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaComponentBase, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &AreaComponentBase::m_configuration)
                ;
        }
    }

    AreaComponentBase::AreaComponentBase(const AreaConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void AreaComponentBase::Activate()
    {
        m_refreshPending = false;
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AreaNotificationBus::Handler::BusConnect(GetEntityId());
        AreaInfoBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RegisterArea, GetEntityId());
    }

    void AreaComponentBase::Deactivate()
    {
        AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::UnregisterArea, GetEntityId());
        AreaNotificationBus::Handler::BusDisconnect();
        AreaInfoBus::Handler::BusDisconnect();
        AreaRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        m_refreshPending = false;
    }

    bool AreaComponentBase::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AreaConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool AreaComponentBase::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AreaConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float AreaComponentBase::GetPriority() const
    {
        return (float)m_configuration.m_layer + AZ::GetClamp(m_configuration.m_priority, 0.0f, 1.0f);
    }

    AZ::u32 AreaComponentBase::GetChangeIndex() const
    {
        return m_changeIndex;
    }

    void AreaComponentBase::OnCompositionChanged()
    {
        if (!m_refreshPending)
        {
            m_refreshPending = true;
            AreaSystemRequestBus::Broadcast(&AreaSystemRequestBus::Events::RefreshArea, GetEntityId());
        }
        ++m_changeIndex;
    }

    void AreaComponentBase::OnAreaConnect()
    {
        AreaRequestBus::Handler::BusConnect(GetEntityId());
    }

    void AreaComponentBase::OnAreaDisconnect()
    {
        AreaRequestBus::Handler::BusDisconnect();
    }

    void AreaComponentBase::OnAreaRefreshed()
    {
        m_refreshPending = false;
    }

    void AreaComponentBase::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);
        OnCompositionChanged();
    }

    void AreaComponentBase::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);
        OnCompositionChanged();
    }
}