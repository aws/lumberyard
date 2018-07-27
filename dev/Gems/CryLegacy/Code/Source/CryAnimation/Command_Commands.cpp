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

#include "Helper.h"

#include "ControllerOpt.h"

#include "CharacterInstance.h"
#include "SkeletonAnim.h"
#include "SkeletonPose.h"

#include "Command_Buffer.h"
#include "Command_Commands.h"

#include "Skeleton.h"
#include "LoaderDBA.h"
#include "CharacterManager.h"


extern float g_YLine;

namespace Command {
    void LoadControllers(const GlobalAnimationHeaderCAF& rGAH, const Command::CState& state, IController** controllers)
    {
        memset(controllers, 0, state.m_jointCount * sizeof(IController*));

        if (rGAH.IsAssetOnDemand() && !rGAH.IsAssetLoaded())
        {
            CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "LoadControllers: Skipping asset because it's not loaded (%s)", rGAH.GetFilePath());
            return;
        }

        if (rGAH.m_nControllers2)
        {
            if (rGAH.m_nControllers == 0)
            {
                uint32 dba_exists = 0;
                if (rGAH.m_FilePathDBACRC32)
                {
                    size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
                    for (uint32 d = 0; d < numDBA_Files; d++)
                    {
                        CGlobalHeaderDBA& pGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[d];
                        if (rGAH.m_FilePathDBACRC32 != pGlobalHeaderDBA.m_FilePathDBACRC32)
                        {
                            continue;
                        }

                        dba_exists++;
                        break;
                    }
                }

                if (dba_exists)
                {
                    if (Console::GetInst().ca_DebugCriticalErrors)
                    {
                        //this case is virtually impossible, unless something went wrong with a DBA or maybe a CAF in a DBA was compressed to death and all controllers removed
                        //  const char* mname = state.m_pInstance->GetFilePath();
                        //  f32 fColor[4] = {1,1,0,1};
                        //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fColor, false,"model: %s",mname);
                        //  g_YLine+=0x10;
                        //  g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.3f, fColor, false,"No Controllers found in Asset: %02x %08x %s",rGAH.m_nControllers2,rGAH.m_FilePathDBACRC32,rGAH.m_FilePath.c_str() );
                        //  g_YLine+=23.0f;
                        CryFatalError("CryAnimation: No Controllers found in Asset: %s", rGAH.GetFilePath());
                    }

                    g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, 0, "No Controllers found in Asset: %s", rGAH.GetFilePath());
                }

                return; //return and don't play animation, because we don't have any controllers
            }
        }

        uint32* pLodJointMask = NULL;
        uint32 lodJointMaskCount = 0;
        if (state.m_lod > 0)
        {
            if (uint32 lodCount = state.m_pDefaultSkeleton->m_arrAnimationLOD.size())
            {
                uint32 lod = state.m_lod;
                if (lod > lodCount)
                {
                    lod = lodCount;
                }
                --lod;

                pLodJointMask = &state.m_pDefaultSkeleton->m_arrAnimationLOD[lod][0];
                lodJointMaskCount = state.m_pDefaultSkeleton->m_arrAnimationLOD[lod].size();
            }
        }

        const CDefaultSkeleton::SJoint* pModelJoint = &state.m_pDefaultSkeleton->m_arrModelJoints[0];
        uint32 jointCount = state.m_jointCount;
        for (uint32 i = 0; i < jointCount; ++i)
        {
            uint32 crc32 = pModelJoint[i].m_nJointCRC32;
            if (pLodJointMask)
            {
                if (Helper::FindFromSorted<uint32>(pLodJointMask, lodJointMaskCount, crc32) == NULL)
                {
                    continue;
                }
            }

            if (!state.IsJointActive(crc32))
            {
                continue;
            }

            controllers[i] = rGAH.GetControllerByJointCRC32(pModelJoint[i].m_nJointCRC32);
        }
    }



    void ClearPoseBuffer::Execute(const CState& state, void* buffers[]) const
    {
        const ClearPoseBuffer& ac = *this;
        void** CBTemp = buffers;

        // PARANOIA: ac.m_TargetBuffer is only ever supposed to have the values Command::TargetBuffer or Command::TmpBuffer
        // (Other commands do this with a flag instead of passing a possibly invalid index ...)
        CRY_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_TargetBuffer == Command::TmpBuffer);

        uint32 numJoints = state.m_jointCount;

        //clear buffer for the relative pose
        f32 fIsIndentity = f32(ac.m_nPoseInit);
        QuatT* parrRelPoseDst = (QuatT*) CBTemp[ac.m_TargetBuffer + 0];
        for (uint32 j = 0; j < numJoints; j++)
        {
            parrRelPoseDst[j].q.v.x = 0.0f;
            parrRelPoseDst[j].q.v.y = 0.0f;
            parrRelPoseDst[j].q.v.z = 0.0f;
            parrRelPoseDst[j].q.w = fIsIndentity;
            parrRelPoseDst[j].t.x = 0.0f;
            parrRelPoseDst[j].t.y = 0.0f;
            parrRelPoseDst[j].t.z = 0.0f;
        }


        //clear buffer joint states
        JointState* parrStatusDst = (JointState*)CBTemp[ac.m_TargetBuffer + 1];
        for (uint32 j = 0; j < numJoints; j++)
        {
            parrStatusDst[j] = ac.m_nJointStatus;
        }


        //clear buffer for weight mask (this is only used for partial body blends)
        Vec2*   parrWeightMask = (Vec2*)CBTemp[ac.m_TargetBuffer + 2];
        if (parrWeightMask == 0)
        {
            return; //not initialized
        }
        for (uint32 j = 0; j < numJoints; j++)
        {
            parrWeightMask[j] = Vec2(ZERO);
        }
    }


    //this function operates on "rGlobalAnimHeaderCAF"
    void SampleAddAnimFull::Execute(const CState& state, void* buffers[]) const
    {
        float fColor[4] = {1, 0, 0, 1};
        const SampleAddAnimFull& ac = *this;
        void** CBTemp = buffers;

        const int32 nBufferID = (ac.m_flags & Flag_TmpBuffer) ? Command::TmpBuffer : Command::TargetBuffer;
        QuatT*      parrRelPoseDst  = (QuatT*)  CBTemp[nBufferID + 0];
        JointState* parrStatusDst       = (JointState*)CBTemp[nBufferID + 1];

        uint32 numJoints = state.m_jointCount;
        JointState& getOPResult = parrStatusDst[0];

        CAnimationSet* pAnimationSet = state.m_pDefaultSkeleton->m_pAnimationSet;
        const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);

        // PARANOIA: SampleAddAnim is only called from locations that have already pulled the MAG and checked the asset type
        CRY_ASSERT(pMAG && pMAG->m_nAssetType == CAF_File);

        int32 nEGlobalID = pMAG->m_nGlobalAnimId;
        if (nEGlobalID < 0 || nEGlobalID >= g_AnimationManager.m_arrGlobalCAF.size())
        {
            // Unexpected: Header references an asset that doesn't exist
            return;
        }

        const GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[nEGlobalID];
        if (rCAF.IsAssetOnDemand())
        {
            if (rCAF.IsAssetLoaded() == 0)
            {
                int nCurrentFrameID = g_pCharacterManager->m_nUpdateCounter;
                g_pIRenderer->Draw2dLabel(1, g_YLine, 2.3f, fColor, false, "CryAnimation: Asset Not Loaded: %s   nCurrentFrameID: %d  Weight: %f", rCAF.GetFilePath(), nCurrentFrameID, ac.m_fWeight);
            }
        }

        //if we have an animation in the base-layer, then we ALWAYS initialize all joints
        memset(parrStatusDst, eJS_Position | eJS_Orientation | eJS_Scale, numJoints);


        PREFAST_SUPPRESS_WARNING(6255)
        IController * *parrController = (IController**)alloca(state.m_jointCount * sizeof(IController*));
        LoadControllers(rCAF, state, parrController);

        PREFAST_SUPPRESS_WARNING(6255)
        QuatT * qtemp = (QuatT*)alloca(state.m_jointCount * sizeof(QuatT));
        const QuatT* parrDefJoints = state.m_pDefaultSkeleton->m_poseDefaultData.GetJointsRelativeMain();
        {
            DEFINE_PROFILER_SECTION("cryMemcpy");
            cryMemcpy(&qtemp[0], parrDefJoints, numJoints * sizeof(QuatT));
        }

        f32 fDuration       = max(1.0f / rCAF.GetSampleRate(), rCAF.m_fTotalDuration);
        f32 fKeyTime1       = rCAF.NTime2KTime(1);
        f32 fKeyTimeNew = rCAF.NTime2KTime(ac.m_fETimeNew);

        uint32 nStartJoint = 0;
        if (ac.m_flags & Flag_ADMotion)
        {
            nStartJoint = 1;

            // When root motion is enabled, set to first frame of the animation, rather than just setting identity.
            // This is tolerant to animations with non-identity root transforms on first frame, even if the root isn't animated.
            if (parrController[0])
            {
                parrController[0]->GetOP(rCAF.NTime2KTime(0.f), parrRelPoseDst[0].q, parrRelPoseDst[0].t);
            }
        }


        //-------------------------------------------------------------------------------------
        //----             evaluate all controllers for this animation                    -----
        //-------------------------------------------------------------------------------------
        uint32 nAdditiveAnimation = rCAF.IsAssetAdditive();
        if (nAdditiveAnimation)
        {
            //additive asset
            for (uint32 j = nStartJoint; j < numJoints; j++)
            {
                if (parrController[j])
                {
                    JointState ops = parrController[j]->GetOP(fKeyTimeNew, qtemp[j].q, qtemp[j].t);
                    if (ops & eJS_Orientation)
                    {
                        qtemp[j].q = qtemp[j].q * parrDefJoints[j].q;
                    }
                    if (ops & eJS_Position)
                    {
                        qtemp[j].t = qtemp[j].t + parrDefJoints[j].t;
                    }
                }

                ANIM_ASSERT(qtemp[j].q.IsUnit());
                ANIM_ASSERT(qtemp[j].q.IsValid());
                ANIM_ASSERT(qtemp[j].t.IsValid());

                parrRelPoseDst[j].q += ac.m_fWeight * qtemp[j].q * fsgnnz(parrRelPoseDst[j].q | qtemp[j].q);
                parrRelPoseDst[j].t += ac.m_fWeight * qtemp[j].t;
            }
        }
        else
        {
            //overwrite asset
            for (uint32 j = nStartJoint; j < numJoints; j++)
            {
                if (parrController[j])
                {
                    parrController[j]->GetOP(fKeyTimeNew, qtemp[j].q, qtemp[j].t);
                }

                ANIM_ASSERT(qtemp[j].q.IsUnit());
                ANIM_ASSERT(qtemp[j].q.IsValid());
                ANIM_ASSERT(qtemp[j].t.IsValid());

                parrRelPoseDst[j].q += ac.m_fWeight * qtemp[j].q * fsgnnz(parrRelPoseDst[j].q | qtemp[j].q);
                parrRelPoseDst[j].t += ac.m_fWeight * qtemp[j].t;
            }
        }

