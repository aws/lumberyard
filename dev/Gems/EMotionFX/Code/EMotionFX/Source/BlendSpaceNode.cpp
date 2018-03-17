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

#include "BlendSpaceNode.h"
#include "BlendSpaceManager.h"
#include "MotionInstance.h"
#include "MotionEventTable.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"
#include <MCore/Source/AttributeSettings.h>



namespace EMotionFX
{
    BlendSpaceNode::MotionInfo::MotionInfo()
        : m_motionInstance(nullptr)
        , m_syncIndex(MCORE_INVALIDINDEX32)
        , m_playSpeed(1.0f)
        , m_currentTime(0.0f)
        , m_preSyncTime(0.0f)
    {
    }

    BlendSpaceNode::BlendSpaceNode(AnimGraph* animGraph, const char* name, uint32 typeID)
        : AnimGraphNode(animGraph, name, typeID)
        , mInteractiveMode(false)
    {
    }

    void BlendSpaceNode::RegisterCalculationMethodAttribute(const char* name, const char* internalName, const char* description)
    {
        MCore::AttributeSettings* attributeInfo = RegisterAttribute(name, internalName, description, MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attributeInfo->ResizeComboValues(2);
        attributeInfo->SetComboValue(static_cast<int>(ECalculationMethod::AUTO), "Automatically calculate motion coordinates");
        attributeInfo->SetComboValue(static_cast<int>(ECalculationMethod::MANUAL), "Manually enter motion coordinates");
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(static_cast<int>(ECalculationMethod::AUTO)));
        attributeInfo->SetFlag(MCore::AttributeSettings::FLAGINDEX_REINIT_ATTRIBUTEWINDOW, true);
    }

    void BlendSpaceNode::RegisterBlendSpaceEvaluatorAttribute(const char* name, const char* internalName, const char* description)
    {
        const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();

        MCore::AttributeSettings* attribute = RegisterAttribute(name, internalName, description, MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);

        const size_t evaluatorCount = blendSpaceManager->GetParameterEvaluatorCount();
        attribute->ResizeComboValues(static_cast<uint32>(evaluatorCount));
        for (size_t i = 0; i < evaluatorCount; ++i)
        {
            EMotionFX::BlendSpaceParamEvaluator* evaluator = blendSpaceManager->GetParameterEvaluator(i);
            attribute->SetComboValue(static_cast<uint32>(i), evaluator->GetName());
        }

        attribute->SetDefaultValue(MCore::AttributeFloat::Create(0.0f));
        attribute->SetFlag(MCore::AttributeSettings::FLAGINDEX_REINIT_ATTRIBUTEWINDOW, true);
    }

    BlendSpaceNode::ECalculationMethod BlendSpaceNode::GetBlendSpaceCalculationMethod(uint32 attribIndex) const
    {
        return static_cast<ECalculationMethod>(GetAttributeFloatAsUint32(attribIndex));
    }

    BlendSpaceParamEvaluator* BlendSpaceNode::GetBlendSpaceParamEvaluator(uint32 attribIndex) const
    {
#ifdef EMFX_EMSTUDIOBUILD
        if (GetIsAttributeDisabled(attribIndex))
        {
            return nullptr;
        }
#endif

        const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();

        const size_t evaluatorIndex = static_cast<size_t>(GetAttributeFloatAsUint32(attribIndex));
        AZ_Assert(evaluatorIndex < blendSpaceManager->GetParameterEvaluatorCount(), "Invalid blend space parameter evaluator.");

        return blendSpaceManager->GetParameterEvaluator(evaluatorIndex);
    }

