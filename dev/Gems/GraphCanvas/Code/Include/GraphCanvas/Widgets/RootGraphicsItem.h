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

#include <type_traits>

#include <QGraphicsSceneEvent>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QDebug>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/definitions.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Utils/StateControllers/PrioritizedStateController.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(QGraphicsItem, "{054358C3-B3D7-4035-9A74-2D7B2741271A}");
}

namespace GraphCanvas
{
    //! Generates EBus notifications for some QGraphicsItem events.
    template<typename GraphicsItem>
    class RootGraphicsItem
        : public GraphicsItem
        , public ViewSceneNotificationBus::Handler
        , public RootGraphicsItemRequestBus::Handler
        , public StateController<RootGraphicsItemDisplayState>::Notifications::Handler
    {
        static_assert(std::is_base_of<QGraphicsItem, GraphicsItem>::value, "GraphicsItem must be a descendant of QGraphicsItem");        

    public:

        using GraphicsItem::setAcceptHoverEvents;

        RootGraphicsItem(AZ::EntityId itemId)
            : m_resizeToGrid(false)
            , m_snapToGrid(false)
            , m_gridX(1)
            , m_gridY(1)
            , m_itemId(itemId)
            , m_anchorPoint(0,0)
            , m_forcedStateDisplayState(RootGraphicsItemDisplayState::Neutral)
            , m_internalDisplayState(RootGraphicsItemDisplayState::Neutral)
            , m_actualDisplayState(RootGraphicsItemDisplayState::Neutral)
        {
            setAcceptHoverEvents(true);
            RootGraphicsItemRequestBus::Handler::BusConnect(GetEntityId());
            StateController<RootGraphicsItemDisplayState>::Notifications::Handler::BusConnect(&m_forcedStateDisplayState);
        }

        ~RootGraphicsItem() override = default;

        // QGraphicsItem
        enum
        {
            Type = QGraphicsItem::UserType + 1
        };

        AZ::EntityId GetEntityId() const 
        {
            return m_itemId;
        }

        bool IsSnappedToGrid() const
        {
            return m_snapToGrid;
        }

        bool IsResizedToGrid() const
        {
            return m_resizeToGrid;
        }

        int GetGridXStep() const
        {
            return m_gridX;
        }

        int GetGridYStep() const
        {
            return m_gridY;
        }

        void SetSnapToGridEnabled(bool enabled)
        {
            m_snapToGrid = enabled;

            if (m_snapToGrid)
            {
                GraphicsItem* thisItem = static_cast<GraphicsItem*>(this);
                thisItem->setPos(CalculateSnapPosition(thisItem->pos()));
            }
        }

        void SetResizeToGridEnabled(bool enabled)
        {
            m_resizeToGrid = enabled;
        }

        void SetGridSize(const AZ::Vector2& gridSize)
        {
            if (gridSize.GetX() >= 0)
            {
                m_gridX = static_cast<unsigned int>(gridSize.GetX());
            }
            else
            {
                m_gridX = 1;
                AZ_Error("VisualNotificationsHelper", false, "Invalid X-Step to snap grid to.");
            }

            if (gridSize.GetY() >= 0)
            {
                m_gridY = static_cast<unsigned int>(gridSize.GetY());
            }
            else
            {
                m_gridY = 1;
                AZ_Error("VisualNotificationsHelper", false, "Invalid Y-Step to snap grid to.");
            }
        }

        void SetAnchorPoint(const AZ::Vector2& anchorPoint)
        {
            m_anchorPoint = anchorPoint;
        }

        // StateController<RootGraphicsItemDisplayState>
        void OnStateChanged(const RootGraphicsItemDisplayState& displayState)
        {
            UpdateActualDisplayState();
        }
        ////

        // RootGraphicsItemRequestBus
        StateController<RootGraphicsItemDisplayState>* GetDisplayStateStateController() override
        {
            return &m_forcedStateDisplayState;
        }

        RootGraphicsItemDisplayState GetDisplayState() const override
        {
            return m_actualDisplayState;
        }
        ////

    protected:        
        RootGraphicsItem(const RootGraphicsItem&) = delete;

        void SetDisplayState(RootGraphicsItemDisplayState displayState)
        {
            if (m_internalDisplayState != displayState)
            {
                m_internalDisplayState = displayState;
                UpdateActualDisplayState();
            }
        }

        // ViewSceneNotifications
        void OnAltModifier(bool enabled) override
        {
            if (enabled)
            {
                SetDisplayState(RootGraphicsItemDisplayState::Deletion);
            }
            else
            {
                SetDisplayState(RootGraphicsItemDisplayState::Inspection);
            }
        }
        ////
        
        // QGraphicsItem
        void hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent) override
        {
            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            ViewSceneNotificationBus::Handler::BusConnect(sceneId);

            if (hoverEvent->modifiers() & Qt::KeyboardModifier::AltModifier)
            {
                SetDisplayState(RootGraphicsItemDisplayState::Deletion);
            }
            else
            {
                SetDisplayState(RootGraphicsItemDisplayState::Inspection);
            }

            GraphicsItem::hoverEnterEvent(hoverEvent);
        }

