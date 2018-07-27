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

#include "FileUtil.h"
#include "AnimationLoader.h"
#include "CompressionController.h"
#include "AnimationManager.h"
#include "TrackStorage.h"
#include "CryCrc32.h"
#include "AnimationCompiler.h"
#include "CGF/ChunkData.h"
#include "CGF/CAFSaver.h"
#include "FileUtil.h"

CAnimationCompressor::CAnimationCompressor(const CInternalSkinningInfo* skinningInfo)
{
    AZ_Assert(skinningInfo, "Invalid skinning information provided.");

    m_skinningInfo = skinningInfo;
    m_bAlignTracks = false;

    // Ctor for version of compressor without skeleton.
}

CAnimationCompressor::CAnimationCompressor(const CSkeletonInfo& skeleton)
{
    // copy skeleton, we are going to change root
    m_skeleton = skeleton;

    CSkinningInfo& info = m_skeleton.m_SkinningInfo;
    if (info.m_arrBonesDesc.size() >= 2)
    {
        info.m_arrBonesDesc[0].m_DefaultB2W.SetIdentity();
        info.m_arrBonesDesc[0].m_DefaultW2B.SetIdentity();
        info.m_arrBonesDesc[1].m_DefaultW2B = info.m_arrBonesDesc[0].m_DefaultB2W.GetInverted();
    }
    else
    {
        RCLogWarning("Suspicious number of bones in skeleton: %d", int(info.m_arrBonesDesc.size()));
    }

    m_bAlignTracks = false;
    m_skinningInfo = nullptr;
}

CAnimationCompressor::~CAnimationCompressor()
{
}


bool CAnimationCompressor::LoadCAF(const char* name, const SPlatformAnimationSetup& platform, const SAnimationDesc& animDesc, const CInternalSkinningInfo* controllerSkinningInfo, ECAFLoadMode loadMode)
{
    if (m_GlobalAnimationHeader.LoadCAF(name, loadMode, &m_ChunkFile))
    {
        m_platformSetup = platform;
        LoadCAFPostProcess(name, animDesc, controllerSkinningInfo, loadMode);
        return true;
    }
    return false;
}

//
// Implemented by referencing to CAnimationCompressor::LoadCAF
//
bool CAnimationCompressor::LoadCAFFromMemory(const char* name, const SAnimationDesc& animDesc, ECAFLoadMode loadMode, CContentCGF* content, CInternalSkinningInfo* controllerSkinningInfo)
{
    // Convert in-memory animation data into chunk data
    CSaverCAF cafSaver(name, m_ChunkFile);
    cafSaver.Save(content, controllerSkinningInfo);

    // Load chunk data into animation header
    if (m_GlobalAnimationHeader.LoadCAFFromChunkFile(name, loadMode, &m_ChunkFile))
    {
        LoadCAFPostProcess(name, animDesc, controllerSkinningInfo, loadMode);
        return true;
    }
    return false;
}

void CAnimationCompressor::LoadCAFPostProcess(const char* name, const SAnimationDesc& animDesc, const CInternalSkinningInfo* controllerSkinningInfo, ECAFLoadMode loadMode)
{
    m_filename = name;
    m_LastGoodAnimation = m_CompressedAnimation = m_GlobalAnimationHeader;
    m_AnimDesc = animDesc;
    m_skinningInfo = controllerSkinningInfo;

    if (loadMode == eCAFLoadUncompressedOnly)
    {
        DeleteCompressedChunks();
    }
}

uint32 CAnimationCompressor::AddCompressedChunksToFile(const char* name, bool bigEndianOutput)
{
    FILETIME oldTimeStamp = FileUtil::GetLastWriteFileTime(name);

    CSaverCGF cgfSaver(m_ChunkFile);
    uint32 numChunks = m_ChunkFile.NumChunks();

    //save information here

    SaveMotionParameters(&m_ChunkFile, bigEndianOutput);
    SaveControllers(cgfSaver, m_GlobalAnimationHeader.m_arrController, bigEndianOutput);
#if defined(AZ_PLATFORM_WINDOWS)
    SetFileAttributes(name, FILE_ATTRIBUTE_ARCHIVE);
#endif
    m_ChunkFile.Write(name);

    FileUtil::SetFileTimes(name, oldTimeStamp);

    const __int64 fileSize = FileUtil::GetFileSize(name);
    if (fileSize < 0)
    {
        RCLogError("Failed to get file size of '%s'", name);
        return ~0;
    }

    if (fileSize > 0xffFFffFFU)
    {
        RCLogError("Unexpected huge file '%s' found", name);
        return ~0;
    }

    return (uint32)fileSize;
}


uint32 CAnimationCompressor::SaveOnlyCompressedChunksInFile(const char* name, FILETIME timeStamp, GlobalAnimationHeaderAIM* gaim, bool saveCAFHeader, bool bigEndianFormat)
{
    CChunkFile chunkFile;
    CSaverCGF cgfSaver(chunkFile);

    //save information here

    SaveMotionParameters(&chunkFile, bigEndianFormat);
    const uint32 expectedControlledCount = m_GlobalAnimationHeader.m_nCompressedControllerCount;
    m_GlobalAnimationHeader.m_nCompressedControllerCount = SaveControllers(cgfSaver, m_GlobalAnimationHeader.m_arrController, bigEndianFormat);
    if (expectedControlledCount != m_GlobalAnimationHeader.m_nCompressedControllerCount)
    {
        if (expectedControlledCount > m_GlobalAnimationHeader.m_nCompressedControllerCount)
        {
            RCLogError("Unexpected empty controllers (%u -> %u)", expectedControlledCount, m_GlobalAnimationHeader.m_nCompressedControllerCount);
        }
        else
        {
            RCLogError("Weird failure: controllers %u -> %u", expectedControlledCount, m_GlobalAnimationHeader.m_nCompressedControllerCount);
        }
    }
    if (gaim)
    {
        gaim->SaveToChunkFile(&chunkFile, bigEndianFormat);
    }
    if (saveCAFHeader)
    {
        m_GlobalAnimationHeader.SaveToChunkFile(&chunkFile, bigEndianFormat);
    }
#if defined(AZ_PLATFORM_WINDOWS)
    SetFileAttributes(name, FILE_ATTRIBUTE_ARCHIVE);
#endif
    chunkFile.Write(name);

    FileUtil::SetFileTimes(name, timeStamp);

    const __int64 fileSize = FileUtil::GetFileSize(name);
    if (fileSize < 0)
    {
        RCLogError("Failed to get file size of '%s'", name);
        return ~0;
    }

    if (fileSize > 0xffFFffFFU)
    {
        RCLogError("Unexpected huge file '%s' found", name);
        return ~0;
    }

    return (uint32)fileSize;
}


// Modified version of AnalyseAndModifyAnimations function
bool CAnimationCompressor::ProcessAnimation(bool debugCompression)
{
    if (m_skinningInfo)
    {
        return ProcessAnimationFBX(debugCompression);
    }
    else
    {
        return ProcessAnimationLegacy(debugCompression);
    }   
}

