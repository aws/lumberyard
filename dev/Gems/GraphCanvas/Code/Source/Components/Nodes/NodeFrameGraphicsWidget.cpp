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
        SceneMemberUIRequestBus::Handler::BusConnect(GetEntityId());
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
        SceneMemberUIRequestBus::Handler::BusDisconnect();
    }

    QGraphicsItem* NodeFrameGraphicsWidget::GetRootGraphicsItem()
    {
        return this;
    }

    QGraphicsLayoutItem* NodeFrameGraphicsWidget::GetRootGraphicsLayoutItem()
    {
        return this;
    }

    void NodeFrameGraphicsWidget::SetSelected(bool selected)
    {
        setSelected(selected);
    }

    bool NodeFrameGraphicsWidget::IsSelected() const
    {
        return isSelected();
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

        if (IsResizedToGrid() && (!m_isWrapped))
        {
            int width = static_cast<int>(retVal.width());
            int height = static_cast<int>(retVal.height());

            width = GrowToNextStep(width, GetGridXStep());
            height = GrowToNextStep(height, GetGridYStep());

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
        SetResizeToGridEnabled(false);
    }

    void NodeFrameGraphicsWidget::AdjustSize()
    {
        adjustSize();
    }

    bool NodeFrameGraphicsWidget::IsWrapped() const
    {
        return m_isWrapped;
    }

    void NodeFrameGraphicsWidget::SetSnapToGrid(bool snapToGrid)
    {
        SetSnapToGridEnabled(snapToGrid);
    }

    void NodeFrameGraphicsWidget::SetResizeToGrid(bool resizeToGrid)
    {
        SetResizeToGridEnabled(resizeToGrid);
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

    int NodeFrameGraphicsWidget::GrowToNextStep(int value, int step) const
    {
        int delta = value % step;

        if (delta == 0)
        {
            return value;
        }
        else
        {
            return value + (step - (value % step));
        }
    }

    int NodeFrameGraphicsWidget::RoundToClosestStep(int value, int step) const
    {
        if (step == 1)
        {
            return value;
        }

        int halfStep = step / 2;

        value += halfStep;
        return ShrinkToPreviousStep(value, step);
    }

    int NodeFrameGraphicsWidget::ShrinkToPreviousStep(int value, int step) const
    {
        return value - (value % step);
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