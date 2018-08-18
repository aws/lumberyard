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

#pragma once

#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/Workspace.h>

namespace EMotionFX
{
    class MorphSetup;
    class MotionSet;
}

namespace EMStudio
{
    class MetricsEventSender
    {
    public:
        static void SendStartRecorderEvent(const EMotionFX::Recorder& recorder);
        static void SendStopRecorderEvent(const EMotionFX::Recorder& recorder);

        static void SendCreateNodeEvent(const AZ::TypeId& typeId);
        static void SendCreateConnectionEvent(bool isTransition);

        static void SendDeleteNodesAndConnectionsEvent(AZ::u32 numNodes, AZ::u32 numConnections);

        static void SendPasteConditionsEvent(AZ::u32 numConditions);
        static void SendPasteNodesAndConnectionsEvent(AZ::u32 numNodes, AZ::u32 numConnections);

        static void SendMorphTargetUseEvent(AZ::u32 numMotions, AZ::u32 numSkeletalMotionsWithMorph);
        static void SendMorphTargetConcurrentPlayEvent(const EMotionFX::MotionSet* motionSet);
        static void SendActorMorphTargetSizesEvent(const EMotionFX::MorphSetup * morphSetup);

    private:
        // Not used yet.
        static void SendSaveActorEvent(const EMotionFX::Actor* actor);
        static void SendSaveMotionEvent(const EMotionFX::Motion* motion);
        static void SendSaveMotionSetEvent(const EMotionFX::MotionSet* motionSet);
        static void SendSaveAnimGraphEvent(const EMotionFX::AnimGraph* animGraph);
        static void SendSaveWorkspaceEvent(const EMStudio::Workspace* workspace);

        static void SendSwitchLayoutEvent(const char* layoutName);

        static void CalcMotionSetStatistics(const EMotionFX::MotionSet* motionSet, AZ::u32& outNumTotalMotionEntries, AZ::u32& outMaxHierarchyDepth, const AZ::u32 hierarchyLevel = 0);
    };
} // namespace EMStudio