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

#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeLayoutBus.h>
#include <GraphCanvas/Types/Endpoint.h>

#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Core/Endpoint.h>

namespace GraphCanvas
{
    class StringDataInterface;
    class VariableReferenceDataInterface;
}

namespace ScriptCanvasEditor
{
    enum class NodeDescriptorType
    {
        Unknown,
        EBusHandler,
        EBusHandlerEvent,
        EBusSender,
        EntityRef,
        VariableNode,
        SetVariable,
        GetVariable,
        UserDefined,
        ClassMethod,
        
        Invalid
    };
    
    class NodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual NodeDescriptorType GetType() const = 0;
        virtual bool IsType(const NodeDescriptorType& descriptorType)
        { 
            NodeDescriptorType localType = GetType();
            
            if (localType == NodeDescriptorType::Invalid
                || descriptorType == NodeDescriptorType::Invalid)
            {
                return false;
            }
            
            return GetType() == descriptorType;
        }
    };

    using NodeDescriptorRequestBus = AZ::EBus<NodeDescriptorRequests>;

    class EBusNodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZStd::string GetBusName() const = 0;

        virtual GraphCanvas::WrappedNodeConfiguration GetEventConfiguration(const AZStd::string& eventName) const = 0;
        virtual bool ContainsEvent(const AZStd::string& eventName) const = 0;

        virtual AZStd::vector< AZStd::string > GetEventNames() const = 0;

        virtual AZ::EntityId FindEventNodeId(const AZStd::string& eventName) const = 0;
    };

    using EBusNodeDescriptorRequestBus = AZ::EBus<EBusNodeDescriptorRequests>;

    class EBusEventNodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual bool IsWrapped() const = 0;
        virtual NodeIdPair GetEbusWrapperNodeId() const =0;

        virtual AZStd::string GetBusName() const = 0;
        virtual AZStd::string GetEventName() const = 0;
    };

    using EBusEventNodeDescriptorRequestBus = AZ::EBus<EBusEventNodeDescriptorRequests>;

    class VariableNodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual GraphCanvas::StringDataInterface* CreateNameInterface() = 0;

        virtual ScriptCanvas::Endpoint GetReadEndpoint() const = 0;
        virtual ScriptCanvas::Endpoint GetWriteEndpoint() const = 0;

        virtual void AddConnection(const GraphCanvas::Endpoint& endpoint, const AZ::EntityId& scConnectionId) = 0;
        virtual void RemoveConnection(const GraphCanvas::Endpoint& endpoint) = 0;

        virtual AZ::EntityId FindConnection(const GraphCanvas::Endpoint& endpoint) = 0;
    };

    using VariableNodeDescriptorRequestBus = AZ::EBus<VariableNodeDescriptorRequests>;

    class VariableNodeDescriptorNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnNameChanged() {}
    };

    using VariableNodeDescriptorNotificationBus = AZ::EBus<VariableNodeDescriptorNotifications>;

    class VariableNodeSceneRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the scene.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZ::u32 GetNewVariableCounter() = 0;
    };

    using VariableNodeSceneRequestBus = AZ::EBus<VariableNodeSceneRequests>;

    class GetVariableNodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual GraphCanvas::VariableReferenceDataInterface* CreateVariableDataInterface() = 0;
        virtual ScriptCanvas::Endpoint GetSourceEndpoint(ScriptCanvas::SlotId slotId) const = 0;
		
        virtual AZ::EntityId GetVariableId() const = 0;
        virtual void SetVariableId(const AZ::EntityId& variableId) = 0;
    };

    using GetVariableNodeDescriptorRequestBus = AZ::EBus<GetVariableNodeDescriptorRequests>;

    class GetVariableNodeDescriptorNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

		virtual void OnVariableActivated() {};
        virtual void OnAssignVariableChanged() {};
    };

    using GetVariableNodeDescriptorNotificationBus = AZ::EBus<GetVariableNodeDescriptorNotifications>;

    class SetVariableNodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual GraphCanvas::VariableReferenceDataInterface* CreateVariableDataInterface() = 0;
		
        virtual AZ::EntityId GetVariableId() const = 0;
        virtual void SetVariableId(const AZ::EntityId& variableId) = 0;
    };

    using SetVariableNodeDescriptorRequestBus = AZ::EBus<SetVariableNodeDescriptorRequests>;

    class SetVariableNodeDescriptorNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

		virtual void OnVariableActivated() {};
		virtual void OnAssignVariableChanged() {};
    };

    using SetVariableNodeDescriptorNotificationBus = AZ::EBus<SetVariableNodeDescriptorNotifications>;
}