    void BlendSpaceNode::RegisterBlendSpaceEventFilterAttribute()
    {
        MCore::AttributeSettings* attributeInfo = RegisterAttribute("Event Filter Mode", "eventMode", "The event filter mode, which controls which events are passed further up the hierarchy.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attributeInfo->ResizeComboValues(2);
        attributeInfo->SetComboValue(BSEVENTMODE_ALL_ACTIVE_MOTIONS, "All Currently Active Motions");
        attributeInfo->SetComboValue(BSEVENTMODE_MOST_ACTIVE_MOTION, "Most Active Motion Only");
        attributeInfo->SetDefaultValue(MCore::AttributeFloat::Create(BSEVENTMODE_ALL_ACTIVE_MOTIONS));
        attributeInfo->SetFlag(MCore::AttributeSettings::FLAGINDEX_REINITGUI_ONVALUECHANGE, true);
    }

    void BlendSpaceNode::RegisterMasterMotionAttribute()
    {
        MCore::AttributeSettings* param = RegisterAttribute("Sync Master Motion", "masterMotion", "The master motion used for motion synchronization.", ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONPICKER);
        param->SetReinitGuiOnValueChange(true);
        param->SetDefaultValue(MCore::AttributeString::Create());
    }

    void BlendSpaceNode::DoUpdate(float timePassedInSeconds, const BlendInfos& blendInfos, ESyncMode syncMode, AZ::u32 masterIdx, MotionInfos& motionInfos)
    {
        float blendedDuration = 0;
        for (const BlendInfo& blendInfo : blendInfos)
        {
            const MotionInfo& motionInfo = motionInfos[blendInfo.m_motionIndex];
            blendedDuration += blendInfo.m_weight * motionInfo.m_motionInstance->GetDuration();
        }
        if (blendedDuration < MCore::Math::epsilon)
        {
            return;
        }

        const AZ::u32 numMotions = (AZ::u32)motionInfos.size();
        for (AZ::u32 i=0; i < numMotions; ++i)
        {
            MotionInfo& motionInfo = motionInfos[i];
            MotionInstance* motionInstance = motionInfo.m_motionInstance;
            motionInstance->SetFreezeAtLastFrame(!motionInstance->GetIsPlayingForever());
            motionInstance->SetPlaySpeed(motionInfo.m_playSpeed);
            motionInfo.m_preSyncTime = motionInstance->GetCurrentTime();

            // If syncing is enabled, we are going to update the current play time (m_currentTime) of all motions later based
            // on the master's. Otherwise, we need to update them now itself.
            if ((syncMode == SYNCMODE_DISABLED) || (i == masterIdx))
            {
                float newTime;
                float passedTime;
                float totalPlayTime;
                float timeDifToEnd;
                uint32 numLoops;
                bool hasLooped = false;
                bool frozenInLastFrame;
                motionInstance->CalcNewTimeAfterUpdate(timePassedInSeconds, &newTime, &passedTime, &totalPlayTime, &timeDifToEnd, &numLoops, &hasLooped, &frozenInLastFrame);

                motionInfo.m_currentTime = newTime;
            }

            motionInstance->SetPause(false);
            motionInfo.m_playSpeed = (i == masterIdx) ? motionInstance->GetDuration() / blendedDuration : 1.0f;
        }
    }

    void BlendSpaceNode::DoTopDownUpdate(AnimGraphInstance* animGraphInstance, ESyncMode syncMode,
        AZ::u32 masterIdx, MotionInfos& motionInfos, bool motionsHaveSyncTracks)
    {
        if (motionInfos.empty() || (syncMode == SYNCMODE_DISABLED))
        {
            return;
        }

        SyncMotionToNode(animGraphInstance, syncMode, motionInfos[masterIdx], this);

        ESyncMode motionSyncMode = syncMode;
        if ((motionSyncMode == SYNCMODE_TRACKBASED) && !motionsHaveSyncTracks)
        {
            motionSyncMode = SYNCMODE_CLIPBASED;
        }

        if (motionSyncMode == SYNCMODE_CLIPBASED)
        {
            DoClipBasedSyncOfMotionsToMaster(masterIdx, motionInfos);
        }
        else
        {
            DoEventBasedSyncOfMotionsToMaster(masterIdx, motionInfos);
        }
    }

    void BlendSpaceNode::DoPostUpdate(AnimGraphInstance* animGraphInstance, AZ::u32 masterIdx, BlendInfos& blendInfos, MotionInfos& motionInfos,
            EBlendSpaceEventMode eventFilterMode, AnimGraphRefCountedData* data)
    {
        MCORE_UNUSED(animGraphInstance);
        MCORE_UNUSED(masterIdx);

        const AZ::u32 numMotions = (AZ::u32)motionInfos.size();
        for (AZ::u32 i=0; i < numMotions; ++i)
        {
            MotionInfo& motionInfo = motionInfos[i];
            MotionInstance* motionInstance = motionInfo.m_motionInstance;

            const AZ::u32 indexInBlendInfos = GetIndexOfMotionInBlendInfos(blendInfos, i);
            if (indexInBlendInfos == MCORE_INVALIDINDEX32)
            {
                // It is not part of blend infos. Just update the time in this case without emitting events.
                // We update the time even for these so that motions stay in sync.
                motionInstance->UpdateByTimeValues(motionInfo.m_preSyncTime, motionInfo.m_currentTime, nullptr);
            }
            else
            {
                // If using BSEVENTMODE_MOST_ACTIVE_MOTION, pass nullptr as data for all but the
                // first motion so that we collect the events only for the first motion which has the highest weight.
                if ((eventFilterMode == BSEVENTMODE_MOST_ACTIVE_MOTION) && (indexInBlendInfos != 0))
                {
                    motionInstance->UpdateByTimeValues(motionInfo.m_preSyncTime, motionInfo.m_currentTime, nullptr);
                }
                else
                {
                    motionInstance->UpdateByTimeValues(motionInfo.m_preSyncTime, motionInfo.m_currentTime, &data->GetEventBuffer());
                }
            }
        }
 
        data->GetEventBuffer().UpdateEmitters(this);

        Transform trajectoryDelta;
        Transform trajectoryDeltaAMirrored;
        if (blendInfos.empty())
        {
            trajectoryDelta.ZeroWithIdentityQuaternion();
            trajectoryDeltaAMirrored.ZeroWithIdentityQuaternion();
        }
        else
        {
            trajectoryDelta.Zero();
            trajectoryDeltaAMirrored.Zero();
            for (const BlendInfo& blendInfo : blendInfos)
            {
                MotionInstance* motionInstance = motionInfos[blendInfo.m_motionIndex].m_motionInstance;
                if (!motionInstance->GetIsReadyForSampling())
                {
                    motionInstance->InitForSampling();
                }
                Transform instanceDelta;
                const bool isMirrored = motionInstance->GetMirrorMotion();
                motionInstance->ExtractMotion(instanceDelta);
                trajectoryDelta.Add(instanceDelta, blendInfo.m_weight);

                // extract mirrored version of the current delta
                motionInstance->SetMirrorMotion(!isMirrored);
                motionInstance->ExtractMotion(instanceDelta);
                trajectoryDeltaAMirrored.Add(instanceDelta, blendInfo.m_weight);
                motionInstance->SetMirrorMotion(isMirrored); // restore current mirrored flag
            }
            trajectoryDelta.mRotation.Normalize();
            trajectoryDeltaAMirrored.mRotation.Normalize();
        }

        data->SetTrajectoryDelta(trajectoryDelta);
        data->SetTrajectoryDeltaMirrored(trajectoryDeltaAMirrored);
    }

    void BlendSpaceNode::ClearMotionInfos(MotionInfos& motionInfos)
    {
        MotionInstancePool& motionInstancePool = GetMotionInstancePool();

        for (MotionInfo& motionInfo : motionInfos)
        {
            if (motionInfo.m_motionInstance)
            {
                motionInstancePool.Free(motionInfo.m_motionInstance);
            }
        }
        motionInfos.clear();
    }

    void BlendSpaceNode::AddMotionInfo(MotionInfos& motionInfos, MotionInstance* motionInstance)
    {
        AZ_Assert(motionInstance, "Invalid MotionInstance pointer");

        motionInfos.push_back(MotionInfo());
        MotionInfo& motionInfo = motionInfos.back();

        motionInfo.m_motionInstance = motionInstance;
        motionInstance->SetFreezeAtLastFrame(!motionInstance->GetIsPlayingForever());

        MotionEventTable* eventTable = motionInstance->GetMotion()->GetEventTable();
        const uint32 syncTracIdx = eventTable->FindTrackIndexByName("Sync");
        if (syncTracIdx != MCORE_INVALIDINDEX32)
        {
            const MotionEventTrack* eventTrack = eventTable->GetTrack(syncTracIdx);
            if (!motionInstance->GetMirrorMotion())
            {
                motionInfo.m_syncTrack.InitFromEventTrack(eventTrack);
            }
            else
            {
                motionInfo.m_syncTrack.InitFromEventTrackMirrored(eventTrack);
            }
        }
        else
        {
            motionInfo.m_syncTrack.Clear();
        }

        motionInfo.m_playSpeed = motionInstance->GetPlaySpeed();
    }

    bool BlendSpaceNode::DoAllMotionsHaveSyncTracks(const MotionInfos& motionInfos)
    {
        for (const MotionInfo& motionInfo : motionInfos)
        {
            if (motionInfo.m_syncTrack.GetNumEvents() == 0)
            {
                return false;
            }
        }
        return true;
    }

    void BlendSpaceNode::DoClipBasedSyncOfMotionsToMaster(AZ::u32 masterIdx, MotionInfos& motionInfos)
    {
        const AZ::u32 numMotionInfos = (AZ::u32)motionInfos.size();
        if (masterIdx >= numMotionInfos)
        {
            return;
        }
        const MotionInfo& masterInfo = motionInfos[masterIdx];
        const float masterDuration = masterInfo.m_motionInstance->GetDuration();
        if (masterDuration < MCore::Math::epsilon)
        {
            return;
        }
        const float normalizedTime = masterInfo.m_currentTime / masterDuration;

        for (AZ::u32 motionIdx=0; motionIdx < numMotionInfos; ++motionIdx)
        {
            if (motionIdx != masterIdx)
            {
                MotionInfo& info = motionInfos[motionIdx];
                const float duration = info.m_motionInstance->GetDuration();
                info.m_playSpeed = (masterInfo.m_playSpeed * duration) / masterDuration;
                info.m_currentTime = normalizedTime * duration;
            }
        }
    }

    void BlendSpaceNode::DoEventBasedSyncOfMotionsToMaster(AZ::u32 masterIdx, MotionInfos& motionInfos)
    {
        const AZ::u32 numMotionInfos = (AZ::u32)motionInfos.size();
        if (masterIdx >= numMotionInfos)
        {
            return;
        }
        MotionInfo& srcMotion = motionInfos[masterIdx];
        const AnimGraphSyncTrack& srcTrack = srcMotion.m_syncTrack;

        const float srcCurrentTime = srcMotion.m_currentTime;
        const bool forward = srcMotion.m_motionInstance->GetPlayMode() != PLAYMODE_BACKWARD;

        uint32 srcIndexA;
        uint32 srcIndexB;
        if (!srcTrack.FindEventIndices(srcCurrentTime, &srcIndexA, &srcIndexB))
        {
            return;
        }
        const bool srcSyncIndexChanged = srcMotion.m_syncIndex != srcIndexA;
        srcMotion.m_syncIndex = srcIndexA;
        const float srcDuration = srcTrack.CalcSegmentLength(srcIndexA, srcIndexB);
        // calculate the normalized offset inside the segment
        float normalizedOffset;
        if (srcIndexA < srcIndexB) // normal case
        {
            normalizedOffset = (srcDuration > MCore::Math::epsilon) ? (srcCurrentTime - srcTrack.GetEvent(srcIndexA).mTime) / srcDuration : 0.0f;
        }
        else // looping case
        {
            float timeOffset;
            if (srcCurrentTime > srcTrack.GetEvent(0).mTime)
            {
                timeOffset = srcCurrentTime - srcTrack.GetEvent(srcIndexA).mTime;
            }
            else
            {
                const float srcMotionDuration = srcMotion.m_motionInstance->GetDuration();
                timeOffset = (srcMotionDuration - srcTrack.GetEvent(srcIndexA).mTime) + srcCurrentTime;
            }

            normalizedOffset = (srcDuration > MCore::Math::epsilon) ? timeOffset / srcDuration : 0.0f;
        }

        for (AZ::u32 motionIdx = 0; motionIdx < numMotionInfos; ++motionIdx)
        {
            if (motionIdx == masterIdx)
            {
                continue;
            }
            MotionInfo& targetMotion = motionInfos[motionIdx];
            const AnimGraphSyncTrack& targetTrack = targetMotion.m_syncTrack;
            uint32 startEventIndex = targetMotion.m_syncIndex;
            if (srcSyncIndexChanged)
            {
                if (forward)
                {
                    startEventIndex++;
                    if (startEventIndex >= targetTrack.GetNumEvents())
                    {
                        startEventIndex = 0;
                    }
                }
                else
                {
                    if (startEventIndex == 0)
                    {
                        startEventIndex = targetTrack.GetNumEvents() - 1;
                    }
                    else
                    {
                        startEventIndex--;
                    }
                }
            }

            // Find the matching indices in the target track.
            uint32 targetIndexA;
            uint32 targetIndexB;
            if (!targetMotion.m_syncTrack.FindMatchingEvents(startEventIndex, srcTrack.GetEvent(srcIndexA).mID, srcTrack.GetEvent(srcIndexB).mID, &targetIndexA, &targetIndexB, forward))
            {
                return;
            }

            targetMotion.m_syncIndex = targetIndexA;

            // calculate the segment lengths
            const float targetDuration = targetTrack.CalcSegmentLength(targetIndexA, targetIndexB);

            // calculate the new time in the motion
            float newTargetTime;
            if (targetIndexA < targetIndexB) // if the second segment is a non-wrapping one, so a regular non-looping case
            {
                newTargetTime = targetTrack.GetEvent(targetIndexA).mTime + targetDuration * normalizedOffset;
            }
            else // looping case
            {
                // calculate the new play time
                const float unwrappedTime = targetTrack.GetEvent(targetIndexA).mTime + targetDuration * normalizedOffset;

                // if it is past the motion duration, we need to wrap around
                const float targetMotionDuration = targetMotion.m_motionInstance->GetDuration();
                if (unwrappedTime > targetMotionDuration)
                {
                    // the new wrapped time
                    newTargetTime = MCore::Math::SafeFMod(unwrappedTime, targetMotionDuration);
                }
                else
                {
                    newTargetTime = unwrappedTime;
                }
            }

            targetMotion.m_currentTime = newTargetTime;
            targetMotion.m_playSpeed = (srcDuration > MCore::Math::epsilon) ? targetDuration / srcDuration : 0.0f;
        }
    }


    uint32 BlendSpaceNode::FindBlendSpaceMotionAttributeIndexByMotionId(uint32 motionsAttributeIndex, const AZStd::string& motionId) const
    {
        const MCore::AttributeArray* attributeArray = GetAttributeArray(motionsAttributeIndex);

        const uint32 numMotions = attributeArray->GetNumAttributes();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            const AttributeBlendSpaceMotion* attribute = static_cast<AttributeBlendSpaceMotion*>(attributeArray->GetAttribute(i));
            if (attribute->GetMotionId() == motionId)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    void BlendSpaceNode::SyncMotionToNode(AnimGraphInstance* animGraphInstance, ESyncMode syncMode, MotionInfo& motionInfo, AnimGraphNode* srcNode)
    {
        MCORE_UNUSED(syncMode);

        motionInfo.m_currentTime = srcNode->GetCurrentPlayTime(animGraphInstance);
        motionInfo.m_playSpeed  = srcNode->GetPlaySpeed(animGraphInstance);
    }


    void BlendSpaceNode::RewindMotions(MotionInfos& motionInfos)
    {
        for (MotionInfo& motionInfo : motionInfos)
        {
            MotionInstance* motionInstance = motionInfo.m_motionInstance;
            if (!motionInstance)
            {
                continue;
            }

            motionInstance->Rewind();

            motionInfo.m_currentTime = motionInstance->GetCurrentTime();
            motionInfo.m_preSyncTime = motionInfo.m_currentTime;
            motionInfo.m_syncIndex = MCORE_INVALIDINDEX32;
        }
    }

} // namespace EMotionFX

