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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <ATLEntities.h>

// Forward declarations
class CCustomMemoryHeap;
struct IRenderAuxGeom;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    struct AsyncStreamCompletionState
    {
        AZStd::shared_ptr<AZ::IO::Request> m_request;
        AZ::IO::SizeType m_bytes = 0;
        void* m_buffer = nullptr;
        AZ::IO::Request::StateType m_requestState = AZ::IO::Request::StateType::ST_COMPLETED;

        AsyncStreamCompletionState() = default;
        AsyncStreamCompletionState(const AZStd::shared_ptr<AZ::IO::Request>& request, AZ::IO::SizeType bytes, void* buffer, AZ::IO::Request::StateType requestState)
            : m_request(request)
            , m_bytes(bytes)
            , m_buffer(buffer)
            , m_requestState(requestState)
        {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class AudioFileCacheManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioFileCacheManagerNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - Single Bus Address, Single Handler, Mutex, Queued
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const bool EnableEventQueue = true;
        using MutexType = AZStd::recursive_mutex;
        ///////////////////////////////////////////////////////////////////////////////////////////

        virtual void FinishAsyncStreamRequest(AsyncStreamCompletionState completionState) = 0;
    };

    using AudioFileCacheManagerNotficationBus = AZ::EBus<AudioFileCacheManagerNotifications>;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class CFileCacheManager
        : public AudioFileCacheManagerNotficationBus::Handler
    {
    public:
        explicit CFileCacheManager(TATLPreloadRequestLookup& preloadRequests);
        ~CFileCacheManager() override;

        CFileCacheManager(const CFileCacheManager&) = delete;           // Copy protection
        CFileCacheManager& operator=(const CFileCacheManager&) = delete; // Copy protection

        // Public methods
        void Initialize();
        void Release();
        void Update();
        TAudioFileEntryID TryAddFileCacheEntry(const XmlNodeRef fileXmlNode, const EATLDataScope dataScope, const bool autoLoad);
        bool TryRemoveFileCacheEntry(const TAudioFileEntryID audioFileID, const EATLDataScope dataScope);
        void UpdateLocalizedFileCacheEntries();

        EAudioRequestStatus TryLoadRequest(const TAudioPreloadRequestID preloadRequestID, const bool loadSynchronously, const bool autoLoadOnly);
        EAudioRequestStatus TryUnloadRequest(const TAudioPreloadRequestID preloadRequestID);
        EAudioRequestStatus UnloadDataByScope(const EATLDataScope dataScope);

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        void DrawDebugInfo(IRenderAuxGeom& auxGeom, const float posX, const float posY);
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE

    private:
        // Internal type definitions.
        using TAudioFileEntries = ATLMapLookupType<TAudioFileEntryID, CATLAudioFileEntry*>;

        // Internal methods
        void AllocateHeap(const size_t size, const char* const usage);
        bool UncacheFileCacheEntryInternal(CATLAudioFileEntry* const audioFileEntry, const bool now, const bool ignoreUsedCount = false);
        bool DoesRequestFitInternal(const size_t requestSize);
        void UpdatePreloadRequestsStatus();
        bool FinishCachingFileInternal(CATLAudioFileEntry* const audioFileEntry, AZ::IO::SizeType sizeBytes, AZ::IO::Request::StateType requestState);

        ///////////////////////////////////////////////////////////////////////////////////////////
        // AudioFileCacheManagerNotficationBus
        void FinishAsyncStreamRequest(AsyncStreamCompletionState completionState) override;
        ///////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////
        // AZ::IO::Request::RequestDoneCB
        void OnAsyncStreamFinished(const AZStd::shared_ptr<AZ::IO::Request>& request, AZ::IO::SizeType bytesRead, void* buffer, AZ::IO::Request::StateType requestState);
        ///////////////////////////////////////////////////////////////////////////////////////////

        bool AllocateMemoryBlockInternal(CATLAudioFileEntry* const __restrict audioFileEntry);
        void UncacheFile(CATLAudioFileEntry* const audioFileEntry);
        void TryToUncacheFiles();
        void UpdateLocalizedFileEntryData(CATLAudioFileEntry* const audioFileEntry);
        bool TryCacheFileCacheEntryInternal(CATLAudioFileEntry* const audioFileEntry, const TAudioFileEntryID fileID, const bool loadSynchronously, const bool overrideUseCount = false, const size_t useCount = 0);

        // Internal members
        TATLPreloadRequestLookup& m_preloadRequests;
        TAudioFileEntries m_audioFileEntries;

        AZStd::unique_ptr<CCustomMemoryHeap> m_memoryHeap;
        size_t m_currentByteTotal;
        size_t m_maxByteTotal;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Filter for drawing debug info to the screen
    enum EAudioFileCacheManagerDebugFilter
    {
        eAFCMDF_ALL = 0,
        eAFCMDF_GLOBALS         = BIT(6),   // a
        eAFCMDF_LEVEL_SPECIFICS = BIT(7),   // b
        eAFCMDF_USE_COUNTED     = BIT(8),   // c
        eAFCMDF_LOADED           = BIT(9),  // d
    };

} // namespace Audio
