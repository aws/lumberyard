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

#ifndef ExecutionStackFileLogger_h
#define ExecutionStackFileLogger_h

#pragma once

#include <BehaviorTree/IBehaviorTree.h>

#ifdef USING_BEHAVIOR_TREE_EXECUTION_STACKS_FILE_LOG
namespace BehaviorTree
{
    class ExecutionStackFileLogger
    {
    public:
        explicit ExecutionStackFileLogger(const EntityId entityId);
        ~ExecutionStackFileLogger();
        void LogDebugTree(const DebugTree& debugTree, const UpdateContext& updateContext, const BehaviorTreeInstance& instance);

    private:
        enum LogFileOpenState
        {
            CouldNotAdjustFileName,
            NotYetAttemptedToOpenForWriteAccess,
            OpenForWriteAccessFailed,
            OpenForWriteAccessSucceeded
        };

        ExecutionStackFileLogger(const ExecutionStackFileLogger&);
        ExecutionStackFileLogger& operator=(const ExecutionStackFileLogger&);

        void LogNodeRecursively(const DebugNode& debugNode, const UpdateContext& updateContext, const BehaviorTreeInstance& instance, const int indentLevel);

        string m_agentName;
        char m_logFilePath[ICryPak::g_nMaxPath];
        LogFileOpenState m_openState;
        CCryFile m_logFile;
    };
}
#endif  // USING_BEHAVIOR_TREE_EXECUTION_STACKS_FILE_LOG

#endif  // ExecutionStackFileLogger_h