#ifdef ANIM_ASSERT_ENABLED
        for (uint32 j = 0; j < numJoints; j++)
        {
            ANIM_ASSERT(parrRelPoseDst[j].q.IsValid());
            ANIM_ASSERT(parrRelPoseDst[j].t.IsValid());
        }
#endif

        g_pCharacterManager->g_SkeletonUpdates++;
    }

    //this function operates on "rGlobalAnimHeaderAIM"
    void SampleAddPoseFull::Execute(const CState& state, void* buffers[]) const
    {
        const SampleAddPoseFull& ac = *this;
        void** CBTemp = buffers;

        const int32 nBufferID = (ac.m_flags & SampleAddAnimFull::Flag_TmpBuffer) ? Command::TmpBuffer : Command::TargetBuffer;
        QuatT*      parrRelPoseDst  = (QuatT*)  CBTemp[nBufferID + 0];
        JointState* parrStatusDst       = (JointState*)CBTemp[nBufferID + 1];

        uint32 numJoints = state.m_jointCount;
        JointState& getOPResult = parrStatusDst[0];

        CAnimationSet* pAnimationSet = state.m_pDefaultSkeleton->m_pAnimationSet;
        const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);

        // PARANOIA: SampleAddPoseFull is only called from locations that have already pulled the MAG and checked the asset type
        CRY_ASSERT(pMAG && pMAG->m_nAssetType == AIM_File);

        int32 nEGlobalID = pMAG->m_nGlobalAnimId;
        if (nEGlobalID < 0 || nEGlobalID >= g_AnimationManager.m_arrGlobalAIM.size())
        {
            // Header references an asset that doesn't exist
            return;
        }

        const GlobalAnimationHeaderAIM& rGlobalAnimHeaderAIM = g_AnimationManager.m_arrGlobalAIM[nEGlobalID];

        memset(parrStatusDst, 0xff, numJoints);
        const CDefaultSkeleton::SJoint* pModelJoint = &state.m_pDefaultSkeleton->m_arrModelJoints[0];
        PREFAST_SUPPRESS_WARNING(6255)
        IController * *parrController = (IController**)alloca(state.m_jointCount * sizeof(IController*));
        memset(parrController, 0, state.m_jointCount * sizeof(IController*));
        for (uint32 i = 0; i < numJoints; ++i)
        {
            parrController[i] = rGlobalAnimHeaderAIM.GetControllerByJointCRC32(pModelJoint[i].m_nJointCRC32);
        }

        PREFAST_SUPPRESS_WARNING(6255)
        QuatT * qtemp = (QuatT*)alloca(state.m_jointCount * sizeof(QuatT));
        const QuatT* parrDefJoints = state.m_pDefaultSkeleton->m_poseDefaultData.GetJointsRelativeMain();
        cryMemcpy(&qtemp[0], parrDefJoints, numJoints * sizeof(QuatT));

        f32 fKeyTimeNew = rGlobalAnimHeaderAIM.NTime2KTime(ac.m_fETimeNew);

        //-------------------------------------------------------------------------------------
        //----             evaluate all controllers for this animation                    -----
        //-------------------------------------------------------------------------------------
        for (uint32 j = 0; j < numJoints; j++)
        {
            if (parrController[j])
            {
                parrController[j]->GetOP(fKeyTimeNew, qtemp[j].q, qtemp[j].t);
            }
            ANIM_ASSERT(qtemp[j].q.IsUnit());
            ANIM_ASSERT(qtemp[j].q.IsValid());
            ANIM_ASSERT(qtemp[j].t.IsValid());

            parrRelPoseDst[j].q += ac.m_fWeight * qtemp[j].q * fsgnnz(parrRelPoseDst[j].q | qtemp[j].q);
            parrRelPoseDst[j].t += ac.m_fWeight * qtemp[j].t;
        }

