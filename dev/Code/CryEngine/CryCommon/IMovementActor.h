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

#ifndef __IMOVEMENTACTOR_H__
#define __IMOVEMENTACTOR_H__

#include <IMovementSystem.h>

struct IMovementActorAdapter;

struct IMovementActor
{
    virtual ~IMovementActor() {}

    virtual IMovementActorAdapter& GetAdapter() const = 0;
    virtual void RequestPathTo(const Vec3& destination, float lengthToTrimFromThePathEnd, const MNMDangersFlags dangersFlags = eMNMDangers_None,
        const bool considerActorsAsPathObstacles = false) = 0;
    virtual Movement::PathfinderState GetPathfinderState() const = 0;
    virtual const char* GetName() const = 0;
    virtual void Log(const char* message) = 0;

    virtual bool IsLastPointInPathASmartObject() const = 0;
    virtual EntityId GetEntityId() const = 0;
    virtual MovementActorCallbacks GetCallbacks() const = 0;
};

#endif // __IMOVEMENTACTOR_H__