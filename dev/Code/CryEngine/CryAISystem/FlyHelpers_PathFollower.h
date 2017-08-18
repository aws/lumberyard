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

#ifndef CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATHFOLLOWER_H
#define CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATHFOLLOWER_H
#pragma once

#include "FlyHelpers_Path.h"
#include "FlyHelpers_PathLocation.h"

namespace FlyHelpers
{
    struct PathEntityIn
    {
        Vec3 position;
        Vec3 forward;
        Vec3 velocity;

        PathEntityIn()
            : position(0, 0, 0)
            , forward(0, 1, 0)
            , velocity(0, 0, 0)
        {
        }
    };

    struct PathEntityOut
    {
        Vec3 moveDirection;
        Vec3 lookPosition;
        Vec3 lookDirection;
        float speed;
        float distanceToPathEnd;

        PathEntityOut()
            : moveDirection(0)
            , lookPosition(0, 0, 0)
            , lookDirection(0, 0, 0)
            , speed(0)
            , distanceToPathEnd(0)
        {
        }
    };

    struct PathFollowerParams
    {
        float desiredSpeed;
        float pathRadius;
        float lookAheadDistance;
        float decelerateDistance;
        float maxStartDistanceAlongNonLoopingPath;
        bool loopAlongPath;
        bool startPathFromClosestLocation;

        PathFollowerParams()
            : desiredSpeed(15.0f)
            , pathRadius(1.0f)
            , lookAheadDistance(3.0f)
            , decelerateDistance(10.0f)
            , maxStartDistanceAlongNonLoopingPath(30.0f)
            , loopAlongPath(false)
            , startPathFromClosestLocation(false)
        {
        }
    };


    class PathFollower
    {
    public:
        PathFollower();
        ~PathFollower();

        void Init(const SShape& path, const PathFollowerParams& params, const PathEntityIn& pathEntity);
        PathEntityOut Update(const PathEntityIn& pathEntity, const float elapsedSeconds);

        void SetFinalPathLocation(const Vec3& position);
        void ResetFinalPathLocation();

        void SetDesiredSpeed(const float desiredSpeed);

        void Draw();

        const Path& GetCurrentPath() const
        {
            CRY_ASSERT(m_pCurrentPath);
            return *m_pCurrentPath;
        }

    private:
        void SetPath(const SShape& path, const bool loopAlongPath);

        Vec3 CalculatePathPosition(const PathLocation& pathLocation) const;
        PathLocation CalculateNextPathLocation(const PathLocation& pathLocation, const float advance) const;

        void SetUseReversePath(const bool useReversePath);
        PathLocation GetNonReversedTargetLocation() const;

    private:
        Path m_path;
        Path m_reversePath;

        Path* m_pCurrentPath;

        PathLocation m_targetLocation;
        PathLocation m_lookAtLocation;

        PathLocation m_finalLocation;
        bool m_stopAtFinalLocation;

        Vec3 m_targetPosition;
        Vec3 m_lookAtPosition;
        Vec3 m_finalPosition;

        PathFollowerParams m_params; // TODO: Don't store, pass it as parameter to update / functions that need it.
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_PATHFOLLOWER_H
