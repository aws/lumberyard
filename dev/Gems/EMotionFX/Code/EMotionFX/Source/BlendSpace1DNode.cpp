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

#include "BlendSpace1DNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "BlendSpaceManager.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraph.h"
#include "MotionSet.h"
#include "EMotionFXManager.h"
#include <MCore/Source/AttributeSettings.h>
#include <AzCore/std/sort.h>
#include <AzCore/Casting/numeric_cast.h>

namespace
{
    class MotionSortComparer
    {
    public:
        MotionSortComparer(const AZStd::vector<float>& motionCoordinates)
            : m_motionCoordinates(motionCoordinates)
        {
        }
        bool operator()(AZ::u32 a, AZ::u32 b) const
        {
            return m_motionCoordinates[a] < m_motionCoordinates[b];
        }

    private:
        const AZStd::vector<float>& m_motionCoordinates;
    };
}

namespace EMotionFX
{
    BlendSpace1DNode::UniqueData::~UniqueData()
    {
        BlendSpaceNode::ClearMotionInfos(m_motionInfos);
    }

    BlendSpace1DNode* BlendSpace1DNode::Create(AnimGraph* animGraph)
    {
        return new BlendSpace1DNode(animGraph);
    }

    bool BlendSpace1DNode::GetValidCalculationMethodAndEvaluator() const
    {
        // if the evaluators is null, is in "manual" mode. 
        const ECalculationMethod calculationMethod = GetBlendSpaceCalculationMethod(ATTRIB_CALCULATION_METHOD);
        if (calculationMethod == ECalculationMethod::MANUAL) 
        {
            return true;
        }
        else
        {
            const BlendSpaceParamEvaluator* evaluator = GetBlendSpaceParamEvaluator(ATTRIB_EVALUATOR);
            AZ_Assert(evaluator, "Expected non-null blend space param evaluator");
            return !evaluator->IsNullEvaluator();
        }
    }

    const char* BlendSpace1DNode::GetAxisLabel() const
    {
        BlendSpaceParamEvaluator* evaluator = GetBlendSpaceParamEvaluator(ATTRIB_EVALUATOR);
        if (!evaluator || evaluator->IsNullEvaluator())
        {
            return "X-Axis";
        }
        return evaluator->GetName();
    }

    void BlendSpace1DNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // Find the unique data for this node, if it doesn't exist yet, create it.
        UniqueData* uniqueData = static_cast<BlendSpace1DNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (!uniqueData)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        UpdateMotionInfos(animGraphInstance);
    }

    void BlendSpace1DNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
    }

    void BlendSpace1DNode::RegisterPorts()
    {
        InitInputPorts(1);
        SetupInputPortAsNumber("X", INPUTPORT_VALUE, PORTID_INPUT_VALUE);

        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    void BlendSpace1DNode::RegisterAttributes()
    {
        RegisterCalculationMethodAttribute("Calculation method (X-Axis)", "calculationMethodX", "Calculation method for the X Axis");
        RegisterBlendSpaceEvaluatorAttribute("X-Axis Evaluator", "evaluatorX", "Evaluator for the X axis value of motions");
        RegisterSyncAttribute();
        RegisterMasterMotionAttribute();
        RegisterBlendSpaceEventFilterAttribute();

        MCore::AttributeSettings* attrib = RegisterAttribute("Motions", "motions", "Source motions for blend space", ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONS);
        attrib->SetReinitGuiOnValueChange(true);
        attrib->SetDefaultValue(MCore::AttributeArray::Create(AttributeBlendSpaceMotion::TYPE_ID));
    }

    const char* BlendSpace1DNode::GetTypeString() const
    {
        return "BlendSpace1DNode";
    }

    const char* BlendSpace1DNode::GetPaletteName() const
    {
        return "Blend Space 1D";
    }

    AnimGraphObject::ECategory BlendSpace1DNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }

    // This implementation currently just does what the other node classes in the
    // EMFX code base are doing. This is not a proper implementation of
    // "Clone" since "CopyBaseObjectTo" currently copies only the AnimGraphObject
    AnimGraphObject* BlendSpace1DNode::Clone(AnimGraph* animGraph)
    {
        BlendSpace1DNode* clone = new BlendSpace1DNode(animGraph);

        CopyBaseObjectTo(clone);

        return clone;
    }

    AnimGraphObjectData* BlendSpace1DNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }

    // Called when the attributes change.
    void BlendSpace1DNode::OnUpdateAttributes()
    {
        if (mAttributeValues.GetIsEmpty())
        {
            return;
        }

        // Mark AttributeBlendSpaceMotion attributes as belonging to 1D blend space
        MCore::AttributeArray* attributeArray = GetMotionAttributeArray();
        if (attributeArray)
        {
            const uint32 numMotions = attributeArray->GetNumAttributes();
            for (uint32 i = 0; i < numMotions; ++i)
            {
                AttributeBlendSpaceMotion* attrib = static_cast<AttributeBlendSpaceMotion*>(attributeArray->GetAttribute(i));
                attrib->SetDimension(1);
            }
        }

        const uint32 numAnimGraphInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);

            OnUpdateUniqueData(animGraphInstance);
        }

        // disable GUI items that have no influence
