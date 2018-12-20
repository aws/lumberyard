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
#include <QMessageBox>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

namespace GraphCanvas
{
    ////////////////////////////
    // GraphCanvasGraphicsView
    ////////////////////////////
    
    GraphCanvasGraphicsView::GraphCanvasGraphicsView()
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

        connect(&m_timer, &QTimer::timeout, this, &GraphCanvasGraphicsView::SaveViewParams);
        connect(&m_styleTimer, &QTimer::timeout, this, &GraphCanvasGraphicsView::DisconnectBoundsSignals);

        connect(this, &QGraphicsView::rubberBandChanged, this, &GraphCanvasGraphicsView::OnRubberBandChanged);

        ViewRequestBus::Handler::BusConnect(GetViewId());

        QAction* centerAction = new QAction(this);
        centerAction->setShortcut(QKeySequence(Qt::Key_Z));

        connect(centerAction, &QAction::triggered, [this]()
            {
                if (rubberBandRect().isNull() && !QApplication::mouseButtons() && !m_isEditing)
                {
                    CenterOnArea(GetSelectedArea());
                }
            });

        addAction(centerAction);

        AZStd::vector< Qt::Key > keyIndexes = { Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9 };

        for (int i = 0; i < keyIndexes.size(); ++i)
        {
            Qt::Key currentKey = keyIndexes[i];

            QAction* createBookmarkKeyAction = new QAction(this);
            createBookmarkKeyAction->setShortcut(QKeySequence(Qt::CTRL + currentKey));

            connect(createBookmarkKeyAction, &QAction::triggered, [this, i]()
                {
                    if (rubberBandRect().isNull() && !QApplication::mouseButtons() && !m_isEditing)
                    {
                        this->CreateBookmark(i+1);
                    }
                });

            addAction(createBookmarkKeyAction);

            QAction* activateBookmarkKeyAction = new QAction(this);
            activateBookmarkKeyAction->setShortcut(QKeySequence(currentKey));

            connect(activateBookmarkKeyAction, &QAction::triggered, [this, i]()
                {
                    if (rubberBandRect().isNull() && !QApplication::mouseButtons() && !QApplication::keyboardModifiers() && !m_isEditing)
                    {
                        this->JumpToBookmark(i+1);
                    }
                });

            addAction(activateBookmarkKeyAction);
        }

