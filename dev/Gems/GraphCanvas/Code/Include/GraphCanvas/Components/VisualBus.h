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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

#include <QGraphicsItem>

namespace AZ
{
    class Vector2;
}

class QGraphicsLayoutItem;
class QGraphicsLayout;

namespace GraphCanvas
{

    //! RootVisualRequests
    //! Each entity that is going to be rendered in the scene (nodes, connections, etc.) must provide a "root" visual.
    //! Root visuals must be QGraphicsItems or QGraphicsLayoutItems so that they can be added to the QGraphicsScene
    //! that lies at the core of the Scene component.
    class RootVisualRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;


        //! If the entity provides a root visual that's directly a QGraphicsItem, this will return a pointer to it.
        virtual QGraphicsItem* GetRootGraphicsItem() = 0;
        //! If the entity provides a root visual that's a QGraphicsLayoutItem, this will return a pointer to it.
        //! This generally would apply to slot visuals and those will often need to be subordinate to a node's
        //! layout.
        virtual QGraphicsLayoutItem* GetRootGraphicsLayoutItem() = 0;
    };

    using RootVisualRequestBus = AZ::EBus<RootVisualRequests>;

    //! VisualRequests
    //! Similar to the root visual, which is just the top-level one that will be parented by an owning entity (such as
    //! in the node/slot relationship), every other visual needs to be referenced by means of its Qt interface.
    class VisualRequests : public AZ::EBusTraits
    {
    public:
        // Allow any number of handlers per address.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;


        //! If the visual is a QGraphicsItem, this method will return a pointer to it.
        virtual QGraphicsItem* AsGraphicsItem() = 0;
        //! If the visual is a QGraphicsLayoutItem, this method will return a pointer to that interface.
        //! The default is to return \code nullptr.
        virtual QGraphicsLayoutItem* AsGraphicsLayoutItem() { return nullptr; }

        //! Does the visual contain a given scene coordinate?
        virtual bool Contains(const AZ::Vector2&) const = 0;

        //! Show or hide this element.
        virtual void SetVisibility(bool) {};
        //! Get the visibility of this element.
        virtual bool GetVisibility() const { return false; };
    };

    using VisualRequestBus = AZ::EBus<VisualRequests>;


    //! VisualNotifications
    //! Notifications that provide access to various QGraphicsItem events that are of interest
    class VisualNotifications : public AZ::EBusTraits
    {
    public:
        // Allow any number of handlers per address.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        

        //! Indicates that the mouse has begun hovering over a visual.
        //! This is only emitted for visuals that are hover-enabled (see QGraphicsItem::setAcceptHoverEvents).
        //!
        //! The \code QGraphicsItem the event is for is passed because multiple \code QGraphicsItems can be associated
        //! with one entity (i.e. child visuals).
        virtual void OnHoverEnter(QGraphicsItem*) {}
        //! Indicates that the mouse has stopped hovering over a visual.
        //! This is only emitted for visuals that are hover-enabled (see QGraphicsItem::setAcceptHoverEvents).
        //!
        //! The \code QGraphicsItem the event is for is passed because multiple \code QGraphicsItems can be associated
        //! with one entity (i.e. child visuals).
        virtual void OnHoverLeave(QGraphicsItem*) {}

        virtual bool OnKeyPress(const AZ::EntityId&, const QKeyEvent*) { return false; };
        //! Receives KeyRelease events when the visual has focus
        //! This is only emitted for visuals with GraphicsItemFlag::ItemIsFocusable set
        virtual bool OnKeyRelease(const AZ::EntityId&, const QKeyEvent*) { return false; };

        virtual bool OnMousePress(const AZ::EntityId&, const QGraphicsSceneMouseEvent*) { return false; };
        virtual bool OnMouseRelease(const AZ::EntityId&, const QGraphicsSceneMouseEvent*) { return false; };
        virtual bool OnMouseDoubleClick(const AZ::EntityId&, const QGraphicsSceneMouseEvent*) { return false; };

        //! Forwards QGraphicsItem::onItemChange events to the event bus system.
        //! QGraphicsItems can produce a wide variety of informational events, relating to all sorts of changes in their
        //! state. See QGraphicsItem::itemChange and QGraphicsItem::GraphicsItemChange.
        //!
        //! # Parameters
        //! 1. The entity that has changed.
        //! 2. The type of change.
        //! 3. The value (if any) associated with the change.
        virtual void OnItemChange(const AZ::EntityId&, QGraphicsItem::GraphicsItemChange, const QVariant&) {}
    };

    using VisualNotificationBus = AZ::EBus<VisualNotifications>;

}