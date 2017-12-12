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
#include "FileCacheManager.h"
#include <SoundCVars.h>
#include <CustomMemoryHeap.h>
#include <IAudioSystemImplementation.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CFileCacheManager::CFileCacheManager(TATLPreloadRequestLookup& rPreloadRequests)
        : m_rPreloadRequests(rPreloadRequests)
        , m_nCurrentByteTotal(0)
        , m_nMaxByteTotal(0)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CFileCacheManager::~CFileCacheManager()
    {
        // Empty on purpose
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Initialize()
    {
        AllocateHeap(static_cast<size_t>(g_audioCVars.m_nFileCacheManagerSize), "AudioFileCacheManager");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Release()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Update()
    {
        // Not used for now as we do not queue entries!
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::AllocateHeap(const size_t nSize, const char* const sUsage)
    {
        if (nSize > 0)
        {
            m_pMemoryHeap = static_cast<CCustomMemoryHeap*>(gEnv->pSystem->GetIMemoryManager()->CreateCustomMemoryHeapInstance(IMemoryManager::eapCustomAlignment));

            if (m_pMemoryHeap.get())
            {
                m_nMaxByteTotal = nSize << 10;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioFileEntryID CFileCacheManager::TryAddFileCacheEntry(const XmlNodeRef pFileNode, const EATLDataScope eDataScope, const bool bAutoLoad)
    {
        TAudioFileEntryID nID = INVALID_AUDIO_FILE_ENTRY_ID;

        SATLAudioFileEntryInfo oFileEntryInfo;

        EAudioRequestStatus eResult = eARS_FAILURE;
        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::ParseAudioFileEntry, pFileNode, &oFileEntryInfo);
        if (eResult == eARS_SUCCESS)
        {
            const char* fileLocation = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(fileLocation, &AudioSystemImplementationRequestBus::Events::GetAudioFileLocation, &oFileEntryInfo);
            CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFullPath(fileLocation);
            sFullPath += oFileEntryInfo.sFileName;
            auto pNewAudioFileEntry = azcreate(CATLAudioFileEntry, (sFullPath, oFileEntryInfo.pImplData), Audio::AudioSystemAllocator, "ATLAudioFileEntry");

            if (pNewAudioFileEntry)
            {
                pNewAudioFileEntry->m_nMemoryBlockAlignment = oFileEntryInfo.nMemoryBlockAlignment;

                if (oFileEntryInfo.bLocalized)
                {
                    pNewAudioFileEntry->m_nFlags |= eAFF_LOCALIZED;
                }

                nID = AudioStringToID<TAudioFileEntryID>(pNewAudioFileEntry->m_sPath.c_str());
                CATLAudioFileEntry* const __restrict pExisitingAudioFileEntry = stl::find_in_map(m_cAudioFileEntries, nID, nullptr);

                if (!pExisitingAudioFileEntry)
                {
                    if (!bAutoLoad)
                    {
                        // Can now be ref-counted and therefore manually unloaded.
                        pNewAudioFileEntry->m_nFlags |= eAFF_USE_COUNTED;
                    }

                    pNewAudioFileEntry->m_eDataScope = eDataScope;
                    pNewAudioFileEntry->m_sPath.MakeLower();
                    const size_t nFileSize = gEnv->pCryPak->FGetSize(pNewAudioFileEntry->m_sPath.c_str());

                    if (nFileSize > 0)
                    {
                        pNewAudioFileEntry->m_nSize = nFileSize;
                        pNewAudioFileEntry->m_nFlags = (pNewAudioFileEntry->m_nFlags | eAFF_NOTCACHED) & ~eAFF_NOTFOUND;
                        pNewAudioFileEntry->m_eStreamTaskType = eStreamTaskTypeSound;
                    }

                    m_cAudioFileEntries[nID] = pNewAudioFileEntry;
                }
                else
                {
                    if ((pExisitingAudioFileEntry->m_nFlags & eAFF_USE_COUNTED) != 0 && bAutoLoad)
                    {
                        // This file entry is upgraded from "manual loading" to "auto loading" but needs a reset to "manual loading" again!
                        pExisitingAudioFileEntry->m_nFlags = (pExisitingAudioFileEntry->m_nFlags | eAFF_NEEDS_RESET_TO_MANUAL_LOADING) & ~eAFF_USE_COUNTED;
                        g_audioLogger.Log(eALT_ALWAYS, "Upgraded file entry from \"manual loading\" to \"auto loading\": %s", pExisitingAudioFileEntry->m_sPath.c_str());
                    }

                    // Entry already exists, free the memory!
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioFileEntryData, pNewAudioFileEntry->m_pImplData);
                    azdestroy(pNewAudioFileEntry, Audio::AudioSystemAllocator);
                }
            }
        }

        return nID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::TryRemoveFileCacheEntry(const TAudioFileEntryID nAudioFileID, const EATLDataScope eDataScope)
    {
        bool bSuccess = false;
        const TAudioFileEntries::iterator Iter(m_cAudioFileEntries.find(nAudioFileID));

        if (Iter != m_cAudioFileEntries.end())
        {
            CATLAudioFileEntry* const pAudioFileEntry = Iter->second;

            if (pAudioFileEntry->m_eDataScope == eDataScope)
            {
                UncacheFileCacheEntryInternal(pAudioFileEntry, true, true);
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioFileEntryData, pAudioFileEntry->m_pImplData);
                azdestroy(pAudioFileEntry, Audio::AudioSystemAllocator);
                m_cAudioFileEntries.erase(Iter);
            }
            else if ((eDataScope == eADS_LEVEL_SPECIFIC) && ((pAudioFileEntry->m_nFlags & eAFF_NEEDS_RESET_TO_MANUAL_LOADING) != 0))
            {
                pAudioFileEntry->m_nFlags = (pAudioFileEntry->m_nFlags | eAFF_USE_COUNTED) & ~eAFF_NEEDS_RESET_TO_MANUAL_LOADING;
                g_audioLogger.Log(eALT_ALWAYS, "Downgraded file entry from \"auto loading\" to \"manual loading\": %s", pAudioFileEntry->m_sPath.c_str());
            }
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UpdateLocalizedFileCacheEntries()
    {
        for (auto& audioFileEntryPair : m_cAudioFileEntries)
        {
            CATLAudioFileEntry* const pAudioFileEntry = audioFileEntryPair.second;

            if (pAudioFileEntry && (pAudioFileEntry->m_nFlags & eAFF_LOCALIZED) != 0)
            {
                if ((pAudioFileEntry->m_nFlags & (eAFF_CACHED | eAFF_LOADING)) != 0)
                {
                    // The file needs to be unloaded first.
                    const size_t nUseCount = pAudioFileEntry->m_nUseCount;
                    pAudioFileEntry->m_nUseCount = 0;// Needed to uncache without an error.
                    UncacheFile(pAudioFileEntry);

                    UpdateLocalizedFileEntryData(pAudioFileEntry);

                    TryCacheFileCacheEntryInternal(pAudioFileEntry, audioFileEntryPair.first, true, true, nUseCount);
                }
                else
                {
                    // The file is not cached or loading, it is safe to update the corresponding CATLAudioFileEntry data.
                    UpdateLocalizedFileEntryData(pAudioFileEntry);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::TryLoadRequest(const TAudioPreloadRequestID nPreloadRequestID, const bool bLoadSynchronously, const bool bAutoLoadOnly)
    {
        bool bFullSuccess = false;
        bool bFullFailure = true;
        CATLPreloadRequest* const pPreloadRequest = stl::find_in_map(m_rPreloadRequests, nPreloadRequestID, nullptr);

        if (pPreloadRequest && !pPreloadRequest->m_cFileEntryIDs.empty() && (!bAutoLoadOnly || (bAutoLoadOnly && pPreloadRequest->m_bAutoLoad)))
        {
            bFullSuccess = true;
            for (auto fileId : pPreloadRequest->m_cFileEntryIDs)
            {
                CATLAudioFileEntry* const pAudioFileEntry = stl::find_in_map(m_cAudioFileEntries, fileId, nullptr);

                if (pAudioFileEntry)
                {
                    const bool bTemp = TryCacheFileCacheEntryInternal(pAudioFileEntry, fileId, bLoadSynchronously);
                    bFullSuccess = bFullSuccess && bTemp;
                    bFullFailure = bFullFailure && !bTemp;
                }
            }
        }

        return bFullSuccess ? eARS_SUCCESS : (bFullFailure ? eARS_FAILURE : eARS_PARTIAL_SUCCESS);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::TryUnloadRequest(const TAudioPreloadRequestID nPreloadRequestID)
    {
        bool bFullSuccess = false;
        bool bFullFailure = true;
        CATLPreloadRequest* const pPreloadRequest = stl::find_in_map(m_rPreloadRequests, nPreloadRequestID, nullptr);

        if (pPreloadRequest && !pPreloadRequest->m_cFileEntryIDs.empty())
        {
            bFullSuccess = true;
            for (auto fileId : pPreloadRequest->m_cFileEntryIDs)
            {
                CATLAudioFileEntry* const pAudioFileEntry = stl::find_in_map(m_cAudioFileEntries, fileId, nullptr);

                if (pAudioFileEntry)
                {
                    const bool bTemp = UncacheFileCacheEntryInternal(pAudioFileEntry, true);
                    bFullSuccess = bFullSuccess && bTemp;
                    bFullFailure = bFullFailure && !bTemp;
                }
            }
        }

        return bFullSuccess ? eARS_SUCCESS : (bFullFailure ? eARS_FAILURE : eARS_PARTIAL_SUCCESS);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::UnloadDataByScope(const EATLDataScope eDataScope)
    {
        for (auto it = m_cAudioFileEntries.begin(); it != m_cAudioFileEntries.end(); )
        {
            CATLAudioFileEntry* const pAudioFileEntry = it->second;

            if (pAudioFileEntry && pAudioFileEntry->m_eDataScope == eDataScope)
            {
                if (UncacheFileCacheEntryInternal(pAudioFileEntry, true, true))
                {
                    it = m_cAudioFileEntries.erase(it);
                    continue;
                }
            }

            ++it;
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::UncacheFileCacheEntryInternal(CATLAudioFileEntry* const pAudioFileEntry, const bool bNow, const bool bIgnoreUsedCount /* = false */)
    {
        bool bSuccess = false;

        // In any case decrement the used count.
        if (pAudioFileEntry->m_nUseCount > 0)
        {
            --pAudioFileEntry->m_nUseCount;
        }

        if (pAudioFileEntry->m_nUseCount < 1 || bIgnoreUsedCount)
        {
            // Must be cached to proceed.
            if ((pAudioFileEntry->m_nFlags & eAFF_CACHED) != 0)
            {
                // Only "use-counted" files can become removable!
                if ((pAudioFileEntry->m_nFlags & eAFF_USE_COUNTED) != 0)
                {
                    pAudioFileEntry->m_nFlags |= eAFF_REMOVABLE;
                }

                if (bNow || bIgnoreUsedCount)
                {
                    UncacheFile(pAudioFileEntry);
                }
            }
            else if ((pAudioFileEntry->m_nFlags & (eAFF_LOADING | eAFF_MEMALLOCFAIL)) != 0)
            {
                g_audioLogger.Log(eALT_ALWAYS, "Trying to remove a loading or mem-failed file cache entry %s", pAudioFileEntry->m_sPath.c_str());

                // Reset the entry in case it's still loading or was a memory allocation fail.
                UncacheFile(pAudioFileEntry);
            }

            // The file was either properly uncached, queued for uncache or not cached at all.
            bSuccess = true;
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::StreamAsyncOnComplete(IReadStream* pStream, unsigned int nError)
    {
        // We "user abort" quite frequently so this is not something we want to assert on.
        AZ_Assert(nError == 0 || nError == ERROR_USER_ABORT, "FileCacheManager StreamAsyncOnComplete - received error %d!", nError);

        FinishStreamInternal(pStream, nError);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, const float fPosX, const float fPosY)
    {
    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_FILECACHE_MANAGER_INFO) != 0)
        {
            EATLDataScope eDataScope = eADS_ALL;

            if ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_ALL) == 0)
            {
                if ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_GLOBALS) != 0)
                {
                    eDataScope = eADS_GLOBAL;
                }
                else if ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_LEVEL_SPECIFICS) != 0)
                {
                    eDataScope = eADS_LEVEL_SPECIFIC;
                }
            }

            const CTimeValue tFrameTime = gEnv->pTimer->GetAsyncTime();

            CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> sTemp;
            const float fEntryDrawSize = 1.5f;
            const float fEntryStepSize = 12.0f;
            float fPositionY = fPosY + 20.0f;
            float fPositionX = fPosX + 20.0f;
            float fTime = 0.0f;
            float fRatio = 0.0f;
            float fOriginalAlpha = 0.7f;
            float* pfColor = nullptr;

            // The colors.
            float fWhite[4] = { 1.0f, 1.0f, 1.0f, fOriginalAlpha };
            float fCyan[4] = { 0.0f, 1.0f, 1.0f, fOriginalAlpha };
            float fOrange[4] = { 1.0f, 0.5f, 0.0f, fOriginalAlpha };
            float fGreen[4] = { 0.0f, 1.0f, 0.0f, fOriginalAlpha };
            float fRed[4] = { 1.0f, 0.0f, 0.0f, fOriginalAlpha };
            float fRedish[4] = { 0.7f, 0.0f, 0.0f, fOriginalAlpha };
            float fBlue[4] = { 0.1f, 0.2f, 0.8f, fOriginalAlpha };
            float fYellow[4] = { 1.0f, 1.0f, 0.0f, fOriginalAlpha };
            float fDarkish[4] = { 0.3f, 0.3f, 0.3f, fOriginalAlpha };

            auxGeom.Draw2dLabel(fPosX, fPositionY, 1.6f, fOrange, false, "FileCacheManager (%d of %d KiB) [Entries: %d]", static_cast<int>(m_nCurrentByteTotal >> 10), static_cast<int>(m_nMaxByteTotal >> 10), static_cast<int>(m_cAudioFileEntries.size()));
            fPositionY += 15.0f;

            for (auto& audioFileEntryPair : m_cAudioFileEntries)
            {
                CATLAudioFileEntry* const pAudioFileEntry = audioFileEntryPair.second;

                if (pAudioFileEntry->m_eDataScope == eADS_GLOBAL &&
                    ((g_audioCVars.m_nFileCacheManagerDebugFilter == eAFCMDF_ALL) ||
                     (g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_GLOBALS) != 0 ||
                     ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_USE_COUNTED) != 0 &&
                      (pAudioFileEntry->m_nFlags & eAFF_USE_COUNTED) != 0)))
                {
                    if ((pAudioFileEntry->m_nFlags & eAFF_LOADING) != 0)
                    {
                        pfColor = fRed;
                    }
                    else if ((pAudioFileEntry->m_nFlags & eAFF_MEMALLOCFAIL) != 0)
                    {
                        pfColor = fBlue;
                    }
                    else if ((pAudioFileEntry->m_nFlags & eAFF_REMOVABLE) != 0)
                    {
                        pfColor = fGreen;
                    }
                    else if ((pAudioFileEntry->m_nFlags & eAFF_NOTCACHED) != 0)
                    {
                        pfColor = fWhite;
                    }
                    else if ((pAudioFileEntry->m_nFlags & eAFF_NOTFOUND) != 0)
                    {
                        pfColor = fRedish;
                    }
                    else
                    {
                        pfColor = fCyan;
                    }

                    fTime = (tFrameTime - pAudioFileEntry->m_oTimeCached).GetSeconds();
                    fRatio = fTime / 5.0f;
                    fOriginalAlpha = pfColor[3];
                    pfColor[3] *= clamp_tpl(fRatio, 0.2f, 1.0f);

                    sTemp.clear();

                    if ((pAudioFileEntry->m_nFlags & eAFF_USE_COUNTED) != 0)
                    {
                        if (pAudioFileEntry->m_nSize < 1024)
                        {
                            sTemp.Format(sTemp + "%s (%" PRISIZE_T " Bytes) [%" PRISIZE_T "]", pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nSize, pAudioFileEntry->m_nUseCount);
                        }
                        else
                        {
                            sTemp.Format(sTemp + "%s (%" PRISIZE_T " KiB) [%" PRISIZE_T "]", pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nSize >> 10, pAudioFileEntry->m_nUseCount);
                        }
                    }
                    else
                    {
                        if (pAudioFileEntry->m_nSize < 1024)
                        {
                            sTemp.Format(sTemp + "%s (%" PRISIZE_T " Bytes)", pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nSize);
                        }
                        else
                        {
                            sTemp.Format(sTemp + "%s (%" PRISIZE_T " KiB)", pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nSize >> 10);
                        }
                    }

                    auxGeom.Draw2dLabel(fPositionX, fPositionY, fEntryDrawSize, pfColor, false, sTemp.c_str());
                    pfColor[3] = fOriginalAlpha;
                    fPositionY += fEntryStepSize;
                }
            }

            for (auto& audioFileEntryPair : m_cAudioFileEntries)
            {
                CATLAudioFileEntry* const pAudioFileEntry = audioFileEntryPair.second;

                if (pAudioFileEntry->m_eDataScope == eADS_LEVEL_SPECIFIC &&
                    ((g_audioCVars.m_nFileCacheManagerDebugFilter == eAFCMDF_ALL) ||
                     (g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_LEVEL_SPECIFICS) != 0 ||
                     ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_USE_COUNTED) != 0 &&
                      (pAudioFileEntry->m_nFlags & eAFF_USE_COUNTED) != 0)))
                {
                    if ((pAudioFileEntry->m_nFlags & eAFF_LOADING) != 0)
                    {
                        pfColor = fRed;
                    }
                    else if ((pAudioFileEntry->m_nFlags & eAFF_MEMALLOCFAIL) != 0)
                    {
                        pfColor = fBlue;
                    }
                    else if ((pAudioFileEntry->m_nFlags & eAFF_REMOVABLE) != 0)
                    {
                        pfColor = fGreen;
                    }
                    else if ((pAudioFileEntry->m_nFlags & eAFF_NOTCACHED) != 0)
                    {
                        pfColor = fWhite;
                    }
                    else if ((pAudioFileEntry->m_nFlags & eAFF_NOTFOUND) != 0)
                    {
                        pfColor = fRedish;
                    }
                    else
                    {
                        pfColor = fYellow;
                    }

                    fTime = (tFrameTime - pAudioFileEntry->m_oTimeCached).GetSeconds();
                    fRatio = fTime / 5.0f;
                    fOriginalAlpha = pfColor[3];
                    pfColor[3] *= clamp_tpl(fRatio, 0.2f, 1.0f);

                    sTemp.clear();

                    if ((pAudioFileEntry->m_nFlags & eAFF_USE_COUNTED) != 0)
                    {
                        if (pAudioFileEntry->m_nSize < 1024)
                        {
                            sTemp.Format(sTemp + "%s (%" PRISIZE_T " Bytes) [%" PRISIZE_T "]", pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nSize, pAudioFileEntry->m_nUseCount);
                        }
                        else
                        {
                            sTemp.Format(sTemp + "%s (%" PRISIZE_T " KiB) [%" PRISIZE_T "]", pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nSize >> 10, pAudioFileEntry->m_nUseCount);
                        }
                    }
                    else
                    {
                        if (pAudioFileEntry->m_nSize < 1024)
                        {
                            sTemp.Format(sTemp + "%s (%" PRISIZE_T " Bytes)", pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nSize);
                        }
                        else
                        {
                            sTemp.Format(sTemp + "%s (%" PRISIZE_T " KiB)", pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nSize >> 10);
                        }
                    }

                    auxGeom.Draw2dLabel(fPositionX, fPositionY, fEntryDrawSize, pfColor, false, sTemp.c_str());
                    pfColor[3] = fOriginalAlpha;
                    fPositionY += fEntryStepSize;
                }
            }
        }
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::DoesRequestFitInternal(const size_t nRequestSize)
    {
        // Make sure these unsigned values don't flip around.
        AZ_Assert(m_nCurrentByteTotal <= m_nMaxByteTotal, "FileCacheManager DoesRequestFitInternal - Unsigned wraparound detected!");
        bool bSuccess = false;

        if (nRequestSize <= (m_nMaxByteTotal - m_nCurrentByteTotal))
        {
            // Here the requested size is available without the need of first cleaning up.
            bSuccess = true;
        }
        else
        {
            // Determine how much memory would get freed if all eAFF_REMOVABLE files get thrown out.
            // We however skip files that are already queued for unload. The request will get queued up in that case.
            size_t nPossibleMemoryGain = 0;

            // Check the single file entries for removability.
            for (auto& audioFileEntryPair : m_cAudioFileEntries)
            {
                CATLAudioFileEntry* const pAudioFileEntry = audioFileEntryPair.second;

                if (pAudioFileEntry
                    && (pAudioFileEntry->m_nFlags & eAFF_CACHED) != 0
                    && (pAudioFileEntry->m_nFlags & eAFF_REMOVABLE) != 0)
                {
                    nPossibleMemoryGain += pAudioFileEntry->m_nSize;
                }
            }

            const size_t nMaxAvailableSize = (m_nMaxByteTotal - (m_nCurrentByteTotal - nPossibleMemoryGain));

            if (nRequestSize <= nMaxAvailableSize)
            {
                // Here we need to cleanup first before allowing the new request to be allocated.
                TryToUncacheFiles();

                // We should only indicate success if there's actually really enough room for the new entry!
                bSuccess = (m_nMaxByteTotal - m_nCurrentByteTotal) >= nRequestSize;
            }
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::FinishStreamInternal(const IReadStreamPtr pStream, const unsigned int nError)
    {
        bool bSuccess = false;

        const TAudioFileEntryID nFileID = static_cast<TAudioFileEntryID>(pStream->GetUserData());
        CATLAudioFileEntry* const pAudioFileEntry = stl::find_in_map(m_cAudioFileEntries, nFileID, nullptr);
        AZ_Assert(pAudioFileEntry, "FileCacheManager FinishStreamInternal - Failed to find the file entry for fileID %d!", (uint32)nFileID);

        // Must be loading in to proceed.
        if (pAudioFileEntry && (pAudioFileEntry->m_nFlags & eAFF_LOADING) != 0)
        {
            if (nError == 0)
            {
                pAudioFileEntry->m_pReadStream = nullptr;
                pAudioFileEntry->m_nFlags = (pAudioFileEntry->m_nFlags | eAFF_CACHED) & ~(eAFF_LOADING | eAFF_NOTCACHED);

            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                pAudioFileEntry->m_oTimeCached = gEnv->pTimer->GetAsyncTime();
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                SATLAudioFileEntryInfo sFileEntryInfo;
                sFileEntryInfo.nMemoryBlockAlignment = pAudioFileEntry->m_nMemoryBlockAlignment;
                sFileEntryInfo.pFileData = pAudioFileEntry->m_pMemoryBlock->GetData();
                sFileEntryInfo.nSize = pAudioFileEntry->m_nSize;
                sFileEntryInfo.pImplData = pAudioFileEntry->m_pImplData;
                sFileEntryInfo.sFileName = PathUtil::GetFile(pAudioFileEntry->m_sPath.c_str());

                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::RegisterInMemoryFile, &sFileEntryInfo);
                bSuccess = true;
            }
            else if (nError == ERROR_USER_ABORT)
            {
                // We abort this stream only during entry Uncache().
                // Therefore there's no need to call Uncache() during stream abort with error code ERROR_USER_ABORT.
                g_audioLogger.Log(eALT_ALWAYS, "AFCM: user aborted stream for file %s (error: %u)", pAudioFileEntry->m_sPath.c_str(), nError);
            }
            else
            {
                UncacheFileCacheEntryInternal(pAudioFileEntry, true, true);
                g_audioLogger.Log(eALT_ERROR, "AFCM: failed to stream in file %s (error: %u)", pAudioFileEntry->m_sPath.c_str(), nError);
            }
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::AllocateMemoryBlockInternal(CATLAudioFileEntry* const __restrict pAudioFileEntry)
    {
        // Must not have valid memory yet.
        AZ_Assert(!pAudioFileEntry->m_pMemoryBlock.get(), "FileCacheManager AllocateMemoryBlockInternal - Memory appears to be set already!");

        if (m_pMemoryHeap.get())
        {
            pAudioFileEntry->m_pMemoryBlock = m_pMemoryHeap->AllocateBlock(pAudioFileEntry->m_nSize, pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nMemoryBlockAlignment);
        }

        if (!pAudioFileEntry->m_pMemoryBlock.get())
        {
            // Memory block is either full or too fragmented, let's try to throw everything out that can be removed and allocate again.
            TryToUncacheFiles();

            // And try again!
            if (m_pMemoryHeap.get())
            {
                pAudioFileEntry->m_pMemoryBlock = m_pMemoryHeap->AllocateBlock(pAudioFileEntry->m_nSize, pAudioFileEntry->m_sPath.c_str(), pAudioFileEntry->m_nMemoryBlockAlignment);
            }
        }

        return pAudioFileEntry->m_pMemoryBlock.get() != nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UncacheFile(CATLAudioFileEntry* const pAudioFileEntry)
    {
        m_nCurrentByteTotal -= pAudioFileEntry->m_nSize;

        if (pAudioFileEntry->m_pReadStream)
        {
            pAudioFileEntry->m_pReadStream->Abort();
            pAudioFileEntry->m_pReadStream = nullptr;
        }

        if (pAudioFileEntry->m_pMemoryBlock.get() && pAudioFileEntry->m_pMemoryBlock->GetData())
        {
            SATLAudioFileEntryInfo sFileEntryInfo;
            sFileEntryInfo.nMemoryBlockAlignment = pAudioFileEntry->m_nMemoryBlockAlignment;
            sFileEntryInfo.pFileData = pAudioFileEntry->m_pMemoryBlock->GetData();
            sFileEntryInfo.nSize = pAudioFileEntry->m_nSize;
            sFileEntryInfo.pImplData = pAudioFileEntry->m_pImplData;
            sFileEntryInfo.sFileName = PathUtil::GetFile(pAudioFileEntry->m_sPath.c_str());

            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::UnregisterInMemoryFile, &sFileEntryInfo);
        }

        pAudioFileEntry->m_pMemoryBlock = nullptr;
        pAudioFileEntry->m_nFlags = (pAudioFileEntry->m_nFlags | eAFF_NOTCACHED) & ~(eAFF_CACHED | eAFF_REMOVABLE);
        AZ_Assert(pAudioFileEntry->m_nUseCount == 0, "FileCacheManager UncacheFile - Expected use count of file to be zero!");
        pAudioFileEntry->m_nUseCount = 0;

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        pAudioFileEntry->m_oTimeCached.SetValue(0);
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::TryToUncacheFiles()
    {
        for (auto& audioFileEntryPair : m_cAudioFileEntries)
        {
            CATLAudioFileEntry* const pAudioFileEntry = audioFileEntryPair.second;

            if (pAudioFileEntry
                && (pAudioFileEntry->m_nFlags & eAFF_CACHED) != 0
                && (pAudioFileEntry->m_nFlags & eAFF_REMOVABLE) != 0)
            {
                UncacheFileCacheEntryInternal(pAudioFileEntry, true);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UpdateLocalizedFileEntryData(CATLAudioFileEntry* const pAudioFileEntry)
    {
        static SATLAudioFileEntryInfo oFileEntryInfo;
        oFileEntryInfo.bLocalized = true;
        oFileEntryInfo.nSize = 0;
        oFileEntryInfo.pFileData = nullptr;
        oFileEntryInfo.nMemoryBlockAlignment = 0;

        CryFixedStringT<MAX_AUDIO_FILE_NAME_LENGTH> sFileName(PathUtil::GetFile(pAudioFileEntry->m_sPath.c_str()));
        oFileEntryInfo.pImplData = pAudioFileEntry->m_pImplData;
        oFileEntryInfo.sFileName = sFileName.c_str();

        const char* fileLocation = nullptr;
        AudioSystemImplementationRequestBus::BroadcastResult(fileLocation, &AudioSystemImplementationRequestBus::Events::GetAudioFileLocation, &oFileEntryInfo);
        pAudioFileEntry->m_sPath = fileLocation;
        pAudioFileEntry->m_sPath += sFileName.c_str();
        pAudioFileEntry->m_sPath.MakeLower();

        pAudioFileEntry->m_nSize = gEnv->pCryPak->FGetSize(pAudioFileEntry->m_sPath.c_str());

        AZ_Assert(pAudioFileEntry->m_nSize > 0, "FileCacheManager UpdateLocalizedFileEntryData - Expected file size to be greater than zero!");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::TryCacheFileCacheEntryInternal(
        CATLAudioFileEntry* const pAudioFileEntry,
        const TAudioFileEntryID nFileID,
        const bool bLoadSynchronously,
        const bool bOverrideUseCount /* = false */,
        const size_t nUseCount /* = 0 */)
    {
        bool bSuccess = false;

        if (!pAudioFileEntry->m_sPath.empty()
            && (pAudioFileEntry->m_nFlags & eAFF_NOTCACHED) != 0
            && (pAudioFileEntry->m_nFlags & (eAFF_CACHED | eAFF_LOADING)) == 0)
        {
            if (DoesRequestFitInternal(pAudioFileEntry->m_nSize) && AllocateMemoryBlockInternal(pAudioFileEntry))
            {
                StreamReadParams oStreamReadParams;
                oStreamReadParams.nOffset = 0;
                oStreamReadParams.nFlags = IStreamEngine::FLAGS_NO_SYNC_CALLBACK;
                oStreamReadParams.dwUserData = static_cast<DWORD_PTR>(nFileID);
                oStreamReadParams.nLoadTime = 0;
                oStreamReadParams.nMaxLoadTime = 0;
                oStreamReadParams.ePriority = estpUrgent;
                oStreamReadParams.pBuffer = pAudioFileEntry->m_pMemoryBlock->GetData();
                oStreamReadParams.nSize = static_cast<int unsigned>(pAudioFileEntry->m_nSize);

                pAudioFileEntry->m_nFlags |= eAFF_LOADING;
                pAudioFileEntry->m_pReadStream = gEnv->pSystem->GetStreamEngine()->StartRead(eStreamTaskTypeFSBCache, pAudioFileEntry->m_sPath.c_str(), this, &oStreamReadParams);

                if (bLoadSynchronously)
                {
                    pAudioFileEntry->m_pReadStream->Wait();
                }

                // Always add to the total size.
                m_nCurrentByteTotal += pAudioFileEntry->m_nSize;
                bSuccess = true;
            }
            else
            {
                // Cannot have a valid memory block!
                AZ_Assert(!pAudioFileEntry->m_pMemoryBlock.get() || !pAudioFileEntry->m_pMemoryBlock->GetData(),
                    "FileCacheManager TryCacheFileCacheEntryInternal - Cannot have a valid memory block!");

                // This unfortunately is a total memory allocation fail.
                pAudioFileEntry->m_nFlags |= eAFF_MEMALLOCFAIL;

                // The user should be made aware of it.
                g_audioLogger.Log(eALT_ERROR, "AFCM: could not cache \"%s\" as we are out of memory!", pAudioFileEntry->m_sPath.c_str());
            }
        }
        else if ((pAudioFileEntry->m_nFlags & (eAFF_CACHED | eAFF_LOADING)) != 0)
        {
            // The user should be made aware of it.
            g_audioLogger.Log(eALT_WARNING, "AFCM: could not cache \"%s\" as it is either already loaded or currently loading!", pAudioFileEntry->m_sPath.c_str());
            bSuccess = true;
        }
        else if ((pAudioFileEntry->m_nFlags & eAFF_NOTFOUND) != 0)
        {
            // The user should be made aware of it.
            g_audioLogger.Log(eALT_ERROR, "AFCM: could not cache \"%s\" as it was not found at the target location!", pAudioFileEntry->m_sPath.c_str());
        }

        // Increment the used count on GameHints.
        if ((pAudioFileEntry->m_nFlags & eAFF_USE_COUNTED) != 0 && (pAudioFileEntry->m_nFlags & (eAFF_CACHED | eAFF_LOADING)) != 0)
        {
            if (bOverrideUseCount)
            {
                pAudioFileEntry->m_nUseCount = nUseCount;
            }
            else
            {
                ++pAudioFileEntry->m_nUseCount;
            }

            // Make sure to handle the eAFCS_REMOVABLE flag according to the m_nUsedCount count.
            if (pAudioFileEntry->m_nUseCount != 0)
            {
                pAudioFileEntry->m_nFlags &= ~eAFF_REMOVABLE;
            }
            else
            {
                pAudioFileEntry->m_nFlags |= eAFF_REMOVABLE;
            }
        }

        return bSuccess;
    }
} // namespace Audio
