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

#include <qevent.h>
#include <QtMath>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>

#include <Components/Connections/ConnectionVisualComponent.h>

#include <Components/GeometryBus.h>
#include <Components/Slots/SlotBus.h>
#include <Components/VisualBus.h>
#include <Styling/definitions.h>
#include <GraphCanvas/tools.h>

namespace GraphCanvas
{
    //////////////////////////////
    // ConnectionVisualComponent
    //////////////////////////////
    void ConnectionVisualComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ConnectionVisualComponent>()
                ->Version(1)
                ;
        }
    }

    ConnectionVisualComponent::ConnectionVisualComponent()
    {
    }

    void ConnectionVisualComponent::Init()
    {
        CreateConnectionVisual();
    }

    void ConnectionVisualComponent::Activate()
    {
        if (m_connectionGraphicsItem)
        {
            m_connectionGraphicsItem->Activate();
        }

        VisualRequestBus::Handler::BusConnect(GetEntityId());
        RootVisualRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ConnectionVisualComponent::Deactivate()
    {
        VisualRequestBus::Handler::BusDisconnect();
        RootVisualRequestBus::Handler::BusDisconnect();

        if (m_connectionGraphicsItem)
        {
            m_connectionGraphicsItem->Deactivate();
        }
    }

    QGraphicsItem* ConnectionVisualComponent::AsGraphicsItem()
    {
        return m_connectionGraphicsItem.get();
    }
	
	bool ConnectionVisualComponent::Contains(const AZ::Vector2&) const
	{
		return false;
	}

    QGraphicsItem* ConnectionVisualComponent::GetRootGraphicsItem()
    {
        return m_connectionGraphicsItem.get();
    }

    QGraphicsLayoutItem* ConnectionVisualComponent::GetRootGraphicsLayoutItem()
    {
        return nullptr;
    }
	
	void ConnectionVisualComponent::CreateConnectionVisual()
    {
        m_connectionGraphicsItem = AZStd::make_unique<ConnectionGraphicsItem>(GetEntityId());
    }

    ////////////////////////////
    // ConnectionGraphicsItem
    ////////////////////////////
	
	qreal ConnectionGraphicsItem::VectorLength(QPointF vectorPoint)
    {
        return qSqrt(vectorPoint.x() * vectorPoint.x() + vectorPoint.y() * vectorPoint.y());
    }

    ConnectionGraphicsItem::ConnectionGraphicsItem(const AZ::EntityId& connectionEntityId)
        : RootVisualNotificationsHelper(connectionEntityId)
        , m_displayState(ConnectionDisplayState::None)
        , m_hovered(false)
        , m_trackMove(false)
        , m_moveSource(false)
        , m_offset(0.0)
        , m_connectionEntityId(connectionEntityId)
    {
        setFlags(ItemIsSelectable);
        setData(GraphicsItemName, QStringLiteral("DefaultConnectionVisual/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));
    }

    void ConnectionGraphicsItem::Activate()
    {
        AZStd::string tooltip;
        ConnectionRequestBus::EventResult(tooltip, GetEntityId(), &ConnectionRequests::GetTooltip);
        setToolTip(Tools::qStringFromUtf8(tooltip));

        ConnectionNotificationBus::Handler::BusConnect(GetEntityId());
        StyleNotificationBus::Handler::BusConnect(GetEntityId());

        AZ::EntityId sourceId;
        ConnectionRequestBus::EventResult(sourceId, GetConnectionEntityId(), &ConnectionRequests::GetSourceSlotId);
        OnSourceSlotIdChanged(AZ::EntityId(), sourceId);

        AZ::EntityId targetId;
        ConnectionRequestBus::EventResult(targetId, GetConnectionEntityId(), &ConnectionRequests::GetTargetSlotId);
        OnTargetSlotIdChanged(AZ::EntityId(), targetId);

        ConnectionUIRequestBus::Handler::BusConnect(GetConnectionEntityId());

        OnStyleChanged();
        UpdateConnectionPath();

        OnActivate();
    }

    void ConnectionGraphicsItem::Deactivate()
    {
        ConnectionUIRequestBus::Handler::BusDisconnect();
        ConnectionNotificationBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();

        VisualNotificationBus::MultiHandler::BusDisconnect();
    }

    void ConnectionGraphicsItem::RefreshStyle()
    {
        m_style.SetStyle(GetEntityId());

        UpdatePen();
    }

    const Styling::StyleHelper& ConnectionGraphicsItem::GetStyle() const
    {
        return m_style;
    }

    void ConnectionGraphicsItem::UpdateOffset()
    {
        // This works for all default dash/dot patterns, for now
        static const double offsetReset = 24.0;

        // TODO: maybe calculate this based on an "animation-speed" attribute?
        m_offset += 0.25;

        if (m_offset >= offsetReset)
        {
            m_offset -= offsetReset;
        }

        QPen currentPen = pen();
        currentPen.setDashOffset(-m_offset);
        setPen(currentPen);
    }

    void ConnectionGraphicsItem::OnSourceSlotIdChanged(const AZ::EntityId& oldSlotId, const AZ::EntityId& newSlotId)
    {
        if (oldSlotId.IsValid())
        {
            VisualNotificationBus::MultiHandler::BusDisconnect(oldSlotId);
        }

        if (newSlotId.IsValid())
        {
            VisualNotificationBus::MultiHandler::BusConnect(newSlotId);
        }

        UpdateConnectionPath();
    }

    void ConnectionGraphicsItem::OnTargetSlotIdChanged(const AZ::EntityId& oldSlotId, const AZ::EntityId& newSlotId)
    {
        if (oldSlotId.IsValid())
        {
            VisualNotificationBus::MultiHandler::BusDisconnect(oldSlotId);
        }

        if (newSlotId.IsValid())
        {
            VisualNotificationBus::MultiHandler::BusConnect(newSlotId);
        }

        UpdateConnectionPath();
    }

    void ConnectionGraphicsItem::OnTooltipChanged(const AZStd::string& tooltip)
    {
        setToolTip(Tools::qStringFromUtf8(tooltip));
    }

    void ConnectionGraphicsItem::OnStyleChanged()
    {
        RefreshStyle();

        bool animate = m_style.GetAttribute(Styling::Attribute::LineStyle, Qt::SolidLine) != Qt::SolidLine;

        if (animate)
        {
            if (!AZ::SystemTickBus::Handler::BusIsConnected())
            {
                AZ::SystemTickBus::Handler::BusConnect();
            }
        }
        else if (AZ::SystemTickBus::Handler::BusIsConnected())
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
        }

        setZValue(m_style.GetAttribute(Styling::Attribute::ZValue, 0));
        UpdateConnectionPath();
    }

    void ConnectionGraphicsItem::OnSystemTick()
    {
        UpdateOffset();
    }

    void ConnectionGraphicsItem::OnItemChange(const AZ::EntityId& entityId, QGraphicsItem::GraphicsItemChange change, const QVariant& value)
    {
        switch (change)
        {
        case QGraphicsItem::ItemScenePositionHasChanged:
        {
            UpdateConnectionPath();
            break;
        }
        }
    }

    void ConnectionGraphicsItem::UpdateConnectionPath()
    {
        AZ::EntityId sourceId;
        ConnectionRequestBus::EventResult(sourceId, GetConnectionEntityId(), &ConnectionRequests::GetSourceSlotId);

        AZ::EntityId targetId;
        ConnectionRequestBus::EventResult(targetId, GetConnectionEntityId(), &ConnectionRequests::GetTargetSlotId);

        QPointF start;
        ConnectionRequestBus::EventResult(start, GetEntityId(), &ConnectionRequests::GetSourcePosition);

        QPointF end;
        ConnectionRequestBus::EventResult(end, GetEntityId(), &ConnectionRequests::GetTargetPosition);

        bool loopback = false;
        float nodeHeight = 0;
        AZ::Vector2 nodePos(0, 0);

        if (end.isNull())
        {
            end = start;
        }
        else
        {
            // Determine if this connection is from and to the same node (self-connection)
            AZ::EntityId sourceNode;
            SlotRequestBus::EventResult(sourceNode, sourceId, &SlotRequestBus::Events::GetNode);

            AZ::EntityId targetNode;
            SlotRequestBus::EventResult(targetNode, targetId, &SlotRequestBus::Events::GetNode);

            loopback = sourceNode == targetNode;

            if (loopback)
            {
                QGraphicsItem* rootVisual = nullptr;
                RootVisualRequestBus::EventResult(rootVisual, sourceNode, &RootVisualRequests::GetRootGraphicsItem);

                if (rootVisual)
                {
                    nodeHeight = rootVisual->boundingRect().height();
                }

                GeometryRequestBus::EventResult(nodePos, sourceNode, &GeometryRequestBus::Events::GetPosition);
            }
        }

        ConnectionType connectionType = ConnectionType::CT_Invalid;
        SlotRequestBus::EventResult(connectionType, sourceId, &SlotRequests::GetConnectionType);

        QPainterPath path = QPainterPath(start);

        Styling::Curves curve = m_style.GetAttribute(Styling::Attribute::LineCurve, Styling::Curves::Curved);

        if (curve == Styling::Curves::Curved)
        {
            // Scale the control points based on the length of the line to make sure the curve looks pretty
            QPointF offset = (end - start);
            QPointF midVector = (start + end) / 2.0 - start;

            // Mathemagic to make the curvature look nice
            qreal magnitude = 0;

            if (offset.x() < 0)
            {
                magnitude = AZ::GetMax(qSqrt(VectorLength(offset)) * 5, qFabs(offset.x()) * 0.25f);
            }
            else
            {
                magnitude = AZ::GetMax(qSqrt(VectorLength(offset)) * 5, offset.x() * 0.5f);
            }
            magnitude = AZ::GetClamp(magnitude, (qreal) 10.0f, qMax(VectorLength(midVector), (qreal) 10.0f));

            // Makes the line come out horizontally from the start and end points
            QPointF offsetStart = start + QPointF(magnitude, 0);
            QPointF offsetEnd = end - QPointF(magnitude, 0);

            if (!loopback)
            {
                path.cubicTo(offsetStart, offsetEnd, end);
            }
            else
            {
                // Make the connection wrap around the node,
                // leaving some space between the connection and the node
                qreal heightOffset = (qreal)nodePos.GetY() + nodeHeight + 20.0f;

                path.cubicTo(offsetStart, { offsetStart.x(), heightOffset }, { start.x(), heightOffset });
                path.lineTo({ end.x(), heightOffset });
                path.cubicTo({ offsetEnd.x(), heightOffset }, offsetEnd, end);
            }
        }
        else
        {
            float connectionJut = m_style.GetAttribute(Styling::Attribute::ConnectionJut, 0.0f);

            QPointF startOffset = start + QPointF(connectionJut, 0);
            QPointF endOffset = end - QPointF(connectionJut, 0);
            path.lineTo(startOffset);
            path.lineTo(endOffset);
            path.lineTo(end);
        }

        setPath(path);
        update();

        OnPathChanged();
    }

    void ConnectionGraphicsItem::SetDisplayState(ConnectionDisplayState displayState)
    {
        if (m_displayState != displayState)
        {
            const bool addStyle = true;
            const bool removeStyle = false;
            UpdateDisplayState(m_displayState, removeStyle);

            m_displayState = displayState;

            UpdateDisplayState(m_displayState, addStyle);

            OnDisplayStateChanged();
        }
    }

    void ConnectionGraphicsItem::SetSelected(bool selected)
    {
        setSelected(selected);
    }

    bool ConnectionGraphicsItem::IsSelected() const
    {
        return isSelected();
    }

    void ConnectionGraphicsItem::OnAltModifier(bool enabled)
    {
        if (m_hovered)
        {
            if (enabled)
            {
                SetDisplayState(ConnectionDisplayState::Deletion);
            }
            else
            {
                SetDisplayState(ConnectionDisplayState::Inspection);
            }
        }
    }

    const AZ::EntityId& ConnectionGraphicsItem::GetConnectionEntityId() const
    {
        return m_connectionEntityId;
    }

    AZ::EntityId ConnectionGraphicsItem::GetSourceSlotEntityId() const
    {
        AZ::EntityId sourceId;
        ConnectionRequestBus::EventResult(sourceId, GetConnectionEntityId(), &ConnectionRequests::GetSourceSlotId);

        return sourceId;
    }

    AZ::EntityId ConnectionGraphicsItem::GetTargetSlotEntityId() const
    {
        AZ::EntityId targetId;
        ConnectionRequestBus::EventResult(targetId, GetConnectionEntityId(), &ConnectionRequests::GetTargetSlotId);

        return targetId;
    }

    void ConnectionGraphicsItem::UpdatePen()
    {
        QPen pen = m_style.GetPen(Styling::Attribute::LineWidth, Styling::Attribute::LineStyle, Styling::Attribute::LineColor, Styling::Attribute::CapStyle);
        setPen(pen);
    }

    void ConnectionGraphicsItem::OnActivate()
    {
    }

    void ConnectionGraphicsItem::OnDeactivate()
    {
    }

    void ConnectionGraphicsItem::OnPathChanged()
    {
    }

    void ConnectionGraphicsItem::OnDisplayStateChanged()
    {
    }

    ConnectionDisplayState ConnectionGraphicsItem::GetDisplayState() const
    {
        return m_displayState;
    }

    QPainterPath ConnectionGraphicsItem::shape() const
    {
        // Creates a "selectable area" around the connection
        QPainterPathStroker stroker;
        qreal padding = m_style.GetAttribute(Styling::Attribute::LineSelectionPadding, 0);
        stroker.setWidth(padding);
        return stroker.createStroke(path());
    }

    void ConnectionGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
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

        SceneUIRequestBus::Event(sceneId, &SceneUIRequests::OnConnectionContextMenuEvent, GetEntityId(), contextMenuEvent);
    }

    void ConnectionGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        m_hovered = true;

        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        ViewSceneNotificationBus::Handler::BusConnect(sceneId);

        if (hoverEvent->modifiers() & Qt::KeyboardModifier::AltModifier)
        {
            SetDisplayState(ConnectionDisplayState::Deletion);
        }
        else
        {
            SetDisplayState(ConnectionDisplayState::Inspection);
        }
        
        QGraphicsPathItem::hoverEnterEvent(hoverEvent);
    }

    void ConnectionGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        m_hovered = false;
        ViewSceneNotificationBus::Handler::BusDisconnect();

        SetDisplayState(ConnectionDisplayState::None);

        QGraphicsPathItem::hoverLeaveEvent(hoverEvent);
    }

    void ConnectionGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (mouseEvent->button() == Qt::MouseButton::LeftButton)
        {
            m_trackMove = false;

            if ((mouseEvent->modifiers() & Qt::KeyboardModifier::AltModifier))
            {
                AZStd::unordered_set<AZ::EntityId> deleteIds;
                deleteIds.insert(GetEntityId());

                AZ::EntityId sceneId;
                SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

                SceneRequestBus::Event(sceneId, &SceneRequests::Delete, deleteIds);
            }
            else
            {
                const QPainterPath painterPath = path();

                QPointF clickPoint = mouseEvent->scenePos();
                QPointF startPoint = painterPath.pointAtPercent(0);
                QPointF endPoint = painterPath.pointAtPercent(1);

                float distanceToSource = (clickPoint - startPoint).manhattanLength();
                float distanceToTarget = (clickPoint - endPoint).manhattanLength();

                float maxDistance = m_style.GetAttribute(Styling::Attribute::ConnectionDragMaximumDistance, 100.0f);
                float dragPercentage = m_style.GetAttribute(Styling::Attribute::ConnectionDragPercent, 0.1f);

                float acceptanceDistance = AZStd::GetMin<float>(maxDistance, painterPath.length() * dragPercentage);

                if (distanceToSource < acceptanceDistance)
                {
                    m_trackMove = true;
                    m_moveSource = true;
                    m_initialPoint = mouseEvent->scenePos();
                }
                else if (distanceToTarget < acceptanceDistance)
                {
                    m_trackMove = true;
                    m_moveSource = false;
                    m_initialPoint = mouseEvent->scenePos();
                }
            }
        }
        else
        {
            QGraphicsPathItem::mousePressEvent(mouseEvent);
        }
    }

    void ConnectionGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (m_trackMove)
        {
            float maxDistance = m_style.GetAttribute(Styling::Attribute::ConnectionDragMoveBuffer, 0.0f);
            float distanceFromInitial = (m_initialPoint - mouseEvent->scenePos()).manhattanLength();

            if (distanceFromInitial > maxDistance)
            {
                m_trackMove = false;

                if (m_moveSource)
                {
                    ConnectionRequestBus::Event(GetEntityId(), &ConnectionRequests::StartSourceMove);
                }
                else
                {
                    ConnectionRequestBus::Event(GetEntityId(), &ConnectionRequests::StartTargetMove);
                }
            }
        }
        else
        {
            QGraphicsPathItem::mouseMoveEvent(mouseEvent);
        }
    }

    void ConnectionGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        // Handle the case where we clicked but didn't drag.
        // So we want to select ourselves.
        if (mouseEvent->button() == Qt::MouseButton::LeftButton && shape().contains(mouseEvent->scenePos()))
        {
            if ((mouseEvent->modifiers() & Qt::KeyboardModifier::ControlModifier) == 0)
            {
                AZ::EntityId sceneId;
                SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);
                SceneRequestBus::Event(sceneId, &SceneRequests::ClearSelection);
            }

            setSelected(true);
            m_trackMove = false;
        }
        else
        {
            QGraphicsPathItem::mouseReleaseEvent(mouseEvent);
        }
    }


    void ConnectionGraphicsItem::UpdateDisplayState(ConnectionDisplayState displayState, bool enabled)
    {
        switch (displayState)
        {
        case ConnectionDisplayState::Deletion:
            if (enabled)
            {
                StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Deletion);
            }
            else
            {
                StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Deletion);
            }
            break;
        case ConnectionDisplayState::Inspection:
            if (enabled)
            {
                StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Hovered);
            }
            else
            {
                StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Hovered);
            }
            break;
        case ConnectionDisplayState::None:
            break;
        default:
            break;
        }        
    }
}
