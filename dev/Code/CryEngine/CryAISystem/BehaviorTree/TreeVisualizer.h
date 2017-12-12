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

#ifndef CRYINCLUDE_CRYAISYSTEM_BEHAVIORTREE_TREEVISUALIZER_H
#define CRYINCLUDE_CRYAISYSTEM_BEHAVIORTREE_TREEVISUALIZER_H
#pragma once

#include <BehaviorTree/IBehaviorTree.h>

#ifdef USING_BEHAVIOR_TREE_VISUALIZER
namespace BehaviorTree
{
    class TreeVisualizer
    {
    public:
        TreeVisualizer(const UpdateContext& updateContext);

        void Draw(
            const DebugTree& tree
            , const char* behaviorTreeName
            , const char* agentName
#ifdef USING_BEHAVIOR_TREE_LOG
            , const MessageQueue& behaviorLog
#endif // USING_BEHAVIOR_TREE_LOG
            , const TimestampCollection& timestampCollection
            , const Blackboard& blackboard
#ifdef USING_BEHAVIOR_TREE_EVENT_DEBUGGING
            , const MessageQueue& eventsLog
#endif // USING_BEHAVIOR_TREE_EVENT_DEBUGGING
            );

    private:
        void SetLinePosition(float x, float y);
        void DrawLine(const char* label, const ColorB& color);

        void DrawNode(const DebugNode& node, const uint32 depth);

#ifdef USING_BEHAVIOR_TREE_LOG
        void DrawBehaviorLog(const MessageQueue& behaviorLog);
#endif // USING_BEHAVIOR_TREE_LOG

#ifdef USING_BEHAVIOR_TREE_TIMESTAMP_DEBUGGING
        void DrawTimestampCollection(const TimestampCollection& timestampCollection);
#endif // USING_BEHAVIOR_TREE_TIMESTAMP_DEBUGGING

        void DrawBlackboard(const Blackboard& blackboard);

#ifdef USING_BEHAVIOR_TREE_EVENT_DEBUGGING
        void DrawEventLog(const MessageQueue& eventsLog);
#endif // USING_BEHAVIOR_TREE_EVENT_DEBUGGING

        const UpdateContext& m_updateContext;
        float m_currentLinePositionX;
        float m_currentLinePositionY;
    };
}
#endif // USING_BEHAVIOR_TREE_VISUALIZER

#endif // CRYINCLUDE_CRYAISYSTEM_BEHAVIORTREE_TREEVISUALIZER_H
