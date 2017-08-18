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

#ifndef CRYINCLUDE_CRYANIMATION_LOADERDBA_H
#define CRYINCLUDE_CRYANIMATION_LOADERDBA_H
#pragma once

#include "ControllerOpt.h"


struct CInternalDatabaseInfo
    : public IStreamCallback
    , public IControllerRelocatableChain
{
    uint32 m_FilePathCRC;
    string m_strFilePath;

    bool   m_bLoadFailed;
    string m_strLastError;

    uint32 m_nRelocatableCAFs;
    int m_iTotalControllers;

    CControllerDefragHdl m_hStorage;
    size_t m_nStorageLength;
    CControllerOptNonVirtual* m_pControllersInplace;

    CInternalDatabaseInfo(uint32 filePathCRC, const string& strFilePath);
    ~CInternalDatabaseInfo();

    void GetMemoryUsage(ICrySizer* pSizer) const;

    void* StreamOnNeedStorage(IReadStream* pStream, unsigned nSize, bool& bAbortOnFailToAlloc);
    void StreamAsyncOnComplete(IReadStream* pStream, unsigned nError);
    void StreamOnComplete (IReadStream* pStream, unsigned nError);

    bool LoadChunks(IChunkFile* pChunkFile, bool bStreaming);

    bool ReadControllers(IChunkFile::ChunkDesc* pChunkDesc, bool bStreaming);
    bool ReadController905 (IChunkFile::ChunkDesc* pChunkDesc, bool bStreaming);

    void Relocate(char* pDst, char* pSrc);

    static int32 FindFormat(uint32 num, std::vector<uint32>& vec)
    {
        for (size_t i = 0; i < vec.size(); ++i)
        {
            if (num < vec[i + 1])
            {
                return (int)i;
            }
        }
        return -1;
    }

private:
    CInternalDatabaseInfo(const CInternalDatabaseInfo&);
    CInternalDatabaseInfo& operator = (const CInternalDatabaseInfo&);
};


struct CGlobalHeaderDBA
{
    friend class CAnimationManager;


    CGlobalHeaderDBA();
    ~CGlobalHeaderDBA();

    void GetMemoryUsage(ICrySizer* pSizer) const;
    const char* GetLastError() const { return m_strLastError.c_str(); }

    void CreateDatabaseDBA(const char* filename);
    void LoadDatabaseDBA(const char* sForCharacter);
    bool StartStreamingDBA(bool highPriority);
    void CompleteStreamingDBA(CInternalDatabaseInfo* pInfo);

    void DeleteDatabaseDBA();
    const size_t SizeOf_DBA() const;
    bool InMemory();
    void ReportLoadError(const char* sForCharacter, const char* sReason);

    CInternalDatabaseInfo* m_pDatabaseInfo;
    IReadStreamPtr m_pStream;
    string m_strFilePathDBA;
    string m_strLastError;
    uint32 m_FilePathDBACRC32;
    uint16 m_nUsedAnimations;
    uint16 m_nEmpty;
    uint32 m_nLastUsedTimeDelta;
    uint8  m_bDBALock;
    bool   m_bLoadFailed;
};

// mark CGlobalHeaderDBA as moveable, to prevent problems with missing copy-constructors
// and to not waste performance doing expensive copy-constructor operations
template<>
inline bool raw_movable<CGlobalHeaderDBA>(CGlobalHeaderDBA const& dest)
{
    return true;
}

#endif // CRYINCLUDE_CRYANIMATION_LOADERDBA_H
