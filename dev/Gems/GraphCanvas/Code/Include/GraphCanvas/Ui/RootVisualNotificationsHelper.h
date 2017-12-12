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

#include <QGraphicsItem>
#include <QDebug>

#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/tools.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(QGraphicsItem, "{054358C3-B3D7-4035-9A74-2D7B2741271A}");
}

namespace GraphCanvas
{
    //! Generates EBus notifications for some QGraphicsItem events.
    template<typename GraphicsItem>
    class RootVisualNotificationsHelper
        : public GraphicsItem
    {
        static_assert(std::is_base_of<QGraphicsItem, GraphicsItem>::value, "GraphicsItem must be a descendant of QGraphicsItem");

    public:

        using GraphicsItem::setAcceptHoverEvents;

        RootVisualNotificationsHelper(AZ::EntityId itemId)
            : m_snapToGrid(false)
            , m_gridX(1)
            , m_gridY(1)
            , m_itemId(itemId)
        {
            setAcceptHoverEvents(true);
        }
        ~RootVisualNotificationsHelper() override = default;

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


    protected:        
        RootVisualNotificationsHelper(const RootVisualNotificationsHelper&) = delete;
        // QGraphicsItem
        void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override
        {
            VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnHoverEnter, static_cast<QGraphicsItem*>(this));
            GraphicsItem::hoverEnterEvent(event);
        }

        void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
        {
            VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnHoverLeave, static_cast<QGraphicsItem*>(this));
            GraphicsItem::hoverLeaveEvent(event);
        }

        void mousePressEvent(QGraphicsSceneMouseEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMousePress, GetEntityId(), event);
            if (!result)
            {
                GraphicsItem::mousePressEvent(event);
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

        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMouseDoubleClick, GetEntityId(), event);
            if (!result)
            {
                GraphicsItem::mouseDoubleClickEvent(event);
            }
        }

        void keyPressEvent(QKeyEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnKeyPress, GetEntityId(), event);
            if (!result)
            {
                GraphicsItem::keyPressEvent(event);
            }
        }

        void keyReleaseEvent(QKeyEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnKeyRelease, GetEntityId(), event);
            if (!result)
            {
                GraphicsItem::keyReleaseEvent(event);
            }
        }

        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override
        {
            // leaving this here for now because it's useful                
#if 0
            qDebug() << (AZ::Component*)this << GetEntityId() << change << value;
#endif                

            if (m_snapToGrid && change == QAbstractGraphicsShapeItem::ItemPositionChange)
            {
                QPointF rawPoint = value.toPointF();

                int xPoint = rawPoint.x();
                int yPoint = rawPoint.y();

                if (xPoint < 0)
                {
                    xPoint = static_cast<int>(xPoint - 0.5);
                    xPoint += abs(xPoint) % m_gridX;
                }
                else
                {
                    xPoint = static_cast<int>(rawPoint.x() + 0.5);
                    xPoint -= xPoint % m_gridX;
                }

                if (yPoint < 0)
                {
                    yPoint = static_cast<int>(yPoint - 0.5);
                    yPoint += abs(yPoint) % m_gridY;
                }
                else
                {
                    yPoint = static_cast<int>(yPoint + 0.5);
                    yPoint -= yPoint % m_gridY;
                }

                rawPoint.setX(xPoint);
                rawPoint.setY(yPoint);

                QVariant snappedValue(rawPoint);

                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnItemChange, GetEntityId(), change, snappedValue);

                return snappedValue;
            }
            else
            {
                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnItemChange, GetEntityId(), change, value);
            }

            return GraphicsItem::itemChange(change, value);
        }
        
        int type() const override
        {
            return Type;
        }
        ////
        
    private:
        bool m_snapToGrid;
        unsigned int m_gridX;
        unsigned int m_gridY;
        AZ::EntityId m_itemId;
    };
}