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
#include "CAFSaver.h"
#include "CGF/ChunkData.h"


void CSaverCAF::Save(const CContentCGF* const pCGF, const CInternalSkinningInfo* const pSkinningInfo)
{
    assert(pCGF);
    assert(pSkinningInfo);

    SaveExportFlags(pCGF);
    SaveTiming(pSkinningInfo);

    for (int i = 0; i < pSkinningInfo->m_pControllers.size(); i++)
    {
        SaveController(pSkinningInfo, i);
    }

    SaveBoneNameList(pSkinningInfo);
}

int CSaverCAF::SaveController(const CInternalSkinningInfo* const pSkinningInfo, int ctrlIndex)
{
    CONTROLLER_CHUNK_DESC_0830 chunk;
    ZeroStruct(chunk);

    CControllerPQLog* pController  = static_cast<CControllerPQLog*>(pSkinningInfo->m_pControllers[ctrlIndex].get());

    assert(pController->m_arrTimes.size() == pController->m_arrKeys.size());

    int numKeys = pController->m_arrTimes.size();
    int nControllerId = pController->m_nControllerId;
    int nFlags = pController->GetFlags();

    CryKeyPQLog* pData = new CryKeyPQLog[numKeys];

    chunk.numKeys = numKeys;
    chunk.nControllerId = nControllerId;
    chunk.nFlags = nFlags;

    for (int i = 0; i < numKeys; i++)
    {
        PQLog& q = pController->m_arrKeys[i];

        pData[i].nTime = pController->m_arrTimes[i];        // what's the measure??  (at load: /TICKS_CONVERT)
        pData[i].vPos = q.vPos;                 // at load: /100.0f (SCALE_TO_CGF ?)
        pData[i].vRotLog = q.vRotLog;
    }

    CChunkData chunkData;
    chunkData.Add(chunk);
    chunkData.AddData(pData, numKeys * sizeof(CryKeyPQLog));
    delete[] pData;

    return m_pChunkFile->AddChunk(
        ChunkType_Controller,
        CONTROLLER_CHUNK_DESC_0830::VERSION,
        eEndianness_Native,
        chunkData.data, chunkData.size);
}

int CSaverCAF::SaveBoneNameList(const CInternalSkinningInfo* const pSkinningInfo)
{
    BONENAMELIST_CHUNK_DESC_0745 hdr;
    ZeroStruct(hdr);

    int numBones = pSkinningInfo->m_arrBoneNameTable.size();
    hdr.numEntities = numBones;
    int dataSize = 0;
    for (int i = 0; i < numBones; i++)
    {
        int len = pSkinningInfo->m_arrBoneNameTable[i].length();
        dataSize += (len + 1);
    }
    dataSize++;         // the last string terminated with double 0

    char* pData = new char[dataSize];
    char* p = pData;

    for (int i = 0; i < numBones; i++)
    {
        strcpy(p, pSkinningInfo->m_arrBoneNameTable[i].c_str());
        int len = pSkinningInfo->m_arrBoneNameTable[i].length();
        p += (len + 1);
    }
    *p = 0;
    p++;

    CChunkData chunkData;
    chunkData.Add(hdr);
    chunkData.AddData(pData, dataSize);
    delete[] pData;

    return m_pChunkFile->AddChunk(
        ChunkType_BoneNameList,
        BONENAMELIST_CHUNK_DESC_0745::VERSION,
        eEndianness_Native,
        chunkData.data, chunkData.size);
}
