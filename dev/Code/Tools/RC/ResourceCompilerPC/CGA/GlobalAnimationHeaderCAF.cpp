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
#include "GlobalAnimationHeaderCAF.h"
#include "AnimationCompiler.h" // for SEndiannessSwapper
#include "CGF/ChunkData.h"

static const size_t MAX_NUMBER_OF_KEYS = 10000;

GlobalAnimationHeaderCAF::GlobalAnimationHeaderCAF()
{
    m_nFlags = 0;
    m_nCompression       = -2;

    m_FilePathCRC32      = 0;
    m_FilePathDBACRC32 = 0;

    m_fSecsPerTick   = 0;
    m_nTicksPerFrame = 0;
    m_nStartKey = -1;
    m_nEndKey   = -1;
    m_fStartSec = -1; // Start time in seconds.
    m_fEndSec   = -1; // End time in seconds.
    m_nCompressedControllerCount = 0;

    m_fDistance     = -1.0f;
    m_fSpeed            = -1.0f;
    m_fSlope            =  0.0f;
    m_fTurnSpeed    =   0.0f;                   //asset-features
    m_fAssetTurn    =   0.0f;                   //asset-features

    m_StartLocation2.SetIdentity();
    m_LastLocatorKey2.SetIdentity();
}

IController* GlobalAnimationHeaderCAF::GetController(uint32 nControllerID)
{
    const size_t numController = m_arrController.size();
    for (size_t i = 0; i < numController; ++i)
    {
        if (m_arrController[i] && m_arrController[i]->GetID() == nControllerID)
        {
            return m_arrController[i];
        }
    }
    return 0;
}


bool GlobalAnimationHeaderCAF::ReplaceController(IController* old, IController* newContoller)
{
    const size_t numControllers = m_arrController.size();
    for (size_t i = 0; i < numControllers; ++i)
    {
        if (m_arrController[i] == old)
        {
            m_arrController[i] = newContoller;
            return true;
        }
    }
    return false;
}

size_t GlobalAnimationHeaderCAF::CountCompressedControllers() const
{
    size_t result = 0;

    const size_t numControllers = m_arrController.size();
    for (uint32 i = 0; i < numControllers; ++i)
    {
        const IController_AutoPtr& ptr = m_arrController[i];
        if (ptr && ptr->GetCController())
        {
            bool alreadyCounted = false;
            for (size_t j = 0; j < i; ++j)
            {
                if (ptr.get() == m_arrController[j].get())
                {
                    alreadyCounted = true;
                    break;
                }
            }
            if (!alreadyCounted)
            {
                ++result;
            }
        }
    }
    return result;
}

bool GlobalAnimationHeaderCAF::LoadCAF(const string& filename, ECAFLoadMode loadMode, CChunkFile* chunkFile)
{
    CChunkFile localChunkFile;
    if (!chunkFile)
    {
        chunkFile = &localChunkFile;
    }

    if (!chunkFile->Read(filename.c_str()))
    {
        RCLogError("Chunk file error: %s", chunkFile->GetLastError());
        return false;
    }

    return LoadCAFFromChunkFile(filename, loadMode, chunkFile);
}

//
// Implemented by referencing to GlobalAnimationHeaderCAF::LoadCAF
//
bool GlobalAnimationHeaderCAF::LoadCAFFromChunkFile(const string& filename, ECAFLoadMode loadMode, CChunkFile* chunkFile)
{
    m_arrController.clear();
    m_FootPlantBits.clear();

    int32 lastControllerID = -1;
    if (!LoadChunks(chunkFile, loadMode, lastControllerID, filename.c_str()))
    {
        // TODO descriptive message
        /*
        const char *lastControllerName = "NONE";
        for (uint32 boneID = 0; boneID < m_pInfo->m_SkinningInfo.m_arrBonesDesc.size(); boneID++)
        if (lastControllerID == m_pInfo->m_SkinningInfo.m_arrBonesDesc[boneID].m_nControllerID)
        {
        lastControllerName = m_pInfo->m_SkinningInfo.m_arrBonesDesc[boneID].m_arrBoneName;
        break;
        }
        RCLogError( "Failed to load animation file %s - %s \n Last Controller: %s", filename.c_str(), cgfLoader.GetLastError(), lastControllerName);
        */
        RCLogError("Failed to load animation file %s \n Last Controller: %i", filename.c_str(), lastControllerID);
        return false;
    }

    BindControllerIdsToControllerNames();

    return true;
}

