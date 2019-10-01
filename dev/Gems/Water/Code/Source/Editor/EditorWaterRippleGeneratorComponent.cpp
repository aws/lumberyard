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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "EditorWaterRippleGeneratorComponent.h"

namespace Water
{
    void EditorWaterRippleGeneratorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWaterRippleGeneratorComponent, EditorComponentBase>()
                ->Version(0)
                ->Field("RippleGenerator", &EditorWaterRippleGeneratorComponent::m_rippleGenerator)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorWaterRippleGeneratorComponent>(
                    "Water Ripple Generator", "Generates Ripples at the parent entity's location.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(0, &EditorWaterRippleGeneratorComponent::m_rippleGenerator, "Ripple Generator Settings", "Settings for the Ripple Generator.")
                    ;
            }
        }
    }

    void EditorWaterRippleGeneratorComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<WaterRippleGeneratorComponent>(m_rippleGenerator);
    }

    void EditorWaterRippleGeneratorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        WaterRippleGeneratorComponent::GetProvidedServices(provided);
    }

    void EditorWaterRippleGeneratorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        WaterRippleGeneratorComponent::GetIncompatibleServices(incompatible);
    }

    void EditorWaterRippleGeneratorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        WaterRippleGeneratorComponent::GetRequiredServices(required);
    }

    void EditorWaterRippleGeneratorComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        WaterRippleGeneratorComponent::GetDependentServices(dependent);
    }

    void EditorWaterRippleGeneratorComponent::Activate()
    {
        EditorComponentBase::Activate();
    }

    void EditorWaterRippleGeneratorComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
    }

} // namespace Water
