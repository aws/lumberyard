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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONLOADER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONLOADER_H
#pragma once

#include <CryCharAnimationParams.h>
#include "Controller.h"
#include "AnimationInfoLoader.h"
#include "../../../CryXML/IXMLSerializer.h"
#include "../CryEngine/Cry3DEngine/CGF/CGFLoader.h"
#include "CGF/CGFSaver.h"
#include "CGF/LoaderCAF.h"
#include "ControllerPQLog.h"
#include "ControllerPQ.h"
#include "SkeletonInfo.h"
#include "CGFContent.h"
#include "ConvertContext.h"
#include "CryVersion.h"
#include "CompressionController.h"
#include "GlobalAnimationHeader.h"
#include "GlobalAnimationHeaderCAF.h"
#include "GlobalAnimationHeaderAIM.h"
#include "IResCompiler.h"

#if defined(AZ_PLATFORM_WINDOWS)
#include <Windows.h> // for FILETIME
#endif

class CSkeletonInfo;
struct ConvertContext;
class CContentCGF;
class CTrackStorage;
struct SPlatformAnimationSetup;
class CInternalSkinningInfo;

class CAnimationCompressor
{
public:
    CAnimationCompressor(const CInternalSkinningInfo* skinningInfo);
    CAnimationCompressor(const CSkeletonInfo& skeleton);
    ~CAnimationCompressor(void);

    bool LoadCAF(const char* name, const SPlatformAnimationSetup& platform, const SAnimationDesc& animDesc, const CInternalSkinningInfo* controllerSkinningInfo, ECAFLoadMode mode);
    bool LoadCAFFromMemory(const char* name, const SAnimationDesc& animDesc, ECAFLoadMode loadMode, CContentCGF* content, CInternalSkinningInfo* controllerSkinningInfo);
    void LoadCAFPostProcess(const char* name, const SAnimationDesc& animDesc, const CInternalSkinningInfo* controllerSkinningInfo, ECAFLoadMode loadMode);
    bool ProcessAnimation(bool debugCompression);

    uint32 AddCompressedChunksToFile(const char* name, bool bigEndianOutput);
    uint32 SaveOnlyCompressedChunksInFile(const char* name, FILETIME timeStamp, GlobalAnimationHeaderAIM* gaim, bool saveCAFHeader, bool bigEndianOutput);

    bool ProcessAimPoses(bool isLook, GlobalAnimationHeaderAIM& rAIM);

    bool CompareKeyTimes(KeyTimesInformationPtr& ptr1, KeyTimesInformationPtr& ptr2);

    // Export functions for use in FBX imported animations, since we process data differently.
    bool ProcessAnimationFBX(bool debugCompression);
    bool ProcessRootTracksFBX();
    void EvaluateSpeedFBX();
    void DetectCycleAnimationsFBX();

    // Legacy export functions.
    bool ProcessAnimationLegacy(bool debugCompression);
    void EvaluateStartLocationLegacy();
    void ReplaceRootByLocatorLegacy(bool bCheckLocator, float minRootPosEpsilon, float minRootRotEpsilon);
    void SetIdentityLocatorLegacy(GlobalAnimationHeaderAIM& gaim);
    void EvaluateSpeedLegacy();
    void DetectCycleAnimationsLegacy();

    void EnableTrackAligning(bool bEnable);

    uint32 SaveControllers(CSaverCGF& saver, TControllersVector& controllers, bool bigEndianOutput);
    void SaveMotionParameters(CChunkFile* pChunkFile, bool bigEndianOutput);
    bool SaveController831(CSaverCGF& saver, CController* pController, bool bigEndianOutput);

    GlobalAnimationHeaderCAF m_GlobalAnimationHeader;
    GlobalAnimationHeaderCAF m_CompressedAnimation;
    GlobalAnimationHeaderCAF m_LastGoodAnimation;
    CSkeletonInfo m_skeleton;
    SAnimationDesc m_AnimDesc;
    CChunkFile m_ChunkFile;
    const CInternalSkinningInfo* m_skinningInfo;

private:
    void DeleteCompressedChunks();
    void ExtractKeys(std::vector<int>& times, std::vector<PQLog>& logKeys, const CControllerPQLog* controller) const;
    int ConvertRemainingControllers(TControllersVector& controllers) const;
    static CController* CreateNewController(int controllerID, int flags, const std::vector<int>& arrFullTimes, const std::vector<PQLog>& arrFullQTKeys, float positionTolerance, float rotationToleranceInDegrees);
    static CController* CreateNewControllerAIM(int controllerID, int flags, const std::vector<int>& arrFullTimes, const std::vector<PQLog>& arrFullQTKeys);

    string m_filename; // filename which was used to LoadCAF
    SPlatformAnimationSetup m_platformSetup;
    bool m_bAlignTracks;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONLOADER_H
