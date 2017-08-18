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

#pragma once

#include <ATLEntities.h>
#include <IStreamEngine.h>

// Forward declarations
class CCustomMemoryHeap;
struct IRenderAuxGeom;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Filter for drawing debug info to the screen
    enum EAudioFileCacheManagerDebugFilter
    {
        eAFCMDF_ALL              = 0,
        eAFCMDF_GLOBALS          = BIT(6),// a
        eAFCMDF_LEVEL_SPECIFICS  = BIT(7),// b
        eAFCMDF_USE_COUNTED      = BIT(8),// c
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CFileCacheManager
        : public IStreamCallback
    {
    public:
        explicit CFileCacheManager(TATLPreloadRequestLookup& rPreloadRequests);
        ~CFileCacheManager() override;

        CFileCacheManager(const CFileCacheManager&) = delete;           // Copy protection
        CFileCacheManager& operator=(const CFileCacheManager&) = delete; // Copy protection

        // Public methods
        void Initialize();
        void Release();
        void Update();
        TAudioFileEntryID TryAddFileCacheEntry(const XmlNodeRef pFileNode, const EATLDataScope eDataScope, const bool bAutoLoad);
        bool TryRemoveFileCacheEntry(const TAudioFileEntryID nAudioFileID, const EATLDataScope eDataScope);
        void UpdateLocalizedFileCacheEntries();
        void DrawDebugInfo(IRenderAuxGeom& auxGeom, const float fPosX, const float fPosY);

        EAudioRequestStatus TryLoadRequest(const TAudioPreloadRequestID nPreloadRequestID, const bool bLoadSynchronously, const bool bAutoLoadOnly);
        EAudioRequestStatus TryUnloadRequest(const TAudioPreloadRequestID nPreloadRequestID);
        EAudioRequestStatus UnloadDataByScope(const EATLDataScope eDataScope);

    private:
        // Internal type definitions.
        using TAudioFileEntries = ATLMapLookupType<TAudioFileEntryID, CATLAudioFileEntry*>;

        // IStreamCallback
        void StreamAsyncOnComplete(IReadStream* pStream, unsigned int nError) override;
        void StreamOnComplete(IReadStream* pStream, unsigned int nError) override {}
        // ~IStreamCallback

        // Internal methods
        void AllocateHeap(const size_t nSize, const char* const sUsage);
        bool UncacheFileCacheEntryInternal(CATLAudioFileEntry* const pAudioFileEntry, const bool bNow, const bool bIgnoreUsedCount = false);
        bool DoesRequestFitInternal(const size_t nRequestSize);
        bool FinishStreamInternal(const IReadStreamPtr pStream, const unsigned int nError);
        bool AllocateMemoryBlockInternal(CATLAudioFileEntry* const __restrict pAudioFileEntry);
        void UncacheFile(CATLAudioFileEntry* const pAudioFileEntry);
        void TryToUncacheFiles();
        void UpdateLocalizedFileEntryData(CATLAudioFileEntry* const pAudioFileEntry);
        bool TryCacheFileCacheEntryInternal(CATLAudioFileEntry* const pAudioFileEntry, const TAudioFileEntryID nFileID, const bool bLoadSynchronously, const bool bOverrideUseCount = false, const size_t nUseCount = 0);

        // Internal members
        TATLPreloadRequestLookup& m_rPreloadRequests;
        TAudioFileEntries m_cAudioFileEntries;

        _smart_ptr<CCustomMemoryHeap> m_pMemoryHeap;
        size_t m_nCurrentByteTotal;
        size_t m_nMaxByteTotal;
    };
} // namespace Audio
