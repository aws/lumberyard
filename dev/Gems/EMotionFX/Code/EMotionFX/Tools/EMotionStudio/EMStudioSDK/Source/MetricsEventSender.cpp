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

#include "MetricsEventSender.h"
#include <LyMetricsProducer/LyMetricsAPI.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/KeyTrackLinear.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSubMotion.h>
#include <EMotionFX/Source/MorphTarget.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>


namespace EMStudio
{
    void MetricsEventSender::SendSaveActorEvent(const EMotionFX::Actor* actor)
    {
        if (!actor)
        {
            return;
        }

        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("SaveActor");

        // NumLODs
        const uint32 numLODs = actor->GetNumLODLevels();
        AZStd::to_string(tempString, numLODs);
        LyMetrics_AddAttribute(eventId, "NumLODs", tempString.c_str());

        // NumNodes
        EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        AZStd::to_string(tempString, numNodes);
        LyMetrics_AddAttribute(eventId, "NumNodes", tempString.c_str());

        const uint32 lodLevel   = 0;
        uint32 numBones         = 0;
        uint32 numTotalPolygons = 0;
        uint32 numTotalVertices = 0;
        uint32 numTotalIndices  = 0;
        uint32 numMorphTargets  = 0;
        uint32 numPhonemes      = 0;
        if (numLODs > lodLevel)
        {
            const EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);
            if (morphSetup)
            {
                numMorphTargets = morphSetup->GetNumMorphTargets();
                for (uint32 i = 0; i < numMorphTargets; ++i)
                {
                    const EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);

                    if (morphTarget->GetIsPhoneme())
                    {
                        numPhonemes++;
                    }
                }
            }

            actor->CalcMeshTotals(lodLevel, &numTotalPolygons, &numTotalVertices, &numTotalIndices);

