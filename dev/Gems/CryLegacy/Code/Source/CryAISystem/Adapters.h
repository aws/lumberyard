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

// Description : Implements adapters for AI objects from external interfaces to internal
//               This is purely a translation layer without concrete instances
//               They can have no state and must remain abstract


#ifndef CRYINCLUDE_CRYAISYSTEM_ADAPTERS_H
#define CRYINCLUDE_CRYAISYSTEM_ADAPTERS_H
#pragma once

#include <IAgent.h>
#include "AIObject.h"
#include "ObjectContainer.h"

#include <IAIGroup.h>

CWeakRef<CAIObject> GetWeakRefSafe(IAIObject* pObj);


class CPipeUserAdapter
    : public IPipeUser
{
public:
    virtual ~CPipeUserAdapter() {}

    virtual bool SelectPipe(int id, const char* name, CWeakRef<CAIObject> refArgument, int goalPipeId = 0, bool resetAlways = false, const GoalParams* node = 0) = 0;
    virtual IGoalPipe* InsertSubPipe(int id, const char* name, CWeakRef<CAIObject> refArgument = NILREF, int goalPipeId = 0, const GoalParams* node = 0) = 0;

private:
    bool SelectPipe(int id, const char* name, IAIObject* pArgument = 0, int goalPipeId = 0, bool resetAlways = false, const GoalParams* node = 0)
    { return SelectPipe(id, name, GetWeakRefSafe(pArgument), goalPipeId, resetAlways, node); }
    IGoalPipe* InsertSubPipe(int id, const char* name, IAIObject* pArgument = 0, int goalPipeId = 0, const GoalParams* node = 0)
    { return InsertSubPipe(id, name, GetWeakRefSafe(pArgument), goalPipeId, node); }
};

class CAIGroupAdapter
    : public IAIGroup
{
public:
    virtual CWeakRef<CAIObject> GetAttentionTarget(bool bHostileOnly = false, bool bLiveOnly = false, const CWeakRef<CAIObject> refSkipTarget = NILREF) const = 0;

private:
    IAIObject*  GetAttentionTarget(bool bHostileOnly = false, bool bLiveOnly = false) const;
};

#endif // CRYINCLUDE_CRYAISYSTEM_ADAPTERS_H
