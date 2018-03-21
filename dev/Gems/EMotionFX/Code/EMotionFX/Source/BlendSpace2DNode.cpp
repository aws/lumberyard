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

#include "BlendSpace2DNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "BlendSpaceManager.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraph.h"
#include "MotionSet.h"
#include "EMotionFXManager.h"
#include <MCore/Source/DelaunayTriangulator.h>
#include <MCore/Source/AttributeSettings.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/algorithm.h>

namespace
{
    // Dimensions of the 2D grid into which we place the triangles for quick lookup
    const uint32_t kGridCellCountX = 10;
    const uint32_t kGridCellCountY = 10;

    AZ_FORCE_INLINE void GetBoundsOfTriangle(const AZ::Vector2 triVerts[3],
        float& minX, float& minY, float& maxX, float& maxY)
    {
        minX = AZStd::min(triVerts[0].GetX(), AZStd::min(triVerts[1].GetX(), triVerts[2].GetX()));
        minY = AZStd::min(triVerts[0].GetY(), AZStd::min(triVerts[1].GetY(), triVerts[2].GetY()));
        maxX = AZStd::max(triVerts[0].GetX(), AZStd::max(triVerts[1].GetX(), triVerts[2].GetX()));
        maxY = AZStd::max(triVerts[0].GetY(), AZStd::max(triVerts[1].GetY(), triVerts[2].GetY()));
    }

    AZ_FORCE_INLINE bool IsDegenerateTriangle(const AZ::Vector2& p0, const AZ::Vector2& p1, const AZ::Vector2& p2)
    {
        const AZ::Vector2 v01(p1 - p0);
        const AZ::Vector2 v02(p2 - p0);
        const float perpProduct = v01.GetX() * v02.GetY() - v01.GetY() * v02.GetX();
        return (::fabsf(perpProduct) < 0.001f);
    }

    AZ_FORCE_INLINE bool IsPointInTriangle(const AZ::Vector2& a, const AZ::Vector2& b,
        const AZ::Vector2& c, const AZ::Vector2& p, float& v, float& w, float epsilon)
    {
        const AZ::Vector2 v0(b - a);
        const AZ::Vector2 v1(c - a);
        const AZ::Vector2 v2(p - a);

        const float dot00 = v0.Dot(v0);
        const float dot01 = v0.Dot(v1);
        const float dot02 = v0.Dot(v2);
        const float dot11 = v1.Dot(v1);
        const float dot12 = v1.Dot(v2);

        const float denom = (dot00 * dot11 - dot01 * dot01);
        if (denom < MCore::Math::epsilon)
        {
            return false;
        }

        // Compute barycentric coordinates.
        const float invDenom = 1 / denom;
        v = (dot11 * dot02 - dot01 * dot12) * invDenom;
        w = (dot00 * dot12 - dot01 * dot02) * invDenom;

        if ((v < 0) && (v > -epsilon))
        {
            v = 0;
        }
        if ((w < 0) && (w > -epsilon))
        {
            w = 0;
        }

        return (v >= 0) && (w >= 0) && (v + w < 1 + epsilon);
    }

    AZ::Vector2 GetClosestPointOnLineSegment(const AZ::Vector2& segStart, const AZ::Vector2& segEnd, const AZ::Vector2& pt, float& u)
    {
        const AZ::Vector2 segVec(segEnd - segStart);
        const AZ::Vector2 vec(pt - segStart);

        const float d1 = segVec.Dot(vec);
        if (d1 <= 0)
        {
            u = 0;
            return segStart;
        }
        const float segLenSqr = segVec.Dot(segVec);
        if (segLenSqr <= d1)
        {
            u = 1.0f;
            return segEnd;
        }
        u = d1 / segLenSqr;
        return segStart + u * segVec;
    }
}

namespace EMotionFX
{
    const float BlendSpace2DNode::s_epsilonForBarycentricCoords = 0.001f;

    BlendSpace2DNode::Triangle::Triangle(uint16_t indexA, uint16_t indexB, uint16_t indexC)
    {
        m_vertIndices[0] = indexA;
        m_vertIndices[1] = indexB;
        m_vertIndices[2] = indexC;
    }

    BlendSpace2DNode::UniqueData::~UniqueData()
    {
        BlendSpaceNode::ClearMotionInfos(m_motionInfos);
    }