bool CAnimationCompressor::ProcessAnimationFBX(bool debugCompression)
{
    // Convert to the new controller format.
    ConvertRemainingControllers(m_GlobalAnimationHeader.m_arrController);

    // Analyze root motion, extract turn information, massage, etc.
    if (!ProcessRootTracksFBX())
    {
        return false;
    }

    // Mark asset as looped if start and end poses roughly match.
    DetectCycleAnimationsFBX();

    // Extract translation speed.
    EvaluateSpeedFBX();                 

    // Normalize the ticks to be equivalent to frames and discard the TICK_CONVERT normalized values from the intermediate asset
    m_GlobalAnimationHeader.m_fSecsPerTick = m_GlobalAnimationHeader.m_fSecsPerTick * m_GlobalAnimationHeader.m_nTicksPerFrame;
    m_GlobalAnimationHeader.m_nTicksPerFrame = TICKS_PER_FRAME;

    CCompressonator compressonator;
    compressonator.CompressAnimation(m_GlobalAnimationHeader, m_GlobalAnimationHeader, &m_skeleton, m_platformSetup, m_AnimDesc, m_skinningInfo, debugCompression);

    return true;
}

bool CAnimationCompressor::ProcessRootTracksFBX()
{
    CController* rootController = static_cast<CController*>(m_GlobalAnimationHeader.GetController(m_skinningInfo->m_rootBoneId));
    if (!rootController)
    {
        AZ_TracePrintf("Animation_Warning", "Cannot find root controller");
        return false;
    }

    // Notable difference vs legacy importer:
    // We are not re-building the root track and compressing it separately.
    // We may choose to do so down the line if we massage the data further, but the new importer
    // is deliberately preserving the integrity of source data, and only extracting metadata and compressing.
    // If it turns out we need to enforce certain conventions here and massage the data, we can easily do so.

    //
    // Sample root motion at t = 0, t = 0.5, and t = 1 (normalized).
    //

    Diag33 scale;
    QuatT firstKey, midKey, lastKey;
    rootController->GetOPS(m_GlobalAnimationHeader.NTime2KTime(0.0f), firstKey.q, firstKey.t, scale);
    rootController->GetOPS(m_GlobalAnimationHeader.NTime2KTime(0.5f), midKey.q, midKey.t, scale);
    rootController->GetOPS(m_GlobalAnimationHeader.NTime2KTime(1.0f), lastKey.q, lastKey.t, scale);
    
    // Start & end locations are cached in the header data.
    m_GlobalAnimationHeader.m_StartLocation2 = firstKey;
    m_GlobalAnimationHeader.m_LastLocatorKey2 = lastKey;
    
    //
    // Extract slope, turn, and turn speed values and cache in the asset header.
    // Magic #s preserved from previous Cry implementation.
    //
    
    m_GlobalAnimationHeader.m_fSlope = 0;
    const Vec3 p0 = firstKey.t;
    const Vec3 p1 = lastKey.t;
    Vec3 vdir = Vec3(ZERO);

    const float fLength = (p1 - p0).GetLength();
    if (fLength > 0.01f)
    {
        vdir = ((p1 - p0) * firstKey.q).GetNormalized();
    }
    const f64 l = sqrt(vdir.x * vdir.x + vdir.y * vdir.y);
    if (l > 0.0001)
    {
        const float rad = static_cast<float>(atan2(-vdir.z * (vdir.y / l), l));
        m_GlobalAnimationHeader.m_fSlope = RAD2DEG(rad);
    }

    const Vec3 startDir = firstKey.q.GetColumn1();
    const Vec3 middleDir = midKey.q.GetColumn1();
    const Vec3 endDir = lastKey.q.GetColumn1();
    const float leftAngle = Ang3::CreateRadZ(startDir, middleDir);
    const float rightAngle = Ang3::CreateRadZ(middleDir, endDir);
    const float angle = leftAngle + rightAngle;
    const float duration = m_GlobalAnimationHeader.m_fEndSec - m_GlobalAnimationHeader.m_fStartSec;
    m_GlobalAnimationHeader.m_fAssetTurn = angle;
    m_GlobalAnimationHeader.m_fTurnSpeed = angle / duration;

    return true;
}



void CAnimationCompressor::EvaluateSpeedFBX()
{
    f32 fStart      =   m_GlobalAnimationHeader.m_fStartSec;
    f32 fDuration   =   m_GlobalAnimationHeader.m_fEndSec - m_GlobalAnimationHeader.m_fStartSec;
    f32 fDistance   =   0.0f;

    m_GlobalAnimationHeader.m_fDistance = 0.0f;
    m_GlobalAnimationHeader.m_fSpeed = 0.0f;

    IController* pController = m_GlobalAnimationHeader.GetController(m_skinningInfo->m_rootBoneId);
    if (pController == 0)
    {
        return;
    }
    CInfo ERot0 = pController->GetControllerInfo();
    if (ERot0.realkeys < 2)
    {
        return;
    }

    GlobalAnimationHeaderCAF& rCAF = m_GlobalAnimationHeader;

    Vec3 SPos0, SPos1;
    f32 newtime = 0;
    pController->GetP(rCAF.NTime2KTime(newtime), SPos0);
    for (f32 t = 0.01f; t <= 1.0f; t = t + 0.01f)
    {
        newtime = t;
        pController->GetP(rCAF.NTime2KTime(newtime), SPos1);
        if (SPos0 != SPos1)
        {
            fDistance += (SPos0 - SPos1).GetLength();
            SPos0 = SPos1;
        }
    }

    m_GlobalAnimationHeader.m_fDistance = fDistance;
    if (fDuration)
    {
        m_GlobalAnimationHeader.m_fSpeed = fDistance / fDuration;
    }

    if (fDuration < 0.001f)
    {
        AZ_TracePrintf("Animation_Warning", "Animation-asset '%s' has just one keyframe. Impossible to detect Duration", m_GlobalAnimationHeader.GetFilePath());
    }
}


void CAnimationCompressor::DetectCycleAnimationsFBX()
{
    GlobalAnimationHeaderCAF& rCAF = m_GlobalAnimationHeader;

    AZStd::vector<QuatT> m_startTransforms;
    m_startTransforms.reserve(128);

    bool equal = true;

    for (IController_AutoPtr controller : rCAF.m_arrController)
    {
        QuatT start, end;
        controller->GetOP(rCAF.NTime2KTime(0.0f), start.q, start.t);
        controller->GetOP(rCAF.NTime2KTime(1.0f), end.q, end.t);

        equal &= IsQuatEquivalent_dot(start.q, end.q, 0.1f);
    }

    if (equal)
    {
        m_GlobalAnimationHeader.m_nFlags |= CA_ASSET_CYCLE;
    }
    else
    {
        m_GlobalAnimationHeader.m_nFlags &= ~CA_ASSET_CYCLE;
    }

    if (m_AnimDesc.m_bAdditiveAnimation)
    {
        m_GlobalAnimationHeader.OnAssetAdditive();
    }
}