#ifdef ANIM_ASSERT_ENABLED
        for (uint32 j = 0; j < numJoints; j++)
        {
            CRY_ASSERT(parrRelPoseDst[j].q.IsValid());
            CRY_ASSERT(parrRelPoseDst[j].t.IsValid());
        }
#endif
    }

    //reads content from m_SourceBuffer, multiplies the pose by a blend weight, and adds the result to the m_TargetBuffer
    void AddPoseBuffer::Execute(const CState& state, void* buffers[]) const
    {
        const AddPoseBuffer& ac = *this;
        void** CBTemp = buffers;

        // PARANOIA: ac.m_TargetBuffer and ac.m_SourceBuffer are only ever supposed to be assigned this way
        // (why these are parameters is anyone's guess)
        CRY_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_SourceBuffer == Command::TmpBuffer);

        QuatT*      parrRelPoseSrc      = (QuatT*)  CBTemp[ac.m_SourceBuffer + 0];
        JointState* parrStatusSrc       = (JointState*)CBTemp[ac.m_SourceBuffer + 1];

        QuatT*      parrRelPoseDst      = (QuatT*)  CBTemp[ac.m_TargetBuffer + 0];
        JointState* parrStatusDst       = (JointState*)CBTemp[ac.m_TargetBuffer + 1];

        f32 t = ac.m_fWeight;

        uint32 numJoints = state.m_jointCount;
        for (uint32 i = 0; i < numJoints; i++)
        {
            parrRelPoseDst[i].q += parrRelPoseSrc[i].q * t;
            parrRelPoseDst[i].t += parrRelPoseSrc[i].t * t;
            parrStatusDst[i]    |= parrStatusSrc[i];
        }
