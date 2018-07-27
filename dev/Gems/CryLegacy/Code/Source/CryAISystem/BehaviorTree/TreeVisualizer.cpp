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
#include "TreeVisualizer.h"

#include <IAIDebugRenderer.h>
#include <BehaviorTree/Node.h>
#include <BehaviorTree/TimestampCollection.h>

#ifdef USING_BEHAVIOR_TREE_VISUALIZER
namespace BehaviorTree
{
    const float nodePosX = 10.0f;
    const float nodePosY = 30.0f;
    const float behaviorLogPosX = 600.0f;
    const float behaviorLogPosY = 30.0f;
    const float timestampPosX = behaviorLogPosX;
    const float timestampPosY = 370.0f;
    const float blackboardPosX = 900.0f;
    const float blackboardPosY = 30.0f;

    const float eventLogPosX = 1250.0f;
    const float eventLogPosY = 370.0f;

    const float fontSize = 1.25f;
    const float lineHeight = 11.5f * fontSize;

    TreeVisualizer::TreeVisualizer(const UpdateContext& updateContext)
        : m_updateContext(updateContext)
        , m_currentLinePositionX(0.0f)
        , m_currentLinePositionY(0.0f)
    {
    }

    void TreeVisualizer::Draw(
        const DebugTree& tree,
        const char* behaviorTreeName,
        const char* agentName,
#ifdef USING_BEHAVIOR_TREE_LOG
        const MessageQueue& behaviorLog,
#endif // USING_BEHAVIOR_TREE_LOG
        const TimestampCollection& timestampCollection,
        const Blackboard& blackboard
#ifdef USING_BEHAVIOR_TREE_EVENT_DEBUGGING
        , const MessageQueue& eventsLog
#endif // USING_BEHAVIOR_TREE_EVENT_DEBUGGING
        )
    {
        // Nodes
        {
            const DebugNode* firstNode = tree.GetFirstNode().get();
            if (firstNode)
            {
                SetLinePosition(nodePosX, nodePosY);
                stack_string caption;
                caption.Format("  Modular Behavior Tree '%s' for agent '%s'", behaviorTreeName, agentName);
                DrawLine(caption, Col_Yellow);
                DrawLine("", Col_White);
                DrawNode(*firstNode, 0);
            }
        }

#ifdef USING_BEHAVIOR_TREE_LOG
        DrawBehaviorLog(behaviorLog);
#endif // USING_BEHAVIOR_TREE_LOG

#ifdef USING_BEHAVIOR_TREE_TIMESTAMP_DEBUGGING
        if (gAIEnv.CVars.DebugTimestamps)
        {
            DrawTimestampCollection(timestampCollection);
        }
#endif // USING_BEHAVIOR_TREE_TIMESTAMP_DEBUGGING

        DrawBlackboard(blackboard);

#ifdef USING_BEHAVIOR_TREE_EVENT_DEBUGGING
        DrawEventLog(eventsLog);
#endif // USING_BEHAVIOR_TREE_EVENT_DEBUGGING
    }

    void TreeVisualizer::DrawNode(const DebugNode& node, const uint32 depth)
    {
        // Construct a nice readable debug text for this node
        stack_string str;

        // Line
        const uint32 xmlLine = static_cast<const Node*>(node.node)->GetXmlLine();
        if (xmlLine > 0)
        {
            str.Format("%5d ", xmlLine);
        }
        else
        {
            str.Format("      ", xmlLine);
        }

        // Indention
        for (uint32 i = 0; i < (depth + 1); ++i)
        {
            if ((i % 2) == 1)
            {
                str += "- ";
            }
            else
            {
                str += "  ";
            }
        }

        // Node type
        const char* nodeType = static_cast<const Node*>(node.node)->GetCreator()->GetTypeName();
        str += nodeType;

        bool hasCustomText = false;

#ifdef USING_BEHAVIOR_TREE_NODE_CUSTOM_DEBUG_TEXT
        // Custom debug text from the node
        stack_string customDebugText;

        UpdateContext updateContext = m_updateContext;
        const Node* nodeToDraw = static_cast<const Node*>(node.node);
        const RuntimeDataID runtimeDataID = MakeRuntimeDataID(updateContext.entityId, nodeToDraw->m_id);
        updateContext.runtimeData = nodeToDraw->GetCreator()->GetRuntimeData(runtimeDataID);

        nodeToDraw->GetCustomDebugText(updateContext, customDebugText);
        if (!customDebugText.empty())
        {
            str += " - " + customDebugText;
            hasCustomText = true;
        }
#endif // USING_BEHAVIOR_TREE_NODE_CUSTOM_DEBUG_TEXT

        // Draw
        ColorB color;
        const bool leaf = node.children.empty();
        if (leaf)
        {
            color = Col_SlateBlue;
        }
        else if (hasCustomText)
        {
            color = Col_White;
        }
        else
        {
            color = Col_DarkSlateGrey;
        }

        DrawLine(str.c_str(), color);

        // Recursively draw children
        DebugNode::Children::const_iterator it = node.children.begin();
        DebugNode::Children::const_iterator end = node.children.end();
        for (; it != end; ++it)
        {
            DrawNode(*(*it), depth + 1);
        }
    }

