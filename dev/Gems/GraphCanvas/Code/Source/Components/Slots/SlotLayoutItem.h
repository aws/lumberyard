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

#include <QGraphicsItem>
#include <QGraphicsLayoutItem>

#include <GraphCanvas/Components/VisualBus.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{
    //! Generates Ebus notifications for QGraphicsItem event. 
    //! Requires that the derived class has a GetEntityId() function
    class SlotLayoutItem
        : public QGraphicsItem
        , public QGraphicsLayoutItem
    {
    public:

        AZ_RTTI(SlotLayoutItem, "{ED76860E-35B8-4FEE-A2A0-04B467F778B6}", QGraphicsLayoutItem, QGraphicsItem);
        AZ_CLASS_ALLOCATOR(SlotLayoutItem, AZ::SystemAllocator, 0);

        SlotLayoutItem()
        {
            setGraphicsItem(this);
            setAcceptHoverEvents(true);
            setOwnedByLayout(false);
        }
        virtual ~SlotLayoutItem() = default;

        const Styling::StyleHelper& GetStyle() const { return m_style; }

    protected:
        // QGraphicsItem  
        void mousePressEvent(QGraphicsSceneMouseEvent* event)
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMousePress, GetEntityId(), event);
            if (!result)
            {
                QGraphicsItem::mousePressEvent(event);
            }
        }

        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMouseRelease, GetEntityId(), event);
            if (!result)
            {
                QGraphicsItem::mouseReleaseEvent(event);
            }
        }

        void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override
        {
            if (GetEntityId().IsValid())
            {
                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnHoverEnter, this);
            }
            QGraphicsItem::hoverEnterEvent(event);
        }

        void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
        {
            if (GetEntityId().IsValid())
            {
                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnHoverLeave, this);
            }
            QGraphicsItem::hoverLeaveEvent(event);
        }

        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMouseDoubleClick, GetEntityId(), event);
            if (!result)
            {
                QGraphicsItem::mouseDoubleClickEvent(event);
            }
        }

        void keyPressEvent(QKeyEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnKeyPress, GetEntityId(), event);
            if (!result)
            {
                QGraphicsItem::keyPressEvent(event);
            }
        }

        void keyReleaseEvent(QKeyEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnKeyRelease, GetEntityId(), event);
            if (!result)
            {
                QGraphicsItem::keyReleaseEvent(event);
            }
        }

        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override
        {
            if (GetEntityId().IsValid())
            {
                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnItemChange, GetEntityId(), change, value);
            }
            return QGraphicsItem::itemChange(change, value);
        }

        virtual void RefreshStyle() { };
        virtual AZ::EntityId GetEntityId() const = 0;

        Styling::StyleHelper m_style;
    };
}
