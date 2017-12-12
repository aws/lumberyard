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

#include "StdAfx.h"
#include "AnimSaver.h"
#include "CGF/ChunkData.h"
#include "StringHelpers.h"


CSaverAnim::CSaverAnim(const char* filename, CChunkFile& chunkFile)
{
    m_filename = filename;
    m_pChunkFile = &chunkFile;
}

int CSaverAnim::SaveExportFlags(const CContentCGF* const pCGF)
{
    EXPORT_FLAGS_CHUNK_DESC chunk;
    ZeroStruct(chunk);

    const CExportInfoCGF* pExpInfo = pCGF->GetExportInfo();
    if (pExpInfo->bMergeAllNodes)
    {
        chunk.flags |= EXPORT_FLAGS_CHUNK_DESC::MERGE_ALL_NODES;
    }
    if (pExpInfo->bUseCustomNormals)
    {
        chunk.flags |= EXPORT_FLAGS_CHUNK_DESC::USE_CUSTOM_NORMALS;
    }
    if (pExpInfo->bWantF32Vertices)
    {
        chunk.flags |= EXPORT_FLAGS_CHUNK_DESC::WANT_F32_VERTICES;
    }
    if (pExpInfo->b8WeightsPerVertex)
    {
        chunk.flags |= EXPORT_FLAGS_CHUNK_DESC::EIGHT_WEIGHTS_PER_VERTEX;
    }

    if (pExpInfo->bFromColladaXSI)
    {
        chunk.assetAuthorTool |= EXPORT_FLAGS_CHUNK_DESC::FROM_COLLADA_XSI;
    }
    if (pExpInfo->bFromColladaMAX)
    {
        chunk.assetAuthorTool |= EXPORT_FLAGS_CHUNK_DESC::FROM_COLLADA_MAX;
    }
    if (pExpInfo->bFromColladaMAYA)
    {
        chunk.assetAuthorTool |= EXPORT_FLAGS_CHUNK_DESC::FROM_COLLADA_MAYA;
    }

    enum
    {
        NUM_RC_VERSION_ELEMENTS = sizeof(pExpInfo->rc_version) / sizeof(pExpInfo->rc_version[0])
    };
    std::copy(pExpInfo->rc_version, pExpInfo->rc_version + NUM_RC_VERSION_ELEMENTS, chunk.rc_version);
    StringHelpers::SafeCopy(chunk.rc_version_string, sizeof(chunk.rc_version_string), pExpInfo->rc_version_string);

    return m_pChunkFile->AddChunk(
        ChunkType_ExportFlags,
        EXPORT_FLAGS_CHUNK_DESC::VERSION,
        eEndianness_Native,
        &chunk, sizeof(chunk));
}

int CSaverAnim::SaveTiming(const CInternalSkinningInfo* const pSkinningInfo)
{
    return SaveTiming(m_pChunkFile, pSkinningInfo);
}

int CSaverAnim::SaveTCB3Track(CChunkFile* pChunkFile, const CInternalSkinningInfo* const pSkinningInfo, int trackIndex)
{
    CONTROLLER_CHUNK_DESC_0826 chunk;
    ZeroStruct(chunk);

    chunk.type = CTRL_TCB3;
    chunk.nKeys = pSkinningInfo->m_TrackVec3[trackIndex]->size();

    CChunkData chunkData;
    chunkData.Add(chunk);
    chunkData.AddData(&(*pSkinningInfo->m_TrackVec3[trackIndex])[0], chunk.nKeys * sizeof(CryTCB3Key));

    return pChunkFile->AddChunk(
        ChunkType_Controller,
        CONTROLLER_CHUNK_DESC_0826::VERSION,
        eEndianness_Native,
        chunkData.data, chunkData.size);
}

int CSaverAnim::SaveTCBQTrack(CChunkFile* pChunkFile, const CInternalSkinningInfo* const pSkinningInfo, int trackIndex)
{
    CONTROLLER_CHUNK_DESC_0826 chunk;
    ZeroStruct(chunk);

    chunk.type = CTRL_TCBQ;
    chunk.nKeys = pSkinningInfo->m_TrackQuat[trackIndex]->size();

    CChunkData chunkData;
    chunkData.Add(chunk);
    chunkData.AddData(&(*pSkinningInfo->m_TrackQuat[trackIndex])[0], chunk.nKeys * sizeof(CryTCBQKey));

    return pChunkFile->AddChunk(
        ChunkType_Controller,
        CONTROLLER_CHUNK_DESC_0826::VERSION,
        eEndianness_Native,
        chunkData.data, chunkData.size);
}

int CSaverAnim::SaveTiming(CChunkFile* pChunkFile, const CInternalSkinningInfo* const pSkinningInfo)
{
    TIMING_CHUNK_DESC_0918 chunk;
    ZeroStruct(chunk);

    chunk.m_TicksPerFrame = pSkinningInfo->m_nTicksPerFrame;
    chunk.m_SecsPerTick = pSkinningInfo->m_secsPerTick;
    chunk.global_range.start = pSkinningInfo->m_nStart;
    chunk.global_range.end = pSkinningInfo->m_nEnd;
    cry_strcpy(chunk.global_range.name, "GlobalRange");     //TODO: for what? (copied from max exporter)

    return pChunkFile->AddChunk(
        ChunkType_Timing,
        TIMING_CHUNK_DESC_0918::VERSION,
        eEndianness_Native,
        &chunk, sizeof(chunk));
}
