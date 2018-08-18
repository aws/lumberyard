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

#include "LmbrCentral_precompiled.h"

#include "BoxShapeComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeComponentConverters.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void BoxShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Configuration", &BoxShapeDebugDisplayComponent::m_boxShapeConfig)
                ;
        }
    }


    void BoxShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
    }

    void BoxShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void BoxShapeDebugDisplayComponent::Draw(AzFramework::EntityDebugDisplayRequests* displayContext)
    {
        DrawBoxShape(g_defaultShapeDrawParams, m_boxShapeConfig, *displayContext);
    }

    bool BoxShapeDebugDisplayComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const BoxShapeConfig*>(baseConfig))
        {
            m_boxShapeConfig = *config;
            return true;
        }
        return false;
    }

    bool BoxShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<BoxShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_boxShapeConfig;
            return true;
        }
        return false;
    }

    void BoxShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            BoxShapeComponentRequestsBus::EventResult(m_boxShapeConfig, GetEntityId(), &BoxShapeComponentRequests::GetBoxConfiguration);
        }
    }

    namespace ClassConverters
    {
        static bool DeprecateBoxColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static bool DeprecateBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void BoxShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: BoxColliderConfiguration -> BoxShapeConfig
            serializeContext->ClassDeprecate(
                "BoxColliderConfiguration",
                "{282E47CB-9F6D-47AE-A210-4CE879527EFD}",
                &ClassConverters::DeprecateBoxColliderConfiguration)
                ;

            serializeContext->Class<BoxShapeConfig>()
                ->Version(1)
                ->Field("Dimensions", &BoxShapeConfig::m_dimensions)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BoxShapeConfig>("Configuration", "Box shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BoxShapeConfig::m_dimensions, "Dimensions", "Dimensions of the box along its axes")
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<BoxShapeConfig>()
                ->Constructor()
                ->Constructor<AZ::Vector3&>()
                ->Property("Dimensions", BehaviorValueProperty(&BoxShapeConfig::m_dimensions))
                ;
        }
    }

    void BoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        BoxShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: BoxColliderComponent -> BoxShapeComponent
            serializeContext->ClassDeprecate(
                "BoxColliderComponent",
                "{C215EB2A-1803-4EDC-B032-F7C92C142337}",
                &ClassConverters::DeprecateBoxColliderComponent)
                ;

            serializeContext->Class<BoxShapeComponent, Component>()
                ->Version(2, &ClassConverters::UpgradeBoxShapeComponent)
                ->Field("BoxShape", &BoxShapeComponent::m_boxShape)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("BoxShapeComponentTypeId", BehaviorConstant(BoxShapeComponentTypeId));

            behaviorContext->EBus<BoxShapeComponentRequestsBus>("BoxShapeComponentRequestsBus")
                ->Event("GetBoxConfiguration", &BoxShapeComponentRequestsBus::Events::GetBoxConfiguration)
                ->Event("GetBoxDimensions", &BoxShapeComponentRequestsBus::Events::GetBoxDimensions)
                ->Event("SetBoxDimensions", &BoxShapeComponentRequestsBus::Events::SetBoxDimensions)
                ;
        }
    }

    void BoxShapeComponent::Activate()
    {
        m_boxShape.Activate(GetEntityId());
    }

    void BoxShapeComponent::Deactivate()
    {
        m_boxShape.Deactivate();
    }

    bool BoxShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const BoxShapeConfig*>(baseConfig))
        {
            m_boxShape.SetBoxConfiguration(*config);
            return true;
        }
        return false;
    }

    bool BoxShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<BoxShapeConfig*>(outBaseConfig))
        {
            *config = m_boxShape.GetBoxConfiguration();
            return true;
        }
        return false;
    }

    namespace ClassConverters
    {
        static bool DeprecateBoxColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="BoxColliderConfiguration" field="Configuration" version="1" type="{282E47CB-9F6D-47AE-A210-4CE879527EFD}">
            <Class name="Vector3" field="Size" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
            </Class>

            New:
            <Class name="BoxShapeConfig" field="Configuration" version="1" type="{F034FBA2-AC2F-4E66-8152-14DFB90D6283}">
            <Class name="Vector3" field="Dimensions" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
            </Class>
            */

            // Cache the Dimensions
            AZ::Vector3 oldDimensions;
            const int oldIndex = classElement.FindElement(AZ_CRC("Size", 0xf7c0246a));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<AZ::Vector3>(oldDimensions);
            }

            // Convert to BoxShapeConfig
            const bool result = classElement.Convert(context, "{F034FBA2-AC2F-4E66-8152-14DFB90D6283}");
            if (result)
            {
                const int newIndex = classElement.AddElement<AZ::Vector3>(context, "Dimensions");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<AZ::Vector3>(context, oldDimensions);
                    return true;
                }
            }
            return false;
        }

        static bool DeprecateBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="BoxColliderComponent" version="1" type="{C215EB2A-1803-4EDC-B032-F7C92C142337}">
             <Class name="BoxColliderConfiguration" field="Configuration" version="1" type="{282E47CB-9F6D-47AE-A210-4CE879527EFD}">
              <Class name="Vector3" field="Size" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
             </Class>
            </Class>

            New:
            <Class name="BoxShapeComponent" version="1" type="{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}">
             <Class name="BoxShapeConfig" field="Configuration" version="1" type="{F034FBA2-AC2F-4E66-8152-14DFB90D6283}">
              <Class name="Vector3" field="Dimensions" value="1.0000000 2.0000000 3.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            BoxShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<BoxShapeConfig>(configuration);
            }

            // Convert to BoxShapeComponent
            const bool result = classElement.Convert(context, BoxShapeComponentTypeId);
            if (result)
            {
                configIndex = classElement.AddElement<BoxShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<BoxShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
