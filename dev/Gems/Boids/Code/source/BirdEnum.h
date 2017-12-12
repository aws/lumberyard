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
#ifndef CRYINCLUDE_GAMEDLL_BOIDS_BIRDENUM_H
#define CRYINCLUDE_GAMEDLL_BOIDS_BIRDENUM_H
#pragma once

namespace Bird
{
    typedef enum
    {
        FLYING,
        TAKEOFF,
        LANDING,
        ON_GROUND,
        DEAD
    } EStatus;

    // sub-stati of EStatus::ON_GROUND
    typedef enum
    {
        OGS_WALKING,
        OGS_SLOWINGDOWN, // transitioning from walking to idle
        OGS_IDLE
    } EOnGroundStatus;

    enum
    {
        ANIM_FLY,
        ANIM_TAKEOFF,
        ANIM_WALK,
        ANIM_IDLE,
        ANIM_LANDING_DECELERATING, // we've been in the landing state for a while and are now decelerating to land softly on
                                   // the ground
    };
} // namespace

#endif // CRYINCLUDE_GAMEDLL_BOIDS_BIRDENUM_H
