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

#include "stdafx.h"

#include "AnimationLoader.h"
#include "CompressionController.h"
#include "ControllerPQ.h"
#include "Util.h"

// Returns true if angular difference between two quaternions is bigger than
// the one specified by cosOfHalfMaxAngle.
// cosOfHalfMaxAngle == cosf(maxAngleDifferenceInRadians * 0.5f)
static inline bool error_quatCos(const Quat& q1, const Quat& q2, const float cosOfHalfMaxAngle)
{
    //  const float cosOfHalfAngle = fabsf(q1.v.x * q2.v.x + q1.v.y * q2.v.y + q1.v.z * q2.v.z + q1.v.w * q1.v.w);
    // Note about "1 - fabsf(" and "- 1)": q1 | q1 might return values > 1, so we map them to values < 1.
    const float cosOfHalfAngle = 1 - fabsf(fabsf(q1 | q2) - 1);
    return cosOfHalfAngle < cosOfHalfMaxAngle;
}


static inline bool error_vec3(const Vec3& v1, const Vec3& v2, const float maxAllowedDistanceSquared)
{
    const float x = v1.x - v2.x;
    const float y = v1.y - v2.y;
    const float z = v1.z - v2.z;

    const float distanceSquared = x * x + y * y + z * z;

    return distanceSquared > maxAllowedDistanceSquared;
}



bool ObtainTrackData(
    const GlobalAnimationHeaderCAF& header,
    uint32 c,
    uint32& curControllerId,
    std::vector<Quat>& Rotations,
    std::vector<Vec3>& Positions,
    std::vector<int>& RotTimes,
    std::vector<int>& PosTimes)
{
    Rotations.clear();
    Positions.clear();
    PosTimes.clear();
    RotTimes.clear();

    const CControllerPQLog* const pController = dynamic_cast<const CControllerPQLog*>(header.m_arrController[c].get());

    if (pController)
    {
        uint32 numTimes = pController->m_arrTimes.size();
        Rotations.reserve(numTimes);
        Positions.reserve(numTimes);
        PosTimes.reserve(numTimes);
        RotTimes.reserve(numTimes);

        curControllerId = pController->m_nControllerId;

        int oldtime = INT_MIN;

        for (uint32 i = 0; i < numTimes; ++i)
        {
            if (oldtime != pController->m_arrTimes[i])
            {
                oldtime = pController->m_arrTimes[i];

                Vec3 vRot = pController->m_arrKeys[i].vRotLog;

                if (vRot.x < -gf_PI || vRot.x > gf_PI)
                {
                    vRot.x = 0;
                }
                if (vRot.y < -gf_PI || vRot.y > gf_PI)
                {
                    vRot.y = 0;
                }
                if (vRot.z < -gf_PI || vRot.z > gf_PI)
                {
                    vRot.z = 0;
                }

                Vec3 vPos = pController->m_arrKeys[i].vPos;

                Rotations.push_back(!Quat::exp(vRot));
                Positions.push_back(vPos);

                RotTimes.push_back(pController->m_arrTimes[i]);
                PosTimes.push_back(pController->m_arrTimes[i]);
            }
        }
    }
    else
    {
        const CController* const pCController = dynamic_cast<const CController*>(header.m_arrController[c].get());

        if (!pCController)
        {
            assert(0);
            RCLogError("Unknown controller type");
            return false;
        }

        curControllerId = pCController->m_nControllerId;

        const IRotationController* const rotationController = pCController->GetRotationController().get();
        uint32 numRotKeys = rotationController->GetNumKeys();

        Rotations.resize(numRotKeys, Quat(IDENTITY));
        RotTimes.resize(numRotKeys);
        for (uint32 i = 0; i < numRotKeys; ++i)
        {
            rotationController->GetValueFromKey(i, Rotations[i]);
            RotTimes[i] = int(rotationController->GetTimeFromKey(i));
        }

        const IPositionController* const positionController = pCController->GetPositionController().get();
        uint32 numPosKeys = positionController->GetNumKeys();
        Positions.resize(numPosKeys, Vec3(ZERO));
        PosTimes.resize(numPosKeys);
        for (uint32 i = 0; i < numPosKeys; ++i)
        {
            positionController->GetValueFromKey(i, Positions[i]);
            PosTimes[i] = int(positionController->GetTimeFromKey(i));
        }
    }

    if (Rotations.empty() || Positions.empty())
    {
        assert(0);
        RCLogError("Unexpected empty positions and/or rotations");
        return false;
    }

    return true;
}

static bool IsDynamicController(
    const GlobalAnimationHeaderCAF& header,
    uint32 controllerIndex)
{
    const IController* const pController = header.m_arrController[controllerIndex].get();
    if (pController != nullptr)
    {
        return (pController->GetFlags() & IController::DYNAMIC_CONTROLLER) != 0;
    }

    return false;
}

