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
#pragma once

class ViewportInteraction
    : public QObject
{
    Q_OBJECT

public:

    ViewportInteraction(EditorWindow* editorWindow);
    virtual ~ViewportInteraction();

    bool GetLeftButtonIsActive();
    bool GetSpaceBarIsActive();

    void Draw(Draw2dHelper& draw2d,
        QTreeWidgetItemRawPtrQList& selectedItems);

    void MousePressEvent(QMouseEvent* ev);
    void MouseMoveEvent(QMouseEvent* ev,
        QTreeWidgetItemRawPtrQList& selectedItems);
    void MouseReleaseEvent(QMouseEvent* ev,
        QTreeWidgetItemRawPtrQList& selectedItems);
    void MouseWheelEvent(QWheelEvent* ev);

    void KeyPressEvent(QKeyEvent* ev);
    void KeyReleaseEvent(QKeyEvent* ev);

    //! Mode of interaction in the viewport
    //! This is driven by a toolbar.
    enum class InteractionMode
    {
        SELECTION,
        MOVE,
        ROTATE,
        RESIZE
    };

    //! Type of coordinate system in the viewport
    enum class CoordinateSystem
    {
        LOCAL,
        VIEW
    };

    //! Type of interaction in the viewport
    //! This is driven by hovering and the left mouse button.
    enum class InteractionType
    {
        DIRECT, //!< The bounding box.
        TRANSFORM_GIZMO, //!< The base axes or circular manipulator.
        ANCHORS,
        PIVOT, //!< The dot.
        NONE
    };

    void SetMode(InteractionMode mode);
    InteractionMode GetMode() const;

    InteractionType GetInteractionType() const;

    AZ::Entity* GetActiveElement() const;
    const AZ::EntityId& GetActiveElementId() const;

    ViewportHelpers::SelectedAnchors GetGrabbedAnchors() const;

    void SetCoordinateSystem(CoordinateSystem s);
    CoordinateSystem GetCoordinateSystem() const;

    void InitializeToolbars();

    float GetCanvasToViewportScale() const { return m_canvasViewportMatrixProps.scale; }
    AZ::Vector3 GetCanvasToViewportTranslation() const { return m_canvasViewportMatrixProps.translation; }

    //! Centers the entirety of the canvas so that it's viewable within the viewport.
    //
    //! The scale of the canvas-to-viewport matrix is decreased (zoomed out) for
    //! canvases that are bigger than the viewport, and increased (zoomed in) for
    //! canvases that are smaller than the viewport. This scaled view of the canvas
    //! is then use to center the canvas within the viewport.
    //
    //! \param newCanvasSize Because of a one-frame delay in canvas size, if the canvas
    //! size was recently changed, and the caller knows the new canvas size, the size
    //! can be passed to this function to be immediately applied.
    void CenterCanvasInViewport(const AZ::Vector2* newCanvasSize = nullptr);

    //! "Zooms out" the view of the canvas in the viewport by an incremental amount
    void DecreaseCanvasToViewportScale();

    //! "Zooms in" the view of the canvas in the viewport by an incremental amount
    void IncreaseCanvasToViewportScale();

    //! Assigns a scale of 1.0 to the canvas-to-viewport matrix.
    void ResetCanvasToViewportScale();

    //! Return whether the canvas should be scaled to fit when the viewport is resized
    bool ShouldScaleToFitOnViewportResize() const { return m_shouldScaleToFitOnViewportResize; }

    void UpdateZoomFactorLabel();

    //! Reset all transform interaction variables except the interaction mode
    void ClearInteraction(bool clearSpaceBarIsActive = true);

private: // types

    struct TranslationAndScale
    {
        TranslationAndScale()
            : translation(0.0)
            , scale(1.0f) { }

        bool operator == (const TranslationAndScale& other) const
        {
            return translation == other.translation && scale == other.scale;
        }

        AZ::Vector3 translation;
        float scale;
    };

private: // member functions

    //! Update the interaction type based on where the cursor is right now
    void UpdateInteractionType(const AZ::Vector2& mousePosition,
        QTreeWidgetItemRawPtrQList& selectedItems);

    //! Update the cursor based on the current interaction
    void UpdateCursor();

    //! Should be called when our translation and scale properties change for the canvas-to-viewport matrix.
    void UpdateCanvasToViewportMatrix();

    //! Assigns the given scale to the canvas-to-viewport matrix, clamped between 0.1 and 10.0.
    //
    //! Note that this method is private since ViewportInteraction matrix exclusively manages
    //! the viewport-to-canvas matrix.
    void SetCanvasToViewportScale(float newScale, Vec2i* optionalPivotPoint = nullptr);

    void GetScaleToFitTransformProps(const AZ::Vector2* newCanvasSize, TranslationAndScale& propsOut);

    //! Called when a pan or a zoom is performed.
    //! Updates flag that determines whether the canvas will scale to fit when the viewport resizes.
    void UpdateShouldScaleToFitOnResize();

    //! Process click and drag interaction
    void ProcessInteraction(const AZ::Vector2& mousePosition,
        Qt::KeyboardModifiers modifiers,
        QTreeWidgetItemRawPtrQList& selectedItems);

    // Draw a transform gizmo on the element
    void DrawAxisGizmo(Draw2dHelper& draw2d, const AZ::Entity* element, CoordinateSystem coordinateSystem, const ViewportIcon* lineTextureX, const ViewportIcon* lineTextureY);
    void DrawCircleGizmo(Draw2dHelper& draw2d, const AZ::Entity* element);

    // The coordinate system toolbar updates based on the interaction mode and coordinate system setting
    void UpdateCoordinateSystemToolbarSection();

    bool AreaSelectionIsActive();

    void BeginReversibleAction(QTreeWidgetItemRawPtrQList& selectedItems);
    void EndReversibleAction();

    void PanOnMouseMoveEvent(const AZ::Vector2& mousePosition);

    const AZ::Uuid& InitAndGetTransformComponentType();

private: // data

    EditorWindow* m_editorWindow;

    // The element that is being interacted with
    AZ::EntityId m_activeElementId;

    // Used for anchor picking
    std::unique_ptr< ViewportIcon > m_anchorWhole;

    // Used for pivot picking
    std::unique_ptr< ViewportIcon > m_pivotIcon;

    // Used for transform interaction
    InteractionMode m_interactionMode;
    InteractionType m_interactionType;
    CoordinateSystem m_coordinateSystem;
    bool m_spaceBarIsActive;                                    //!< True when the spacebar is held down, false otherwise.
    bool m_leftButtonIsActive;                                  //!< True when the left mouse button is down, false otherwise.
    bool m_middleButtonIsActive;                                //!< True when the middle mouse button is down, false otherwise.
    bool m_reversibleActionStarted;
    AZ::Vector2 m_startMouseDragPos;
    AZ::Vector2 m_lastMouseDragPos;
    LyShine::EntityArray m_selectedElementsAtSelectionStart;
    TranslationAndScale m_canvasViewportMatrixProps;            //!< Stores translation and scale properties for canvasToViewport matrix.
                                                                //!< Used for zoom and pan functionality.
    bool m_shouldScaleToFitOnViewportResize;

    // Used to refresh the properties panel
    AZ::Uuid m_transformComponentType;

    ViewportHelpers::ElementEdges m_grabbedEdges;
    UiTransform2dInterface::Anchors m_startAnchors;
    ViewportHelpers::SelectedAnchors m_grabbedAnchors;
    ViewportHelpers::GizmoParts m_grabbedGizmoParts;

    // Used for drawing the transform gizmo
    std::unique_ptr<ViewportIcon> m_lineTriangleX;
    std::unique_ptr<ViewportIcon> m_lineTriangleY;
    std::unique_ptr<ViewportIcon> m_circle;
    std::unique_ptr<ViewportIcon> m_lineSquareX;
    std::unique_ptr<ViewportIcon> m_lineSquareY;
    std::unique_ptr<ViewportIcon> m_centerSquare;

    // Used for rubber band selection
    std::unique_ptr< ViewportIcon > m_dottedLine;

    std::unique_ptr< ViewportInteraction > m_viewportInteraction;
};

ADD_ENUM_CLASS_ITERATION_OPERATORS(ViewportInteraction::InteractionMode,
    ViewportInteraction::InteractionMode::SELECTION,
    ViewportInteraction::InteractionMode::RESIZE);

ADD_ENUM_CLASS_ITERATION_OPERATORS(ViewportInteraction::CoordinateSystem,
    ViewportInteraction::CoordinateSystem::LOCAL,
    ViewportInteraction::CoordinateSystem::VIEW);
