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

#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QPoint>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QTimer>
#include <QWheelEvent>

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>

namespace GraphCanvas
{
    //! GraphCanvasGraphicsView
    //! CanvasWidget that should be used to display GraphCanvas scenes.
    //! Will provide a lot of basic UX functionality.
    class GraphCanvasGraphicsView
        : public QGraphicsView
        , protected ViewRequestBus::Handler
        , protected SceneNotificationBus::Handler
    {
    private:
        const qreal ZOOM_MIN = 0.1;
        const qreal ZOOM_MAX = 2.0;
        const int KEYBOARD_MOVE = 50;
        const int WHEEL_ZOOM = 120;

    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasGraphicsView, AZ::SystemAllocator, 0);

        GraphCanvasGraphicsView();
        ~GraphCanvasGraphicsView();

        const ViewId& GetViewId() const;

        // ViewRequestBus::Handler
        void SetEditorId(const EditorId& editorId) override;
        EditorId GetEditorId() const override;

        void SetScene(const AZ::EntityId& sceneId) override;
        AZ::EntityId GetScene() const override;

        void ClearScene() override;

        AZ::Vector2 GetViewSceneCenter() const override;

        AZ::Vector2 MapToScene(const AZ::Vector2& view) override;
        AZ::Vector2 MapFromScene(const AZ::Vector2& scene) override;

        void SetViewParams(const ViewParams& viewParams) override;
        void DisplayArea(const QRectF& viewArea) override;
        void CenterOnArea(const QRectF& viewArea) override;

        void CenterOn(const QPointF& posInSceneCoordinates) override;
        QRectF GetCompleteArea() override;
        void WheelEvent(QWheelEvent* ev) override;
        QRectF GetViewableAreaInSceneCoordinates() override;

        GraphCanvasGraphicsView* AsGraphicsView() override;
        ////

        QRectF GetSelectedArea();

        // SceneNotificationBus::Handler
        void OnStylesChanged() override;
        void OnNodeIsBeingEdited(bool isEditing) override;
        ////

        // QGraphicsView
        void keyReleaseEvent(QKeyEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        void wheelEvent(QWheelEvent* event) override;

        void focusOutEvent(QFocusEvent* event);

        void resizeEvent(QResizeEvent* event) override;
        void scrollContentsBy(int dx, int dy) override;
        ////

        bool GetIsEditing(){ return m_isEditing; }

    protected:

        void CreateBookmark(int bookmarkShortcut);
        void JumpToBookmark(int bookmarkShortcut);        

    private:

        void ConnectBoundsSignals();
        void DisconnectBoundsSignals();
        void OnBoundsChanged();

        void QueueSave();
        void SaveViewParams();

        void OnRubberBandChanged(QRect rubberBandRect, QPointF fromScenePoint, QPointF toScenePoint);

        GraphCanvasGraphicsView(const GraphCanvasGraphicsView&) = delete;        

        ViewId       m_viewId;
        AZ::EntityId m_sceneId;
        EditorId     m_editorId;

        bool m_isDragSelecting;

        bool   m_checkForDrag;
        QPoint m_initialClick;

        bool m_ignoreValueChange;
        bool m_reapplyViewParams;

        ViewParams m_viewParams;

        QTimer m_timer;
        QTimer m_styleTimer;

        bool m_isEditing;
    };
}