static bool GetControllerId(
    const GlobalAnimationHeaderCAF& header,
    uint32 controllerIndex,
    uint32& controllerId)
{
    const CControllerPQLog* const pController = dynamic_cast<const CControllerPQLog*>(header.m_arrController[controllerIndex].get());

    if (pController)
    {
        controllerId = pController->m_nControllerId;
        return true;
    }
    else
    {
        const CController* const pCController = dynamic_cast<const CController*>(header.m_arrController[controllerIndex].get());

        if (!pCController)
        {
            assert(0);
            RCLogError("Unknown controller type");
            return false;
        }

        controllerId = pCController->m_nControllerId;
        return true;
    }

    return false;
}


static int FindBoneByControllerId(
    const GlobalAnimationHeaderCAF& header,
    const CSkeletonInfo* pSkeleton,
    const CInternalSkinningInfo* controllerSkinningInfo,
    const uint32 controllerId,
    const char*& pBoneName)
{
    pBoneName = 0;

    if (pSkeleton)
    {
        const int jointCount = (int)pSkeleton->m_SkinningInfo.m_arrBonesDesc.size();
        for (int b = 0; b < jointCount; ++b)
        {
            if (pSkeleton->m_SkinningInfo.m_arrBonesDesc[b].m_nControllerID == controllerId)
            {
                pBoneName = pSkeleton->m_SkinningInfo.m_arrBonesDesc[b].m_arrBoneName;
                return b;
            }
        }
    }

    if (controllerSkinningInfo)
    {
        const size_t controllerCount = header.m_arrController.size();
        const size_t nameCount = controllerSkinningInfo->m_arrBoneNameTable.size();

        if (controllerCount == nameCount)
        {
            for (uint32 controllerIndex = 0; controllerIndex < controllerCount; ++controllerIndex)
            {
                uint32 thisControllerId;
                if (GetControllerId(header, controllerIndex, thisControllerId) && controllerId == thisControllerId)
                {
                    pBoneName = controllerSkinningInfo->m_arrBoneNameTable[controllerIndex];
                    return controllerIndex;
                }
            }
        }
    }

    return -1;
}


static void PrintControllerInfo(
    const GlobalAnimationHeaderCAF& header,
    const CSkeletonInfo* pSkeleton,
    uint32 controllerIndex,
    const SPlatformAnimationSetup& platform,
    const CInternalSkinningInfo* controllerSkinningInfo,
    const SAnimationDesc& desc)
{
    if (controllerIndex >= header.m_arrController.size())
    {
        assert(0);
        return;
    }

    uint32 controllerId;
    if (!GetControllerId(header, controllerIndex, controllerId))
    {
        return;
    }

    const char* pBoneName = 0;
    const int boneNumber = FindBoneByControllerId(header, pSkeleton, controllerSkinningInfo, controllerId, pBoneName);

    const SBoneCompressionValues bc = desc.GetBoneCompressionValues(pBoneName, platform.m_compressionMultiplier);

    {
        const char* const pName = (pBoneName ? pBoneName : "<no name>");
        const char* pPos =
            (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Yes ? "Delete    " : (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_No ? "DontDelete" : "AutoDelete"));
        const char* pRot =
            (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Yes ? "Delete    " : (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_No ? "DontDelete" : "AutoDelete"));
        RCLog("[%3i] %s %s : %s", controllerIndex, pPos, pRot, pBoneName);
        RCLog(" pos:%20gx rot:%20gx", bc.m_compressPosTolerance, bc.m_compressRotToleranceInDegrees);
    }
}