    void TreeVisualizer::DrawLine(const char* label, const ColorB& color)
    {
        gEnv->pAISystem->GetAIDebugRenderer()->Draw2dLabel(m_currentLinePositionX, m_currentLinePositionY, fontSize, color, false, label);
        m_currentLinePositionY += lineHeight;
    }

#ifdef USING_BEHAVIOR_TREE_LOG
    void TreeVisualizer::DrawBehaviorLog(const MessageQueue& behaviorLog)
    {
        SetLinePosition(behaviorLogPosX, behaviorLogPosY);
        DrawLine("Behavior Log", Col_Yellow);
        DrawLine("", Col_White);

        PersonalLog::Messages::const_iterator it = behaviorLog.GetMessages().begin();
        PersonalLog::Messages::const_iterator end = behaviorLog.GetMessages().end();
        for (; it != end; ++it)
        {
            DrawLine(*it, Col_White);
        }
    }
#endif // USING_BEHAVIOR_TREE_LOG

#ifdef USING_BEHAVIOR_TREE_TIMESTAMP_DEBUGGING
    void TreeVisualizer::DrawTimestampCollection(const TimestampCollection& timestampCollection)
    {
        SetLinePosition(timestampPosX, timestampPosY);
        DrawLine("Timestamp Collection", Col_Yellow);
        DrawLine("", Col_White);

        CTimeValue timeNow = gEnv->pTimer->GetFrameStartTime();
        Timestamps::const_iterator it = timestampCollection.GetTimestamps().begin();
        Timestamps::const_iterator end = timestampCollection.GetTimestamps().end();
        for (; it != end; ++it)
        {
            stack_string s;
            ColorB color;
            if (it->IsValid())
            {
                s.Format("%s [%.2f]", it->id.timestampName, (timeNow - it->time).GetSeconds());
                color = Col_ForestGreen;
            }
            else
            {
                s.Format("%s [--]", it->id.timestampName);
                color = Col_Gray;
            }
            DrawLine(s.c_str(), color);
        }
    }
#endif // USING_BEHAVIOR_TREE_TIMESTAMP_DEBUGGING

    void TreeVisualizer::SetLinePosition(float x, float y)
    {
        m_currentLinePositionX = x;
        m_currentLinePositionY = y;
    }

    void TreeVisualizer::DrawBlackboard(const Blackboard& blackboard)
    {
#ifdef STORE_BLACKBOARD_VARIABLE_NAMES
        SetLinePosition(blackboardPosX, blackboardPosY);

        DrawLine("Blackboard variables", Col_Yellow);
        DrawLine("", Col_White);

        Blackboard::BlackboardVariableMap blackboardVariableMap = blackboard.GetBlackboardVariableMap();

        Blackboard::BlackboardVariableMap::const_iterator it = blackboardVariableMap.begin();
        Blackboard::BlackboardVariableMap::const_iterator end = blackboardVariableMap.end();
        for (; it != end; ++it)
        {
            BlackboardVariableId id = it->first;
            IBlackboardVariablePtr variable = it->second;

            Serialization::TypeID typeId = variable->GetDataTypeId();
            if (typeId == Serialization::TypeID::get<Vec3>())
            {
                stack_string variableText;

                Vec3 data;
                variable->GetData(data);
                variableText.Format("%s - (%f, %f, %f)", id.name.c_str(), data.x, data.y, data.z);

                DrawLine(variableText.c_str(), Col_White);
            }
        }
#endif
    }

#ifdef USING_BEHAVIOR_TREE_EVENT_DEBUGGING
    void TreeVisualizer::DrawEventLog(const MessageQueue& eventsLog)
    {
        SetLinePosition(eventLogPosX, eventLogPosY);
        DrawLine("Event Log", Col_Yellow);
        DrawLine("", Col_White);

        PersonalLog::Messages::const_reverse_iterator it = eventsLog.GetMessages().rbegin();
        PersonalLog::Messages::const_reverse_iterator end = eventsLog.GetMessages().rend();
        for (; it != end; ++it)
        {
            DrawLine(*it, Col_White);
        }
    }
#endif // USING_BEHAVIOR_TREE_EVENT_DEBUGGING
}

#endif // USING_BEHAVIOR_TREE_VISUALIZER
