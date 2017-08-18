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

#ifndef CRYINCLUDE_CRYAISYSTEM_COVER_COVER_H
#define CRYINCLUDE_CRYAISYSTEM_COVER_COVER_H
#pragma once


#include <ICoverSystem.h>


enum ECoverUsageType
{
    eCU_Hide = 0,
    eCU_UnHide,
};


const float LowCoverMinHeight = 0.75f;
const float LowCoverMaxHeight = 1.1f;
const float HighCoverMaxHeight = 2.0f;
const float CoverOffsetUp = 0.085f;
const float CoverExtraClearance = 0.15f;
const float CoverSamplerLoopExtraOffset = 0.15f;

const Vec3 CoverSamplerBiasUp = Vec3(0.0f, 0.0f, 0.005f);
const Vec3 CoverUp = Vec3(0.0f, 0.0f, 1.0f);


const uint32 CoverCollisionEntities = ent_static | ent_terrain | ent_ignore_noncolliding;


struct CoverInterval
{
    CoverInterval()
        : left(0.0f)
        , right(0.0f)
    {
    }

    CoverInterval(float l, float r)
        : left(l)
        , right(r)
    {
    }

    float left;
    float right;

    inline bool Empty() const
    {
        return (right - left) <= 0.0001f;
    }

    inline float Width() const
    {
        return std::max<float>(0.0f, (right - left));
    }

    inline bool Intersect(const CoverInterval& other)
    {
        left = std::max<float>(left, other.left);
        right = std::min<float>(right, other.right);

        return !Empty();
    }

    inline CoverInterval Intersection(const CoverInterval& other) const
    {
        CoverInterval result = *this;
        result.Intersect(other);

        return result;
    }
};


#endif // CRYINCLUDE_CRYAISYSTEM_COVER_COVER_H