        // Ctrl+"0" overview shortcut.
        {
            QAction* keyAction = new QAction(this);
            keyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));

            connect(keyAction, &QAction::triggered, [this]()
                {
                    if (!m_sceneId.IsValid())
                    {
                        // There's no scene.
                        return;
                    }

                    CenterOnArea(GetCompleteArea());
                });

            addAction(keyAction);
        }

        // Ctrl+"+" zoom-in shortcut.
        {
            QAction* keyAction = new QAction(this);
            keyAction->setShortcuts({QKeySequence(Qt::CTRL + Qt::Key_Plus),
                                     QKeySequence(Qt::CTRL + Qt::Key_Equal)});

            connect(keyAction, &QAction::triggered, [this]()
                {
                    if (!m_sceneId.IsValid())
                    {
                        // There's no scene.
                        return;
                    }

                    QWheelEvent ev(QPoint(0, 0), WHEEL_ZOOM, Qt::NoButton, Qt::NoModifier);
                    wheelEvent(&ev);
                });
            addAction(keyAction);
        }

        // Ctrl+"-" zoom-out shortcut.
        {
            QAction* keyAction = new QAction(this);
            keyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));

            connect(keyAction, &QAction::triggered, [this]()
                {
                    if (!m_sceneId.IsValid())
                    {
                        // There's no scene.
                        return;
                    }

                    QWheelEvent ev(QPoint(0, 0), -WHEEL_ZOOM, Qt::NoButton, Qt::NoModifier);
                    wheelEvent(&ev);
                });

            addAction(keyAction);
        }
    }

    GraphCanvasGraphicsView::~GraphCanvasGraphicsView()
    {
        ClearScene();
    }

    const ViewId& GraphCanvasGraphicsView::GetViewId() const
    {
        return m_viewId;
    }

    void GraphCanvasGraphicsView::SetEditorId(const EditorId& editorId)
    {
        m_editorId = editorId;

        if (m_sceneId.IsValid())
        {
            SceneRequestBus::Event(m_sceneId, &SceneRequests::SetEditorId, m_editorId);
        }
    }

    EditorId GraphCanvasGraphicsView::GetEditorId() const
    {
        return m_editorId;
    }

    void GraphCanvasGraphicsView::SetScene(const AZ::EntityId& sceneId)
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

        SceneRequestBus::Event(m_sceneId, &SceneRequests::RegisterView, GetViewId());

        ConnectBoundsSignals();
        OnBoundsChanged();
    }

    AZ::EntityId GraphCanvasGraphicsView::GetScene() const
    {
        return m_sceneId;
    }

    void GraphCanvasGraphicsView::ClearScene()
    {
        if (m_sceneId.IsValid())
        {
            SceneRequestBus::Event(m_sceneId, &SceneRequests::RemoveView, GetViewId());
            SceneNotificationBus::Handler::BusDisconnect(m_sceneId);
        }

        m_sceneId.SetInvalid();
        setScene(nullptr);
    }

    AZ::Vector2 GraphCanvasGraphicsView::GetViewSceneCenter() const
    {
        AZ::Vector2 viewCenter(0, 0);
        QPointF centerPoint = mapToScene(rect()).boundingRect().center();

        viewCenter.SetX(centerPoint.x());
        viewCenter.SetY(centerPoint.y());

        return viewCenter;
    }

    AZ::Vector2 GraphCanvasGraphicsView::MapToScene(const AZ::Vector2& view)
    {
        QPointF mapped = mapToScene(QPoint(view.GetX(), view.GetY()));
        return AZ::Vector2(mapped.x(), mapped.y());
    }

    AZ::Vector2 GraphCanvasGraphicsView::MapFromScene(const AZ::Vector2& scene)
    {
        QPoint mapped = mapFromScene(QPointF(scene.GetX(), scene.GetY()));
        return AZ::Vector2(mapped.x(), mapped.y());
    }

    void GraphCanvasGraphicsView::SetViewParams(const ViewParams& viewParams)
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
        qreal scaleValue = 1.0 / m_viewParams.m_scale;

        // Apply the new scale
        scaleValue *= viewParams.m_scale;

        // Update view params before setting scale
        m_viewParams = viewParams;

        // Set out scale
        scale(scaleValue, scaleValue);
        OnBoundsChanged();
    }

    void GraphCanvasGraphicsView::DisplayArea(const QRectF& viewArea)
    {
        // Fit into view.
        fitInView(viewArea, Qt::AspectRatioMode::KeepAspectRatio);

        QTransform xfm = transform();

        // Clamp the scaling factor.
        m_viewParams.m_scale = AZ::GetClamp(xfm.m11(), ZOOM_MIN, ZOOM_MAX);

        // Apply the new scale
        xfm.setMatrix(m_viewParams.m_scale, xfm.m12(), xfm.m13(),
            xfm.m21(), m_viewParams.m_scale, xfm.m23(),
            xfm.m31(), xfm.m32(), xfm.m33());
        setTransform(xfm);

        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewCenteredOnArea);
    }

    void GraphCanvasGraphicsView::CenterOnArea(const QRectF& viewArea)
    {
        qreal originalZoom = transform().m11();

        // Fit into view.
        fitInView(viewArea, Qt::AspectRatioMode::KeepAspectRatio);

        QTransform xfm = transform();

        qreal newZoom = AZ::GetMin(xfm.m11(), originalZoom);

        // Clamp the scaling factor.
        m_viewParams.m_scale = AZ::GetClamp(newZoom, ZOOM_MIN, ZOOM_MAX);

        // Apply the new scale.
        xfm.setMatrix(m_viewParams.m_scale, xfm.m12(), xfm.m13(),
            xfm.m21(), m_viewParams.m_scale, xfm.m23(),
            xfm.m31(), xfm.m32(), xfm.m33());

        setTransform(xfm);

        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewCenteredOnArea);
    }

    void GraphCanvasGraphicsView::CenterOn(const QPointF& posInSceneCoordinates)
    {
        centerOn(posInSceneCoordinates);
    }

    void GraphCanvasGraphicsView::WheelEvent(QWheelEvent* ev)
    {
        wheelEvent(ev);
    }

    QRectF GraphCanvasGraphicsView::GetViewableAreaInSceneCoordinates()
    {
        return mapToScene(rect()).boundingRect();
    }

    GraphCanvasGraphicsView* GraphCanvasGraphicsView::AsGraphicsView()
    {
        return this;
    }
    
    QRectF GraphCanvasGraphicsView::GetCompleteArea()
    {
        QList<QGraphicsItem*> itemsList = items();
        QRectF completeArea;

        // Get the grid.
        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, m_sceneId, &SceneRequests::GetGrid);

        QGraphicsItem* gridItem = nullptr;
        VisualRequestBus::EventResult(gridItem, gridId, &VisualRequests::AsGraphicsItem);

        // Get the area.
        for (auto item : itemsList)
        {
            if (item == gridItem)
            {
                // Ignore the grid.
                continue;
            }

            completeArea |= item->sceneBoundingRect();
        }

        return completeArea;
    }

    QRectF GraphCanvasGraphicsView::GetSelectedArea()
    {
        QList<QGraphicsItem*> itemsList = items();
        QList<QGraphicsItem*> selectedList;
        QRectF selectedArea;
        QRectF completeArea;
        QRectF area;

        // Get the grid.
        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, m_sceneId, &SceneRequests::GetGrid);

        QGraphicsItem* gridItem = nullptr;
        VisualRequestBus::EventResult(gridItem, gridId, &VisualRequests::AsGraphicsItem);

        // Get the area.
        for (auto item : itemsList)
        {
            if (item == gridItem)
            {
                // Ignore the grid.
                continue;
            }

            completeArea |= item->sceneBoundingRect();

            if (item->isSelected())
            {
                selectedList.push_back(item);
                selectedArea |= item->sceneBoundingRect();
            }
        }

        if (selectedList.empty())
        {
            if (itemsList.size() > 1) // >1 because the grid is always contained.
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

        return area;
    }
    
    void GraphCanvasGraphicsView::OnStylesChanged()
    {
        update();
    }

    void GraphCanvasGraphicsView::OnNodeIsBeingEdited(bool isEditing)
    {
        m_isEditing = isEditing;
    }
    
    void GraphCanvasGraphicsView::keyReleaseEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
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

    void GraphCanvasGraphicsView::keyPressEvent(QKeyEvent* event)
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

    void GraphCanvasGraphicsView::contextMenuEvent(QContextMenuEvent *event)
    {
        if (event->reason() == QContextMenuEvent::Mouse)
        {
            // invoke the context menu in mouseReleaseEvent, otherwise we cannot drag with right mouse button
            return;
        }
        QGraphicsView::contextMenuEvent(event);
    }

    void GraphCanvasGraphicsView::mousePressEvent(QMouseEvent* event)
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

    void GraphCanvasGraphicsView::mouseMoveEvent(QMouseEvent* event)
    {
        if ((event->buttons() & (Qt::RightButton | Qt::MiddleButton)) == Qt::NoButton)
        {
            // it might be that the mouse button already was released but the mouseReleaseEvent
            // was sent somewhere else (context menu)
            m_checkForDrag = false;
        }
        else if (m_checkForDrag && isInteractive())
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
    
    void GraphCanvasGraphicsView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::RightButton)
        {
            // If we move less than ~0.5% in both directions, we'll consider it an unintended move
            if ((m_initialClick - event->pos()).manhattanLength() <= (width() * 0.005f + height() * 0.005f))
            {
                QContextMenuEvent ce(QContextMenuEvent::Mouse, event->pos(), event->globalPos(), event->modifiers());
                QGraphicsView::contextMenuEvent(&ce);
                return;
            }
        }
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
    
    void GraphCanvasGraphicsView::wheelEvent(QWheelEvent* event)
    {
        if (!(event->modifiers() & Qt::ControlModifier))
        {
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            // Scale the view / do the zoom
            // delta is 1/8th of a degree. We want to change the zoom in or out 0.01 per degree.
            // so: 1/8/100 = 1/800 = 0.00125
            qreal scaleFactor = 1.0 + (event->delta() * 0.00125);
            qreal newScale = m_viewParams.m_scale * scaleFactor;

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

    void GraphCanvasGraphicsView::focusOutEvent(QFocusEvent* event)
    {
        QGraphicsView::focusOutEvent(event);
    }

    void GraphCanvasGraphicsView::resizeEvent(QResizeEvent* event)
    {
        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewResized, event);

        QGraphicsView::resizeEvent(event);
    }

    void GraphCanvasGraphicsView::scrollContentsBy(int dx, int dy)
    {
        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewScrolled);

        QGraphicsView::scrollContentsBy(dx, dy);
    }

    void GraphCanvasGraphicsView::CreateBookmark(int bookmarkShortcut)
    {
        AZ::EntityId sceneId = GetScene();

        bool remapId = false;
        AZ::EntityId bookmark;

        AZStd::vector< AZ::EntityId > selectedItems;
        SceneRequestBus::EventResult(selectedItems, sceneId, &SceneRequests::GetSelectedItems);

        for (const AZ::EntityId& selectedItem : selectedItems)
        {
            if (BookmarkRequestBus::FindFirstHandler(selectedItem) != nullptr)
            {
                if (bookmark.IsValid())
                {
                    remapId = false;
                }
                else
                {
                    remapId = true;
                    bookmark = selectedItem;
                }
            }
        }

        AZ::EntityId existingBookmark;
        BookmarkManagerRequestBus::EventResult(existingBookmark, sceneId, &BookmarkManagerRequests::FindBookmarkForShortcut, bookmarkShortcut);

        if (existingBookmark.IsValid())
        {
            AZStd::string bookmarkName;
            BookmarkRequestBus::EventResult(bookmarkName, existingBookmark, &BookmarkRequests::GetBookmarkName);
                
            QMessageBox::StandardButton response = QMessageBox::StandardButton::No;
            response = QMessageBox::question(this, QString("Bookmarking Conflict"), QString("Bookmark (%1) already registered with shortcut (%2).\nProceed with action and remove previous bookmark?").arg(bookmarkName.c_str()).arg(bookmarkShortcut), QMessageBox::StandardButton::Yes|QMessageBox::No);

            if (response == QMessageBox::StandardButton::No)
            {
                return;
            }
            else if (response == QMessageBox::StandardButton::Yes)
            {
                BookmarkRequestBus::Event(existingBookmark, &BookmarkRequests::RemoveBookmark);
            }
        }

        if (remapId)
        {
            BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::RequestShortcut, bookmark, bookmarkShortcut);
            GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RequestUndoPoint);
        }
        else
        {
            bool createdAnchor = false;
            AZ::Vector2 position = GetViewSceneCenter();
            BookmarkManagerRequestBus::EventResult(createdAnchor, sceneId, &BookmarkManagerRequests::CreateBookmarkAnchor, position, bookmarkShortcut);

            if (createdAnchor)
            {
                GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RequestUndoPoint);
            }
        }
    }


    void GraphCanvasGraphicsView::JumpToBookmark(int bookmarkShortcut)
    {
        AZ::EntityId sceneId = GetScene();
        BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::ActivateShortcut, bookmarkShortcut);
    }

    void GraphCanvasGraphicsView::ConnectBoundsSignals()
    {
        m_reapplyViewParams = true;
        connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &GraphCanvasGraphicsView::OnBoundsChanged);
        connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &GraphCanvasGraphicsView::OnBoundsChanged);
    }

    void GraphCanvasGraphicsView::DisconnectBoundsSignals()
    {
        if (m_reapplyViewParams)
        {
            m_reapplyViewParams = false;
            disconnect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &GraphCanvasGraphicsView::OnBoundsChanged);
            disconnect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &GraphCanvasGraphicsView::OnBoundsChanged);
        }
    }

    void GraphCanvasGraphicsView::OnBoundsChanged()
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

    void GraphCanvasGraphicsView::QueueSave()
    {
        m_timer.stop();
        m_timer.start(250);
    }

    void GraphCanvasGraphicsView::SaveViewParams()
    {
        QPointF centerPoint = mapToScene(rect().center());
        QPointF anchorPoint = mapToScene(rect().topLeft());

        m_viewParams.m_anchorPointX = anchorPoint.x();
        m_viewParams.m_anchorPointY = anchorPoint.y();

        ViewNotificationBus::Event(GetViewId(), &ViewNotifications::OnViewParamsChanged, m_viewParams);
    }

    void GraphCanvasGraphicsView::OnRubberBandChanged(QRect rubberBandRect, QPointF fromScenePoint, QPointF toScenePoint)
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
}
