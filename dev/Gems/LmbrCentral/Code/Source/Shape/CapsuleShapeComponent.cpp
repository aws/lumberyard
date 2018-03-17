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
#include "CapsuleShapeComponent.h"
#include <MathConversion.h>
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateCapsuleColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static bool DeprecateCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void CapsuleShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: CapsuleColliderConfiguration -> CapsuleShapeConfig
            serializeContext->ClassDeprecate(
                "CapsuleColliderConfiguration",
                "{902BCDA9-C9E5-429C-991B-74C241ED2889}",
                &ClassConverters::DeprecateCapsuleColliderConfiguration
                );

            serializeContext->Class<CapsuleShapeConfig>()
                ->Version(1)
                ->Field("Height", &CapsuleShapeConfig::m_height)
                ->Field("Radius", &CapsuleShapeConfig::m_radius)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<CapsuleShapeConfig>("Configuration", "Capsule shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfig::m_height, "Height", "End to end height of capsule, this includes the cylinder and both caps")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfig::m_radius, "Radius", "Radius of capsule")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<CapsuleShapeConfig>()
                ->Property("Height", BehaviorValueProperty(&CapsuleShapeConfig::m_height))
                ->Property("Radius", BehaviorValueProperty(&CapsuleShapeConfig::m_radius))
                ;
        }
    }

    void CapsuleShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        CapsuleShapeConfig::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: CapsuleColliderComponent -> CapsuleShapeComponent
            serializeContext->ClassDeprecate(
                "CapsuleColliderComponent",
                "{D1F746A9-FC24-48E4-88DE-5B3122CB6DE7}",
                &ClassConverters::DeprecateCapsuleColliderComponent
                );

            serializeContext->Class<CapsuleShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &CapsuleShapeComponent::m_configuration)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Constant("CapsuleShapeComponentTypeId", BehaviorConstant(CapsuleShapeComponentTypeId));

            behaviorContext->EBus<CapsuleShapeComponentRequestsBus>("CapsuleShapeComponentRequestsBus")
                ->Event("GetCapsuleConfiguration", &CapsuleShapeComponentRequestsBus::Events::GetCapsuleConfiguration)
                ->Event("SetHeight", &CapsuleShapeComponentRequestsBus::Events::SetHeight)
                ->Event("SetRadius", &CapsuleShapeComponentRequestsBus::Events::SetRadius)
                ;
        }

    }

    void CapsuleShapeComponent::Activate()
    {
        CapsuleShape::Activate(GetEntityId());
    }

    void CapsuleShapeComponent::Deactivate()
    {
        CapsuleShape::Deactivate();
    }

    bool CapsuleShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const CapsuleShapeConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool CapsuleShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<CapsuleShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_configuration;
            return true;
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    namespace ClassConverters
    {
        static bool DeprecateCapsuleColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CapsuleColliderConfiguration" field="Configuration" version="1" type="{902BCDA9-C9E5-429C-991B-74C241ED2889}">
             <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>

            New:
            <Class name="CapsuleShapeConfig" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
             <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            */

            // Cache the Height and Radius
            float oldHeight = 0.f;
            float oldRadius = 0.f;

            int oldIndex = classElement.FindElement(AZ_CRC("Height", 0xf54de50f));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldHeight);
            }

            oldIndex = classElement.FindElement(AZ_CRC("Radius", 0x3b7c6e5a));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldRadius);
            }

            // Convert to CapsuleShapeConfig
            bool result = classElement.Convert<CapsuleShapeConfig>(context);
            if (result)
            {
                int newIndex = classElement.AddElement<float>(context, "Height");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldHeight);
                }

                newIndex = classElement.AddElement<float>(context, "Radius");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldRadius);
                }
                return true;
            }
            return false;
        }

        static bool DeprecateCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CapsuleColliderComponent" version="1" type="{D1F746A9-FC24-48E4-88DE-5B3122CB6DE7}">
             <Class name="CapsuleColliderConfiguration" field="Configuration" version="1" type="{902BCDA9-C9E5-429C-991B-74C241ED2889}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="CapsuleShapeComponent" version="1" type="{967EC13D-364D-4696-AB5C-C00CC05A2305}">
             <Class name="CapsuleShapeConfig" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            CapsuleShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CapsuleShapeConfig>(configuration);
            }

            // Convert to CapsuleShapeComponent
            bool result = classElement.Convert<CapsuleShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<CapsuleShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CapsuleShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
