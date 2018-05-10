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

#include <QPointF>

#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/Types.h>

class QGraphicsItem;

namespace AZ
{
    class any;
}

namespace GraphCanvas
{

    //! Represents whether a connection can be made between two entities, including a detail message explaining why.
    struct Connectability
    {
        enum Status
        {
            NotConnectable,
            Connectable,
            AlreadyConnected
        };

        AZ::EntityId entity;
        Status status;
        AZStd::string details;

        Connectability()
            : status(NotConnectable)
            , details(AZStd::string())
        {
        }

        Connectability(AZ::EntityId entity, Status status, const AZStd::string& details = AZStd::string())
            : entity(entity)
            , status(status)
            , details(details)
        {
        }

        Connectability& operator=(const Connectability&) = default;
    };

    //! ConnectionRequests
    //! Requests which are serviced by the \ref GraphCanvas::Connection component.
    class ConnectionRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Get this connection's source slot ID.
        virtual AZ::EntityId GetSourceSlotId() const = 0;
        //! Resolve the node the source slot belongs to.
        virtual AZ::EntityId GetSourceNodeId() const = 0;
        //! Retrieves the source endpoint for this connection
        virtual Endpoint GetSourceEndpoint() const = 0;
        //! Retrieves the source position for this connection
        virtual QPointF GetSourcePosition() const = 0;
        //! Begins moving the source of this connection.
        virtual void StartSourceMove() = 0;

        //! Get this connection's target slot ID.
        virtual AZ::EntityId GetTargetSlotId() const = 0;
        //! Resolve the Node the target slot belongs to.
        virtual AZ::EntityId GetTargetNodeId() const = 0;
        //! Retrieves the target endpoint for this connection
        virtual Endpoint GetTargetEndpoint() const = 0;
        //! Retrieves the target position for this connection
        virtual QPointF GetTargetPosition() const = 0;
        //! Begins moving the target of this connection.
        virtual void StartTargetMove() = 0;

        virtual bool ContainsEndpoint(const Endpoint& endpoint) const = 0;

        //! Get this connection's tooltip.
        virtual AZStd::string GetTooltip() const = 0;
        //! Set this connection's tooltip.
        virtual void SetTooltip(const AZStd::string&) {}

        //! Get user data from this connection
        virtual AZStd::any* GetUserData() = 0;
    };

    using ConnectionRequestBus = AZ::EBus<ConnectionRequests>;

    //! ConnectionNotifications
    //! Notifications about changes to a connection's state.
    class ConnectionNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! The source slot for the connection changed
        //! # Parameters
        //! 1. The previous source slot entity ID.
        //! 2. The new source slot entity ID.
        virtual void OnSourceSlotIdChanged(const AZ::EntityId&, const AZ::EntityId&) {};
        //! The target slot for the connection changed.
        //! # Parameters
        //! 1. The previous target slot entity ID.
        //! 2. The new target slot entity ID.
        virtual void OnTargetSlotIdChanged(const AZ::EntityId&, const AZ::EntityId&) {};
        //! The connection's tooltip changed.
        virtual void OnTooltipChanged(const AZStd::string&) {};
    };

    using ConnectionNotificationBus = AZ::EBus<ConnectionNotifications>;

    // Various requests that can be made of the Connection visuals
    class ConnectionUIRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void UpdateConnectionPath() = 0;
    };

    using ConnectionUIRequestBus = AZ::EBus<ConnectionUIRequests>;

    //! Requests that are serviced by any object that wishes to be connectible with other entities by means of
    //! GraphCanvas::Connection.
    class ConnectableObjectRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Check whether the entity is connectible with another entity.
        //! # Parameters
        //! 1. The entity that is asking for connection partners
        virtual Connectability CanConnectWith(const AZ::EntityId& slotId) const = 0;
    };

    using ConnectableObjectRequestBus = AZ::EBus<ConnectableObjectRequests>;
}
