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

#include "EditorWhiteBoxComponentModeBus.h"
#include "EditorWhiteBoxEdgeModifierBus.h"
#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "Util/WhiteBoxMathUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "WhiteBoxEdgeTranslationModifier.h"
#include "WhiteBoxManipulatorViews.h"

#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(EdgeTranslationModifier, AZ::SystemAllocator, 0)

    static bool BeginningExtrude(
        const AzToolsFramework::PlanarManipulator::Action& action, const AppendStage appendStage)
    {
        return action.m_modifiers.Ctrl() && appendStage == AppendStage::None;
    }

    static bool EndingExtrude(const AzToolsFramework::PlanarManipulator::Action& action, const AppendStage appendStage)
    {
        return !action.m_modifiers.Ctrl() && appendStage != AppendStage::None;
    }

    static bool AppendInactive(const AppendStage appendStage)
    {
        return appendStage == AppendStage::None || appendStage == AppendStage::Complete;
    }

    AZStd::array<AZ::Vector3, 2> GetEdgeNormalAxes(const AZ::Vector3& start, const AZ::Vector3& end)
    {
        AZ::Vector3 axis1, axis2;
        CalculateOrthonormalBasis((start - end).GetNormalizedExact(), axis1, axis2);
        return {axis1, axis2};
    }

    EdgeTranslationModifier::EdgeTranslationModifier(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const Api::EdgeHandle edgeHandle,
        const AZ::Vector3& intersectionPoint)
        : m_hoveredEdgeHandle{edgeHandle}
        , m_entityComponentIdPair(entityComponentIdPair)
        , m_intersectionPoint(intersectionPoint)
    {
        CreateManipulator();
    }

    EdgeTranslationModifier::~EdgeTranslationModifier()
    {
        DestroyManipulator();
    }

    // return all vertex handles from a collection of edge handles
    // (ensure to remove duplicates as vertices will be shared across edges)
    static Api::VertexHandles VertexHandlesForEdges(const WhiteBoxMesh& whiteBox, const Api::EdgeHandles& edgeHandles)
    {
        auto vertexHandles = std::reduce(
            edgeHandles.cbegin(), edgeHandles.cend(), Api::VertexHandles{},
            [&whiteBox](Api::VertexHandles vertexHandles, const Api::EdgeHandle edgeHandle)
            {
                const auto edgeVertexHandles = Api::EdgeVertexHandles(whiteBox, edgeHandle);
                vertexHandles.push_back(edgeVertexHandles[0]);
                vertexHandles.push_back(edgeVertexHandles[1]);
                return vertexHandles;
            });

        std::sort(vertexHandles.begin(), vertexHandles.end());
        vertexHandles.erase(std::unique(vertexHandles.begin(), vertexHandles.end()), vertexHandles.end());

        return vertexHandles;
    }

    // note: edgeHandles is an inout param and will be modified if its size is 1
    static Api::EdgeHandle AttemptEdgeAppend(
        WhiteBoxMesh& whiteBox, Api::EdgeHandle hoveredEdgeHandle, Api::EdgeHandles& edgeHandles,
        const AZ::Vector3& extrudeVector)
    {
        // only allow edge extrusion with a single edge
        if (edgeHandles.size() == 1)
        {
            edgeHandles = {Api::TranslateEdgeAppend(whiteBox, hoveredEdgeHandle, extrudeVector)};
            return edgeHandles.front();
        }

        // no append occurred, return original edge handle
        return hoveredEdgeHandle;
    }

    void EdgeTranslationModifier::CreateManipulator()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // calculate edge handle group (will be > 1 if connecting vertices have been hidden)
        m_edgeHandles = Api::EdgeGrouping(*whiteBox, m_hoveredEdgeHandle);

        const auto vertexPositions = Api::EdgeVertexPositions(*whiteBox, m_hoveredEdgeHandle);
        const auto axes = GetEdgeNormalAxes(vertexPositions[0], vertexPositions[1]);

        m_translationManipulator = AzToolsFramework::PlanarManipulator::MakeShared(
            AzToolsFramework::WorldFromLocalWithUniformScale(m_entityComponentIdPair.GetEntityId()));

        m_translationManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_translationManipulator->SetLocalPosition(m_intersectionPoint);
        m_translationManipulator->SetAxes(axes[0], axes[1]);

        CreateView(m_intersectionPoint);

        m_translationManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);

        struct SharedState
        {
            // the previous position when moving the manipulator, used to calculate manipulator delta position
            AZ::Vector3 m_prevPosition;
            // the distance from where the user clicked to the midpoint of the edge being moved
            AZ::Vector3 m_midpointToIntersection;
            // the position of the manipulator the moment an append is initiated
            AZ::Vector3 m_initiateAppendPosition = AZ::Vector3::CreateZero();
            // the distance the manipulator has moved from where it started when an append begins
            AZ::Vector3 m_activeAppendOffset = AZ::Vector3::CreateZero();
            // what state of appending are we currently in
            AppendStage m_appendStage = AppendStage::None;
            // has the modifier moved during the action
            bool m_moved = false;
        };

        auto sharedState = AZStd::make_shared<SharedState>();

        m_translationManipulator->InstallLeftMouseDownCallback(
            [this, sharedState](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                // record initial state at mouse down
                sharedState->m_prevPosition = action.LocalPosition();
                sharedState->m_midpointToIntersection =
                    m_intersectionPoint - Api::EdgeMidpoint(*whiteBox, m_hoveredEdgeHandle);
                sharedState->m_appendStage = AppendStage::None;
            });

        m_translationManipulator->InstallMouseMoveCallback(
            [this, sharedState](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                // has the modifier moved during this interaction
                sharedState->m_moved = sharedState->m_moved ||
                    action.LocalPositionOffset().GetLength() >= cl_whiteBoxMouseClickDeltaThreshold;

                // reset append
                if (EndingExtrude(action, sharedState->m_appendStage))
                {
                    sharedState->m_appendStage = AppendStage::None;
                }

                // start trying to extrude
                if (BeginningExtrude(action, sharedState->m_appendStage))
                {
                    sharedState->m_appendStage = AppendStage::Initiated;
                    sharedState->m_initiateAppendPosition = action.LocalPosition();
                }

                const AZ::Vector3 position = action.LocalPosition();
                if (sharedState->m_appendStage == AppendStage::Initiated)
                {
                    const AZ::Vector3 extrudeVector = action.LocalPosition() - sharedState->m_initiateAppendPosition;
                    const AZ::VectorFloat extrudeMagnitude = extrudeVector.Dot(action.m_fixed.m_axis1).GetAbs() +
                        extrudeVector.Dot(action.m_fixed.m_axis2).GetAbs();

                    // only extrude after having moved a small amount (to prevent overlapping verts
                    // and normals being calculated incorrectly)
                    if (extrudeMagnitude.IsGreaterThan(AZ::VectorFloat::CreateZero()))
                    {
                        sharedState->m_activeAppendOffset = action.LocalPositionOffset();

                        const Api::EdgeHandle nextEdgeHandle =
                            AttemptEdgeAppend(*whiteBox, m_hoveredEdgeHandle, m_edgeHandles, extrudeVector);

                        m_intersectionPoint =
                            Api::EdgeMidpoint(*whiteBox, nextEdgeHandle) + sharedState->m_midpointToIntersection;
                        sharedState->m_appendStage = AppendStage::Complete;

                        EditorWhiteBoxEdgeModifierNotificationBus::Broadcast(
                            &EditorWhiteBoxEdgeModifierNotificationBus::Events::OnEdgeModifierUpdatedEdgeHandle,
                            m_hoveredEdgeHandle, nextEdgeHandle);

                        m_hoveredEdgeHandle = nextEdgeHandle;
                    }
                }
                else if (AppendInactive(sharedState->m_appendStage))
                {
                    // get the distance the manipulator has moved since the last mouse move
                    const AZ::Vector3 displacement = position - sharedState->m_prevPosition;

                    // have to make sure we don't move verts more than once
                    for (const auto vertexHandle : VertexHandlesForEdges(*whiteBox, m_edgeHandles))
                    {
                        SetVertexPosition(
                            *whiteBox, vertexHandle, VertexPosition(*whiteBox, vertexHandle) + displacement);
                    }
                }

                sharedState->m_prevPosition = position;

                // regular movement/translation of vertices
                if (AppendInactive(sharedState->m_appendStage))
                {
                    m_translationManipulator->SetLocalPosition(
                        m_intersectionPoint + action.LocalPositionOffset() - sharedState->m_activeAppendOffset);

                    EditorWhiteBoxComponentModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonScaleModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeScaleModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonTranslationModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeTranslationModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshVertexSelectionModifier);
                }

                Api::CalculateNormals(*whiteBox);
                Api::CalculatePlanarUVs(*whiteBox);

                EditorWhiteBoxComponentNotificationBus::Event(
                    m_entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
            });

        m_translationManipulator->InstallLeftMouseUpCallback(
            [this, sharedState](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                // we haven't moved, count as a click
                if (!sharedState->m_moved)
                {
                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair, &EditorWhiteBoxDefaultModeRequestBus::Events::CreateEdgeScaleModifier,
                        m_hoveredEdgeHandle);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::AssignSelectedEdgeTranslationModifier);
                }
                else
                {
                    EditorWhiteBoxComponentRequestBus::Event(
                        m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
                }
            });
    }

    void EdgeTranslationModifier::DestroyManipulator()
    {
        m_translationManipulator->Unregister();
        m_translationManipulator.reset();
    }

    bool EdgeTranslationModifier::MouseOver() const
    {
        return m_translationManipulator->MouseOver();
    }

    void EdgeTranslationModifier::ForwardMouseOverEvent(
        const AzToolsFramework::ViewportInteraction::MouseInteraction& interaction)
    {
        m_translationManipulator->ForwardMouseOverEvent(interaction);
    }

    void EdgeTranslationModifier::Refresh()
    {
        DestroyManipulator();
        CreateManipulator();
    }

    void EdgeTranslationModifier::UpdateIntersectionPoint(const AZ::Vector3& intersectionPoint)
    {
        m_intersectionPoint = intersectionPoint;
        // set the translation modifier to be the exact location of the intersection
        m_translationManipulator->SetLocalPosition(m_intersectionPoint);

        CreateView(intersectionPoint);
    }

    void EdgeTranslationModifier::CreateView(const AZ::Vector3& intersectionPoint)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // if the size of the edge handles and views has changed
        // we know we need to either add or remove views
        if (m_edgeViews.size() != m_edgeHandles.size())
        {
            m_edgeViews.resize(m_edgeHandles.size());

            AZStd::generate(
                AZStd::begin(m_edgeViews), AZStd::end(m_edgeViews),
                []
                {
                    return AZStd::make_shared<ManipulatorViewEdge>();
                });
        }

        for (size_t edgeIndex = 0; edgeIndex < m_edgeHandles.size(); ++edgeIndex)
        {
            auto view = m_edgeViews[edgeIndex];
            const auto edgeHandle = m_edgeHandles[edgeIndex];

            // vertex positions in the local space of the entity
            const auto vertexPositions = Api::EdgeVertexPositions(*whiteBox, edgeHandle);

            // transform edge start/end positions to be in manipulator space (see UpdateIntersectionPoint)
            // (relative to m_translationManipulator local position)
            view->m_start = vertexPositions[0] - intersectionPoint;
            view->m_end = vertexPositions[1] - intersectionPoint;

            // only do selection colors for 'selected/hovered' edge handle
            if (edgeHandle == m_hoveredEdgeHandle)
            {
                view->SetColor(m_color, m_hoverColor);
            }
            else
            {
                view->SetColor(cl_whiteBoxEdgeHoveredColor, cl_whiteBoxEdgeHoveredColor);
            }

            view->SetWidth(m_width, m_hoverWidth);
        }

        m_translationManipulator->SetViews(AzToolsFramework::ManipulatorViews
                                           {m_edgeViews.cbegin(), m_edgeViews.cend()});
    }

    void EdgeTranslationModifier::SetColors(const AZ::Color& color, const AZ::Color& hoverColor)
    {
        m_color = color;
        m_hoverColor = hoverColor;
    }

    void EdgeTranslationModifier::SetWidths(const float width, const float hoverWidth)
    {
        m_width = width;
        m_hoverWidth = hoverWidth;
    }

    bool EdgeTranslationModifier::PerformingAction() const
    {
        return m_translationManipulator->PerformingAction();
    }
} // namespace WhiteBox
