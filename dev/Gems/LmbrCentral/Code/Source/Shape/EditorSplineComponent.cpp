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
#include "EditorSplineComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include "MathConversion.h"

namespace LmbrCentral
{
    static const float s_selectionTolerance = 0.5f;

    SplineHoverSelection::SplineHoverSelection() {}

    SplineHoverSelection::~SplineHoverSelection() {}

    void SplineHoverSelection::Create(AZ::EntityId entityId, AzToolsFramework::ManipulatorManagerId managerId)
    {
        m_splineSelectionManipulator = AZStd::make_unique<AzToolsFramework::SplineSelectionManipulator>(entityId);
        m_splineSelectionManipulator->Register(managerId);

        if (const AZStd::shared_ptr<const AZ::Spline> spline = m_spline.lock())
        {
            const float splineWidth = 0.05f;
            m_splineSelectionManipulator->SetSpline(spline);
            m_splineSelectionManipulator->SetView(CreateManipulatorViewSplineSelect(
                *m_splineSelectionManipulator, AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), splineWidth));
        }

        m_splineSelectionManipulator->InstallLeftMouseUpCallback([this](
            const AzToolsFramework::SplineSelectionManipulator::Action& action)
        {
            if (AZStd::shared_ptr<AZ::Spline> spline = m_spline.lock())
            {
                // wrap vertex container in variable vertices interface
                AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector3> vertices(spline->m_vertexContainer);
                AzToolsFramework::InsertVertex(vertices,
                    action.m_splineAddress.m_segmentIndex,
                    action.m_localSplineHitPosition);
            }
        });
    }

    void SplineHoverSelection::Destroy()
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->Unregister();
            m_splineSelectionManipulator.reset();
        }
    }

    void SplineHoverSelection::Register(AzToolsFramework::ManipulatorManagerId managerId)
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->Register(managerId);
        }
    }

    void SplineHoverSelection::Unregister()
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->Unregister();
        }
    }

    void SplineHoverSelection::SetBoundsDirty()
    {
        if (m_splineSelectionManipulator)
        {
            m_splineSelectionManipulator->SetBoundsDirty();
        }
    }

    void SplineHoverSelection::Refresh()
    {
        SetBoundsDirty();
    }

    static const float s_controlPointSize = 0.1f;

    EditorSplineComponent::~EditorSplineComponent()
    {
    }

    void EditorSplineComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSplineComponent, EditorComponentBase>()
                ->Version(2)
                ->Field("Visible", &EditorSplineComponent::m_visibleInEditor)
                ->Field("Configuration", &EditorSplineComponent::m_splineCommon)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSplineComponent>(
                    "Spline", "Defines a sequence of points that can be interpolated.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Spline.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Spline.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/userguide/spline-component")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSplineComponent::m_visibleInEditor, "Visible", "Always display this shape in the editor viewport")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSplineComponent::m_splineCommon, "Configuration", "Spline Configuration")
                        //->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly) // disabled - prevents ChangeNotify attribute firing correctly
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSplineComponent::SplineChanged)
                        ;
            }
        }
    }

    void EditorSplineComponent::Activate()
    {
        EditorComponentBase::Activate();

        const AZ::EntityId entityId = GetEntityId();

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        EntitySelectionEvents::Bus::Handler::BusConnect(entityId);
        SplineComponentRequestBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        ToolsApplicationEvents::Bus::Handler::BusConnect();

        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        bool selected = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            selected, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSelected);

        // placeholder - create initial spline if empty
        AZ::VertexContainer<AZ::Vector3>& vertexContainer = m_splineCommon.m_spline->m_vertexContainer;
        if (selected && vertexContainer.Empty())
        {
            vertexContainer.AddVertex(AZ::Vector3(-3.0f, 0.0f, 0.0f));
            vertexContainer.AddVertex(AZ::Vector3(-1.0f, 0.0f, 0.0f));
            vertexContainer.AddVertex(AZ::Vector3(1.0f, 0.0f, 0.0f));
            vertexContainer.AddVertex(AZ::Vector3(3.0f, 0.0f, 0.0f));

            CreateManipulators();
        }

        auto containerChanged = [this]()
        {
            // destroy and recreate manipulators when container is modified (vertices are added or removed)
            m_vertexSelection.Destroy();
            CreateManipulators();
            SplineChanged();
        };

        auto vertexAdded = [this, containerChanged](size_t index)
        {
            containerChanged();
            SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexAdded, index);
            AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
            m_vertexSelection.CreateTranslationManipulator(GetEntityId(), managerId,
                AzToolsFramework::TranslationManipulator::Dimensions::Three,
                m_splineCommon.m_spline->m_vertexContainer.GetVertices()[index], index,
                AzToolsFramework::ConfigureTranslationManipulatorAppearance3d);
        };

        auto vertexRemoved = [this, containerChanged](size_t index)
        {
            containerChanged();
            SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnVertexRemoved, index);
        };

        auto vertexUpdated = [this]()
        {
            SplineChanged();
            m_vertexSelection.Refresh();
        };

        auto verticesSet = [this, containerChanged]()
        {
            containerChanged();
            SplineComponentNotificationBus::Event(
                GetEntityId(),
                &SplineComponentNotificationBus::Events::OnVerticesSet,
                m_splineCommon.m_spline->GetVertices()
            );
        };

        auto verticesCleared = [this, containerChanged]()
        {
            containerChanged();
            SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnVerticesCleared);
        };

        m_splineCommon.SetCallbacks(
            vertexAdded,
            vertexRemoved,
            vertexUpdated,
            verticesSet,
            verticesCleared,
            [this]() { OnChangeSplineType(); });
    }

    void EditorSplineComponent::Deactivate()
    {
        m_splineCommon.SetCallbacks(
            nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr);

        m_vertexSelection.Destroy();

        EditorComponentBase::Deactivate();

        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        SplineComponentRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
    }

    void EditorSplineComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<SplineComponent>())
        {
            component->m_splineCommon = m_splineCommon;
        }
    }

    static void DrawSpline(const AZ::Spline& spline, size_t begin, size_t end, AzFramework::EntityDebugDisplayRequests* displayContext)
    {
        const size_t granularity = spline.GetSegmentGranularity();

        for (size_t i = begin; i < end; ++i)
        {
            AZ::Vector3 p1 = spline.GetVertex(i - 1);
            for (size_t j = 1; j <= granularity; ++j)
            {
                AZ::Vector3 p2 = spline.GetPosition(AZ::SplineAddress(i - 1, j / static_cast<float>(granularity)));
                displayContext->DrawLine(p1, p2);
                p1 = p2;
            }
        }
    }

    void EditorSplineComponent::DisplayEntity(bool& handled)
    {
        const bool mouseHovered = m_accentType == AzToolsFramework::EntityAccentType::Hover;
        if (!IsSelected() && !m_visibleInEditor && !mouseHovered)
        {
            return;
        }

        handled = true;

        AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "Invalid display context.");

        const AZ::Spline* spline = m_splineCommon.m_spline.get();
        const size_t vertexCount = spline->GetVertexCount();
        if (vertexCount == 0)
        {
            return;
        }

        displayContext->PushMatrix(GetWorldTM());

        // Set color
        if (IsSelected())
        {
            displayContext->SetColor(AzFramework::ViewportColors::SelectedColor);
        }
        else if(mouseHovered)
        {
            displayContext->SetColor(AzFramework::ViewportColors::HoverColor);
        }
        else
        {
            displayContext->SetColor(AzFramework::ViewportColors::DeselectedColor);
        }

        // render spline
        if (spline->RTTI_IsTypeOf(AZ::LinearSpline::RTTI_Type()) || spline->RTTI_IsTypeOf(AZ::BezierSpline::RTTI_Type()))
        {
            DrawSpline(*spline, 1, spline->IsClosed() ? vertexCount + 1 : vertexCount, displayContext);
        }
        else if (spline->RTTI_IsTypeOf(AZ::CatmullRomSpline::RTTI_Type()))
        {
            // catmull-rom splines use the first and last points as control points only, omit those for display
            DrawSpline(*spline, spline->IsClosed() ? 1 : 2, spline->IsClosed() ? vertexCount + 1 : vertexCount - 1, displayContext);
        }

        AzToolsFramework::EditorVertexSelectionUtil::DisplayVertexContainerIndices(*displayContext, m_vertexSelection, GetWorldTM(), IsSelected());
        
        displayContext->PopMatrix();
    }

    void EditorSplineComponent::OnSelected()
    {
        // ensure any maniulators are destroyed before recreated - (for undo/redo)
        m_vertexSelection.Destroy();
        CreateManipulators();
    }

    void EditorSplineComponent::OnDeselected()
    {
        m_vertexSelection.Destroy();
    }

    AZ::Aabb EditorSplineComponent::GetEditorSelectionBounds()
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        m_splineCommon.m_spline->GetAabb(aabb, m_currentTransform);
        
        const AZ::VectorFloat selectionTolerance = AZ::VectorFloat(s_selectionTolerance);
        AZ::Vector3 expandExtents = AZ::Vector3::CreateZero();
        const AZ::Vector3 extents = aabb.GetExtents();
        
        if (extents.GetX().IsLessThan(s_selectionTolerance))
        {
            expandExtents.SetX(selectionTolerance);
        }
        
        if (extents.GetY().IsLessThan(s_selectionTolerance))
        {
            expandExtents.SetY(selectionTolerance);
        }
        
        if (extents.GetZ().IsLessThan(s_selectionTolerance))
        {
            expandExtents.SetY(selectionTolerance);
        }

        aabb.Expand(expandExtents);

        return aabb;
    }

    bool EditorSplineComponent::EditorSelectionIntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        const auto rayIntersectData = IntersectSpline(m_currentTransform, src, dir, *m_splineCommon.m_spline);
        distance = rayIntersectData.m_rayDistance * m_currentTransform.RetrieveScale().GetMaxElement();
        
        return static_cast<float>(rayIntersectData.m_distanceSq) < powf(s_selectionTolerance, 2.0f);
    }

    void EditorSplineComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        // refresh all manipulator bounds when entity moves
        m_vertexSelection.SetBoundsDirty();
    }

    void EditorSplineComponent::CreateManipulators()
    {
        bool selected = false;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, GetEntityId());

        if (!selected)
        {
            return;
        }

        // if we have no vertices, do not attempt to create any manipulators
        if (m_splineCommon.m_spline->m_vertexContainer.Empty())
        {
            return;
        }

        AZStd::unique_ptr<SplineHoverSelection> splineHoverSelection =
            AZStd::make_unique<SplineHoverSelection>();
        splineHoverSelection->m_spline = m_splineCommon.m_spline;
        m_vertexSelection.m_hoverSelection = AZStd::move(splineHoverSelection);

        // create interface wrapping internal vertex container for use by vertex selection
        m_vertexSelection.m_vertices =
            AZStd::make_unique<AzToolsFramework::VariableVerticesVertexContainer<AZ::Vector3>>(
                m_splineCommon.m_spline->m_vertexContainer);

        const AzToolsFramework::ManipulatorManagerId managerId = AzToolsFramework::ManipulatorManagerId(1);
        m_vertexSelection.Create(GetEntityId(), managerId,
            AzToolsFramework::TranslationManipulator::Dimensions::Three,
            AzToolsFramework::ConfigureTranslationManipulatorAppearance3d);
    }

    void EditorSplineComponent::AfterUndoRedo()
    {
        bool selected;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, GetEntityId());
        if (selected)
        {
            m_vertexSelection.Destroy();
            CreateManipulators();
        }
    }

    void EditorSplineComponent::OnChangeSplineType()
    {
        m_vertexSelection.Destroy();
        CreateManipulators();
        SplineChanged();
    }

    void EditorSplineComponent::SplineChanged() const
    {
        SplineComponentNotificationBus::Event(GetEntityId(), &SplineComponentNotificationBus::Events::OnSplineChanged);
    }

    AZ::SplinePtr EditorSplineComponent::GetSpline()
    {
        return m_splineCommon.m_spline;
    }

    void EditorSplineComponent::ChangeSplineType(AZ::u64 splineType)
    {
        m_splineCommon.ChangeSplineType(splineType);
    }

    void EditorSplineComponent::SetClosed(bool closed)
    {
        m_splineCommon.m_spline->SetClosed(closed);
        SplineChanged();
    }

    bool EditorSplineComponent::GetVertex(size_t index, AZ::Vector3& vertex) const
    {
        return m_splineCommon.m_spline->m_vertexContainer.GetVertex(index, vertex);
    }

    bool EditorSplineComponent::UpdateVertex(size_t index, const AZ::Vector3& vertex)
    {
        if (m_splineCommon.m_spline->m_vertexContainer.UpdateVertex(index, vertex))
        {
            SplineChanged();
            return true;
        }

        return false;
    }

    void EditorSplineComponent::AddVertex(const AZ::Vector3& vertex)
    {
        m_splineCommon.m_spline->m_vertexContainer.AddVertex(vertex);
        SplineChanged();
    }

    bool EditorSplineComponent::InsertVertex(size_t index, const AZ::Vector3& vertex)
    {
        if (m_splineCommon.m_spline->m_vertexContainer.InsertVertex(index, vertex))
        {
            SplineChanged();
            return true;
        }

        return false;
    }

    bool EditorSplineComponent::RemoveVertex(size_t index)
    {
        if (m_splineCommon.m_spline->m_vertexContainer.RemoveVertex(index))
        {
            SplineChanged();
            return true;
        }

        return false;
    }

    void EditorSplineComponent::SetVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_splineCommon.m_spline->m_vertexContainer.SetVertices(vertices);
        SplineChanged();
    }

    void EditorSplineComponent::ClearVertices()
    {
        m_splineCommon.m_spline->m_vertexContainer.Clear();
        SplineChanged();
    }

    size_t EditorSplineComponent::Size() const
    {
        return m_splineCommon.m_spline->m_vertexContainer.Size();
    }

    bool EditorSplineComponent::Empty() const
    {
        return m_splineCommon.m_spline->m_vertexContainer.Empty();
    }
} // namespace LmbrCentral