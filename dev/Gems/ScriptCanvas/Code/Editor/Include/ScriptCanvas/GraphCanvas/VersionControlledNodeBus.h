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

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    class VersionControlledScrapperRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        
        virtual bool IsOutOfDate() const = 0;
        virtual AZ::EntityId GetNodeId() const = 0;
    };
    
    using VersionControlledScrapperBus = AZ::EBus<VersionControlledScrapperRequests>;
    
    class VersionControlledNodeRequests : public AZ::EBusTraits
    {
    public:
        //! The id here is the id of the node.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual void UpdateNodeVersion() = 0;
    };

    using VersionControlledNodeRequestBus = AZ::EBus<VersionControlledNodeRequests>;

    class VersionControlledNodeNotifications : public AZ::EBusTraits
    {
    public:
        // This bus will be keyed off of the GraphCanvas::MemberId.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnVersionConversionBegin() {};
        virtual void OnVersionConversionEnd() {};
    };

    using VersionControlledNodeNotificationBus = AZ::EBus<VersionControlledNodeNotifications>;

    class VersionControlledNodeInterface
        : public VersionControlledScrapperBus::Handler
        , public VersionControlledNodeRequestBus::Handler
    {
    public:
        void SetVersionedNodeId(AZ::EntityId nodeId)
        {
            m_nodeId = nodeId;

            VersionControlledScrapperBus::Handler::BusConnect();
            VersionControlledNodeRequestBus::Handler::BusConnect(m_nodeId);
        }

        AZ::EntityId GetNodeId() const override
        {
            return m_nodeId;
        }

    private:

        AZ::EntityId m_nodeId;
    };
}