#ifdef ANIM_ASSERT_ENABLED
        for (uint32 j = 0; j < numJoints; j++)
        {
            CRY_ASSERT(parrRelPoseDst[j].q.IsValid());
            CRY_ASSERT(parrRelPoseDst[j].t.IsValid());
        }
#endif
    }

    void NormalizeFull::Execute(const CState& state, void* buffers[]) const
    {
        const NormalizeFull& ac = *this;
        void** CBTemp = buffers;

        // PARANOIA: ac.m_TargetBuffer is only ever supposed to have the values Command::TargetBuffer or Command::TmpBuffer
        // (Other commands do this with a flag instead of passing a possibly invalid index ...)
        CRY_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_TargetBuffer == Command::TmpBuffer);

        QuatT*          parrRelPoseDst  = (QuatT*)  CBTemp[ac.m_TargetBuffer + 0];
        JointState*     parrStatusDst       = (JointState*)CBTemp[ac.m_TargetBuffer + 1];

        f32 fDotLocator = fabsf(parrRelPoseDst[0].q | parrRelPoseDst[0].q);
        if (fDotLocator > 0.0001f)
        {
            parrRelPoseDst[0].q *= isqrt_tpl(fDotLocator);
        }
        else
        {
            parrRelPoseDst[0].q.SetIdentity();
        }

        uint32 numJoints = state.m_jointCount;
        for (uint32 i = 1; i < numJoints; i++)
        {
            f32 dot = fabsf(parrRelPoseDst[i].q | parrRelPoseDst[i].q);
            if (dot > 0.0001f)
            {
                parrRelPoseDst[i].q *= isqrt_tpl(dot);
            }
            else
            {
                parrRelPoseDst[i].q.SetIdentity();
            }

            ANIM_ASSERT(parrRelPoseDst[i].q.IsUnit());
        }
    }

    void ScaleUniformFull::Execute(const CState& state, void* buffers[]) const
    {
        const ScaleUniformFull& ac = *this;
        void** CBTemp = buffers;

        // PARANOIA: ac.m_TargetBuffer is only ever supposed to have the values Command::TargetBuffer or Command::TmpBuffer
        // (Other commands do this with a flag instead of passing a possibly invalid index ...)
        CRY_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_TargetBuffer == Command::TmpBuffer);

        QuatT*          parrRelPoseDst  = (QuatT*)    CBTemp[ac.m_TargetBuffer + 0];
        JointState*     parrStatusDst       = (JointState*)  CBTemp[ac.m_TargetBuffer + 1];
        uint32 numJoints = state.m_jointCount;
        for (uint32 j = 0; j < numJoints; j++)
        {
            if (parrStatusDst[j] & eJS_Position)
            {
                parrRelPoseDst[j].t *= ac.m_fScale;
            }
        }
#ifdef ANIM_ASSERT_ENABLED
        for (uint32 j = 0; j < numJoints; j++)
        {
            ANIM_ASSERT(parrRelPoseDst[j].q.IsValid());
            ANIM_ASSERT(parrRelPoseDst[j].t.IsValid());
        }
#endif
    }

#if defined(USE_PROTOTYPE_ABS_BLENDING)
    struct SAbsoluteTransform
    {
        Quat prevAbs;
        Quat newAbs;
    };