#ifdef EMFX_EMSTUDIOBUILD
        EnableAllAttributes(true);
        // Here, disable any attribute as needed.

        const ECalculationMethod calculationMethod = GetBlendSpaceCalculationMethod(ATTRIB_CALCULATION_METHOD);
        if (calculationMethod == ECalculationMethod::MANUAL)
        {
            SetAttributeDisabled(ATTRIB_EVALUATOR);
            GetAttributeFloat(ATTRIB_EVALUATOR)->SetValue(0.0f);
        }
        else // Automatic calculation method
        {
            SetAttributeEnabled(ATTRIB_EVALUATOR);
        }
#endif
    }

    void BlendSpace1DNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // If the node is disabled, simply output a bind pose.
        if (mDisabled)
        {
            SetBindPoseAtOutput(animGraphInstance);
            return;
        }

        OutputAllIncomingNodes(animGraphInstance);

        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        outputPose->InitFromBindPose(actorInstance);
        Pose& outputLocalPose = outputPose->GetPose();
        outputLocalPose.Zero();

        const uint32 threadIndex = actorInstance->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();

        AnimGraphPose* bindPose = posePool.RequestPose(actorInstance);
        bindPose->InitFromBindPose(actorInstance);
        AnimGraphPose* motionOutPose = posePool.RequestPose(actorInstance);
        Pose& motionOutLocalPose = motionOutPose->GetPose();

        if (uniqueData->m_currentSegment.m_segmentIndex != MCORE_INVALIDINDEX32)
        {
            const AZ::u32 segIndex = uniqueData->m_currentSegment.m_segmentIndex;
            for (int i = 0; i < 2; ++i)
            {
                MotionInstance* motionInstance = uniqueData->m_motionInfos[uniqueData->m_sortedMotions[segIndex + i]].m_motionInstance;
                motionOutPose->InitFromBindPose(actorInstance);
                motionInstance->GetMotion()->Update(&bindPose->GetPose(), &motionOutLocalPose, motionInstance);

                if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled())
                {
                    motionOutLocalPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
                }

                const float weight = (i == 0) ? (1 - uniqueData->m_currentSegment.m_weightForSegmentEnd) : uniqueData->m_currentSegment.m_weightForSegmentEnd;
                outputLocalPose.Sum(&motionOutLocalPose, weight);
            }
            outputLocalPose.NormalizeQuaternions();
        }
        else if (!uniqueData->m_motionInfos.empty())
        {
            const AZ::u16 motionIdx = (uniqueData->m_currentPosition < uniqueData->GetRangeMin()) ? uniqueData->m_sortedMotions.front() : uniqueData->m_sortedMotions.back();
            MotionInstance* motionInstance = uniqueData->m_motionInfos[motionIdx].m_motionInstance;
            motionOutPose->InitFromBindPose(actorInstance);
            motionInstance->GetMotion()->Update(&bindPose->GetPose(), &motionOutLocalPose, motionInstance);

            if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled())
            {
                motionOutLocalPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
            }

            outputLocalPose.Sum(&motionOutLocalPose, 1.0f);
            outputLocalPose.NormalizeQuaternions();
        }
        else
        {
            SetBindPoseAtOutput(animGraphInstance);
        }

        posePool.FreePose(motionOutPose);
        posePool.FreePose(bindPose);


#ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
#endif
    }

    void BlendSpace1DNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (mDisabled)
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        const ESyncMode syncMode = static_cast<ESyncMode>(static_cast<uint32>(GetAttributeFloat(ATTRIB_SYNC)->GetValue()));
        DoTopDownUpdate(animGraphInstance, syncMode, uniqueData->m_masterMotionIdx,
            uniqueData->m_motionInfos, uniqueData->m_allMotionsHaveSyncTracks);

        EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(INPUTPORT_VALUE).mConnection;
        if (paramConnection)
        {
            AnimGraphNode* paramSrcNode = paramConnection->GetSourceNode();
            if (paramSrcNode)
            {
                paramSrcNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
            }
        }
    }

    void BlendSpace1DNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (!mDisabled)
        {
            EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(INPUTPORT_VALUE).mConnection;
            if (paramConnection)
            {
                UpdateIncomingNode(animGraphInstance, paramConnection->GetSourceNode(), timePassedInSeconds);
            }
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        AZ_Assert(uniqueData, "UniqueData not found for BlendSpace1DNode");
        uniqueData->Clear();

        if (mDisabled)
        {
            return;
        }

        uniqueData->m_currentPosition = GetCurrentSamplePosition(animGraphInstance, *uniqueData);

        // Set the duration and current play time etc to the master motion index, or otherwise just the first motion in the list if syncing is disabled.
        const ESyncMode syncMode = (ESyncMode)((uint32)GetAttributeFloat(ATTRIB_SYNC)->GetValue());
        AZ::u32 motionIndex = (uniqueData->m_masterMotionIdx != MCORE_INVALIDINDEX32) ? uniqueData->m_masterMotionIdx : MCORE_INVALIDINDEX32;
        if (syncMode == ESyncMode::SYNCMODE_DISABLED || motionIndex == MCORE_INVALIDINDEX32)
            motionIndex = 0;

        UpdateBlendingInfoForCurrentPoint(*uniqueData);

        DoUpdate(timePassedInSeconds, uniqueData->m_blendInfos, syncMode, uniqueData->m_masterMotionIdx, uniqueData->m_motionInfos);

        if (!uniqueData->m_motionInfos.empty())
        {
            const MotionInfo& motionInfo = uniqueData->m_motionInfos[motionIndex];
            uniqueData->SetDuration( motionInfo.m_motionInstance ? motionInfo.m_motionInstance->GetDuration() : 0.0f );
            uniqueData->SetCurrentPlayTime( motionInfo.m_currentTime );
            uniqueData->SetSyncTrack( motionInfo.m_syncTrack );
            uniqueData->SetSyncIndex( motionInfo.m_syncIndex );
            uniqueData->SetPreSyncTime( motionInfo.m_preSyncTime);
            uniqueData->SetPlaySpeed( motionInfo.m_playSpeed );               
        }
    }

    void BlendSpace1DNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        if (mDisabled)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(INPUTPORT_VALUE).mConnection;
        if (paramConnection)
        {
            paramConnection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        if (uniqueData->m_motionInfos.empty())
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        const EBlendSpaceEventMode eventFilterMode = (EBlendSpaceEventMode)((uint32)GetAttributeFloat(ATTRIB_EVENTMODE)->GetValue());
        DoPostUpdate(animGraphInstance, uniqueData->m_masterMotionIdx, uniqueData->m_blendInfos, uniqueData->m_motionInfos, eventFilterMode, data);
    }

    BlendSpace1DNode::BlendSpace1DNode(AnimGraph* animGraph)
        : BlendSpaceNode(animGraph, nullptr, TYPE_ID)
        , m_currentPositionSetInteractively(false)
    {
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }

    BlendSpace1DNode::~BlendSpace1DNode()
    {
    }

    bool BlendSpace1DNode::UpdateMotionInfos(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        if (!actorInstance)
        {
            return false;
        }
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        ClearMotionInfos(uniqueData->m_motionInfos);

        MotionSet* motionSet = animGraphInstance->GetMotionSet();
        if (!motionSet)
        {
            return false;
        }

        // Initialize motion instance and parameter value arrays.
        MCore::AttributeArray* attributeArray = GetMotionAttributeArray();
        const uint32 numMotions = attributeArray->GetNumAttributes();
        AZ_Assert(uniqueData->m_motionInfos.empty(), "This is assumed to have been cleared already");
        uniqueData->m_motionInfos.reserve(numMotions);

        MotionInstancePool& motionInstancePool = GetMotionInstancePool();

        const AZStd::string masterMotionId(GetAttributeString(ATTRIB_SYNC_MASTERMOTION)->GetValue().AsChar());
        uniqueData->m_masterMotionIdx = 0;

        PlayBackInfo playInfo;// TODO: Init from attributes
        for (uint32 i = 0; i < numMotions; ++i)
        {
            AttributeBlendSpaceMotion* attribute = static_cast<AttributeBlendSpaceMotion*>(attributeArray->GetAttribute(i));
            const AZStd::string& motionId = attribute->GetMotionId();
            Motion* motion = motionSet->RecursiveFindMotionByStringID(motionId.c_str());
            if (!motion)
            {
                attribute->SetFlag(AttributeBlendSpaceMotion::TypeFlags::InvalidMotion);
                continue;
            }
            attribute->UnsetFlag(AttributeBlendSpaceMotion::TypeFlags::InvalidMotion);

            MotionInstance* motionInstance = motionInstancePool.RequestNew(motion, actorInstance, playInfo.mStartNodeIndex);
            motionInstance->InitFromPlayBackInfo(playInfo, true);
            motionInstance->SetRetargetingEnabled(animGraphInstance->GetRetargetingEnabled() && playInfo.mRetarget);

            if (!motionInstance->GetIsReadyForSampling())
            {
                motionInstance->InitForSampling();
            }
            motionInstance->UnPause();
            motionInstance->SetIsActive(true);
            motionInstance->SetWeight(1.0f, 0.0f);
            AddMotionInfo(uniqueData->m_motionInfos, motionInstance);

            if (motionId == masterMotionId)
            {
                uniqueData->m_masterMotionIdx = (AZ::u32)uniqueData->m_motionInfos.size() - 1;
            }
        }
        uniqueData->m_allMotionsHaveSyncTracks = DoAllMotionsHaveSyncTracks(uniqueData->m_motionInfos);
        
        UpdateMotionPositions(*uniqueData);

        SortMotionInstances(*uniqueData);
        uniqueData->m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;

        return true;
    }


    void BlendSpace1DNode::UpdateMotionPositions(UniqueData& uniqueData)
    {
        const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();

        // Get the motion parameter evaluator.
        BlendSpaceParamEvaluator* evaluator = nullptr;
        const ECalculationMethod calculationMethod = GetBlendSpaceCalculationMethod(ATTRIB_CALCULATION_METHOD);
        if (calculationMethod == ECalculationMethod::AUTO)
        {
            const size_t evaluatorIndex = static_cast<size_t>(GetAttributeFloatAsUint32(ATTRIB_EVALUATOR));
            evaluator = blendSpaceManager->GetParameterEvaluator(evaluatorIndex);
            if (evaluator && evaluator->IsNullEvaluator())
            {
                // "Null evaluator" is really not an evaluator.
                evaluator = nullptr;
            }
        }

        const MCore::AttributeArray* attributeArray = GetMotionAttributeArray();
        // the motions in the attributes could not match the ones in the unique data. The attribute could have some invalid motions
        const uint32 attributeMotionCount = attributeArray->GetNumAttributes();
        const size_t uniqueDataMotionCount = uniqueData.m_motionInfos.size();

        // Iterate through all motions and calculate their location in the blend space.
        uniqueData.m_motionCoordinates.resize(uniqueDataMotionCount);
        size_t uniqueDataMotionIndex = 0;
        for (uint32 iAttributeMotionIndex = 0; iAttributeMotionIndex < attributeMotionCount; ++iAttributeMotionIndex)
        {
            const AttributeBlendSpaceMotion* attribute = static_cast<AttributeBlendSpaceMotion*>(attributeArray->GetAttribute(iAttributeMotionIndex));
            if (attribute->TestFlag(AttributeBlendSpaceMotion::TypeFlags::InvalidMotion))
            {
                continue;
            }

            // Calculate the position of the motion in the blend space.
            if (attribute->IsXCoordinateSetByUser())
            {
                // Did the user set the values manually? If so, use that.
                uniqueData.m_motionCoordinates[uniqueDataMotionIndex] = attribute->GetXCoordinate();
            }
            else if (evaluator)
            {
                // Position was not set by user. Use evaluator for automatic computation.
                MotionInstance* motionInstance = uniqueData.m_motionInfos[uniqueDataMotionIndex].m_motionInstance;
                uniqueData.m_motionCoordinates[uniqueDataMotionIndex] = evaluator->ComputeParamValue(*motionInstance);
            }

            ++uniqueDataMotionIndex;
        }
    }

    void BlendSpace1DNode::SetCurrentPosition(float point)
    {
        m_currentPositionSetInteractively = point;
    }

    void BlendSpace1DNode::ComputeMotionPosition(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        AZ_Assert(uniqueData, "Unique data not found for blend space 1D node '%s'.", GetName());

        const uint32 attributeIndex = FindBlendSpaceMotionAttributeIndexByMotionId(ATTRIB_MOTIONS, motionId);
        if (attributeIndex == MCORE_INVALIDINDEX32)
        {
            AZ_Assert(false, "Can't find blend space motion attribute for motion id '%s'.", motionId.c_str());
            return;
        }

        // Get the motion parameter evaluator.
        BlendSpaceParamEvaluator* evaluator = nullptr;
        const ECalculationMethod calculationMethod = GetBlendSpaceCalculationMethod(ATTRIB_CALCULATION_METHOD);
        if (calculationMethod == ECalculationMethod::AUTO)
        {
            const size_t evaluatorIndex = static_cast<size_t>(GetAttributeFloatAsUint32(ATTRIB_EVALUATOR));
            const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();
            evaluator = blendSpaceManager->GetParameterEvaluator(evaluatorIndex);
            if (evaluator && evaluator->IsNullEvaluator())
            {
                // "Null evaluator" is really not an evaluator.
                evaluator = nullptr;
            }
        }

        if (!evaluator)
        {
            position = AZ::Vector2::CreateZero();
            return;
        }
        
        // If the motion is invalid, we dont have anything to update.
        MCore::AttributeArray* motionsAttribute = GetMotionAttributeArray();
        AttributeBlendSpaceMotion* motionAttribute = static_cast<AttributeBlendSpaceMotion*>(motionsAttribute->GetAttribute(attributeIndex));
        if (motionAttribute->TestFlag(AttributeBlendSpaceMotion::TypeFlags::InvalidMotion))
        {
            return;
        }

        // Compute the unique data motion index by skipping those motions from the attribute that are invalid
        uint32 uniqueDataMotionIndex = 0;
        for (uint32 i = 0; i < attributeIndex; ++i)
        {
            AttributeBlendSpaceMotion* theMotionAttribute = static_cast<AttributeBlendSpaceMotion*>(motionsAttribute->GetAttribute(i));
            if (theMotionAttribute->TestFlag(AttributeBlendSpaceMotion::TypeFlags::InvalidMotion))
            {
                continue;
            }
            else
            {
                ++uniqueDataMotionIndex;
            }
        }

        AZ_Assert(uniqueDataMotionIndex < uniqueData->m_motionInfos.size(), "Invalid amount of motion infos in unique data");
        MotionInstance* motionInstance = uniqueData->m_motionInfos[uniqueDataMotionIndex].m_motionInstance;
        position.SetX(evaluator->ComputeParamValue(*motionInstance));
        position.SetY(0.0f);
    }


    void BlendSpace1DNode::RestoreMotionCoords(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance)
    {
        const ECalculationMethod calculationMethodX = GetBlendSpaceCalculationMethod(ATTRIB_CALCULATION_METHOD);

        AZ::Vector2 computedMotionCoords;
        ComputeMotionPosition(motionId, animGraphInstance, computedMotionCoords);

        const uint32 attributeIndex = FindBlendSpaceMotionAttributeIndexByMotionId(ATTRIB_MOTIONS, motionId);
        if (attributeIndex == MCORE_INVALIDINDEX32)
        {
            AZ_Assert(false, "Can't find blend space motion attribute for motion id '%s'.", motionId.c_str());
            return;
        }

        MCore::AttributeArray* motionsAttribute = GetMotionAttributeArray();
        AttributeBlendSpaceMotion* motionAttribute = static_cast<AttributeBlendSpaceMotion*>(motionsAttribute->GetAttribute(attributeIndex));

        // Reset the motion coordinates in case the user manually set the value and we're in automatic mode.
        if (!motionAttribute->IsXCoordinateSetByUser() && calculationMethodX == ECalculationMethod::AUTO)
        {
            motionAttribute->SetXCoordinate(computedMotionCoords.GetX());
            motionAttribute->MarkXCoordinateSetByUser(false);
        }
    }


    MCore::AttributeArray* BlendSpace1DNode::GetMotionAttributeArray() const
    {
        return GetAttributeArray(ATTRIB_MOTIONS);
    }

    void BlendSpace1DNode::SortMotionInstances(UniqueData& uniqueData)
    {
        const AZ::u16 numMotions = aznumeric_caster(uniqueData.m_motionCoordinates.size());
        uniqueData.m_sortedMotions.resize(numMotions);
        for (AZ::u16 i = 0; i < numMotions; ++i)
        {
            uniqueData.m_sortedMotions[i] = i;
        }
        MotionSortComparer comparer(uniqueData.m_motionCoordinates);
        AZStd::sort(uniqueData.m_sortedMotions.begin(), uniqueData.m_sortedMotions.end(), comparer);

        // Detect if we have coordinates overlapping
        uniqueData.m_hasOverlappingCoordinates = false;
        for (AZ::u32 i = 1; i < numMotions; ++i) 
        {
            const AZ::u16 motionA = uniqueData.m_sortedMotions[i - 1];
            const AZ::u16 motionB = uniqueData.m_sortedMotions[i];
            if (AZ::IsClose(uniqueData.m_motionCoordinates[motionA],
                            uniqueData.m_motionCoordinates[motionB],
                            0.0001f))
            {
                uniqueData.m_hasOverlappingCoordinates = true;
                break;
            }
        }
    }

    float BlendSpace1DNode::GetCurrentSamplePosition(AnimGraphInstance* animGraphInstance, const UniqueData& uniqueData)
    {
        if (IsInInteractiveMode())
        {
            return m_currentPositionSetInteractively;
        }
        else
        {
            EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(INPUTPORT_VALUE).mConnection;

#ifdef EMFX_EMSTUDIOBUILD
            // We do require the user to make connections into the value port.
            SetHasError(animGraphInstance, (paramConnection == nullptr));
#endif

            float samplePoint;
            if (paramConnection)
            {
                samplePoint = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_VALUE);
            }
            else
            {
                // Nothing connected to input port. Just return the middle of the parameter range as a default choice.
                samplePoint = (uniqueData.GetRangeMin() + uniqueData.GetRangeMax()) / 2;
            }

            return samplePoint;
        }
    }

    void BlendSpace1DNode::UpdateBlendingInfoForCurrentPoint(UniqueData& uniqueData)
    {
        uniqueData.m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;
        FindLineSegmentForCurrentPoint(uniqueData);

        uniqueData.m_blendInfos.clear();

        if (uniqueData.m_currentSegment.m_segmentIndex != MCORE_INVALIDINDEX32)
        {
            const AZ::u32 segIndex = uniqueData.m_currentSegment.m_segmentIndex;
            uniqueData.m_blendInfos.resize(2);
            for (int i = 0; i < 2; ++i)
            {
                BlendInfo& blendInfo = uniqueData.m_blendInfos[i];
                blendInfo.m_motionIndex = uniqueData.m_sortedMotions[segIndex + i];
                blendInfo.m_weight = (i == 0) ? (1 - uniqueData.m_currentSegment.m_weightForSegmentEnd) : uniqueData.m_currentSegment.m_weightForSegmentEnd;
            }
        }
        else if (!uniqueData.m_motionInfos.empty())
        {
            uniqueData.m_blendInfos.resize(1);
            BlendInfo& blendInfo = uniqueData.m_blendInfos[0];
            blendInfo.m_motionIndex = (uniqueData.m_currentPosition < uniqueData.GetRangeMin()) ? uniqueData.m_sortedMotions.front() : uniqueData.m_sortedMotions.back();
            blendInfo.m_weight = 1.0f;
        }

        AZStd::sort(uniqueData.m_blendInfos.begin(), uniqueData.m_blendInfos.end());
    }

    bool BlendSpace1DNode::FindLineSegmentForCurrentPoint(UniqueData& uniqueData)
    {
        const AZ::u32 numPoints = (AZ::u32)uniqueData.m_sortedMotions.size();
        if ((numPoints < 2) || (uniqueData.m_currentPosition < uniqueData.GetRangeMin()) || (uniqueData.m_currentPosition > uniqueData.GetRangeMax()))
        {
            uniqueData.m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;
            return false;
        }
        for (AZ::u32 i = 1; i < numPoints; ++i)
        {
            const float segStart = uniqueData.m_motionCoordinates[uniqueData.m_sortedMotions[i - 1]];
            const float segEnd = uniqueData.m_motionCoordinates[uniqueData.m_sortedMotions[i]];
            AZ_Assert(segStart <= segEnd, "The values should have been sorted");
            if ((uniqueData.m_currentPosition >= segStart) && (uniqueData.m_currentPosition <= segEnd))
            {
                uniqueData.m_currentSegment.m_segmentIndex = i - 1;
                const float segLength = segEnd - segStart;
                if (segLength <= 0)
                {
                    uniqueData.m_currentSegment.m_weightForSegmentEnd = 0;
                }
                else
                {
                    uniqueData.m_currentSegment.m_weightForSegmentEnd = (uniqueData.m_currentPosition - segStart) / segLength;
                }
                return true;
            }
        }
        uniqueData.m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;
        return false;
    }

    void BlendSpace1DNode::SetBindPoseAtOutput(AnimGraphInstance* animGraphInstance)
    {
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        outputPose->InitFromBindPose(actorInstance);
    }

    void BlendSpace1DNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<BlendSpace1DNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        RewindMotions(uniqueData->m_motionInfos);
    }

} // namespace EMotionFX

