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
#include "stdafx.h"

#include "EditorCommon.h"
#include <LyShine/UiComponentTypes.h>

static const float g_elementEdgeForgiveness = 10.0f;

// The square of the minimum corner-to-corner distance for an area selection
static const float g_minAreaSelectionDistance2 = 100.0f;

#define UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_KEY         "ViewportWidget::m_interactionMode"
#define UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_DEFAULT     ( ViewportInteraction::InteractionMode::SELECTION )

#define UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_KEY         "ViewportWidget::m_coordinateSystem"
#define UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_DEFAULT     ( ViewportInteraction::CoordinateSystem::LOCAL )

namespace
{
    const float defaultCanvasToViewportScaleIncrement = 0.15f;

    ViewportInteraction::InteractionMode PersistentGetInteractionMode()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        int default = static_cast<int>(UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_DEFAULT);
        int result = settings.value(
                UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_KEY,
                default).toInt();

        settings.endGroup();

        return static_cast<ViewportInteraction::InteractionMode>(result);
    }

    void PersistentSetInteractionMode(ViewportInteraction::InteractionMode mode)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        settings.setValue(UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_INTERACTION_MODE_KEY,
            static_cast<int>(mode));

        settings.endGroup();
    }

    ViewportInteraction::CoordinateSystem PersistentGetCoordinateSystem()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        int default = static_cast<int>(UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_DEFAULT);
        int result = settings.value(
                UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_KEY,
                default).toInt();

        settings.endGroup();

        return static_cast<ViewportInteraction::CoordinateSystem>(result);
    }

    void PersistentSetCoordinateSystem(ViewportInteraction::CoordinateSystem coordinateSystem)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);

        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        settings.setValue(UICANVASEDITOR_SETTINGS_VIEWPORTINTERACTION_COORDINATE_SYSTEM_KEY,
            static_cast<int>(coordinateSystem));

        settings.endGroup();
    }

    QCursor cursorRotate(CMFCUtils::LoadCursor(IDC_POINTER_OBJECT_ROTATE));
} // anonymous namespace.