#endif //!(USE_PROTOTYPE_ABS_BLENDING)

    //replace handposes in BSpaces
    void SampleReplaceAnimPart::Execute(const CState& state, void* buffers[]) const
    {
        const SampleReplaceAnimPart& ac = *this;
        void** CBTemp = buffers;

        // PARANOIA: ac.m_TargetBuffer is only ever supposed to have the values Command::TargetBuffer or Command::TmpBuffer
        // (Other commands do this with a flag instead of passing a possibly invalid index ...)
        CRY_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_TargetBuffer == Command::TmpBuffer);

        QuatT*          parrRelPoseDst  = (QuatT*)      CBTemp[ac.m_TargetBuffer + 0];

        CAnimationSet* pAnimationSet = state.m_pDefaultSkeleton->m_pAnimationSet;
        const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);

        // PARANOIA: SampleReplaceAnimPart is only called from locations that have already pulled the MAG and checked the asset type
        CRY_ASSERT(pMAG && pMAG->m_nAssetType == CAF_File);

        int32 nEGlobalID = pMAG->m_nGlobalAnimId;
        if (nEGlobalID < 0 || nEGlobalID >= g_AnimationManager.m_arrGlobalCAF.size())
        {
            // Header references an asset that doesn't exist
            return;
        }

        // cache vars to local pointers (spares using this pointer everytime)
        uint32 numJoints = state.m_jointCount;
        GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[nEGlobalID];

        PREFAST_SUPPRESS_WARNING(6255)
        IController * *parrController = (IController**)alloca(state.m_jointCount * sizeof(IController*));
        LoadControllers(rCAF, state, parrController);

        f32 fKeyTimeNew = rCAF.NTime2KTime(ac.m_fAnimTime);
        uint32 nAdditiveAnimation = rCAF.IsAssetAdditive();
        //-------------------------------------------------------------------------------------
        //----             evaluate all controllers for this animation                    -----
        //-------------------------------------------------------------------------------------
        g_pCharacterManager->g_SkeletonUpdates++;

        if (nAdditiveAnimation)
        {
            //add full Additive-pose to the base-pose
            for (uint32 j = 1; j < numJoints; j++)
            {
                if (parrController[j])
                {
                    Quat rot;
                    Vec3 pos;
                    JointState jointState = parrController[j]->GetOP(fKeyTimeNew, rot, pos);
                    if (jointState & eJS_Orientation)
                    {
                        parrRelPoseDst[j].q = rot * parrRelPoseDst[j].q;
                    }
                    if (jointState & eJS_Position)
                    {
                        parrRelPoseDst[j].t += pos;
                    }
                }
            }
        }
        else
        {
            const QuatT* parrDefJoints = state.m_pDefaultSkeleton->m_poseDefaultData.GetJointsRelativeMain();
            //replace base-pose with controllers in Override-Animations
            for (uint32 j = 1; j < numJoints; j++)
            {
                if (parrController[j])
                {
                    Quat rot;
                    Vec3 pos;
                    JointState jointState = parrController[j]->GetOP(fKeyTimeNew, rot, pos);

                    if (jointState & eJS_Orientation)
                    {
                        parrRelPoseDst[j].q = rot;
                    }
                    if (jointState & eJS_Position)
                    {
                        parrRelPoseDst[j].t = pos;
                    }
                }
            }
        }
    }



    void SampleAddAnimPart::Execute(const CState& state, void* buffers[]) const
    {
        const SampleAddAnimPart& ac = *this;
        void** CBTemp = buffers;

        // PARANOIA: ac.m_TargetBuffer is only ever supposed to have the values Command::TargetBuffer or Command::TmpBuffer
        // (Other commands do this with a flag instead of passing a possibly invalid index ...)
        CRY_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_TargetBuffer == Command::TmpBuffer);

        QuatT*          parrRelPoseDst  = (QuatT*)      CBTemp[ac.m_TargetBuffer + 0];
        JointState* parrStatusDst       = (JointState*) CBTemp[ac.m_TargetBuffer + 1];
        Vec2*               parrJWeightsDst = (Vec2*)       CBTemp[ac.m_TargetBuffer + 2];
        if (parrJWeightsDst == 0)
        {
            return; //not initialized
        }
        CAnimationSet* pAnimationSet = state.m_pDefaultSkeleton->m_pAnimationSet;
        const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);

        // PARANOIA: SampleAddAnimPart is only called from locations that have already pulled the MAG and checked the asset type
        CRY_ASSERT(pMAG && pMAG->m_nAssetType == CAF_File);

        int32 nEGlobalID = pMAG->m_nGlobalAnimId;
        if (nEGlobalID < 0 || nEGlobalID >= g_AnimationManager.m_arrGlobalCAF.size())
        {
            // Header references an asset that doesn't exist
            return;
        }

        // cache vars to local pointers (spares using this pointer everytime)
        uint32 numJoints = state.m_jointCount;
        CRY_ASSERT(ac.m_fAnimTime >= 0.0f && ac.m_fAnimTime <= 1.0f);

        GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[nEGlobalID];
        uint32 nAdditiveAnimation = rCAF.IsAssetAdditive();

        PREFAST_SUPPRESS_WARNING(6255)
        IController * *parrController = (IController**)alloca(state.m_jointCount * sizeof(IController*));
        LoadControllers(rCAF, state, parrController);

        f32 fKeyTimeNew = rCAF.NTime2KTime(ac.m_fAnimTime);

        //-------------------------------------------------------------------------------------
        //----             evaluate all controllers for this animation                    -----
        //-------------------------------------------------------------------------------------
        g_pCharacterManager->g_SkeletonUpdates++;

        if (nAdditiveAnimation)
        {
            //accumulate all Additive-Animations and store the result in a temp buffer
            for (uint32 j = 1; j < numJoints; j++)
            {
                if (parrController[j])
                {
                    Quat rot;
                    Vec3 pos;
                    JointState jointState = parrController[j]->GetOP(fKeyTimeNew, rot, pos);
                    if (jointState & eJS_Orientation)
                    {
                        parrJWeightsDst[j].x += ac.m_fWeight;
                        parrRelPoseDst[j].q = Quat::CreateNlerp(IDENTITY, rot, ac.m_fWeight) * parrRelPoseDst[j].q;
                    }
                    if (jointState & eJS_Position)
                    {
                        parrJWeightsDst[j].y += ac.m_fWeight;
                        parrRelPoseDst[j].t += pos * ac.m_fWeight;
                    }
                    parrStatusDst[j] |= jointState;
                }
            }
        }