    BlendSpace2DNode* BlendSpace2DNode::Create(AnimGraph* animGraph)
    {
        return new BlendSpace2DNode(animGraph);
    }

    bool BlendSpace2DNode::GetValidCalculationMethodsAndEvaluators() const
    {
        const ECalculationMethod calculationMethodX = GetBlendSpaceCalculationMethod(ATTRIB_CALCULATION_METHOD_X);
        const ECalculationMethod calculationMethodY = GetBlendSpaceCalculationMethod(ATTRIB_CALCULATION_METHOD_Y);

        // If both calculation methods are manual, we have valid blend space param evaluators
        if (calculationMethodX == ECalculationMethod::MANUAL
            && calculationMethodY == ECalculationMethod::MANUAL)
        {
            return true;
        }
        else
        {
            const BlendSpaceParamEvaluator* evaluatorX = GetBlendSpaceParamEvaluator(ATTRIB_EVALUATOR_X);
            const BlendSpaceParamEvaluator* evaluatorY = GetBlendSpaceParamEvaluator(ATTRIB_EVALUATOR_Y);

            AZ_Assert(calculationMethodX == ECalculationMethod::MANUAL || evaluatorX, "Expected non-null blend space param evaluator for X-Axis with auto calculation method");
            AZ_Assert(calculationMethodY == ECalculationMethod::MANUAL || evaluatorY, "Expected non-null blend space param evaluator for Y-Axis with auto calculation method");

            if ((calculationMethodX == ECalculationMethod::AUTO && evaluatorX->IsNullEvaluator())
                || (calculationMethodY == ECalculationMethod::AUTO && evaluatorY->IsNullEvaluator()))
            {
                // If any of the calculation methods is auto and it doesnt have an evaluator, then is invalid
                return false;
            }
            else if (evaluatorX == evaluatorY)
            {
                // if both evaluators are the same, then it is invalid
                return false;
            }
            else
            {
                return true;
            }
        }
    }

    const char* BlendSpace2DNode::GetAxisLabel(int axisIndex) const
    {
        AZ_Assert((axisIndex == 0) || (axisIndex == 1), "Invalid axis index");

        const uint32 attribIndex = (axisIndex == 0) ? ATTRIB_EVALUATOR_X : ATTRIB_EVALUATOR_Y;
        BlendSpaceParamEvaluator* evaluator = GetBlendSpaceParamEvaluator(attribIndex);
        if (!evaluator || evaluator->IsNullEvaluator())
        {
            return (axisIndex == 0) ? "X-Axis" : "Y-Axis";
        }
        return evaluator->GetName();
    }