ViewportInteraction::ViewportInteraction(EditorWindow* editorWindow)
    : QObject()
    , m_editorWindow(editorWindow)
    , m_activeElementId()
    , m_anchorWhole(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Anchor_Whole.tif"))
    , m_pivotIcon(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Pivot.tif"))
    , m_interactionMode(PersistentGetInteractionMode())
    , m_interactionType(InteractionType::NONE)
    , m_coordinateSystem(PersistentGetCoordinateSystem())
    , m_spaceBarIsActive(false)
    , m_leftButtonIsActive(false)
    , m_middleButtonIsActive(false)
    , m_reversibleActionStarted(false)
    , m_startMouseDragPos(0.0f, 0.0f)
    , m_lastMouseDragPos(0.0f, 0.0f)
    , m_selectedElementsAtSelectionStart()
    , m_canvasViewportMatrixProps()
    , m_shouldScaleToFitOnViewportResize(true)
    , m_transformComponentType(AZ::Uuid::CreateNull())
    , m_grabbedEdges(ViewportHelpers::ElementEdges())
    , m_startAnchors(UiTransform2dInterface::Anchors())
    , m_grabbedAnchors(ViewportHelpers::SelectedAnchors())
    , m_grabbedGizmoParts(ViewportHelpers::GizmoParts())
    , m_lineTriangleX(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Transform_Gizmo_Line_Triangle_X.tif"))
    , m_lineTriangleY(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Transform_Gizmo_Line_Triangle_Y.tif"))
    , m_circle(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Transform_Gizmo_Circle.tif"))
    , m_lineSquareX(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Transform_Gizmo_Line_Square_X.tif"))
    , m_lineSquareY(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Transform_Gizmo_Line_Square_Y.tif"))
    , m_centerSquare(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Transform_Gizmo_Center_Square.tif"))
    , m_dottedLine(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/DottedLine.tif"))
{
}

ViewportInteraction::~ViewportInteraction()
{
}

void ViewportInteraction::ClearInteraction(bool clearSpaceBarIsActive)
{
    m_activeElementId.SetInvalid();
    m_interactionType = InteractionType::NONE;
    m_spaceBarIsActive = clearSpaceBarIsActive ? false : m_spaceBarIsActive;
    m_leftButtonIsActive = false;
    m_middleButtonIsActive = false;
    m_startMouseDragPos = AZ::Vector2::CreateZero();
    m_lastMouseDragPos = AZ::Vector2::CreateZero();
    m_grabbedEdges = ViewportHelpers::ElementEdges();
    m_startAnchors = UiTransform2dInterface::Anchors();
    m_grabbedAnchors = ViewportHelpers::SelectedAnchors();
    m_grabbedGizmoParts = ViewportHelpers::GizmoParts();
    m_selectedElementsAtSelectionStart.clear();
}

bool ViewportInteraction::GetLeftButtonIsActive()
{
    return m_leftButtonIsActive;
}

bool ViewportInteraction::GetSpaceBarIsActive()
{
    return m_spaceBarIsActive;
}

void ViewportInteraction::Draw(Draw2dHelper& draw2d,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    // Draw the transform gizmo where appropriate
    if (m_interactionMode != InteractionMode::SELECTION)
    {
        LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        switch (m_interactionMode)
        {
        case InteractionMode::MOVE:
            for (auto element : selectedElements)
            {
                if (!ViewportHelpers::IsControlledByLayout(element))
                {
                    DrawAxisGizmo(draw2d, element, m_coordinateSystem, m_lineTriangleX.get(), m_lineTriangleY.get());
                }
            }
            break;
        case InteractionMode::ROTATE:
            for (auto element : selectedElements)
            {
                DrawCircleGizmo(draw2d, element);
            }
            break;
        case InteractionMode::RESIZE:
            for (auto element : selectedElements)
            {
                if (!ViewportHelpers::IsControlledByLayout(element))
                {
                    DrawAxisGizmo(draw2d, element, CoordinateSystem::LOCAL, m_lineSquareX.get(), m_lineSquareY.get());
                }
            }
            break;
        }
    }

    // Draw the area selection, if there is one
    if (AreaSelectionIsActive())
    {
        m_dottedLine->DrawAxisAlignedBoundingBox(draw2d, m_startMouseDragPos, m_lastMouseDragPos);
    }
}

bool ViewportInteraction::AreaSelectionIsActive()
{
    return ((!m_spaceBarIsActive) && m_leftButtonIsActive && m_interactionType == InteractionType::NONE);
}

void ViewportInteraction::BeginReversibleAction(QTreeWidgetItemRawPtrQList& selectedItems)
{
    if ((m_reversibleActionStarted) ||
        (m_interactionType == InteractionType::NONE) ||
        (m_interactionMode == InteractionMode::SELECTION))
    {
        // Nothing to do.
        return;
    }

    // we are about to change something and we have not started an undo action yet, start one
    m_reversibleActionStarted = true;

    // Tell the Properties panel that we're about to do a reversible action
    m_editorWindow->GetProperties()->BeforePropertyModified(nullptr);

    // Snapping.
    bool isSnapping = false;
    EBUS_EVENT_ID_RESULT(isSnapping, m_editorWindow->GetCanvas(), UiCanvasBus, GetIsSnapEnabled);
    if (isSnapping)
    {
        // Set all initial non-snapped values.

        HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(m_editorWindow->GetHierarchy(),
                selectedItems);
        for (auto i : items)
        {
            UiTransform2dInterface::Offsets offsets;
            EBUS_EVENT_ID_RESULT(offsets, i->GetEntityId(), UiTransform2dBus, GetOffsets);
            i->SetNonSnappedOffsets(offsets);

            float rotation;
            EBUS_EVENT_ID_RESULT(rotation, i->GetEntityId(), UiTransformBus, GetZRotation);
            i->SetNonSnappedZRotation(rotation);
        }
    }
}

void ViewportInteraction::EndReversibleAction()
{
    if (!m_reversibleActionStarted)
    {
        // Nothing to do.
        return;
    }

    m_reversibleActionStarted = false;

    if (AreaSelectionIsActive())
    {
        // Nothing to do.
        return;
    }

    m_editorWindow->GetProperties()->AfterPropertyModified(nullptr);
}

void ViewportInteraction::MousePressEvent(QMouseEvent* ev)
{
    AZ::Vector2 mousePosition = EntityHelpers::QPointFToVec2(ev->localPos());
    m_startMouseDragPos = m_lastMouseDragPos = mousePosition;
    const bool ctrlKeyPressed = ev->modifiers().testFlag(Qt::ControlModifier);
    m_reversibleActionStarted = false;

    // Prepare to handle panning
    if ((!m_leftButtonIsActive) &&
        (ev->button() == Qt::MiddleButton))
    {
        m_middleButtonIsActive = true;
    }
    else if ((!m_middleButtonIsActive) &&
             (ev->button() == Qt::LeftButton))
    {
        // Prepare for clicking and dragging

        m_leftButtonIsActive = true;

        if ((m_activeElementId.IsValid()) && m_grabbedAnchors.Any())
        {
            // Prepare to move anchors
            EBUS_EVENT_ID_RESULT(m_startAnchors, m_activeElementId, UiTransform2dBus, GetAnchors);
        }
    }

    // If there isn't another interaction happening, try to select an element
    if ((!m_spaceBarIsActive && !m_middleButtonIsActive && m_interactionType == InteractionType::NONE) ||
        m_interactionMode == InteractionMode::MOVE && m_interactionType == InteractionType::DIRECT && ctrlKeyPressed)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, m_editorWindow->GetCanvas(), UiCanvasBus, PickElement, mousePosition);

        HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();
        QTreeWidgetItem* widgetItem = nullptr;
        bool itemDeselected = false;

        // store the selected items at the start of the selection
        QTreeWidgetItemRawPtrQList selectedItems = m_editorWindow->GetHierarchy()->selectedItems();
        m_selectedElementsAtSelectionStart = SelectionHelpers::GetSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);

        if (element)
        {
            widgetItem = HierarchyHelpers::ElementToItem(hierarchyWidget, element, false);

            // If user is selecting something with the control key pressed, the
            // element may need to be de-selected (if it's already selected).
            itemDeselected = HierarchyHelpers::HandleDeselect(widgetItem, ctrlKeyPressed);
        }

        // If the item didn't need to be de-selected, then we should select it
        if (!itemDeselected)
        {
            // Note that widgetItem could still be null at this point, but
            // SetSelectedItem will handle this situation for us.
            HierarchyHelpers::SetSelectedItem(hierarchyWidget, element);
        }
    }

    UpdateCursor();
}

void ViewportInteraction::PanOnMouseMoveEvent(const AZ::Vector2& mousePosition)
{
    AZ::Vector2 deltaPosition = mousePosition - m_lastMouseDragPos;
    AZ::Vector3 mousePosDelta(EntityHelpers::MakeVec3(deltaPosition));
    m_canvasViewportMatrixProps.translation += mousePosDelta;
    UpdateCanvasToViewportMatrix();
    UpdateShouldScaleToFitOnResize();
}

const AZ::Uuid& ViewportInteraction::InitAndGetTransformComponentType()
{
    if (m_transformComponentType.IsNull())
    {
        m_transformComponentType = LyShine::UiTransform2dComponentUuid;
    }

    return m_transformComponentType;
}

void ViewportInteraction::MouseMoveEvent(QMouseEvent* ev,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    AZ::Vector2 mousePosition = EntityHelpers::QPointFToVec2(ev->localPos());

    if (m_spaceBarIsActive)
    {
        if (m_leftButtonIsActive || m_middleButtonIsActive)
        {
            PanOnMouseMoveEvent(mousePosition);
        }
    }
    else if (m_leftButtonIsActive)
    {
        // Click and drag
        ProcessInteraction(mousePosition,
            ev->modifiers(),
            selectedItems);
    }
    else if (m_middleButtonIsActive)
    {
        PanOnMouseMoveEvent(mousePosition);
    }
    else if (ev->buttons() == Qt::NoButton)
    {
        // Hover

        m_interactionType = InteractionType::NONE;
        m_grabbedEdges.SetAll(false);
        m_grabbedAnchors.SetAll(false);
        m_grabbedGizmoParts.SetBoth(false);

        UpdateInteractionType(mousePosition,
            selectedItems);
        UpdateCursor();
    }

    m_lastMouseDragPos = mousePosition;
}

void ViewportInteraction::MouseReleaseEvent(QMouseEvent* ev,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    // if the mouse press and release were in the same position and
    // no changes have been made then we can treat it as a mouse-click which can
    // do selection. This is useful in the case where we are in move mode but just
    // clicked on something that is either:
    // - one of multiple things selected and we want to just select this
    // - an element in front of something that is selected
    // In this case the mouse press will not have been treated as selection in
    // MousePressEvent so we need to handle this as a special case
    if (!m_reversibleActionStarted && m_lastMouseDragPos == m_startMouseDragPos &&
        ev->button() == Qt::LeftButton && !ev->modifiers().testFlag(Qt::ControlModifier) &&
        m_interactionMode == InteractionMode::MOVE && m_interactionType == InteractionType::DIRECT)
    {
        AZ::Vector2 mousePosition = EntityHelpers::QPointFToVec2(ev->localPos());

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, m_editorWindow->GetCanvas(), UiCanvasBus, PickElement, mousePosition);

        HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();
        if (element)
        {
            HierarchyHelpers::SetSelectedItem(hierarchyWidget, element);
        }
    }

    // Tell the Properties panel to update
    const AZ::Uuid& transformComponentType = InitAndGetTransformComponentType();
    m_editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values, &transformComponentType);

    // Tell the Properties panel that the reversible action is complete
    EndReversibleAction();

    // Reset the interaction
    ClearInteraction(false);

    if (!m_spaceBarIsActive)
    {
        // Immediately update the interaction type and cursor
        AZ::Vector2 mousePosition = EntityHelpers::QPointFToVec2(ev->localPos());
        UpdateInteractionType(mousePosition,
            selectedItems);
    }

    UpdateCursor();
}

void ViewportInteraction::MouseWheelEvent(QWheelEvent* ev)
{
    if (m_leftButtonIsActive || m_middleButtonIsActive)
    {
        // Ignore event.
        return;
    }

    const QPoint numDegrees(ev->angleDelta());

    if (!numDegrees.isNull())
    {
        // Angle delta returns distance rotated by mouse wheel in eigths of a
        // degree.
        static const int numStepsPerDegree = 8;
        const float numScrollDegress = numDegrees.y() / numStepsPerDegree;

        static const float zoomMultiplier = 1 / 100.0f;
        Vec2i pivotPoint(
            static_cast<int>(ev->posF().x()),
            static_cast<int>(ev->posF().y()));
        SetCanvasToViewportScale(m_canvasViewportMatrixProps.scale + numScrollDegress * zoomMultiplier, &pivotPoint);
    }
}

void ViewportInteraction::KeyPressEvent(QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Space)
    {
        if (!ev->isAutoRepeat())
        {
            m_spaceBarIsActive = true;
            UpdateCursor();
        }
    }
}

void ViewportInteraction::KeyReleaseEvent(QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Space)
    {
        if (!ev->isAutoRepeat())
        {
            ClearInteraction();
            UpdateCursor();
        }
    }
    else
    {
        const AZ::Uuid& transformComponentType = InitAndGetTransformComponentType();

        ViewportNudge::KeyReleaseEvent(m_editorWindow,
            m_interactionMode,
            m_editorWindow->GetViewport(),
            ev,
            m_editorWindow->GetHierarchy()->selectedItems(),
            m_coordinateSystem,
            transformComponentType);
    }
}

void ViewportInteraction::SetMode(InteractionMode m)
{
    ClearInteraction();
    m_interactionMode = m;
    PersistentSetInteractionMode(m_interactionMode);
    UpdateCoordinateSystemToolbarSection();
    m_editorWindow->GetViewport()->Refresh();
}

ViewportInteraction::InteractionMode ViewportInteraction::GetMode() const
{
    return m_interactionMode;
}

ViewportInteraction::InteractionType ViewportInteraction::GetInteractionType() const
{
    return m_interactionType;
}

void ViewportInteraction::SetCoordinateSystem(CoordinateSystem s)
{
    m_coordinateSystem = s;
    PersistentSetCoordinateSystem(s);
    m_editorWindow->GetViewport()->Refresh();
}

ViewportInteraction::CoordinateSystem ViewportInteraction::GetCoordinateSystem() const
{
    return m_coordinateSystem;
}

void ViewportInteraction::InitializeToolbars()
{
    m_editorWindow->GetModeToolbar()->SetCheckedItem(static_cast<int>(m_interactionMode));
    m_editorWindow->GetCoordinateSystemToolbarSection()->SetCurrentIndex(static_cast<int>(m_coordinateSystem));

    UpdateCoordinateSystemToolbarSection();

    AZ::Vector2 canvasSize;
    EBUS_EVENT_ID_RESULT(canvasSize, m_editorWindow->GetCanvas(), UiCanvasBus, GetCanvasSize);

    m_editorWindow->GetCanvasSizeToolbarSection()->SetInitialResolution(canvasSize);

    {
        bool isSnapping = false;
        EBUS_EVENT_ID_RESULT(isSnapping, m_editorWindow->GetCanvas(), UiCanvasBus, GetIsSnapEnabled);

        m_editorWindow->GetCoordinateSystemToolbarSection()->SetSnapToGridIsChecked(isSnapping);
    }
}

void ViewportInteraction::CenterCanvasInViewport(const AZ::Vector2* newCanvasSize)
{
    GetScaleToFitTransformProps(newCanvasSize, m_canvasViewportMatrixProps);

    // Apply scale and translation changes
    UpdateCanvasToViewportMatrix();
    m_shouldScaleToFitOnViewportResize = true;
}

void ViewportInteraction::GetScaleToFitTransformProps(const AZ::Vector2* newCanvasSize, TranslationAndScale& propsOut)
{
    AZ::Vector2 canvasSize;

    // Normally we can just get the canvas size from GetCanvasSize, but if the canvas
    // size was recently changed, the caller can choose to provide a new canvas size
    // so we don't have to wait for the canvas size to update.
    if (newCanvasSize)
    {
        canvasSize = *newCanvasSize;
    }
    else
    {
        EBUS_EVENT_ID_RESULT(canvasSize, m_editorWindow->GetCanvas(), UiCanvasBus, GetCanvasSize);
    }

    const int viewportWidth = m_editorWindow->GetViewport()->size().width();
    const int viewportHeight = m_editorWindow->GetViewport()->size().height();

    // We pad the edges of the viewport to allow the user to easily see the borders of
    // the canvas edges, which is especially helpful if there are anchors sitting on
    // the edges of the canvas.
    static const int canvasBorderPaddingInPixels = 32;
    AZ::Vector2 viewportPaddedSize(
        viewportWidth - canvasBorderPaddingInPixels,
        viewportHeight - canvasBorderPaddingInPixels);

    // Guard against very small viewports
    if (viewportPaddedSize.GetX() <= 0.0f)
    {
        viewportPaddedSize.SetX(viewportWidth);
    }

    if (viewportPaddedSize.GetY() <= 0.0f)
    {
        viewportPaddedSize.SetY(viewportHeight);
    }

    // Use a "scale to fit" approach
    const float canvasToViewportScale = AZ::GetMin<float>(
            viewportPaddedSize.GetX() / canvasSize.GetX(),
            viewportPaddedSize.GetY() / canvasSize.GetY());

    const int scaledCanvasWidth = static_cast<int>(canvasSize.GetX() * canvasToViewportScale);
    const int scaledCanvasHeight = static_cast<int>(canvasSize.GetY() * canvasToViewportScale);

    // Centers the canvas within the viewport
    propsOut.translation = AZ::Vector3(
            0.5f * (viewportWidth - scaledCanvasWidth),
            0.5f * (viewportHeight - scaledCanvasHeight),
            0.0f);

    propsOut.scale = canvasToViewportScale;
}

void ViewportInteraction::DecreaseCanvasToViewportScale()
{
    SetCanvasToViewportScale(m_canvasViewportMatrixProps.scale - defaultCanvasToViewportScaleIncrement);
}

void ViewportInteraction::IncreaseCanvasToViewportScale()
{
    SetCanvasToViewportScale(m_canvasViewportMatrixProps.scale + defaultCanvasToViewportScaleIncrement);
}

void ViewportInteraction::ResetCanvasToViewportScale()
{
    SetCanvasToViewportScale(1.0f);
}

void ViewportInteraction::SetCanvasToViewportScale(float newScale, Vec2i* optionalPivotPoint)
{
    static const float minZoom = 0.1f;
    static const float maxZoom = 10.0f;
    const float currentScale = m_canvasViewportMatrixProps.scale;
    m_canvasViewportMatrixProps.scale = AZ::GetClamp(newScale, minZoom, maxZoom);

    // Pivot the zoom based off the center of the viewport's location in canvas space
    {
        // Calculate diff between the number of viewport pixels occupied by the current
        // scaled canvas view and the new one
        AZ::Vector2 canvasSize;
        EBUS_EVENT_ID_RESULT(canvasSize, m_editorWindow->GetCanvas(), UiCanvasBus, GetCanvasSize);
        const AZ::Vector2 scaledCanvasSize(canvasSize * currentScale);
        const AZ::Vector2 newScaledCanvasSize(canvasSize * m_canvasViewportMatrixProps.scale);
        const AZ::Vector2 scaledCanvasSizeDiff(newScaledCanvasSize - scaledCanvasSize);

        // Use the center of our viewport as the pivot point
        Vec2i pivotPoint;
        if (optionalPivotPoint)
        {
            pivotPoint = *optionalPivotPoint;
        }
        else
        {
            pivotPoint = Vec2i(
                    static_cast<int>(m_editorWindow->GetViewport()->size().width() * 0.5f),
                    static_cast<int>(m_editorWindow->GetViewport()->size().height() * 0.5f));
        }

        // Get the distance between our pivot point and the upper-left corner of the
        // canvas (in viewport space)
        const Vec2i canvasUpperLeft(
            m_canvasViewportMatrixProps.translation.GetX(),
            m_canvasViewportMatrixProps.translation.GetY());
        Vec2i delta = canvasUpperLeft - pivotPoint;
        const AZ::Vector2 pivotDiff(delta.x, delta.y);

        // Calculate the pivot position relative to the current scaled canvas size. For
        // example, if the pivot position is the upper-left corner of the canvas, this
        // will be (0, 0), whereas if the pivot position is the bottom-right corner of
        // the canvas, this will be (1, 1).
        AZ::Vector2 relativePivotPosition(
            pivotDiff.GetX() / scaledCanvasSize.GetX(),
            pivotDiff.GetY() / scaledCanvasSize.GetY());

        // Use the relative pivot position to essentially determine what percentage of
        // the difference between the two on-screen canvas sizes should be used to move
        // the canvas by to pivot the zoom. For example, if the pivot position is the
        // bottom-right corner of the canvas, then we will use 100% of the difference
        // in on-screen canvas sizes to move the canvas right and up (to maintain the
        // view of the bottom-right corner).
        AZ::Vector2 pivotTranslation(
            scaledCanvasSizeDiff.GetX() * relativePivotPosition.GetX(),
            scaledCanvasSizeDiff.GetY() * relativePivotPosition.GetY());

        m_canvasViewportMatrixProps.translation.SetX(m_canvasViewportMatrixProps.translation.GetX() + pivotTranslation.GetX());
        m_canvasViewportMatrixProps.translation.SetY(m_canvasViewportMatrixProps.translation.GetY() + pivotTranslation.GetY());
    }

    UpdateCanvasToViewportMatrix();
    UpdateShouldScaleToFitOnResize();
}

void ViewportInteraction::UpdateZoomFactorLabel()
{
    float percentage = m_canvasViewportMatrixProps.scale * 100.0f;
    m_editorWindow->GetMainToolbar()->GetZoomFactorLabel()->setText(QString("Zoom: %1 %").arg(QString::number(percentage, 'f', 0)));
}

AZ::Entity* ViewportInteraction::GetActiveElement() const
{
    return EntityHelpers::GetEntity(m_activeElementId);
}

const AZ::EntityId& ViewportInteraction::GetActiveElementId() const
{
    return m_activeElementId;
}

ViewportHelpers::SelectedAnchors ViewportInteraction::GetGrabbedAnchors() const
{
    return m_grabbedAnchors;
}

void ViewportInteraction::UpdateInteractionType(const AZ::Vector2& mousePosition,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    switch (m_interactionMode)
    {
    case InteractionMode::MOVE:
    {
        auto selectedElements = SelectionHelpers::GetSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        if (selectedElements.size() == 1)
        {
            auto selectedElement = selectedElements.front();
            if (!ViewportHelpers::IsControlledByLayout(selectedElement) &&
                ViewportElement::PickAnchors(selectedElement, mousePosition, m_anchorWhole->GetTextureSize(), m_grabbedAnchors))
            {
                // Hovering over anchors
                m_interactionType = InteractionType::ANCHORS;
                m_activeElementId = selectedElement->GetId();
                return;
            }
        }

        auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        for (auto element : topLevelSelectedElements)
        {
            if (!ViewportHelpers::IsControlledByLayout(element) &&
                ViewportElement::PickAxisGizmo(element, m_coordinateSystem, m_interactionMode, mousePosition, m_lineTriangleX->GetTextureSize(), m_grabbedGizmoParts))
            {
                // Hovering over move gizmo
                m_interactionType = InteractionType::TRANSFORM_GIZMO;
                m_activeElementId = element->GetId();
                return;
            }
        }

        for (auto element : selectedElements)
        {
            bool isElementUnderCursor = false;
            EBUS_EVENT_ID_RESULT(isElementUnderCursor, element->GetId(), UiTransformBus, IsPointInRect, mousePosition);

            if (isElementUnderCursor)
            {
                // Hovering over a selected element
                m_interactionType = InteractionType::DIRECT;
                m_activeElementId = element->GetId();
                return;
            }
        }
        break;
    }
    case InteractionMode::ROTATE:
    {
        auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        for (auto element : topLevelSelectedElements)
        {
            if (ViewportElement::PickPivot(element, mousePosition, m_pivotIcon->GetTextureSize()))
            {
                // Hovering over pivot
                m_interactionType = InteractionType::PIVOT;
                m_activeElementId = element->GetId();
                return;
            }
        }
        for (auto element : topLevelSelectedElements)
        {
            if (ViewportElement::PickCircleGizmo(element, mousePosition, m_circle->GetTextureSize(), m_grabbedGizmoParts))
            {
                // Hovering over rotate gizmo
                m_interactionType = InteractionType::TRANSFORM_GIZMO;
                m_activeElementId = element->GetId();
                return;
            }
        }
        break;
    }
    case InteractionMode::RESIZE:
    {
        auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        for (auto element : topLevelSelectedElements)
        {
            if (!ViewportHelpers::IsControlledByLayout(element) &&
                ViewportElement::PickAxisGizmo(element, CoordinateSystem::LOCAL, m_interactionMode, mousePosition, m_lineTriangleX->GetTextureSize(), m_grabbedGizmoParts))
            {
                // Hovering over resize gizmo
                m_interactionType = InteractionType::TRANSFORM_GIZMO;
                m_activeElementId = element->GetId();
                return;
            }
        }

        auto selectedElements = SelectionHelpers::GetSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
        for (auto element : selectedElements)
        {
            if (!ViewportHelpers::IsControlledByLayout(element))
            {
                // Check for grabbing element edges
                ViewportElement::PickElementEdges(element, mousePosition, g_elementEdgeForgiveness, m_grabbedEdges);
                if (m_grabbedEdges.BothHorizontal() || m_grabbedEdges.BothVertical())
                {
                    // Don't grab both opposite edges
                    m_grabbedEdges.SetAll(false);
                }

                if (m_grabbedEdges.Any())
                {
                    m_interactionType = InteractionType::DIRECT;
                    m_activeElementId = element->GetId();
                    return;
                }
            }
        }
        break;
    }
    default:
    {
        // Do nothing
        break;
    }
    } // switch statement
}

void ViewportInteraction::UpdateCursor()
{
    QCursor cursor = Qt::ArrowCursor;

    if (m_spaceBarIsActive)
    {
        cursor = (m_leftButtonIsActive || m_middleButtonIsActive ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
    }
    else if (m_activeElementId.IsValid())
    {
        if (m_interactionMode == InteractionMode::MOVE &&
            m_interactionType == InteractionType::DIRECT)
        {
            cursor = Qt::SizeAllCursor;
        }
        else if (m_interactionMode == InteractionMode::ROTATE &&
                 m_interactionType == InteractionType::TRANSFORM_GIZMO)
        {
            cursor = cursorRotate;
        }
        else if (m_interactionMode == InteractionMode::RESIZE &&
                 m_interactionType == InteractionType::DIRECT)
        {
            UiTransformInterface::RectPoints rect;
            EBUS_EVENT_ID(m_activeElementId, UiTransformBus, GetViewportSpacePoints, rect);

            float topAngle = RAD2DEG(atan2f(rect.TopRight().GetY() - rect.TopLeft().GetY(), rect.TopRight().GetX() - rect.TopLeft().GetX()));
            float leftAngle = RAD2DEG(atan2f(rect.TopLeft().GetY() - rect.BottomLeft().GetY(), rect.TopLeft().GetX() - rect.BottomLeft().GetX()));
            float topLeftAngle = 0.5f * (topAngle + leftAngle);
            float topRightAngle = ViewportHelpers::GetPerpendicularAngle(topLeftAngle);

            if (m_grabbedEdges.TopLeft() || m_grabbedEdges.BottomRight())
            {
                cursor = ViewportHelpers::GetSizingCursor(topLeftAngle);
            }
            else if (m_grabbedEdges.TopRight() || m_grabbedEdges.BottomLeft())
            {
                cursor = ViewportHelpers::GetSizingCursor(topRightAngle);
            }
            else if (m_grabbedEdges.m_left || m_grabbedEdges.m_right)
            {
                cursor = ViewportHelpers::GetSizingCursor(leftAngle);
            }
            else if (m_grabbedEdges.m_top || m_grabbedEdges.m_bottom)
            {
                cursor = ViewportHelpers::GetSizingCursor(topAngle);
            }
        }
    }

    m_editorWindow->GetViewport()->setCursor(cursor);
}

void ViewportInteraction::UpdateCanvasToViewportMatrix()
{
    AZ::Vector3 scaleVec3(m_canvasViewportMatrixProps.scale, m_canvasViewportMatrixProps.scale, 1.0f);
    AZ::Matrix4x4 updatedMatrix = AZ::Matrix4x4::CreateScale(scaleVec3);
    updatedMatrix.SetTranslation(m_canvasViewportMatrixProps.translation);

    EBUS_EVENT_ID(m_editorWindow->GetCanvas(), UiCanvasBus, SetCanvasToViewportMatrix, updatedMatrix);

    UpdateZoomFactorLabel();
}

void ViewportInteraction::UpdateShouldScaleToFitOnResize()
{
    // If the current viewport matrix props match the "scale to fit" props,
    // the canvas will scale to fit when the viewport resizes.
    TranslationAndScale props;
    GetScaleToFitTransformProps(nullptr, props);
    m_shouldScaleToFitOnViewportResize = (props == m_canvasViewportMatrixProps);
}

void ViewportInteraction::ProcessInteraction(const AZ::Vector2& mousePosition,
    Qt::KeyboardModifiers modifiers,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    // Get the mouse move delta, which is in viewport space.
    AZ::Vector2 delta = mousePosition - m_lastMouseDragPos;
    AZ::Vector3 mouseTranslation(delta.GetX(), delta.GetY(), 0.0f);

    BeginReversibleAction(selectedItems);

    bool ctrlIsPressed = modifiers.testFlag(Qt::ControlModifier);
    if (m_interactionType == InteractionType::NONE)
    {
        float mouseDragDistance2 = (mousePosition - m_startMouseDragPos).GetLengthSq();
        if (mouseDragDistance2 >= g_minAreaSelectionDistance2)
        {
            // Area selection
            AZ::Vector2 rectMin(min(m_startMouseDragPos.GetX(), mousePosition.GetX()), min(m_startMouseDragPos.GetY(), mousePosition.GetY()));
            AZ::Vector2 rectMax(max(m_startMouseDragPos.GetX(), mousePosition.GetX()), max(m_startMouseDragPos.GetY(), mousePosition.GetY()));

            LyShine::EntityArray elementsToSelect;
            EBUS_EVENT_ID_RESULT(elementsToSelect, m_editorWindow->GetCanvas(), UiCanvasBus, PickElements, rectMin, rectMax);

            if (ctrlIsPressed)
            {
                // NOTE: We are fighting against SetSelectedItems a bit here. SetSelectedItems uses Qt
                // to set the selection and the control and shift modifiers affect its behavior.
                // When Ctrl is down, unless you pass null or an empty list it adds to the existing
                // selected items. To get the behavior we want when ctrl is held down we have to clear
                // the selection before setting it. NOTE: if you area select over a group and (during
                // same drag) move the cursor so that they are not in the box then they should not
                // be added to the selection.
                HierarchyHelpers::SetSelectedItem(m_editorWindow->GetHierarchy(), nullptr);

                // when control is pressed we add the selected elements in a drag select to the already selected elements
                // NOTE: It would be nice to allow ctrl-area-select to deselect already selected items. However, the main
                // level editor does not behave that way and we are trying to be consistent (see LMBR-10377)
                for (auto element : m_selectedElementsAtSelectionStart)
                {
                    // if not already in the selectedElements then add it
                    auto iter = AZStd::find(elementsToSelect.begin(), elementsToSelect.end(), element);
                    if (iter == elementsToSelect.end())
                    {
                        elementsToSelect.push_back(element);
                    }
                }
            }

            HierarchyHelpers::SetSelectedItems(m_editorWindow->GetHierarchy(), &elementsToSelect);
        }
        else
        {
            // Selection area too small, ignore
        }
    }
    else if (m_interactionType == InteractionType::PIVOT)
    {
        // Move the pivot that was grabbed
        ViewportElement::MovePivot(m_lastMouseDragPos, EntityHelpers::GetEntity(m_activeElementId), mousePosition);
    }
    else if (m_interactionType == InteractionType::ANCHORS)
    {
        // Move the anchors of the active element
        ViewportElement::MoveAnchors(m_grabbedAnchors, m_startAnchors, m_startMouseDragPos, EntityHelpers::GetEntity(m_activeElementId), mousePosition, ctrlIsPressed);
    }
    else if (m_interactionType == InteractionType::TRANSFORM_GIZMO)
    {
        // Transform all selected elements by interacting with one element's transform gizmo
        switch (m_interactionMode)
        {
        case InteractionMode::MOVE:
            ViewportElement::Move(m_editorWindow->GetHierarchy(), selectedItems, EntityHelpers::GetEntity(m_activeElementId), m_editorWindow->GetCanvas(), m_coordinateSystem, m_grabbedGizmoParts, m_interactionType, mouseTranslation);
            break;
        case InteractionMode::ROTATE:
        {
            LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
            for (auto element : selectedElements)
            {
                ViewportElement::Rotate(m_editorWindow->GetHierarchy(), m_editorWindow->GetCanvas(), m_lastMouseDragPos, m_activeElementId, element, mousePosition);
            }
        }
        break;
        case InteractionMode::RESIZE:
        {
            if (!ViewportHelpers::IsControlledByLayout(EntityHelpers::GetEntity(m_activeElementId)))
            {
                LyShine::EntityArray selectedElements = SelectionHelpers::GetTopLevelSelectedElements(m_editorWindow->GetHierarchy(), selectedItems);
                for (auto element : selectedElements)
                {
                    ViewportElement::ResizeByGizmo(m_editorWindow->GetHierarchy(), m_editorWindow->GetCanvas(), m_grabbedGizmoParts, m_activeElementId, element, mouseTranslation);
                }
            }
        }
        break;
        default:
            AZ_Assert(0, "Unexpected combination of m_interactionMode and m_interactionType.");
            break;
        }
    }
    else if (m_interactionType == InteractionType::DIRECT)
    {
        // Transform all selected elements by interacting with one element directly
        switch (m_interactionMode)
        {
        case InteractionMode::MOVE:
            ViewportElement::Move(m_editorWindow->GetHierarchy(), selectedItems, EntityHelpers::GetEntity(m_activeElementId), m_editorWindow->GetCanvas(), m_coordinateSystem, m_grabbedGizmoParts, m_interactionType, mouseTranslation);
            break;
        case InteractionMode::RESIZE:
            // Exception: Direct resizing (grabbing an edge) only affects the element you grabbed
            ViewportElement::ResizeDirectly(m_editorWindow->GetHierarchy(), m_editorWindow->GetCanvas(), m_grabbedEdges, EntityHelpers::GetEntity(m_activeElementId), mouseTranslation);
            break;
        default:
            AZ_Assert(0, "Unexpected combination of m_interactionMode and m_interactionType.");
            break;
        }
    }
    else
    {
        AZ_Assert(0, "Unexpected value for m_interactionType.");
    }

    // Tell the Properties panel to update
    const AZ::Uuid& transformComponentType = InitAndGetTransformComponentType();
    m_editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values, &transformComponentType);
}

void ViewportInteraction::DrawAxisGizmo(Draw2dHelper& draw2d, const AZ::Entity* element, CoordinateSystem coordinateSystem, const ViewportIcon* lineTextureX, const ViewportIcon* lineTextureY)
{
    if (UiTransformBus::FindFirstHandler(element->GetId()))
    {
        AZ::Vector2 pivotPosition;
        AZ::Matrix4x4 transform;

        if (coordinateSystem == CoordinateSystem::LOCAL)
        {
            EBUS_EVENT_ID_RESULT(pivotPosition, element->GetId(), UiTransformBus, GetCanvasSpacePivotNoScaleRotate);

            // LOCAL MOVE in the parent element's LOCAL space.
            AZ::EntityId elementId(m_interactionMode == InteractionMode::MOVE ? EntityHelpers::GetParentElement(element)->GetId() : element->GetId());
            EBUS_EVENT_ID(elementId, UiTransformBus, GetTransformToViewport, transform);
        }
        else
        {
            // View coordinate system: do everything in viewport space
            EBUS_EVENT_ID_RESULT(pivotPosition, element->GetId(), UiTransformBus, GetViewportSpacePivot);
            transform = AZ::Matrix4x4::CreateIdentity();
        }

        // Draw up axis
        if (m_interactionMode == InteractionMode::MOVE || !ViewportHelpers::IsVerticallyFit(element))
        {
            AZ::Color color = ((m_activeElementId == element->GetId()) && m_grabbedGizmoParts.m_top) ? ViewportHelpers::highlightColor : ViewportHelpers::yColor;
            lineTextureY->Draw(draw2d, pivotPosition, transform, 0.0f, color);
        }

        // Draw right axis
        if (m_interactionMode == InteractionMode::MOVE || !ViewportHelpers::IsHorizontallyFit(element))
        {
            AZ::Color color = ((m_activeElementId == element->GetId()) && m_grabbedGizmoParts.m_right) ? ViewportHelpers::highlightColor : ViewportHelpers::xColor;
            lineTextureX->Draw(draw2d, pivotPosition, transform, 0.0f, color);
        }

        // Draw center square
        if (m_interactionMode == InteractionMode::MOVE || !ViewportHelpers::IsHorizontallyFit(element) && !ViewportHelpers::IsVerticallyFit(element))
        {
            AZ::Color color = ((m_activeElementId == element->GetId()) && m_grabbedGizmoParts.Both()) ? ViewportHelpers::highlightColor : ViewportHelpers::zColor;
            m_centerSquare->Draw(draw2d, pivotPosition, transform, 0.0f, color);
        }
    }
}

void ViewportInteraction::DrawCircleGizmo(Draw2dHelper& draw2d, const AZ::Entity* element)
{
    if (UiTransformBus::FindFirstHandler(element->GetId()))
    {
        AZ::Vector2 pivotPosition;
        EBUS_EVENT_ID_RESULT(pivotPosition, element->GetId(), UiTransformBus, GetViewportSpacePivot);

        // Draw circle
        AZ::Color color = ((m_activeElementId == element->GetId()) && m_interactionType == InteractionType::TRANSFORM_GIZMO) ? ViewportHelpers::highlightColor : ViewportHelpers::zColor;
        m_circle->Draw(draw2d, pivotPosition, AZ::Matrix4x4::CreateIdentity(), 0.0f, color);
    }
}

void ViewportInteraction::UpdateCoordinateSystemToolbarSection()
{
    // the coordinate system toolbar should only be enabled in move mode
    m_editorWindow->GetCoordinateSystemToolbarSection()->SetIsEnabled(m_interactionMode == InteractionMode::MOVE);
}

#include <ViewportInteraction.moc>