#if defined(USE_PROTOTYPE_ABS_BLENDING)
        else if (ac.m_maskNumJoints > 0)
        {
            const strided_pointer<const int>& maskIDs           = ac.m_maskJointIDs;
            const strided_pointer<const float>& maskWeights = ac.m_maskJointWeights;

            const QuatT* parrRelPoseSrc = (const QuatT*)  CBTemp[ac.m_SourceBuffer + 0];

            const int32 numWeighted = ac.m_maskNumJoints;

            PREFAST_SUPPRESS_WARNING(6255)
            SAbsoluteTransform * pAbsoluteTransforms = (SAbsoluteTransform*)alloca(numWeighted * sizeof(SAbsoluteTransform));
            PREFAST_SUPPRESS_WARNING(6255)
            uint8 * pJointSettings = (uint8*)alloca(numJoints * sizeof(uint8));

            memset(pJointSettings, 0, numJoints * sizeof(uint8));

            JointState jointState = 0;
            const IController* pController;

            Skeleton::CPoseData* pPoseData = &state.m_pDefaultSkeleton->m_poseData;
            const QuatT* bindPoseTrans = pPoseData->GetJointsRelativeMain();

            //--- Calculate absolute rotations
            Quat rot;
            Vec3 pos;
            Quat rotAbsPrev(IDENTITY);
            Quat rotAbsNew(IDENTITY);
            Quat rotAbsPrevParent(IDENTITY);
            Quat rotAbsNewParent(IDENTITY);
            Quat rotAbsBlendedRot(IDENTITY);
            for (int32 i = 0; i < numWeighted; i++)
            {
                uint32 root = maskIDs[i];
                float weight = maskWeights[i];

                rotAbsPrevParent.SetIdentity();
                rotAbsNewParent.SetIdentity();

                bool isDone = false;
                for (int32 j = pModelJoint[root].m_idxParent; (j >= 0) && !isDone; j = pModelJoint[j].m_idxParent)
                {
                    for (int32 k = 0; k < i; k++)
                    {
                        if (j == maskIDs[k])
                        {
                            //--- Early exit, we have this already
                            rotAbsPrevParent = pAbsoluteTransforms[k].prevAbs * rotAbsPrevParent;
                            rotAbsNewParent  = pAbsoluteTransforms[k].newAbs * rotAbsNewParent;
                            isDone = true;
                            break;
                        }
                    }

                    if ((j >= 0) && !isDone)
                    {
                        rotAbsPrevParent = parrRelPoseSrc[j].q * rotAbsPrevParent;
                        jointState = 0;
                        if (parrController[j])
                        {
                            jointState = parrController[j]->GetO(fKeyTimeNew, rot);
                        }
                        if ((jointState & eJS_Orientation) == 0)
                        {
                            rot = bindPoseTrans[j].q;
                        }

                        rotAbsNewParent = rot * rotAbsNewParent;
                    }
                }

                //--- Now extract the root's animated state and setup the blended absolute rotation
                jointState = 0;
                if (parrController[root])
                {
                    jointState =  parrController[root]->GetOP(fKeyTimeNew, rot, pos);
                }
                if ((jointState & (eJS_Orientation | eJS_Position)) != (eJS_Orientation | eJS_Position))
                {
                    const QuatT& tran = bindPoseTrans[root];

                    if ((jointState & eJS_Orientation) == 0)
                    {
                        rot = tran.q;
                    }
                    if ((jointState & eJS_Position) == 0)
                    {
                        pos = tran.t;
                    }
                }

                rotAbsPrev = rotAbsPrevParent * parrRelPoseSrc[root].q;
                rotAbsNew  = rotAbsNewParent * rot;

                //--- Blend joint
                rotAbsBlendedRot.SetNlerp(rotAbsPrev, rotAbsNew, ac.m_fWeight * weight);

                //--- Store for future blends
                pAbsoluteTransforms[i].prevAbs = rotAbsBlendedRot;
                pAbsoluteTransforms[i].newAbs  = rotAbsNew;

                JointState& RESTRICT_REFERENCE rStatusDstRoot = parrStatusDst[root];
                parrRelPoseDst[root].t += (pos * (ac.m_fWeight * weight));
                parrRelPoseDst[root].q += (!rotAbsPrevParent * rotAbsNew) * (ac.m_fWeight * weight);
                parrJWeightsDst[root].x += (ac.m_fWeight * weight);
                parrJWeightsDst[root].y += (ac.m_fWeight * weight);
                rStatusDstRoot |= (jointState | eJS_Orientation);

                pJointSettings[root] = (weight >= 0.999f) ? 1 : 2;
            }

            for (uint32 j = 1; j < numJoints; j++)
            {
                if (pJointSettings[j] == 0)
                {
                    QuatT& RESTRICT_REFERENCE rRelPoseDst = parrRelPoseDst[j];
                    JointState& RESTRICT_REFERENCE rStatusDst = parrStatusDst[j];

                    int parent = pModelJoint[j].m_idxParent;
                    if ((parent >= -1) && (pJointSettings[pModelJoint[j].m_idxParent] == 1))
                    {
                        pJointSettings[j] = 1;
                        jointState = 0;

                        pController = parrController[j];

                        if (pController)
                        {
                            jointState = pController->GetOP(fKeyTimeNew, rot, pos);
                            rStatusDst |= jointState;
                        }
                        if ((jointState & (eJS_Orientation | eJS_Position)) != (eJS_Orientation | eJS_Position))
                        {
                            const QuatT& tran = bindPoseTrans[j];

                            if ((jointState & eJS_Orientation) == 0)
                            {
                                rot = tran.q;
                            }
                            if ((jointState & eJS_Position) == 0)
                            {
                                pos = tran.t;
                            }
                        }

                        rRelPoseDst.q += rot * fsgnnz(rRelPoseDst[j].q | rot) * ac.m_fWeight;
                        rRelPoseDst.t += pos * ac.m_fWeight;
                        rStatusDst |= jointState;

                        parrJWeightsDst[j].x += ac.m_fWeight;
                        parrJWeightsDst[j].y += ac.m_fWeight;
                    }
                }
            }
        }
