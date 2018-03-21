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
#include "EditorWindVolumeComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>

namespace LmbrCentral
{
    void EditorWindVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWindVolumeComponent, EditorComponentBase>()
                ->Field("Visible", &EditorWindVolumeComponent::m_visibleInEditor)
                ->Field("Configuration", &EditorWindVolumeComponent::m_configuration)
                ->Version(1)
            ;
            
            if (auto editContext = serializeContext->GetEditContext())
            {
                // EditorWindVolumeComponent
                editContext->Class<EditorWindVolumeComponent>(
                    "Wind Volume", "The wind volume component is used to apply a physical force on objects within the volume")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/WindVolume.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/WindVolume.png")
                    //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c)) Disabled for v1.12
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview) // Hidden for v1.12
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/wind-volume-component")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorWindVolumeComponent::m_visibleInEditor, "Visible", "Always display this component in the editor viewport")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorWindVolumeComponent::m_configuration, "Configuration", "Configuration for wind volume.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWindVolumeComponent::OnConfigurationChanged)
                ;

                // WindVolumeConfiguration
                editContext->Class<WindVolumeConfiguration>(
                    "Wind Volume Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &WindVolumeConfiguration::m_falloff, "Falloff", "Distance from the volume center where the falloff begins. A value of 1 has no effect.")
                    ->Attribute("Suffix", " m")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default , &WindVolumeConfiguration::m_speed, "Speed", "Speed of the wind. The air resistance must be non zero to affect physical objects.")
                    ->Attribute("Suffix", " m/s")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindVolumeConfiguration::m_airResistance, "Air Resistance", "Causes objects to slow down. If zero, it has no effect.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindVolumeConfiguration::m_airDensity, "Air Density", "Objects with lower density will experience a buoyancy force. Objects with higher density will sink If zero, it has no effect.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute("Suffix", " kg/m^3")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindVolumeConfiguration::m_direction, "Direction", "Direction the wind is blowing. If 0 then direction is omnidirectional")
                    ->Attribute("Suffix", " m")
                ;
            }
        }
    }

    void EditorWindVolumeComponent::Activate()
    {
        EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        WindVolume::Activate(GetEntityId());
    }

    void EditorWindVolumeComponent::Deactivate()
    {
        WindVolume::Deactivate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        EditorComponentBase::Deactivate();
    }

    void EditorWindVolumeComponent::OnConfigurationChanged()
    {
        WindVolume::DestroyWindVolume();
        WindVolume::CreateWindVolume();
    }

    void EditorWindVolumeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<WindVolumeComponent>(m_configuration);
    }

    AZ::Vector3 EditorWindVolumeComponent::GetLocalWindDirection(const AZ::Vector3& point) const
    {
        return m_configuration.m_direction.IsZero()
            ? point
            : m_configuration.m_direction;
    }

    void EditorWindVolumeComponent::DisplayEntity(bool& handled)
    {
        if (!IsSelected() && !m_visibleInEditor)
        {
            return;
        }

        handled = true;

        AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "Invalid display context.");

        displayContext->PushMatrix(GetWorldTM());
        displayContext->SetColor(AZ::Vector4(1.f, 0.f, 0.f, 0.7f));

        if (m_isSphere)
        {
            DrawSphere(displayContext);
        }
        else
        {
            DrawBox(displayContext);
        }

        displayContext->PopMatrix();
    }

    void EditorWindVolumeComponent::DrawArrow(AzFramework::EntityDebugDisplayRequests* displayContext, const AZ::Vector3& point)
    {
        AZ::Vector3 windDir = GetLocalWindDirection(point);
        windDir.SetLength(0.8f);
        displayContext->DrawArrow(point - windDir, point + windDir, 1.2f);
    }

    void EditorWindVolumeComponent::DrawBox(AzFramework::EntityDebugDisplayRequests* displayContext)
    {
        const auto minSamples = 2; // Always have a minimum of 2 points in each dimension - 2x2x2 (each corner of a cube)
        const auto maxSamples = 8; // Max out at 8x8x8 samples otherwise the editor will start slowing down.
        const auto sampleDensity = 2.f; // Arbitary scaling factor which will increase samples from min->max relative to size

        auto min = -m_size * 0.5f;

        // Sample count increases with size, but limited to 8*8*8 samples
        int numSamples[] =
        {
            static_cast<int>((m_size.GetX() / sampleDensity).GetClamp(minSamples, maxSamples)),
            static_cast<int>((m_size.GetY() / sampleDensity).GetClamp(minSamples, maxSamples)),
            static_cast<int>((m_size.GetZ() / sampleDensity).GetClamp(minSamples, maxSamples))
        };
        
        float sampleDelta[] =
        {
            m_size.GetX() / static_cast<float>(numSamples[0] - 1),
            m_size.GetY() / static_cast<float>(numSamples[1] - 1),
            m_size.GetZ() / static_cast<float>(numSamples[2] - 1)
        };

        for (auto i = 0; i < numSamples[0]; ++i)
        {
            for (auto j = 0; j < numSamples[1]; ++j)
            {
                for (auto k = 0; k < numSamples[2]; ++k)
                {
                    AZ::Vector3 point(
                        min.GetX() + i * sampleDelta[0],
                        min.GetY() + j * sampleDelta[1],
                        min.GetZ() + k * sampleDelta[2]
                    );

                    DrawArrow(displayContext, point);
                }
            }
        }
    }

    void EditorWindVolumeComponent::DrawSphere(AzFramework::EntityDebugDisplayRequests* displayContext)
    {
        float radius = m_size.GetX();
        int nSamples = radius * 5;
        nSamples = AZ::GetClamp(nSamples, 5, 512);

        // Draw arrows using Fibonacci sphere
        float offset = 2.f / nSamples;
        float increment = AZ::Constants::Pi * (3.f - sqrt(5.f));
        for(int i = 0; i < nSamples; ++i)
        {
            float phi = ((i + 1) % nSamples) * increment;
            float y = ((i * offset) - 1) + (offset / 2.f);
            float r = sqrt(1 - pow(y, 2));
            float x = cos(phi) * r;
            float z = sin(phi) * r;
            DrawArrow(displayContext, AZ::Vector3(x * radius, y * radius, z * radius));
        }
    }
} // namespace LmbrCentral