    void BlendSpace2DNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // Find the unique data for this node, if it doesn't exist yet, create it.
        UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (!uniqueData)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        UpdateMotionInfos(animGraphInstance);
    }

    void BlendSpace2DNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
    }

    void BlendSpace2DNode::RegisterPorts()
    {
        InitInputPorts(2);
        SetupInputPortAsNumber("X", INPUTPORT_XVALUE, PORTID_INPUT_XVALUE);
        SetupInputPortAsNumber("Y", INPUTPORT_YVALUE, PORTID_INPUT_YVALUE);

        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    void BlendSpace2DNode::RegisterAttributes()
    {
        RegisterCalculationMethodAttribute("Calculation method (X-Axis)", "calculationMethodX", "Calculation method for the X Axis");
        RegisterBlendSpaceEvaluatorAttribute("X-Axis Evaluator", "evaluatorX", "Evaluator for the X axis value of motions");
        RegisterCalculationMethodAttribute("Calculation method (Y-Axis)", "calculationMethodY", "Calculation method for the Y Axis");
        RegisterBlendSpaceEvaluatorAttribute("Y-Axis Evaluator", "evaluatorY", "Evaluator for the Y axis value of motions");
        RegisterSyncAttribute();
        RegisterMasterMotionAttribute();
        RegisterBlendSpaceEventFilterAttribute();

        MCore::AttributeSettings* attribute = RegisterAttribute("Motions", "motions", "Source motions for blend space", ATTRIBUTE_INTERFACETYPE_BLENDSPACEMOTIONS);
        attribute->SetReinitGuiOnValueChange(true);
        attribute->SetDefaultValue(MCore::AttributeArray::Create(AttributeBlendSpaceMotion::TYPE_ID));
    }

    const char* BlendSpace2DNode::GetTypeString() const
    {
        return "BlendSpace2DNode";
    }

    const char* BlendSpace2DNode::GetPaletteName() const
    {
        return "Blend Space 2D";
    }

    AnimGraphObject::ECategory BlendSpace2DNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }

    // This implementation currently just does what the other node classes in the
    // EMFX code base are doing. This is not a proper implementation of
    // "Clone" since "CopyBaseObjectTo" currently copies only the AnimGraphObject
    AnimGraphObject* BlendSpace2DNode::Clone(AnimGraph* animGraph)
    {
        BlendSpace2DNode* clone = new BlendSpace2DNode(animGraph);

        CopyBaseObjectTo(clone);

        return clone;
    }

    AnimGraphObjectData* BlendSpace2DNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }

    // Called when the attributes change.
    void BlendSpace2DNode::OnUpdateAttributes()
    {
        if (mAttributeValues.GetIsEmpty())
        {
            return;
        }

        // Mark AttributeBlendSpaceMotion attributes as belonging to 2D blend space
        MCore::AttributeArray* attributeArray = GetMotionAttributeArray();
        if (attributeArray)
        {
            const uint32 numMotions = attributeArray->GetNumAttributes();
            for (uint32 i = 0; i < numMotions; ++i)
            {
                AttributeBlendSpaceMotion* attrib = static_cast<AttributeBlendSpaceMotion*>(attributeArray->GetAttribute(i));
                attrib->SetDimension(2);
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
        const ECalculationMethod calculationMethodX = static_cast<ECalculationMethod>(static_cast<int>(GetAttributeFloat(ATTRIB_CALCULATION_METHOD_X)->GetValue()));
        if (calculationMethodX == ECalculationMethod::MANUAL)
        {
            SetAttributeDisabled(ATTRIB_EVALUATOR_X);
            GetAttributeFloat(ATTRIB_EVALUATOR_X)->SetValue(0.0f);
        }
        else // Automatic calculation method
        {
            SetAttributeEnabled(ATTRIB_EVALUATOR_X);
        }

        const ECalculationMethod calculationMethodY = static_cast<ECalculationMethod>(static_cast<int>(GetAttributeFloat(ATTRIB_CALCULATION_METHOD_Y)->GetValue()));
        if (calculationMethodY == ECalculationMethod::MANUAL)
        {
            SetAttributeDisabled(ATTRIB_EVALUATOR_Y);
            GetAttributeFloat(ATTRIB_EVALUATOR_Y)->SetValue(0.0f);
        }
        else // Automatic calculation method
        {
            SetAttributeEnabled(ATTRIB_EVALUATOR_Y);
        }
#endif
    }

    void BlendSpace2DNode::Output(AnimGraphInstance* animGraphInstance)
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

        if (uniqueData->m_currentTriangle.m_triangleIndex != MCORE_INVALIDINDEX32)
        {
            const Triangle& triangle = uniqueData->m_triangles[uniqueData->m_currentTriangle.m_triangleIndex];
            for (int i = 0; i < 3; ++i)
            {
                MotionInstance* motionInstance = uniqueData->m_motionInfos[triangle.m_vertIndices[i]].m_motionInstance;
                motionOutPose->InitFromBindPose(actorInstance);
                motionInstance->GetMotion()->Update(&bindPose->GetPose(), &motionOutLocalPose, motionInstance);

                if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled())
                {
                    motionOutLocalPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
                }

                outputLocalPose.Sum(&motionOutLocalPose, uniqueData->m_currentTriangle.m_weights[i]);
            }
        }
        else if (uniqueData->m_currentEdge.m_edgeIndex != MCORE_INVALIDINDEX32)
        {
            const Edge& edge = uniqueData->m_outerEdges[uniqueData->m_currentEdge.m_edgeIndex];
            for (int i = 0; i < 2; ++i)
            {
                MotionInstance* motionInstance = uniqueData->m_motionInfos[edge.m_vertIndices[i]].m_motionInstance;
                motionOutPose->InitFromBindPose(actorInstance);
                motionInstance->GetMotion()->Update(&bindPose->GetPose(), &motionOutLocalPose, motionInstance);

                if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled())
                {
                    motionOutLocalPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
                }

                const float weight = (i == 0) ? (1.0f - uniqueData->m_currentEdge.m_u) : uniqueData->m_currentEdge.m_u;
                outputLocalPose.Sum(&motionOutLocalPose, weight);
            }
        }
        else
        {
            SetBindPoseAtOutput(animGraphInstance);
        }

        outputLocalPose.NormalizeQuaternions();

        posePool.FreePose(motionOutPose);
        posePool.FreePose(bindPose);


#ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
#endif
    }

    void BlendSpace2DNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (mDisabled)
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        const ESyncMode syncMode = (ESyncMode)((uint32)GetAttributeFloat(ATTRIB_SYNC)->GetValue());
        DoTopDownUpdate(animGraphInstance, syncMode, uniqueData->m_masterMotionIdx,
            uniqueData->m_motionInfos, uniqueData->m_allMotionsHaveSyncTracks);

        for (int i = 0; i < 2; ++i)
        {
            const uint32 portIdx = (i == 0) ? INPUTPORT_XVALUE : INPUTPORT_YVALUE;
            EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(portIdx).mConnection;
            if (paramConnection)
            {
                AnimGraphNode* paramSrcNode = paramConnection->GetSourceNode();
                if (paramSrcNode)
                {
                    paramSrcNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
                }
            }
        }
    }

    void BlendSpace2DNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (!mDisabled)
        {
            EMotionFX::BlendTreeConnection* param1Connection = GetInputPort(INPUTPORT_XVALUE).mConnection;
            if (param1Connection)
            {
                UpdateIncomingNode(animGraphInstance, param1Connection->GetSourceNode(), timePassedInSeconds);
            }
            EMotionFX::BlendTreeConnection* param2Connection = GetInputPort(INPUTPORT_YVALUE).mConnection;
            if (param2Connection)
            {
                UpdateIncomingNode(animGraphInstance, param2Connection->GetSourceNode(), timePassedInSeconds);
            }
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        AZ_Assert(uniqueData, "Unique data not found for blend space 2D node '%s'.", GetName());
        uniqueData->Clear();

        if (mDisabled)
        {
            return;
        }

        uniqueData->m_currentPosition = GetCurrentSamplePosition(animGraphInstance, *uniqueData);
        uniqueData->m_normCurrentPosition = uniqueData->ConvertToNormalizedSpace(uniqueData->m_currentPosition);

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

    void BlendSpace2DNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
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

        EMotionFX::BlendTreeConnection* param1Connection = GetInputPort(INPUTPORT_XVALUE).mConnection;
        if (param1Connection)
        {
            param1Connection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }
        EMotionFX::BlendTreeConnection* param2Connection = GetInputPort(INPUTPORT_YVALUE).mConnection;
        if (param2Connection)
        {
            param2Connection->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
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

    BlendSpace2DNode::BlendSpace2DNode(AnimGraph* animGraph)
        : BlendSpaceNode(animGraph, nullptr, TYPE_ID)
        , m_currentPositionSetInteractively(false)
    {
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }

    BlendSpace2DNode::~BlendSpace2DNode()
    {
    }

    bool BlendSpace2DNode::UpdateMotionInfos(AnimGraphInstance* animGraphInstance)
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

        ComputeNormalizationInfo(*uniqueData);
        const size_t numPoints = uniqueData->m_motionCoordinates.size();
        uniqueData->m_normMotionPositions.resize(numPoints);
        for (size_t i = 0; i < numPoints; ++i)
        {
            uniqueData->m_normMotionPositions[i] = uniqueData->ConvertToNormalizedSpace(uniqueData->m_motionCoordinates[i]);
        }
        UpdateTriangulation(*uniqueData);
        uniqueData->m_currentTriangle.m_triangleIndex = MCORE_INVALIDINDEX32;
        uniqueData->m_currentEdge.m_edgeIndex = MCORE_INVALIDINDEX32;

        return true;
    }

    void BlendSpace2DNode::SetCurrentPosition(const AZ::Vector2& point)
    {
        m_currentPositionSetInteractively = point;
    }

    void BlendSpace2DNode::ComputeMotionPosition(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        AZ_Assert(uniqueData, "Unique data not found for blend space 2D node '%s'.", GetName());

        const uint32 attributeIndex = FindBlendSpaceMotionAttributeIndexByMotionId(ATTRIB_MOTIONS, motionId);
        if (attributeIndex == MCORE_INVALIDINDEX32)
        {
            AZ_Assert(false, "Can't find blend space motion attribute for motion id '%s'.", motionId.c_str());
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
        const MotionInstance* motionInstance = uniqueData->m_motionInfos[uniqueDataMotionIndex].m_motionInstance;
        const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();

        position = AZ::Vector2::CreateZero();
        
        for (int i = 0; i < 2; ++i)
        {
            const uint32 calcMethodAttrib = (i == 0) ? ATTRIB_CALCULATION_METHOD_X : ATTRIB_CALCULATION_METHOD_Y;
            const ECalculationMethod calculationMethod = static_cast<ECalculationMethod>(static_cast<int>(GetAttributeFloat(calcMethodAttrib)->GetValue()));
            if (calculationMethod == ECalculationMethod::AUTO)
            {
                const uint32 evaluatorAttrib = (i == 0) ? ATTRIB_EVALUATOR_X : ATTRIB_EVALUATOR_Y;
                const size_t evaluatorIndex = static_cast<size_t>(GetAttributeFloatAsUint32(evaluatorAttrib));
                BlendSpaceParamEvaluator* evaluator = blendSpaceManager->GetParameterEvaluator(evaluatorIndex);
                if (evaluator && !evaluator->IsNullEvaluator())
                {
                    position.SetElement(i, evaluator->ComputeParamValue(*motionInstance));
                }
            }
        }
    }


    void BlendSpace2DNode::RestoreMotionCoords(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance)
    {
        const ECalculationMethod calculationMethodX = static_cast<ECalculationMethod>(static_cast<int>(GetAttributeFloat(ATTRIB_CALCULATION_METHOD_X)->GetValue()));
        const ECalculationMethod calculationMethodY = static_cast<ECalculationMethod>(static_cast<int>(GetAttributeFloat(ATTRIB_CALCULATION_METHOD_Y)->GetValue()));

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
        if (calculationMethodX == ECalculationMethod::AUTO)
        {
            motionAttribute->SetXCoordinate(computedMotionCoords.GetX());
            motionAttribute->MarkXCoordinateSetByUser(false);
        }

        if (calculationMethodY == ECalculationMethod::AUTO)
        {
            motionAttribute->SetYCoordinate(computedMotionCoords.GetY());
            motionAttribute->MarkYCoordinateSetByUser(false);
        }
    }


    MCore::AttributeArray* BlendSpace2DNode::GetMotionAttributeArray() const
    {
        return GetAttributeArray(ATTRIB_MOTIONS);
    }

    void BlendSpace2DNode::UpdateMotionPositions(UniqueData& uniqueData)
    {
        const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();

        // Get the motion parameter evaluators.
        BlendSpaceParamEvaluator* evaluatorX = nullptr;
        BlendSpaceParamEvaluator* evaluatorY = nullptr;

        const ECalculationMethod calculationMethodX = static_cast<ECalculationMethod>(static_cast<int>(GetAttributeFloat(ATTRIB_CALCULATION_METHOD_X)->GetValue()));
        if (calculationMethodX == ECalculationMethod::AUTO)
        {
            const size_t evaluatorIndex = static_cast<size_t>(GetAttributeFloatAsUint32(ATTRIB_EVALUATOR_X));
            evaluatorX = blendSpaceManager->GetParameterEvaluator(evaluatorIndex);
            if (evaluatorX && evaluatorX->IsNullEvaluator())
            {
                // "Null evaluator" is really not an evaluator.
                evaluatorX = nullptr;
            }
        }

        const ECalculationMethod calculationMethodY = static_cast<ECalculationMethod>(static_cast<int>(GetAttributeFloat(ATTRIB_CALCULATION_METHOD_Y)->GetValue()));
        if (calculationMethodY == ECalculationMethod::AUTO)
        {
            const size_t evaluatorIndex = static_cast<size_t>(GetAttributeFloatAsUint32(ATTRIB_EVALUATOR_Y));
            evaluatorY = blendSpaceManager->GetParameterEvaluator(evaluatorIndex);
            if (evaluatorY && evaluatorY->IsNullEvaluator())
            {
                // "Null evaluator" is really not an evaluator.
                evaluatorY = nullptr;
            }
        }

        const MCore::AttributeArray* attributeArray = GetMotionAttributeArray();
        // the motions in the attributes could not match the ones in the unique data. The attribute could have some invalid motions
        const uint32 attributeMotionCount = attributeArray->GetNumAttributes();
        const size_t uniqueDataMotionCount = uniqueData.m_motionInfos.size();

        // Iterate through all motions and calculate their location in the blend space.
        uniqueData.m_motionCoordinates.resize(uniqueDataMotionCount);
        size_t iUniqueDataMotionIndex = 0;
        for (uint32 iAttributeMotionIndex = 0; iAttributeMotionIndex < attributeMotionCount; ++iAttributeMotionIndex)
        {
            const AttributeBlendSpaceMotion* attribute = static_cast<AttributeBlendSpaceMotion*>(attributeArray->GetAttribute(iAttributeMotionIndex));
            if (attribute->TestFlag(AttributeBlendSpaceMotion::TypeFlags::InvalidMotion))
            {
                continue;
            }

            MotionInstance* motionInstance = uniqueData.m_motionInfos[iUniqueDataMotionIndex].m_motionInstance;
            AZ::Vector2& point = uniqueData.m_motionCoordinates[iUniqueDataMotionIndex];

            // X
            // Did the user set the x coordinate manually? If so, use the shared value from the attribute.
            if (attribute->IsXCoordinateSetByUser() || !evaluatorX)
            {
                point.SetX(attribute->GetXCoordinate());
            }
            else
            {
                // Motion x coordinate was not set by user. Use evaluator for automatic computation.
                const float computedXCoord = evaluatorX->ComputeParamValue(*motionInstance);
                point.SetX(computedXCoord);
            }

            // Y
            // Did the user set the y coordinate manually? If so, use the shared value from the attribute.
            if (attribute->IsYCoordinateSetByUser() || !evaluatorY)
            {
                point.SetY(attribute->GetYCoordinate());
            }
            else
            {
                // Motion y coordinate was not set by user. Use evaluator for automatic computation.
                const float computedYCoord = evaluatorY->ComputeParamValue(*motionInstance);
                point.SetY(computedYCoord);
            }

            ++iUniqueDataMotionIndex;
        }
    }

    void BlendSpace2DNode::ComputeNormalizationInfo(UniqueData& uniqueData)
    {
        float minX, minY, maxX, maxY;

        minX = minY = FLT_MAX;
        maxX = maxY = -FLT_MAX;
        for (const AZ::Vector2& point : uniqueData.m_motionCoordinates)
        {
            if (point.GetX() < minX)
            {
                minX = point.GetX();
            }
            if (point.GetY() < minY)
            {
                minY = point.GetY();
            }
            if (point.GetX() > maxX)
            {
                maxX = point.GetX();
            }
            if (point.GetY() > maxY)
            {
                maxY = point.GetY();
            }
        }
        uniqueData.m_rangeMin.Set(minX, minY);
        uniqueData.m_rangeMax.Set(maxX, maxY);
        uniqueData.m_rangeCenter = (uniqueData.m_rangeMin + uniqueData.m_rangeMax) / 2;

        for (int i = 0; i < 2; ++i)
        {
            float scale;
            if (uniqueData.m_rangeMax(i) <= uniqueData.m_rangeMin(i))
            {
                scale = 1.0f;
            }
            else
            {
                scale = 1.0f / (uniqueData.m_rangeMax(i) - uniqueData.m_rangeMin(i));
            }
            uniqueData.m_normalizationScale.SetElement(i, 2 * scale);// multiplying by 2 because the target range is 2 (-1 to 1)
        }
    }

    void BlendSpace2DNode::UpdateTriangulation(UniqueData& uniqueData)
    {
        if (uniqueData.m_normMotionPositions.empty())
        {
            uniqueData.m_triangles.clear();
            uniqueData.m_outerEdges.clear();
        }
        else
        {
            MCore::DelaunayTriangulator triangulator;

            const MCore::DelaunayTriangulator::Triangles& triangles =
                triangulator.Triangulate(uniqueData.m_normMotionPositions);

            const size_t numTriangles = triangles.size();
            AZ_Assert(numTriangles < UINT16_MAX, "More triangles than our 16 bit indices can handle");

            uniqueData.m_triangles.clear();
            uniqueData.m_triangles.reserve(numTriangles);

            // detect degenerate triangles
            uniqueData.m_hasDegenerateTriangles = false;
            for (size_t i = 0; i < numTriangles; ++i)
            {
                const MCore::DelaunayTriangulator::Triangle& srcTri = triangles[i];
                const AZStd::vector<AZ::Vector2>& normPositions = uniqueData.m_normMotionPositions;

                uniqueData.m_hasDegenerateTriangles |= IsDegenerateTriangle(normPositions[srcTri.VertIndex(0)],
                        normPositions[srcTri.VertIndex(1)],
                        normPositions[srcTri.VertIndex(2)]);

                uniqueData.m_triangles.emplace_back(Triangle(static_cast<uint16_t>(srcTri.VertIndex(0)),
                        static_cast<uint16_t>(srcTri.VertIndex(1)),
                        static_cast<uint16_t>(srcTri.VertIndex(2))));
            }
            DetermineOuterEdges(uniqueData);
        }
        //UpdateTriangleGrid(uniqueData);
    }

    // Determines the outer (i.e., boundary) edges of the triangulated region.
    // To do this, we make use of the fact that the inner edges are shared between
    // two triangles while the outer edges are not shared.
    //
    void BlendSpace2DNode::DetermineOuterEdges(UniqueData& uniqueData)
    {
        uniqueData.m_outerEdges.clear();

        AZStd::unordered_map<Edge, AZ::u32, EdgeHasher> edgeToCountMap;
        for (const Triangle& tri : uniqueData.m_triangles)
        {
            for (int i = 0; i < 3; ++i)
            {
                int j = (i + 1) % 3;
                Edge edge;
                edge.m_vertIndices[0] = tri.m_vertIndices[i];
                edge.m_vertIndices[1] = tri.m_vertIndices[j];
                if (edge.m_vertIndices[0] > edge.m_vertIndices[1])
                {
                    AZStd::swap(edge.m_vertIndices[0], edge.m_vertIndices[1]);
                }
                auto mapIt = edgeToCountMap.find(edge);
                if (mapIt == edgeToCountMap.end())
                {
                    edgeToCountMap[edge] = 1;
                }
                else
                {
                    mapIt->second++;
                }
            }
        }
        for (auto& mapEntry : edgeToCountMap)
        {
            const AZ::u32 shareCount = mapEntry.second;
            AZ_Assert((shareCount == 1) || (shareCount == 2), "Edges should be shared by at most 2 triangles");
            if (shareCount == 1)
            {
                uniqueData.m_outerEdges.push_back(mapEntry.first);
            }
        }
    }

    AZ::Vector2 BlendSpace2DNode::GetCurrentSamplePosition(AnimGraphInstance* animGraphInstance, const UniqueData& uniqueData)
    {
        if (IsInInteractiveMode())
        {
            return m_currentPositionSetInteractively;
        }
        else
        {
            AZ::Vector2 samplePoint;

            EMotionFX::BlendTreeConnection* inputConnectionX = GetInputPort(INPUTPORT_XVALUE).mConnection;
            EMotionFX::BlendTreeConnection* inputConnectionY = GetInputPort(INPUTPORT_YVALUE).mConnection;
#ifdef EMFX_EMSTUDIOBUILD
            if (inputConnectionX && inputConnectionY)
            {
                SetHasError(animGraphInstance, false);
            }
            else
            {
                // We do require the user to make connections into the value ports.
                SetHasError(animGraphInstance, true);
            }
#endif

            if (inputConnectionX)
            {
                samplePoint.SetX(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_XVALUE));
            }
            else
            {
                // Nothing connected to input port. Just set the middle of the range as a default choice.
                const float value = (uniqueData.m_rangeMin(0) + uniqueData.m_rangeMax(0)) / 2;
                samplePoint.SetX(value);
            }


            if (inputConnectionY)
            {
                samplePoint.SetY(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_YVALUE));
            }
            else
            {
                // Nothing connected to input port. Just set the middle of the range
                // as a default choice.
                const float value = (uniqueData.m_rangeMin(1) + uniqueData.m_rangeMax(1)) / 2;
                samplePoint.SetY(value);
            }

            return samplePoint;
        }
    }

    void BlendSpace2DNode::UpdateBlendingInfoForCurrentPoint(UniqueData& uniqueData)
    {
        uniqueData.m_currentTriangle.m_triangleIndex = MCORE_INVALIDINDEX32;
        uniqueData.m_currentEdge.m_edgeIndex = MCORE_INVALIDINDEX32;

        if (!FindTriangleForCurrentPoint(uniqueData))
        {
            FindOuterEdgeClosestToCurrentPoint(uniqueData);
        }

        uniqueData.m_blendInfos.clear();

        if (uniqueData.m_currentTriangle.m_triangleIndex != MCORE_INVALIDINDEX32)
        {
            const Triangle& triangle = uniqueData.m_triangles[uniqueData.m_currentTriangle.m_triangleIndex];
            uniqueData.m_blendInfos.resize(3);
            for (int i = 0; i < 3; ++i)
            {
                BlendInfo& blendInfo = uniqueData.m_blendInfos[i];
                blendInfo.m_motionIndex = triangle.m_vertIndices[i];
                blendInfo.m_weight = uniqueData.m_currentTriangle.m_weights[i];
            }
        }
        else if (uniqueData.m_currentEdge.m_edgeIndex != MCORE_INVALIDINDEX32)
        {
            const Edge& edge = uniqueData.m_outerEdges[uniqueData.m_currentEdge.m_edgeIndex];
            uniqueData.m_blendInfos.resize(2);
            for (int i = 0; i < 2; ++i)
            {
                BlendInfo& blendInfo = uniqueData.m_blendInfos[i];
                blendInfo.m_motionIndex = edge.m_vertIndices[i];
                blendInfo.m_weight = (i == 0) ? (1 - uniqueData.m_currentEdge.m_u) : uniqueData.m_currentEdge.m_u;
            }
        }

        AZStd::sort(uniqueData.m_blendInfos.begin(), uniqueData.m_blendInfos.end());
    }

    bool BlendSpace2DNode::FindTriangleForCurrentPoint(UniqueData& uniqueData)
    {
        // As of now, we go over all the triangles. We can speed this up by
        // some spatial organization of the triangles.
        const AZ::u32 numTriangles = (AZ::u32)uniqueData.m_triangles.size();
        for (AZ::u32 i = 0; i < numTriangles; ++i)
        {
            float v, w;
            const uint16_t* triVerts = uniqueData.m_triangles[i].m_vertIndices;
            if (IsPointInTriangle(uniqueData.m_normMotionPositions[triVerts[0]],
                    uniqueData.m_normMotionPositions[triVerts[1]], uniqueData.m_normMotionPositions[triVerts[2]],
                    uniqueData.m_normCurrentPosition,
                    v, w, s_epsilonForBarycentricCoords))
            {
                uniqueData.m_currentTriangle.m_triangleIndex = i;
                uniqueData.m_currentTriangle.m_weights[0] = 1 - (v + w);
                if (uniqueData.m_currentTriangle.m_weights[0] < 0)
                {
                    uniqueData.m_currentTriangle.m_weights[0] = 0;
                }
                uniqueData.m_currentTriangle.m_weights[1] = v;
                uniqueData.m_currentTriangle.m_weights[2] = w;
                return true;
            }
        }
        return false;
    }

    bool BlendSpace2DNode::FindOuterEdgeClosestToCurrentPoint(UniqueData& uniqueData)
    {
        float   minDistSqr = FLT_MAX;
        AZ::u32 closestEdgeIdx = MCORE_INVALIDINDEX32;
        float   uOnClosestEdge;

        const AZ::u32 numEdges = (AZ::u32)uniqueData.m_outerEdges.size();
        for (AZ::u32 i = 0; i < numEdges; ++i)
        {
            const Edge& edge = uniqueData.m_outerEdges[i];
            float u;
            AZ::Vector2 pointOnEdge = GetClosestPointOnLineSegment(uniqueData.m_normMotionPositions[edge.m_vertIndices[0]],
                    uniqueData.m_normMotionPositions[edge.m_vertIndices[1]], uniqueData.m_normCurrentPosition, u);
            float distSqr = pointOnEdge.GetDistanceSq(uniqueData.m_normCurrentPosition);
            if (distSqr < minDistSqr)
            {
                minDistSqr = distSqr;
                closestEdgeIdx = i;
                uOnClosestEdge = u;
            }
        }
        if (closestEdgeIdx != MCORE_INVALIDINDEX32)
        {
            uniqueData.m_currentEdge.m_edgeIndex = closestEdgeIdx;
            uniqueData.m_currentEdge.m_u = uOnClosestEdge;
            return true;
        }
        return false;
    }

    void BlendSpace2DNode::SetBindPoseAtOutput(AnimGraphInstance* animGraphInstance)
    {
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        outputPose->InitFromBindPose(actorInstance);
    }

    void BlendSpace2DNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        RewindMotions(uniqueData->m_motionInfos);
    }

} // namespace EMotionFX

