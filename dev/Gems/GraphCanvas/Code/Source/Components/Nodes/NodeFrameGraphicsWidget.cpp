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
#include "precompiled.h"

#include <tools.h>

#include <QPainter>
#include <QGraphicsLayout>
#include <QGraphicsSceneEvent>

#include <Components/Nodes/NodeFrameGraphicsWidget.h>

#include <GraphCanvas/Components/GridBus.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{
    ////////////////////////////
    // NodeFrameGraphicsWidget
    ////////////////////////////

    NodeFrameGraphicsWidget::NodeFrameGraphicsWidget(const AZ::EntityId& entityKey)
        : RootVisualNotificationsHelper(entityKey)
        , m_isWrapped(false)
    {
        setFlags(ItemIsSelectable | ItemIsFocusable | ItemIsMovable | ItemSendsScenePositionChanges);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        setData(GraphicsItemName, QStringLiteral("DefaultNodeVisual/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));
    }

    void NodeFrameGraphicsWidget::Activate()
    {
        RootVisualRequestBus::Handler::BusConnect(GetEntityId());
        GeometryNotificationBus::Handler::BusConnect(GetEntityId());
        StyleNotificationBus::Handler::BusConnect(GetEntityId());
        NodeNotificationBus::Handler::BusConnect(GetEntityId());
        NodeUIRequestBus::Handler::BusConnect(GetEntityId());
        VisualRequestBus::Handler::BusConnect(GetEntityId());

        OnActivated();
    }

    void NodeFrameGraphicsWidget::Deactivate()
    {
        StyleNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        NodeUIRequestBus::Handler::BusDisconnect();
        VisualRequestBus::Handler::BusDisconnect();
        GeometryNotificationBus::Handler::BusDisconnect();
        RootVisualRequestBus::Handler::BusDisconnect();
    }

    QGraphicsItem* NodeFrameGraphicsWidget::GetRootGraphicsItem()
    {
        return this;
    }

    QGraphicsLayoutItem* NodeFrameGraphicsWidget::GetRootGraphicsLayoutItem()
    {
        return this;
    }

    void NodeFrameGraphicsWidget::OnPositionChanged(const AZ::EntityId& entityId, const AZ::Vector2& position)
    {
        setPos(QPointF(position.GetX(), position.GetY()));
    }

    void NodeFrameGraphicsWidget::OnStyleChanged()
    {
        m_style.SetStyle(GetEntityId());

        setZValue(m_style.GetAttribute(Styling::Attribute::ZValue, 0));

        OnRefreshStyle();
    }

    void NodeFrameGraphicsWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        // Need to release the mouse in case we did the start move.
        if (mouseEvent->button() == Qt::MouseButton::LeftButton)
        {
            ungrabMouse();
        }

        RootVisualNotificationsHelper<QGraphicsWidget>::mouseReleaseEvent(mouseEvent);
    }

    void NodeFrameGraphicsWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
    {
        contextMenuEvent->ignore();

        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        bool isCurrentIdAlreadySelected = IsSelected();
        bool shouldAppendSelection = contextMenuEvent->modifiers().testFlag(Qt::ControlModifier);

        // clear the current selection if you are not multi selecting and clicking on an unselected node
        if (!isCurrentIdAlreadySelected && !shouldAppendSelection)
        {
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::ClearSelection);
        }

        if (!isCurrentIdAlreadySelected)
        {
            SetSelected(true);
        }

        SceneUIRequestBus::Event(sceneId, &SceneUIRequests::OnNodeContextMenuEvent, GetEntityId(), contextMenuEvent);
    }

    QSizeF NodeFrameGraphicsWidget::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
    {
        QSizeF retVal = QGraphicsWidget::sizeHint(which, constraint);

        if (IsSnappedToGrid() && (!m_isWrapped))
        {
            int width = retVal.width();
            int height = retVal.height();

            width += (GetGridXStep() - width % GetGridXStep());
            height += (GetGridYStep() - height % GetGridYStep());

            retVal = QSizeF(width, height);
        }

        return retVal;
    }

    QGraphicsItem* NodeFrameGraphicsWidget::AsGraphicsItem()
    {
        return this;
    }

    bool NodeFrameGraphicsWidget::Contains(const AZ::Vector2& position) const
    {
        auto local = mapFromScene(QPointF(position.GetX(), position.GetY()));
        return boundingRect().contains(local);
    }

    void NodeFrameGraphicsWidget::OnNodeActivated()
    {
        AZStd::string tooltip;
        NodeRequestBus::EventResult(tooltip, GetEntityId(), &NodeRequests::GetTooltip);
        setToolTip(Tools::qStringFromUtf8(tooltip));
        //TODO setEnabled(node->IsEnabled());

        AZ::Vector2 position;
        GeometryRequestBus::EventResult(position, GetEntityId(), &GeometryRequests::GetPosition);
        setPos(QPointF(position.GetX(), position.GetY()));
    }

    void NodeFrameGraphicsWidget::OnNodeWrapped(const AZ::EntityId&)
    {
        GeometryNotificationBus::Handler::BusDisconnect();
        setFlag(QGraphicsItem::ItemIsMovable, false);
        adjustSize();
        m_isWrapped = true;

        SetSnapToGridEnabled(false);
    }

    void NodeFrameGraphicsWidget::SetSelected(bool selected)
    {
        setSelected(selected);
    }

    bool NodeFrameGraphicsWidget::IsSelected() const
    {
        return isSelected();
    }

    bool NodeFrameGraphicsWidget::IsWrapped() const
    {
        return m_isWrapped;
    }

    void NodeFrameGraphicsWidget::StartItemMove(const QPointF& initialPosition, const QPointF& currentPosition)
    {
        // Ideally, we'd just send a fake mousePressEvent, but that didn't seem to work.
        // Grabbing the mouse and sending out the move event does though.
        grabMouse();

        setSelected(true);

        QGraphicsSceneMouseEvent fakeMouseMoveEvent(QEvent::GraphicsSceneMouseMove);
        fakeMouseMoveEvent.setButtonDownScreenPos(Qt::MouseButton::LeftButton, currentPosition.toPoint());

        this->mouseMoveEvent(&fakeMouseMoveEvent);
    }

    void NodeFrameGraphicsWidget::SetSnapToGrid(bool snapToGrid)
    {
        SetSnapToGridEnabled(snapToGrid);
    }

    void NodeFrameGraphicsWidget::SetGrid(AZ::EntityId gridId)
    {
        AZ::Vector2 gridSize;
        GridRequestBus::EventResult(gridSize, gridId, &GridRequests::GetMinorPitch);

        SetGridSize(gridSize);
    }

    qreal NodeFrameGraphicsWidget::GetCornerRadius() const
    {
        return m_style.GetAttribute(Styling::Attribute::BorderRadius, 5.0);
    }

    void NodeFrameGraphicsWidget::OnActivated()
    {
    }

    void NodeFrameGraphicsWidget::OnDeactivated()
    {
    }

    void NodeFrameGraphicsWidget::OnRefreshStyle()
    {
    }
}