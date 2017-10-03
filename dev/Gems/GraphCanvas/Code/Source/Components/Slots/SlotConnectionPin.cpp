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

#include <QGraphicsItem>
#include <QGraphicsLayoutItem>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QGraphicsScene>

#include <Components/Slots/SlotConnectionPin.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <Styling/definitions.h>

namespace GraphCanvas
{
    //////////////////////
    // SlotConnectionPin
    //////////////////////

    SlotConnectionPin::SlotConnectionPin(const AZ::EntityId& slotId)
        : m_slotId(slotId)
    {
        setFlags(ItemSendsScenePositionChanges);
        setZValue(1);
        setOwnedByLayout(true);
    }

    SlotConnectionPin::~SlotConnectionPin()
    {
    }

    void SlotConnectionPin::Activate()
    {
        SlotUIRequestBus::Handler::BusConnect(m_slotId);
    }

    void SlotConnectionPin::Deactivate()
    {
        SlotUIRequestBus::Handler::BusDisconnect();
    }
	
	void SlotConnectionPin::RefreshStyle()
    {
        m_style.SetStyle(m_slotId, Styling::Elements::ConnectionPin);
    }
	
	QPointF SlotConnectionPin::GetConnectionPoint() const
    {
        qreal padding = m_style.GetAttribute(Styling::Attribute::Padding, 2.0);

        ConnectionType connectionType = ConnectionType::CT_Invalid;
        SlotRequestBus::EventResult(connectionType, GetEntityId(), &SlotRequests::GetConnectionType);

        QPointF localPoint = boundingRect().center();

        switch (connectionType)
        {
        case ConnectionType::CT_Input:
            localPoint.setX(boundingRect().left() + padding);
            break;
        case ConnectionType::CT_Output:
            localPoint.setX(boundingRect().right() - padding);
            break;
        default:
            break;
        }

        return mapToScene(localPoint);
    }

    QRectF SlotConnectionPin::boundingRect() const
    {
        return{ { 0, 0 }, geometry().size() };
    }

    void SlotConnectionPin::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget /*= nullptr*/)
    {
        qreal decorationPadding = m_style.GetAttribute(Styling::Attribute::Padding, 2.);
        qreal borderWidth = m_style.GetBorder().widthF();

        painter->setBrush(m_style.GetBrush(Styling::Attribute::BackgroundColor));

        // determine rect for drawing shape
        qreal margin = decorationPadding + (borderWidth * 0.5);
        QRectF drawRect = boundingRect().marginsRemoved({ margin, margin, margin, margin });

        bool hasConnections = false;
        SlotRequestBus::EventResult(hasConnections, GetEntityId(), &SlotRequests::HasConnections);

        DrawConnectionPin(painter, drawRect, hasConnections);
    }

    void SlotConnectionPin::keyPressEvent(QKeyEvent* keyEvent)
    {
        SlotLayoutItem::keyPressEvent(keyEvent);

        if (m_hovered)
        {
            if (keyEvent->key() == Qt::Key::Key_Alt)
            {
                SlotRequestBus::Event(m_slotId, &SlotRequests::SetConnectionDisplayState, ConnectionDisplayState::Deletion);
            }            
        }
    }

    void SlotConnectionPin::keyReleaseEvent(QKeyEvent* keyEvent)
    {
        SlotLayoutItem::keyReleaseEvent(keyEvent);

        if (m_hovered)
        {
            if (keyEvent->key() == Qt::Key::Key_Alt)
            {
                SlotRequestBus::Event(m_slotId, &SlotRequests::SetConnectionDisplayState, ConnectionDisplayState::Inspection);
            }
        }
    }

    void SlotConnectionPin::hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        m_hovered = true;
        grabKeyboard();

        if (hoverEvent->modifiers() & Qt::KeyboardModifier::AltModifier)
        {
            SlotRequestBus::Event(m_slotId, &SlotRequests::SetConnectionDisplayState, ConnectionDisplayState::Deletion);
        }
        else
        {
            SlotRequestBus::Event(m_slotId, &SlotRequests::SetConnectionDisplayState, ConnectionDisplayState::Inspection);
        }
    }

    void SlotConnectionPin::hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        m_hovered = false;
        ungrabKeyboard();

        SlotRequestBus::Event(m_slotId, &SlotRequests::SetConnectionDisplayState, ConnectionDisplayState::None);
    }

    void SlotConnectionPin::mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton && boundingRect().contains(event->pos()))
        {
            if (event->modifiers() & Qt::KeyboardModifier::AltModifier)
            {
                SlotRequestBus::Event(m_slotId, &SlotRequests::ClearConnections);
            }
            else
            {
                AZ::EntityId nodeId;
                SlotRequestBus::EventResult(nodeId, m_slotId, &SlotRequests::GetNode);
                AZ::EntityId sceneId;
                SceneMemberRequestBus::EventResult(sceneId, nodeId, &SceneMemberRequests::GetScene);

                SceneRequestBus::Event(sceneId, &SceneRequestBus::Events::ClearSelection);
                SlotRequestBus::Event(m_slotId, &SlotRequests::CreateConnection);
            }

            return;
        }

        SlotLayoutItem::mousePressEvent(event);
    }
	
    void SlotConnectionPin::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
    {
        contextMenuEvent->ignore();
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);
        SceneUIRequestBus::Event(sceneId, &SceneUIRequests::OnSlotContextMenuEvent, GetEntityId(), contextMenuEvent);
    }
	
	void SlotConnectionPin::setGeometry(const QRectF& rect)
    {
        prepareGeometryChange();
        QGraphicsLayoutItem::setGeometry(rect);
        setPos(rect.topLeft());
    }

    QSizeF SlotConnectionPin::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
    {
        const qreal decorationPadding = m_style.GetAttribute(Styling::Attribute::Padding, 2.);

        QSizeF rectSize = m_style.GetSize({ 8., 8. }) + QSizeF{ decorationPadding, decorationPadding } * 2.;

        switch (which)
        {
        case Qt::MinimumSize:
            return m_style.GetMinimumSize(rectSize);
        case Qt::PreferredSize:
            return rectSize;
        case Qt::MaximumSize:
            return m_style.GetMaximumSize();
        case Qt::MinimumDescent:
        default:
            return QSizeF();
        }
    }

    void SlotConnectionPin::DrawConnectionPin(QPainter* painter, QRectF drawRect, bool isConnected)
    {
        painter->setPen(m_style.GetBorder());

        if (isConnected)
        {
            painter->fillRect(drawRect, Qt::BrushStyle::SolidPattern);
        }
        else
        {
            painter->drawRect(drawRect);
        }
    }
}