static CControllerPQLog* ReadControllerV0830(const CONTROLLER_CHUNK_DESC_0830* chunk, bool bSwapEndianness, const char* chunkStart, const char* chunkEnd, const char* logFilename, bool& strippedFrames)
{
    int numKeys = chunk->numKeys;
    if (numKeys > MAX_NUMBER_OF_KEYS)
    {
        RCLogError("animation not loaded: insane number of keys (%u). Please stay below %u: %s", numKeys, MAX_NUMBER_OF_KEYS, logFilename);
        return 0;
    }
    CryKeyPQLog* pCryKey = (CryKeyPQLog*)(chunkStart);
    SwapEndian(pCryKey, numKeys, bSwapEndianness);
    if ((char*)(pCryKey + numKeys) > chunkEnd)
    {
        RCLogError("animation not loaded: estimated keys data size exceeds chunk size: %s", logFilename);
        return 0;
    }

    std::unique_ptr<CControllerPQLog> controller(new CControllerPQLog);
    controller->m_nControllerId = chunk->nControllerId;
    controller->SetFlags(chunk->nFlags);

    controller->m_arrKeys.reserve(numKeys);
    controller->m_arrTimes.reserve(numKeys);

    int lasttime = INT_MIN;
    for (int i = 0; i < numKeys; ++i)
    {
        // Using zero-based time allows our compressor to encode time as uint8 or uint16 instead of float
        // We are adding +1 here as some assets have negative keys shifted by one tick, eg. -319 -159 0 160 320 ...
        int curtime = (pCryKey[i].nTime - pCryKey[0].nTime + 1) / TICKS_CONVERT;
        if (curtime > lasttime)
        {
            if (!pCryKey[i].vPos.IsValid())
            {
                RCLogError("animation not loaded: controller key %u has invalid float value(s) in position: %s", i, logFilename);
                return 0;
            }

            if (!pCryKey[i].vRotLog.IsValid())
            {
                RCLogError("animation not loaded: controller key %u has invalid float value(s) in rotation: %s", i, logFilename);
                return 0;
            }
            PQLog pq;
            pq.vPos = pCryKey[i].vPos / 100.0f;
            pq.vRotLog = pCryKey[i].vRotLog;
            controller->m_arrKeys.push_back(pq);
            controller->m_arrTimes.push_back(curtime);
            lasttime = curtime;
        }
        else if (curtime == lasttime && i == numKeys - 1)
        {
            // Gap between last two frames can be different from typical frame duration.
            strippedFrames = true;
        }
        else
        {
            RCLogError("controller contains repeated or non-sorted time key: %s", logFilename);
            return 0;
        }
    }
    return controller.release();
}