#endif //!USE_PROTOTYPE_ABS_BLENDING
        else
        {
            const QuatT* parrDefJoints = state.m_pDefaultSkeleton->m_poseDefaultData.GetJointsRelativeMain();
            //accumulate all Override-Animations and store the result in a temp buffer
            for (uint32 j = 1; j < numJoints; j++)
            {
                if (parrController[j])
                {
                    Quat rot;
                    Vec3 pos;
                    JointState jointState = parrController[j]->GetOP(fKeyTimeNew, rot, pos);

                    if (jointState & eJS_Orientation)
                    {
                        parrJWeightsDst[j].x += ac.m_fWeight;
                        parrRelPoseDst[j].q += rot * fsgnnz(parrRelPoseDst[j].q | rot) * ac.m_fWeight;
                    }
                    if (jointState & eJS_Position)
                    {
                        parrJWeightsDst[j].y += ac.m_fWeight;
                        parrRelPoseDst[j].t += pos * ac.m_fWeight;
                    }
                    parrStatusDst[j] |= jointState;
                }
            }
        }


#ifdef ANIM_ASSERT_ENABLED
        uint32 o = 0;
        uint32 p = 0;
        for (uint32 j = 0; j < numJoints; j++)
        {
            if (parrStatusDst[j] & eJS_Orientation)
            {
                ANIM_ASSERT(parrRelPoseDst[j].q.IsValid());
                o++;
            }
            if (parrStatusDst[j] & eJS_Position)
            {
                ANIM_ASSERT(parrRelPoseDst[j].t.IsValid());
                p++;
            }
        }
#endif
    }



    void PerJointBlending::Execute(const CState& state, void* buffers[]) const
    {
        const PerJointBlending& ac = *this;
        void** CBTemp = buffers;

        // PARANOIA: ac.m_TargetBuffer is only ever supposed to have the values Command::TargetBuffer or Command::TmpBuffer
        // (Other commands do this with a flag instead of passing a possibly invalid index ...)
        CRY_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_TargetBuffer == Command::TmpBuffer);

        //this is source-buffer No.1
        QuatT* parrRelPoseLayer         = (QuatT*)CBTemp[ac.m_SourceBuffer + 0];
        JointState* parrStatusLayer = (JointState*)CBTemp[ac.m_SourceBuffer + 1];
        Vec2* parrWeightsLayer          = (Vec2*)CBTemp[ac.m_SourceBuffer + 2];
        if (parrWeightsLayer == 0)
        {
            return; //not initialized
        }
        //this is Source-Buffer No.2 and also the Target-Buffer
        QuatT* parrRelPoseBase          = (QuatT*)CBTemp[ac.m_TargetBuffer + 0];

        uint32 numJoints = state.m_jointCount;
        if (ac.m_BlendMode)
        {
            for (uint32 j = 0; j < numJoints; j++)
            {
                //Additive Blending
                if (parrStatusLayer[j] & eJS_Orientation)
                {
                    parrRelPoseBase[j].q = Quat::CreateNlerp(IDENTITY, parrRelPoseLayer[j].q, parrWeightsLayer[j].x) * parrRelPoseBase[j].q;
                }
                if (parrStatusLayer[j] & eJS_Position)
                {
                    parrRelPoseBase[j].t += Vec3::CreateLerp(ZERO, parrRelPoseLayer[j].t, parrWeightsLayer[j].y);
                }
            }
        }
        else
        {
            for (uint32 j = 0; j < numJoints; j++)
            {
                //Override Blending
                if (parrStatusLayer[j] & eJS_Orientation)
                {
                    parrRelPoseBase[j].q = (parrRelPoseBase[j].q * (1.0f - parrWeightsLayer[j].x) + parrRelPoseLayer[j].q * fsgnnz(parrRelPoseBase[j].q | parrRelPoseLayer[j].q)).GetNormalized(); //Quaternion LERP
                }
                if (parrStatusLayer[j] & eJS_Position)
                {
                    parrRelPoseBase[j].t = parrRelPoseBase[j].t * (1.0f - parrWeightsLayer[j].y) +  parrRelPoseLayer[j].t;//Vector LERP
                }
            }
        }
    }


    //used to playback the frames in a CAF-file which stored is in GlobalAnimationHeaderAIM
    void SamplePosePart::Execute(const CState& state, void* buffers[]) const
    {
        const SamplePosePart& ac = *this;
        void** CBTemp = buffers;

        // PARANOIA: ac.m_TargetBuffer is only ever supposed to have the values Command::TargetBuffer or Command::TmpBuffer
        // (Other commands do this with a flag instead of passing a possibly invalid index ...)
        CRY_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_TargetBuffer == Command::TmpBuffer);

        QuatT* parrRelPoseDst = (QuatT*)CBTemp[ac.m_TargetBuffer + 0];
        JointState* parrStatusDst = (JointState*)CBTemp[ac.m_TargetBuffer + 1];

        CAnimationSet* pAnimationSet = state.m_pDefaultSkeleton->m_pAnimationSet;
        const ModelAnimationHeader* pMAG = pAnimationSet->GetModelAnimationHeader(ac.m_nEAnimID);

        // PARANOIA: SamplePosePart is only called from locations that have already pulled the MAG and checked the asset type
        CRY_ASSERT(pMAG && pMAG->m_nAssetType == AIM_File);

        int32 nEGlobalID = pMAG->m_nGlobalAnimId;
        if (nEGlobalID < 0 || nEGlobalID >= g_AnimationManager.m_arrGlobalAIM.size())
        {
            // Header references an asset that doesn't exist
            return;
        }

        // cache vars to local pointers (spares using this pointer everytime)
        uint32 numJoints                = state.m_jointCount;
        CRY_ASSERT(ac.m_fAnimTime >= 0.0f && ac.m_fAnimTime <= 1.0f);


        GlobalAnimationHeaderAIM& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalAIM[nEGlobalID];

        const CDefaultSkeleton::SJoint* pModelJoint = &state.m_pDefaultSkeleton->m_arrModelJoints[0];

        //f32 fFadeColor[4] = {1,0,0,1};
        //if (nAdditiveAnimation)
        //  g_pIRenderer->Draw2dLabel( 1,300, 6.0f, fFadeColor, false,"SingleEvaluation" ); g_YLine+=0x18;

        // f32 fFadeColor[4] = {1,0,0,1};
        // g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.0f, fFadeColor, false,"SamplePosePart: %f",ac.m_fWeight); g_YLine+=0x18;

        //-------------------------------------------------------------------------------------
        //----             evaluate all controllers for this animation                    -----
        //-------------------------------------------------------------------------------------
        g_pCharacterManager->g_SkeletonUpdates++;

        f32 fKeyTimeNew = rGlobalAnimHeader.NTime2KTime(ac.m_fAnimTime);
        //override animations
        for (uint32 j = 1; j < numJoints; j++)
        {
            IController* pController = rGlobalAnimHeader.GetControllerByJointCRC32(pModelJoint[j].m_nJointCRC32);
            if (pController)
            {
                QuatT& RESTRICT_REFERENCE rRelPoseDst = parrRelPoseDst[j];
                JointState& RESTRICT_REFERENCE rStatusDst = parrStatusDst[j];
                Quat rot;
                Vec3 pos;
                JointState jointState = pController->GetOP(fKeyTimeNew, rot, pos);
                //Overwrite Mode
                if (jointState & eJS_Orientation)
                {
                    if (rStatusDst & eJS_Orientation)
                    {
                        rRelPoseDst.q.SetNlerp(rRelPoseDst.q, rot, ac.m_fWeight);
                    }
                    else
                    {
                        rRelPoseDst.q = rot;
                    }
                }
                if (jointState & eJS_Position)
                {
                    if (rStatusDst & eJS_Position)
                    {
                        rRelPoseDst.t.SetLerp(rRelPoseDst.t, pos, ac.m_fWeight);
                    }
                    else
                    {
                        rRelPoseDst.t = pos;
                    }
                }
                rStatusDst |= jointState;
            }
        }


