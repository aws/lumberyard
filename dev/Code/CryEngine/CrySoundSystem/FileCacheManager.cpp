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

#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/StreamerUtil.h>

#include <IAudioSystemImplementation.h>
#include <IRenderAuxGeom.h>
#include <SoundCVars.h>
#include <CustomMemoryHeap.h>

#include <CrySoundSystem_Traits_Platform.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    CFileCacheManager::CFileCacheManager(TATLPreloadRequestLookup& preloadRequests)
        : m_preloadRequests(preloadRequests)
        , m_currentByteTotal(0)
        , m_maxByteTotal(0)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    CFileCacheManager::~CFileCacheManager()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Initialize()
    {
        AllocateHeap(static_cast<size_t>(g_audioCVars.m_nFileCacheManagerSize), "AudioFileCacheManager");

        AudioFileCacheManagerNotficationBus::Handler::BusConnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Release()
    {
        AudioFileCacheManagerNotficationBus::Handler::BusDisconnect();

        // Should we check here for any lingering files?
        // ATL unloads everything before getting here, but a stop-gap could be safer.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::Update()
    {
        AudioFileCacheManagerNotficationBus::ExecuteQueuedEvents();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::AllocateHeap(const size_t size, const char* const usage)
    {
        if (size > 0)
        {
            m_memoryHeap.reset(static_cast<CCustomMemoryHeap*>(gEnv->pSystem->GetIMemoryManager()->CreateCustomMemoryHeapInstance(AZ_TRAIT_CRYSOUNDSYSTEM_FILE_CACHE_MANAGER_ALLOCATION_POLICY)));

            if (m_memoryHeap.get())
            {
                m_maxByteTotal = size << 10;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    TAudioFileEntryID CFileCacheManager::TryAddFileCacheEntry(const XmlNodeRef fileXmlNode, const EATLDataScope dataScope, const bool autoLoad)
    {
        TAudioFileEntryID fileEntryId = INVALID_AUDIO_FILE_ENTRY_ID;
        SATLAudioFileEntryInfo fileEntryInfo;

        EAudioRequestStatus result = eARS_FAILURE;
        AudioSystemImplementationRequestBus::BroadcastResult(result, &AudioSystemImplementationRequestBus::Events::ParseAudioFileEntry, fileXmlNode, &fileEntryInfo);
        if (result == eARS_SUCCESS)
        {
            const char* fileLocation = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(fileLocation, &AudioSystemImplementationRequestBus::Events::GetAudioFileLocation, &fileEntryInfo);
            CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> fullPath(fileLocation);
            fullPath += fileEntryInfo.sFileName;
            auto newAudioFileEntry = azcreate(CATLAudioFileEntry, (fullPath, fileEntryInfo.pImplData), Audio::AudioSystemAllocator, "ATLAudioFileEntry");

            if (newAudioFileEntry)
            {
                newAudioFileEntry->m_memoryBlockAlignment = fileEntryInfo.nMemoryBlockAlignment;

                if (fileEntryInfo.bLocalized)
                {
                    newAudioFileEntry->m_flags.AddFlags(eAFF_LOCALIZED);
                }

                fileEntryId = AudioStringToID<TAudioFileEntryID>(newAudioFileEntry->m_filePath.c_str());
                CATLAudioFileEntry* const __restrict exisitingAudioFileEntry = stl::find_in_map(m_audioFileEntries, fileEntryId, nullptr);

                if (!exisitingAudioFileEntry)
                {
                    if (!autoLoad)
                    {
                        // Can now be ref-counted and therefore manually unloaded.
                        newAudioFileEntry->m_flags.AddFlags(eAFF_USE_COUNTED);
                    }

                    newAudioFileEntry->m_dataScope = dataScope;
                    newAudioFileEntry->m_filePath.MakeLower();
                    const size_t fileSize = gEnv->pCryPak->FGetSize(newAudioFileEntry->m_filePath.c_str());

                    if (fileSize > 0)
                    {
                        newAudioFileEntry->m_fileSize = fileSize;
                        newAudioFileEntry->m_flags.ClearFlags(eAFF_NOTFOUND);
                    }

                    m_audioFileEntries[fileEntryId] = newAudioFileEntry;
                }
                else
                {
                    if (autoLoad && exisitingAudioFileEntry->m_flags.AreAnyFlagsActive(eAFF_USE_COUNTED))
                    {
                        // This file entry is upgraded from "manual loading" to "auto loading" but needs a reset to "manual loading" again!
                        exisitingAudioFileEntry->m_flags.AddFlags(eAFF_NEEDS_RESET_TO_MANUAL_LOADING);
                        exisitingAudioFileEntry->m_flags.ClearFlags(eAFF_USE_COUNTED);
                        g_audioLogger.Log(eALT_COMMENT, "Upgraded file entry from 'Manual' to 'Auto' loading: %s", exisitingAudioFileEntry->m_filePath.c_str());
                    }

                    // Entry already exists, free the memory!
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioFileEntryData, newAudioFileEntry->m_implData);
                    azdestroy(newAudioFileEntry, Audio::AudioSystemAllocator);
                }
            }
        }

        return fileEntryId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::TryRemoveFileCacheEntry(const TAudioFileEntryID audioFileID, const EATLDataScope dataScope)
    {
        bool success = false;
        const TAudioFileEntries::iterator iter(m_audioFileEntries.find(audioFileID));

        if (iter != m_audioFileEntries.end())
        {
            CATLAudioFileEntry* const audioFileEntry = iter->second;

            if (audioFileEntry->m_dataScope == dataScope)
            {
                UncacheFileCacheEntryInternal(audioFileEntry, true, true);
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioFileEntryData, audioFileEntry->m_implData);
                azdestroy(audioFileEntry, Audio::AudioSystemAllocator);
                m_audioFileEntries.erase(iter);
            }
            else if (dataScope == eADS_LEVEL_SPECIFIC && audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_NEEDS_RESET_TO_MANUAL_LOADING))
            {
                audioFileEntry->m_flags.AddFlags(eAFF_USE_COUNTED);
                audioFileEntry->m_flags.ClearFlags(eAFF_NEEDS_RESET_TO_MANUAL_LOADING);
                g_audioLogger.Log(eALT_COMMENT, "Downgraded file entry from 'Auto' to 'Manual' loading: %s", audioFileEntry->m_filePath.c_str());
            }
        }

        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UpdateLocalizedFileCacheEntries()
    {
        for (auto& audioFileEntryPair : m_audioFileEntries)
        {
            CATLAudioFileEntry* const audioFileEntry = audioFileEntryPair.second;

            if (audioFileEntry && audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_LOCALIZED))
            {
                if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED | eAFF_LOADING))
                {
                    // The file needs to be unloaded first.
                    const size_t useCount = audioFileEntry->m_useCount;
                    audioFileEntry->m_useCount = 0;// Needed to uncache without an error.
                    UncacheFile(audioFileEntry);

                    UpdateLocalizedFileEntryData(audioFileEntry);

                    TryCacheFileCacheEntryInternal(audioFileEntry, audioFileEntryPair.first, true, true, useCount);
                }
                else
                {
                    // The file is not cached or loading, it is safe to update the corresponding CATLAudioFileEntry data.
                    UpdateLocalizedFileEntryData(audioFileEntry);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::TryLoadRequest(const TAudioPreloadRequestID preloadRequestID, const bool loadSynchronously, const bool autoLoadOnly)
    {
        bool fullSuccess = false;
        bool fullFailure = true;
        CATLPreloadRequest* const preloadRequest = stl::find_in_map(m_preloadRequests, preloadRequestID, nullptr);

        if (preloadRequest)
        {
            if (!preloadRequest->m_cFileEntryIDs.empty() && (!autoLoadOnly || (autoLoadOnly && preloadRequest->m_bAutoLoad)))
            {
                fullSuccess = true;
                for (auto fileId : preloadRequest->m_cFileEntryIDs)
                {
                    CATLAudioFileEntry* const audioFileEntry = stl::find_in_map(m_audioFileEntries, fileId, nullptr);

                    if (audioFileEntry)
                    {
                        const bool tempResult = TryCacheFileCacheEntryInternal(audioFileEntry, fileId, loadSynchronously);
                        fullSuccess = (fullSuccess && tempResult);
                        fullFailure = (fullFailure && !tempResult);
                    }
                }
            }

            if (fullSuccess && preloadRequest->m_allLoaded)
            {
                // Notify to handlers that the preload is already loaded/cached.
                AudioPreloadNotificationBus::Event(preloadRequestID, &AudioPreloadNotificationBus::Events::OnAudioPreloadCached);
            }
        }

        return (fullSuccess ? eARS_SUCCESS : (fullFailure ? eARS_FAILURE : eARS_PARTIAL_SUCCESS));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::TryUnloadRequest(const TAudioPreloadRequestID preloadRequestID)
    {
        bool fullSuccess = false;
        bool fullFailure = true;
        CATLPreloadRequest* const preloadRequest = stl::find_in_map(m_preloadRequests, preloadRequestID, nullptr);

        if (preloadRequest)
        {
            if (!preloadRequest->m_cFileEntryIDs.empty())
            {
                fullSuccess = true;
                for (auto fileId : preloadRequest->m_cFileEntryIDs)
                {
                    CATLAudioFileEntry* const audioFileEntry = stl::find_in_map(m_audioFileEntries, fileId, nullptr);

                    if (audioFileEntry)
                    {
                        const bool tempResult = UncacheFileCacheEntryInternal(audioFileEntry, true);
                        fullSuccess = (fullSuccess && tempResult);
                        fullFailure = (fullFailure && !tempResult);
                    }
                }
            }

            if (fullSuccess && !preloadRequest->m_allLoaded)
            {
                // Notify to handlers the the preload is already unloaded.
                AudioPreloadNotificationBus::Event(preloadRequestID, &AudioPreloadNotificationBus::Events::OnAudioPreloadUncached);
            }
        }

        return (fullSuccess ? eARS_SUCCESS : (fullFailure ? eARS_FAILURE : eARS_PARTIAL_SUCCESS));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CFileCacheManager::UnloadDataByScope(const EATLDataScope dataScope)
    {
        for (auto it = m_audioFileEntries.begin(); it != m_audioFileEntries.end(); )
        {
            CATLAudioFileEntry* const audioFileEntry = it->second;

            if (audioFileEntry && audioFileEntry->m_dataScope == dataScope)
            {
                if (UncacheFileCacheEntryInternal(audioFileEntry, true, true))
                {
                    it = m_audioFileEntries.erase(it);
                    continue;
                }
            }

            ++it;
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::UncacheFileCacheEntryInternal(CATLAudioFileEntry* const audioFileEntry, const bool now, const bool ignoreUsedCount /* = false */)
    {
        bool success = false;

        // In any case decrement the used count.
        if (audioFileEntry->m_useCount > 0)
        {
            --audioFileEntry->m_useCount;
        }

        if (audioFileEntry->m_useCount < 1 || ignoreUsedCount)
        {
            // Must be cached to proceed.
            if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED))
            {
                // Only "use-counted" files can become removable!
                if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_USE_COUNTED))
                {
                    audioFileEntry->m_flags.AddFlags(eAFF_REMOVABLE);
                }

                if (now || ignoreUsedCount)
                {
                    UncacheFile(audioFileEntry);
                }
            }
            else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_LOADING | eAFF_MEMALLOCFAIL))
            {
                g_audioLogger.Log(eALT_ALWAYS, "FileCacheManager - Trying to remove a loading or mem-failed file cache entry '%s'", audioFileEntry->m_filePath.c_str());

                // Reset the entry in case it's still loading or was a memory allocation fail.
                UncacheFile(audioFileEntry);
            }

            // The file was either properly uncached, queued for uncache or not cached at all.
            success = true;
        }

        return success;
    }

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, const float posX, const float posY)
    {
        if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_FILECACHE_MANAGER_INFO) != 0)
        {
            EATLDataScope dataScope = eADS_ALL;

            if ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_ALL) == 0)
            {
                if ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_GLOBALS) != 0)
                {
                    dataScope = eADS_GLOBAL;
                }
                else if ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_LEVEL_SPECIFICS) != 0)
                {
                    dataScope = eADS_LEVEL_SPECIFIC;
                }
            }

            const CTimeValue frameTime = gEnv->pTimer->GetAsyncTime();

            CryFixedStringT<MAX_AUDIO_MISC_STRING_LENGTH> displayString;
            const float entryDrawSize = 1.5f;
            const float entryStepSize = 15.0f;
            float positionY = posY + 20.0f;
            float positionX = posX + 20.0f;
            float time = 0.0f;
            float ratio = 0.0f;
            float originalAlpha = 0.7f;
            float* color = nullptr;

            // The colors.
            float white[4] = { 1.0f, 1.0f, 1.0f, originalAlpha };
            float cyan[4] = { 0.0f, 1.0f, 1.0f, originalAlpha };
            float orange[4] = { 1.0f, 0.5f, 0.0f, originalAlpha };
            float green[4] = { 0.0f, 1.0f, 0.0f, originalAlpha };
            float red[4] = { 1.0f, 0.0f, 0.0f, originalAlpha };
            float redish[4] = { 0.7f, 0.0f, 0.0f, originalAlpha };
            float blue[4] = { 0.1f, 0.2f, 0.8f, originalAlpha };
            float yellow[4] = { 1.0f, 1.0f, 0.0f, originalAlpha };
            float darkish[4] = { 0.3f, 0.3f, 0.3f, originalAlpha };

            auxGeom.Draw2dLabel(posX, positionY, 1.6f, orange, false,
                "FileCacheManager (%" PRISIZE_T " of %" PRISIZE_T " KiB) [Entries: %" PRISIZE_T "]", m_currentByteTotal >> 10, m_maxByteTotal >> 10, m_audioFileEntries.size());
            positionY += 15.0f;

            bool displayAll = (g_audioCVars.m_nFileCacheManagerDebugFilter == eAFCMDF_ALL);
            bool displayGlobals = ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_GLOBALS) != 0);
            bool displayLevels = ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_LEVEL_SPECIFICS) != 0);
            bool displayUseCounted = ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_USE_COUNTED) != 0);
            bool displayLoaded = ((g_audioCVars.m_nFileCacheManagerDebugFilter & eAFCMDF_LOADED) != 0);

            for (auto& audioFileEntryPair : m_audioFileEntries)
            {
                CATLAudioFileEntry* const audioFileEntry = audioFileEntryPair.second;

                bool isGlobal = (audioFileEntry->m_dataScope == eADS_GLOBAL);
                bool isLevel = (audioFileEntry->m_dataScope == eADS_LEVEL_SPECIFIC);
                bool isUseCounted = audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_USE_COUNTED);
                bool isLoaded = audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED);

                if (displayAll || (displayGlobals && isGlobal) || (displayLevels && isLevel) || (displayUseCounted && isUseCounted) || (displayLoaded && isLoaded))
                {
                    if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_LOADING))
                    {
                        color = blue;
                    }
                    else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_MEMALLOCFAIL))
                    {
                        color = red;
                    }
                    else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_REMOVABLE))
                    {
                        color = green;
                    }
                    else if (!audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED))
                    {
                        color = darkish;
                    }
                    else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_NOTFOUND))
                    {
                        color = redish;
                    }
                    else if (isGlobal)
                    {
                        color = cyan;
                    }
                    else if (isLevel)
                    {
                        color = yellow;
                    }
                    else  // isUseCounted
                    {
                        color = white;
                    }

                    time = (frameTime - audioFileEntry->m_timeCached).GetSeconds();
                    ratio = time / 5.0f;
                    originalAlpha = color[3];
                    color[3] *= clamp_tpl(ratio, 0.2f, 1.0f);

                    displayString.clear();

                    bool kiloBytes = false;
                    AZStd::size_t fileSize = audioFileEntry->m_fileSize;
                    if (fileSize >= 1024)
                    {
                        fileSize >>= 10;
                        kiloBytes = true;
                    }

                    // Format: "relative/path/filename.ext (230 KiB) [2]"
                    displayString.Format("%s (%" PRISIZE_T " %s) [%" PRISIZE_T "]",
                        audioFileEntry->m_filePath.c_str(),
                        fileSize,
                        kiloBytes ? "KiB" : "Bytes",
                        audioFileEntry->m_useCount);

                    auxGeom.Draw2dLabel(positionX, positionY, entryDrawSize, color, false, displayString.c_str());
                    color[3] = originalAlpha;
                    positionY += entryStepSize;
                }
            }
        }
    }
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::DoesRequestFitInternal(const size_t requestSize)
    {
        // Make sure these unsigned values don't flip around.
        AZ_Assert(m_currentByteTotal <= m_maxByteTotal, "FileCacheManager DoesRequestFitInternal - Unsigned wraparound detected!");
        bool success = false;

        if (requestSize <= (m_maxByteTotal - m_currentByteTotal))
        {
            // Here the requested size is available without the need of first cleaning up.
            success = true;
        }
        else
        {
            // Determine how much memory would get freed if all eAFF_REMOVABLE files get thrown out.
            // We however skip files that are already queued for unload. The request will get queued up in that case.
            size_t possibleMemoryGain = 0;

            // Check the single file entries for removability.
            for (auto& audioFileEntryPair : m_audioFileEntries)
            {
                CATLAudioFileEntry* const audioFileEntry = audioFileEntryPair.second;

                if (audioFileEntry && audioFileEntry->m_flags.AreAllFlagsActive(eAFF_CACHED | eAFF_REMOVABLE))
                {
                    possibleMemoryGain += audioFileEntry->m_fileSize;
                }
            }

            const size_t maxAvailableSize = (m_maxByteTotal - (m_currentByteTotal - possibleMemoryGain));

            if (requestSize <= maxAvailableSize)
            {
                // Here we need to cleanup first before allowing the new request to be allocated.
                TryToUncacheFiles();

                // We should only indicate success if there's actually really enough room for the new entry!
                success = (m_maxByteTotal - m_currentByteTotal) >= requestSize;
            }
        }

        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::FinishAsyncStreamRequest(AsyncStreamCompletionState completionState)
    {
        AZ_Assert(completionState.m_request, "FileCacheManager - The request handle is invalid!");

        // Find the file entry that matches the request handle...
        auto fileEntryIter = AZStd::find_if(m_audioFileEntries.begin(), m_audioFileEntries.end(),
            [&completionState] (const AZStd::pair<TAudioFileEntryID, CATLAudioFileEntry*>& data) -> bool
            {
                return (data.second->m_asyncStreamRequest == completionState.m_request);
            }
        );

        // If found, we finish processing the async file load request...
        if (fileEntryIter != m_audioFileEntries.end())
        {
            CATLAudioFileEntry* audioFileEntry = fileEntryIter->second;
            AZ_Assert(audioFileEntry, "FileCacheManager - Audio file entry is null!");
            AZ_Assert(completionState.m_buffer == audioFileEntry->m_memoryBlock->GetData(), "FileCacheManager - The memory buffer doesn't match the file entry memory block!");
            FinishCachingFileInternal(audioFileEntry, completionState.m_bytes, completionState.m_requestState);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::FinishCachingFileInternal(CATLAudioFileEntry* const audioFileEntry, AZ::IO::SizeType bytesRead, AZ::IO::Request::StateType requestState)
    {
        bool success = false;
        
        audioFileEntry->m_asyncStreamRequest.reset();

        switch (requestState)
        {
            case AZ::IO::Request::StateType::ST_COMPLETED:
            {
                AZ_Assert(bytesRead == audioFileEntry->m_fileSize, "FileCacheManager - Sync Streamed Read completed, but bytes read does not match file size!");

                if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_LOADING))
                {
                    audioFileEntry->m_flags.AddFlags(eAFF_CACHED);
                    audioFileEntry->m_flags.ClearFlags(eAFF_LOADING);

                #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    audioFileEntry->m_timeCached = gEnv->pTimer->GetAsyncTime();
                #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                    SATLAudioFileEntryInfo fileEntryInfo;
                    fileEntryInfo.nMemoryBlockAlignment = audioFileEntry->m_memoryBlockAlignment;
                    fileEntryInfo.pFileData = audioFileEntry->m_memoryBlock->GetData();
                    fileEntryInfo.nSize = audioFileEntry->m_fileSize;
                    fileEntryInfo.pImplData = audioFileEntry->m_implData;
                    fileEntryInfo.sFileName = PathUtil::GetFile(audioFileEntry->m_filePath.c_str());

                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::RegisterInMemoryFile, &fileEntryInfo);
                    success = true;

                g_audioLogger.Log(eALT_COMMENT, "FileCacheManager - File Cached: '%s'\n", fileEntryInfo.sFileName);
                }
                break;
            }
            case AZ::IO::Request::StateType::ST_ERROR_FAILED_TO_OPEN_FILE:
            {
                AZ_Error("FileCacheManager", false, "FileCacheManager - Failed to open file stream '%s'!", audioFileEntry->m_filePath.c_str());
                // what do we do for errors?  remove the m_memoryBlock?  not update the m_currentByteTotal?
                break;
            }
            case AZ::IO::Request::StateType::ST_ERROR_FAILED_IN_OPERATION:
            {
                AZ_Error("FileCacheManager", false, "FileCacheManager - Async file stream '%s' failed during operation!", audioFileEntry->m_filePath.c_str());
                UncacheFileCacheEntryInternal(audioFileEntry, true, true);
                break;
            }
            case AZ::IO::Request::StateType::ST_CANCELLED:
            {
                g_audioLogger.Log(eALT_COMMENT, "FileCacheManager - Async file stream '%s' was cancelled by user!", audioFileEntry->m_filePath.c_str());
                UncacheFileCacheEntryInternal(audioFileEntry, true, true);
                break;
            }
            default:
            {
                break;
            }
        }

        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UpdatePreloadRequestsStatus()
    {
        // Run through the list of preload requests and their fileEntryIDs.
        // Check the fileEntries for the CACHED flags and accumulate the 'allLoaded' and 'anyLoaded' status of each preload request.
        // If the result is different than what is stored on the preload request, update it and send a notification of
        // either cached or uncached.
        for (auto& preloadPair : m_preloadRequests)
        {
            CATLPreloadRequest* preloadRequest = preloadPair.second;
            bool wasLoaded = preloadRequest->m_allLoaded;
            bool allLoaded = !preloadRequest->m_cFileEntryIDs.empty();
            bool anyLoaded = false;
            for (auto fileId : preloadRequest->m_cFileEntryIDs)
            {
                bool cached = false;
                auto iter = m_audioFileEntries.find(fileId);
                if (iter != m_audioFileEntries.end())
                {
                    cached = iter->second->m_flags.AreAnyFlagsActive(eAFF_CACHED);
                }
                allLoaded = (allLoaded && cached);
                anyLoaded = (anyLoaded || cached);
            }

            if (allLoaded != wasLoaded && allLoaded)
            {
                // Loaded now...
                preloadRequest->m_allLoaded = allLoaded;
                AudioPreloadNotificationBus::Event(preloadPair.first, &AudioPreloadNotificationBus::Events::OnAudioPreloadCached);
            }

            if (anyLoaded != wasLoaded && !anyLoaded)
            {
                // Unloaded now...
                preloadRequest->m_allLoaded = anyLoaded;
                AudioPreloadNotificationBus::Event(preloadPair.first, &AudioPreloadNotificationBus::Events::OnAudioPreloadUncached);
            }
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::OnAsyncStreamFinished(const AZStd::shared_ptr<AZ::IO::Request>& request, AZ::IO::SizeType bytesRead, void* buffer, AZ::IO::Request::StateType requestState)
    {
        // This callback happens on a device thread, so queue up a notification to handle it on the Audio Thread.
        AudioFileCacheManagerNotficationBus::QueueBroadcast(&AudioFileCacheManagerNotficationBus::Events::FinishAsyncStreamRequest, AsyncStreamCompletionState { request, bytesRead, buffer, requestState });
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::AllocateMemoryBlockInternal(CATLAudioFileEntry* const __restrict audioFileEntry)
    {
        // Must not have valid memory yet.
        AZ_Assert(!audioFileEntry->m_memoryBlock, "FileCacheManager AllocateMemoryBlockInternal - Memory appears to be set already!");

        if (m_memoryHeap)
        {
            audioFileEntry->m_memoryBlock.reset(m_memoryHeap->AllocateBlock(audioFileEntry->m_fileSize, audioFileEntry->m_filePath.c_str(), audioFileEntry->m_memoryBlockAlignment));
        }

        if (!audioFileEntry->m_memoryBlock)
        {
            // Memory block is either full or too fragmented, let's try to throw everything out that can be removed and allocate again.
            TryToUncacheFiles();

            // And try again!
            if (m_memoryHeap)
            {
                audioFileEntry->m_memoryBlock.reset(m_memoryHeap->AllocateBlock(audioFileEntry->m_fileSize, audioFileEntry->m_filePath.c_str(), audioFileEntry->m_memoryBlockAlignment));
            }
        }

        return (audioFileEntry->m_memoryBlock != nullptr);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UncacheFile(CATLAudioFileEntry* const audioFileEntry)
    {
        m_currentByteTotal -= audioFileEntry->m_fileSize;

        if (audioFileEntry->m_asyncStreamRequest)
        {
            AZ::IO::GetStreamer()->CancelRequest(audioFileEntry->m_asyncStreamRequest);
            audioFileEntry->m_asyncStreamRequest.reset();
        }

        if (audioFileEntry->m_memoryBlock && audioFileEntry->m_memoryBlock->GetData())
        {
            SATLAudioFileEntryInfo fileEntryInfo;
            fileEntryInfo.nMemoryBlockAlignment = audioFileEntry->m_memoryBlockAlignment;
            fileEntryInfo.pFileData = audioFileEntry->m_memoryBlock->GetData();
            fileEntryInfo.nSize = audioFileEntry->m_fileSize;
            fileEntryInfo.pImplData = audioFileEntry->m_implData;
            fileEntryInfo.sFileName = PathUtil::GetFile(audioFileEntry->m_filePath.c_str());

            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::UnregisterInMemoryFile, &fileEntryInfo);
            g_audioLogger.Log(eALT_COMMENT, "FileCacheManager - File Uncached: '%s'\n", fileEntryInfo.sFileName);
        }

        audioFileEntry->m_memoryBlock.reset();
        audioFileEntry->m_flags.ClearFlags(eAFF_CACHED | eAFF_REMOVABLE);
        AZ_Warning("FileCacheManager", audioFileEntry->m_useCount == 0, "Use-count of file '%s' is non-zero while uncaching it! Use Count: %d", audioFileEntry->m_filePath.c_str(), audioFileEntry->m_useCount);
        audioFileEntry->m_useCount = 0;

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        audioFileEntry->m_timeCached.SetValue(0);
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::TryToUncacheFiles()
    {
        for (auto& audioFileEntryPair : m_audioFileEntries)
        {
            CATLAudioFileEntry* const audioFileEntry = audioFileEntryPair.second;

            if (audioFileEntry && audioFileEntry->m_flags.AreAllFlagsActive(eAFF_CACHED | eAFF_REMOVABLE))
            {
                UncacheFileCacheEntryInternal(audioFileEntry, true);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void CFileCacheManager::UpdateLocalizedFileEntryData(CATLAudioFileEntry* const audioFileEntry)
    {
        static SATLAudioFileEntryInfo fileEntryInfo;
        fileEntryInfo.bLocalized = true;
        fileEntryInfo.nSize = 0;
        fileEntryInfo.pFileData = nullptr;
        fileEntryInfo.nMemoryBlockAlignment = 0;

        CryFixedStringT<MAX_AUDIO_FILE_NAME_LENGTH> fileName(PathUtil::GetFile(audioFileEntry->m_filePath.c_str()));
        fileEntryInfo.pImplData = audioFileEntry->m_implData;
        fileEntryInfo.sFileName = fileName.c_str();

        const char* fileLocation = nullptr;
        AudioSystemImplementationRequestBus::BroadcastResult(fileLocation, &AudioSystemImplementationRequestBus::Events::GetAudioFileLocation, &fileEntryInfo);
        audioFileEntry->m_filePath = fileLocation;
        audioFileEntry->m_filePath += fileName.c_str();
        audioFileEntry->m_filePath.MakeLower();

        audioFileEntry->m_fileSize = gEnv->pCryPak->FGetSize(audioFileEntry->m_filePath.c_str());

        AZ_Assert(audioFileEntry->m_fileSize > 0, "FileCacheManager UpdateLocalizedFileEntryData - Expected file size to be greater than zero!");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool CFileCacheManager::TryCacheFileCacheEntryInternal(
        CATLAudioFileEntry* const audioFileEntry,
        const TAudioFileEntryID fileEntryId,
        const bool loadSynchronously,
        const bool overrideUseCount /* = false */,
        const size_t useCount /* = 0 */)
    {
        bool success = false;

        if (!audioFileEntry->m_filePath.empty() && !audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED | eAFF_LOADING))
        {
            if (DoesRequestFitInternal(audioFileEntry->m_fileSize) && AllocateMemoryBlockInternal(audioFileEntry))
            {
                AZ_Assert(AZ::IO::Streamer::IsReady(), "FileCacheManager - Streamer should be ready!");

                audioFileEntry->m_flags.AddFlags(eAFF_LOADING);

                if (loadSynchronously)
                {
                    AZ::IO::Request::StateType ioState;
                    AZ::IO::SizeType bytesRead = AZ::IO::GetStreamer()->Read(
                        audioFileEntry->m_filePath,
                        0,
                        audioFileEntry->m_fileSize,
                        audioFileEntry->m_memoryBlock->GetData(),
                        //The priority is only relevant when multiple files are at risk of missing their deadline,
                        //in which case the higher priority file is prioritized first. Since the deadline is infinite,
                        //this request will never be late and therefore the priority will never apply.
                        AZStd::chrono::milliseconds::max(),
                        AZ::IO::Request::PriorityType::DR_PRIORITY_ABOVE_NORMAL, &ioState
                        );

                    m_currentByteTotal += audioFileEntry->m_fileSize;
                    success = FinishCachingFileInternal(audioFileEntry, bytesRead, ioState);
                }
                else
                {
                    //  Async loading...
                    audioFileEntry->m_asyncStreamRequest = AZ::IO::GetStreamer()->CreateAsyncRead(
                        audioFileEntry->m_filePath,
                        0,
                        audioFileEntry->m_fileSize,
                        audioFileEntry->m_memoryBlock->GetData(),
                        AZStd::bind(&CFileCacheManager::OnAsyncStreamFinished, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4),
                        //See note above priority.
                        AZStd::chrono::milliseconds::max(),
                        AZ::IO::Request::PriorityType::DR_PRIORITY_ABOVE_NORMAL
                        );

                    if (!audioFileEntry->m_asyncStreamRequest)
                    {
                        AZ_Error("FileCacheManager", false, "FileCacheManager - Async file stream request for '%s' returned an invalid handle.", 
                            audioFileEntry->m_filePath.c_str());
                        success = false;
                    }
                    else
                    {
                        if (!AZ::IO::GetStreamer()->QueueRequest(audioFileEntry->m_asyncStreamRequest))
                        {
                            AZ_Error("FileCacheManager", false, "FileCacheManager - Async file stream request for '%s' failed to be queued.",
                                audioFileEntry->m_filePath.c_str());
                            success = false;
                        }
                        else
                        {
                            // Increase total size even though async request is processing...
                            m_currentByteTotal += audioFileEntry->m_fileSize;
                            success = true;
                        }
                    }
                }
            }
            else
            {
                // Cannot have a valid memory block!
                AZ_Assert(!audioFileEntry->m_memoryBlock || !audioFileEntry->m_memoryBlock->GetData(),
                    "FileCacheManager TryCacheFileCacheEntryInternal - Cannot have a valid memory block after memory allocation failure!");

                // This unfortunately is a total memory allocation fail.
                audioFileEntry->m_flags.AddFlags(eAFF_MEMALLOCFAIL);

                // The user should be made aware of it.
                g_audioLogger.Log(eALT_ERROR, "FileCacheManager: Could not cache '%s' - out of memory or fragmented memory!", audioFileEntry->m_filePath.c_str());
            }
        }
        else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED | eAFF_LOADING))
        {
            g_audioLogger.Log(eALT_COMMENT, "AFCM: Could not cache '%s' as it is either already loaded or currently loading!", audioFileEntry->m_filePath.c_str());
            success = true;
        }
        else if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_NOTFOUND))
        {
            g_audioLogger.Log(eALT_ERROR, "FileCacheManager: Could not cache '%s' - file was not found at the target location!", audioFileEntry->m_filePath.c_str());
        }

        // Increment the used count on manually-loaded files.
        if (audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_USE_COUNTED) && audioFileEntry->m_flags.AreAnyFlagsActive(eAFF_CACHED | eAFF_LOADING))
        {
            if (overrideUseCount)
            {
                audioFileEntry->m_useCount = useCount;
            }
            else
            {
                ++audioFileEntry->m_useCount;
            }

            // Make sure to handle the eAFCS_REMOVABLE flag according to the m_useCount count.
            if (audioFileEntry->m_useCount != 0)
            {
                audioFileEntry->m_flags.ClearFlags(eAFF_REMOVABLE);
            }
            else
            {
                audioFileEntry->m_flags.AddFlags(eAFF_REMOVABLE);
            }
        }

        return success;
    }

} // namespace Audio