static CController* ReadControllerV0831(const CONTROLLER_CHUNK_DESC_0831* chunk, bool bSwapEndianness, char* chunkStart, const char* chunkEnd, const char* logFilename)
{
    if (bSwapEndianness)
    {
        RCLogError("%s: changing endianness is not implemented yet: %s", __FUNCTION__, logFilename);
        return 0;
    }

    std::unique_ptr<CController> pController(new CController);
    pController->m_nControllerId = chunk->nControllerId;
    pController->SetFlags(chunk->nFlags);

    char* pData = chunkStart;
    int nAlignment = chunk->TracksAligned ? 4 : 1;

    KeyTimesInformationPtr pRotTimeKeys = 0;

    if (chunk->numRotationKeys)
    {
        if (chunk->numRotationKeys > MAX_NUMBER_OF_KEYS)
        {
            RCLogError("animation not loaded: insane number of rotation keys (%u). Please stay below %u. %s", (unsigned)chunk->numRotationKeys, MAX_NUMBER_OF_KEYS, logFilename);
            return 0;
        }
        // we have a rotation info
        TrackRotationStoragePtr pStorage = ControllerHelper::GetRotationControllerPtr((ECompressionFormat)chunk->RotationFormat);
        if (!pStorage)
        {
            RCLogError("Unable to to get rotation storage: %s", logFilename);
            return 0;
        }

        RotationControllerPtr rotation = RotationControllerPtr(new RotationTrackInformation);
        rotation->SetRotationStorage(pStorage);


        pStorage->Resize(chunk->numRotationKeys);
        if (pData + pStorage->GetDataRawSize() > chunkEnd)
        {
            RCLogError("animation not loaded: estimated data size exceeds chunk size: %s", logFilename);
            return 0;
        }
        // its not very safe but its extremely fast!
        memcpy(pStorage->GetData(), pData, pStorage->GetDataRawSize());
        pData += Align(pStorage->GetDataRawSize(), nAlignment);

        pRotTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(chunk->RotationTimeFormat);//new F32KeyTimesInformation;//CKeyTimesInformation;
        pRotTimeKeys->ResizeKeyTime(chunk->numRotationKeys);

        if (pData + pRotTimeKeys->GetDataRawSize() > chunkEnd)
        {
            RCLogError("animation not loaded: estimated data size exceeds chunk size: %s", logFilename);
            return 0;
        }
        memcpy(pRotTimeKeys->GetData(), pData, pRotTimeKeys->GetDataRawSize());
        pData += Align(pRotTimeKeys->GetDataRawSize(), nAlignment);

        rotation->SetKeyTimesInformation(pRotTimeKeys);
        pController->SetRotationController(rotation);

        float lastTime = -FLT_MAX;
        for (int i = 0; i < chunk->numRotationKeys; ++i)
        {
            float t = pRotTimeKeys->GetKeyValueFloat(i);
            if (t <= lastTime)
            {
                RCLogError("Controller contains repeated or non-sorted time keys");
                return 0;
            }
            lastTime = t;
        }
    }

    if (chunk->numPositionKeys)
    {
        if (chunk->numPositionKeys > MAX_NUMBER_OF_KEYS)
        {
            RCLogError("animation not loaded: insane number of position keys (%u). Please stay below %u. %s", (unsigned)chunk->numPositionKeys, MAX_NUMBER_OF_KEYS, logFilename);
            return 0;
        }

        TrackPositionStoragePtr pStorage = ControllerHelper::GetPositionControllerPtr((ECompressionFormat)chunk->PositionFormat);
        if (!pStorage)
        {
            RCLogError("Unable to to get position storage: %s", logFilename);
            return 0;
        }

        PositionControllerPtr pPosition = PositionControllerPtr(new PositionTrackInformation);
        pPosition->SetPositionStorage(pStorage);
        pStorage->Resize(chunk->numPositionKeys);
        if (pData + pStorage->GetDataRawSize() > chunkEnd)
        {
            RCLogError("animation not loaded: estimated data size exceeds chunk size: %s", logFilename);
            return 0;
        }

        memcpy(pStorage->GetData(), pData, pStorage->GetDataRawSize());
        pData += Align(pStorage->GetDataRawSize(), nAlignment);

        if (chunk->PositionKeysInfo == CONTROLLER_CHUNK_DESC_0831::eKeyTimeRotation)
        {
            pPosition->SetKeyTimesInformation(pRotTimeKeys);
        }
        else
        {
            // load from chunk
            KeyTimesInformationPtr pPosTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(chunk->PositionTimeFormat);
            ;                                                                                                            //PositionTimeFormat;new F32KeyTimesInformation;//CKeyTimesInformation;
            pPosTimeKeys->ResizeKeyTime(chunk->numPositionKeys);

            if (pData + pPosTimeKeys->GetDataRawSize() > chunkEnd)
            {
                RCLogError("animation not loaded: estimated data size exceeds chunk size: %s", logFilename);
                return 0;
            }
            memcpy(pPosTimeKeys->GetData(), pData, pPosTimeKeys->GetDataRawSize());
            pData += Align(pPosTimeKeys->GetDataRawSize(), nAlignment);

            pPosition->SetKeyTimesInformation(pPosTimeKeys);

            float lastTime = -FLT_MAX;
            for (int i = 0; i < chunk->numPositionKeys; ++i)
            {
                float t = pPosTimeKeys->GetKeyValueFloat(i);
                if (t <= lastTime)
                {
                    RCLogError("Controller contains repeating or non-sorted time keys");
                    return 0;
                }
                lastTime = t;
            }
        }
        pController->SetPositionController(pPosition);
    }
    return pController.release();
}

static CControllerPQLog* ReadControllerV0827(const CONTROLLER_CHUNK_DESC_0827* chunk, bool bSwapEndianness, const char* chunkEnd, const char* logFilename, bool& strippedFrames)
{
    CONTROLLER_CHUNK_DESC_0830 newFormatChunk(chunk);
    char* chunkStart = (char*)(chunk + 1);

    return ReadControllerV0830(&newFormatChunk, bSwapEndianness, chunkStart, chunkEnd, logFilename, strippedFrames);
}

static CController* ReadControllerV0829(const CONTROLLER_CHUNK_DESC_0829* chunk, bool bSwapEndianness, const char* chunkEnd, const char* logFilename)
{
    CONTROLLER_CHUNK_DESC_0831 newFormatChunk(chunk);
    char* chunkStart = (char*)(chunk + 1);

    return ReadControllerV0831(&newFormatChunk, bSwapEndianness, chunkStart, chunkEnd, logFilename);
}


