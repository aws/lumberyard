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

#include <QApplication>
#include <QKeyEvent>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QPoint>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QTimer>
#include <QWheelEvent>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>

namespace GraphCanvas
{
	//! CanvasGraphicsView
	//! CanvasWidget that should be used to display GraphCanvas scenes.
	//! Will provide a lot of basic UX functionality.
    class CanvasGraphicsView
        : public QGraphicsView
        , protected ViewRequestBus::Handler
        , protected SceneNotificationBus::Handler
    {
    private:
        const double ZOOM_MIN = 0.1;
        const double ZOOM_MAX = 2.0;
        const int KEYBOARD_MOVE = 50;
    public:
        AZ_CLASS_ALLOCATOR(CanvasGraphicsView, AZ::SystemAllocator, 0);

        CanvasGraphicsView()
            : QGraphicsView()
            , m_isDragSelecting(false)
            , m_checkForDrag(false)
            , m_ignoreValueChange(false)
            , m_reapplyViewParams(false)
            , m_isEditing(false)
        {
            m_viewId = AZ::Entity::MakeId();

            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
            setDragMode(QGraphicsView::RubberBandDrag);
            setAlignment(Qt::AlignTop | Qt::AlignLeft);
            setCacheMode(QGraphicsView::CacheBackground);

            m_timer.setSingleShot(true);
            m_timer.setInterval(250);
            m_timer.stop();

            m_styleTimer.setSingleShot(true);
            m_styleTimer.setInterval(250);
            m_styleTimer.stop();

            connect(&m_timer, &QTimer::timeout, this, &CanvasGraphicsView::SaveViewParams);
            connect(&m_styleTimer, &QTimer::timeout, this, &CanvasGraphicsView::DisconnectBoundsSignals);

            connect(this, &QGraphicsView::rubberBandChanged, this, &CanvasGraphicsView::OnRubberBandChanged);

            ViewRequestBus::Handler::BusConnect(GetId());
        }

        ~CanvasGraphicsView()
        {
            ClearScene();
        }

        const ViewId& GetId() const
        {
            return m_viewId;
        }

        // ViewRequestBus::Handler
        void SetScene(const AZ::EntityId& sceneId) override
        {
            if (!sceneId.IsValid())
            {
                return;
            }

            if (SceneRequestBus::FindFirstHandler(sceneId) == nullptr)
            {
                AZ_Assert(false, "Couldn't find the Scene component on entity with ID %s", sceneId.ToString().data());
                return;
            }

            if (m_sceneId == sceneId)
            {
                return;
            }

            ClearScene();
            m_sceneId = sceneId;

            QGraphicsScene* graphicsScene = nullptr;
            SceneRequestBus::EventResult(graphicsScene, m_sceneId, &SceneRequests::AsQGraphicsScene);
            setScene(graphicsScene);

            SceneNotificationBus::Handler::BusConnect(m_sceneId);

            SceneRequestBus::Event(m_sceneId, &SceneRequests::RegisterView, GetId());

            ConnectBoundsSignals();
            OnBoundsChanged();
        }

        AZ::EntityId GetScene() const override
        {
            return m_sceneId;
        }

        void ClearScene() override
        {
            if (m_sceneId.IsValid())
            {
                SceneRequestBus::Event(m_sceneId, &SceneRequests::RemoveView, GetId());
                SceneNotificationBus::Handler::BusDisconnect(m_sceneId);
            }

            m_sceneId.SetInvalid();
            setScene(nullptr);
        }
        
        AZ::Vector2 MapToScene(const AZ::Vector2& view) override
        {
            QPointF mapped = mapToScene(QPoint(view.GetX(), view.GetY()));
            return AZ::Vector2(mapped.x(), mapped.y());
        }

        AZ::Vector2 MapFromScene(const AZ::Vector2& scene) override
        {
            QPoint mapped = mapFromScene(QPointF(scene.GetX(), scene.GetY()));
            return AZ::Vector2(mapped.x(), mapped.y());
        }

        void SetViewParams(const ViewParams& viewParams) override
        {
            // Fun explanation time.
            //
            // The GraphicsView is pretty chaotic with its bounds, as it depends on the scene,
            // the size of the widget, and the scale value to determine it's ultimate value.
            // Because of this, and our start-up process, there are 3-4 different timings of when
            // widgets are added, when they have data, when they display, and when they process events.
            // All this means that this function in particular does not have a consistent point of execution
            // in all of this.
            // 
            // A small deviation might be alright, and not immediately noticeable, but since the range this view is
            // operating in is about 200k, a deviation of half a percent is incredibly noticeable.
            //
            // The fix: Listen to all of the events affecting our range(range change, value change for the scrollbars) for a period of time, 
            // and constantly re-align ourselves to a fixed left point(also how the graph expands, so this appears fairly natural)
            // in terms of how it handles the window being resized in between loads. Once a reasonable(read magic) amount of time
            // has passed in between resizes, stop doing this, and only listen to events we care about
            // (range change for recalculating our saved anchor point)
            //
            // This oddity handles all of our weird resizing timings.
            m_styleTimer.setInterval(2000);
            m_styleTimer.setSingleShot(true);
            m_styleTimer.start();

            ConnectBoundsSignals();

            // scale back to 1.0
            double scaleValue = 1.0 / m_viewParams.m_scale;

            // Apply the new scale
            scaleValue *= viewParams.m_scale;

            // Update view params before setting scale
            m_viewParams = viewParams;

            // Set out scale
            scale(scaleValue, scaleValue);
            OnBoundsChanged();
        }

        void FrameSelectionIntoView() override
        {
            QList<QGraphicsItem *> itemsList = items();
            QList<QGraphicsItem *> selectedList;
            QRectF selectedArea;
            QRectF completeArea;
            QRectF area;

            // Get the grid.
            AZ::EntityId gridId;
            SceneRequestBus::EventResult(gridId, m_sceneId, &SceneRequests::GetGrid);

            QGraphicsItem* gridItem;
            VisualRequestBus::EventResult(gridItem, gridId, &VisualRequests::AsGraphicsItem);

            // Get the area.
            for(auto item : itemsList)
            {
                if (item == gridItem)
                {
                    // Ignore the grid.
                    continue;
                }

                completeArea |= item->sceneBoundingRect();

                if(item->isSelected())
                {
                    selectedList.push_back(item);
                    selectedArea |= item->sceneBoundingRect();
                }
            }

            if (selectedList.empty())
            {
                if(itemsList.size() > 1) // >1 because the grid is always contained.
                {
                    // Show everything.
                    area = completeArea;
                }
            }
            else
            {
                // Show selection.
                area = selectedArea;
            }

            // Fit into view.
            fitInView(area, Qt::KeepAspectRatio);
            AZ_Assert(transform().m11() == transform().m22(), "The scale DIDN'T preserve the aspect ratio.");

            // Clamp the scaling factor.
            m_viewParams.m_scale = AZ::GetClamp(transform().m11(), ZOOM_MIN, ZOOM_MAX);
            QTransform m = transform();
            m.setMatrix(m_viewParams.m_scale, m.m12(), m.m13(), m.m21(), m_viewParams.m_scale, m.m23(), m.m31(), m.m32(), m.m33());
            setTransform(m);
        }

        QGraphicsView* AsQGraphicsView() override
        {
            return this;
        }
        ////

        // SceneNotificationBus::Handler
        void OnStyleSheetChanged() override
        {
            update();
        }

        void OnNodeIsBeingEdited(bool isEditing)
        {
            m_isEditing = isEditing;
        }
        ////

    protected:
        // QGraphicsView
        void keyReleaseEvent(QKeyEvent* event) override
        {
            switch (event->key())
            {
            case Qt::Key_Z:
                if (rubberBandRect().isNull() && !QApplication::mouseButtons() && !QApplication::keyboardModifiers())
                {
                    if (!m_isEditing)
                    {
                        FrameSelectionIntoView();
                    }
                }
                break;
            case Qt::Key_Control:
                if (rubberBandRect().isNull() && !QApplication::mouseButtons())
                {
                    setDragMode(QGraphicsView::RubberBandDrag);
                    setInteractive(true);
                }
                break;
            case Qt::Key_Alt:
            {
                static const bool enabled = false;
                ViewSceneNotificationBus::Event(GetScene(), &ViewSceneNotifications::OnAltModifier, enabled);
                break;
            }
            default:
                break;
            }

            QGraphicsView::keyReleaseEvent(event);
        }

        void keyPressEvent(QKeyEvent* event) override
        {
            switch (event->key())
            {
            case Qt::Key_Alt:
            {
                static const bool enabled = true;
                ViewSceneNotificationBus::Event(GetScene(), &ViewSceneNotifications::OnAltModifier, enabled);
                break;
            }
            default:
                break;
            }

            QGraphicsView::keyPressEvent(event);
        }

        void mousePressEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::RightButton || event->button() == Qt::MiddleButton)
            {
                m_initialClick = event->pos();
                m_checkForDrag = true;
                event->accept();
                return;
            }

