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

#include "TagComponentBus.h"
#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /*
    * Services requests made to the Trigger Area Component
    */
    class TriggerAreaRequests
        : public AZ::ComponentBus
    {
        public:
            //! Adds a required tag to the Activation filtering criteria of this component
            virtual void AddRequiredTag(const Tag& requiredTag) = 0;
            
            //! Removes a required tag from the Activation filtering criteria of this component
            virtual void RemoveRequiredTag(const Tag& requiredTag) = 0;

            //! Adds an excluded tag to the Activation filtering criteria of this component
            virtual void AddExcludedTag(const Tag& excludedTag) = 0;

            //! Removes an excluded tag from the Activation filtering criteria of this component
            virtual void RemoveExcludedTag(const Tag& excludedTag) = 0;
    };

    using TriggerAreaRequestsBus = AZ::EBus<TriggerAreaRequests>;

    /**
     * Events fired for a given trigger area when an entity enters or leaves.
     */
    class TriggerAreaNotifications
        : public AZ::ComponentBus
    {
    public:

        /// Sent when enteringEntityId enters this trigger.
        virtual void OnTriggerAreaEntered(AZ::EntityId enteringEntityId) { (void)enteringEntityId; }

        /// Sent when enteringEntityId exits this trigger.
        virtual void OnTriggerAreaExited(AZ::EntityId exitingEntityId) { (void)exitingEntityId; }
    };

    using TriggerAreaNotificationBus = AZ::EBus<TriggerAreaNotifications>;

    /**
     * Events fired for a given entity when entering or leaving a trigger area.
     */
    class TriggerAreaEntityNotifications
        : public AZ::ComponentBus
    {
    public:

        /// Sent when enteringEntityId enters this trigger.
        virtual void OnEntityEnteredTriggerArea(AZ::EntityId triggerId) { (void)triggerId; }

        /// Sent when enteringEntityId exits this trigger.
        virtual void OnEntityExitedTriggerArea(AZ::EntityId triggerId) { (void)triggerId; }
    };

    using TriggerAreaEntityNotificationBus = AZ::EBus<TriggerAreaEntityNotifications>;
} // namespace LmbrCentral