void CCompressonator::CompressAnimation(
    const GlobalAnimationHeaderCAF& header,
    GlobalAnimationHeaderCAF& newheader,
    const CSkeletonInfo* pSkeleton,
    const SPlatformAnimationSetup& platform,
    const SAnimationDesc& desc,
    const CInternalSkinningInfo* controllerSkinningInfo,
    bool bDebugCompression)
{
    if (bDebugCompression)
    {
        RCLog("Bone compression (%s) for %s:", (desc.m_bAdditiveAnimation ? "additive" : "override"), header.GetFilePath());
    }

    const uint32 controllerCount = header.m_arrController.size();

    std::vector<Operations> ops;
    {
        Operations o;
        if (!desc.m_bNewFormat && desc.format.oldFmt.m_CompressionQuality == 0)
        {
            o.m_ePosOperation = eOperation_DontCompress;
            o.m_eRotOperation = eOperation_DontCompress;
        }
        else
        {
            o.m_ePosOperation = eOperation_Compress;
            o.m_eRotOperation = eOperation_Compress;
        }

        ops.resize(controllerCount, o);
    }

    // Find deletable channels and modify ops[] accordingly
    for (uint32 c = 0; c < controllerCount; ++c)
    {
        if (c == 0 && !desc.m_bAdditiveAnimation)
        {
            // FIXME: why do we skip controller 0?
            continue;
        }

        const bool bOk = desc.m_bAdditiveAnimation
            ? ComputeControllerDelete_Additive(ops[c], header, pSkeleton, c, platform, desc, controllerSkinningInfo, bDebugCompression)
            : ComputeControllerDelete_Override(ops[c], header, pSkeleton, c, platform, desc, controllerSkinningInfo, bDebugCompression);

        if (!bOk)
        {
            RCLogError("Pre-compression failed");
            return;
        }
    }

    // Compress/delete
    for (uint32 c = 0; c < controllerCount; ++c)
    {
        if (c == 0 && !desc.m_bAdditiveAnimation)
        {
            // FIXME: why do we skip controller 0?
            continue;
        }

        const bool bOk = desc.m_bAdditiveAnimation
            ? CompressController_Additive(ops[c], header, newheader, pSkeleton, c, platform, desc, controllerSkinningInfo, bDebugCompression)
            : CompressController_Override(ops[c], header, newheader, pSkeleton, c, platform, desc, controllerSkinningInfo, bDebugCompression);

        if (!bOk)
        {
            RCLogError("Compression failed");
            return;
        }
    }
}


