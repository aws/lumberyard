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

#include "EditorVertexSelection.h"

#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    /**
     * Remove a vertex from the container and ensure the associated manipulator is unset and
     * property display values are refreshed.
     */
    template<typename Vertex>
    static void SafeRemoveVertex(ManipulatorManagerId managerId,
        AZ::VariableVertices<Vertex>& vertices, size_t index)
    {
        // when removing vertex ensure we clear the active manipulator
        ManipulatorManagerRequestBus::Event(managerId,
            &ManipulatorManagerRequestBus::Events::SetActiveManipulator, nullptr);

        vertices.RemoveVertex(index);

        // ensure property grid values are refreshed
        ToolsApplicationNotificationBus::Broadcast(
            &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
            Refresh_EntireTree);
    }

    /**
     * Iterate over all vertices currently associated with the translation manipulator and update their
     * positions by taking their starting positions and modifying them by an offset.
     */
    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::UpdateManipulatorsAndVerticesFromOffset(
        IndexedTranslationManipulator<Vertex>& translationManipulator, const AZ::Vector3& localOffset)
    {
        translationManipulator.Process([this, localOffset](typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
        {
            GetVertices()->UpdateVertex(vertex.m_index, vertex.m_start + AdaptVertexIn<Vertex>(localOffset));
        });

        ManipulatorsAndVerticesUpdated();
    }

    /**
     * Iterate over all vertices currently associated with the translation manipulator and update their
     * positions by passing a new local position.
     */
    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::UpdateManipulatorsAndVerticesFromPosition(
        IndexedTranslationManipulator<Vertex>& translationManipulator, const AZ::Vector3& localPosition)
    {
        translationManipulator.Process([this, localPosition](typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
        {
            GetVertices()->UpdateVertex(vertex.m_index, AdaptVertexIn<Vertex>(localPosition));
        });

        ManipulatorsAndVerticesUpdated();
    }

    /**
     * Called after modifying vertices from UpdateManipulatorsAndVerticesFromOffset/Position - ensure dependent
     * code is notified in the positionsUpdated callback and refresh manipulators and ui.
     */
    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::ManipulatorsAndVerticesUpdated()
    {
        // after vertex positions have changed, anything else which relies on their positions may update
        if (m_onVertexPositionsUpdated)
        {
            m_onVertexPositionsUpdated();
        }

        // after updating the underlying vertex postion, ensure we
        // refresh manipulators with the new positions.
        Refresh();

        // ensure property grid values are refreshed
        ToolsApplicationNotificationBus::Broadcast(
            &ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay,
            Refresh_Values);
    }

    /**
     * In OnMouseDown for various manipulators (linear/planar/surface), ensure we record the vertex starting position
     * for each vertex associated with the translation manipulator to use with offset calculations when updating.
     */
    template<typename Vertex>
    void InitializeVertexLookup(
        IndexedTranslationManipulator<Vertex>& translationManipulator, const AZ::FixedVertices<Vertex>& vertices,
        const AZ::Vector3& snapOffset)
    {
        translationManipulator.Process([&vertices, snapOffset](
            typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertexLookup)
        {
            Vertex vertex;
            if (vertices.GetVertex(vertexLookup.m_index, vertex))
            {
                vertexLookup.m_start = vertex + AdaptVertexIn<Vertex>(snapOffset);
            }
        });
    }

    /**
     * Create a translation manipulator for a specific vertex and setup its corresponding callbacks etc.
     */
    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::CreateTranslationManipulator(
        AZ::EntityId entityId, ManipulatorManagerId managerId,
        TranslationManipulator::Dimensions dimensions, const Vertex& vertex, size_t index,
        TranslationManipulatorConfiguratorFn<Vertex> translationManipulatorConfigurator)
    {
        // if we have a vertex (translation) manipulator active, ensure
        // it gets removed when clicking on another selection manipulator
        ClearSelected();

        // hide this selection manipulator when clicked
        // note: this will only happen when not no translation manipulator already exists, or when
        // not shift+clicking - in those cases we want to be able to toggle selected vertex on and off
        m_selectionManipulators[index]->Unregister();
        m_selectionManipulators[index]->ToggleSelected();

        // create a new translation manipulator bound for the selected index
        m_translationManipulator = AZStd::make_shared<IndexedTranslationManipulator<Vertex>>(
            entityId, dimensions, index, vertex);

        // setup how the manipulator should look
        translationManipulatorConfigurator(&m_translationManipulator->m_manipulator, vertex);

        // linear manipulator callbacks
        m_translationManipulator->m_manipulator.InstallLinearManipulatorMouseDownCallback([this](
            const LinearManipulator::Action& action)
        {
            InitializeVertexLookup(*m_translationManipulator, *GetVertices(), action.m_start.m_snapOffset);
        });

        m_translationManipulator->m_manipulator.InstallLinearManipulatorMouseMoveCallback([this](
            const LinearManipulator::Action& action)
        {
            UpdateManipulatorsAndVerticesFromOffset(*m_translationManipulator, action.m_current.m_localOffset);
        });

        // planar manipulator callbacks
        m_translationManipulator->m_manipulator.InstallPlanarManipulatorMouseDownCallback([this](
            const PlanarManipulator::Action& action)
        {
            InitializeVertexLookup(*m_translationManipulator, *GetVertices(), action.m_start.m_snapOffset);
        });

        m_translationManipulator->m_manipulator.InstallPlanarManipulatorMouseMoveCallback([this](
            const PlanarManipulator::Action& action)
        {
            UpdateManipulatorsAndVerticesFromOffset(*m_translationManipulator, action.m_current.m_localOffset);
        });

        // surface manipulator callbacks
        m_translationManipulator->m_manipulator.InstallSurfaceManipulatorMouseDownCallback([this](
            const SurfaceManipulator::Action& action)
        {
            InitializeVertexLookup(*m_translationManipulator, *GetVertices(), action.m_start.m_snapOffset);
        });

        m_translationManipulator->m_manipulator.InstallSurfaceManipulatorMouseMoveCallback([this](
            const SurfaceManipulator::Action& action)
        {
            UpdateManipulatorsAndVerticesFromOffset(*m_translationManipulator, action.m_current.m_localOffset);
        });

        // register the m_translation manipulator so it appears where the selection manipulator previously was
        m_translationManipulator->m_manipulator.Register(managerId);

        // set new edit tool while in translation mode so we can 'click off' to go back to normal selection mode
        EditorRequests::Bus::Broadcast(
            &EditorRequests::Bus::Events::SetEditTool, "EditTool.Manipulator");
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::Create(
        AZ::EntityId entityId, ManipulatorManagerId managerId,
        TranslationManipulator::Dimensions dimensions,
        TranslationManipulatorConfiguratorFn<Vertex> translationManipulatorConfigurator)
    {
        ManipulatorRequestBus::Handler::BusConnect(entityId);

        const size_t vertexCount = GetVertices()->Size();
        m_selectionManipulators.reserve(vertexCount);

        // initialize manipulators for all spline vertices
        for (size_t i = 0; i < vertexCount; ++i)
        {
            Vertex vertex;
            GetVertices()->GetVertex(i, vertex);

            m_selectionManipulators.push_back(AZStd::make_shared<SelectionManipulator>(entityId));
            const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator = m_selectionManipulators.back();

            selectionManipulator->SetPosition(AdaptVertexOut(vertex));

            SetupSelectionManipulator(
                selectionManipulator, entityId, managerId, dimensions, i, translationManipulatorConfigurator);

            selectionManipulator->Register(managerId);
        }

        // create our hover manipulators so new points can be inserted
        m_hoverSelection->Create(entityId, managerId);

        OnCreated(entityId);
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::Destroy()
    {
        for (auto& manipulator : m_selectionManipulators)
        {
            manipulator->Unregister();
        }

        m_selectionManipulators.clear();

        if (m_translationManipulator)
        {
            m_translationManipulator->m_manipulator.Unregister();
            m_translationManipulator.reset();
        }

        if (m_hoverSelection)
        {
            m_hoverSelection->Destroy();
            m_hoverSelection.reset();
        }

        ManipulatorRequestBus::Handler::BusDisconnect();
        OnDestroyed();
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::ClearSelected()
    {
        const ManipulatorManagerId managerId = ManipulatorManagerId(1);

        // if translation manipulator is active, remove it when receiving this event and enable
        // the hover maniuplator bounds again so points can be inserted again
        if (m_translationManipulator)
        {
            // re-enable all selection manipulators associtated with the translation
            // manipulator which is now being removed.
            m_translationManipulator->Process([this, managerId](
                typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
            {
                m_selectionManipulators[vertex.m_index]->Register(managerId);
                m_selectionManipulators[vertex.m_index]->Deselect();
            });

            m_translationManipulator->m_manipulator.Unregister();
            m_translationManipulator.reset();
        }

        m_hoverSelection->Register(managerId);
    }

    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::DestroySelected()
    {
        // if translation manipulator is active, remove it and destroy the vertex at its
        // index when receiving this event and enable the hover maniuplator bounds again
        // so points can be inserted again
        const ManipulatorManagerId managerId = ManipulatorManagerId(1);
        if (EditorVertexSelectionBase<Vertex>::m_translationManipulator)
        {
            // keep a ref to translation manipulator while removing it so it is not destroyed prematurely
            AZStd::shared_ptr<IndexedTranslationManipulator<Vertex>> translationManipulator = EditorVertexSelectionBase<Vertex>::m_translationManipulator;
            translationManipulator->Process([this, managerId](
                typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
            {
                SafeRemoveVertex(managerId, *m_vertices, vertex.m_index);
            });

            translationManipulator->m_manipulator.Unregister();
            translationManipulator.reset();
        }

        EditorVertexSelectionBase<Vertex>::m_hoverSelection->Register(managerId);
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SetSelectedPosition(const AZ::Vector3& localPosition)
    {
        if (m_translationManipulator)
        {
            InitializeVertexLookup(*m_translationManipulator, *GetVertices(), AZ::Vector3::CreateZero());
            // note: AdaptVertexIn/Out is to ensure we clamp the vertex local Z position to 0 if
            // dealing with Vector2s when setting the position of the manipulator.
            const AZ::Vector3 localOffset = localPosition - m_translationManipulator->m_manipulator.GetPosition();
            UpdateManipulatorsAndVerticesFromOffset(
                *m_translationManipulator, AdaptVertexOut<Vertex>(AdaptVertexIn<Vertex>(localOffset)));
        }
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::Refresh()
    {
        for (size_t i = 0; i < m_selectionManipulators.size(); ++i)
        {
            Vertex vertex;
            if (GetVertices()->GetVertex(i, vertex))
            {
                m_selectionManipulators[i]->SetPosition(AdaptVertexOut(vertex));
                m_selectionManipulators[i]->SetBoundsDirty();
            }
        }

        if (m_translationManipulator)
        {
            const size_t selectedVertexCount = m_translationManipulator->m_vertices.size();
            if (selectedVertexCount > 0)
            {
                // calculate average position of selected vertices for translation manipulator
                Vertex averageVertex = Vertex::CreateZero();
                m_translationManipulator->Process([this, &averageVertex](
                    typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
                {
                    Vertex v;
                    if (GetVertices()->GetVertex(vertex.m_index, v))
                    {
                        averageVertex += v;
                    }
                });

                averageVertex /= static_cast<float>(selectedVertexCount);

                m_translationManipulator->m_manipulator.SetPosition(AdaptVertexOut(averageVertex));
                m_translationManipulator->m_manipulator.SetBoundsDirty();
            }
        }

        if (m_hoverSelection)
        {
            m_hoverSelection->Refresh();
        }
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SetBoundsDirty()
    {
        for (auto& manipulator : m_selectionManipulators)
        {
            manipulator->SetBoundsDirty();
        }

        if (m_translationManipulator)
        {
            m_translationManipulator->m_manipulator.SetBoundsDirty();
        }

        if (m_hoverSelection)
        {
            m_hoverSelection->SetBoundsDirty();
        }
    }

    /**
     * Handle correctly selecting/deselecting vertices in a vertex selection.
     */
    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SelectionManipulatorSelectCallback(
        size_t index, const ViewportInteraction::MouseInteraction& interaction, AZ::EntityId entityId,
        ManipulatorManagerId managerId, TranslationManipulator::Dimensions dimensions,
        TranslationManipulatorConfiguratorFn<Vertex> translationManipulatorConfigurator)
    {
        Vertex vertex;
        GetVertices()->GetVertex(index, vertex);
        if (m_translationManipulator != nullptr && interaction.m_keyboardModifiers.Shift())
        {
            // ensure all selection manipulators are enabled when selecting more than one (the first
            // will have been disabled when only selecting an individual vertex
            m_translationManipulator->Process([this, managerId](
                typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertexLookup)
            {
                m_selectionManipulators[vertexLookup.m_index]->Register(managerId);
            });

            // if selection manipulator was selected, find it in the vector of vertices stored in
            // the translation manipulator and remove it
            if (m_selectionManipulators[index]->Selected())
            {
                auto it = AZStd::find_if(m_translationManipulator->m_vertices.begin(), m_translationManipulator->m_vertices.end(),
                    [index](const typename IndexedTranslationManipulator<Vertex>::VertexLookup vertexLookup)
                {
                    return index == vertexLookup.m_index;
                });

                if (it != m_translationManipulator->m_vertices.end())
                {
                    m_translationManipulator->m_vertices.erase(it);
                }
            }
            else
            {
                // otherwise add the new selected vertex
                m_translationManipulator->m_vertices.push_back(
                    typename IndexedTranslationManipulator<Vertex>::VertexLookup{ vertex, index });
            }

            // if we have deselected a selection manipulator, and there is only one left, unregister/disable
            // it as we do not want to draw it at the same position as the translation manipulator
            if (m_translationManipulator->m_vertices.size() == 1)
            {
                m_selectionManipulators[m_translationManipulator->m_vertices.back().m_index]->Unregister();
            }

            // toggle state after a press
            m_selectionManipulators[index]->ToggleSelected();

            Refresh();
        }
        else
        {
            // if one does not already exist, or we're not holding shift, create a new translation
            // manipulator at this vertex
            CreateTranslationManipulator(
                entityId, managerId, dimensions, vertex,
                index, translationManipulatorConfigurator);
        }
    }

    /**
     * Configure the selection manipulator for fixed edtor selection - this configures the view and action
     * of interacting with the selection manipulator. Vertices can just be selected (create a translation
     * manipulator) but not added or removed.
     */
    template<typename Vertex>
    void EditorVertexSelectionFixed<Vertex>::SetupSelectionManipulator(
        const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator, AZ::EntityId entityId,
        ManipulatorManagerId managerId, TranslationManipulator::Dimensions dimensions, size_t index,
        TranslationManipulatorConfiguratorFn<Vertex> translationManipulatorConfigurator)
    {
        // setup selection manipulator
        selectionManipulator->SetView(AzToolsFramework::CreateManipulatorViewSphere(AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
            g_defaultManipulatorSphereRadius,
            [&selectionManipulator](const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, bool mouseOver, const AZ::Color& defaultColor)
        {
            if (selectionManipulator->Selected())
            {
                return AZ::Color(1.0f, 1.0f, 0.0f, 1.0f);
            }

            const float opacity[2] = { 0.5f, 1.0f };
            return AZ::Color(defaultColor.GetR(), defaultColor.GetG(), defaultColor.GetB(), AZ::VectorFloat(opacity[mouseOver]));
        }));

        selectionManipulator->InstallLeftMouseUpCallback([
            this, entityId, index, managerId, dimensions, translationManipulatorConfigurator](
                const ViewportInteraction::MouseInteraction& interaction)
        {
            EditorVertexSelectionBase<Vertex>::SelectionManipulatorSelectCallback(
                index, interaction, entityId, managerId, dimensions, translationManipulatorConfigurator);
        });
    }

    /**
     * Configure the selection manipulator for variable edtor selection - this configures the view and action
     * of interacting with the selection manipulator. In this case, hovering the mouse with a modifier key held
     * will indicate removal, and clicking with a modifier key will remove the vertex.
     */
    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::SetupSelectionManipulator(
        const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator, AZ::EntityId entityId,
        ManipulatorManagerId managerId, TranslationManipulator::Dimensions dimensions, size_t index,
        TranslationManipulatorConfiguratorFn<Vertex> translationManipulatorConfigurator)
    {
        // setup selection manipulator
        selectionManipulator->SetView(AzToolsFramework::CreateManipulatorViewSphere(AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
            g_defaultManipulatorSphereRadius,
            [&selectionManipulator](const ViewportInteraction::MouseInteraction& mouseInteraction, bool mouseOver, const AZ::Color& defaultColor)
        {
            if (mouseInteraction.m_keyboardModifiers.Alt() && mouseOver)
            {
                // indicate removal of manipulator
                return AZ::Color(0.5f, 0.5f, 0.5f, 0.5f);
            }

            // highlight or not if mouse is over
            const float opacity[2] = { 0.5f, 1.0f };
            if (selectionManipulator->Selected())
            {
                return AZ::Color(1.0f, 1.0f, 0.0f, opacity[mouseOver]);
            }

            return AZ::Color(
                defaultColor.GetR(), defaultColor.GetG(), defaultColor.GetB(), AZ::VectorFloat(opacity[mouseOver]));
        }));

        selectionManipulator->InstallLeftMouseUpCallback([
            this, entityId, index, managerId, dimensions,
                translationManipulatorConfigurator, &selectionManipulator]( // very important @selectionManipulator is captured by ref here
                    const ViewportInteraction::MouseInteraction& interaction)
        {
            if (interaction.m_keyboardModifiers.Alt())
            {
                // keep a ref count of the manipulator when removing a vertex so it does not get
                // destroyed too early (before undo batch has been recorded)
                AZStd::shared_ptr<SelectionManipulator> current = selectionManipulator;
                SafeRemoveVertex(managerId, *m_vertices, index);
                AZ_UNUSED(current);
            }
            else
            {
                EditorVertexSelectionBase<Vertex>::SelectionManipulatorSelectCallback(
                    index, interaction, entityId, managerId, dimensions, translationManipulatorConfigurator);
            }
        });
    }

    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::OnCreated(AZ::EntityId entityId)
    {
        DestructiveManipulatorRequestBus::Handler::BusConnect(entityId);
    }

    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::OnDestroyed()
    {
        DestructiveManipulatorRequestBus::Handler::BusDisconnect();
    }

    // explicit instantiations
    template class EditorVertexSelectionBase<AZ::Vector2>;
    template class EditorVertexSelectionBase<AZ::Vector3>;
    template class EditorVertexSelectionVariable<AZ::Vector2>;
    template class EditorVertexSelectionVariable<AZ::Vector3>;
    template class EditorVertexSelectionFixed<AZ::Vector2>;
    template class EditorVertexSelectionFixed<AZ::Vector3>;
}