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

#ifndef CRYINCLUDE_CRYCOMMON_MOVEMENTREQUEST_H
#define CRYINCLUDE_CRYCOMMON_MOVEMENTREQUEST_H
#pragma once

#include <MovementRequestID.h>
#include <MovementStyle.h>
#include "IPathfinder.h"    // MNMDangersFlags

// Passed along as a parameter to movement request callbacks.
struct MovementRequestResult
{
    enum Result
    {
        Success,
        Failure,

        // Alias for 'Success' to increase readability in callback code.
        ReachedDestination = Success,
    };

    enum FailureReason
    {
        NoReason,
        CouldNotFindPathToRequestedDestination,
        CouldNotMoveAlongDesignerDesignedPath,
        FailedToProduceSuccessfulPlanAfterMaximumNumberOfAttempts,
    };

    MovementRequestResult(
        const MovementRequestID& _id,
        const Result _result,
        const FailureReason _failureReason)
        : requestID(_id)
        , result(_result)
        , failureReason(_failureReason)
    {
    }

    MovementRequestResult(
        const MovementRequestID& _id,
        const Result _result)
        : requestID(_id)
        , result(_result)
        , failureReason(NoReason)
    {
    }

    bool operator == (const Result& rhs) const
    {
        return result == rhs;
    }

    operator bool () const
    {
        return result == Success;
    }

    const MovementRequestID requestID;
    const Result result;
    const FailureReason failureReason;
};

// Contains everything needed for the movement system to make informed
// decisions about movement. You specify where you want to move
// and how you want to do it. You can receive information about
// your request by setting up a callback function.
struct MovementRequest
{
    typedef Functor1<const MovementRequestResult&> Callback;

    enum Type
    {
        MoveTo,
        Stop,
        CountTypes,  // this has to stay the last one for places where the number of elements of this enum is relevant
    };

    MovementRequest()
        : destination(ZERO)
        , type(MoveTo)
        , callback(0)
        , entityID(0)
        , dangersFlags(eMNMDangers_None)
        , considerActorsAsPathObstacles(false)
        , lengthToTrimFromThePathEnd(0.0f)
    {
    }

#ifdef COMPILE_WITH_MOVEMENT_SYSTEM_DEBUG
    static const char* GetTypeAsDebugName(Type type)
    {
        COMPILE_TIME_ASSERT(CountTypes == 2);   // if this fails, then most likely a new value in the Type enum got introduced

        if (type == MoveTo)
        {
            return "MoveTo";
        }
        else if (type == Stop)
        {
            return "Stop";
        }

        return "Undefined";
    }
#endif

    MovementStyle style;
    Vec3 destination;
    Type type;
    Callback callback;
    EntityId entityID;
    MNMDangersFlags dangersFlags;
    bool considerActorsAsPathObstacles;
    float lengthToTrimFromThePathEnd;
};

// Contains information about the status of a request.
// You'll see if it's in a queue, path finding, or
// what block of a plan it's currently executing.
struct MovementRequestStatus
{
    MovementRequestStatus()
        : currentBlockIndex(0)
        , id(NotQueued) {}

    struct BlockInfo
    {
        BlockInfo()
            : name(0) {}
        BlockInfo(const char* _name)
            : name(_name) {}

        const char* name;
    };

    typedef StaticDynArray<BlockInfo, 32> BlockInfos;

    enum ID
    {
        NotQueued,
        Queued,
        FindingPath,
        ExecutingPlan
    };

    operator ID () const {
        return id;
    }

    BlockInfos blockInfos;
    uint32 currentBlockIndex;
    ID id;
};

#if defined(COMPILE_WITH_MOVEMENT_SYSTEM_DEBUG)
inline void ConstructHumanReadableText(IN const MovementRequestStatus& status, OUT stack_string& statusText)
{
    switch (status)
    {
    case MovementRequestStatus::Queued:
    {
        statusText = "Request In Queue";
        break;
    }

    case MovementRequestStatus::FindingPath:
    {
        statusText.Format("Finding Path");
        break;
    }

    case MovementRequestStatus::ExecutingPlan:
    {
        statusText = "Executing Plan: ";

        const size_t totalBlockInfos = status.blockInfos.size();
        for (size_t index = 0; index < totalBlockInfos; ++index)
        {
            if (index != 0)
            {
                statusText += " ";
            }

            const bool active = (index == status.currentBlockIndex);

            if (active)
            {
                statusText += "[";
            }

            statusText += status.blockInfos[index].name;

            if (active)
            {
                statusText += "]";
            }
        }

        break;
    }

    case MovementRequestStatus::NotQueued:
    {
        statusText = "Request Not Queued";
        break;
    }

    default:
    {
        statusText = "Unknown Status";
        break;
    }
    }
}
#endif // COMPILE_WITH_MOVEMENT_SYSTEM_DEBUG

#endif // CRYINCLUDE_CRYCOMMON_MOVEMENTREQUEST_H
