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
#include "ExecutionStackFileLogger.h"
#include <BehaviorTree/Node.h>
#include "TypeInfo_impl.h"  // ARRAY_COUNT macro


#ifdef USING_BEHAVIOR_TREE_EXECUTION_STACKS_FILE_LOG
namespace BehaviorTree
{
    static bool IsCharAllowedInFileNames(const char ch)
    {
        if (ch >= 'a' && ch <= 'z')
        {
            return true;
        }

        if (ch >= 'A' && ch <= 'Z')
        {
            return true;
        }

        if (ch >= '0' && ch <= '9')
        {
            return true;
        }

        if (ch == '_' || ch == '-' || ch == '.')
        {
            return true;
        }

        return false;
    }

    static void SanitizeFileName(stack_string& fileName)
    {
        for (stack_string::iterator it = fileName.begin(); it != fileName.end(); ++it)
        {
            if (!IsCharAllowedInFileNames(*it))
            {
                *it = '_';
            }
        }
    }

    ExecutionStackFileLogger::ExecutionStackFileLogger(const EntityId entityId)
    {
        if (const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId))
        {
            m_agentName = pEntity->GetName();
        }

        stack_string fileName;
        fileName.Format("MBT_%s_%i.txt", m_agentName.c_str(), entityId);
        SanitizeFileName(fileName);

        stack_string filePath("@log@/MBT_Logs/");
        filePath.append(fileName);

        m_logFilePath[0] = '\0';
        if (gEnv->pCryPak->AdjustFileName(filePath.c_str(), m_logFilePath, AZ_ARRAY_SIZE(m_logFilePath), ICryPak::FLAGS_FOR_WRITING))
        {
            m_openState = NotYetAttemptedToOpenForWriteAccess;
        }
        else
        {
            m_openState = CouldNotAdjustFileName;
        }
    }

    ExecutionStackFileLogger::~ExecutionStackFileLogger()
    {
        m_logFile.Close();
    }

    void ExecutionStackFileLogger::LogDebugTree(const DebugTree& debugTree, const UpdateContext& updateContext, const BehaviorTreeInstance& instance)
    {
        if (m_openState == NotYetAttemptedToOpenForWriteAccess)
        {
            m_openState = m_logFile.Open(m_logFilePath, "w") ? OpenForWriteAccessSucceeded : OpenForWriteAccessFailed;

            // successfully opened? -> write header
            if (m_openState == OpenForWriteAccessSucceeded)
            {
                stack_string header;
                char startTime[1024] = "???";
                time_t ltime;
                if (time(&ltime) != (time_t)-1)
                {
                    tm _tm;
                    if (localtime_s(&_tm, &ltime) != 0)
                    {
                        strftime(startTime, ARRAY_COUNT(startTime), "%Y-%m-%d %H:%M:%S", &_tm);
                    }
                }
                header.Format("Modular Behavior Tree '%s' for agent '%s' with EntityId = %i (logging started at %s)\n\n", instance.behaviorTreeTemplate->mbtFilename.c_str(), m_agentName.c_str(), updateContext.entityId, startTime);
                m_logFile.Write(header.c_str(), header.length() + 1);
                m_logFile.Flush();
            }
        }

        if (m_openState == OpenForWriteAccessSucceeded)
        {
            if (const DebugNode* pRoot = debugTree.GetFirstNode().get())
            {
                LogNodeRecursively(*pRoot, updateContext, instance, 0);
                stack_string textLine("-----------------------------------------\n");
                m_logFile.Write(textLine.c_str(), textLine.length() + 1);
            }
        }
    }

    void ExecutionStackFileLogger::LogNodeRecursively(const DebugNode& debugNode, const UpdateContext& updateContext, const BehaviorTreeInstance& instance, const int indentLevel)
    {
        const Node* pNode = static_cast<const Node*>(debugNode.node);
        const uint32 lineNum = pNode->GetXmlLine();
        const char* nodeType = pNode->GetCreator()->GetTypeName();
        stack_string customText;
        const RuntimeDataID runtimeDataID = MakeRuntimeDataID(updateContext.entityId, pNode->GetNodeID());
        UpdateContext updateContextWithValidRuntimeData = updateContext;
        updateContextWithValidRuntimeData.runtimeData = pNode->GetCreator()->GetRuntimeData(runtimeDataID);
        pNode->GetCustomDebugText(updateContextWithValidRuntimeData, customText);
        if (!customText.empty())
        {
            customText.insert(0, " - ");
        }

        stack_string textLine;
        textLine.Format("%5i %*s %s%s\n", lineNum, indentLevel * 2, "", nodeType, customText.c_str());
        m_logFile.Write(textLine.c_str(), textLine.length() + 1);

        for (DebugNode::Children::const_iterator it = debugNode.children.begin(), end = debugNode.children.end(); it != end; ++it)
        {
            LogNodeRecursively(*(*it), updateContext, instance, indentLevel + 1);
        }
    }
}
#endif  // USING_BEHAVIOR_TREE_EXECUTION_STACKS_FILE_LOG
