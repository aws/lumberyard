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
#include <ScriptCanvas/Core/Core.h>
#include <AzCore/EBus/EBus.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        class ConnectionRequests
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // Bus configuration
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            //////////////////////////////////////////////////////////////////////////

            virtual void Attach(const AZ::EntityId& graphId) = 0;
            virtual void Detach(const AZ::EntityId& graphId) = 0;
            virtual void DetachAll() = 0;

        };

        using ConnectionRequestBus = AZ::EBus<ConnectionRequests>;

        class Requests
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // Bus configuration
            using BusIdType = AZ::EntityId;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            //////////////////////////////////////////////////////////////////////////

            virtual void SetBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot) = 0;
            virtual void RemoveBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot) = 0;
            
            virtual void StepOver() = 0;
            virtual void StepIn() = 0;
            virtual void StepOut() = 0;
            virtual void Stop() = 0;
            virtual void Continue() = 0;

            virtual void NodeStateUpdate(Node* /*node*/, const AZ::EntityId& /*graphId*/) {}
        };

        using RequestBus = AZ::EBus<Requests>;

        class Notifications
            : public AZ::EBusTraits
        {
        public:
            virtual void OnAttach(const AZ::EntityId& /*graphId*/) {}
            virtual void OnDetach(const AZ::EntityId& /*graphId*/) {}

            virtual void OnNodeEntry(Node* /*node*/, const AZ::EntityId& /*graphId*/) {}
        };
        using NotificationBus = AZ::EBus<Notifications>;
    }
}