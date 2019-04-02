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

namespace AZ
{
    class Vector3;
}

namespace Physics
{
    /// Messages serviced by character controllers.
    class CharacterRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~CharacterRequests() = default;

        /// Gets the base position of the character.
        virtual AZ::Vector3 GetBasePosition() const = 0;

        /// Directly moves (teleports) the character to a new base position.
        /// @param position The new base position for the character.
        virtual void SetBasePosition(const AZ::Vector3& position) = 0;

        /// Gets the position of the center of the character.
        virtual AZ::Vector3 GetCenterPosition() const = 0;

        /// Gets the step height (the parameter which affects how high the character can step).
        virtual float GetStepHeight() const = 0;

        /// Sets the step height (the parameter which affects how high the character can step).
        /// @param stepHeight The new value for the step height parameter.
        virtual void SetStepHeight(float stepHeight) = 0;

        /// Gets the character's up direction (the direction used by various controller logic, for example stepping).
        virtual AZ::Vector3 GetUpDirection() const = 0;

        /// Sets the character's up direction (the direction used by various controller logic, for example stepping).
        /// @param upDirection The new value for the up direction.
        virtual void SetUpDirection(const AZ::Vector3& upDirection) = 0;

        /// Gets the maximum slope which the character can climb, in degrees.
        virtual float GetSlopeLimitDegrees() const = 0;

        /// Sets the maximum slope which the character can climb, in degrees.
        /// @param slopeLimitDegrees the new slope limit value (in degrees, should be between 0 and 90).
        virtual void SetSlopeLimitDegrees(float slopeLimitDegrees) = 0;

        /// Gets the observed velocity of the character, which may differ from the desired velocity if the character is obstructed.
        virtual AZ::Vector3 GetVelocity() const = 0;

        /// Tries to move the character by the requested position change relative to its current position.
        /// Obstacles may prevent the actual movement from exactly matching the requested movement.
        /// @param deltaPosition The change in position relative to the current position.
        /// @param deltaTime Elapsed time.
        /// @return The new base (foot) position.
        virtual AZ::Vector3 TryRelativeMove(const AZ::Vector3& deltaPosition, float deltaTime) = 0;
    };

    using CharacterRequestBus = AZ::EBus<CharacterRequests>;
} // namespace Physics