bool CCompressonator::ComputeControllerDelete_Override(
    Operations& operations,
    const GlobalAnimationHeaderCAF& header,
    const CSkeletonInfo* pSkeleton,
    uint32 controllerIndex,
    const SPlatformAnimationSetup& platform,
    const SAnimationDesc& desc,
    const CInternalSkinningInfo* controllerSkinningInfo,
    bool bDebugCompression)
{
    if (controllerIndex >= header.m_arrController.size())
    {
        assert(0);
        return false;
    }

    if (bDebugCompression)
    {
        PrintControllerInfo(header, pSkeleton, controllerIndex, platform, controllerSkinningInfo, desc);
    }

    if (operations.m_ePosOperation == eOperation_Delete &&
        operations.m_eRotOperation == eOperation_Delete)
    {
        return true;
    }

    uint32 controllerId;
    if (!ObtainTrackData(header, controllerIndex, controllerId, m_Rotations, m_Positions, m_RotTimes, m_PosTimes))
    {
        return false;
    }

    const Quat firstKeyRot = m_Rotations[0];
    const Vec3 firstKeyPos = m_Positions[0];

    const char* pBoneName = 0;
    const int boneNumber = FindBoneByControllerId(header, pSkeleton, controllerSkinningInfo, controllerId, pBoneName);

    SBoneCompressionValues bc = desc.GetBoneCompressionValues(pBoneName, platform.m_compressionMultiplier);

    if (!pBoneName)
    {
        if (IsDynamicController(header, controllerIndex))
        {
            bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_No;
            bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_No;
            return true;
        }

        static const uint32 controllerId_Locator_Locomotion = 0x06C9327A;  // CRC32 of "Locator_Locomotion"
        if (controllerId != controllerId_Locator_Locomotion)
        {
            RCLogWarning("Missing joint in skeleton, controller '%s' (id 0x%08X) will be removed.", header.GetOriginalControllerName(controllerId), controllerId);
        }

        bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_Yes;
        bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_Yes;
    }

    if (boneNumber == 0)
    {
        if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Auto)
        {
            bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_No;
        }
        if (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Auto)
        {
            bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_No;
        }
    }

    if (operations.m_ePosOperation == eOperation_Delete)
    {
        bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_Yes;
    }
    if (operations.m_eRotOperation == eOperation_Delete)
    {
        bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_Yes;
    }

    if (pSkeleton->m_SkinningInfo.m_arrBonesDesc.empty())
    {
        // Can't delete keys if no bind pose is available.
        bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_No;
        bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_No;
    }

    if (desc.m_bNewFormat)
    {
        if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Auto || bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Auto)
        {
            QuatT relRigValue = QuatT(pSkeleton->m_SkinningInfo.m_arrBonesDesc[boneNumber].m_DefaultB2W);
            {
                const int32 offset = pSkeleton->m_SkinningInfo.m_arrBonesDesc[boneNumber].m_nOffsetParent;
                const int32 pidx = boneNumber + offset;
                assert(pidx > -1);
                const Matrix34 p34 = pSkeleton->m_SkinningInfo.m_arrBonesDesc[ pidx ].m_DefaultB2W;
                const Matrix34 c34 = pSkeleton->m_SkinningInfo.m_arrBonesDesc[ boneNumber ].m_DefaultB2W;
                relRigValue = QuatT(p34.GetInverted() * c34);
            }

            if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Auto)
            {
                bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_Yes;
                const float maxDistSquared = bc.m_autodeletePosEps * bc.m_autodeletePosEps;
                const uint32 numPosKeys = m_Positions.size();
                for (uint32 i = 0; i < numPosKeys; ++i)
                {
                    if (error_vec3(relRigValue.t, m_Positions[i], maxDistSquared))
                    {
                        bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_No;
                        break;
                    }
                }
            }

            if (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Auto)
            {
                bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_Yes;
                const float maxAngleInRadians = Util::getClamped(DEG2RAD(bc.m_autodeleteRotEps), 0.0f, (float)gf_PI);
                const float cosOfHalfMaxAngle = cosf(maxAngleInRadians * 0.5f);
                const uint32 numRotKeys = m_Rotations.size();
                for (uint32 i = 0; i < numRotKeys; ++i)
                {
                    if (error_quatCos(relRigValue.q, m_Rotations[i], cosOfHalfMaxAngle))
                    {
                        bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_No;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Auto)
        {
            //are all rotation-keys nearly identical?
            const uint32 numRotKeys = m_Rotations.size();

            // FIXME: handle new compression parameters format also
            assert(!desc.m_bNewFormat);

            for (uint32 i = 1; i < numRotKeys; ++i)
            {
                if (!IsQuatEquivalent_dot(firstKeyRot, m_Rotations[i], bc.m_autodeleteRotEps))
                {
                    //rot-channel has animation, so keep it
                    bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_No;
                    break;
                }
            }
        }

        if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Auto)
        {
            //are all position-keys nearly identical?
            f32 pos_epsilon = bc.m_autodeletePosEps;
            {
                const int32 offset = pSkeleton->m_SkinningInfo.m_arrBonesDesc[boneNumber].m_nOffsetParent;
                const int32 pidx = boneNumber + offset;
                assert(pidx > -1);
                const Vec3 p34 = pSkeleton->m_SkinningInfo.m_arrBonesDesc[    pidx    ].m_DefaultB2W.GetTranslation();
                const Vec3 c34 = pSkeleton->m_SkinningInfo.m_arrBonesDesc[ boneNumber ].m_DefaultB2W.GetTranslation();
                const Vec3 BoneSegment = p34 - c34;
                pos_epsilon = (BoneSegment * bc.m_autodeletePosEps).GetLength();
                pos_epsilon = Util::getMin(pos_epsilon, 0.02f);
            }

            const uint32 numPosKeys = m_Positions.size();
            for (uint32 i = 1; i < numPosKeys; ++i)
            {
                // Note: this check does per-component comparison (i.e. |dx| <= epsilon)
                if (!firstKeyPos.IsEquivalent(m_Positions[i], pos_epsilon))
                {
                    //pos-channel has animation, so keep it
                    bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_No;
                    break;
                }
            }
        }

        if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Auto || bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Auto)
        {
            // compare with reference model (rig)

            f32 pos_epsilon = bc.m_autodeletePosEps;
            QuatT relRigValue = QuatT(pSkeleton->m_SkinningInfo.m_arrBonesDesc[boneNumber].m_DefaultB2W);
            {
                const int32 offset = pSkeleton->m_SkinningInfo.m_arrBonesDesc[boneNumber].m_nOffsetParent;
                const int32 pidx = boneNumber + offset;
                assert(pidx > -1);
                const Matrix34 p34 = pSkeleton->m_SkinningInfo.m_arrBonesDesc[ pidx ].m_DefaultB2W;
                const Matrix34 c34 = pSkeleton->m_SkinningInfo.m_arrBonesDesc[ boneNumber ].m_DefaultB2W;
                relRigValue = QuatT(p34.GetInverted() * c34);
                pos_epsilon = (relRigValue.t * bc.m_autodeletePosEps).GetLength();
                pos_epsilon = Util::getMin(pos_epsilon, 0.02f);
            }

            //is this rotation-channel identical with rig?
            if (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Auto)
            {
                if (IsQuatEquivalent_dot(firstKeyRot, relRigValue.q, bc.m_autodeleteRotEps))
                {
                    bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_Yes;
                }
                else
                {
                    bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_No;
                }
            }

            //is this position-channel identical with rig?
            if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Auto)
            {
                // Note: this check does per-component comparison (i.e. |dx| <= epsilon)
                if (firstKeyPos.IsEquivalent(relRigValue.t, Util::getMax(pos_epsilon, 0.0001f)))
                {
                    bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_Yes;
                }
                else
                {
                    bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_No;
                }
            }
        }
    }

    if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Yes)
    {
        operations.m_ePosOperation = eOperation_Delete;
    }

    if (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Yes)
    {
        operations.m_eRotOperation = eOperation_Delete;
    }

    return true;
}


