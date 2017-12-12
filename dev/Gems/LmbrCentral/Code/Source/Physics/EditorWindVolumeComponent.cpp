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

#include "StdAfx.h"
#include "EditorWindVolumeComponent.h"
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorWindVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWindVolumeComponent, EditorComponentBase>()
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
                    //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c)) Disabled for v1.11
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorWindVolumeComponent::m_configuration, "Configuration", "Configuration for wind volume.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;

                // WindVolumeConfiguration
                editContext->Class<WindVolumeConfiguration>(
                    "Wind Volume Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(0, &WindVolumeConfiguration::m_ellipsoidal, "Ellipsoidal", "Forces an ellipsoidal falloff")
                    ->DataElement(0, &WindVolumeConfiguration::m_falloffInner, "Falloff Inner", "Distance after which the distance-based falloff begins")
                        ->Attribute("Suffix", " m")
                    ->DataElement(0, &WindVolumeConfiguration::m_speed, "Speed", "Strength of the wind")
                        ->Attribute("Suffix", " m/s")
                    ->DataElement(0, &WindVolumeConfiguration::m_airResistance, "Air Resistance", "Causes very light physicalised objects to experience a buoyancy force, if >0")
                    ->DataElement(0, &WindVolumeConfiguration::m_airDensity, "Air Density", "Causes physicalised objects moving through the air to slow down, if >0")
                        ->Attribute("Suffix", " kg/m^3")
                    ->DataElement(0, &WindVolumeConfiguration::m_direction, "Direction", "Direction the wind is blowing. If 0 then direction is omnidirectional")
                    ->DataElement(0, &WindVolumeConfiguration::m_size, "Size", "Size of the wind volume")
                        ->Attribute("Suffix", " m")
                ;
            }
        }
    }

    void EditorWindVolumeComponent::Activate()
    {
        EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorWindVolumeComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    EditorWindVolumeComponent::EditorWindVolumeComponent(const WindVolumeConfiguration& configuration)
        : m_configuration(configuration)
    {
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
        if (!IsSelected())
        {
            return;
        }

        handled = true;

        AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "Invalid display context.");

        displayContext->PushMatrix(GetWorldTM());
        displayContext->SetColor(AZ::Vector4(1.f, 0.f, 0.f, 0.7f));

        AZ::Vector3 size = m_configuration.m_size;
        if (m_configuration.m_ellipsoidal)
        {            
            float radius = WindVolumeComponent::GetSphereVolumeRadius(m_configuration);
            size = AZ::Vector3(radius, radius, radius);
        }
        const int minSamples = 2; // Always have a minimum of 2 points in each dimension - 2x2x2 (each corner of a cube)
        const int maxSamples = 8; // Max out at 8x8x8 samples otherwise the editor will start slowing down.
        const float sampleDensity = 2.5f; // Arbitary scaling factor which will increase samples from min->max relative to size
        AZ::Vector3 min = -size;
        AZ::Vector3 max = size;

        // Sample count increases with size, but limited to 8*8*8 samples
        int numSamples[] =
        {
            static_cast<int>((size.GetX() / sampleDensity).GetClamp(minSamples, maxSamples)),
            static_cast<int>((size.GetY() / sampleDensity).GetClamp(minSamples, maxSamples)),
            static_cast<int>((size.GetZ() / sampleDensity).GetClamp(minSamples, maxSamples))
        };
        
        float sampleDelta[] =
        {
            (size.GetX() * 2.f) / static_cast<float>(numSamples[0] - 1),
            (size.GetY() * 2.f) / static_cast<float>(numSamples[1] - 1),
            (size.GetZ() * 2.f) / static_cast<float>(numSamples[2] - 1)
        };

        for (auto i = 0; i < numSamples[0]; ++i)
        {
            for (auto j = 0; j < numSamples[1]; ++j)
            {
                for (auto k = 0; k < numSamples[2]; ++k)
                {
                    AZ::Vector3 point(
                        min.GetX() + i* sampleDelta[0],
                        min.GetY() + j* sampleDelta[1],
                        min.GetZ() + k* sampleDelta[2]
                        );

                    AZ::Vector3 windDir = GetLocalWindDirection(point);
                    windDir.SetLength(0.5f);

                    displayContext->DrawArrow(point - windDir, point + windDir, 1.f);
                }
            }
        }

        displayContext->SetColor(AZ::Vector4(0.f, 1.f, 0.f, 0.7f));
        displayContext->DrawWireBox(min, max);

        displayContext->PopMatrix();
    }
} // namespace LmbrCentral