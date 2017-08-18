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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#ifndef MovementUpdateContext_h
#define MovementUpdateContext_h

struct IMovementActor;
struct IMovementSystem;
class CPipeUser;
class IPathFollower;
struct SOBJECTSTATE;
struct MovementRequest;

namespace Movement
{
    struct IPlanner;
}

// Contains information needed while updating the movement system.
// Constructed every frame in the movement system update.
struct MovementUpdateContext
{
    MovementUpdateContext(
        IMovementActor& _actor,
        IMovementSystem& _movementSystem,
        IPathFollower& _pathFollower,
        Movement::IPlanner& _planner,
        const float _updateTime
        )
        : actor(_actor)
        , movementSystem(_movementSystem)
        , pathFollower(_pathFollower)
        , planner(_planner)
        , updateTime(_updateTime)
    {
    }

    IMovementActor& actor;
    IMovementSystem& movementSystem;
    IPathFollower& pathFollower;
    Movement::IPlanner& planner;
    const float updateTime;
};

#endif // MovementUpdateContext_h