bool CCompressonator::CompressController_Override(
    const Operations& operations,
    const GlobalAnimationHeaderCAF& header,
    GlobalAnimationHeaderCAF& newheader,
    const CSkeletonInfo* pSkeleton,
    uint32 controllerIndex,
    const SPlatformAnimationSetup& platform,
    const SAnimationDesc& desc,
    const CInternalSkinningInfo* controllerSkinningInfo,
    bool bDebugCompression)
{
    if (!pSkeleton || controllerIndex >= header.m_arrController.size())
    {
        RCLogError("Error: Invalid skeleton or controller specified for animation %s!", header.GetFilePath());
        return false;
    }
    
    if (operations.m_ePosOperation == eOperation_Delete &&
        operations.m_eRotOperation == eOperation_Delete)
    {
        newheader.m_arrController[controllerIndex] = 0;
        return true;
    }

    uint32 controllerId;
    if (!ObtainTrackData(header, controllerIndex, controllerId, m_Rotations, m_Positions, m_RotTimes, m_PosTimes))
    {
        return false;
    }

    const char* pBoneName = 0;
    const int boneNumber = FindBoneByControllerId(header, pSkeleton, controllerSkinningInfo, controllerId, pBoneName);
    if ((boneNumber < 0 || pBoneName == 0) && !IsDynamicController(header, controllerIndex))
    {
        RCLogWarning("Warning: Could not find bone corresponding to ID %i in skeleton for animation %s", controllerId, header.GetFilePath());
        return false;
    }

    const SBoneCompressionValues bc = desc.GetBoneCompressionValues(pBoneName, platform.m_compressionMultiplier);

    if (header.m_arrController[controllerIndex]->GetID() != controllerId)
    {
        assert(0);
        RCLogError("controller id mismatch");
        return false;
    }

    CompressController(
        bc.m_compressPosTolerance, bc.m_compressRotToleranceInDegrees,
        operations,
        newheader,
        controllerIndex,
        header.m_arrController[controllerIndex]->GetID(),
        header.m_arrController[controllerIndex]->GetFlags());
    return true;
}


bool CCompressonator::ComputeControllerDelete_Additive(
    Operations& operations,
    const GlobalAnimationHeaderCAF& header,
    const CSkeletonInfo* pSkeleton,
    uint32 controllerIndex,
    const SPlatformAnimationSetup& platform,
    const SAnimationDesc& desc,
    const CInternalSkinningInfo* controllerSkinningInfo,
    bool bDebugCompression)
{
    if (controllerIndex >= header.m_arrController.size())
    {
        assert(0);
        return false;
    }

    if (bDebugCompression)
    {
        PrintControllerInfo(header, pSkeleton, controllerIndex, platform, controllerSkinningInfo, desc);
    }

    if (operations.m_ePosOperation == eOperation_Delete &&
        operations.m_eRotOperation == eOperation_Delete)
    {
        return true;
    }
    
    uint32 controllerId;
    if (!ObtainTrackData(header, controllerIndex, controllerId, m_Rotations, m_Positions, m_RotTimes, m_PosTimes))
    {
        return false;
    }

    const char* pBoneName = 0;
    const int boneNumber = FindBoneByControllerId(header, pSkeleton, controllerSkinningInfo, controllerId, pBoneName);

    SBoneCompressionValues bc;
    if (controllerIndex > 0 && boneNumber >= 0 && pBoneName)
    {
        bc = desc.GetBoneCompressionValues(pBoneName, platform.m_compressionMultiplier);
    }
    else
    {
        RCLogWarning("Missing joint in skeleton, controller '%s' (id 0x%08X) will be removed.", header.GetOriginalControllerName(controllerId), controllerId);
        bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_Yes;
        bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_Yes;
    }

    // By a convention, we will compute additive animation as difference between
    // a key and the first key

    if (bc.m_eAutodeleteRot != SBoneCompressionValues::eDelete_Yes && m_Rotations.size() <= 1)
    {
        assert(0);
        RCLogWarning("Found a *single-frame* additive animation (rotation) - it is not allowed");
        bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_Yes;
    }

    if (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Auto)
    {
        bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_Yes;

        const uint32 numRotKeys  = m_Rotations.size();

        const Quat qI(IDENTITY);

        if (desc.m_bNewFormat)
        {
            const float maxAngleInRadians = Util::getClamped(DEG2RAD(bc.m_autodeleteRotEps), 0.0f, (float)gf_PI);
            const float cosOfHalfMaxAngle = cosf(maxAngleInRadians * 0.5f);
            for (uint32 k = 1; k < numRotKeys; ++k)
            {
                const Quat q = m_Rotations[k] * !m_Rotations[0];
                if (error_quatCos(q, qI, cosOfHalfMaxAngle))
                {
                    bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_No;
                    break;
                }
            }
        }
        else
        {
            for (uint32 k = 1; k < numRotKeys; ++k)
            {
                const Quat q = m_Rotations[k] * !m_Rotations[0];
                const f32 dot = fabsf((qI | q) - 1.0f);
                if (dot > bc.m_autodeleteRotEps)
                {
                    bc.m_eAutodeleteRot = SBoneCompressionValues::eDelete_No;
                    break;
                }
            }
        }
    }

    if (bc.m_eAutodeletePos != SBoneCompressionValues::eDelete_Yes && m_Positions.size() <= 1)
    {
        assert(0);
        RCLogWarning("Found a *single-frame* additive animation (position) - it is not allowed");
        bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_Yes;
    }

    if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Auto)
    {
        bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_Yes;
        const uint32 numPosKeys  = m_Positions.size();

        if (desc.m_bNewFormat)
        {
            const float maxDistSquared = bc.m_autodeletePosEps * bc.m_autodeletePosEps;
            for (uint32 k = 1; k < numPosKeys; ++k)
            {
                if (error_vec3(m_Positions[0], m_Positions[k], maxDistSquared))
                {
                    bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_No;
                    break;
                }
            }
        }
        else
        {
            const f32 fLength = m_Positions[0].GetLength();
            for (uint32 k = 1; k < numPosKeys; ++k)
            {
                const Vec3 p = m_Positions[k] - m_Positions[0];
                if (p.GetLength() > fLength * bc.m_autodeletePosEps)
                {
                    bc.m_eAutodeletePos = SBoneCompressionValues::eDelete_No;
                    break;
                }
            }
        }
    }

    if (bc.m_eAutodeletePos == SBoneCompressionValues::eDelete_Yes)
    {
        operations.m_ePosOperation = eOperation_Delete;
    }
    if (bc.m_eAutodeleteRot == SBoneCompressionValues::eDelete_Yes)
    {
        operations.m_eRotOperation = eOperation_Delete;
    }

    return true;
}


