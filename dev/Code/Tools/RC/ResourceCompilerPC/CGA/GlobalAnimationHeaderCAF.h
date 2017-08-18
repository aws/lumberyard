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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_GLOBALANIMATIONHEADERCAF_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_GLOBALANIMATIONHEADERCAF_H
#pragma once


#include <CryCharAnimationParams.h>
#include "GlobalAnimationHeader.h"
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
#include "CryCrc32.h"
#include "Util.h"

//#define PRINTOUT (1)

struct SFootPlant
{
    f32 m_LHeelStart, m_LHeelEnd;
    f32 m_LToe0Start, m_LToe0End;
    f32 m_RHeelStart, m_RHeelEnd;
    f32 m_RToe0Start, m_RToe0End;

    SFootPlant()
    {
        m_LHeelStart = -10000.0f;
        m_LHeelEnd = -10000.0f;
        m_LToe0Start = -10000.0f;
        m_LToe0End = -10000.0f;
        m_RHeelStart = -10000.0f;
        m_RHeelEnd = -10000.0f;
        m_RToe0Start = -10000.0f;
        m_RToe0End = -10000.0f;
    };
};

ILINE SFootPlant operator * (const SFootPlant& fp, f32 t)
{
    SFootPlant rfp;
    rfp.m_LHeelStart = fp.m_LHeelStart * t;
    rfp.m_LHeelEnd = fp.m_LHeelEnd * t;
    rfp.m_LToe0Start = fp.m_LToe0Start * t;
    rfp.m_LToe0End = fp.m_LToe0End * t;
    rfp.m_RHeelStart = fp.m_RHeelStart * t;
    rfp.m_RHeelEnd = fp.m_RHeelEnd * t;
    rfp.m_RToe0Start = fp.m_RToe0Start * t;
    rfp.m_RToe0End = fp.m_RToe0End * t;
    return rfp;
}

//vector self-addition
ILINE void operator += (SFootPlant& v0, const SFootPlant& v1)
{
    v0.m_LHeelStart += v1.m_LHeelStart;
    v0.m_LHeelEnd += v1.m_LHeelEnd;
    v0.m_LToe0Start += v1.m_LToe0Start;
    v0.m_LToe0End += v1.m_LToe0End;
    v0.m_RHeelStart += v1.m_RHeelStart;
    v0.m_RHeelEnd += v1.m_RHeelEnd;
    v0.m_RToe0Start += v1.m_RToe0Start;
    v0.m_RToe0End += v1.m_RToe0End;
}


enum ECAFLoadMode
{
    eCAFLoadUncompressedOnly,
    eCAFLoadCompressedOnly
};

// shared code for CAF loading, not best place for it possible
bool CAF_EnsureLoadModeApplies(const IChunkFile* chunkFile, ECAFLoadMode loadMode, const char* logFilename);
bool CAF_ReadController(IChunkFile::ChunkDesc* chunk, ECAFLoadMode loadMode, IController_AutoPtr& controller, int32& lastControllerID, const char* logFilename, bool& strippedFrames);

struct GlobalAnimationHeaderCAF
    : public GlobalAnimationHeader
{
    GlobalAnimationHeaderCAF();

    virtual ~GlobalAnimationHeaderCAF()
    {
    }


    bool LoadCAF(const string& filename, ECAFLoadMode loadMethod, CChunkFile* chunkFile = 0);
    bool LoadCAFFromChunkFile(const string& filename, ECAFLoadMode loadMode, CChunkFile* chunkFile);
    bool SaveToChunkFile(IChunkFile* file, bool bigEndianOutput) const;
    void ExtractMotionParameters(MotionParams905* mp, bool bigEndianOutput) const;

    const char* GetFilePath() const {   return m_FilePath.c_str(); };
    void SetFilePathCAF(const string& name)
    {
        m_FilePath = name;
        const char* pname = m_FilePath.c_str();
        m_FilePathCRC32 = CCrc32::ComputeLowercase(pname);
    }

    void SetFilePathDBA(const string& name)
    {
        m_FilePathDBA = name;
        m_FilePathDBACRC32 = CCrc32::ComputeLowercase(name.c_str());
    }

    f32 NTime2KTime(f32 ntime)
    {
        ntime = Util::getMin(ntime, 1.0f);
        assert(ntime >= 0 && ntime <= 1);
        f32 duration = m_fEndSec - m_fStartSec;
        f32 start = m_fStartSec;
        f32 framesPerSecond = 1.f / (m_fSecsPerTick * m_nTicksPerFrame);
        f32 key = (ntime * duration * framesPerSecond) + (start * framesPerSecond);
        return key;
    }

    IController* GetController(uint32 nControllerID);
    bool ReplaceController(IController* old, IController* newContoller);
    size_t CountCompressedControllers() const;

    const char* GetOriginalControllerName(uint32 nControllerId) const;

    uint32 m_nCompression;
    string m_FilePath;                              //path-name - unique per-model
    uint32 m_FilePathCRC32;                     //hash value for searching animations
    string m_FilePathDBA;                     //path-name of DBA (if this animation is is a DBA)
    uint32 m_FilePathDBACRC32;              //hash value (if the file is coming from a DBA)

    // timing data, retrieved from the timing_chunk_desc
    int32 m_nTicksPerFrame;
    f32 m_fSecsPerTick;
    int32 m_nStartKey;
    int32 m_nEndKey;
    f32 m_fStartSec; //Start time in seconds.
    f32 m_fEndSec; //End time in seconds.

    SFootPlant m_FootPlantVectors;
    std::vector<uint8> m_FootPlantBits;

    std::vector<CControllerInfo> m_arrControllerInfo;
    // controllers comprising the animation; within the animation, they're sorted by ids
    TControllersVector m_arrController;
    uint32 m_nCompressedControllerCount;

    f32 m_fDistance; //the absolute distance this objects is moving
    f32 m_fSpeed; //speed (meters in second)
    f32 m_fSlope; //uphill-downhill measured in degrees
    f32 m_fTurnSpeed;                   //asset-features
    f32 m_fAssetTurn;                   //asset-features

    QuatT m_StartLocation2;
    QuatT m_LastLocatorKey2;

    struct SOriginalControllerNameAndId
    {
        SOriginalControllerNameAndId()
            : id(0)
        {
        }

        string name;
        uint32 id;
    };
    std::vector<SOriginalControllerNameAndId> m_arrOriginalControllerNameAndId;

private:
    bool LoadChunks(IChunkFile* chunkFile, ECAFLoadMode loadMode, int32& lastControllerID, const char* logFilename);
    bool ReadMotionParameters(const IChunkFile::ChunkDesc* chunk, ECAFLoadMode loadMode, const char* logFilename);
    bool ReadTiming(const IChunkFile::ChunkDesc* chunk, ECAFLoadMode loadMode, const char* logFilename);
    bool ReadGlobalAnimationHeader(const IChunkFile::ChunkDesc* chunk, ECAFLoadMode loadMode, const char* logFilename);
    bool ReadBoneNameList(IChunkFile::ChunkDesc* pChunkDesc, ECAFLoadMode loadMode, const char* logFilename);
    void BindControllerIdsToControllerNames();
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_GLOBALANIMATIONHEADERCAF_H
