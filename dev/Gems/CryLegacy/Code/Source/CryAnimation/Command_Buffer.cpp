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
#include "Command_Buffer.h"
#include "Command_Commands.h"

#include "CharacterInstance.h"
#include "SkeletonAnim.h"
#include "SkeletonPose.h"

#include "PoseModifier/PoseModifierHelper.h"
void ProcessAnimationDrivenIk(CCharInstance& instance, const SAnimationPoseModifierParams& params)
{
    DEFINE_PROFILER_FUNCTION();

    Skeleton::CPoseData* pPoseData = Skeleton::CPoseData::GetPoseData(params.pPoseData);
    if (!pPoseData)
    {
        return;
    }

    const int jointCount = static_cast<int>(pPoseData->GetJointCount());

    const CDefaultSkeleton& modelSkeleton = *instance.m_pDefaultSkeleton;

    QuatT* const __restrict pPoseRelative = pPoseData->GetJointsRelative();
    QuatT* const __restrict pPoseAbsolute = pPoseData->GetJointsAbsolute();

    uint targetCount = uint(modelSkeleton.m_ADIKTargets.size());
    for (uint i = 0; i < targetCount; i++)
    {
        LimbIKDefinitionHandle nHandle = modelSkeleton.m_ADIKTargets[i].m_nHandle;

        int targetJointIndex = modelSkeleton.m_ADIKTargets[i].m_idxTarget;
        if (targetJointIndex < 0 || targetJointIndex >= jointCount)
        {
            continue;
        }

        int targetWeightJointIndex = modelSkeleton.m_ADIKTargets[i].m_idxWeight;
        if (targetWeightJointIndex < 0 || targetWeightJointIndex >= jointCount)
        {
            continue;
        }

        int32 limbDefinitionIndex = modelSkeleton.GetLimbDefinitionIdx(nHandle);
        if (limbDefinitionIndex < 0)
        {
            continue;
        }

        const IKLimbType& limbType = modelSkeleton.m_IKLimbTypes[limbDefinitionIndex];

        const QuatT& targetLocationAbsolute = pPoseAbsolute[targetJointIndex];
        float targetBlendWeight = pPoseRelative[targetWeightJointIndex].t.x;
        targetBlendWeight = clamp_tpl(targetBlendWeight, 0.0f, 1.0f);
        if (targetBlendWeight > 0.01f)
        {
            uint numLinks = uint(limbType.m_arrJointChain.size());
            int endEffectorJointIndex = limbType.m_arrJointChain[numLinks - 1].m_idxJoint;
            int endEffectorParentJointIndex = modelSkeleton.m_arrModelJoints[endEffectorJointIndex].m_idxParent;

            pPoseAbsolute[0] = pPoseRelative[0];
            ANIM_ASSERT(pPoseRelative[0].q.IsUnit());
            for (uint j = 1; j < numLinks; ++j)
            {
                int p = limbType.m_arrRootToEndEffector[j - 1];
                int c = limbType.m_arrRootToEndEffector[j];
                ANIM_ASSERT(pPoseRelative[c].q.IsUnit());
                ANIM_ASSERT(pPoseRelative[p].q.IsUnit());
                pPoseAbsolute[c] = pPoseAbsolute[p] * pPoseRelative[c];
                ANIM_ASSERT(pPoseRelative[c].q.IsUnit());
            }

            const char* _2BIK = "2BIK";
            const char* _3BIK = "3BIK";
            const char* _CCDX = "CCDX";
            Vec3 vLocalTarget;
            vLocalTarget.SetLerp(pPoseAbsolute[endEffectorJointIndex].t, targetLocationAbsolute.t, targetBlendWeight);
            if (limbType.m_nSolver == *(uint32*)_2BIK)
            {
                PoseModifierHelper::IK_Solver2Bones(vLocalTarget, limbType, *pPoseData);
            }
            if (limbType.m_nSolver == *(uint32*)_3BIK)
            {
                PoseModifierHelper::IK_Solver3Bones(vLocalTarget, limbType, *pPoseData);
            }
            if (limbType.m_nSolver == *(uint32*)_CCDX)
            {
                PoseModifierHelper::IK_SolverCCD(vLocalTarget, limbType, *pPoseData);
            }

            for (uint j = 1; j < numLinks; ++j)
            {
                int c = limbType.m_arrJointChain[j].m_idxJoint;
                int p = limbType.m_arrJointChain[j - 1].m_idxJoint;
                pPoseAbsolute[c] = pPoseAbsolute[p] * pPoseRelative[c];
                ANIM_ASSERT(pPoseAbsolute[c].q.IsUnit());
            }

#ifdef _DEBUG
            ANIM_ASSERT(targetLocationAbsolute.q.IsUnit());
            ANIM_ASSERT(targetLocationAbsolute.q.IsValid());
            ANIM_ASSERT(pPoseAbsolute[endEffectorJointIndex].q.IsUnit());
            ANIM_ASSERT(pPoseAbsolute[endEffectorJointIndex].q.IsValid());
#endif
            pPoseAbsolute[endEffectorJointIndex].q.SetNlerp(
                pPoseAbsolute[endEffectorJointIndex].q, targetLocationAbsolute.q, targetBlendWeight);
#ifdef _DEBUG
            ANIM_ASSERT(pPoseAbsolute[endEffectorParentJointIndex].q.IsUnit());
            ANIM_ASSERT(pPoseAbsolute[endEffectorParentJointIndex].q.IsValid());
            ANIM_ASSERT(pPoseAbsolute[endEffectorJointIndex].q.IsUnit());
            ANIM_ASSERT(pPoseAbsolute[endEffectorJointIndex].q.IsValid());
#endif
            pPoseRelative[endEffectorJointIndex].q = !pPoseAbsolute[endEffectorParentJointIndex].q * pPoseAbsolute[endEffectorJointIndex].q;

            uint numChilds = modelSkeleton.m_IKLimbTypes[limbDefinitionIndex].m_arrLimbChildren.size();
            const int16* pJointIdx = &modelSkeleton.m_IKLimbTypes[limbDefinitionIndex].m_arrLimbChildren[0];
            for (uint u = 0; u < numChilds; u++)
            {
                int c = pJointIdx[u];
                int p = modelSkeleton.m_arrModelJoints[c].m_idxParent;
                pPoseAbsolute[c] = pPoseAbsolute[p] * pPoseRelative[c];
#ifdef _DEBUG
                ANIM_ASSERT(pPoseAbsolute[c].q.IsUnit());
                ANIM_ASSERT(pPoseAbsolute[p].q.IsUnit());
                ANIM_ASSERT(pPoseRelative[c].q.IsUnit());
                ANIM_ASSERT(pPoseAbsolute[c].q.IsValid());
                ANIM_ASSERT(pPoseAbsolute[p].q.IsValid());
                ANIM_ASSERT(pPoseRelative[c].q.IsValid());
#endif
            }
        }

        if (Console::GetInst().ca_DebugADIKTargets)
        {
            const char* pName = modelSkeleton.m_ADIKTargets[i].m_strTarget;

            float fColor[4] = {0, 1, 0, 1};
            g_pIRenderer->Draw2dLabel(1, g_YLine, 1.4f, fColor, false, "LHand_IKTarget: name: %s  rot: (%f %f %f %f)  pos: (%f %f %f) blend: %f", pName, targetLocationAbsolute.q.w, targetLocationAbsolute.q.v.x, targetLocationAbsolute.q.v.y, targetLocationAbsolute.q.v.z,  targetLocationAbsolute.t.x, targetLocationAbsolute.t.y, targetLocationAbsolute.t.z, pPoseRelative[targetWeightJointIndex].t.x);
            g_YLine += 16.0f;

            static Ang3 angle(0, 0, 0);
            angle += Ang3(0.01f, +0.02f, +0.03f);
            AABB aabb = AABB(Vec3(-0.015f, -0.015f, -0.015f), Vec3(+0.015f, +0.015f, +0.015f));

            Matrix33 m33 = Matrix33::CreateRotationXYZ(angle);
            OBB obb = OBB::CreateOBBfromAABB(m33, aabb);
            Vec3 obbPos = params.location * targetLocationAbsolute.t;
            g_pAuxGeom->DrawOBB(obb, obbPos, 1, RGBA8(0xff, 0x00, 0x1f, 0xff), eBBD_Extremes_Color_Encoded);
        }
    }
}

