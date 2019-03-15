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

#include <AzCore/Component/ComponentBus.h>

namespace PhysX
{
    /**
     * Services provided by the PhysX Trigger Area Component.
     */
    class PhysXTriggerAreaEvents
        : public AZ::ComponentBus
    {
    public:
        // Ebus Traits. ID'd on trigger entity Id
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual ~PhysXTriggerAreaEvents() {}

        /// Dispatched when an entity enters a trigger. The bus message is ID'd on the triggers entity Id.
        virtual void OnTriggerEnter(AZ::EntityId /*entityEntering*/) {};

        /// Dispatched when an entity exits a trigger. The bus message is ID'd on the triggers entity Id.
        virtual void OnTriggerExit(AZ::EntityId /*entityExiting*/) {};
    };

    /**
     * Bus to service the PhysX Trigger Area Component event group.
     */
    using PhysXTriggerAreaEventBus = AZ::EBus<PhysXTriggerAreaEvents>;
} // namespace PhysX