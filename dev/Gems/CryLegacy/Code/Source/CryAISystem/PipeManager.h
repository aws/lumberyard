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

#ifndef CRYINCLUDE_CRYAISYSTEM_PIPEMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_PIPEMANAGER_H
#pragma once

#include <map>
#include "IAgent.h"

class CGoalPipe;
class CAISystem;
typedef std::map<string, CGoalPipe*> GoalMap;


class CPipeManager
{
    friend class CAISystem;
public:
    enum ActionToTakeWhenDuplicateFound
    {
        SilentlyReplaceDuplicate,
        ReplaceDuplicateAndReportError,
    };

    CPipeManager(void);
    ~CPipeManager(void);

    void    ClearAllGoalPipes();
    IGoalPipe*  CreateGoalPipe(const char* pName,
        const ActionToTakeWhenDuplicateFound actionToTakeWhenDuplicateFound);
    IGoalPipe*  OpenGoalPipe(const char* pName);
    CGoalPipe*  IsGoalPipe(const char* pName);
    void        Serialize(TSerialize ser);

    /// For debug. Checks the script files for created and used goalpipes and
    /// dumps the unused goalpipes into console. Does not actually use the loaded pipes.
    void CheckGoalpipes();

private:

    // keeps all possible goal pipes that the agents can use
    GoalMap m_mapGoals;
    bool        m_bDynamicGoalPipes;    // to indicate if goalpipe is created after AISystem initialization, loading of \aiconfig.lua
};


#endif // CRYINCLUDE_CRYAISYSTEM_PIPEMANAGER_H
