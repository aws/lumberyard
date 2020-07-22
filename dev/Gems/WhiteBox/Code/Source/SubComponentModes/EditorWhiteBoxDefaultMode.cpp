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

#include "EditorWhiteBoxDefaultMode.h"
#include "Viewport/WhiteBoxEdgeScaleModifier.h"
#include "Viewport/WhiteBoxEdgeTranslationModifier.h"
#include "Viewport/WhiteBoxPolygonScaleModifier.h"
#include "Viewport/WhiteBoxPolygonTranslationModifier.h"
#include "Viewport/WhiteBoxVertexSelectionModifier.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <QKeySequence>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    static const AZ::Crc32 HideEdge = AZ_CRC("com.amazon.action.whitebox.hide_edge", 0x6a60ae23);
    static const AZ::Crc32 HideVertex = AZ_CRC("com.amazon.action.whitebox.hide_vertex", 0x4a4bd092);

    static const char* const HideEdgeTitle = "Hide Edge";
    static const char* const HideEdgeDesc = "Hide the selected edge to merge the two connected polygons";
    static const char* const HideVertexTitle = "Hide Vertex";
    static const char* const HideVertexDesc = "Hide the selected vertex to merge the two connected edges";

    static const char* const HideEdgeUndoRedoDesc = "Hide an edge to merge two connected polygons together";
    static const char* const HideVertexUndoRedoDesc = "Hide a vertex to merge two connected edges together";

    const QKeySequence HideKey = QKeySequence{Qt::Key_H};

    // handle translation and scale modifiers for either polygon or edge - if a translation
    // modifier is set (either polygon or edge), update the intersection point so the next time
    // mouse down happens the delta offsets of the manipulator are calculated from the correct
    // position and also handle clicking off of a selected modifier to clear the selected state
    // note: the Intersection type must match the geometry for the translation and scale modifier
    template<typename TranslationModifier, typename Intersection, typename DestroyOtherModifierFn>
    static void HandleMouseInteractionForModifier(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        DefaultMode::SelectedTranslationModifier& selectedTranslationModifier,
        DestroyOtherModifierFn&& destroyOtherModifierFn, const AZStd::optional<Intersection>& geometryIntersection)
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<TranslationModifier>>(&selectedTranslationModifier))
        {
            if (geometryIntersection.has_value())
            {
                (*modifier)->UpdateIntersectionPoint(geometryIntersection->m_intersection.m_localIntersectionPoint);
            }

            // handle clicking off of selected geometry
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up &&
                !geometryIntersection.has_value())
            {
                selectedTranslationModifier = AZStd::monostate{};
                destroyOtherModifierFn();

                AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                    &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);
            }
        }
    }

    // in this generic context TranslationModifier and OtherTranslationModifier correspond
    // to Polygon/Edge or Edge/Polygon depending on the context (which was intersected)
    template<typename Intersection, typename TranslationModifier, typename DestroyOtherModifierFn>
    static void HandleCreatingDestroyingModifiersOnIntersectionChange(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        DefaultMode::SelectedTranslationModifier& selectedTranslationModifier,
        AZStd::unique_ptr<TranslationModifier>& translationModifier, DestroyOtherModifierFn&& destroyOtherModifierFn,
        const AZStd::optional<Intersection>& geometryIntersection,
        const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        // if we have a valid mouse ray intersection with the geometry (e.g. edge or polygon)
        if (geometryIntersection.has_value())
        {
            // update intersection point of existing hovered translation
            // modifier (does not have to be selected)
            if (translationModifier)
            {
                translationModifier->UpdateIntersectionPoint(
                    geometryIntersection->m_intersection.m_localIntersectionPoint);
            }

            // does the geometry the mouse is hovering over match the currently selected geometry
            const bool matchesSelected = [&geometryIntersection, &selectedTranslationModifier]()
            {
                if (auto modifier = AZStd::get_if<AZStd::unique_ptr<TranslationModifier>>(&selectedTranslationModifier))
                {
                    return (*modifier)->GetHandle() == geometryIntersection->GetHandle();
                }
                return false;
            }();

            // check if there's currently no modifier or if we need to make a different modifier as
            // the geometry is different
            const bool shouldCreateTranslationModifier =
                !translationModifier || (translationModifier->GetHandle() != geometryIntersection->GetHandle());

            if (shouldCreateTranslationModifier && !matchesSelected)
            {
                // created a modifier for the intersected geometry
                translationModifier = AZStd::make_unique<TranslationModifier>(
                    entityComponentIdPair, geometryIntersection->GetHandle(),
                    geometryIntersection->m_intersection.m_localIntersectionPoint);

                translationModifier->ForwardMouseOverEvent(mouseInteraction.m_mouseInteraction);

                destroyOtherModifierFn();
            }
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(DefaultMode, AZ::SystemAllocator, 0)

    DefaultMode::DefaultMode(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
    {
        EditorWhiteBoxDefaultModeRequestBus::Handler::BusConnect(entityComponentIdPair);
        EditorWhiteBoxPolygonModifierNotificationBus::Handler::BusConnect(entityComponentIdPair);
        EditorWhiteBoxEdgeModifierNotificationBus::Handler::BusConnect(entityComponentIdPair);
    }

    DefaultMode::~DefaultMode()
    {
        EditorWhiteBoxEdgeModifierNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxPolygonModifierNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxDefaultModeRequestBus::Handler::BusDisconnect();
    }

    void DefaultMode::Refresh()
    {
        // destroy all active modifiers
        m_polygonScaleModifier.reset();
        m_edgeScaleModifier.reset();
        m_polygonTranslationModifier.reset();
        m_edgeTranslationModifier.reset();
        m_selectedModifier = AZStd::monostate{};
    }

    AZStd::vector<AzToolsFramework::ActionOverride> DefaultMode::PopulateActions(
        const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        using ComponentModeRequestBus = AzToolsFramework::ComponentModeFramework::ComponentModeRequestBus;

        // edge selection test - ensure an edge is selected before allowing this shortcut
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            return AZStd::vector<AzToolsFramework::ActionOverride>{
                AzToolsFramework::ActionOverride()
                    .SetUri(HideEdge)
                    .SetKeySequence(HideKey)
                    .SetTitle(HideEdgeTitle)
                    .SetTip(HideEdgeDesc)
                    .SetEntityComponentIdPair(entityComponentIdPair)
                    .SetCallback(
                        [this, entityComponentIdPair, modifier]()
                        {
                            WhiteBoxMesh* whiteBox = nullptr;
                            EditorWhiteBoxComponentRequestBus::EventResult(
                                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                            Api::HideEdge(*whiteBox, (*modifier)->GetEdgeHandle());
                            (*modifier)->SetEdgeHandle(Api::EdgeHandle{});

                            AzToolsFramework::ScopedUndoBatch undoBatch(HideEdgeUndoRedoDesc);
                            AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityComponentIdPair.GetEntityId());

                            EditorWhiteBoxComponentRequestBus::Event(
                                entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);

                            ComponentModeRequestBus::Event(
                                entityComponentIdPair, &ComponentModeRequestBus::Events::Refresh);
                        })};
        }
        // vertex selection test - ensure a vertex is selected before allowing this shortcut
        else if (auto modifier = AZStd::get_if<AZStd::unique_ptr<VertexSelectionModifier>>(&m_selectedModifier))
        {
            return AZStd::vector<AzToolsFramework::ActionOverride>{
                AzToolsFramework::ActionOverride()
                    .SetUri(HideVertex)
                    .SetKeySequence(HideKey)
                    .SetTitle(HideVertexTitle)
                    .SetTip(HideVertexDesc)
                    .SetEntityComponentIdPair(entityComponentIdPair)
                    .SetCallback(
                        [this, entityComponentIdPair, modifier]()
                        {
                            WhiteBoxMesh* whiteBox = nullptr;
                            EditorWhiteBoxComponentRequestBus::EventResult(
                                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                            Api::HideVertex(*whiteBox, (*modifier)->GetVertexHandle());
                            (*modifier)->SetVertexHandle(Api::VertexHandle{});

                            AzToolsFramework::ScopedUndoBatch undoBatch(HideVertexUndoRedoDesc);
                            AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityComponentIdPair.GetEntityId());

                            EditorWhiteBoxComponentRequestBus::Event(
                                entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);

                            ComponentModeRequestBus::Event(
                                entityComponentIdPair, &ComponentModeRequestBus::Events::Refresh);
                        })};
        }
        else
        {
            return {};
        }
    }

    template<typename ModifierUniquePtr>
    static void TryDestroyModifier(ModifierUniquePtr& modifier)
    {
        // has the mouse moved off of the modifier
        if (modifier && !modifier->MouseOver())
        {
            modifier.reset();
        }
    }

    void DefaultMode::Display(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Transform& worldFromLocal,
        const IntersectionAndRenderData& renderData, const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        TryDestroyModifier(m_polygonTranslationModifier);
        TryDestroyModifier(m_edgeTranslationModifier);
        TryDestroyModifier(m_vertexSelectionModifier);

        debugDisplay.PushMatrix(worldFromLocal);

        DrawEdges(
            debugDisplay, cl_whiteBoxEdgeUserColor, renderData.m_whiteBoxIntersectionData.m_edgeBounds,
            FindInteractiveEdgeHandles(entityComponentIdPair));

        debugDisplay.PopMatrix();
    }

    Api::EdgeHandles DefaultMode::FindInteractiveEdgeHandles(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // get all edge handles for hovered polygon
        const Api::EdgeHandles polygonHoveredEdgeHandles = m_polygonTranslationModifier
            ? Api::PolygonBorderEdgeHandlesFlattened(*whiteBox, m_polygonTranslationModifier->GetPolygonHandle())
            : Api::EdgeHandles{};

        // find edge handles being used by active modifiers
        const Api::EdgeHandles selectedEdgeHandles = [whiteBox, this]()
        {
            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
            {
                return Api::PolygonBorderEdgeHandlesFlattened(*whiteBox, (*modifier)->GetPolygonHandle());
            }

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
            {
                return Api::EdgeHandles{(*modifier)->GetEdgeHandle()};
            }

            return Api::EdgeHandles{};
        }();

        // combine all potentially interactive edge handles
        Api::EdgeHandles interactiveEdgeHandles{polygonHoveredEdgeHandles};
        interactiveEdgeHandles.insert(
            interactiveEdgeHandles.end(), selectedEdgeHandles.begin(), selectedEdgeHandles.end());

        if (m_edgeTranslationModifier)
        {
            // get edge handle for hovered edge (and associated group)
            interactiveEdgeHandles.insert(
                interactiveEdgeHandles.end(), m_edgeTranslationModifier->EdgeHandlesBegin(),
                m_edgeTranslationModifier->EdgeHandlesEnd());
        }

        return interactiveEdgeHandles;
    }

    // if an edge or polygon scale modifier are selected, their scale manipulators (situated
    // at the same position as a vertex) should take priority, so do not attempt to create
    // a vertex selection modifier for those vertices that currently have a scale modifier
    static bool IgnoreVertexHandle(
        const WhiteBoxMesh* whiteBox, const PolygonScaleModifier* polygonScaleModifier,
        const EdgeScaleModifier* edgeScaleModifier, const Api::VertexHandle vertexHandle)
    {
        if (Api::VertexIsHidden(*whiteBox, vertexHandle))
        {
            return true;
        }

        Api::VertexHandles vertexHandlesToIgnore;

        if (polygonScaleModifier)
        {
            const auto polygonVertexHandles =
                Api::PolygonVertexHandles(*whiteBox, polygonScaleModifier->GetPolygonHandle());

            vertexHandlesToIgnore.insert(
                vertexHandlesToIgnore.end(), polygonVertexHandles.cbegin(), polygonVertexHandles.cend());
        }

        if (edgeScaleModifier)
        {
            const auto edgeVertexHandles = Api::EdgeVertexHandles(*whiteBox, edgeScaleModifier->GetEdgeHandle());

            vertexHandlesToIgnore.insert(
                vertexHandlesToIgnore.end(), edgeVertexHandles.cbegin(), edgeVertexHandles.cend());
        }

        return AZStd::find(vertexHandlesToIgnore.cbegin(), vertexHandlesToIgnore.cend(), vertexHandle) !=
            vertexHandlesToIgnore.cend();
    }

    bool DefaultMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const AZStd::optional<EdgeIntersection>& edgeIntersection,
        const AZStd::optional<PolygonIntersection>& polygonIntersection,
        const AZStd::optional<VertexIntersection>& vertexIntersection)
    {
        // polygon
        HandleMouseInteractionForModifier<PolygonTranslationModifier, PolygonIntersection>(
            mouseInteraction, m_selectedModifier,
            [this]
            {
                m_polygonScaleModifier.reset();
            },
            polygonIntersection);

        // edge
        HandleMouseInteractionForModifier<EdgeTranslationModifier, EdgeIntersection>(
            mouseInteraction, m_selectedModifier,
            [this]
            {
                m_edgeScaleModifier.reset();
            },
            edgeIntersection);

        // vertex
        HandleMouseInteractionForModifier<VertexSelectionModifier, VertexIntersection>(
            mouseInteraction, m_selectedModifier, [] { /*noop*/ }, vertexIntersection);

        switch (FindClosestGeometryIntersection(edgeIntersection, polygonIntersection, vertexIntersection))
        {
        case GeometryIntersection::Edge:
            {
                HandleCreatingDestroyingModifiersOnIntersectionChange<EdgeIntersection, EdgeTranslationModifier>(
                    mouseInteraction, m_selectedModifier, m_edgeTranslationModifier,
                    [this]
                    {
                        m_polygonTranslationModifier.reset();
                        m_vertexSelectionModifier.reset();
                    },
                    edgeIntersection, entityComponentIdPair);
            }
            break;
        case GeometryIntersection::Polygon:
            {
                HandleCreatingDestroyingModifiersOnIntersectionChange<PolygonIntersection, PolygonTranslationModifier>(
                    mouseInteraction, m_selectedModifier, m_polygonTranslationModifier,
                    [this]
                    {
                        m_edgeTranslationModifier.reset();
                        m_vertexSelectionModifier.reset();
                    },
                    polygonIntersection, entityComponentIdPair);
            }
            break;
        case GeometryIntersection::Vertex:
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                if (!IgnoreVertexHandle(
                        whiteBox, m_polygonScaleModifier.get(), m_edgeScaleModifier.get(),
                        vertexIntersection->GetHandle()))
                {
                    HandleCreatingDestroyingModifiersOnIntersectionChange<VertexIntersection, VertexSelectionModifier>(
                        mouseInteraction, m_selectedModifier, m_vertexSelectionModifier,
                        [this]
                        {
                            m_edgeTranslationModifier.reset();
                            m_polygonTranslationModifier.reset();
                        },
                        vertexIntersection, entityComponentIdPair);
                }
            }
            break;
        case GeometryIntersection::None:
            // do nothing
            break;
        default:
            // do nothing
            break;
        }

        return false;
    }

    void DefaultMode::CreatePolygonScaleModifier(const Api::PolygonHandle& polygonHandle)
    {
        m_polygonScaleModifier = AZStd::make_unique<PolygonScaleModifier>(polygonHandle, m_entityComponentIdPair);
    }

    void DefaultMode::CreateEdgeScaleModifier(const Api::EdgeHandle edgeHandle)
    {
        m_edgeScaleModifier = AZStd::make_unique<EdgeScaleModifier>(edgeHandle, m_entityComponentIdPair);
    }

    void DefaultMode::AssignSelectedPolygonTranslationModifier()
    {
        if (m_polygonTranslationModifier)
        {
            m_selectedModifier = AZStd::move(m_polygonTranslationModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColors(
                    AZ::Color::CreateFromVector3AndFloat(
                        static_cast<AZ::Color>(cl_whiteBoxSelectedModifierColor).GetAsVector3(), 0.5f),
                    AZ::Color::CreateFromVector3AndFloat(
                        static_cast<AZ::Color>(cl_whiteBoxSelectedModifierColor).GetAsVector3(), 1.0f));
            }

            m_edgeScaleModifier.reset();
            m_vertexSelectionModifier.reset();

            m_polygonTranslationModifier = nullptr;
        }
    }

    void DefaultMode::AssignSelectedEdgeTranslationModifier()
    {
        if (m_edgeTranslationModifier)
        {
            m_selectedModifier = AZStd::move(m_edgeTranslationModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColors(cl_whiteBoxSelectedModifierColor, cl_whiteBoxSelectedModifierColor);
                (*modifier)->SetWidths(cl_whiteBoxSelectedEdgeVisualWidth, cl_whiteBoxSelectedEdgeVisualWidth);
            }

            m_polygonScaleModifier.reset();
            m_vertexSelectionModifier.reset();

            m_edgeTranslationModifier = nullptr;
        }
    }

    void DefaultMode::AssignSelectedVertexSelectionModifier()
    {
        if (m_vertexSelectionModifier)
        {
            m_selectedModifier = AZStd::move(m_vertexSelectionModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<VertexSelectionModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColor(cl_whiteBoxVertexSelectedModifierColor);
            }

            m_polygonScaleModifier.reset();
            m_edgeScaleModifier.reset();

            m_vertexSelectionModifier = nullptr;
        }
    }

    void DefaultMode::RefreshPolygonScaleModifier()
    {
        if (m_polygonScaleModifier)
        {
            m_polygonScaleModifier->Refresh();
        }
    }

    void DefaultMode::RefreshEdgeScaleModifier()
    {
        if (m_edgeScaleModifier)
        {
            m_edgeScaleModifier->Refresh();
        }
    }

    void DefaultMode::RefreshPolygonTranslationModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_polygonTranslationModifier && !m_polygonTranslationModifier->PerformingAction())
        {
            m_polygonTranslationModifier->Refresh();
        }
    }

    void DefaultMode::RefreshEdgeTranslationModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_edgeTranslationModifier && !m_edgeTranslationModifier->PerformingAction())
        {
            m_edgeTranslationModifier->Refresh();
        }
    }

    void DefaultMode::RefreshVertexSelectionModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<VertexSelectionModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_vertexSelectionModifier && !m_vertexSelectionModifier->PerformingAction())
        {
            m_vertexSelectionModifier->Refresh();
        }
    }

    void DefaultMode::OnPolygonModifierUpdatedPolygonHandle(
        const Api::PolygonHandle& previousPolygonHandle, const Api::PolygonHandle& nextPolygonHandle)
    {
        // an operation has caused the currently selected polygon handle to update (e.g. an append/extrusion)
        // if the previous polygon handle matches the selected polygon translation modifier, we know it caused
        // the extrusion and should be updated
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
        {
            if ((*modifier)->GetPolygonHandle() == previousPolygonHandle)
            {
                (*modifier)->SetPolygonHandle(nextPolygonHandle);
                m_polygonScaleModifier->SetPolygonHandle(nextPolygonHandle);
            }
        }
    }

    void DefaultMode::OnEdgeModifierUpdatedEdgeHandle(
        const Api::EdgeHandle previousEdgeHandle, const Api::EdgeHandle nextEdgeHandle)
    {
        // an operation has caused the currently selected edge handle to update (e.g. an append/extrusion)
        // if the previous edge handle matches the selected edge translation modifier, we know it caused
        // the extrusion and should be updated
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            if ((*modifier)->GetEdgeHandle() == previousEdgeHandle)
            {
                m_edgeScaleModifier->SetEdgeHandle(nextEdgeHandle);
            }
        }
    }
} // namespace WhiteBox