        void hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent) override
        {
            ViewSceneNotificationBus::Handler::BusDisconnect();

            SetDisplayState(RootGraphicsItemDisplayState::Neutral);

            GraphicsItem::hoverLeaveEvent(hoverEvent);
        }

        void mousePressEvent(QGraphicsSceneMouseEvent* event) override
        {
            if (event->modifiers() & Qt::KeyboardModifier::AltModifier)
            {
                OnDeleteItem();
            }
            else
            {
                bool result = false;
                VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMousePress, GetEntityId(), event);
                if (!result)
                {
                    GraphicsItem::mousePressEvent(event);
                }
            }
        }

        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMouseRelease, GetEntityId(), event);
            if (!result)
            {
                GraphicsItem::mouseReleaseEvent(event);
            }
        }

        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override
        {
            if (m_snapToGrid && change == QAbstractGraphicsShapeItem::ItemPositionChange)
            {
                QVariant snappedValue(CalculateSnapPosition(value.toPointF()));

                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnItemChange, GetEntityId(), change, snappedValue);

                return snappedValue;
            }
            else
            {
                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnItemChange, GetEntityId(), change, value);
            }

            return GraphicsItem::itemChange(change, value);
        }

        virtual QRectF GetBoundingRect() const = 0;

        int type() const override
        {
            return Type;
        }
        ////

        virtual void OnDeleteItem()
        {
            AZ::EntityId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

            AZStd::unordered_set<AZ::EntityId> deleteIds = { GetEntityId() };
            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deleteIds);
        }        

    private:

        void UpdateActualDisplayState()
        {
            RootGraphicsItemDisplayState desiredDisplayState = m_internalDisplayState;

            if (m_forcedStateDisplayState.HasState())
            {
                desiredDisplayState = m_forcedStateDisplayState.GetState();
            }

            if (desiredDisplayState != m_actualDisplayState)
            {
                RootGraphicsItemDisplayState oldDisplayState = m_actualDisplayState;

                switch (m_actualDisplayState)
                {
                case RootGraphicsItemDisplayState::Deletion:
                    LeaveDeletionState();
                    break;
                case RootGraphicsItemDisplayState::InspectionTransparent:
                    LeaveInspectionTransparentState();
                    break;
                case RootGraphicsItemDisplayState::Inspection:
                    LeaveInspectionState();
                    break;
                case RootGraphicsItemDisplayState::GroupHighlight:
                    LeaveGroupHighlightState();
                    break;
                case RootGraphicsItemDisplayState::Preview:
                    LeavePreviewState();
                    break;
                case RootGraphicsItemDisplayState::Neutral:
                    LeaveNeutralState();
                    break;
                default:
                    break;
                }

                m_actualDisplayState = desiredDisplayState;

                switch (m_actualDisplayState)
                {
                case RootGraphicsItemDisplayState::Deletion:
                    EnterDeletionState();
                    break;
                case RootGraphicsItemDisplayState::InspectionTransparent:
                    EnterInspectionTransparentState();
                    break;
                case RootGraphicsItemDisplayState::Inspection:
                    EnterInspectionState();
                    break;
                case RootGraphicsItemDisplayState::GroupHighlight:
                    EnterGroupHighlightState();
                    break;
                case RootGraphicsItemDisplayState::Preview:
                    EnterPreviewState();
                    break;
                case RootGraphicsItemDisplayState::Neutral:
                    EnterNeutralState();
                    break;
                default:
                    break;
                }

                RootGraphicsItemNotificationBus::Event(GetEntityId(), &RootGraphicsItemNotifications::OnDisplayStateChanged, oldDisplayState, m_actualDisplayState);
            }
        }

        void EnterPreviewState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Preview);
        }

        void LeavePreviewState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Preview);
        }

        void EnterDeletionState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Deletion);
        }

        void LeaveDeletionState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Deletion);
        }

        void EnterInspectionState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Hovered);
        }

        void LeaveInspectionState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Hovered);
        }

        void EnterInspectionTransparentState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::InspectionTransparent);
        }

        void LeaveInspectionTransparentState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::InspectionTransparent);
        }

        void EnterGroupHighlightState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Hovered);
        }

        void LeaveGroupHighlightState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Hovered);
        }

        void EnterNeutralState()
        {
        }

        void LeaveNeutralState()
        {
        }

        QPointF CalculateSnapPosition(QPointF position) const
        {
            QSizeF offset;
            offset.setWidth(GetBoundingRect().width() * m_anchorPoint.GetX());
            offset.setHeight(GetBoundingRect().height() * m_anchorPoint.GetY());

            int xPoint = position.x() + offset.width();
            int yPoint = position.y() + offset.height();

            if (xPoint < 0)
            {
                xPoint = xPoint - (m_gridX * 0.5f);
                xPoint += abs(xPoint) % m_gridX;
            }
            else
            {
                xPoint = xPoint + (m_gridX * 0.5f);
                xPoint -= xPoint % m_gridX;
            }

            if (yPoint < 0)
            {
                yPoint = yPoint - (m_gridY * 0.5f);
                yPoint += abs(yPoint) % m_gridY;
            }
            else
            {
                yPoint = yPoint + (m_gridY * 0.5f);
                yPoint -= yPoint % m_gridY;
            }

            position.setX(xPoint - offset.width());
            position.setY(yPoint - offset.height());

            return position;
        }

        bool m_resizeToGrid;
        bool m_snapToGrid;

        unsigned int m_gridX;
        unsigned int m_gridY;

        PrioritizedStateController<RootGraphicsItemDisplayState>  m_forcedStateDisplayState;
        RootGraphicsItemDisplayState                           m_internalDisplayState;

        RootGraphicsItemDisplayState                           m_actualDisplayState;

        AZ::EntityId m_itemId;

        AZ::Vector2 m_anchorPoint;
    };
}