// Modified version of AnalyseAndModifyAnimations function
bool CAnimationCompressor::ProcessAnimationLegacy(bool debugCompression)
{
    EvaluateStartLocationLegacy();

    const float minRootPosEpsilon = m_AnimDesc.m_bNewFormat ? FLT_MAX : m_AnimDesc.format.oldFmt.m_fPOS_EPSILON;
    const float minRootRotEpsilon = m_AnimDesc.m_bNewFormat ? FLT_MAX : m_AnimDesc.format.oldFmt.m_fROT_EPSILON;

    ReplaceRootByLocatorLegacy(true, minRootPosEpsilon, minRootRotEpsilon);   //replace root by locator, apply simple compression to the root-joint and re-transform all children
    ReplaceRootByLocatorLegacy(false, minRootPosEpsilon, minRootRotEpsilon);  //apply simple compression to the root-joint and re-transform all children

    DetectCycleAnimationsLegacy();
    EvaluateSpeedLegacy();              // obsolete and not used in C3, but still used in Warface, so we can't erase it

    // Normalize the ticks to be equivalent to frames and discard the TICK_CONVERT normalized values from the intermediate asset
    m_GlobalAnimationHeader.m_fSecsPerTick = m_GlobalAnimationHeader.m_fSecsPerTick * m_GlobalAnimationHeader.m_nTicksPerFrame; //pSkinningInfo->m_secsPerTick
    m_GlobalAnimationHeader.m_nTicksPerFrame = TICKS_PER_FRAME; //pSkinningInfo->m_nTicksPerFrame


    uint32 numJoints = m_skeleton.m_SkinningInfo.m_arrBonesDesc.size();
    uint32 numController = m_GlobalAnimationHeader.m_arrController.size();
    if (numController > numJoints)
    {
        numJoints = numController;
    }

    CCompressonator compressonator;
    m_GlobalAnimationHeader.m_nCompression = m_AnimDesc.format.oldFmt.m_CompressionQuality;  // FIXME: Sokov: it seems that it's not used. remove this line?
    compressonator.CompressAnimation(m_GlobalAnimationHeader, m_GlobalAnimationHeader, &m_skeleton, m_platformSetup, m_AnimDesc, m_skinningInfo, debugCompression);

    return true;
}


void CAnimationCompressor::EvaluateStartLocationLegacy()
{
    uint32 numJoints = m_skeleton.m_SkinningInfo.m_arrBonesDesc.size();
    if (numJoints == 0)
    {
        return;
    }

    uint32 locator_locoman01 = CCrc32::Compute("Locator_Locomotion");
    IController* pLocomotionController = m_GlobalAnimationHeader.GetController(locator_locoman01);
    GlobalAnimationHeaderCAF& rCAF = m_GlobalAnimationHeader;

    const char* pRootName = m_skeleton.m_SkinningInfo.m_arrBonesDesc[0].m_arrBoneName;
    uint32 RootControllerID = CCrc32::Compute(pRootName);
    IController* pRootController = m_GlobalAnimationHeader.GetController(RootControllerID);

    GlobalAnimationHeaderCAF& rGlobalAnimHeader = m_GlobalAnimationHeader;

    Diag33 Scale;
    QuatT absFirstKey;
    absFirstKey.SetIdentity();
    QuatT absLastKey;
    absLastKey.SetIdentity();
    if (pLocomotionController)
    {
        pLocomotionController->GetOPS(rCAF.NTime2KTime(0), absFirstKey.q, absFirstKey.t, Scale);
        pLocomotionController->GetOPS(rCAF.NTime2KTime(1), absLastKey.q, absLastKey.t, Scale);
        //for some reason there is some noise in the original animation.
        if (1.0f - fabsf(absFirstKey.q.w) < 0.00001f && fabsf(absFirstKey.q.v.x) < 0.00001f && fabsf(absFirstKey.q.v.y) < 0.00001f && fabsf(absFirstKey.q.v.z) < 0.00001f)
        {
            absFirstKey.q.SetIdentity();
        }
    }
    else if (pRootController)
    {
        pRootController->GetOPS(rCAF.NTime2KTime(0), absFirstKey.q, absFirstKey.t, Scale);
        pRootController->GetOPS(rCAF.NTime2KTime(1), absLastKey.q, absLastKey.t, Scale);
        //for some reason there is some noise in the original animation.
        if (1.0f - fabsf(absFirstKey.q.w) < 0.00001f && fabsf(absFirstKey.q.v.x) < 0.00001f && fabsf(absFirstKey.q.v.y) < 0.00001f && fabsf(absFirstKey.q.v.z) < 0.00001f)
        {
            absFirstKey.q.SetIdentity();
        }
    }
    rGlobalAnimHeader.m_StartLocation2  = absFirstKey;
    rGlobalAnimHeader.m_LastLocatorKey2 = absLastKey;
}

