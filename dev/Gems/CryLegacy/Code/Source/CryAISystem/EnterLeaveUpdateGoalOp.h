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

#ifndef CRYINCLUDE_CRYAISYSTEM_ENTERLEAVEUPDATEGOALOP_H
#define CRYINCLUDE_CRYAISYSTEM_ENTERLEAVEUPDATEGOALOP_H
#pragma once

#include "GoalOp.h"

// Helper class which you can derive from when you want to get clear
// entry/exit/update points for your goal op.
//
// Preferably, this should be provided by the goal pipe system itself.
//
// Methods you may override:
// - Enter is called when the execution starts.
// - Leave is called when the execution stops.
// - Update is continuously called on non-dry updates until:
//   a) You explicitly say you've succeeded or failed with your operation.
//   b) A new goal pipe is selected and your goal op is pulled out.

class EnterLeaveUpdateGoalOp
    : public CGoalOp
{
public:
    EnterLeaveUpdateGoalOp()
        : m_initialized(false)
        , m_status(eGOR_IN_PROGRESS)
    {
    }

    // Called when the pipe user starts running this instance of the goal op.
    virtual void Enter(CPipeUser& pipeUser)
    {
    }

    // Called when the pipe user stops running this instance of the goal op.
    virtual void Leave(CPipeUser& pipeUser)
    {
    }

    // Don't override. Do your work in Update.
    virtual EGoalOpResult Execute(CPipeUser* pPipeUser)
    {
        assert(pPipeUser);

        if (!m_initialized)
        {
            Enter(*pPipeUser);
            m_initialized = true;
        }

        Update(*pPipeUser);

        return m_status;
    }

    // Don't override. Do your work in Update.
    virtual void ExecuteDry(CPipeUser* pPipeUser)
    {
    }

    // Don't override. Do your work in OnLeave.
    virtual void Reset(CPipeUser* pPipeUser)
    {
        if (m_initialized)
        {
            Leave(*pPipeUser);

            m_status = eGOR_IN_PROGRESS;
            m_initialized = false;
        }
    }

protected:
    // Override with your own Update code.
    virtual void Update(CPipeUser& pipeUser)
    {
    }

    void GoalOpSucceeded() { m_status = eGOR_SUCCEEDED; }
    void GoalOpFailed() { m_status = eGOR_FAILED; }
    EGoalOpResult GetStatus() { return m_status; }

private:
    EGoalOpResult m_status;
    bool m_initialized;
};

#endif // CRYINCLUDE_CRYAISYSTEM_ENTERLEAVEUPDATEGOALOP_H