bool CCompressonator::CompressController_Additive(
    const Operations& operations,
    const GlobalAnimationHeaderCAF& header,
    GlobalAnimationHeaderCAF& newheader,
    const CSkeletonInfo* pSkeleton,
    uint32 controllerIndex,
    const SPlatformAnimationSetup& platform,
    const SAnimationDesc& desc,
    const CInternalSkinningInfo* controllerSkinningInfo,
    bool bDebugCompression)
{
    if (controllerIndex >= header.m_arrController.size())
    {
        assert(0);
        return false;
    }

    if (operations.m_ePosOperation == eOperation_Delete &&
        operations.m_eRotOperation == eOperation_Delete)
    {
        newheader.m_arrController[controllerIndex] = 0;
        return true;
    }

    uint32 controllerId;
    if (!ObtainTrackData(header, controllerIndex, controllerId, m_Rotations, m_Positions, m_RotTimes, m_PosTimes))
    {
        return false;
    }

    const char* pBoneName = 0;
    const int boneNumber = FindBoneByControllerId(header, pSkeleton, controllerSkinningInfo, controllerId, pBoneName);
    if (boneNumber < 0 || pBoneName == 0)
    {
        assert(0);
        return false;
    }

    // By a convention, we will compute additive animation as difference between
    // a key and the first key. Note that after such computation the first frame
    // will always contain zeros. So after computation we delete the first frame
    // (animators are expected to be aware of that).
    if (operations.m_eRotOperation != eOperation_Delete)
    {
        const uint32 numRotKeys  = m_Rotations.size();
        for (uint32 k = 1; k < numRotKeys; k++)
        {
            m_Rotations[k] = m_Rotations[k] * !m_Rotations[0];
        }
        m_RotTimes.erase(m_RotTimes.begin());
        m_Rotations.erase(m_Rotations.begin());
    }
    if (operations.m_ePosOperation != eOperation_Delete)
    {
        const uint32 numPosKeys  = m_Positions.size();
        for (uint32 k = 1; k < numPosKeys; k++)
        {
            m_Positions[k] = m_Positions[k] - m_Positions[0];
        }
        m_PosTimes.erase(m_PosTimes.begin());
        m_Positions.erase(m_Positions.begin());
    }

    const SBoneCompressionValues bc = desc.GetBoneCompressionValues(pBoneName, platform.m_compressionMultiplier);

    if (header.m_arrController[controllerIndex]->GetID() != controllerId)
    {
        assert(0);
        RCLogError("controller id mismatch");
        return false;
    }

    CompressController(
        bc.m_compressPosTolerance, bc.m_compressRotToleranceInDegrees,
        operations,
        newheader,
        controllerIndex,
        header.m_arrController[controllerIndex]->GetID(),
        header.m_arrController[controllerIndex]->GetFlags());
    return true;
}