void CAnimationCompressor::ReplaceRootByLocatorLegacy(bool bCheckLocator, float minRootPosEpsilon, float minRootRotEpsilon)
{
    uint32 locator_locoman01 = CCrc32::Compute("Locator_Locomotion");
    IController* pLocomotionController = m_GlobalAnimationHeader.GetController(locator_locoman01);
    if (bCheckLocator)
    {
        if (pLocomotionController == 0)
        {
            // It's required to replace all old type controllers with new type controllers
            ConvertRemainingControllers(m_GlobalAnimationHeader.m_arrController);
            return;
        }
    }

    uint32 numJoints = m_skeleton.m_SkinningInfo.m_arrBonesDesc.size();
    if (numJoints == 0)
    {
        return;
    }

    const char* const pRootName = m_skeleton.m_SkinningInfo.m_arrBonesDesc[0].m_arrBoneName;
    int32 RootParent = m_skeleton.m_SkinningInfo.m_arrBonesDesc[0].m_nOffsetParent;
    const uint32 RootControllerID = CCrc32::Compute(pRootName);
    IController* pRootController = m_GlobalAnimationHeader.GetController(RootControllerID);
    if (pRootController == 0)
    {
        RCLog("Can't find root-controller with name: %s", pRootName);

        ConvertRemainingControllers(m_GlobalAnimationHeader.m_arrController);
        return;
    }

    if (!bCheckLocator)
    {
        pLocomotionController = pRootController;
    }

    // Direct children of the root bone
    std::vector<IController_AutoPtr> arrpChildController;

    uint32 nDirectChildren = 0;
    for (uint32 i = 0; i < numJoints; i++)
    {
        const int32 nParent = m_skeleton.m_SkinningInfo.m_arrBonesDesc[i].m_nOffsetParent;
        const char* pChildName  = m_skeleton.m_SkinningInfo.m_arrBonesDesc[ i].m_arrBoneName;

        const bool bLocator = strcmp(pChildName, "Locator_Locomotion") == 0;
        if (bLocator)
        {
            continue;
        }
        const bool bRoot = strcmp(pChildName, pRootName) == 0;
        if (bRoot)
        {
            continue;
        }

        const char* pParentName = m_skeleton.m_SkinningInfo.m_arrBonesDesc[i + nParent].m_arrBoneName;
        const bool bParentIsRoot = (0 == m_skeleton.m_SkinningInfo.m_arrBonesDesc[i + nParent].m_nOffsetParent);
        if (bParentIsRoot)
        {
            uint32 ChildControllerID = CCrc32::Compute(pChildName);
            IController* pChildController = m_GlobalAnimationHeader.GetController(ChildControllerID);
            if (pChildController)
            {
                arrpChildController.push_back(pChildController);
                nDirectChildren++;
            }
        }
    }


    //evaluate the distance and the duration of this animation (we use it to calculate the speed)
    Diag33 Scale;
    CInfo ERot0 =   pRootController->GetControllerInfo();
    if (ERot0.realkeys == 0)
    {
        return;
    }
    if (ERot0.realkeys < 0)
    {
        RCLogError("Corrupted keys in %s", m_filename.c_str());
        return;
    }

    QuatT absFirstKey;

    GlobalAnimationHeaderCAF& rCAF = m_GlobalAnimationHeader;
    pLocomotionController->GetOPS(rCAF.NTime2KTime(0), absFirstKey.q, absFirstKey.t, Scale);
    //for some reason there is some noise in the original animation.
    if (1.0f - fabsf(absFirstKey.q.w) < 0.00001f && fabsf(absFirstKey.q.v.x) < 0.00001f && fabsf(absFirstKey.q.v.y) < 0.00001f && fabsf(absFirstKey.q.v.z) < 0.00001f)
    {
        absFirstKey.q.SetIdentity();
    }


    const QuatT identityQT(IDENTITY);

    PQLog zeroPQL;
    zeroPQL.vPos.Set(0.0f, 0.0f, 0.0f);
    zeroPQL.vRotLog.Set(0.0f, 0.0f, 0.0f);

    std::vector< std::vector<QuatT> > arrChildAbsKeys;
    std::vector< std::vector<PQLog> > arrChildPQKeys;

    const uint32 numChildController = arrpChildController.size();

    arrChildAbsKeys.resize(numChildController);
    arrChildPQKeys.resize(numChildController);
    for (uint32 c = 0; c < numChildController; c++)
    {
        arrChildAbsKeys[c].resize(ERot0.realkeys, identityQT);
        arrChildPQKeys[c].resize(ERot0.realkeys, zeroPQL);
    }


    GlobalAnimationHeaderCAF& rGlobalAnimHeader = m_GlobalAnimationHeader;

    std::vector<int> arrTimes;
    arrTimes.resize(ERot0.realkeys);
    std::vector<QuatT> arrRootKeys;
    arrRootKeys.resize(ERot0.realkeys, identityQT);

    float framesPerSecond = 1.f / (rGlobalAnimHeader.m_fSecsPerTick * rGlobalAnimHeader.m_nTicksPerFrame);
    int32 startkey  =   int32(rGlobalAnimHeader.m_fStartSec * framesPerSecond);
    int32 stime =   startkey;
    for (int32 t = 0; t < ERot0.realkeys; t++)
    {
        f32 normalized_time = 0.0f;
        if (stime != startkey)
        {
            normalized_time = f32(stime - startkey) / (TICKS_PER_FRAME * (ERot0.realkeys - 1));
        }

        QuatT absRootKey;
        pRootController->GetOPS(rCAF.NTime2KTime(normalized_time), absRootKey.q, absRootKey.t, Scale);
        arrRootKeys[t] = absFirstKey.GetInverted() * absRootKey;
        for (uint32 c = 0; c < numChildController; c++)
        {
            QuatT relChildKey;
            if (arrpChildController[c])
            {
                arrpChildController[c]->GetOPS(rCAF.NTime2KTime(normalized_time), relChildKey.q, relChildKey.t, Scale);
                arrChildAbsKeys[c][t] = arrRootKeys[t] * relChildKey; //child of root
            }
        }
        arrTimes[t] = stime;
        stime += TICKS_PER_FRAME;
    }


    Diag33 s;
    QuatT fkey;
    pLocomotionController->GetOPS(rCAF.NTime2KTime(0.0f), fkey.q, fkey.t, s);
    QuatT mkey;
    pLocomotionController->GetOPS(rCAF.NTime2KTime(0.5f), mkey.q, mkey.t, s);
    QuatT lkey;
    pLocomotionController->GetOPS(rCAF.NTime2KTime(1.0f), lkey.q, lkey.t, s);

    stime   =   startkey;
    std::vector<PQLog> arrRootPQKeys;
    for (int32 key = 0; key < ERot0.realkeys; key++)
    {
        f32 normalized_time = 0.0f;
        if (stime != startkey)
        {
            normalized_time = f32(stime - startkey) / (TICKS_PER_FRAME * (ERot0.realkeys - 1));
        }

        QuatT relLocatorKey;
        pLocomotionController->GetOPS(rCAF.NTime2KTime(normalized_time), relLocatorKey.q, relLocatorKey.t, Scale);

        relLocatorKey = absFirstKey.GetInverted() * relLocatorKey;

        //convert into the old FarCry format
        PQLog pqlog;
        pqlog.vRotLog = Quat::log(!relLocatorKey.q);
        pqlog.vPos = relLocatorKey.t;
        arrRootPQKeys.push_back(pqlog);


        // For debug.
        // Sometimes it's not clear what data are broken - input or output. This logging helps to enssure that the input data are ok.
#if 0
        Quat q = Quat::exp(pqlog.vRotLog);
        if (key > 0)
        {
            const float cosOfHalfAngle = 1 - fabsf(fabsf(q | prev) - 1);
            const float a = RAD2DEG(acosf(cosOfHalfAngle)) * 2.0f;
            RCLog("-key:%i a:%f", key, a);
        }
        prev = q;
#endif

        stime += TICKS_PER_FRAME;
    }

    if (pRootController->GetID() != RootControllerID)
    {
        RCLogError("Internal error in ", __FUNCTION__);
        return;
    }

    const float rootPositionTolerance = Util::getMin(0.0005f, minRootPosEpsilon); // 0.5mm. previously it was 1cm
    const float rootRotationToleranceInDegrees = Util::getMin(0.1f, minRootRotEpsilon); // previously it was 1.145915590262f;
    IController_AutoPtr newController(CreateNewController(pRootController->GetID(), pRootController->GetFlags(), arrTimes, arrRootPQKeys, rootPositionTolerance, rootRotationToleranceInDegrees));
    m_GlobalAnimationHeader.ReplaceController(pRootController, newController);
    pRootController = m_GlobalAnimationHeader.GetController(RootControllerID);

    //--------------------------------------------------------------------------------------
    //---                     calculate the slope value                                 ----
    //--------------------------------------------------------------------------------------
    rGlobalAnimHeader.m_fSlope = 0;
    Vec3 p0 = fkey.t;
    Vec3 p1 = lkey.t;
    Vec3 vdir = Vec3(ZERO);

    f32 fLength = (p1 - p0).GetLength();
    if (fLength > 0.01f) //only if there is enough movement
    {
        vdir = ((p1 - p0) * fkey.q).GetNormalized();
    }
    f64 l = sqrt(vdir.x * vdir.x + vdir.y * vdir.y);
    if (l > 0.0001)
    {
        f32 rad         = f32(atan2(-vdir.z * (vdir.y / l), l));
        f32 deg         = RAD2DEG(rad);
        rGlobalAnimHeader.m_fSlope = rad;
    }

    const Vec3 startDir     = fkey.q.GetColumn1();
    const Vec3 middleDir    = mkey.q.GetColumn1();
    const Vec3 endDir           = lkey.q.GetColumn1();
    const f32 leftAngle     = Ang3::CreateRadZ(startDir, middleDir);
    const f32 rightAngle    = Ang3::CreateRadZ(middleDir, endDir);
    f32 angle           = leftAngle + rightAngle;
    const f32 duration      = rCAF.m_fEndSec - rCAF.m_fStartSec;
    rGlobalAnimHeader.m_fAssetTurn = angle;
    rGlobalAnimHeader.m_fTurnSpeed = angle / duration;

    //--------------------------------------------------------------------------------
    Vec3 fkey2;
    pRootController->GetP(rCAF.NTime2KTime(0), fkey2);
    Vec3 lkey2;
    pRootController->GetP(rCAF.NTime2KTime(1), lkey2);

    stime = startkey;
    for (int32 t = 0; t < ERot0.realkeys; t++)
    {
        QuatT absRootKey;

        f32 normalized_time = 0.0f;
        if (stime != startkey)
        {
            normalized_time = f32(stime - startkey) / (TICKS_PER_FRAME * (ERot0.realkeys - 1));
        }

        pRootController->GetOPS(rCAF.NTime2KTime(normalized_time), absRootKey.q, absRootKey.t, Scale);
        for (uint32 c = 0; c < numChildController; c++)
        {
            QuatT relChildKeys = absRootKey.GetInverted() * arrChildAbsKeys[c][t]; //relative Pivot
            arrChildPQKeys[c][t].vRotLog    =   Quat::log(!relChildKeys.q);
            arrChildPQKeys[c][t].vPos       =   relChildKeys.t;
        }
        stime += TICKS_PER_FRAME;
    }


    const float positionTolerance = Util::getMin(0.000031622777f, minRootPosEpsilon);
    const float rotationToleranceInDegrees = Util::getMin(0.003623703272f, minRootRotEpsilon);

    for (uint32 c = 0; c < numChildController; c++)
    {
        IController_AutoPtr oldController = arrpChildController[c];
        IController_AutoPtr newController(CreateNewController(oldController->GetID(), oldController->GetFlags(), arrTimes, arrChildPQKeys[c], positionTolerance, rotationToleranceInDegrees));
        m_GlobalAnimationHeader.ReplaceController(oldController, newController);
    }
}