bool CAF_ReadController(IChunkFile::ChunkDesc* chunk, ECAFLoadMode loadMode, IController_AutoPtr& controller, int32& lastControllerID, const char* logFilename, bool& strippedFrames)
{
    const char* chunkEnd = (const char*)chunk->data + chunk->size;

    switch (chunk->chunkVersion)
    {
    case CONTROLLER_CHUNK_DESC_0826::VERSION:
    {
        RCLogError("Controller V0826 is not support any more: %s", logFilename);
        return false;
    }
    case CONTROLLER_CHUNK_DESC_0827::VERSION:
    {
        if (loadMode == eCAFLoadCompressedOnly)
        {
            RCLogError("Old controller (V0827) in 'compressed' animation. Aborting: %s", logFilename);
            return false;
        }

        CONTROLLER_CHUNK_DESC_0827* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0827*)chunk->data;
        const bool bSwapEndianness = chunk->bSwapEndian;
        SwapEndian(*pCtrlChunk, bSwapEndianness);
        chunk->bSwapEndian = false;
        lastControllerID = pCtrlChunk->nControllerId;
        if (controller = ReadControllerV0827(pCtrlChunk, bSwapEndianness, chunkEnd, logFilename, strippedFrames))
        {
            return true;
        }
        return false;
    }
    case CONTROLLER_CHUNK_DESC_0828::VERSION:
    {
        RCLogWarning("Animation contains controllers in unsupported format 0x0828, please reexport it: %s", logFilename);
        controller = 0;
        return true;
    }
    case CONTROLLER_CHUNK_DESC_0829::VERSION:
    {
        if (loadMode == eCAFLoadUncompressedOnly)
        {
            // skip compressed controllers
            controller = 0;
            return true;
        }

        CONTROLLER_CHUNK_DESC_0829* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0829*)chunk->data;
        const bool bSwapEndianness = chunk->bSwapEndian;
        SwapEndian(*pCtrlChunk, bSwapEndianness);
        chunk->bSwapEndian = false;
        lastControllerID = pCtrlChunk->nControllerId;
        if (controller = ReadControllerV0829(pCtrlChunk, bSwapEndianness, chunkEnd, logFilename))
        {
            return true;
        }
        return false;
    }
    case CONTROLLER_CHUNK_DESC_0830::VERSION:
    {
        if (loadMode == eCAFLoadCompressedOnly)
        {
            RCLogError("Old controller (V0827) in 'compressed' animation. Aborting: %s", logFilename);
            return false;
        }

        CONTROLLER_CHUNK_DESC_0830* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0830*)chunk->data;
        const bool bSwapEndianness = chunk->bSwapEndian;
        SwapEndian(*pCtrlChunk, bSwapEndianness);
        chunk->bSwapEndian = false;
        lastControllerID = pCtrlChunk->nControllerId;
        char* chunkStart = (char*)(pCtrlChunk + 1);
        if (controller = ReadControllerV0830(pCtrlChunk, bSwapEndianness, chunkStart, chunkEnd, logFilename, strippedFrames))
        {
            return true;
        }
        return false;
    }
    case CONTROLLER_CHUNK_DESC_0831::VERSION:
    {
        if (loadMode == eCAFLoadUncompressedOnly)
        {
            // skip compressed controllers
            controller = 0;
            return true;
        }

        CONTROLLER_CHUNK_DESC_0831* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0831*)chunk->data;
        const bool bSwapEndianness = chunk->bSwapEndian;
        SwapEndian(*pCtrlChunk, bSwapEndianness);
        chunk->bSwapEndian = false;
        lastControllerID = pCtrlChunk->nControllerId;
        char* chunkStart = (char*)(pCtrlChunk + 1);
        if (controller = ReadControllerV0831(pCtrlChunk, bSwapEndianness, chunkStart, chunkEnd, logFilename))
        {
            return true;
        }
        return false;
    }
    default:
        RCLogWarning("Unknown controller chunk version (0x%08x): %s", chunk->chunkVersion, logFilename);
        return false;
    }
}

bool CAF_EnsureLoadModeApplies(const IChunkFile* chunkFile, ECAFLoadMode loadMode, const char* logFilename)
{
    int numChunks = chunkFile->NumChunks();

    bool hasTCBFormat = false;
    bool hasOldFormat = false;
    bool hasNewFormat = false;

    for (int i = 0; i < numChunks; i++)
    {
        const IChunkFile::ChunkDesc* const pChunkDesc = chunkFile->GetChunk(i);

        if (pChunkDesc->chunkType == ChunkType_Controller)
        {
            switch (pChunkDesc->chunkVersion)
            {
            case CONTROLLER_CHUNK_DESC_0826::VERSION:
                hasTCBFormat = true;
                break;
            case CONTROLLER_CHUNK_DESC_0827::VERSION:
            case CONTROLLER_CHUNK_DESC_0828::VERSION:
            case CONTROLLER_CHUNK_DESC_0830::VERSION:
                hasOldFormat = true;
                break;
            case CONTROLLER_CHUNK_DESC_0829::VERSION:
            case CONTROLLER_CHUNK_DESC_0831::VERSION:
                hasNewFormat = true;
                break;
            default:
                hasNewFormat = true;
                RCLogWarning("Unrecognized controller chunk version (0x%08x): %s", pChunkDesc->chunkVersion, logFilename);
                break;
            }
        }
    }

    if (loadMode == eCAFLoadUncompressedOnly && !hasOldFormat)
    {
        RCLogError("CAF doesn't contain animation in uncompressed format: %s", logFilename);
        return false;
    }

    if (loadMode == eCAFLoadCompressedOnly && !hasNewFormat && (hasOldFormat || hasTCBFormat))
    {
        RCLogWarning("CAF doesn't contain compressed animation (in new format): %s", logFilename);
    }

    return true;
}