using namespace Command;

/*
CState
*/

bool CState::Initialize(CCharInstance* pInstance, const QuatTS& location)
{
    m_pInstance = pInstance;
    m_pDefaultSkeleton = pInstance->m_pDefaultSkeleton;

    m_location = location;

    m_pPoseData = pInstance->m_SkeletonPose.GetPoseDataWriteable();

    m_jointCount = pInstance->m_pDefaultSkeleton->GetJointCount();

    m_lod = m_pInstance->GetAnimationLOD();

    m_timeDelta = m_pInstance->m_fDeltaTime;

    m_pJointMask = NULL;
    m_jointMaskCount = 0;

    return true;
}

/*
CBuffer
*/

bool CBuffer::Initialize(CCharInstance* pInstance, const QuatTS& location)
{
    m_pCommands = m_pBuffer;
    m_commandCount = 0;

    m_state.Initialize(pInstance, location);

    m_pInstance = pInstance;
    return true;
}

void CBuffer::SetPoseData(Skeleton::CPoseData& poseData)
{
    m_state.m_pPoseData = &poseData;
}

void CBuffer::Execute()
{
    DEFINE_PROFILER_FUNCTION();

    if (!m_pInstance->m_SkeletonPose.m_physics.m_bPhysicsRelinquished)
    {
        m_state.m_pPoseData->ResetToDefault(*m_pInstance->m_pDefaultSkeleton);
    }

    PREFAST_SUPPRESS_WARNING(6255)
    QuatT * pJointsTemp = (QuatT*)alloca(m_state.m_jointCount * sizeof(QuatT));
    PREFAST_SUPPRESS_WARNING(6255)
    Vec2 * pJWeightSumTemp = (Vec2*)alloca(m_state.m_jointCount * sizeof(Vec2));
    PREFAST_SUPPRESS_WARNING(6255)
    JointState * pJointsStatusTemp = (JointState*)alloca(m_state.m_jointCount * sizeof(JointState));
    ::memset(pJointsStatusTemp, 0, m_state.m_jointCount * sizeof(JointState));

    QuatT* const __restrict pJointsRelative = m_state.m_pPoseData->GetJointsRelative();
    JointState* const __restrict pJointsStatusBase = m_state.m_pPoseData->GetJointsStatus();
    ::memset(pJointsStatusBase, 0, m_state.m_jointCount * sizeof(JointState));

    m_state.m_pJointMask = NULL;
    m_state.m_jointMaskCount = 0;

    void* CBTemp[TargetBuffer + 3];
    CBTemp[TmpBuffer + 0] = pJointsTemp;
    CBTemp[TmpBuffer + 1] = pJointsStatusTemp;
    CBTemp[TmpBuffer + 2] = pJWeightSumTemp;

    CBTemp[TargetBuffer + 0] = pJointsRelative;
    CBTemp[TargetBuffer + 1] = pJointsStatusBase;
    CBTemp[TargetBuffer + 2] = 0;

    uint8* pCommands = m_pBuffer;
    uint32 commandOffset = 0;
    for (uint32 i = 0; i < m_commandCount; ++i)
    {
        uint8& command = pCommands[commandOffset];
        switch (command)
        {
        case ClearPoseBuffer::ID:
        {
            ((ClearPoseBuffer&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(ClearPoseBuffer);
            break;
        }

        case PerJointBlending::ID:
        {
            ((PerJointBlending&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(PerJointBlending);
            break;
        }

        case SampleAddAnimFull::ID:
        {
            ((SampleAddAnimFull&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(SampleAddAnimFull);
            break;
        }

        case AddPoseBuffer::ID:
        {
            ((AddPoseBuffer&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(AddPoseBuffer);
            break;
        }

        case NormalizeFull::ID:
        {
            ((NormalizeFull&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(NormalizeFull);
            break;
        }

        case ScaleUniformFull::ID:
        {
            ((ScaleUniformFull&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(ScaleUniformFull);
            break;
        }
        case PoseModifier::ID:
        {
            ((PoseModifier&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(PoseModifier);
            break;
        }

        case SampleAddAnimPart::ID:
        {
            ((SampleAddAnimPart&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(SampleAddAnimPart);
            break;
        }

        case SampleReplaceAnimPart::ID:
        {
            ((SampleReplaceAnimPart&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(SampleReplaceAnimPart);
            break;
        }

        case JointMask::ID:
        {
            if (Console::GetInst().ca_UseJointMasking)
            {
                m_state.m_pJointMask = ((JointMask&)command).m_pMask;
                m_state.m_jointMaskCount = ((JointMask&)command).m_count;
            }
            commandOffset += sizeof(JointMask);
            break;
        }

        case SampleAddPoseFull::ID:
        {
            ((SampleAddPoseFull&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(SampleAddPoseFull);
            break;
        }

        case SamplePosePart::ID:
        {
            ((SamplePosePart&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(SamplePosePart);
            break;
        }
    #ifdef _DEBUG
        case VerifyFull::ID:
            ((VerifyFull&)command).Execute(m_state, CBTemp);
            commandOffset += sizeof(VerifyFull);
            break;
    #endif
        default:
            CryFatalError("CryAnimation: Command-Buffer Invalid: Command %d  Model: %s", command, m_state.m_pDefaultSkeleton->GetModelFilePath());
            break;
        }
    }

    m_state.m_pPoseData->ValidateRelative(*m_pInstance->m_pDefaultSkeleton);
    m_state.m_pPoseData->ComputeAbsolutePose(*m_pInstance->m_pDefaultSkeleton, m_state.m_pDefaultSkeleton->m_ObjectType == CHR);
    m_state.m_pPoseData->ValidateAbsolute(*m_pInstance->m_pDefaultSkeleton);

    SAnimationPoseModifierParams params;
    params.pCharacterInstance = m_state.m_pInstance;
    params.pPoseData = m_state.m_pPoseData;
    params.timeDelta = m_state.m_timeDelta;
    params.location = m_state.m_location;

    if (!m_pInstance->m_SkeletonPose.m_physics.m_bPhysicsRelinquished && m_pInstance->m_SkeletonAnim.m_IsAnimPlaying && Console::GetInst().ca_useADIKTargets)
    {
        ProcessAnimationDrivenIk(*m_pInstance, params);
    }

    m_pInstance->m_SkeletonPose.m_physics.Job_Physics_SynchronizeFrom(*m_state.m_pPoseData, m_pInstance->m_fOriginalDeltaTime);

    // NOTE: Temporary layer -1 PoseModifier queue to allow post-physics sync
    // PoseModifiers to be executed. This should not be needed once the
    // physics sync itself will become a PoseModifier.
    const CPoseModifierQueue& poseModifierQueue = m_pInstance->m_SkeletonAnim.m_poseModifierQueue;
    const CPoseModifierQueue::SEntry* pEntries = poseModifierQueue.GetEntries();
    uint entryCount = poseModifierQueue.GetEntryCount();
    for (uint i = 0; i < entryCount; ++i)
    {
        pEntries[i].poseModifier.get()->Execute(params);
    }
    if (m_pInstance->GetCharEditMode() == 0)
    {
        m_pInstance->m_AttachmentManager.UpdateAllRedirectedTransformations(*m_state.m_pPoseData);
    }

    if (Console::GetInst().ca_DebugCommandBuffer)
    {
        DebugDraw();
    }
}

void CBuffer::DebugDraw()
{
    float charsize = 1.4f;
    uint32 yscan =  14;
    float fColor2[4] = {1, 0, 1, 1};
    g_pIRenderer->Draw2dLabel(1, g_YLine, 1.3f, fColor2, false,
        "m_CommandBufferCounter %d  Commands: %d", GetLengthUsed(), m_commandCount);
    g_YLine += 10;

    IAnimationSet* pAnimationSet = m_state.m_pDefaultSkeleton->GetIAnimationSet();

    uint8* pCommands = m_pBuffer;
    uint32 commandOffset = 0;
    for (uint32 c = 0; c < m_commandCount; c++)
    {
        int32 anim_command = pCommands[commandOffset];
        switch (anim_command)
        {
        case ClearPoseBuffer::ID:
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "ClearPoseBuffer");
            g_YLine += yscan;
            commandOffset += sizeof(ClearPoseBuffer);
            break;

        case SampleAddAnimFull::ID:
        {
            const SampleAddAnimFull* pCommand = reinterpret_cast< const SampleAddAnimFull* >(&pCommands[commandOffset]);
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "SampleAddAnimFull: w:%f t:%f %s", pCommand->m_fWeight, pCommand->m_fETimeNew, pAnimationSet->GetNameByAnimID(pCommand->m_nEAnimID));
            g_YLine += yscan;
            commandOffset += sizeof(SampleAddAnimFull);
            break;
        }

        case AddPoseBuffer::ID:
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "AddPoseBuffer");
            g_YLine += yscan;
            commandOffset += sizeof(AddPoseBuffer);
            break;

        case NormalizeFull::ID:
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "NormalizeFull");
            g_YLine += yscan;
            commandOffset += sizeof(NormalizeFull);
            break;

        case ScaleUniformFull::ID:
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "ScaleUniformFull");
            g_YLine += yscan;
            commandOffset += sizeof(ScaleUniformFull);
            break;


        case SampleAddAnimPart::ID:
        {
            const SampleAddAnimPart* pCommand = reinterpret_cast< const SampleAddAnimPart* >(&pCommands[commandOffset]);
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "SampleAddAnimPart: w:%f t:%f %s", pCommand->m_fWeight, pCommand->m_fAnimTime, pAnimationSet->GetNameByAnimID(pCommand->m_nEAnimID));
            g_YLine += yscan;
            commandOffset += sizeof(SampleAddAnimPart);
            break;
        }

        case PerJointBlending::ID:
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "PerJointBlending");
            g_YLine += yscan;
            commandOffset += sizeof(PerJointBlending);
            break;

        case SampleReplaceAnimPart::ID:
        {
            const SampleReplaceAnimPart* pCommand = reinterpret_cast< const SampleReplaceAnimPart* >(&pCommands[commandOffset]);
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "SampleReplaceAnimPart: w:%f t:%f %s", pCommand->m_fWeight, pCommand->m_fAnimTime, pAnimationSet->GetNameByAnimID(pCommand->m_nEAnimID));
            g_YLine += yscan;
            commandOffset += sizeof(SampleReplaceAnimPart);
            break;
        }


#ifdef _DEBUG
        case VerifyFull::ID:
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "VerifyFull");
            g_YLine += yscan;
            commandOffset += sizeof(VerifyFull);
            break;
#endif

        case PoseModifier::ID:
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "PoseModifier: %s", ((PoseModifier*)&pCommands[commandOffset])->m_pPoseModifier->GetFactory()->GetName());
            g_YLine += yscan;
            commandOffset += sizeof(PoseModifier);
            break;

        case JointMask::ID:
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "State_JOINTMASK");
            g_YLine += yscan;
            commandOffset += sizeof(JointMask);
            break;


        //just for aim-files
        case SampleAddPoseFull::ID:
        {
            const SampleAddPoseFull* pCommand = reinterpret_cast< const SampleAddPoseFull* >(&pCommands[commandOffset]);
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "SampleAddPoseFull: w:%f t:%f %s", pCommand->m_fWeight, pCommand->m_fETimeNew, pAnimationSet->GetNameByAnimID(pCommand->m_nEAnimID));
            g_YLine += yscan;
            commandOffset += sizeof(SampleAddPoseFull);
            break;
        }
        case SamplePosePart::ID:
        {
            const SamplePosePart* pCommand = reinterpret_cast< const SamplePosePart* >(&pCommands[commandOffset]);
            g_pIRenderer->Draw2dLabel(1, g_YLine, charsize, fColor2, false, "SamplePosePart: w:%f t:%f %s", pCommand->m_fWeight, pCommand->m_fAnimTime, pAnimationSet->GetNameByAnimID(pCommand->m_nEAnimID));
            g_YLine += yscan;
            commandOffset += sizeof(SamplePosePart);
            break;
        }



        default:
            CryFatalError("CryAnimation: Command-Buffer Invalid: Command %d  Model: %s", anim_command, m_state.m_pDefaultSkeleton->GetModelFilePath());
            break;
        }
    }
}