void CCompressonator::CompressController(
    float positionTolerance,
    float rotationToleranceInDegrees,
    const Operations& operations,
    GlobalAnimationHeaderCAF& newheader,
    uint32 controllerIndex,
    uint32 controllerId,
    uint32 flags)
{
    if (operations.m_ePosOperation == eOperation_Delete &&
        operations.m_eRotOperation == eOperation_Delete)
    {
        newheader.m_arrController[controllerIndex] = 0;
        return;
    }

    const ECompressionFormat ePositionFormat = eNoCompressVec3;
    ECompressionFormat eRotationFormat;
    if (rotationToleranceInDegrees == 0)
    {
        eRotationFormat = eSmallTree64BitExtQuat;
    }
    else
    {
        eRotationFormat = eAutomaticQuat;
    }

    // We will try few rotation formats and choose one that produces smallest size
    ECompressionFormat formats[] =
    {
        eNoCompressQuat,
        eSmallTree48BitQuat,
        eSmallTree64BitExtQuat
    };
    uint32 formatCount = sizeof(formats) / sizeof(formats[0]);
    if (eRotationFormat != eAutomaticQuat)
    {
        formatCount = 1;
        formats[0] = eRotationFormat;
    }

    std::vector<_smart_ptr<CController> > newControllers(formatCount);

    uint32 minSize = -1;
    uint32 bestIndex;

    for (uint32 formatIndex = 0; formatIndex < formatCount; ++formatIndex)
    {
        newControllers[formatIndex] = new CController;
        newControllers[formatIndex]->m_nControllerId = controllerId;
        newControllers[formatIndex]->m_nFlags = flags;

        if (operations.m_eRotOperation != eOperation_Delete)
        {
            CompressRotations(
                newControllers[formatIndex],
                formats[formatIndex],
                (operations.m_eRotOperation == eOperation_Compress),
                rotationToleranceInDegrees);
        }

        if (operations.m_ePosOperation != eOperation_Delete)
        {
            // Note: position format is the same for each loop iteration, so we need to compress it only once
            if (formatIndex == 0)
            {
                CompressPositions(
                    newControllers[formatIndex],
                    ePositionFormat,
                    (operations.m_ePosOperation == eOperation_Compress),
                    positionTolerance);
            }
            else
            {
                newControllers[formatIndex]->SetPositionController(newControllers[0]->GetPositionController());
            }
        }

        const uint32 currentSize = newControllers[formatIndex]->SizeOfThis();
        if (currentSize < minSize)
        {
            minSize = currentSize;
            bestIndex = formatIndex;
        }
    }

    newheader.m_arrController[controllerIndex] = newControllers[bestIndex];
}


static BaseCompressedQuat* GetCompressedQuat(ECompressionFormat format)
{
    switch (format)
    {
    case eNoCompress:
    case eNoCompressQuat:
        return new TNoCompressedQuat();

    case eShotInt3Quat:
        return new TShortInt3CompressedQuat();

    case eSmallTreeDWORDQuat:
        return new TSmallTreeDWORDCompressedQuat();

    case eSmallTree48BitQuat:
        return new TSmallTree48BitCompressedQuat();

    case eSmallTree64BitQuat:
        return new TSmallTree64BitCompressedQuat();

    case eSmallTree64BitExtQuat:
        return new TSmallTree64BitExtCompressedQuat();

    case ePolarQuat:
        return new TPolarQuatQuat();
    }

    return nullptr;
}

static BaseCompressedVec3* GetCompressedVec3(ECompressionFormat format)
{
    switch (format)
    {
    case eNoCompressVec3:
    default:
        return new TBaseCompressedVec3();
    }

    return nullptr;
}

