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

#include "EditorRiverSurfaceDataComponent.h"

#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace RoadsAndRivers
{
    void EditorRiverSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorRiverSurfaceDataComponent, BaseClassType>()
                ->Version(2, LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType,2>)
                ->Field("ViewSurfaceVolume", &EditorRiverSurfaceDataComponent::m_viewSurfaceVolume)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorRiverSurfaceDataComponent>(
                    s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorRiverSurfaceDataComponent::m_viewSurfaceVolume, "View Emitted River Volume", "Enable/Disable visualization of the river volume used for modifying surface tags")
                    ;
            }
        }
    }

    void EditorRiverSurfaceDataComponent::Activate()
    {
        BaseClassType::Activate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorRiverSurfaceDataComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        
        BaseClassType::Deactivate();
    }

    void EditorRiverSurfaceDataComponent::DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // use a translucent blue for visualizing the river surface data volume
        const AZ::Color riverVolumeColor(0.0f, 0.0f, 1.0f, 0.5f);

        if (IsSelected() && m_viewSurfaceVolume)
        {
            if (m_component.m_surfaceShapeBoundsIsValid)
            {
                AZ::EntityId entityId = GetEntityId();

                AZ::Vector3 depth(0.0f, 0.0f, -m_component.m_riverDepth);
                const size_t vertexCount = m_component.m_shapeVertices.size();

                // All our vertices are already transformed into world space, so make sure the displayContext doesn't try to transform them as well.
                AZ::Transform transform = AZ::Transform::CreateIdentity();
                debugDisplay.PushMatrix(transform);
                debugDisplay.SetColor(riverVolumeColor);

                // Draw our quads, assuming we have a valid quad list.
                // All of the draw calls below use a deliberate winding order.  We're drawing translucent
                // quads for our debug volume, so we need to make sure they face outwards.
                if ((vertexCount >= 4) && (vertexCount % 4 == 0))
                {
                    for (size_t vertex = 0; vertex < vertexCount; vertex += 4)
                    {
                        const AZ::Vector3 &topLeft = m_component.m_shapeVertices[vertex + 0];
                        const AZ::Vector3 &topRight = m_component.m_shapeVertices[vertex + 1];
                        const AZ::Vector3 &bottomLeft = m_component.m_shapeVertices[vertex + 2];
                        const AZ::Vector3 &bottomRight = m_component.m_shapeVertices[vertex + 3];

                        // Draw the river surface (top) and a bottom extruded to the correct depth
                        debugDisplay.DrawQuad(topLeft, topRight, bottomRight, bottomLeft);
                        debugDisplay.DrawQuad(topLeft + depth, bottomLeft + depth, bottomRight + depth, topRight + depth);

                        // Draw the river volume "sides"
                        debugDisplay.DrawQuad(topLeft, bottomLeft, bottomLeft + depth, topLeft + depth);
                        debugDisplay.DrawQuad(bottomRight, topRight, topRight + depth, bottomRight + depth);
                    }

                    // Draw the "front" of the river volume (uses the first two vertices and extrudes to depth).
                    debugDisplay.DrawQuad(m_component.m_shapeVertices[1],
                                             m_component.m_shapeVertices[0],
                                             m_component.m_shapeVertices[0] + depth,
                                             m_component.m_shapeVertices[1] + depth);

                    // Draw the "back" of the river volume (uses the last two vertices and extrudes to depth)
                    debugDisplay.DrawQuad(m_component.m_shapeVertices[vertexCount - 2],
                                             m_component.m_shapeVertices[vertexCount - 1],
                                             m_component.m_shapeVertices[vertexCount - 1] + depth,
                                             m_component.m_shapeVertices[vertexCount - 2] + depth);
                }

                debugDisplay.PopMatrix();
            }
        }
    }
}