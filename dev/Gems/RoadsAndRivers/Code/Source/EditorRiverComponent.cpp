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

#include "EditorRiverComponent.h"
#include "RiverComponent.h"

namespace RoadsAndRivers
{
    void EditorRiverComponent::Activate()
    {
        Base::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::SplineAttributeNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        m_river.Activate(GetEntityId());
    }

    void EditorRiverComponent::Deactivate()
    {
        Base::Deactivate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        LmbrCentral::SplineAttributeNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_river.Deactivate();
    }

    void EditorRiverComponent::Reflect(AZ::ReflectContext * context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorRiverComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("River", &EditorRiverComponent::m_river)
                ->Field("TerrainBorderWidth", &EditorRiverComponent::m_terrainBorderWidth)
                ->Field("Embankment", &EditorRiverComponent::m_embankment)
                ->Field("RiverBedDepth", &EditorRiverComponent::m_riverBedDepth)
                ->Field("RiverBedWidthOffset", &EditorRiverComponent::m_riverBedWidthOffset)
                ->Field("HeightFieldMap", &EditorRiverComponent::m_heightMapButton)
                ->Field("EraseVegetationWidth", &EditorRiverComponent::m_eraseVegetationWidth)
                ->Field("EraseVegetationVariance", &EditorRiverComponent::m_eraseVegetationVariance)
                ->Field("EraseVegetation", &EditorRiverComponent::m_eraseVegetationButton)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorRiverComponent>(
                    "River", "The River component is used to create rivers on the terrain")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Terrain")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/River.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/River.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/river-component")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ProximityTriggerService", 0x561f262c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorRiverComponent::m_river, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                     ->ClassElement(AZ::Edit::ClassElements::Group, "Terrain Editing") 
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRiverComponent::m_terrainBorderWidth, "Border width", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRiverComponent::m_embankment, "Embankment height", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRiverComponent::m_riverBedDepth, "Depth of the river bed", "")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRiverComponent::m_riverBedWidthOffset, "River bed width offset", "How narrower or wider is river bed comparing to the width of the river")
                            ->Attribute(AZ::Edit::Attributes::Min, &EditorRiverComponent::GetMinBorderWidthForTools)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Button, &EditorRiverComponent::m_heightMapButton, "", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRiverComponent::HeightmapButtonClicked)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Carve River Bed")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Vegetation Editing")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRiverComponent::m_eraseVegetationWidth, "Erase width", "Distance from road edge to erase vegetation within. Negative values mean vegetation to be left inside the object")
                            ->Attribute(AZ::Edit::Attributes::Min, &EditorRiverComponent::GetMinBorderWidthForTools)
                            ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorRiverComponent::m_eraseVegetationVariance, "Erase variance", "Randomization of erase border")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(AZ::Edit::UIHandlers::Button, &EditorRiverComponent::m_eraseVegetationButton, "", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRiverComponent::EraseVegetationButtonClicked)
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Erase vegetation")
                    ;
                    ;
            }
        }
    }

    void EditorRiverComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("RiverService"));
    }

    void EditorRiverComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("RiverService"));
    }

    void EditorRiverComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("SplineService"));
        required.push_back(AZ_CRC("TransformService"));
    }

    void EditorRiverComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto riverComponent = gameEntity->CreateComponent<RiverComponent>())
        {
            riverComponent->SetRiver(m_river);
        }
    }

    void EditorRiverComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (IsSelected())
        {
            m_river.DrawGeometry(debugDisplay, AzFramework::ViewportColors::SelectedColor);
        }
        else if (m_accentType == AzToolsFramework::EntityAccentType::Hover)
        {
            m_river.DrawGeometry(debugDisplay, AzFramework::ViewportColors::HoverColor);
        }
    }

    void EditorRiverComponent::HeightmapButtonClicked()
    {
        EditorUtils::AlignHeightmapParams params;
        params.terrainBorderWidth = m_terrainBorderWidth;
        params.embankment = m_embankment;
        params.riverBedDepth = m_riverBedDepth;
        params.riverBedWidth = m_riverBedWidthOffset;
        params.dontExceedSplineLength = true;

        EditorUtils::AlignHeightMap(m_river, params);
    }

    void EditorRiverComponent::EraseVegetationButtonClicked()
    {
        EditorUtils::EraseVegetation(m_river, m_eraseVegetationWidth, m_eraseVegetationVariance);
    }

    float EditorRiverComponent::GetMinBorderWidthForTools()
    {
        return -0.5f * m_river.GetWidthModifiers().GetMaximumWidth();
    }

    AZ::Aabb EditorRiverComponent::GetEditorSelectionBoundsViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        return m_river.GetAabb();
    }

    bool EditorRiverComponent::EditorSelectionIntersectRayViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/,
        const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        return m_river.IntersectRay(src, dir, distance);
    }

    void EditorRiverComponent::OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent)
    {
        m_accentType = accent;
    }

    void EditorRiverComponent::OnAttributeAdded(size_t index)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorRiverComponent::OnAttributeRemoved(size_t index)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorRiverComponent::OnAttributesSet(size_t size)
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorRiverComponent::OnAttributesCleared()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EditorRiverComponent::OnEditorSpecChange()
    {
        m_river.Rebuild();
    }
} // namespace RoadsAndRivers