void CCompressonator::CompressRotations(CController* pController, ECompressionFormat rotationFormat, bool bRemoveKeys, float rotationToleranceInDegrees)
{
    const uint32 numKeys = m_Rotations.size();
    assert(numKeys > 0);

    KeyTimesInformationPtr pTimes;// = new F32KeyTimesInformation;//CKeyTimesInformation;

    if (m_RotTimes[0] >= 0 && m_RotTimes[numKeys - 1] < 256)
    {
        pTimes = new ByteKeyTimesInformation;
    }
    else if (m_RotTimes[0] >= 0 && m_RotTimes[numKeys - 1] < 65536)
    {
        pTimes = new UINT16KeyTimesInformation;
    }
    else
    {
        pTimes = new F32KeyTimesInformation;
    }

    RotationControllerPtr pNewTrack  =  RotationControllerPtr(new RotationTrackInformation);
    TrackRotationStoragePtr pStorage = ControllerHelper::GetRotationControllerPtr(rotationFormat);
    pNewTrack->SetRotationStorage(pStorage);

    pNewTrack->SetKeyTimesInformation(pTimes);
    pController->SetRotationController(pNewTrack);

    unsigned int iFirstKey = 0;

    pTimes->AddKeyTime(f32(m_RotTimes[iFirstKey]));
    pStorage->AddValue(m_Rotations[iFirstKey]);

    if (bRemoveKeys)
    {
        // note that GetCompressedQuat returns a c-array allocated with new[], so it must be deleted with delete[].
        BaseCompressedQuat* compressed = GetCompressedQuat(rotationFormat);
        AZ_Assert(compressed, "Could not get compressed quaterian object for rotation format");
        Quat first, last, interpolated;

        const float maxAngleInRadians = Util::getClamped(DEG2RAD(rotationToleranceInDegrees), 0.0f, (float)gf_PI);
        const float cosOfHalfMaxAngle = cosf(maxAngleInRadians * 0.5f);

        while (iFirstKey + 2 < numKeys)
        {
            compressed->FromQuat(m_Rotations[iFirstKey]);
            first = compressed->ToQuat();

            unsigned int iLastKey;
            for (iLastKey = iFirstKey + 2; iLastKey < numKeys; ++iLastKey)
            {
                compressed->FromQuat(m_Rotations[iLastKey]);
                last = compressed->ToQuat();

                const unsigned int count = iLastKey - iFirstKey;
                for (unsigned int i = 1; i < count; ++i)
                {
                    interpolated.SetNlerp(first, last, i / (float)count);

                    if (error_quatCos(interpolated, m_Rotations[iFirstKey + i], cosOfHalfMaxAngle))
                    {
                        goto addKey;
                    }
                }
            }
addKey:
            iFirstKey = iLastKey - 1;
            pTimes->AddKeyTime(f32(m_RotTimes[iFirstKey]));
            pStorage->AddValue(m_Rotations[iFirstKey]);
        }

        delete compressed;
    }

    for (++iFirstKey; iFirstKey < numKeys; ++iFirstKey)
    {
        pTimes->AddKeyTime(f32(m_RotTimes[iFirstKey]));
        pStorage->AddValue(m_Rotations[iFirstKey]);
    }
}

void CCompressonator::CompressPositions(CController* pController, ECompressionFormat positionFormat, bool bRemoveKeys, float positionTolerance)
{
    const uint32 numKeys = m_Positions.size();
    assert(numKeys > 0);

    KeyTimesInformationPtr pTimes;// = new F32KeyTimesInformation;//CKeyTimesInformation;
    if (m_PosTimes[0] >= 0 && m_PosTimes[numKeys - 1] < 256)
    {
        pTimes = new ByteKeyTimesInformation;
    }
    else if (m_PosTimes[0] >= 0 && m_PosTimes[numKeys - 1] < 65536)
    {
        pTimes = new UINT16KeyTimesInformation;
    }
    else
    {
        pTimes = new F32KeyTimesInformation;
    }

    //  PositionControllerPtr pNewTrack = ControllerHelper::GetPositionControllerPtr(positionFormat);
    PositionControllerPtr pNewTrack = PositionControllerPtr(new PositionTrackInformation);
    TrackPositionStoragePtr pStorage = ControllerHelper::GetPositionControllerPtr(positionFormat);

    pNewTrack->SetPositionStorage(pStorage);

    //GetCompressedPositionTrack(positionFormat);
    pNewTrack->SetKeyTimesInformation(pTimes);
    pController->SetPositionController(pNewTrack);

    unsigned int iFirstKey = 0;

    pTimes->AddKeyTime(f32(m_PosTimes[iFirstKey]));
    pStorage->AddValue(m_Positions[iFirstKey]);

    if (bRemoveKeys)
    {
        BaseCompressedVec3* compressed = GetCompressedVec3(positionFormat);
        AZ_Assert(compressed, "Could not get compressed vector3 object for position format");

        Vec3 first, last, interpolated;

        const float squaredPositionTolerance = positionTolerance * positionTolerance;

        while (iFirstKey + 2 < numKeys)
        {
            compressed->FromVec3(m_Positions[iFirstKey]);
            first = compressed->ToVec3();

            unsigned int iLastKey;
            for (iLastKey = iFirstKey + 2; iLastKey < numKeys; ++iLastKey)
            {
                compressed->FromVec3(m_Positions[iLastKey]);
                last = compressed->ToVec3();

                const unsigned int count = iLastKey - iFirstKey;
                for (unsigned int i = 1; i < count; ++i)
                {
                    interpolated.SetLerp(first, last, i / (float)count);

                    if (error_vec3(interpolated, m_Positions[iFirstKey + i], squaredPositionTolerance))
                    {
                        goto addKey;
                    }
                }
            }
addKey:
            iFirstKey = iLastKey - 1;
            pTimes->AddKeyTime(f32(m_PosTimes[iFirstKey]));
            pStorage->AddValue(m_Positions[iFirstKey]);
        }

        delete compressed;
    }

    for (++iFirstKey; iFirstKey < numKeys; ++iFirstKey)
    {
        pTimes->AddKeyTime(f32(m_PosTimes[iFirstKey]));
        pStorage->AddValue(m_Positions[iFirstKey]);
    }
}