#ifdef ANIM_ASSERT_ENABLED
        uint32 o = 0;
        uint32 p = 0;
        for (uint32 j = 0; j < numJoints; j++)
        {
            if (parrStatusDst[j] & eJS_Orientation)
            {
                ANIM_ASSERT(parrRelPoseDst[j].q.IsValid());
                o++;
            }
            if (parrStatusDst[j] & eJS_Position)
            {
                ANIM_ASSERT(parrRelPoseDst[j].t.IsValid());
                p++;
            }
        }
#endif
    }

    void PoseModifier::Execute(const CState& state, void* buffers[]) const
    {
        const PoseModifier& ac = *this;
        void** CBTemp = buffers;

        SAnimationPoseModifierParams params;
        params.pCharacterInstance = state.m_pInstance;
        params.pPoseData = state.m_pPoseData;
        params.timeDelta = state.m_pInstance->m_fOriginalDeltaTime;
        params.location = state.m_location;
        ac.m_pPoseModifier->Execute(params);
        /*
        #ifdef ANIM_ASSERT_ENABLED
            for (uint32 j=0; j<params.jointCount; j++)
            {
                ANIM_ASSERT( params.pPoseRelative[j].q.IsUnit() );
                ANIM_ASSERT( params.pPoseAbsolute[j].q.IsUnit() );
                ANIM_ASSERT( params.pPoseRelative[j].IsValid() );
                ANIM_ASSERT( params.pPoseAbsolute[j].IsValid() );
            }
        #endif
        */
    }



    void VerifyFull::Execute(const CState& state, void* buffers[]) const
    {
#ifdef ANIM_ASSERT_ENABLED
        const VerifyFull& ac = *this;
        void** CBTemp = buffers;

        ANIM_ASSERT(ac.m_TargetBuffer == Command::TargetBuffer || ac.m_TargetBuffer == Command::TmpBuffer);

        QuatT*          parrRelPoseDst  = (QuatT*)    CBTemp[ac.m_TargetBuffer + 0];
        JointState*     parrStatusDst       = (JointState*)  CBTemp[ac.m_TargetBuffer + 1];
        uint32 numJoints = state.m_jointCount;
        for (uint32 j = 0; j < numJoints; j++)
        {
            ANIM_ASSERT(parrRelPoseDst[j].q.IsValid());
            ANIM_ASSERT(parrRelPoseDst[j].t.IsValid());
        }
#endif
    }
} // namespace Command
