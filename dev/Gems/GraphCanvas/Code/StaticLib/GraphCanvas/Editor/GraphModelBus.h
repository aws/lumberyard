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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{    
    class NodePropertyDisplay;
    
    class GraphSettingsRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = GraphId;
    };
    
    using GraphSettingsRequestBus = AZ::EBus<GraphSettingsRequests>;
    
    class GraphModelRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = GraphId;

        //! Callback for requesting an Undo Point to be posted.
        virtual void RequestUndoPoint() = 0;

        //! Callback for requesting the incrementation of the value of the ignore undo point tracker
        virtual void RequestPushPreventUndoStateUpdate() = 0;

        //! Callback for requesting the decrementation of the value of the ignore undo point tracker
        virtual void RequestPopPreventUndoStateUpdate() = 0;

        //! Request to create a NodePropertyDisplay class for a particular DataSlot.
        virtual NodePropertyDisplay* CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const NodeId& nodeId, const SlotId& slotId) const { return nullptr; }
        virtual NodePropertyDisplay* CreateDataSlotVariablePropertyDisplay(const AZ::Uuid& dataType, const NodeId& nodeId, const SlotId& slotId) const { return nullptr; }
        virtual NodePropertyDisplay* CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const NodeId& nodeId, const SlotId& slotId) const { return nullptr; }

        //! This is sent when a connection is disconnected.
        virtual void DisconnectConnection(const ConnectionId& connectionId) = 0;

        //! This is sent when attempting to create a given connection.
        virtual bool CreateConnection(const ConnectionId& connectionId, const Endpoint& sourcePoint, const Endpoint& targetPoint) = 0;

        //! This is sent to confirm whether or not a connection can take place.
        virtual bool IsValidConnection(const Endpoint& sourcePoint, const Endpoint& targetPoint) const = 0;

        //! This is sent to confirm whether or not a variable assignment can take place.
        virtual bool IsValidVariableAssignment(const AZ::EntityId& variableId, const Endpoint& targetPoint) const = 0;        

        //! Get the Display Type name for the given AZ type
        virtual AZStd::string GetDataTypeString(const AZ::Uuid& typeId) = 0;

        // Signals out that the specified elements save data is dirty.
        virtual void OnSaveDataDirtied(const AZ::EntityId& savedElement) = 0;

        // Returns whether or not the specified wrapper node should accept the given drop
        virtual bool ShouldWrapperAcceptDrop(const NodeId& wrapperNode, const QMimeData* mimeData) const
        {
            AZ_UNUSED(wrapperNode); AZ_UNUSED(mimeData);
            return false;
        }

        // Signals out that we want to drop onto the specified wrapper node
        virtual void AddWrapperDropTarget(const NodeId& wrapperNode) { AZ_UNUSED(wrapperNode);  };

        // Signals out that we no longer wish to drop onto the specified wrapper node
        virtual void RemoveWrapperDropTarget(const NodeId& wrapperNode) { AZ_UNUSED(wrapperNode); }
    };

    using GraphModelRequestBus = AZ::EBus<GraphModelRequests>;

    class GraphModelNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = GraphId;        
    };

    using GraphModelNotificationBus = AZ::EBus<GraphModelNotifications>;
}