bool GlobalAnimationHeaderCAF::ReadMotionParameters(const IChunkFile::ChunkDesc* chunk, ECAFLoadMode loadMode, const char* logFilename)
{
    // TODO check chunk data size

    if (chunk->chunkVersion == SPEED_CHUNK_DESC_2::VERSION)
    {
        if (loadMode == eCAFLoadCompressedOnly)
        {
            RCLogWarning("Old motion-parameters chunk in 'compressed' animation: %s", logFilename);
            return false;
        }

        SPEED_CHUNK_DESC_2* desc = (SPEED_CHUNK_DESC_2*)chunk->data;
        SwapEndian(*desc);

        m_fSpeed        = desc->Speed;
        m_fDistance = desc->Distance;
        m_fSlope        = desc->Slope;

        const uint32 validFlags = (CA_ASSET_BIG_ENDIAN | CA_ASSET_CREATED | CA_ASSET_ONDEMAND | CA_ASSET_LOADED | CA_ASSET_ADDITIVE | CA_ASSET_CYCLE | CA_ASSET_NOT_FOUND);
        m_nFlags = desc->AnimFlags & validFlags;
        m_StartLocation2 = desc->StartPosition;
        assert(m_StartLocation2.IsValid());
        return true;
    }
    else if (chunk->chunkVersion == CHUNK_MOTION_PARAMETERS::VERSION)
    {
        if (loadMode == eCAFLoadUncompressedOnly)
        {
            // someone checked in a file, that has only compressed data.
            // RC will not process this data, but we should initialize the header correctly to prevent a crash
        }

        CHUNK_MOTION_PARAMETERS* c = (CHUNK_MOTION_PARAMETERS*)chunk->data;
        COMPILE_TIME_ASSERT(eLittleEndian == false);

        const MotionParams905* motionParams = &c->mp;

        m_nFlags = motionParams->m_nAssetFlags;
        m_nTicksPerFrame = motionParams->m_nTicksPerFrame;
        m_fSecsPerTick = motionParams->m_fSecsPerTick;
        m_nCompression = motionParams->m_nCompression;

        const int32 nStartKey = 0;
        const int32 nEndKey = motionParams->m_nEnd - motionParams->m_nStart;
        m_nStartKey = nStartKey;
        m_nEndKey   = nEndKey;

        const int32 nTicksPerFrame = m_nTicksPerFrame;
        const f32 fSecsPerTick = m_fSecsPerTick;
        const f32 fSecsPerFrame = fSecsPerTick * nTicksPerFrame;
        m_fStartSec = nStartKey * fSecsPerFrame;
        m_fEndSec = nEndKey * fSecsPerFrame;
        if (m_fEndSec <= m_fStartSec)
        {
            m_fEndSec  = m_fStartSec + (1.0f / 30.0f);
        }

        m_fSpeed = motionParams->m_fMoveSpeed;
        m_fTurnSpeed = motionParams->m_fTurnSpeed;
        m_fAssetTurn = motionParams->m_fAssetTurn;
        m_fDistance = motionParams->m_fDistance;
        m_fSlope = motionParams->m_fSlope;

        m_StartLocation2 = motionParams->m_StartLocation;
        m_LastLocatorKey2 = motionParams->m_EndLocation;

        m_FootPlantVectors.m_LHeelStart = motionParams->m_LHeelStart;
        m_FootPlantVectors.m_LHeelEnd = motionParams->m_LHeelEnd;
        m_FootPlantVectors.m_LToe0Start = motionParams->m_LToe0Start;
        m_FootPlantVectors.m_LToe0End = motionParams->m_LToe0End;
        m_FootPlantVectors.m_RHeelStart = motionParams->m_RHeelStart;
        m_FootPlantVectors.m_RHeelEnd = motionParams->m_RHeelEnd;
        m_FootPlantVectors.m_RToe0Start = motionParams->m_RToe0Start;
        m_FootPlantVectors.m_RToe0End = motionParams->m_RToe0End;

        return true;
    }
    else
    {
        RCLogWarning("Unknown version of motion parameters chunk: 0x%08x in %s", chunk->chunkVersion, logFilename);
        return false;
    }
}

