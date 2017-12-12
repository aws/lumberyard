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

#ifndef CRYINCLUDE_CRYAISYSTEM_COVER_COVERSCORER_H
#define CRYINCLUDE_CRYAISYSTEM_COVER_COVERSCORER_H
#pragma once


#include "Cover.h"


struct ICoverLocationScorer
{
    virtual ~ICoverLocationScorer(){}
    struct Params
    {
        Vec3 location;
        Vec3 direction;
        float height;

        Vec3 userLocation;
        float totalLength;

        ECoverUsageType usage;

        Vec3 target;
    };

    virtual float Score(const Params& params) const = 0;
};


struct DefaultCoverScorer
    : public ICoverLocationScorer
{
public:
    float ScoreByDistance(const ICoverLocationScorer::Params& params) const;
    float ScoreByDistanceToTarget(const ICoverLocationScorer::Params& params) const;
    float ScoreByAngle(const ICoverLocationScorer::Params& params) const;
    float ScoreByCoverage(const ICoverLocationScorer::Params& params) const;

    virtual float Score(const ICoverLocationScorer::Params& params) const;
};


#endif // CRYINCLUDE_CRYAISYSTEM_COVER_COVERSCORER_H
