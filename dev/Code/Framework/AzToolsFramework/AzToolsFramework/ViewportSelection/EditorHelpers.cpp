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

#include "EditorHelpers.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponentBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorHelpers, AZ::SystemAllocator, 0)

    static const int s_iconSize = 36; // icon display size (in pixels)
    static const float s_iconMinScale = 0.1f; // minimum scale for icons in the distance
    static const float s_iconMaxScale = 1.0f; // maximum scale for icons near the camera
    static const float s_iconCloseDist = 3.f; // distance at which icons are at maximum scale
    static const float s_iconFarDist = 40.f; // distance at which icons are at minimum scale

    // helper function to wrap EBus call to check if helpers are being displayed
    // note: the ['?'] icon in the top right of the editor
    static bool HelpersVisible()
    {
        bool helpersVisible = false;
        EditorRequestBus::BroadcastResult(
            helpersVisible, &EditorRequests::DisplayHelpersVisible);
        return helpersVisible;
    }

    // calculate the icon scale based on how far away it is (distanceSq) from a given point
    // note: this is mostly likely distance from the camera
    static float GetIconScale(const float distSq)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        return s_iconMinScale + (s_iconMaxScale - s_iconMinScale) *
            (1.0f - AZ::GetClamp(AZ::GetMax(0.0f, sqrtf(distSq) - s_iconCloseDist) / s_iconFarDist, 0.0f, 1.0f));
    }

    static void DisplayComponents(
        const AZ::EntityId entityId, const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
        // @deprecated DisplayEntity call
        bool unused;
        AzFramework::EntityDebugDisplayEventBus::Event(
            entityId, &AzFramework::EntityDebugDisplayEvents::DisplayEntity, unused);
        AZ_POP_DISABLE_WARNING

        // preferred DisplayEntity call - DisplayEntityViewport
        AzFramework::EntityDebugDisplayEventBus::Event(
            entityId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            viewportInfo, debugDisplay);
    }

    static AZ::Aabb CalculateComponentAabbs(const int viewportId, const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::EBusReduceResult<AZ::Aabb, AabbAggregator> aabbResult(AZ::Aabb::CreateNull());
        EditorComponentSelectionRequestsBus::EventResult(
            aabbResult, entityId, &EditorComponentSelectionRequests::GetEditorSelectionBoundsViewport,
            AzFramework::ViewportInfo{ viewportId });

        return aabbResult.value;
    }

    AZ::EntityId EditorHelpers::HandleMouseInteraction(
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;

        // set the widget context before calls to ViewportWorldToScreen so we are not
        // going to constantly be pushing/popping the widget context
        ViewportInteraction::WidgetContextGuard widgetContextGuard(viewportId);

        const bool helpersVisible = HelpersVisible();

        // selecting new entities
        AZ::EntityId entityIdUnderCursor;
        AZ::VectorFloat closestDistance = AZ::g_fltMax;
        for (size_t entityCacheIndex = 0; entityCacheIndex < m_entityDataCache->VisibleEntityDataCount(); ++entityCacheIndex)
        {
            const AZ::EntityId entityId = m_entityDataCache->GetVisibleEntityId(entityCacheIndex);

            if (    m_entityDataCache->IsVisibleEntityLocked(entityCacheIndex)
                || !m_entityDataCache->IsVisibleEntityVisible(entityCacheIndex))
            {
                continue;
            }

            // 2d screen space selection - did we click an icon
            if (helpersVisible)
            {
                // some components choose to hide their icons (e.g. meshes)
                if (!m_entityDataCache->IsVisibleEntityIconHidden(entityCacheIndex))
                {
                    const AZ::Vector3& entityPosition = m_entityDataCache->GetVisibleEntityPosition(entityCacheIndex);

                    // selecting based on 2d icon - should only do it when visible and not selected
                    const QPoint screenPosition = GetScreenPosition(viewportId, entityPosition);

                    const AZ::VectorFloat distSqFromCamera = cameraState.m_position.GetDistanceSq(entityPosition);
                    const auto iconRange = static_cast<float>(GetIconScale(distSqFromCamera) * s_iconSize * 0.5f);
                    const auto screenCoords = mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates;

                    if (    screenCoords.m_x >= screenPosition.x() - iconRange
                        &&  screenCoords.m_x <= screenPosition.x() + iconRange
                        &&  screenCoords.m_y >= screenPosition.y() - iconRange
                        &&  screenCoords.m_y <= screenPosition.y() + iconRange)
                    {
                        entityIdUnderCursor = entityId;
                        break;
                    }
                }
            }

            // check if components provide an aabb
            const AZ::Aabb aabb = CalculateComponentAabbs(viewportId, entityId);

            if (aabb.IsValid())
            {
                // coarse grain check
                if (AabbIntersectMouseRay(mouseInteraction.m_mouseInteraction, aabb))
                {
                    // if success, pick against specific component
                    if (PickEntity(
                        entityId, mouseInteraction.m_mouseInteraction,
                        closestDistance, viewportId))
                    {
                        entityIdUnderCursor = entityId;
                    }
                }
            }
        }

        return entityIdUnderCursor;
    }

    void EditorHelpers::DisplayHelpers(
        const AzFramework::ViewportInfo& viewportInfo, const AzFramework::CameraState& cameraState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZStd::function<bool(AZ::EntityId)>& showIconCheck)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (HelpersVisible())
        {
            for (size_t entityCacheIndex = 0; entityCacheIndex < m_entityDataCache->VisibleEntityDataCount(); ++entityCacheIndex)
            {
                const AZ::EntityId entityId = m_entityDataCache->GetVisibleEntityId(entityCacheIndex);

                if (!m_entityDataCache->IsVisibleEntityVisible(entityCacheIndex))
                {
                    continue;
                }

                // notify components to display
                DisplayComponents(entityId, viewportInfo, debugDisplay);

                if (    m_entityDataCache->IsVisibleEntityIconHidden(entityCacheIndex)
                    || (m_entityDataCache->IsVisibleEntitySelected(entityCacheIndex) && !showIconCheck(entityId)))
                {
                    continue;
                }

                int iconTextureId = 0;
                EditorEntityIconComponentRequestBus::EventResult(
                    iconTextureId, entityId, &EditorEntityIconComponentRequests::GetEntityIconTextureId);

                const AZ::Vector3& entityPosition = m_entityDataCache->GetVisibleEntityPosition(entityCacheIndex);
                const AZ::VectorFloat distSqFromCamera = cameraState.m_position.GetDistanceSq(entityPosition);

                const float iconScale = GetIconScale(distSqFromCamera);
                const float iconSize = s_iconSize * iconScale;

                using ComponentEntityAccentType = Components::EditorSelectionAccentSystemComponent::ComponentEntityAccentType;
                const AZ::Color iconHighlight = [this, entityCacheIndex]() {
                    if (m_entityDataCache->IsVisibleEntityLocked(entityCacheIndex))
                    {
                        return AZ::Color(AZ::u8(100), AZ::u8(100), AZ::u8(100), AZ::u8(255));
                    }

                    if (m_entityDataCache->GetVisibleEntityAccent(entityCacheIndex) == ComponentEntityAccentType::Hover)
                    {
                        return AZ::Color(AZ::u8(255), AZ::u8(120), AZ::u8(0), AZ::u8(204));
                    }

                    return AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
                }();

                debugDisplay.SetColor(iconHighlight);
                debugDisplay.DrawTextureLabel(
                    iconTextureId, entityPosition, iconSize, iconSize,
                    /*DisplayContext::ETextureIconFlags::TEXICON_ON_TOP=*/ 0x0008);
            }
        }
    }
} // namespace AzToolsFramework