void CAnimationCompressor::EvaluateSpeedLegacy()
{
    uint32 numJoints = m_skeleton.m_SkinningInfo.m_arrBonesDesc.size();
    if (numJoints == 0)
    {
        return;
    }

    f32 fStart      =   m_GlobalAnimationHeader.m_fStartSec;
    f32 fDuration   =   m_GlobalAnimationHeader.m_fEndSec - m_GlobalAnimationHeader.m_fStartSec;
    f32 fDistance   =   0.0f;

    m_GlobalAnimationHeader.m_fDistance = 0.0f;
    m_GlobalAnimationHeader.m_fSpeed    = 0.0f;


    //  const char* pRootNameParam = m_AnimDesc.m_strRoot;
    const char* pRootName = m_skeleton.m_SkinningInfo.m_arrBonesDesc[0].m_arrBoneName;

    IController* pController = m_GlobalAnimationHeader.GetController(m_skeleton.m_SkinningInfo.m_arrBonesDesc[0].m_nControllerID);
    if (pController == 0)
    {
        return;
    }
    CInfo ERot0 =   pController->GetControllerInfo();
    if (ERot0.realkeys < 2)
    {
        return;
    }

    GlobalAnimationHeaderCAF& rCAF = m_GlobalAnimationHeader;

    Vec3 SPos0, SPos1;
    f32 newtime = 0;
    pController->GetP(rCAF.NTime2KTime(newtime), SPos0);
    for (f32 t = 0.01f; t <= 1.0f; t = t + 0.01f)
    {
        newtime = t;
        pController->GetP(rCAF.NTime2KTime(newtime), SPos1);
        if (SPos0 != SPos1)
        {
            fDistance += (SPos0 - SPos1).GetLength();
            SPos0 = SPos1;
        }
        else
        {
            int a  = 0;
        }
    }

    m_GlobalAnimationHeader.m_fDistance         =   fDistance;
    if (fDuration)
    {
        m_GlobalAnimationHeader.m_fSpeed      = fDistance / fDuration;
    }

    if (fDuration < 0.001f)
    {
        RCLogWarning ("CryAnimation: Animation-asset '%s' has just one keyframe. Impossible to detect Duration", m_GlobalAnimationHeader.GetFilePath());
    }
}


void CAnimationCompressor::DetectCycleAnimationsLegacy()
{
    GlobalAnimationHeaderCAF& rCAF = m_GlobalAnimationHeader;

    uint32 numJoints = m_skeleton.m_SkinningInfo.m_arrBonesDesc.size();
    bool equal = 1;
    for (uint32 j = 0; j < numJoints; j++)
    {
        if (m_skeleton.m_SkinningInfo.m_arrBonesDesc[j].m_nOffsetParent == 0)
        {
            continue;
        }

        int AnimID = 0;
        IController* pController = m_GlobalAnimationHeader.GetController(m_skeleton.m_SkinningInfo.m_arrBonesDesc[j].m_nControllerID);
        ;                                                                                                                              //m_GlobalAnimationHeader.m_arrController[AnimID];//pModelJoint[j].m_arrControllersMJoint[AnimID];
        if (pController == 0)
        {
            continue;
        }

        Quat SRot;
        pController->GetO(rCAF.NTime2KTime(0), SRot);
        Quat ERot   =   pController->GetControllerInfo().quat;

        Ang3 sang = Ang3(SRot);
        Ang3 eang = Ang3(ERot);

        CInfo info = pController->GetControllerInfo();
        bool status = IsQuatEquivalent_dot(SRot, ERot, 0.1f);
        equal &= status;
    }

    if (equal)
    {
        m_GlobalAnimationHeader.m_nFlags |= CA_ASSET_CYCLE;
    }
    else
    {
        m_GlobalAnimationHeader.m_nFlags &= ~CA_ASSET_CYCLE;
    }

    if (m_AnimDesc.m_bAdditiveAnimation)
    {
        m_GlobalAnimationHeader.OnAssetAdditive();
    }
}