bool GlobalAnimationHeaderCAF::ReadGlobalAnimationHeader(const IChunkFile::ChunkDesc* chunk, ECAFLoadMode loadMode, const char* logFilename)
{
    if (chunk->chunkVersion == CHUNK_GAHCAF_INFO::VERSION)
    {
        if (loadMode != eCAFLoadCompressedOnly)
        {
            RCLogError("Unexpected CHUNK_GAHCAF_INFO (ChunkType_GlobalAnimationHeaderCAF) in %s", logFilename);
            return false;
        }

        const CHUNK_GAHCAF_INFO* const gah = (CHUNK_GAHCAF_INFO*)chunk->data;

        m_FilePathCRC32 = gah->m_FilePathCRC32;
        m_FilePathDBACRC32 = gah->m_FilePathDBACRC32;

        const size_t maxFilePathLen = sizeof(gah->m_FilePath);
        const size_t chunkFilePathLen = strnlen(gah->m_FilePath, maxFilePathLen);
        m_FilePath = string(gah->m_FilePath, gah->m_FilePath + chunkFilePathLen);

        m_nFlags = gah->m_Flags;
        m_nCompressedControllerCount = gah->m_nControllers;

        m_fSpeed = gah->m_fSpeed;
        m_fDistance = gah->m_fDistance;
        m_fSlope = gah->m_fSlope;
        m_fAssetTurn = gah->m_fAssetTurn;
        m_fTurnSpeed = gah->m_fTurnSpeed;

        m_StartLocation2 = gah->m_StartLocation;
        m_LastLocatorKey2 = gah->m_LastLocatorKey;

        m_fStartSec = 0;
        m_fEndSec = gah->m_fEndSec - gah->m_fStartSec;

        m_FootPlantVectors.m_LHeelStart = gah->m_LHeelStart;
        m_FootPlantVectors.m_LHeelEnd   = gah->m_LHeelEnd;
        m_FootPlantVectors.m_LToe0Start = gah->m_LToe0Start;
        m_FootPlantVectors.m_LToe0End   = gah->m_LToe0End;
        m_FootPlantVectors.m_RHeelStart = gah->m_RHeelStart;
        m_FootPlantVectors.m_RHeelEnd   = gah->m_RHeelEnd;
        m_FootPlantVectors.m_RToe0Start = gah->m_RToe0Start;
        m_FootPlantVectors.m_RToe0End   = gah->m_RToe0End;
        return true;
    }
    else
    {
        RCLogWarning("Unknown GAH_CAF version (0x%08x) in %s", chunk->chunkVersion, logFilename);
        return false;
    }
}

bool GlobalAnimationHeaderCAF::ReadTiming(const IChunkFile::ChunkDesc* chunk, ECAFLoadMode loadMode, const char* logFilename)
{
    if (chunk->chunkVersion != TIMING_CHUNK_DESC_0918::VERSION)
    {
        RCLogWarning("Unknown timing chunk version (0x%08x)", chunk->chunkVersion);
        return false;
    }

    if (loadMode == eCAFLoadCompressedOnly)
    {
        RCLogWarning("Old timing chunk in 'compressed' animation: %s", logFilename);
        return true;
    }

    // TODO check chunk data size
    TIMING_CHUNK_DESC_0918* c = (TIMING_CHUNK_DESC_0918*)chunk->data;
    SwapEndian(*c);

    // Make keys zero-based. We will do the same for time stored in
    // controllers. It allows our compressor to encode time as uint8
    // or uint16 instead of float.
    m_nStartKey = 0;
    m_nEndKey = c->global_range.end - c->global_range.start;

    assert(m_nStartKey < 0xffff && m_nEndKey < 0xffff);

    m_nTicksPerFrame = c->m_TicksPerFrame;
    m_fSecsPerTick = c->m_SecsPerTick;

    f32 fSecsPerFrame = m_nTicksPerFrame * m_fSecsPerTick;
    m_fStartSec = m_nStartKey * fSecsPerFrame;
    m_fEndSec = m_nEndKey * fSecsPerFrame;

    if (m_fEndSec <= m_fStartSec)
    {
        m_fEndSec = m_fStartSec + fSecsPerFrame;
    }

    return true;
}

bool GlobalAnimationHeaderCAF::ReadBoneNameList(IChunkFile::ChunkDesc* const pChunkDesc, ECAFLoadMode loadMode, const char* const logFilename)
{
    if (pChunkDesc->chunkVersion != BONENAMELIST_CHUNK_DESC_0745::VERSION)
    {
        RCLogError("Unknown version of bone name list chunk in animation '%s'", logFilename);
        return false;
    }

    // the chunk contains variable-length packed strings following tightly each other
    BONENAMELIST_CHUNK_DESC_0745* const pNameChunk = (BONENAMELIST_CHUNK_DESC_0745*)(pChunkDesc->data);
    SwapEndian(*pNameChunk, pChunkDesc->bSwapEndian);
    pChunkDesc->bSwapEndian = false;

    const uint32 expectedNameCount = pNameChunk->numEntities;

    // we know how many strings there are there; note that the whole bunch
    // of strings may be followed by padding zeros
    m_arrOriginalControllerNameAndId.resize(expectedNameCount);

    // scan through all the strings (strings are zero-terminated)
    const char* const pNameListEnd = ((const char*)pNameChunk) + pChunkDesc->size;
    const char* pName = (const char*)(pNameChunk + 1);
    uint32 nameCount = 0;
    while (*pName && (pName < pNameListEnd) && (nameCount < expectedNameCount))
    {
        m_arrOriginalControllerNameAndId[nameCount].name = pName;
        pName += strnlen(pName, pNameListEnd - pName) + 1;
        ++nameCount;
    }
    if (nameCount < expectedNameCount)
    {
        RCLogError("Inconsistent bone name list chunk: only %d out of %d bone names have been read in animation '%s'", nameCount, expectedNameCount, logFilename);
        return false;
    }

    return true;
}

