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

// Description : Interface to apply modification to an AI's handling of the
//               Rate of Death balancing system

#ifndef CRYINCLUDE_CRYCOMMON_IAIRATEOFDEATHHANDLER_H
#define CRYINCLUDE_CRYCOMMON_IAIRATEOFDEATHHANDLER_H
#pragma once


#include <IAgent.h> // <> required for Interfuscator

struct IAIRateOfDeathHandler
{
    // <interfuscator:shuffle>
    virtual ~IAIRateOfDeathHandler() {}

    // Calculate and return how long the target should stay alive from now
    virtual float GetTargetAliveTime(const IAIObject* pAI, const IAIObject* pTarget, EAITargetZone eTargetZone, float& fFireDazzleTime) = 0;

    // Calculate and return how long the AI should take to react to firing at the target from now
    virtual float GetFiringReactionTime(const IAIObject* pAI, const IAIObject* pTarget, const Vec3& vTargetPos) = 0;

    // Calculate and return the zone the target is currently in
    virtual EAITargetZone GetTargetZone(const IAIObject* pAI, const IAIObject* pTarget) = 0;
    // </interfuscator:shuffle>

    void GetMemoryUsage(ICrySizer* pSizer) const{ /*LATER*/}
};

#endif // CRYINCLUDE_CRYCOMMON_IAIRATEOFDEATHHANDLER_H