void CAnimationCompressor::SetIdentityLocatorLegacy(GlobalAnimationHeaderAIM& gaim)
{
    // Note: This is only called for AIM animations.


    uint32 numJoints = m_skeleton.m_SkinningInfo.m_arrBonesDesc.size();
    if (numJoints == 0)
    {
        return;
    }

    uint32 locator_locoman01 = CCrc32::Compute("Locator_Locomotion");
    IController* pLocomotionController = gaim.GetController(locator_locoman01);

    const char* pRootName = m_skeleton.m_SkinningInfo.m_arrBonesDesc[0].m_arrBoneName;
    int32 RootParent = m_skeleton.m_SkinningInfo.m_arrBonesDesc[0].m_nOffsetParent;
    uint32 RootControllerID = CCrc32::Compute(pRootName);
    IController_AutoPtr pRootController = gaim.GetController(RootControllerID);
    if (!pRootController)
    {
        RCLog("Can't find root-joint with name: %s", pRootName);
        return;
    }

    std::vector<IController*> arrpChildController;

    uint32 nDirectChildren = 0;
    for (uint32 i = 0; i < numJoints; i++)
    {
        int32 nParent = m_skeleton.m_SkinningInfo.m_arrBonesDesc[ i].m_nOffsetParent;
        const char* pChildName  = m_skeleton.m_SkinningInfo.m_arrBonesDesc[ i].m_arrBoneName;

        int32 IsLocator = strcmp(pChildName, "Locator_Locomotion") == 0;
        if (IsLocator)
        {
            continue;
        }
        int32 IsRoot = strcmp(pChildName, pRootName) == 0;
        if (IsRoot)
        {
            continue;
        }

        const char* pParentName = m_skeleton.m_SkinningInfo.m_arrBonesDesc[ i + nParent].m_arrBoneName;
        int32 idx = m_skeleton.m_SkinningInfo.m_arrBonesDesc[ i + nParent].m_nOffsetParent;
        if (idx == 0)
        {
            uint32 ChildControllerID = CCrc32::Compute(pChildName);
            IController* pChildController = gaim.GetController(ChildControllerID);
            if (pChildController)
            {
                arrpChildController.push_back(pChildController);
                nDirectChildren++;
            }
        }
    }


    //evaluate the distance and the duration of this animation (we use it to calculate the speed)
    CInfo ERot0 =   pRootController->GetControllerInfo();
    if (ERot0.realkeys == 0)
    {
        return;
    }

    Diag33 Scale;
    QuatT absFirstKey;
    absFirstKey.SetIdentity();
    if (pLocomotionController)
    {
        pLocomotionController->GetOPS(gaim.NTime2KTime(0), absFirstKey.q, absFirstKey.t, Scale);
    }
    //for some reason there is some noise in the original animation.
    if (1.0f - fabsf(absFirstKey.q.w) < 0.00001f && fabsf(absFirstKey.q.v.x) < 0.00001f && fabsf(absFirstKey.q.v.y) < 0.00001f && fabsf(absFirstKey.q.v.z) < 0.00001f)
    {
        absFirstKey.q.SetIdentity();
    }


    const QuatT identityQT(IDENTITY);

    PQLog zeroPQL;
    zeroPQL.vPos.Set(0.0f, 0.0f, 0.0f);
    zeroPQL.vRotLog.Set(0.0f, 0.0f, 0.0f);

    std::vector<int> arrTimes;
    arrTimes.resize(ERot0.realkeys);
    std::vector<QuatT> arrRootKeys;
    arrRootKeys.resize(ERot0.realkeys, identityQT);

    std::vector< std::vector<QuatT> > arrChildAbsKeys;
    std::vector< std::vector<PQLog> > arrChildPQKeys;

    const size_t numChildController = arrpChildController.size();
    arrChildAbsKeys.resize(numChildController);
    arrChildPQKeys.resize(numChildController);
    for (size_t c = 0; c < numChildController; c++)
    {
        arrChildAbsKeys[c].resize(ERot0.realkeys, identityQT);
        arrChildPQKeys[c].resize(ERot0.realkeys, zeroPQL);
    }

    float framesPerSecond = 1.f / (m_GlobalAnimationHeader.m_fSecsPerTick * m_GlobalAnimationHeader.m_nTicksPerFrame);
    int32 startkey  =   int32(gaim.m_fStartSec * framesPerSecond);
    int32 stime =   startkey;
    for (int32 t = 0; t < ERot0.realkeys; t++)
    {
        f32 normalized_time = 0.0f;
        if (stime != startkey)
        {
            normalized_time = f32(stime - startkey) / (TICKS_PER_FRAME * (ERot0.realkeys - 1));
        }

        QuatT absRootKey;
        pRootController->GetOPS(gaim.NTime2KTime(normalized_time), absRootKey.q, absRootKey.t, Scale);
        arrRootKeys[t] = absFirstKey.GetInverted() * absRootKey;
        for (uint32 c = 0; c < numChildController; c++)
        {
            QuatT relChildKey;
            relChildKey.SetIdentity();
            if (arrpChildController[c])
            {
                arrpChildController[c]->GetOPS(gaim.NTime2KTime(normalized_time), relChildKey.q, relChildKey.t, Scale);
                arrChildAbsKeys[c][t] = arrRootKeys[t] * relChildKey; //child of root
            }
        }
        arrTimes[t] = stime;
        stime += TICKS_PER_FRAME;
    }


    stime   =   startkey;
    std::vector<PQLog> arrRootPQKeys;
    for (int32 key = 0; key < ERot0.realkeys; key++)
    {
        f32 normalized_time = 0.0f;
        if (stime != startkey)
        {
            normalized_time = f32(stime - startkey) / (TICKS_PER_FRAME * (ERot0.realkeys - 1));
        }

        QuatT relLocatorKey;
        relLocatorKey.SetIdentity();
        if (pLocomotionController)
        {
            pLocomotionController->GetOPS(gaim.NTime2KTime(normalized_time), relLocatorKey.q, relLocatorKey.t, Scale);
        }
        relLocatorKey = absFirstKey.GetInverted() * relLocatorKey;

        //convert into the old FarCry format
        PQLog pqlog;
        pqlog.vRotLog = Quat::log(!relLocatorKey.q);
        pqlog.vPos = relLocatorKey.t;
        arrRootPQKeys.push_back(pqlog);

        stime += TICKS_PER_FRAME;
    }

    gaim.ReplaceController(pRootController.get(), CreateNewControllerAIM(pRootController->GetID(), pRootController->GetFlags(), arrTimes, arrRootPQKeys));

    //--------------------------------------------------------------------------------
    pRootController = gaim.GetController(RootControllerID);

    stime = startkey;
    for (int32 t = 0; t < ERot0.realkeys; t++)
    {
        QuatT absRootKey;

        f32 normalized_time = 0.0f;
        if (stime != startkey)
        {
            normalized_time = f32(stime - startkey) / (TICKS_PER_FRAME * (ERot0.realkeys - 1));
        }

        pRootController->GetOPS(gaim.NTime2KTime(normalized_time), absRootKey.q, absRootKey.t, Scale);
        for (uint32 c = 0; c < numChildController; c++)
        {
            QuatT relChildKeys = absRootKey.GetInverted() * arrChildAbsKeys[c][t]; //relative Pivot
            arrChildPQKeys[c][t].vRotLog    =   Quat::log(!relChildKeys.q);
            arrChildPQKeys[c][t].vPos       =   relChildKeys.t;
        }
        stime += TICKS_PER_FRAME;
    }


    for (size_t c = 0; c < numChildController; ++c)
    {
        gaim.ReplaceController(arrpChildController[c], CreateNewControllerAIM(arrpChildController[c]->GetID(), arrpChildController[c]->GetFlags(), arrTimes, arrChildPQKeys[c]));
    }
}


void CAnimationCompressor::EnableTrackAligning(bool bEnable)
{
    m_bAlignTracks = bEnable;
}