bool GlobalAnimationHeaderCAF::LoadChunks(IChunkFile* chunkFile, ECAFLoadMode loadMode, int32& lastControllerID, const char* logFilename)
{
    if (!CAF_EnsureLoadModeApplies(chunkFile, loadMode, logFilename))
    {
        return false;
    }

    int numChunk = chunkFile->NumChunks();
    m_arrController.reserve(numChunk);

    bool strippedFrames = false;

    for (int i = 0; i < numChunk; ++i)
    {
        IChunkFile::ChunkDesc* const pChunkDesc = chunkFile->GetChunk(i);
        switch (pChunkDesc->chunkType)
        {
        case ChunkType_Controller:
        {
            IController_AutoPtr controller;
            if (CAF_ReadController(pChunkDesc, loadMode, controller, lastControllerID, logFilename, strippedFrames))
            {
                if (controller)
                {
                    m_arrController.push_back(controller);
                }
            }
            else
            {
                RCLogError("Unable to read controller: %i (v%04x) in %s", lastControllerID, pChunkDesc->chunkVersion, logFilename);
                return false;
            }
            break;
        }
        case ChunkType_Timing:
            if (!ReadTiming(pChunkDesc, loadMode, logFilename))
            {
                return false;
            }
            break;

        case ChunkType_MotionParameters:
            if (!ReadMotionParameters(pChunkDesc, loadMode, logFilename))
            {
                return false;
            }
            break;
        case ChunkType_GlobalAnimationHeaderCAF:
            if (!ReadGlobalAnimationHeader(pChunkDesc, loadMode, logFilename))
            {
                return false;
            }
            break;
        case ChunkType_Timestamp:
            // ignore timestamp
            break;
        case ChunkType_BoneNameList:
            if (!ReadBoneNameList(pChunkDesc, loadMode, logFilename))
            {
                return false;
            }
            break;
        }
    }

    if (strippedFrames)
    {
        RCLogWarning("Stripping last incomplete frame of animation. Please adjust animation range to be a multiple of the frame duration and re-export animation: %s", logFilename);
    }

    return true;
}

bool GlobalAnimationHeaderCAF::SaveToChunkFile(IChunkFile* chunkFile, bool bigEndianOutput) const
{
    SEndiannessSwapper swap(bigEndianOutput);

    CHUNK_GAHCAF_INFO GAH_Info;
    ZeroStruct(GAH_Info);

    uint32 num = m_FilePath.size();
    if (num > sizeof(GAH_Info.m_FilePath) - 1)
    {
        RCLogError("Truncating animation path to %i characters: %s", sizeof(GAH_Info.m_FilePath) - 1, m_FilePath.c_str());
        num = sizeof(GAH_Info.m_FilePath) - 1;
    }
    memcpy(GAH_Info.m_FilePath, m_FilePath.c_str(), num);
    GAH_Info.m_FilePath[sizeof(GAH_Info.m_FilePath) - 1] = '\0';

    swap(GAH_Info.m_FilePathCRC32 = m_FilePathCRC32);
    swap(GAH_Info.m_FilePathDBACRC32 = m_FilePathDBACRC32);

    swap(GAH_Info.m_Flags   =   m_nFlags);
    swap(GAH_Info.m_nControllers = m_nCompressedControllerCount);

    swap(GAH_Info.m_fSpeed = m_fSpeed);
    assert(m_fDistance >= 0.0f);
    swap(GAH_Info.m_fDistance = m_fDistance);
    swap(GAH_Info.m_fSlope = m_fSlope);

    swap(GAH_Info.m_fAssetTurn = m_fAssetTurn);
    swap(GAH_Info.m_fTurnSpeed = m_fTurnSpeed);

    GAH_Info.m_StartLocation        = m_StartLocation2;
    swap(GAH_Info.m_StartLocation.q.w);
    swap(GAH_Info.m_StartLocation.q.v.x);
    swap(GAH_Info.m_StartLocation.q.v.y);
    swap(GAH_Info.m_StartLocation.q.v.z);
    swap(GAH_Info.m_StartLocation.t.x);
    swap(GAH_Info.m_StartLocation.t.y);
    swap(GAH_Info.m_StartLocation.t.z);

    GAH_Info.m_LastLocatorKey       = m_LastLocatorKey2;
    swap(GAH_Info.m_LastLocatorKey.q.w);
    swap(GAH_Info.m_LastLocatorKey.q.v.x);
    swap(GAH_Info.m_LastLocatorKey.q.v.y);
    swap(GAH_Info.m_LastLocatorKey.q.v.z);
    swap(GAH_Info.m_LastLocatorKey.t.x);
    swap(GAH_Info.m_LastLocatorKey.t.y);
    swap(GAH_Info.m_LastLocatorKey.t.z);

    swap(GAH_Info.m_fStartSec = m_fStartSec); //asset-feature: Start time in seconds.
    swap(GAH_Info.m_fEndSec = m_fEndSec); //asset-feature: End time in seconds.
    swap(GAH_Info.m_fTotalDuration = m_fEndSec - m_fStartSec); //asset-feature: asset-feature: total duration in seconds.

    swap(GAH_Info.m_LHeelStart = m_FootPlantVectors.m_LHeelStart);
    swap(GAH_Info.m_LHeelEnd   = m_FootPlantVectors.m_LHeelEnd);
    swap(GAH_Info.m_LToe0Start = m_FootPlantVectors.m_LToe0Start);
    swap(GAH_Info.m_LToe0End   = m_FootPlantVectors.m_LToe0End);
    swap(GAH_Info.m_RHeelStart = m_FootPlantVectors.m_RHeelStart);
    swap(GAH_Info.m_RHeelEnd   = m_FootPlantVectors.m_RHeelEnd);
    swap(GAH_Info.m_RToe0Start = m_FootPlantVectors.m_RToe0Start);
    swap(GAH_Info.m_RToe0End   = m_FootPlantVectors.m_RToe0End);

    CChunkData chunkData;
    chunkData.Add(GAH_Info);

    chunkFile->AddChunk(
        ChunkType_GlobalAnimationHeaderCAF,
        CHUNK_GAHCAF_INFO::VERSION,
        (bigEndianOutput ? eEndianness_Big : eEndianness_Little),
        chunkData.data, chunkData.size);

    return true;
}

