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

namespace LmbrCentral
{
    /*!
    * ConstraintComponentRequests
    * Messages serviced by the Constraint component.
    */
    class ConstraintComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual ~ConstraintComponentRequests() {}

        //! Sets the entity that will own the contraint and the contraint target (target can be targetEntity is invalid if constrained to world space)
        virtual void SetConstraintEntities(const AZ::EntityId& owningEntity, const AZ::EntityId& targetEntity) = 0;

        //! Sets the entity that will own the constraint, the target entity, and the animation part ids (bone ids) for the constraint to be centered attached to
        virtual void SetConstraintEntitiesWithPartIds(const AZ::EntityId& owningEntity, int ownerPartId, const AZ::EntityId& targetEntity, int targetPartId) = 0;

        //! Enable all constraints on this entity
        virtual void EnableConstraint() = 0;

        //! Disable all constraints on this entity
        virtual void DisableConstraint() = 0;
    };
    using ConstraintComponentRequestBus = AZ::EBus<ConstraintComponentRequests>;


    /*!
    * ConstraintComponentNotificationBus
    * Events dispatched by the Constraint Component
    */
    class ConstraintComponentNotifications : public AZ::ComponentBus
    {
    public:
        virtual ~ConstraintComponentNotifications() {}

        //! Fires when either the constraint owner or target changes (target is invalid if constrained to world space), note this will also fire if only partIds were changed
        virtual void OnConstraintEntitiesChanged(const AZ::EntityId& oldOwnwer, const AZ::EntityId& oldTarget, const AZ::EntityId& newOwner, const AZ::EntityId& newTarget) {}

        // Fires when constraints have been enabled on this entity
        virtual void OnConstraintEnabled() {}

        // Constraints have been disabled
        virtual void OnConstraintDisabled() {}
    };
    using ConstraintComponentNotificationBus = AZ::EBus<ConstraintComponentNotifications>;
}