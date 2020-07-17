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

#include "WhiteBox_precompiled.h"

#include "EditorWhiteBoxComponentMode.h"
#include "SubComponentModes/EditorWhiteBoxDefaultMode.h"
#include "SubComponentModes/EditorWhiteBoxEdgeRestoreMode.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <QApplication> // required for querying modifier keys
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorWhiteBoxComponentMode, AZ::SystemAllocator, 0)

    // helper function to return what modifier keys move us to restore mode
    static bool RestoreModifier(AzToolsFramework::ViewportInteraction::KeyboardModifiers modifiers)
    {
        return modifiers.Shift() && modifiers.Ctrl();
    }

    // helper function to return what type of edge selection mode we're in
    static EdgeSelectionType DecideEdgeSelectionMode(const bool modifierHeld)
    {
        return modifierHeld ? EdgeSelectionType::All : EdgeSelectionType::Polygon;
    }

    EditorWhiteBoxComponentMode::EditorWhiteBoxComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_worldFromLocal(AZ::Transform::Identity())
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorWhiteBoxComponentModeRequestBus::Handler::BusConnect(entityComponentIdPair);
        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorWhiteBoxComponentNotificationBus::Handler::BusConnect(entityComponentIdPair);

        m_worldFromLocal = AzToolsFramework::WorldFromLocalWithUniformScale(entityComponentIdPair.GetEntityId());
        m_modes = AZStd::make_unique<DefaultMode>(entityComponentIdPair); // start with DefaultMode
    }

    EditorWhiteBoxComponentMode::~EditorWhiteBoxComponentMode()
    {
        EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxComponentModeRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    void EditorWhiteBoxComponentMode::Refresh()
    {
        MarkWhiteBoxIntersectionDataDirty();

        AZStd::visit(
            [](auto& mode)
            {
                mode->Refresh();
            },
            m_modes);

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);
    }

    static AZStd::optional<VertexIntersection> FindClosestVertexIntersection(
        const GeometryIntersectionData& whiteBoxIntersectionData, const AZ::Vector3& localRayOrigin,
        const AZ::Vector3& localRayDirection, const AZ::Transform& worldFromLocal,
        const AzFramework::CameraState& cameraState)
    {
        VertexIntersection vertexIntersection;

        const float scaleRecip = AzToolsFramework::ScaleReciprocal(worldFromLocal);

        // find the closest vertex bound
        for (const auto& vertexBound : whiteBoxIntersectionData.m_vertexBounds)
        {
            const AZ::Vector3 worldCenter = worldFromLocal * vertexBound.m_bound.m_center;

            const float screenRadius = vertexBound.m_bound.m_radius *
                AzToolsFramework::CalculateScreenToWorldMultiplier(worldCenter, cameraState) * scaleRecip;

            float vertexDistance = std::numeric_limits<float>::max();
            const bool intersection = IntersectRayVertex(
                vertexBound.m_bound, screenRadius, localRayOrigin, localRayDirection, vertexDistance);

            if (intersection && vertexDistance < vertexIntersection.m_intersection.m_closestDistance)
            {
                vertexIntersection.m_closestVertexWithHandle = vertexBound;
                vertexIntersection.m_intersection.m_closestDistance = vertexDistance;
            }
        }

        if (vertexIntersection.m_intersection.m_closestDistance < std::numeric_limits<float>::max())
        {
            vertexIntersection.m_intersection.m_localIntersectionPoint =
                localRayOrigin + localRayDirection * vertexIntersection.m_intersection.m_closestDistance;

            return vertexIntersection;
        }
        else
        {
            return AZStd::optional<VertexIntersection>{};
        }
    }

    static AZStd::optional<EdgeIntersection> FindClosestEdgeIntersection(
        const GeometryIntersectionData& whiteBoxIntersectionData, const AZ::Vector3& localRayOrigin,
        const AZ::Vector3& localRayDirection, const AZ::Transform& worldFromLocal,
        const AzFramework::CameraState& cameraState)
    {
        EdgeIntersection edgeIntersection;

        const float scaleRecip = AzToolsFramework::ScaleReciprocal(worldFromLocal);

        // find the closest edge bound
        for (const auto& edgeBound : whiteBoxIntersectionData.m_edgeBounds)
        {
            // degenerate edges cause false positives in the intersection test
            if (edgeBound.m_bound.m_start.IsClose(edgeBound.m_bound.m_end))
            {
                continue;
            }

            const AZ::Vector3 localMidpoint = (edgeBound.m_bound.m_end + edgeBound.m_bound.m_start) * 0.5f;
            const AZ::Vector3 worldMidpoint = worldFromLocal * localMidpoint;

            const float screenRadius = edgeBound.m_bound.m_radius *
                AzToolsFramework::CalculateScreenToWorldMultiplier(worldMidpoint, cameraState) * scaleRecip;

            float edgeDistance = std::numeric_limits<float>::max();
            const bool intersection =
                IntersectRayEdge(edgeBound.m_bound, screenRadius, localRayOrigin, localRayDirection, edgeDistance);

            if (intersection && edgeDistance < edgeIntersection.m_intersection.m_closestDistance)
            {
                edgeIntersection.m_closestEdgeWithHandle = edgeBound;
                edgeIntersection.m_intersection.m_closestDistance = edgeDistance;
            }
        }

        if (edgeIntersection.m_intersection.m_closestDistance < std::numeric_limits<float>::max())
        {
            // calculate closest intersection point
            edgeIntersection.m_intersection.m_localIntersectionPoint =
                localRayOrigin + localRayDirection * edgeIntersection.m_intersection.m_closestDistance;

            return edgeIntersection;
        }
        else
        {
            return AZStd::optional<EdgeIntersection>{};
        }
    }

    static AZStd::optional<PolygonIntersection> FindClosestPolygonIntersection(
        const GeometryIntersectionData& whiteBoxIntersectionData, const AZ::Vector3& localRayOrigin,
        const AZ::Vector3& localRayDirection)
    {
        PolygonIntersection polygonIntersection;

        // find closest polygon bound
        for (const auto& polygonBound : whiteBoxIntersectionData.m_polygonBounds)
        {
            float polygonDistance = std::numeric_limits<float>::max();
            const bool intersection =
                IntersectRayPolygon(polygonBound.m_bound, localRayOrigin, localRayDirection, polygonDistance);

            if (intersection && polygonDistance < polygonIntersection.m_intersection.m_closestDistance)
            {
                polygonIntersection.m_closestPolygonWithHandle = polygonBound;
                polygonIntersection.m_intersection.m_closestDistance = polygonDistance;
                polygonIntersection.m_intersection.m_localIntersectionPoint =
                    localRayOrigin + localRayDirection * polygonDistance;
            }
        }

        return polygonIntersection.m_intersection.m_closestDistance < std::numeric_limits<float>::max()
            ? polygonIntersection
            : AZStd::optional<PolygonIntersection>{};
    }

    bool EditorWhiteBoxComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, GetEntityComponentIdPair(), &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // generate mesh to query if it needs to be rebuilt
        if (!m_intersectionAndRenderData.has_value())
        {
            RecalculateWhiteBoxIntersectionData(
                DecideEdgeSelectionMode(RestoreModifier(mouseInteraction.m_mouseInteraction.m_keyboardModifiers)));
        }

        const AZ::Transform localFromWorld = m_worldFromLocal.GetInverseFull();

        const AZ::Vector3 localRayOrigin = localFromWorld * mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin;
        const AZ::Vector3 localRayDirection = AzToolsFramework::TransformDirectionNoScaling(
            localFromWorld, mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection);

        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportId);

        const AZStd::optional<EdgeIntersection> edgeIntersection = FindClosestEdgeIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection,
            m_worldFromLocal, cameraState);

        const AZStd::optional<PolygonIntersection> polygonIntersection = FindClosestPolygonIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection);

        const AZStd::optional<VertexIntersection> vertexIntersection = FindClosestVertexIntersection(
            m_intersectionAndRenderData->m_whiteBoxIntersectionData, localRayOrigin, localRayDirection,
            m_worldFromLocal, cameraState);

        const bool interactionHandled = AZStd::visit(
            [&mouseInteraction, entityComponentIdPair = GetEntityComponentIdPair(), &edgeIntersection,
             &polygonIntersection, &vertexIntersection](auto& mode)
            {
                return mode->HandleMouseInteraction(
                    mouseInteraction, entityComponentIdPair, edgeIntersection, polygonIntersection, vertexIntersection);
            },
            m_modes);

        return interactionHandled;
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorWhiteBoxComponentMode::PopulateActionsImpl()
    {
        return AZStd::visit(
            [entityComponentIdPair = GetEntityComponentIdPair()](auto& mode)
            {
                return mode->PopulateActions(entityComponentIdPair);
            },
            m_modes);
    }

    void EditorWhiteBoxComponentMode::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        namespace ViewportInteraction = AzToolsFramework::ViewportInteraction;
        const auto modifiers = ViewportInteraction::KeyboardModifiers(
            ViewportInteraction::TranslateKeyboardModifiers(QApplication::queryKeyboardModifiers()));

        // handle mode switch
        {
            auto normalMode = AZStd::get_if<AZStd::unique_ptr<DefaultMode>>(&m_modes);
            auto edgeRestoreMode = AZStd::get_if<AZStd::unique_ptr<EdgeRestoreMode>>(&m_modes);

            if (RestoreModifier(modifiers) && normalMode)
            {
                m_modes = AZStd::make_unique<EdgeRestoreMode>();
                m_intersectionAndRenderData = {};
            }
            else if (!RestoreModifier(modifiers) && edgeRestoreMode)
            {
                m_modes = AZStd::make_unique<DefaultMode>(GetEntityComponentIdPair());
                m_intersectionAndRenderData = {};
            }
        }

        // generate mesh to query
        if (!m_intersectionAndRenderData.has_value())
        {
            RecalculateWhiteBoxIntersectionData(DecideEdgeSelectionMode(RestoreModifier(modifiers)));
        }

        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(cl_whiteBoxEdgeUserColor);
        debugDisplay.SetLineWidth(4.0f);

        AZStd::visit(
            [entityComponentIdPair = GetEntityComponentIdPair(),
             &whiteBoxIntersectionAndRenderData = m_intersectionAndRenderData, viewportInfo, &debugDisplay,
             &worldFromLocal = m_worldFromLocal](auto& mode)
            {
                mode->Display(
                    entityComponentIdPair, worldFromLocal, whiteBoxIntersectionAndRenderData.value(), viewportInfo,
                    debugDisplay);
            },
            m_modes);

        debugDisplay.DepthTestOff();
    }

    void EditorWhiteBoxComponentMode::MarkWhiteBoxIntersectionDataDirty()
    {
        m_intersectionAndRenderData = {};
    }

    void EditorWhiteBoxComponentMode::RecalculateWhiteBoxIntersectionData(const EdgeSelectionType edgeSelectionMode)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, GetEntityComponentIdPair(), &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        m_intersectionAndRenderData = IntersectionAndRenderData{};
        for (const auto& vertexHandle : Api::MeshVertexHandles(*whiteBox))
        {
            const auto vertexPosition = Api::VertexPosition(*whiteBox, vertexHandle);
            m_intersectionAndRenderData->m_whiteBoxIntersectionData.m_vertexBounds.emplace_back(
                VertexBoundWithHandle{{vertexPosition, cl_whiteBoxVertexManipulatorSize}, vertexHandle});
        }

        for (const auto& polygonHandle : Api::MeshPolygonHandles(*whiteBox))
        {
            const auto triangles = Api::FacesPositions(*whiteBox, polygonHandle.m_faceHandles);
            m_intersectionAndRenderData->m_whiteBoxIntersectionData.m_polygonBounds.emplace_back(
                PolygonBoundWithHandle{{triangles}, polygonHandle});
        }

        const auto edgeHandles = [whiteBox, edgeSelectionMode]()
        {
            switch (edgeSelectionMode)
            {
            case EdgeSelectionType::Polygon:
                return Api::MeshPolygonEdgeHandles(*whiteBox);
            case EdgeSelectionType::All:
                return Api::MeshEdgeHandles(*whiteBox);
            default:
                return Api::EdgeHandles{};
            }
        }();

        // all edges that are valid to interact with at this time
        for (const auto edgeHandle : edgeHandles)
        {
            const auto edge = Api::EdgeVertexPositions(*whiteBox, edgeHandle);
            m_intersectionAndRenderData->m_whiteBoxIntersectionData.m_edgeBounds.emplace_back(
                EdgeBoundWithHandle{EdgeBound{edge[0], edge[1], cl_whiteBoxEdgeSelectionWidth}, edgeHandle});
        }

        // handle drawing 'user' and 'mesh' edges slightly differently
        const auto edgeHandlesPair = Api::MeshUserEdgeHandles(*whiteBox);
        for (const auto edgeHandle : edgeHandlesPair.m_user)
        {
            const auto edge = Api::EdgeVertexPositions(*whiteBox, edgeHandle);
            m_intersectionAndRenderData->m_whiteBoxEdgeRenderData.m_bounds.m_user.emplace_back(
                EdgeBoundWithHandle{EdgeBound{edge[0], edge[1], cl_whiteBoxEdgeSelectionWidth}, edgeHandle});
        }

        for (const auto edgeHandle : edgeHandlesPair.m_mesh)
        {
            const auto edge = Api::EdgeVertexPositions(*whiteBox, edgeHandle);
            m_intersectionAndRenderData->m_whiteBoxEdgeRenderData.m_bounds.m_mesh.emplace_back(
                EdgeBoundWithHandle{EdgeBound{edge[0], edge[1], cl_whiteBoxEdgeSelectionWidth}, edgeHandle});
        }
    }

    void EditorWhiteBoxComponentMode::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        m_worldFromLocal = AzToolsFramework::TransformUniformScale(world);
    }

    void EditorWhiteBoxComponentMode::OnDefaultShapeTypeChanged([[maybe_unused]] const DefaultShapeType defaultShape)
    {
        // ensure the mode and all modifiers are refreshed
        Refresh();
    }
} // namespace WhiteBox
