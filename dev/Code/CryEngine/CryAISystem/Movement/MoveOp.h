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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEOP_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEOP_H
#pragma once

#include "EnterLeaveUpdateGoalOp.h"
#include <MovementRequest.h>
#include <MovementStyle.h>

struct MovementRequestResult;

class MoveOp
    : public EnterLeaveUpdateGoalOp
{
public:
    MoveOp();
    MoveOp(const XmlNodeRef& node);

    virtual void Enter(CPipeUser& pipeUser);
    virtual void Leave(CPipeUser& pipeUser);
    virtual void Update(CPipeUser& pipeUser);

    void SetMovementStyle(MovementStyle movementStyle);
    void SetStopWithinDistance(float distance);
    void SetRequestStopWhenLeaving(bool requestStop);

    enum DestinationType
    {
        Target,
        Cover,
        ReferencePoint,
        FollowPath,
        Formation,
    };

private:
    Vec3 DestinationPositionFor(CPipeUser& pipeUser);
    Vec3 GetCoverRegisterLocation(const CPipeUser& pipeUser) const;
    void RequestMovementTo(const Vec3& position, CPipeUser& pipeUser);
    void RequestFollowExistingPath(const char* pathName, CPipeUser& pipeUser);
    void ReleaseCurrentMovementRequest();
    void MovementRequestCallback(const MovementRequestResult& result);
    void ChaseTarget(CPipeUser& pipeUser);
    float GetSquaredDistanceToDestination(CPipeUser& pipeUser);
    void RequestStop(CPipeUser& pipeUser);
    void GetClosestDesignedPath(CPipeUser& pipeUser, stack_string& closestPathName);
    void SetupDangersFlagsForDestination(bool shouldAvoidDangers);
    Vec3 m_destinationAtTimeOfMovementRequest;
    float m_stopWithinDistanceSq;
    MovementStyle m_movementStyle;
    MovementRequestID m_movementRequestID;
    DestinationType m_destination;
    stack_string m_pathName;
    MNMDangersFlags m_dangersFlags;
    bool m_requestStopWhenLeaving;

    // When following according to the formation, we impose 2 radii around our formation slot:
    // If we're *outside* the outer radius and behind, we catch up until we are inside the *inner* radius. From then on
    // we simply match our speed with that of the formation owner. If we fall behind outside the outer radius,
    // we catch up again. If we're outside the outer radius but *ahead*, then we slow down.
    struct FormationInfo
    {
        enum State
        {
            State_MatchingLeaderSpeed,  // we're inside the inner radius
            State_CatchingUp,           // we're outside the outer radius and behind our formation slot
            State_SlowingDown,          // we're outside the outer radius and ahead of our formation slot
        };

        State state;
        Vec3 positionInFormation;
        bool positionInFormationIsReachable;

        FormationInfo()
            : state(State_MatchingLeaderSpeed)
            , positionInFormation(ZERO)
            , positionInFormationIsReachable(true)
        {}
    };

    FormationInfo m_formationInfo;      // only used if m_destination == Formation, ignored for all other destination types
};

struct MoveOpDictionaryCollection
{
    MoveOpDictionaryCollection();

    CXMLAttrReader<MoveOp::DestinationType> destinationTypes;
};

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEOP_H
