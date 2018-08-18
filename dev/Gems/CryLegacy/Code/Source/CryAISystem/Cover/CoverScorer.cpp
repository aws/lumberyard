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

#include "CryLegacy_precompiled.h"
#include "CoverScorer.h"


float DefaultCoverScorer::ScoreByDistance(const ICoverLocationScorer::Params& params) const
{
    float distance = (params.location - params.userLocation).len();
    float maxDistance = params.totalLength;
    if (maxDistance < distance)
    {
        maxDistance = distance;
    }

    assert(distance <= maxDistance);

    float score = 1.0f;
    if (maxDistance >= 0.1f)
    {
        score = 1.0f - min(1.0f, distance / maxDistance);
    }

    return score;
}


float DefaultCoverScorer::ScoreByDistanceToTarget(const ICoverLocationScorer::Params& params) const
{
    float locationToTarget = (params.location - params.target).len();

    const float ConsiderClose = 10.0f;
    const float ConsiderFar = 25.0f;

    bool isClose = (locationToTarget < ConsiderClose);
    bool isFar = (locationToTarget > ConsiderFar);

    float score = 1.0f;
    if (isClose)
    {
        score = (locationToTarget / ConsiderClose);
    }
    else if (isFar)
    {
        score = 1.0f - max(1.0f, (locationToTarget - ConsiderFar) / ConsiderFar);
    }

    return score;
}


float DefaultCoverScorer::ScoreByAngle(const ICoverLocationScorer::Params& params) const
{
    Vec3 locationToTargetDir = (params.target - params.location).GetNormalized();
    float angleCos = locationToTargetDir.Dot(params.direction);
    if (angleCos <= 0.1f)
    {
        return 0.0f;
    }

    return angleCos;
}


float DefaultCoverScorer::Score(const ICoverLocationScorer::Params& params) const
{
    float score = 0.0f;

    switch (params.usage)
    {
    case eCU_Hide:
    {
        score += 0.2f * ScoreByDistance(params);
        score += 0.3f * ScoreByAngle(params);
        score += 0.5f * ScoreByDistanceToTarget(params);
    }
    break;
    case eCU_UnHide:
    {
        score += 0.5f * ScoreByDistance(params);
        score += 0.0f * ScoreByAngle(params);
        score += 0.5f * ScoreByDistanceToTarget(params);
    }
    default:
        assert(0);
        break;
    }

    return score;
}