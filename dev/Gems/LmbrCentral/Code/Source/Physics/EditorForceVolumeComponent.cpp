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
#include "EditorForceVolumeComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    void EditorForceVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorForceVolumeComponent, EditorComponentBase>()
                ->Field("Visible", &EditorForceVolumeComponent::m_visibleInEditor)
                ->Field("Configuration", &EditorForceVolumeComponent::m_configuration)
                ->Version(1)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                // EditorForceVolumeComponent
                editContext->Class<EditorForceVolumeComponent>(
                    "Force Volume", "The force volume component is used to apply a physical force on objects within the volume")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/ForceVolume.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/ForceVolume.png")
                    //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c)) Disabled for v1.12
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview) // Hidden for v1.12
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/force-volume-component")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ProximityTriggerService", 0x561f262c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorForceVolumeComponent::m_visibleInEditor, "Visible", "Always display this component in the editor viewport")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorForceVolumeComponent::m_configuration, "Configuration", "Configuration for force volume.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;

                // ForceVolumeConfiguration
                editContext->Class<ForceVolumeConfiguration>(
                    "Force Volume Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Force")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ForceVolumeConfiguration::m_forceMode, "Mode", "How the direction is calculated. Point it is calculated relative to the center of the volume. Direction is specified manually.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->EnumAttribute(ForceMode::ePoint, "Point")
                    ->EnumAttribute(ForceMode::eDirection, "Direction")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ForceVolumeConfiguration::m_forceSpace, "Space", "Worldspace direction is independent of the volumes orientation. Localspace direction is relative to the volumes orientation.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ForceVolumeConfiguration::UseDirection)
                    ->EnumAttribute(ForceSpace::eWorldSpace, "World Space")
                    ->EnumAttribute(ForceSpace::eLocalSpace, "Local Space")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ForceVolumeConfiguration::m_useMass, "Use Mass", "If true, the force scales proportionally with the mass of the entity, otherwise lighter entities will be affected more than heavier ones.")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ForceVolumeConfiguration::m_forceMagnitude, "Magnitude", "The size of the force applied, if 0 it has no effect.")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " N")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ForceVolumeConfiguration::m_forceDirection, "Direction", "The direction the force is applied.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ForceVolumeConfiguration::UseDirection)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Resistance")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ForceVolumeConfiguration::m_volumeDamping, "Damping", "Applies a force opposite to the objects velocity, if 0 it has no effect.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ForceVolumeConfiguration::m_volumeDensity, "Density", "The density of the volume used to simulate drag, if 0 it has no effect.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " kg/m^3")
                ;
            }
        }
    }

    void EditorForceVolumeComponent::Activate()
    {
        EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorForceVolumeComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    EditorForceVolumeComponent::EditorForceVolumeComponent(const ForceVolumeConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void EditorForceVolumeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<ForceVolumeComponent>(m_configuration);
    }

    void EditorForceVolumeComponent::DisplayEntity(bool& handled)
    {
        if (!IsSelected() && !m_visibleInEditor)
        {
            return;
        }

        handled = true;

        AZ::Aabb volumeAabb;
        AZ::Quaternion volumeRotation = AZ::Quaternion::CreateIdentity();
        ShapeComponentRequestsBus::EventResult(volumeAabb, GetEntityId(), &ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        AZ::TransformBus::EventResult(volumeRotation, GetEntityId(), &AZ::TransformBus::Events::GetWorldRotationQuaternion);

        AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "Invalid display context.");
        displayContext->SetColor(AZ::Vector4(1.f, 0.f, 0.f, 1.0f));

        const auto minSamples = 2; // Always have a minimum of 2 points in each dimension - 2x2x2 (each corner of a cube)
        const auto maxSamples = 8; // Max out at 8x8x8 samples otherwise the editor will start slowing down.
        const auto sampleDensity = 2.f; // Arbitary scaling factor which will increase samples from min->max relative to size

        // Sample slightly inside the bounds
        auto min = volumeAabb.GetMin() + AZ::Vector3(0.0001f, 0.0001f, 0.0001f);
        auto size = volumeAabb.GetExtents() * 0.998f;

        // Sample count increases with size, but limited to 8*8*8 samples
        int numSamples[] =
        {
            static_cast<int>((size.GetX() / sampleDensity).GetClamp(minSamples, maxSamples)),
            static_cast<int>((size.GetY() / sampleDensity).GetClamp(minSamples, maxSamples)),
            static_cast<int>((size.GetZ() / sampleDensity).GetClamp(minSamples, maxSamples))
        };

        float sampleDelta[] =
        {
            size.GetX() / static_cast<float>(numSamples[0] - 1),
            size.GetY() / static_cast<float>(numSamples[1] - 1),
            size.GetZ() / static_cast<float>(numSamples[2] - 1)
        };

        for (auto i = 0; i < numSamples[0]; ++i)
        {
            for (auto j = 0; j < numSamples[1]; ++j)
            {
                for (auto k = 0; k < numSamples[2]; ++k)
                {
                    AZ::Vector3 worldPoint(
                        min.GetX() + i * sampleDelta[0],
                        min.GetY() + j * sampleDelta[1],
                        min.GetZ() + k * sampleDelta[2]
                        );
                    
                    AZ::Vector3 windDir = ForceVolumeComponent::GetWorldForceDirectionAtPoint(m_configuration, worldPoint, volumeAabb.GetCenter(), volumeRotation);
                    windDir.SetLength(AZ::GetSign(m_configuration.m_forceMagnitude));

                    auto isInside = false;
                    ShapeComponentRequestsBus::EventResult(isInside, GetEntityId(), &ShapeComponentRequestsBus::Events::IsPointInside, worldPoint);
                    if (isInside)
                    {
                        displayContext->DrawArrow(worldPoint - windDir, worldPoint + windDir, 1.5f);
                    }
                }
            }
        }
    }
} // namespace LmbrCentral