CController* CAnimationCompressor::CreateNewController(int controllerID, int flags, const std::vector<int>& arrFullTimes, const std::vector<PQLog>& arrFullQTKeys, float positionTolerance, float rotationToleranceInDegrees)
{
    std::unique_ptr<CController> pNewController(new CController);

    pNewController->m_nControllerId = controllerID;
    pNewController->m_nFlags = flags;

    CCompressonator compressonator;

    int32 oldtime =  -1;
    for (size_t i = 0; i < arrFullTimes.size(); ++i)
    {
        if (oldtime != arrFullTimes[i])
        {
            oldtime = arrFullTimes[i];

            compressonator.m_RotTimes.push_back(arrFullTimes[i]);
            compressonator.m_PosTimes.push_back(arrFullTimes[i]);

            Quat q = !Quat::exp(arrFullQTKeys[i].vRotLog);
            compressonator.m_Rotations.push_back(q);
            compressonator.m_Positions.push_back(arrFullQTKeys[i].vPos);
        }
    }

    compressonator.CompressPositions(
        pNewController.get(),
        eNoCompressVec3,
        (positionTolerance != 0.0f),
        positionTolerance);

    compressonator.CompressRotations(
        pNewController.get(),
        eSmallTree64BitExtQuat,
        (rotationToleranceInDegrees != 0.0f),
        rotationToleranceInDegrees);

    return pNewController.release();
}


CController* CAnimationCompressor::CreateNewControllerAIM(int controllerID, int flags, const std::vector<int>& arrFullTimes, const std::vector<PQLog>& arrFullQTKeys)
{
    return CreateNewController(controllerID, flags, arrFullTimes, arrFullQTKeys, 0.0f, 0.0f);
}

#define POSES (0x080)


bool IsOldChunk(CChunkFile::ChunkDesc* ch)
{
    return (ch->chunkVersion < CONTROLLER_CHUNK_DESC_0829::VERSION) || (ch->chunkVersion ==  CONTROLLER_CHUNK_DESC_0830::VERSION);
}

bool IsNewChunk(CChunkFile::ChunkDesc* ch)
{
    return !IsOldChunk(ch);
}

void CAnimationCompressor::DeleteCompressedChunks()
{
    // remove new MotionParams from file
    m_ChunkFile.DeleteChunksByType(ChunkType_MotionParameters);

    // delete controllers in new format
    uint32 numChunck = m_ChunkFile.NumChunks();
    for (uint32 i = 0; i < numChunck; i++)
    {
        CChunkFile::ChunkDesc* cd = m_ChunkFile.GetChunk(i);
        if (cd)
        {
            if (cd->chunkType != ChunkType_Controller)
            {
                continue;
            }

            if (IsNewChunk(cd))
            {
                m_ChunkFile.DeleteChunkById(cd->chunkId);
                --numChunck;
                i = 0;
            }
        }
    }
}


bool CAnimationCompressor::CompareKeyTimes(KeyTimesInformationPtr& ptr1, KeyTimesInformationPtr& ptr2)
{
    if (ptr1->GetNumKeys() != ptr2->GetNumKeys())
    {
        return false;
    }

    for (uint32 i = 0; i < ptr1->GetNumKeys(); ++i)
    {
        if (ptr1->GetKeyValueFloat(i) != ptr2->GetKeyValueFloat(i))
        {
            return false;
        }
    }
    return true;
}


struct CAnimChunkData
{
    std::vector<char> m_data;

    void Align(size_t boundary)
    {
        m_data.resize(::Align(m_data.size(), boundary), (char)0);
    }

    template <class T>
    void Add(const T& object)
    {
        AddData(&object, sizeof(object));
    }
    void AddData(const void* pSrcData, int nSrcDataSize)
    {
        const char* src = static_cast<const char*>(pSrcData);
        if (m_data.empty())
        {
            m_data.assign(src, src + nSrcDataSize);
        }
        else
        {
            m_data.insert(m_data.end(), src, src + nSrcDataSize);
        }
    }
    void* data()
    {
        return &m_data.front();
    }

    size_t size() const{return m_data.size(); }
};


void CAnimationCompressor::SaveMotionParameters(CChunkFile* pChunkFile, bool bigEndianOutput)
{
    SEndiannessSwapper swap(bigEndianOutput);

    CHUNK_MOTION_PARAMETERS chunk;
    ZeroStruct(chunk);

    MotionParams905& mp = chunk.mp;
    uint32 assetFlags = 0;
    if (m_GlobalAnimationHeader.IsAssetCycle())
    {
        assetFlags |= CA_ASSET_CYCLE;
    }
    if (m_AnimDesc.m_bAdditiveAnimation)
    {
        assetFlags |= CA_ASSET_ADDITIVE;
    }
    if (bigEndianOutput)
    {
        assetFlags |= CA_ASSET_BIG_ENDIAN;
    }
    swap(mp.m_nAssetFlags = assetFlags);

    swap(mp.m_nCompression = m_AnimDesc.format.oldFmt.m_CompressionQuality);

    swap(mp.m_nTicksPerFrame = m_GlobalAnimationHeader.m_nTicksPerFrame);
    swap(mp.m_fSecsPerTick = m_GlobalAnimationHeader.m_fSecsPerTick);
    swap(mp.m_nStart = m_GlobalAnimationHeader.m_nStartKey);
    swap(mp.m_nEnd = m_GlobalAnimationHeader.m_nEndKey);

    swap(mp.m_fMoveSpeed = m_GlobalAnimationHeader.m_fSpeed);
    swap(mp.m_fTurnSpeed = m_GlobalAnimationHeader.m_fTurnSpeed);
    swap(mp.m_fAssetTurn = m_GlobalAnimationHeader.m_fAssetTurn);
    swap(mp.m_fDistance = m_GlobalAnimationHeader.m_fDistance);
    swap(mp.m_fSlope = m_GlobalAnimationHeader.m_fSlope);

    mp.m_StartLocation = m_GlobalAnimationHeader.m_StartLocation2;
    swap(mp.m_StartLocation.q.w);
    swap(mp.m_StartLocation.q.v.x);
    swap(mp.m_StartLocation.q.v.y);
    swap(mp.m_StartLocation.q.v.z);
    swap(mp.m_StartLocation.t.x);
    swap(mp.m_StartLocation.t.y);
    swap(mp.m_StartLocation.t.z);

    mp.m_EndLocation        = m_GlobalAnimationHeader.m_LastLocatorKey2;
    swap(mp.m_EndLocation.q.w);
    swap(mp.m_EndLocation.q.v.x);
    swap(mp.m_EndLocation.q.v.y);
    swap(mp.m_EndLocation.q.v.z);
    swap(mp.m_EndLocation.t.x);
    swap(mp.m_EndLocation.t.y);
    swap(mp.m_EndLocation.t.z);


    swap(mp.m_LHeelStart = m_GlobalAnimationHeader.m_FootPlantVectors.m_LHeelStart);
    swap(mp.m_LHeelEnd = m_GlobalAnimationHeader.m_FootPlantVectors.m_LHeelEnd);
    swap(mp.m_LToe0Start = m_GlobalAnimationHeader.m_FootPlantVectors.m_LToe0Start);
    swap(mp.m_LToe0End = m_GlobalAnimationHeader.m_FootPlantVectors.m_LToe0End);

    swap(mp.m_RHeelStart = m_GlobalAnimationHeader.m_FootPlantVectors.m_RHeelStart);
    swap(mp.m_RHeelEnd = m_GlobalAnimationHeader.m_FootPlantVectors.m_RHeelEnd);
    swap(mp.m_RToe0Start = m_GlobalAnimationHeader.m_FootPlantVectors.m_RToe0Start);
    swap(mp.m_RToe0End = m_GlobalAnimationHeader.m_FootPlantVectors.m_RToe0End);

    CChunkData chunkData;
    chunkData.Add(chunk);

    pChunkFile->AddChunk(
        ChunkType_MotionParameters,
        CHUNK_MOTION_PARAMETERS::VERSION,
        (bigEndianOutput ? eEndianness_Big : eEndianness_Little),
        chunkData.data, chunkData.size);
}