            MCore::Array<uint32> boneList;
            actor->ExtractBoneList(lodLevel, &boneList);
            numBones = boneList.GetLength();
        }

        // NumBones
        AZStd::to_string(tempString, numBones);
        LyMetrics_AddAttribute(eventId, "NumBones", tempString.c_str());

        uint32 numMeshes                = 0;
        uint32 numTotalSubMeshes        = 0;
        uint32 numTotalOriginalVertices = 0;
        uint32 maxNumSkinInfluences     = 0;
        for (uint32 i = 0; i < numNodes; i++)
        {
            const EMotionFX::Node* node = skeleton->GetNode(i);
            const uint32 nodeIndex = node->GetNodeIndex();

            if (actor->GetHasMesh(lodLevel, nodeIndex))
            {
                const EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                numMeshes++;

                const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                numTotalSubMeshes += numSubMeshes;

                const uint32 numOriginalVertices = mesh->GetNumOrgVertices();
                numTotalOriginalVertices += numOriginalVertices;

                maxNumSkinInfluences = MCore::Max<uint32>(maxNumSkinInfluences, mesh->CalcMaxNumInfluences());
            }
        }

        // NumMeshes
        AZStd::to_string(tempString, numMeshes);
        LyMetrics_AddAttribute(eventId, "NumMeshes", tempString.c_str());

        // NumTotalSubMeshes
        AZStd::to_string(tempString, numTotalSubMeshes);
        LyMetrics_AddAttribute(eventId, "NumTotalSubMeshes", tempString.c_str());

        // NumTotalPolygons
        AZStd::to_string(tempString, numTotalPolygons);
        LyMetrics_AddAttribute(eventId, "NumTotalPolygons", tempString.c_str());

        // NumTotalOriginalVertices
        AZStd::to_string(tempString, numTotalOriginalVertices);
        LyMetrics_AddAttribute(eventId, "NumTotalOriginalVertices", tempString.c_str());

        // NumTotalVertices
        AZStd::to_string(tempString, numTotalVertices);
        LyMetrics_AddAttribute(eventId, "NumTotalVertices", tempString.c_str());

        // MaxNumSkinInfluences
        AZStd::to_string(tempString, maxNumSkinInfluences);
        LyMetrics_AddAttribute(eventId, "MaxNumSkinInfluences", tempString.c_str());

        // NumMorphTargets
        AZStd::to_string(tempString, numMorphTargets);
        LyMetrics_AddAttribute(eventId, "NumMorphTargets", tempString.c_str());

        // NumPhonemes
        AZStd::to_string(tempString, numPhonemes);
        LyMetrics_AddAttribute(eventId, "NumPhonemes", tempString.c_str());

        // NumNodeGroups
        const uint32 numNodeGroups  = actor->GetNumNodeGroups();
        AZStd::to_string(tempString, numNodeGroups);
        LyMetrics_AddAttribute(eventId, "NumNodeGroups", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendSaveMotionEvent(const EMotionFX::Motion* motion)
    {
        if (!motion)
        {
            return;
        }

        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("SaveMotion");

        // LengthInSeconds
        const float lengthInSecs = motion->GetMaxTime();
        AZStd::to_string(tempString, lengthInSecs);
        LyMetrics_AddAttribute(eventId, "LengthInSeconds", tempString.c_str());

        uint32 numSubMotions = 0;
        uint32 numAnimatedSubMotions = 0;
        uint32 numTotalKeyframes = 0;
        if (motion->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
        {
            const EMotionFX::SkeletalMotion* skeletalMotion = static_cast<const EMotionFX::SkeletalMotion*>(motion);
            numSubMotions += skeletalMotion->GetNumSubMotions();

            for (uint32 i = 0; i < numSubMotions; ++i)
            {
                const EMotionFX::SkeletalSubMotion* subMotion = skeletalMotion->GetSubMotion(i);

                auto* posKeytrack = subMotion->GetPosTrack();
                EMotionFX::KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* rotKeytrack = subMotion->GetRotTrack();
                auto* scaleKeytrack = subMotion->GetScaleTrack();

                if (posKeytrack || rotKeytrack || scaleKeytrack)
                {
                    numAnimatedSubMotions++;
                }
                else
                {
                    continue;
                }

                if (posKeytrack)
                {
                    numTotalKeyframes += posKeytrack->GetNumKeys();
                }

                if (rotKeytrack)
                {
                    numTotalKeyframes += rotKeytrack->GetNumKeys();
                }

                if (scaleKeytrack)
                {
                    numTotalKeyframes += scaleKeytrack->GetNumKeys();
                }
            }
        }

        // NumSubMotions
        AZStd::to_string(tempString, numSubMotions);
        LyMetrics_AddAttribute(eventId, "NumSubMotions", tempString.c_str());

        // NumAnimatedSubMotions
        AZStd::to_string(tempString, numAnimatedSubMotions);
        LyMetrics_AddAttribute(eventId, "NumAnimatedSubMotions", tempString.c_str());

        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        uint32 numEventTracks = 0;
        uint32 numTotalEvents = 0;
        if (eventTable)
        {
            numEventTracks = eventTable->GetNumTracks();
            for (uint32 i = 0; i < numEventTracks; ++i)
            {
                const EMotionFX::MotionEventTrack* eventTrack = eventTable->GetTrack(i);

                numTotalEvents += eventTrack->GetNumEvents();
            }
        }

        // NumEventTracks
        AZStd::to_string(tempString, numEventTracks);
        LyMetrics_AddAttribute(eventId, "NumEventTracks", tempString.c_str());

        // NumTotalEvents
        AZStd::to_string(tempString, numTotalEvents);
        LyMetrics_AddAttribute(eventId, "NumTotalEvents", tempString.c_str());

        // NumTotalKeyframes
        AZStd::to_string(tempString, numTotalKeyframes);
        LyMetrics_AddAttribute(eventId, "NumTotalKeyframes", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::CalcMotionSetStatistics(const EMotionFX::MotionSet* motionSet, AZ::u32& outNumTotalMotionEntries, AZ::u32& outMaxHierarchyDepth, const AZ::u32 hierarchyLevel)
    {
        outNumTotalMotionEntries += static_cast<AZ::u32>(motionSet->GetNumMotionEntries());

        const uint32 numChildSets = motionSet->GetNumChildSets();
        for (uint32 i = 0; i < numChildSets; ++i)
        {
            const EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);

            AZ::u32 numChildMotionEntries = 0;
            AZ::u32 childHierarchyDepth = 0;
            CalcMotionSetStatistics(childSet, numChildMotionEntries, childHierarchyDepth, hierarchyLevel + 1);
        }

        outMaxHierarchyDepth = MCore::Max<AZ::u32>(hierarchyLevel, outMaxHierarchyDepth);
    }


    void MetricsEventSender::SendSaveMotionSetEvent(const EMotionFX::MotionSet* motionSet)
    {
        if (!motionSet)
        {
            return;
        }

        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("SaveMotionSet");

        uint32 numTotalMotionEntries = 0;
        uint32 numEmptyEntries      = 0;
        uint32 maxHierarchyDepth    = 0;
        CalcMotionSetStatistics(motionSet, numTotalMotionEntries, maxHierarchyDepth);

        // NumMotionEntries
        AZStd::to_string(tempString, numTotalMotionEntries);
        LyMetrics_AddAttribute(eventId, "NumMotionEntries", tempString.c_str());

        // NumEmptyEntries
        AZStd::to_string(tempString, numEmptyEntries);
        LyMetrics_AddAttribute(eventId, "NumEmptyEntries", tempString.c_str());

        // HierarchyDepth
        AZStd::to_string(tempString, maxHierarchyDepth);
        LyMetrics_AddAttribute(eventId, "HierarchyDepth", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendSaveAnimGraphEvent(const EMotionFX::AnimGraph* animGraph)
    {
        if (!animGraph)
        {
            return;
        }

        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("SaveAnimGraph");

        EMotionFX::AnimGraph::Statistics animGraphStats;
        animGraph->RecursiveCalcStatistics(animGraphStats);

        // HierarchyDepth
        AZStd::to_string(tempString, animGraphStats.m_maxHierarchyDepth);
        LyMetrics_AddAttribute(eventId, "HierarchyDepth", tempString.c_str());

        // NumTotalNodes
        const uint32 numTotalNodes = animGraph->RecursiveCalcNumNodes();
        AZStd::to_string(tempString, numTotalNodes);
        LyMetrics_AddAttribute(eventId, "NumTotalNodes", tempString.c_str());

        // NumTotalConnections
        const uint32 numTotalConnections = animGraph->RecursiveCalcNumNodeConnections();
        AZStd::to_string(tempString, numTotalConnections);
        LyMetrics_AddAttribute(eventId, "NumTotalConnections", tempString.c_str());

        // NumTotalStates
        AZStd::to_string(tempString, animGraphStats.m_numStates);
        LyMetrics_AddAttribute(eventId, "NumTotalStates", tempString.c_str());

        // NumTotalTransitions
        AZStd::to_string(tempString, animGraphStats.m_numTransitions);
        LyMetrics_AddAttribute(eventId, "NumTotalTransitions", tempString.c_str());

        // NumTotalWildcardTransitions
        AZStd::to_string(tempString, animGraphStats.m_numWildcardTransitions);
        LyMetrics_AddAttribute(eventId, "NumTotalWildcardTransitions", tempString.c_str());

        // NumTotalConditions
        AZStd::to_string(tempString, animGraphStats.m_numTransitionConditions);
        LyMetrics_AddAttribute(eventId, "NumTotalConditions", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendSaveWorkspaceEvent(const EMStudio::Workspace* workspace)
    {
        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("SaveWorkspace");

        // NumActors
        AZStd::to_string(tempString, EMotionFX::GetActorManager().GetNumActors());
        LyMetrics_AddAttribute(eventId, "NumActors", tempString.c_str());

        // NumMotions
        AZStd::to_string(tempString, EMotionFX::GetMotionManager().GetNumMotions());
        LyMetrics_AddAttribute(eventId, "NumMotions", tempString.c_str());

        // NumMotionSets
        AZStd::to_string(tempString, EMotionFX::GetMotionManager().GetNumMotionSets());
        LyMetrics_AddAttribute(eventId, "NumMotionSets", tempString.c_str());

        // NumAnimGraphs
        AZStd::to_string(tempString, EMotionFX::GetAnimGraphManager().GetNumAnimGraphs());
        LyMetrics_AddAttribute(eventId, "NumAnimGraphs", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendSwitchLayoutEvent(const char* layoutName)
    {
        LyMetricIdType eventId = LyMetrics_CreateEvent("SwitchLayout");
        LyMetrics_AddAttribute(eventId, "layoutName", layoutName);
        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendStartRecorderEvent(const EMotionFX::Recorder& recorder)
    {
        LyMetricIdType eventId = LyMetrics_CreateEvent("StartRecorder");

        const AZStd::string sessionUuid = EMotionFX::GetRecorder().GetSessionUuid().ToString<AZStd::string>();
        LyMetrics_AddAttribute(eventId, "RecordingSessionId", sessionUuid.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendStopRecorderEvent(const EMotionFX::Recorder& recorder)
    {
        AZStd::string tempString;

        LyMetricIdType eventId = LyMetrics_CreateEvent("StopRecorder");

        // SessionId
        tempString = EMotionFX::GetRecorder().GetSessionUuid().ToString<AZStd::string>();
        LyMetrics_AddAttribute(eventId, "RecordingSessionId", tempString.c_str());

        // RecordingTime
        AZStd::to_string(tempString, recorder.GetRecordTime());
        LyMetrics_AddAttribute(eventId, "RecordingTime", tempString.c_str());

        // Use the same way to retrieve the recorded actor instance as we do in the time view
        AZ::u32 numMotionEvents = 0;
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance)
        {
            const uint32 actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
            if (actorInstanceDataIndex != MCORE_INVALIDINDEX32)
            {
                const EMotionFX::Recorder::ActorInstanceData& actorInstanceData = recorder.GetActorInstanceData(actorInstanceDataIndex);

                numMotionEvents = actorInstanceData.mEventHistoryItems.GetLength();
            }
        }

        // NumMotionEvents
        AZStd::to_string(tempString, numMotionEvents);
        LyMetrics_AddAttribute(eventId, "NumMotionEvents", tempString.c_str());

        // MaxNumActiveMotions
        const AZ::u32 maxNumActiveMotions = recorder.CalcMaxNumActiveMotions();
        AZStd::to_string(tempString, maxNumActiveMotions);
        LyMetrics_AddAttribute(eventId, "MaxNumActiveMotions", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendCreateNodeEvent(const AZ::TypeId& typeId)
    {
        LyMetricIdType eventId = LyMetrics_CreateEvent("CreateNode");
        LyMetrics_AddAttribute(eventId, "Type", typeId.ToString<AZStd::string>().c_str());
        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendCreateConnectionEvent(bool isTransition)
    {
        LyMetricIdType eventId = LyMetrics_CreateEvent("CreateConnection");
        LyMetrics_AddAttribute(eventId, "IsTransition", AZStd::to_string(isTransition).c_str());
        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendDeleteNodesAndConnectionsEvent(AZ::u32 numNodes, AZ::u32 numConnections)
    {
        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("DeleteNodesAndConnections");

        // NumNodes
        AZStd::to_string(tempString, numNodes);
        LyMetrics_AddAttribute(eventId, "NumNodes", tempString.c_str());

        // NumConnections
        AZStd::to_string(tempString, numConnections);
        LyMetrics_AddAttribute(eventId, "NumConnections", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendPasteConditionsEvent(AZ::u32 numConditions)
    {
        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("PasteConditions");

        // NumConditions
        AZStd::to_string(tempString, numConditions);
        LyMetrics_AddAttribute(eventId, "NumConditions", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendPasteNodesAndConnectionsEvent(AZ::u32 numNodes, AZ::u32 numConnections)
    {
        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("PasteNodesAndConnections");

        // NumSelectedNodes
        AZStd::to_string(tempString, numNodes);
        LyMetrics_AddAttribute(eventId, "NumSelectedNodes", tempString.c_str());

        // NumSelectedConnections
        AZStd::to_string(tempString, numConnections);
        LyMetrics_AddAttribute(eventId, "NumSelectedConnections", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendMorphTargetUseEvent(AZ::u32 numMotions, AZ::u32 numSkeletalMotionsWithMorph)
    {
        AZStd::string tempString;
        LyMetricIdType eventId = LyMetrics_CreateEvent("MorphTargetUse");

        AZStd::to_string(tempString, numMotions);
        LyMetrics_AddAttribute(eventId, "NumMotions", tempString.c_str());

        AZStd::to_string(tempString, numSkeletalMotionsWithMorph);
        LyMetrics_AddAttribute(eventId, "NumMorphMotions", tempString.c_str());

        LyMetrics_SubmitEvent(eventId);
    }


    void MetricsEventSender::SendMorphTargetConcurrentPlayEvent( const EMotionFX::MotionSet* motionSet)
    {
        if (motionSet)
        {
            size_t numMorphMotions = motionSet->GetNumMorphMotions();
            if (numMorphMotions > 0)
            {
                const float timestep = 0.1f;// 10 fps sampling
                //Max # of morph targets active per animation
                const EMotionFX::MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
                AZ::u32 maxNumConcurrentMorphInMotion = 0;
                for (const auto& item : motionEntries)
                {
                    const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;
                    if (motionEntry->GetMotion()->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
                    {
                        EMotionFX::SkeletalMotion* skeletalMotion = static_cast<EMotionFX::SkeletalMotion*>(motionEntry->GetMotion());
                        const uint32 numMorphSubMotion = skeletalMotion->GetNumMorphSubMotions();
                        if (numMorphSubMotion > 0)
                        {
                            const float maxTime = skeletalMotion->GetMaxTime();
                            for (float time = 0; time < maxTime; time += timestep)
                            {
                                uint32 timeValuesAccumulator = 0;
                                for (uint32 subMotionIndex = 0; subMotionIndex < numMorphSubMotion; ++subMotionIndex)
                                {
                                    EMotionFX::MorphSubMotion* subMotion = skeletalMotion->GetMorphSubMotion(subMotionIndex);
                                    EMotionFX::KeyTrackLinear<float, MCore::Compressed16BitFloat>* keyTrack = subMotion->GetKeyTrack();
                                    const float value = keyTrack->GetValueAtTime(time);
                                    if (value > 0)
                                    {
                                        timeValuesAccumulator++;
                                    }
                                }
                                if (timeValuesAccumulator > maxNumConcurrentMorphInMotion)
                                {
                                    maxNumConcurrentMorphInMotion = timeValuesAccumulator;
                                }
                            }
                        }
                    }
                }

                AZStd::string tempString;
                LyMetricIdType eventId = LyMetrics_CreateEvent("MorphTargetConcurrentPlay");

                AZStd::to_string(tempString, maxNumConcurrentMorphInMotion);
                LyMetrics_AddAttribute(eventId, "MaxNumConcurrentMorphPlayback", tempString.c_str());

                LyMetrics_SubmitEvent(eventId);
            }
        }
    }


    void MetricsEventSender::SendActorMorphTargetSizesEvent(const EMotionFX::MorphSetup * morphSetup)
    {
        if (morphSetup)
        {
            const AZ::u32 numMorphTargets = morphSetup->GetNumMorphTargets();
            if (numMorphTargets > 0)
            {
                //Number of vertices in each morph target(min / max / average) defined in loaded actor in Animation Editor
                AZ::u32 min = UINT_MAX;
                AZ::u32 max = 0;
                AZ::u32 accumulator = 0;
                for (size_t morphTargetIndex = 0; morphTargetIndex < numMorphTargets; ++morphTargetIndex)
                {
                    AZ::u32 indexCount = 0;
                    EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(static_cast<uint32>(morphTargetIndex));
                    if (morphTarget->GetType() == EMotionFX::MorphTargetStandard::TYPE_ID)
                    {
                        EMotionFX::MorphTargetStandard* morphTargetStandard = static_cast<EMotionFX::MorphTargetStandard*>(morphTarget);
                        const size_t numDeformData = morphTargetStandard->GetNumDeformDatas();

                        for (size_t deformDataIndex = 0; deformDataIndex < numDeformData; ++deformDataIndex)
                        {
                            EMotionFX::MorphTargetStandard::DeformData* deformData = morphTargetStandard->GetDeformData(static_cast<uint32>(deformDataIndex));
                            indexCount += deformData->mNumVerts;
                        }
                    }
                    min = AZStd::GetMin(min, indexCount);
                    max = AZStd::GetMax(max, indexCount);
                    accumulator += indexCount;
                }

                AZStd::string tempString;
                LyMetricIdType eventId = LyMetrics_CreateEvent("ActorMorphTargetSizes");

                AZStd::to_string(tempString, min);
                LyMetrics_AddAttribute(eventId, "MinSize", tempString.c_str());

                AZStd::to_string(tempString, max);
                LyMetrics_AddAttribute(eventId, "MaxSize", tempString.c_str());

                AZStd::to_string(tempString, accumulator / numMorphTargets);
                LyMetrics_AddAttribute(eventId, "AverageSize", tempString.c_str());

                LyMetrics_SubmitEvent(eventId);
            }
        }
       
    }
} // namespace EMStudio