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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Viewport/ViewportColors.h>

#include "EditorRoadComponent.h"
#include "RoadComponent.h"

namespace RoadsAndRivers
{
    void EditorRoadComponent::Activate()
    {
        Base::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::SplineAttributeNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        m_road.Activate(GetEntityId());
    }

    void EditorRoadComponent::Deactivate()
    {
        Base::Deactivate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        LmbrCentral::SplineAttributeNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_road.Deactivate();
    }

    void EditorRoadComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorRoadComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Road", &EditorRoadComponent::m_road)
                ->Field("TerrainBorderWidth", &EditorRoadComponent::m_terrainBorderWidth)
                ->Field("HeightFieldMap", &EditorRoadComponent::m_heightMapButton)
                ->Field("EraseVegetationWidth", &EditorRoadComponent::m_eraseVegetationWidth)
                ->Field("EraseVegetationVariance", &EditorRoadComponent::m_eraseVegetationVariance)
                ->Field("EraseVegetation", &EditorRoadComponent::m_eraseVegetationButton)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                // EditorForceVolumeComponent
                editContext->Class<EditorRoadComponent>(
                    "Road", "The Road component is used to create roads on the terrain")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Terrain")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Road.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Road.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/road-component")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ProximityTriggerService", 0x561f262c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorRoadComponent::m_road, "Road", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Terrain Editing") 
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRoadComponent::m_terrainBorderWidth, "Border width", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Button, &EditorRoadComponent::m_heightMapButton, "", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRoadComponent::HeightmapButtonClicked)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Align heightmap")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Vegetation Editing")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRoadComponent::m_eraseVegetationWidth, "Erase width", "Distance from road edge to erase vegetation within. Negative values mean vegetation to be left inside the object")
                            ->Attribute(AZ::Edit::Attributes::Min, &EditorRoadComponent::GetMinBorderWidthForTools)
                            ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRoadComponent::m_eraseVegetationVariance, "Erase variance", "Randomization of erase border")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Button, &EditorRoadComponent::m_eraseVegetationButton, "", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRoadComponent::EraseVegetationButtonClicked)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Erase vegetation")
                    ;
            }
        }
    }

    void EditorRoadComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("RoadService"));
    }

    void EditorRoadComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("RoadService"));
    }

    void EditorRoadComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("SplineService"));
        required.push_back(AZ_CRC("TransformService"));
    }

    void EditorRoadComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto roadComponent = gameEntity->CreateComponent<RoadComponent>())
        {
            roadComponent->SetRoad(m_road);
        }
    }

    void EditorRoadComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (IsSelected())
        {
            m_road.DrawGeometry(debugDisplay, AzFramework::ViewportColors::SelectedColor);
        }
        else if (m_accentType == AzToolsFramework::EntityAccentType::Hover)
        {
            m_road.DrawGeometry(debugDisplay, AzFramework::ViewportColors::HoverColor);
        }
    }

    void EditorRoadComponent::HeightmapButtonClicked()
    {
        EditorUtils::AlignHeightmapParams params;
        params.terrainBorderWidth = m_terrainBorderWidth;

        EditorUtils::AlignHeightMap(m_road, params);
    }

    void EditorRoadComponent::EraseVegetationButtonClicked()
    {
        EditorUtils::EraseVegetation(m_road, m_eraseVegetationWidth, m_eraseVegetationVariance);
    }

    float EditorRoadComponent::GetMinBorderWidthForTools()
    {
        return - 0.5f * m_road.GetWidthModifiers().GetMaximumWidth();
    }

    AZ::Aabb EditorRoadComponent::GetEditorSelectionBoundsViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        return m_road.GetAabb();
    }

    bool EditorRoadComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/,
        const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        return m_road.IntersectRay(src, dir, distance);
    }

    void EditorRoadComponent::OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent)
    {
        m_accentType = accent;
    }

    void EditorRoadComponent::OnAttributeAdded(size_t index)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorRoadComponent::OnAttributeRemoved(size_t index)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorRoadComponent::OnAttributesSet(size_t size)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorRoadComponent::OnAttributesCleared()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorRoadComponent::OnEditorSpecChange()
    {
        m_road.Rebuild();
    }
} // namespace RoadsAndRivers