void CAnimationCompressor::ExtractKeys(std::vector<int>& times, std::vector<PQLog>& logKeys, const CControllerPQLog* controller) const
{
    PQLog zeroPQL;
    zeroPQL.vPos.Set(0.0f, 0.0f, 0.0f);
    zeroPQL.vRotLog.Set(0.0f, 0.0f, 0.0f);

    const int32 numKeys = controller->m_arrKeys.size();
    times.resize(numKeys);
    logKeys.resize(numKeys, zeroPQL);

    float framesPerSecond = 1.f / (m_GlobalAnimationHeader.m_nTicksPerFrame * m_GlobalAnimationHeader.m_fSecsPerTick);
    const int32 startkey = int32(m_GlobalAnimationHeader.m_fStartSec * framesPerSecond);
    int32 stime = startkey;
    for (int32 t = 0; t < numKeys; t++)
    {
        logKeys[t] = controller->m_arrKeys[t];
        times[t] = stime;

        stime += TICKS_PER_FRAME;
    }
}


int CAnimationCompressor::ConvertRemainingControllers(TControllersVector& controllers) const
{
    int result = 0;
    std::vector<int> times;
    std::vector<PQLog> logKeys;
    size_t count = controllers.size();
    for (size_t i = 0; i < count; ++i)
    {
        IController_AutoPtr& controller = controllers[i];
        if (CControllerPQLog* logController = dynamic_cast<CControllerPQLog*>(controller.get()))
        {
            ExtractKeys(times, logKeys, logController);
            controller = CreateNewController(controllers[i]->GetID(), controllers[i]->GetFlags(), times, logKeys, 0.0f, 0.0f);
            ++result;
        }
    }
    return result;
}

uint32 CAnimationCompressor::SaveControllers(CSaverCGF& saver, TControllersVector& controllers, bool bigEndianOutput)
{
    uint32 numSaved = 0;
    uint32 numChunks = controllers.size();
    for (uint32 chunk = 0; chunk < numChunks; ++chunk)
    {
        if (CControllerPQLog* controller = dynamic_cast<CControllerPQLog*>(controllers[chunk].get()))
        {
            RCLogError("Saving of old (V0828) controllers is not supported anymore");
            continue;
        }

        // New format controller
        if (CController* controller = dynamic_cast<CController*>(controllers[chunk].get()))
        {
            if (SaveController831(saver, controller, bigEndianOutput))
            {
                ++numSaved;
            }
            continue;
        }
    }
    return numSaved;
}

bool CAnimationCompressor::SaveController831(CSaverCGF& saver, CController* pController, bool bigEndianOutput)
{
    const bool bSwapEndian = (bigEndianOutput ? (eEndianness_Big == eEndianness_NonNative) : (eEndianness_Big == eEndianness_Native));

    SEndiannessSwapper swap(bSwapEndian);

    CONTROLLER_CHUNK_DESC_0831 chunk;
    ZeroStruct(chunk);

    int s = sizeof(chunk);
    CAnimChunkData Data;

    chunk.nControllerId = pController->GetID();
    swap(chunk.nControllerId);
    chunk.nFlags = pController->GetFlags();

    chunk.TracksAligned = m_bAlignTracks ? 1 : 0;
    int nTrackAlignment = m_bAlignTracks ? 4 : 1;

    RotationControllerPtr pRotation = pController->GetRotationController();
    KeyTimesInformationPtr pRotTimes;
    if (pRotation)
    {
        pRotTimes = pRotation->GetKeyTimesInformation();
        chunk.numRotationKeys = pRotation->GetNumKeys();
        float lastTime = -FLT_MAX;
        for (int i = 0; i < chunk.numRotationKeys; ++i)
        {
            float t = pRotTimes->GetKeyValueFloat(i);
            if (t <= lastTime)
            {
                RCLogError("Trying to save controller with repeated or non-sorted time keys.");
                return false;
            }
            lastTime = t;
        }
        swap(chunk.numRotationKeys);

        chunk.RotationFormat = pRotation->GetFormat();
        swap(chunk.RotationFormat);


        chunk.RotationTimeFormat = pRotTimes->GetFormat();
        swap(chunk.RotationTimeFormat);

        //copy to chunk
        if (bSwapEndian)
        {
            pRotation->GetRotationStorage()->SwapBytes();
        }
        Data.Align(nTrackAlignment);
        Data.AddData(pRotation->GetRotationStorage()->GetData(), pRotation->GetRotationStorage()->GetDataRawSize());
        if (bSwapEndian)
        {
            pRotation->GetRotationStorage()->SwapBytes();
        }

        if (bSwapEndian)
        {
            pRotTimes->SwapBytes();
        }
        Data.Align(nTrackAlignment);
        Data.AddData(pRotTimes->GetData(), pRotTimes->GetDataRawSize());
        if (bSwapEndian)
        {
            pRotTimes->SwapBytes();
        }
    }

    PositionControllerPtr pPosition = pController->GetPositionController();
    KeyTimesInformationPtr pPosTimes;
    if (pPosition)
    {
        pPosTimes = pPosition->GetKeyTimesInformation();
        chunk.numPositionKeys = pPosition->GetNumKeys();
        float lastTime = -FLT_MAX;
        for (int i = 0; i < chunk.numPositionKeys; ++i)
        {
            float t = pPosTimes->GetKeyValueFloat(i);
            if (t <= lastTime)
            {
                RCLogError("Trying to save controller with repeated or non-sorted time keys.");
                return false;
            }
            lastTime = t;
        }
        swap(chunk.numPositionKeys);

        chunk.PositionFormat = pPosition->GetFormat();
        swap(chunk.PositionFormat);

        chunk.PositionKeysInfo = CONTROLLER_CHUNK_DESC_0831::eKeyTimePosition;

        if (bSwapEndian)
        {
            pPosition->GetPositionStorage()->SwapBytes();
        }
        Data.Align(nTrackAlignment);
        Data.AddData(pPosition->GetPositionStorage()->GetData(), pPosition->GetPositionStorage()->GetDataRawSize());
        if (bSwapEndian)
        {
            pPosition->GetPositionStorage()->SwapBytes();
        }

        if (pRotation && CompareKeyTimes(pPosTimes, pRotTimes))
        {
            chunk.PositionKeysInfo = CONTROLLER_CHUNK_DESC_0831::eKeyTimeRotation;
        }
        else
        {
            if (bSwapEndian)
            {
                pPosTimes->SwapBytes();
            }

            Data.Align(nTrackAlignment);
            Data.AddData(pPosTimes->GetData(), pPosTimes->GetDataRawSize());
            if (bSwapEndian)
            {
                pPosTimes->SwapBytes();
            }
            chunk.PositionTimeFormat = pPosTimes->GetFormat();
        }
    }

    uint32 numData = Data.size();
    if (numData)
    {
        saver.SaveController831(bSwapEndian, chunk, Data.data(), numData);
        return true;
    }
    return false;
}