            QGraphicsView::mousePressEvent(event);
        }

        void mouseMoveEvent(QMouseEvent* event) override
        {
            if (m_checkForDrag && isInteractive())
            {
                event->accept();

                // If we move ~0.5% in both directions, we'll consider it an intended move
                if ((m_initialClick - event->pos()).manhattanLength() > (width() * 0.005f + height() * 0.005f))
                {
                    setDragMode(QGraphicsView::ScrollHandDrag);
                    setInteractive(false);
                    // This is done because the QGraphicsView::mousePressEvent implementation takes care of 
                    // scrolling only when the DragMode is ScrollHandDrag is set and the mouse LeftButton is pressed
                    /* QT comment/
                    // Left-button press in scroll hand mode initiates hand scrolling.
                    */
                    QMouseEvent startPressEvent(QEvent::MouseButtonPress, m_initialClick, Qt::LeftButton, Qt::LeftButton, event->modifiers());
                    QGraphicsView::mousePressEvent(&startPressEvent);

                    QMouseEvent customEvent(event->type(), event->pos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());
                    QGraphicsView::mousePressEvent(&customEvent);
                }

                return;
            }

            QGraphicsView::mouseMoveEvent(event);
        }

        void focusOutEvent(QFocusEvent* event)
        {
            QGraphicsView::focusOutEvent(event);
        }

        void mouseReleaseEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::RightButton || event->button() == Qt::MiddleButton)
            {
                m_checkForDrag = false;

                if (!isInteractive())
                {
                    // This is done because the QGraphicsView::mouseReleaseEvent implementation takes care of 
                    // setting the mouse cursor back to default
                    /* QT Comment
                    // Restore the open hand cursor. ### There might be items
                    // under the mouse that have a valid cursor at this time, so
                    // we could repeat the steps from mouseMoveEvent().
                    */
                    QMouseEvent customEvent(event->type(), event->pos(), Qt::LeftButton, Qt::LeftButton, event->modifiers());
                    QGraphicsView::mouseReleaseEvent(&customEvent);
                    event->accept();
                    setInteractive(true);
                    setDragMode(QGraphicsView::RubberBandDrag);

                    // If this ever moves off of the right mouse button, we need to not signal this.
                    SceneRequestBus::Event(m_sceneId, &SceneRequests::SuppressNextContextMenu);
                    SaveViewParams();
                    return;
                }
            }

            QGraphicsView::mouseReleaseEvent(event);
        }

        void wheelEvent(QWheelEvent* event) override
        {
            if (!(event->modifiers() & Qt::ControlModifier))
            {
                setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
                // Scale the view / do the zoom
                // delta is 1/8th of a degree. We want to change the zoom in or out 0.01 per degree.
                // so: 1/8/100 = 1/800 = 0.00125
                double scaleFactor = 1.0 + (event->delta() * 0.00125);
                double newScale = m_viewParams.m_scale * scaleFactor;

                if (newScale < ZOOM_MIN)
                {
                    newScale = ZOOM_MIN;
                    scaleFactor = (ZOOM_MIN / m_viewParams.m_scale);
                }
                else if (newScale > ZOOM_MAX)
                {
                    newScale = ZOOM_MAX;
                    scaleFactor = (ZOOM_MAX / m_viewParams.m_scale);
                }

                m_viewParams.m_scale = newScale;
                scale(scaleFactor, scaleFactor);

                QueueSave();
                event->accept();
                setTransformationAnchor(QGraphicsView::AnchorViewCenter);
            }
        }
        ////

    private:

        void ConnectBoundsSignals()
        {
            m_reapplyViewParams = true;
            connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &CanvasGraphicsView::OnBoundsChanged);
            connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &CanvasGraphicsView::OnBoundsChanged);
        }

        void DisconnectBoundsSignals()
        {
            if (m_reapplyViewParams)
            {
                m_reapplyViewParams = false;
                disconnect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &CanvasGraphicsView::OnBoundsChanged);
                disconnect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &CanvasGraphicsView::OnBoundsChanged);
            }
        }

        void OnBoundsChanged()
        {
            if (m_reapplyViewParams)
            {
                if (!m_ignoreValueChange && isInteractive())
                {
                    QScopedValueRollback<bool> scopedSetter(m_ignoreValueChange, true);
                    m_styleTimer.stop();
                    m_styleTimer.start();

                    QPointF knownAnchor = mapToScene(rect().topLeft());
                    QPointF desiredPoint(m_viewParams.m_anchorPointX, m_viewParams.m_anchorPointY);
                    QPointF displacementVector = desiredPoint - knownAnchor;

                    QPointF centerPoint = mapToScene(rect().center());
                    centerPoint += displacementVector;

                    centerOn(centerPoint);
                }
            }
            else
            {
                QueueSave();
            }
        }

        void QueueSave()
        {
            m_timer.stop();
            m_timer.start(250);
        }

        void SaveViewParams()
        {
            QPointF centerPoint = mapToScene(rect().center());
            QPointF anchorPoint = mapToScene(rect().topLeft());

            m_viewParams.m_anchorPointX = anchorPoint.x();
            m_viewParams.m_anchorPointY = anchorPoint.y();

            ViewNotificationBus::Event(GetId(), &ViewNotifications::OnViewParamsChanged, m_viewParams);
        }

        void OnRubberBandChanged(QRect rubberBandRect, QPointF fromScenePoint, QPointF toScenePoint)
        {
            if (fromScenePoint.isNull() && toScenePoint.isNull())
            {
                if (m_isDragSelecting)
                {
                    m_isDragSelecting = false;
                    SceneRequestBus::Event(m_sceneId, &SceneRequests::SignalDragSelectEnd);
                }
            }
            else if (!m_isDragSelecting)
            {
                m_isDragSelecting = true;
                SceneRequestBus::Event(m_sceneId, &SceneRequests::SignalDragSelectStart);
            }
        }

        CanvasGraphicsView(const CanvasGraphicsView&) = delete;

        ViewId       m_viewId;
        AZ::EntityId m_sceneId;

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
