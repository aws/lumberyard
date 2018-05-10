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

#include <QPoint>
#include <QRect>

#include <AzCore/EBus/EBus.h>

#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Editor/EditorTypes.h>

#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Core/Endpoint.h>

#include <ScriptCanvas/Variable/VariableCore.h>

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

    class EBusHandlerNodeDescriptorRequests : public AZ::EBusTraits
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
        virtual GraphCanvas::Endpoint FindEventNodeEndpoint(const ScriptCanvas::SlotId& slotId) const = 0;
    };

    using EBusHandlerNodeDescriptorRequestBus = AZ::EBus<EBusHandlerNodeDescriptorRequests>;

    class EBusHandlerEventNodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZStd::string GetBusName() const = 0;
        virtual AZStd::string GetEventName() const = 0;
    };

    using EBusHandlerEventNodeDescriptorRequestBus = AZ::EBus<EBusHandlerEventNodeDescriptorRequests>;

    class VariableNodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual ScriptCanvas::VariableId GetVariableId() const = 0;
    };

    using VariableNodeDescriptorRequestBus = AZ::EBus<VariableNodeDescriptorRequests>;

    // Bus used when trying to remove variables to help signal to the user how many
    // Nodes will be erased by their action.
    class VariableGraphMemberRefCountRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvas::VariableId;

        virtual AZ::EntityId GetGraphMemberId() const = 0;
    };

    using VariableGraphMemberRefCountRequestBus = AZ::EBus<VariableGraphMemberRefCountRequests>;

    class SceneCounterRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the scene.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual AZ::u32 GetNewVariableCounter() = 0;
    };

    using SceneCounterRequestBus = AZ::EBus<SceneCounterRequests>;

    class ScriptCanvasWrapperNodeDescriptorRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = GraphCanvas::NodeId;

        virtual void OnWrapperAction(const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint) = 0;
    };

    using ScriptCanvasWrapperNodeDescriptorRequestBus = AZ::EBus<ScriptCanvasWrapperNodeDescriptorRequests>;

    namespace Deprecated
    {
        class VariableNodeDescriptorRequests : public AZ::EBusTraits
        {
        public:
            //! The id here is the id of the node.
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;

            virtual ScriptCanvas::Endpoint GetReadEndpoint() const = 0;
            virtual ScriptCanvas::Endpoint GetWriteEndpoint() const = 0;
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

        class GetVariableNodeDescriptorRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;

            virtual AZ::EntityId GetVariableId() const = 0;
        };

        using GetVariableNodeDescriptorRequestBus = AZ::EBus<GetVariableNodeDescriptorRequests>;

        class SetVariableNodeDescriptorRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;

            virtual AZ::EntityId GetVariableId() const = 0;
        };

        using SetVariableNodeDescriptorRequestBus = AZ::EBus<SetVariableNodeDescriptorRequests>;
    }
}