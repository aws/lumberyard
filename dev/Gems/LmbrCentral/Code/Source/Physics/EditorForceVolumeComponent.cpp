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
#include "ForceVolume.h"
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorForceVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorForceVolumeComponent, EditorComponentBase>()
                ->Field("Visible", &EditorForceVolumeComponent::m_visibleInEditor)
                ->Field("DebugForces", &EditorForceVolumeComponent::m_debugForces)
                ->Field("ForceVolume", &EditorForceVolumeComponent::m_forceVolume)
                ->Version(1)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                // EditorForceVolumeComponent
                editContext->Class<EditorForceVolumeComponent>(
                    "Force Volume", "The force volume component is used to apply a physical force on objects within the volume")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Physics (Legacy)")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/ForceVolume.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/ForceVolume.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/force-volume-component")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ProximityTriggerService", 0x561f262c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorForceVolumeComponent::m_visibleInEditor, "Visible", "Always display this component in the editor viewport")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorForceVolumeComponent::m_debugForces, "Debug Forces", "Draw debug lines for forces in game")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorForceVolumeComponent::m_forceVolume, "ForceVolume", "Configuration for force volume.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    void EditorForceVolumeComponent::Activate()
    {
        EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        m_forceVolume.Activate(GetEntityId());
    }

    void EditorForceVolumeComponent::Deactivate()
    {
        m_forceVolume.Deactivate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        EditorComponentBase::Deactivate();
    }

    void EditorForceVolumeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<ForceVolumeComponent>(m_forceVolume, m_debugForces);
    }

    void EditorForceVolumeComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!IsSelected() && !m_visibleInEditor)
        {
            return;
        }

        debugDisplay.SetColor(AZ::Vector4(1.f, 0.f, 0.f, 1.0f));

        m_forceVolume.Display(debugDisplay);
    }

} // namespace LmbrCentral