void GlobalAnimationHeaderCAF::ExtractMotionParameters(MotionParams905* mp, bool bigEndianOutput) const
{
    SEndiannessSwapper swap(bigEndianOutput);

    swap(mp->m_nAssetFlags = m_nFlags);
    swap(mp->m_nCompression = m_nCompression);

    swap(mp->m_nTicksPerFrame   = m_nTicksPerFrame);
    swap(mp->m_fSecsPerTick     = m_fSecsPerTick);
    swap(mp->m_nStart                   = m_nStartKey);
    swap(mp->m_nEnd                     = m_nEndKey);

    swap(mp->m_fMoveSpeed = m_fSpeed);
    swap(mp->m_fAssetTurn = m_fAssetTurn);
    swap(mp->m_fTurnSpeed = m_fTurnSpeed);
    swap(mp->m_fDistance = m_fDistance);
    swap(mp->m_fSlope = m_fSlope);


    mp->m_StartLocation = m_StartLocation2;
    swap(mp->m_StartLocation.q.w);
    swap(mp->m_StartLocation.q.v.x);
    swap(mp->m_StartLocation.q.v.y);
    swap(mp->m_StartLocation.q.v.z);
    swap(mp->m_StartLocation.t.x);
    swap(mp->m_StartLocation.t.y);
    swap(mp->m_StartLocation.t.z);

    mp->m_EndLocation = m_LastLocatorKey2;
    swap(mp->m_EndLocation.q.w);
    swap(mp->m_EndLocation.q.v.x);
    swap(mp->m_EndLocation.q.v.y);
    swap(mp->m_EndLocation.q.v.z);
    swap(mp->m_EndLocation.t.x);
    swap(mp->m_EndLocation.t.y);
    swap(mp->m_EndLocation.t.z);

    swap(mp->m_LHeelEnd = m_FootPlantVectors.m_LHeelEnd);
    swap(mp->m_LHeelStart = m_FootPlantVectors.m_LHeelStart);
    swap(mp->m_LToe0Start = m_FootPlantVectors.m_LToe0Start);
    swap(mp->m_LToe0End = m_FootPlantVectors.m_LToe0End);

    swap(mp->m_RHeelEnd = m_FootPlantVectors.m_RHeelEnd);
    swap(mp->m_RHeelStart = m_FootPlantVectors.m_RHeelStart);
    swap(mp->m_RToe0Start = m_FootPlantVectors.m_RToe0Start);
    swap(mp->m_RToe0End = m_FootPlantVectors.m_RToe0End);
}

const char* GlobalAnimationHeaderCAF::GetOriginalControllerName(uint32 controllerId) const
{
    for (size_t i = 0; i < m_arrOriginalControllerNameAndId.size(); ++i)
    {
        if (m_arrOriginalControllerNameAndId[i].id == controllerId)
        {
            return m_arrOriginalControllerNameAndId[i].name.c_str();
        }
    }
    return "<unknown>";
}

void GlobalAnimationHeaderCAF::BindControllerIdsToControllerNames()
{
    const size_t numOriginalControllerNames = m_arrOriginalControllerNameAndId.size();

    if (!numOriginalControllerNames)
    {
        return; // there was no bone name list (or an empty one)
    }
    if (m_arrController.size() != numOriginalControllerNames) // if there is a bone name list, there should be an entry for every controller
    {
        RCLogWarning("Size of bone name table (%d) does not match number of controllers (%d). Ignoring bone name table.", (int)m_arrOriginalControllerNameAndId.size(), (int)m_arrController.size());
        return;
    }

    for (size_t i = 0; i < numOriginalControllerNames; ++i)
    {
        m_arrOriginalControllerNameAndId[i].id = m_arrController[i]->GetID();
    }
}
