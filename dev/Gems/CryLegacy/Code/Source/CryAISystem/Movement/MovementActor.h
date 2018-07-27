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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTACTOR_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTACTOR_H
#pragma once

#include <IMovementActor.h>
#include "MovementRequest.h"
#include <IPathfinder.h>

// The movement system processes the movement requests for an actor
// in the order they come. Eaten off front-to-back. This is the queue.
typedef std::deque<MovementRequestID> MovementRequestQueue;

// -------------------------------------------------------------------------------------------

// Contains the information related to one specific actor in the
// context of movement.
// Retrieve information about an actor through this object.
struct MovementActor
    : public IMovementActor
{
    MovementActor(const EntityId _entityID, IMovementActorAdapter* _pAdapter)
        : entityID(_entityID)
        , requestIdCurrentlyInPlanner(0)
        , lastPointInPathIsSmartObject(false)
        , pAdapter(_pAdapter)
    {
        assert(pAdapter);
    }

    virtual ~MovementActor() {}

    // IMovementActor
    virtual IMovementActorAdapter& GetAdapter() const override;
    virtual void RequestPathTo(const Vec3& destination, float lengthToTrimFromThePathEnd, const MNMDangersFlags dangersFlags,
        const bool considerActorsAsPathObstacles) override;
    virtual Movement::PathfinderState GetPathfinderState() const override;
    virtual const char* GetName() const override;
    virtual void Log(const char* message) override;

    virtual bool IsLastPointInPathASmartObject() const { return lastPointInPathIsSmartObject; }
    virtual EntityId GetEntityId() const { return entityID; };
    virtual MovementActorCallbacks GetCallbacks() const { return callbacks; };
    // ~IMovementActor

    void SetLowCoverStance();

    CAIActor* GetAIActor();

    AZStd::shared_ptr<Movement::IPlanner> planner;
    EntityId entityID;
    MovementRequestQueue requestQueue;
    MovementRequestID requestIdCurrentlyInPlanner;
    bool lastPointInPathIsSmartObject;

    MovementActorCallbacks callbacks;
private:
    IMovementActorAdapter* pAdapter;
};

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTACTOR_H
