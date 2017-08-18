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

#ifndef CRYINCLUDE_CRYCOMMON_HIDESPOTQUERYCONTEXT_H
#define CRYINCLUDE_CRYCOMMON_HIDESPOTQUERYCONTEXT_H
#pragma once

// Contains information about what hide spots
// you want the AI system to return.
struct HidespotQueryContext
{
    HidespotQueryContext()
        : minRange(0.0f)
        , maxRange(0.0f)
        , hideFromPos(ZERO)
        , centerOfQuery(ZERO)
        , onlyThoseThatGiveCover(true)
        , onlyCollidable(false)
        , maxPoints(0)
        , pCoverPos(NULL)
        , pCoverObjPos(NULL)
        , pCoverObjDir(NULL)
    {
    }

    float minRange;
    float maxRange;
    Vec3 hideFromPos;
    Vec3 centerOfQuery;
    bool onlyThoseThatGiveCover;
    bool onlyCollidable;

    unsigned int maxPoints;
    Vec3* pCoverPos;
    Vec3* pCoverObjPos;
    Vec3* pCoverObjDir;
};

#endif // CRYINCLUDE_CRYCOMMON_HIDESPOTQUERYCONTEXT_H
