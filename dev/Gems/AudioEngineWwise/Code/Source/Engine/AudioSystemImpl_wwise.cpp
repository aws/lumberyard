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

#include <AudioSystemImpl_wwise.h>

#include <platform.h>

#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/conversions.h>

#include <IAudioSystem.h>
#include <AudioAllocators.h>
#include <AudioLogger.h>
#include <AudioSourceManager.h>
#include <AudioSystemImplCVars.h>
#include <Common_wwise.h>
#include <FileIOHandler_wwise.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>        // Sound engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>        // Music Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>          // Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>             // Default memory and stream managers

#include <PluginRegistration_wwise.h>                   // Registration of default set of plugins, customize this header to your needs.


#if !defined(WWISE_FOR_RELEASE)
    #include <AK/Comm/AkCommunication.h>    // Communication between Wwise and the game (excluded in release build)
    #include <AK/Tools/Common/AkMonitorError.h>
    #include <AK/Tools/Common/AkPlatformFuncs.h>
#endif // WWISE_FOR_RELEASE

#if defined(AK_MAX_AUX_PER_OBJ)
    #define LY_MAX_AUX_PER_OBJ  AK_MAX_AUX_PER_OBJ
#else
    #define LY_MAX_AUX_PER_OBJ  (4)
#endif



/////////////////////////////////////////////////////////////////////////////////
//                          AK MEMORY HOOKS SETUP
//
//                          ##### REQUIRED ######
//
// AK declares these hooks as "extern" functions in AkTypes.h.
// Client code is required to give them definitions.
//
/////////////////////////////////////////////////////////////////////////////////

namespace AK
{
    void* AllocHook(size_t in_size)
    {
        return azmalloc(in_size, AUDIO_MEMORY_ALIGNMENT, Audio::AudioImplAllocator, "AudioWwise");
    }

    void FreeHook(void* in_ptr)
    {
        azfree(in_ptr, Audio::AudioImplAllocator);
    }

    void* VirtualAllocHook(void* in_pMemAddress, size_t in_size, DWORD in_dwAllocationType, DWORD in_dwProtect)
    {
        //return VirtualAlloc(in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect);
        return azmalloc(in_size, AUDIO_MEMORY_ALIGNMENT, Audio::AudioImplAllocator, "AudioWwise");
    }

    void VirtualFreeHook(void* in_pMemAddress, size_t in_size, DWORD in_dwFreeType)
    {
        //VirtualFree(in_pMemAddress, in_size, in_dwFreeType);
        azfree(in_pMemAddress, Audio::AudioImplAllocator);
    }
}



namespace Audio
{
    namespace Platform
    {
        void SetupAkSoundEngine(AkPlatformInitSettings& platformInitSettings);
    }

    extern CAudioLogger g_audioImplLogger_wwise;
    extern CAudioWwiseImplCVars g_audioImplCVars_wwise;

    const char* const CAudioSystemImpl_wwise::sWwiseImplSubPath = "wwise/";
    const char* const CAudioSystemImpl_wwise::sWwiseGlobalAudioObjectName = "LY-GlobalAudioObject";
    const float CAudioSystemImpl_wwise::sObstructionOcclusionMin = 0.0f;
    const float CAudioSystemImpl_wwise::sObstructionOcclusionMax = 1.0f;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AK callbacks
    void WwiseEventCallback(AkCallbackType eType, AkCallbackInfo* pCallbackInfo)
    {
        if (eType == AK_EndOfEvent)
        {
            auto const pEventData = static_cast<SATLEventData_wwise*>(pCallbackInfo->pCookie);

            if (pEventData)
            {
                SAudioRequest oRequest;
                SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT> oRequestData(pEventData->nATLID, true);
                oRequest.nFlags = eARF_THREAD_SAFE_PUSH;
                oRequest.pData = &oRequestData;

                AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, oRequest);

                if (pEventData->nSourceId != INVALID_AUDIO_SOURCE_ID)
                {
                    AkPlayingID playingId = AudioSourceManager::Get().FindPlayingSource(pEventData->nSourceId);
                    AudioSourceManager::Get().DeactivateSource(playingId);
                }
            }
        }
        else if (eType == AK_Duration)
        {
            auto durationInfo = static_cast<AkDurationCallbackInfo*>(pCallbackInfo);
            auto const eventData = static_cast<SATLEventData_wwise*>(pCallbackInfo->pCookie);
            if (durationInfo && eventData)
            {
                AudioTriggerNotificationBus::QueueEvent(eventData->m_triggerId, &AudioTriggerNotificationBus::Events::ReportDurationInfo, eventData->nATLID, durationInfo->fDuration, durationInfo->fEstimatedDuration);
            }
        }
    }

    static bool audioDeviceInitializationEvent = false;

    void AudioDeviceCallback(
        AK::IAkGlobalPluginContext* context,
        AkUniqueID audioDeviceSharesetId,
        AkUInt32 deviceId,
        AK::AkAudioDeviceEvent deviceEvent,
        AKRESULT inAkResult
    )
    {
        if (deviceEvent == AK::AkAudioDeviceEvent_Initialization)
        {
            audioDeviceInitializationEvent = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void PrepareEventCallback(
        AkUniqueID nAkEventID,
        const void* pBankPtr,
        AKRESULT eLoadResult,
        AkMemPoolId nMenmPoolID,
        void* pCookie)
    {
        auto const pEventData = static_cast<SATLEventData_wwise*>(pCookie);

        if (pEventData)
        {
            pEventData->nAKID = nAkEventID;

            SAudioRequest oRequest;
            SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT> oRequestData(pEventData->nATLID, eLoadResult ==  AK_Success);
            oRequest.nFlags = eARF_THREAD_SAFE_PUSH;
            oRequest.pData = &oRequestData;

            AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, oRequest);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(WWISE_FOR_RELEASE)
    static void ErrorMonitorCallback(
        AK::Monitor::ErrorCode in_eErrorCode,   ///< Error code number value
        const AkOSChar* in_pszError,            ///< Message or error string to be displayed
        AK::Monitor::ErrorLevel in_eErrorLevel, ///< Specifies whether it should be displayed as a message or an error
        AkPlayingID in_playingID,               ///< Related Playing ID if applicable, AK_INVALID_PLAYING_ID otherwise
        AkGameObjectID in_gameObjID             ///< Related Game Object ID if applicable, AK_INVALID_GAME_OBJECT otherwise
        )
    {
        char* sTemp = nullptr;
        CONVERT_OSCHAR_TO_CHAR(in_pszError, sTemp);
        g_audioImplLogger_wwise.Log(
            ((in_eErrorLevel & AK::Monitor::ErrorLevel_Error) != 0) ? eALT_ERROR : eALT_COMMENT,
            "<Wwise> %s ErrorCode: %d PlayingID: %u GameObjID: %llu", sTemp, in_eErrorCode, in_playingID, in_gameObjID);
    }
#endif // WWISE_FOR_RELEASE

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static int GetAssetType(const SATLSourceData* sourceData)
    {
        if (!sourceData)
        {
            return eAAT_NONE;
        }

        return sourceData->m_sourceInfo.m_codecType == eACT_STREAM_PCM
               ? eAAT_STREAM
               : eAAT_SOURCE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static int GetAkCodecID(EAudioCodecType codecType)
    {
        switch (codecType)
        {
        case eACT_AAC:
            return AKCODECID_AAC;
        case eACT_ADPCM:
            return AKCODECID_ADPCM;
        case eACT_PCM:
            return AKCODECID_PCM;
        case eACT_VORBIS:
            return AKCODECID_VORBIS;
        case eACT_XMA:
            return AKCODECID_XMA;
        case eACT_XWMA:
            return AKCODECID_XWMA;
        case eACT_STREAM_PCM:
        default:
            AZ_Assert(codecType, "Codec not supported");
            return AKCODECID_VORBIS;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImpl_wwise::CAudioSystemImpl_wwise()
        : m_globalGameObjectID(static_cast<AkGameObjectID>(GLOBAL_AUDIO_OBJECT_ID))
        , m_defaultListenerGameObjectID(AK_INVALID_GAME_OBJECT)
        , m_nInitBankID(AK_INVALID_BANK_ID)
#if !defined(WWISE_FOR_RELEASE)
        , m_bCommSystemInitialized(false)
#endif // !WWISE_FOR_RELEASE
    {
        m_soundbankFolder = WWISE_IMPL_BANK_FULL_PATH;

        m_localizedSoundbankFolder = m_soundbankFolder;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        m_fullImplString = AZStd::string::format("%s (%s)", WWISE_IMPL_VERSION_STRING, m_soundbankFolder.c_str());
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

        AudioSystemImplementationRequestBus::Handler::BusConnect();
        AudioSystemImplementationNotificationBus::Handler::BusConnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImpl_wwise::~CAudioSystemImpl_wwise()
    {
        AudioSystemImplementationRequestBus::Handler::BusDisconnect();
        AudioSystemImplementationNotificationBus::Handler::BusDisconnect();
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioSystemImplementationNotificationBus
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemLoseFocus()
    {
    #if AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND
        AKRESULT akResult = AK::SoundEngine::Suspend();
        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to Suspend, AKRESULT = %d\n", akResult);
        }
    #endif // AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemGetFocus()
    {
    #if AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND
        AKRESULT akResult = AK::SoundEngine::WakeupFromSuspend();
        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to WakeupFromSuspend, AKRESULT = %d\n", akResult);
        }
    #endif // AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemMuteAll()
    {
        // With Wwise we drive this via events.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemUnmuteAll()
    {
        // With Wwise we drive this via events.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemRefresh()
    {
        AKRESULT eResult = AK_Fail;

        if (m_nInitBankID != AK_INVALID_BANK_ID)
        {
            eResult = AK::SoundEngine::UnloadBank(m_nInitBankID, nullptr);

            if (!IS_WWISE_OK(eResult))
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to unload Init.bnk, returned the AKRESULT: %d", eResult);
                AZ_Assert(false, "<Wwise> Failed to unload Init.bnk!");
            }
        }

        eResult = AK::SoundEngine::LoadBank(AKTEXT("init.bnk"), AK_DEFAULT_POOL_ID, m_nInitBankID);

        if (!IS_WWISE_OK(eResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to load Init.bnk, returned the AKRESULT: %d", eResult);
            m_nInitBankID = AK_INVALID_BANK_ID;
            AZ_Assert(false, "<Wwise> Failed to load Init.bnk!");
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioSystemImplementationRequestBus
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::Update(const float fUpdateIntervalMS)
    {
        if (AK::SoundEngine::IsInitialized())
        {
    #if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
            AKRESULT eResult = AK_Fail;
            static int nEnableOutputCapture = 0;

            if (g_audioImplCVars_wwise.m_nEnableOutputCapture == 1 && nEnableOutputCapture == 0)
            {
                // This file ends up in the cache folder
                // Need to disable this on LTX, it produces garbage output.  But there's no way to "IsLTX()" yet.
                eResult = AK::SoundEngine::StartOutputCapture(AKTEXT("../wwise_audio_capture.wav"));
                AZ_Assert(IS_WWISE_OK(eResult), "AK::SoundEngine::StartOutputCapture failed!");
                nEnableOutputCapture = g_audioImplCVars_wwise.m_nEnableOutputCapture;
            }
            else if (g_audioImplCVars_wwise.m_nEnableOutputCapture == 0 && nEnableOutputCapture == 1)
            {
                eResult = AK::SoundEngine::StopOutputCapture();
                AZ_Assert(IS_WWISE_OK(eResult), "AK::SoundEngine::StopOutputCapture failed!");
                nEnableOutputCapture = g_audioImplCVars_wwise.m_nEnableOutputCapture;
            }

            if (audioDeviceInitializationEvent)
            {
                AkChannelConfig channelConfig = AK::SoundEngine::GetSpeakerConfiguration();
                int surroundSpeakers = channelConfig.uNumChannels;
                int lfeSpeakers = 0;
                if (AK::HasLFE(channelConfig.uChannelMask))
                {
                    --surroundSpeakers;
                    ++lfeSpeakers;
                }
                m_speakerConfigString = AZStd::string::format("Output: %d.%d", surroundSpeakers, lfeSpeakers);
                m_fullImplString = AZStd::string::format("%s (%s)  %s", WWISE_IMPL_VERSION_STRING, m_soundbankFolder.c_str(), m_speakerConfigString.c_str());

                audioDeviceInitializationEvent = false;
            }
    #endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

            AK::SoundEngine::RenderAudio();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::Initialize()
    {
        // If something fails so severely during initialization that we need to fall back to the NULL implementation
        // we will need to shut down what has been initialized so far. Therefore make sure to call Shutdown() before returning eARS_FAILURE!

        AkMemSettings oMemSettings;
        oMemSettings.uMaxNumPools = 20;
        AKRESULT eResult = AK::MemoryMgr::Init(&oMemSettings);

        if (!IS_WWISE_OK(eResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::MemoryMgr::Init() returned AKRESULT %d", eResult);
            ShutDown();
            return eARS_FAILURE;
        }

        const AkMemPoolId nPrepareMemPoolID = AK::MemoryMgr::CreatePool(nullptr, g_audioImplCVars_wwise.m_nPrepareEventMemoryPoolSize << 10, 16, AkMalloc, 16);

        if (nPrepareMemPoolID == AK_INVALID_POOL_ID)
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::MemoryMgr::CreatePool() PrepareEventMemoryPool failed!\n");
            ShutDown();
            return eARS_FAILURE;
        }

        eResult = AK::MemoryMgr::SetPoolName(nPrepareMemPoolID, "PrepareEventMemoryPool");

        if (eResult != AK_Success)
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::MemoryMgr::SetPoolName() could not set name of event prepare memory pool!\n");
            ShutDown();
            return eARS_FAILURE;
        }

        eResult = AK::SoundEngine::RegisterAudioDeviceStatusCallback(AudioDeviceCallback);
        if (eResult != AK_Success)
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::SoundEngine::RegisterAudioDeviceStatusCallback failed!\n");
        }

        AkStreamMgrSettings oStreamSettings;
        AK::StreamMgr::GetDefaultSettings(oStreamSettings);
        oStreamSettings.uMemorySize = g_audioImplCVars_wwise.m_nStreamManagerMemoryPoolSize << 10;

        if (AK::StreamMgr::Create(oStreamSettings) == nullptr)
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::StreamMgr::Create() failed!\n");
            ShutDown();
            return eARS_FAILURE;
        }

        eResult = m_oFileIOHandler.Init(g_audioImplCVars_wwise.m_nStreamDeviceMemoryPoolSize << 10);

        if (!IS_WWISE_OK(eResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "m_oFileIOHandler.Init() returned AKRESULT %d", eResult);
            ShutDown();
            return eARS_FAILURE;
        }

        const AkOSChar* akSoundbankPath = nullptr;
        CONVERT_CHAR_TO_OSCHAR(m_soundbankFolder.c_str(), akSoundbankPath);
        m_oFileIOHandler.SetBankPath(akSoundbankPath);

        AkInitSettings oInitSettings;
        AK::SoundEngine::GetDefaultInitSettings(oInitSettings);
        oInitSettings.uDefaultPoolSize = g_audioImplCVars_wwise.m_nSoundEngineDefaultMemoryPoolSize << 10;
        oInitSettings.uCommandQueueSize = g_audioImplCVars_wwise.m_nCommandQueueMemoryPoolSize << 10;
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        oInitSettings.uMonitorPoolSize = g_audioImplCVars_wwise.m_nMonitorMemoryPoolSize << 10;
        oInitSettings.uMonitorQueuePoolSize = g_audioImplCVars_wwise.m_nMonitorQueueMemoryPoolSize << 10;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
        oInitSettings.uPrepareEventMemoryPoolID = nPrepareMemPoolID;
        oInitSettings.bEnableGameSyncPreparation = false;//TODO: ???

        AkPlatformInitSettings oPlatformInitSettings;
        AK::SoundEngine::GetDefaultPlatformInitSettings(oPlatformInitSettings);
        oPlatformInitSettings.uLEngineDefaultPoolSize = g_audioImplCVars_wwise.m_nLowerEngineDefaultPoolSize << 10;

        Platform::SetupAkSoundEngine(oPlatformInitSettings);

        eResult = AK::SoundEngine::Init(&oInitSettings, &oPlatformInitSettings);

        if (!IS_WWISE_OK(eResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::SoundEngine::Init() returned AKRESULT %d", eResult);
            ShutDown();
            return eARS_FAILURE;
        }

        AkMusicSettings oMusicInit;
        AK::MusicEngine::GetDefaultInitSettings(oMusicInit);

        eResult = AK::MusicEngine::Init(&oMusicInit);

        if (!IS_WWISE_OK(eResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::MusicEngine::Init() returned AKRESULT %d", eResult);
            ShutDown();
            return eARS_FAILURE;
        }

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        if (g_audioImplCVars_wwise.m_nEnableCommSystem == 1)
        {
            m_bCommSystemInitialized = true;
            AkCommSettings oCommSettings;
            AK::Comm::GetDefaultInitSettings(oCommSettings);

            eResult = AK::Comm::Init(oCommSettings);

            if (!IS_WWISE_OK(eResult))
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::Comm::Init() returned AKRESULT %d. Communication between the Wwise authoring application and the game will not be possible\n", eResult);
                m_bCommSystemInitialized = false;
            }

            eResult = AK::Monitor::SetLocalOutput(AK::Monitor::ErrorLevel_All, ErrorMonitorCallback);

            if (!IS_WWISE_OK(eResult))
            {
                AK::Comm::Term();
                g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", eResult);
                m_bCommSystemInitialized = false;
            }
        }
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

        // Initialize the AudioSourceManager
        AudioSourceManager::Get().Initialize();

        // Register the DummyGameObject used for the events that don't need a location in the game world
        eResult = AK::SoundEngine::RegisterGameObj(m_globalGameObjectID, sWwiseGlobalAudioObjectName);

        if (!IS_WWISE_OK(eResult))
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::SoundEngine::RegisterGameObject() failed for '%s' with AKRESULT %d", sWwiseGlobalAudioObjectName, eResult);
        }

        // Load Init.bnk before making the system available to the users
        eResult = AK::SoundEngine::LoadBank(AKTEXT("init.bnk"), AK_DEFAULT_POOL_ID, m_nInitBankID);

        if (!IS_WWISE_OK(eResult))
        {
            // This does not qualify for a fallback to the NULL implementation!
            // Still notify the user about this failure!
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to load Init.bnk, returned the AKRESULT: %d", eResult);
            m_nInitBankID = AK_INVALID_BANK_ID;
        }

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        // Set up memory pool information...
        AkInt32 numPools = AK::MemoryMgr::GetNumPools();
        m_debugMemoryPoolInfo.reserve(numPools);

        g_audioImplLogger_wwise.Log(eALT_COMMENT, "Number of AK Memory Pools: %d", numPools);

        for (AkInt32 poolId = 0; poolId < numPools; ++poolId)
        {
            eResult = AK::MemoryMgr::CheckPoolId(poolId);
            if (IS_WWISE_OK(eResult))
            {
                AudioImplMemoryPoolInfo poolInfo;

                AkOSChar* poolName = AK::MemoryMgr::GetPoolName(poolId);
                char* nameTemp = nullptr;
                CONVERT_OSCHAR_TO_CHAR(poolName, nameTemp);
                AKPLATFORM::SafeStrCpy(poolInfo.m_poolName, nameTemp, AZ_ARRAY_SIZE(poolInfo.m_poolName));
                poolInfo.m_poolId = poolId;

                g_audioImplLogger_wwise.Log(eALT_COMMENT, "Found Wwise Memory Pool: %d - '%s'", poolId, poolInfo.m_poolName);

                m_debugMemoryPoolInfo.push_back(poolInfo);
            }
        }
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ShutDown()
    {
        AKRESULT eResult = AK_Fail;

#if !defined(WWISE_FOR_RELEASE)
        if (m_bCommSystemInitialized)
        {
            AK::Comm::Term();

            eResult = AK::Monitor::SetLocalOutput(0, nullptr);

            if (!IS_WWISE_OK(eResult))
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", eResult);
            }

            m_bCommSystemInitialized = false;
        }
#endif // !WWISE_FOR_RELEASE

        eResult = AK::SoundEngine::UnregisterAudioDeviceStatusCallback();
        if (eResult != AK_Success)
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::SoundEngine::UnregisterAudioDeviceStatusCallback failed!\n");
        }

        // Shutdown the AudioSourceManager
        AudioSourceManager::Get().Shutdown();

        AK::MusicEngine::Term();

        if (AK::SoundEngine::IsInitialized())
        {
            // UnRegister the DummyGameObject
            eResult = AK::SoundEngine::UnregisterGameObj(m_globalGameObjectID);

            if (!IS_WWISE_OK(eResult))
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::SoundEngine::UnregisterGameObject() failed for '%s' with AKRESULT %d", sWwiseGlobalAudioObjectName, eResult);
            }

            eResult = AK::SoundEngine::ClearBanks();

            if (!IS_WWISE_OK(eResult))
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "Failed to clear sound banks!");
            }

            AK::SoundEngine::Term();
        }

        // Terminate the streaming device and streaming manager
        // CAkFilePackageLowLevelIOBlocking::Term() destroys its associated streaming device
        // that lives in the Stream Manager, and unregisters itself as the File Location Resolver.
        if (AK::IAkStreamMgr::Get())
        {
            m_oFileIOHandler.ShutDown();
            AK::IAkStreamMgr::Get()->Destroy();
        }

        // Terminate the Memory Manager
        if (AK::MemoryMgr::IsInitialized())
        {
            eResult = AK::MemoryMgr::DestroyPool(0);
            AK::MemoryMgr::Term();
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::Release()
    {
        // Deleting this object and destroying the allocator has been moved to AudioEngineWwiseSystemComponent
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::StopAllSounds()
    {
        AK::SoundEngine::StopAll();
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::RegisterAudioObject(
        IATLAudioObjectData* const pObjectData,
        const char* const sObjectName)
    {
        if (pObjectData)
        {
            auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pObjectData);

            const AKRESULT eAKResult = AK::SoundEngine::RegisterGameObj(pAKObjectData->nAKID, sObjectName);

            const bool bAKSuccess = IS_WWISE_OK(eAKResult);

            if (!bAKSuccess)
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise RegisterGameObj failed with AKRESULT: %d", eAKResult);
            }

            return BoolToARS(bAKSuccess);
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise RegisterGameObj failed, pObjectData was null");
            return eARS_FAILURE;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UnregisterAudioObject(IATLAudioObjectData* const pObjectData)
    {
        if (pObjectData)
        {
            auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pObjectData);

            const AKRESULT eAKResult = AK::SoundEngine::UnregisterGameObj(pAKObjectData->nAKID);

            const bool bAKSuccess = IS_WWISE_OK(eAKResult);

            if (!bAKSuccess)
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise UnregisterGameObj failed with AKRESULT: %d", eAKResult);
            }

            return BoolToARS(bAKSuccess);
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise UnregisterGameObj failed, pObjectData was null");
            return eARS_FAILURE;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ResetAudioObject(IATLAudioObjectData* const pObjectData)
    {
        if (pObjectData)
        {
            auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pObjectData);

            pAKObjectData->cEnvironmentImplAmounts.clear();
            pAKObjectData->bNeedsToUpdateEnvironments = false;

            return eARS_SUCCESS;
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "Resetting Audio object failed, pObjectData was null");
            return eARS_FAILURE;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UpdateAudioObject(IATLAudioObjectData* const pObjectData)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        if (pObjectData)
        {
            auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pObjectData);

            if (pAKObjectData->bNeedsToUpdateEnvironments)
            {
                eResult = PostEnvironmentAmounts(pAKObjectData);
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PrepareTriggerSync(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLTriggerImplData* const pTriggerData)
    {
        return PrepUnprepTriggerSync(pTriggerData, true);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UnprepareTriggerSync(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLTriggerImplData* const pTriggerData)
    {
        return PrepUnprepTriggerSync(pTriggerData, false);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PrepareTriggerAsync(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLTriggerImplData* const pTriggerData,
        IATLEventData* const pEventData)
    {
        return PrepUnprepTriggerAsync(pTriggerData, pEventData, true);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UnprepareTriggerAsync(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLTriggerImplData* const pTriggerData,
        IATLEventData* const pEventData)
    {
        return PrepUnprepTriggerAsync(pTriggerData, pEventData, false);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ActivateTrigger(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLTriggerImplData* const pTriggerData,
        IATLEventData* const pEventData,
        const SATLSourceData* const pSourceData)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);
        auto const pAKTriggerImplData = static_cast<const SATLTriggerImplData_wwise*>(pTriggerData);
        auto const pAKEventData = static_cast<SATLEventData_wwise*>(pEventData);

        if (pAKObjectData && pAKTriggerImplData && pAKEventData)
        {
            AkGameObjectID nAKObjectID = AK_INVALID_GAME_OBJECT;

            if (pAKObjectData->bHasPosition)
            {
                nAKObjectID = pAKObjectData->nAKID;
                PostEnvironmentAmounts(pAKObjectData);
            }
            else
            {
                nAKObjectID = m_globalGameObjectID;
            }

            AkPlayingID nAKPlayingID = AK_INVALID_PLAYING_ID;
            switch (GetAssetType(pSourceData))
            {
                case eAAT_SOURCE:
                {
                    AZ_Assert(pSourceData, "SourceData not provided for source type!");
                    auto sTemp = AZStd::string::format("%s%d/%d/%d.wem",
                        WWISE_IMPL_EXTERNAL_PATH,
                        pSourceData->m_sourceInfo.m_collectionId,
                        pSourceData->m_sourceInfo.m_languageId,
                        pSourceData->m_sourceInfo.m_fileId);

                    AkOSChar* pTemp = nullptr;
                    CONVERT_CHAR_TO_OSCHAR(sTemp.c_str(), pTemp);

                    AkExternalSourceInfo sources[1];
                    sources[0].iExternalSrcCookie = static_cast<AkUInt32>(pSourceData->m_sourceInfo.m_sourceId);
                    sources[0].szFile = pTemp;
                    sources[0].idCodec = GetAkCodecID(pSourceData->m_sourceInfo.m_codecType);

                    nAKPlayingID = AK::SoundEngine::PostEvent(
                        pAKTriggerImplData->nAKID,
                        nAKObjectID,
                        AK_EndOfEvent | AK_Duration,
                        &WwiseEventCallback,
                        pAKEventData,
                        1,
                        sources);

                    if (nAKPlayingID != AK_INVALID_PLAYING_ID)
                    {
                        pAKEventData->audioEventState = eAES_PLAYING;
                        pAKEventData->nAKID = nAKPlayingID;
                        eResult = eARS_SUCCESS;
                    }
                    else
                    {
                        // if Posting an Event failed, try to prepare it, if it isn't prepared already
                        g_audioImplLogger_wwise.Log(eALT_WARNING, "Failed to Post Wwise event %u with external source '%s'", pAKTriggerImplData->nAKID, sTemp.c_str());
                    }
                    break;
                }

                case eAAT_STREAM:
                    //[[fallthrough]]
                case eAAT_NONE:
                    //[[fallthrough]]
                default:
                {
                    nAKPlayingID = AK::SoundEngine::PostEvent(
                        pAKTriggerImplData->nAKID,
                        nAKObjectID,
                        AK_EndOfEvent | AK_Duration,
                        &WwiseEventCallback,
                        pAKEventData);

                    if (nAKPlayingID != AK_INVALID_PLAYING_ID)
                    {
                        if (pSourceData)
                        {
                            TAudioSourceId sourceId = pSourceData->m_sourceInfo.m_sourceId;
                            if (sourceId != INVALID_AUDIO_SOURCE_ID)
                            {
                                // Activate the audio input source (associates sourceId w/ playingId)...
                                AudioSourceManager::Get().ActivateSource(sourceId, nAKPlayingID);
                                pAKEventData->nSourceId = sourceId;
                            }
                        }

                        pAKEventData->audioEventState = eAES_PLAYING;
                        pAKEventData->nAKID = nAKPlayingID;
                        eResult = eARS_SUCCESS;
                    }
                    else
                    {
                        // if Posting an Event failed, try to prepare it, if it isn't prepared already
                        g_audioImplLogger_wwise.Log(eALT_WARNING, "Failed to Post Wwise event %u", pAKTriggerImplData->nAKID);
                    }
                    break;
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData, ATLTriggerData or EventData passed to the Wwise implementation of ActivateTrigger.");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::StopEvent(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLEventData* const pEventData)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKEventData = static_cast<const SATLEventData_wwise*>(pEventData);

        if (pAKEventData)
        {
            switch (pAKEventData->audioEventState)
            {
                case eAES_PLAYING:
                {
                    AK::SoundEngine::StopPlayingID(pAKEventData->nAKID, 10);
                    eResult = eARS_SUCCESS;
                    break;
                }
                default:
                {
                    g_audioImplLogger_wwise.Log(eALT_ERROR, "Stopping an event of this type is not supported yet");
                    break;
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid EventData passed to the Wwise implementation of StopEvent.");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::StopAllEvents(IATLAudioObjectData* const pAudioObjectData)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);

        if (pAKObjectData)
        {
            const AkGameObjectID nAKObjectID = pAKObjectData->bHasPosition ? pAKObjectData->nAKID : m_globalGameObjectID;

            AK::SoundEngine::StopAll(nAKObjectID);

            eResult = eARS_SUCCESS;
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of StopAllEvents.");
        }
        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetPosition(
        IATLAudioObjectData* const pAudioObjectData,
        const SATLWorldPosition& sWorldPosition)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);

        if (pAKObjectData)
        {
            AkSoundPosition sAkSoundPos;
            ATLTransformToAkTransform(sWorldPosition, sAkSoundPos);

            const AKRESULT eAKResult = AK::SoundEngine::SetPosition(pAKObjectData->nAKID, sAkSoundPos);
            if (IS_WWISE_OK(eAKResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise SetPosition failed with AKRESULT: %d", eAKResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of SetPosition.");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetMultiplePositions(
        IATLAudioObjectData* const pAudioObjectData,
        const MultiPositionParams& multiPositionParams)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);

        if (pAKObjectData)
        {
            AZStd::vector<AkSoundPosition> akPositions;
            AZStd::for_each(multiPositionParams.m_positions.begin(), multiPositionParams.m_positions.end(),
                [&akPositions](const auto& position)
                {
                    akPositions.emplace_back(AZVec3ToAkTransform(position));
                }
            );

            AK::SoundEngine::MultiPositionType type = AK::SoundEngine::MultiPositionType_MultiDirections; // default 'Blended'

            if (multiPositionParams.m_type == MultiPositionBehaviorType::Separate)
            {
                type = AK::SoundEngine::MultiPositionType_MultiSources;
            }

            const AKRESULT akResult = AK::SoundEngine::SetMultiplePositions(pAKObjectData->nAKID, akPositions.data(), static_cast<AkUInt16>(akPositions.size()), type);
            if (IS_WWISE_OK(akResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise SetMultiplePositions failed with AKRESULT: %d\n", akResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of SetMultiplePositions.");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetEnvironment(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLEnvironmentImplData* const pEnvironmentImplData,
        const float fAmount)
    {
        static const float sEnvEpsilon = 0.0001f;

        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);
        auto const pAKEnvironmentData = static_cast<const SATLEnvironmentImplData_wwise*>(pEnvironmentImplData);

        if (pAKObjectData && pAKEnvironmentData)
        {
            switch (pAKEnvironmentData->eType)
            {
                case eWAET_AUX_BUS:
                {
                    float fCurrentAmount = -1.f;
                    auto it = pAKObjectData->cEnvironmentImplAmounts.find(pAKEnvironmentData->nAKBusID);
                    if (it != pAKObjectData->cEnvironmentImplAmounts.end())
                    {
                        fCurrentAmount = it->second;
                    }

                    if (fCurrentAmount == -1.f || !AZ::IsClose(fCurrentAmount, fAmount, sEnvEpsilon))
                    {
                        pAKObjectData->cEnvironmentImplAmounts[pAKEnvironmentData->nAKBusID] = fAmount;
                        pAKObjectData->bNeedsToUpdateEnvironments = true;
                    }

                    eResult = eARS_SUCCESS;
                    break;
                }
                case eWAET_RTPC:
                {
                    auto fAKValue = static_cast<AkRtpcValue>(pAKEnvironmentData->fMult * fAmount + pAKEnvironmentData->fShift);

                    const AKRESULT eAKResult = AK::SoundEngine::SetRTPCValue(pAKEnvironmentData->nAKRtpcID, fAKValue, pAKObjectData->nAKID);

                    if (IS_WWISE_OK(eAKResult))
                    {
                        eResult = eARS_SUCCESS;
                    }
                    else
                    {
                        g_audioImplLogger_wwise.Log(
                            eALT_WARNING,
                            "Wwise failed to set the Rtpc %u to value %f on object %u in SetEnvironement()",
                            pAKEnvironmentData->nAKRtpcID,
                            fAKValue,
                            pAKObjectData->nAKID);
                    }
                    break;
                }
                default:
                {
                    AZ_Assert(false, "<Wwise> Unknown AudioEnvironmentImplementation type!");
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData or EnvironmentData passed to the Wwise implementation of SetEnvironment");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetRtpc(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLRtpcImplData* const pRtpcData,
        const float fValue)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);
        auto const pAKRtpcData = static_cast<const SATLRtpcImplData_wwise*>(pRtpcData);

        if (pAKObjectData && pAKRtpcData)
        {
            auto fAKValue = static_cast<AkRtpcValue>(pAKRtpcData->m_fMult * fValue + pAKRtpcData->m_fShift);

            const AKRESULT eAKResult = AK::SoundEngine::SetRTPCValue(pAKRtpcData->nAKID, fAKValue, pAKObjectData->nAKID);

            if (IS_WWISE_OK(eAKResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise failed to set the Rtpc %llu to value %f on object %llu",
                    pAKRtpcData->nAKID,
                    static_cast<AkRtpcValue>(fValue),
                    pAKObjectData->nAKID);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData or RtpcData passed to the Wwise implementation of SetRtpc");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetSwitchState(
        IATLAudioObjectData* const pAudioObjectData,
        const IATLSwitchStateImplData* const pSwitchStateData)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);
        auto const pAKSwitchStateData = static_cast<const SATLSwitchStateImplData_wwise*>(pSwitchStateData);

        if (pAKObjectData && pAKSwitchStateData)
        {
            switch (pAKSwitchStateData->eType)
            {
                case eWST_SWITCH:
                {
                    const AkGameObjectID nAKObjectID = pAKObjectData->bHasPosition ? pAKObjectData->nAKID : m_globalGameObjectID;

                    const AKRESULT eAKResult = AK::SoundEngine::SetSwitch(
                            pAKSwitchStateData->nAKSwitchID,
                            pAKSwitchStateData->nAKStateID,
                            nAKObjectID);

                    if (IS_WWISE_OK(eAKResult))
                    {
                        eResult = eARS_SUCCESS;
                    }
                    else
                    {
                        g_audioImplLogger_wwise.Log(
                            eALT_WARNING,
                            "Wwise failed to set the switch group %u to state %u on object %llu",
                            pAKSwitchStateData->nAKSwitchID,
                            pAKSwitchStateData->nAKStateID,
                            nAKObjectID);
                    }
                    break;
                }
                case eWST_STATE:
                {
                    const AKRESULT eAKResult = AK::SoundEngine::SetState(
                            pAKSwitchStateData->nAKSwitchID,
                            pAKSwitchStateData->nAKStateID);

                    if (IS_WWISE_OK(eAKResult))
                    {
                        eResult = eARS_SUCCESS;
                    }
                    else
                    {
                        g_audioImplLogger_wwise.Log(
                            eALT_WARNING,
                            "Wwise failed to set the state group %u to state %u",
                            pAKSwitchStateData->nAKSwitchID,
                            pAKSwitchStateData->nAKStateID);
                    }
                    break;
                }
                case eWST_RTPC:
                {
                    const AkGameObjectID nAKObjectID = pAKObjectData->nAKID;

                    const AKRESULT eAKResult = AK::SoundEngine::SetRTPCValue(
                            pAKSwitchStateData->nAKSwitchID,
                            static_cast<AkRtpcValue>(pAKSwitchStateData->fRtpcValue),
                            nAKObjectID);

                    if (IS_WWISE_OK(eAKResult))
                    {
                        eResult = eARS_SUCCESS;
                    }
                    else
                    {
                        g_audioImplLogger_wwise.Log(
                            eALT_WARNING,
                            "Wwise failed to set the Rtpc %u to value %f on object %llu",
                            pAKSwitchStateData->nAKSwitchID,
                            static_cast<AkRtpcValue>(pAKSwitchStateData->fRtpcValue),
                            nAKObjectID);
                    }
                    break;
                }
                case eWST_NONE:
                {
                    break;
                }
                default:
                {
                    g_audioImplLogger_wwise.Log(eALT_WARNING, "Unknown EWwiseSwitchType: %u", pAKSwitchStateData->eType);
                    AZ_Assert(false, "<Wwise> Unknown EWwiseSwitchType");
                    break;
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData or RtpcData passed to the Wwise implementation of SetRtpc");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetObstructionOcclusion(
        IATLAudioObjectData* const pAudioObjectData,
        const float fObstruction,
        const float fOcclusion)
    {
        if (fObstruction < sObstructionOcclusionMin || fObstruction > sObstructionOcclusionMax)
        {
            g_audioImplLogger_wwise.Log(
                eALT_WARNING,
                "Obstruction value %f is out of range, Obstruction should be between %f and %f.",
                fObstruction, sObstructionOcclusionMin, sObstructionOcclusionMax);
        }

        if (fOcclusion < sObstructionOcclusionMin || fOcclusion > sObstructionOcclusionMax)
        {
            g_audioImplLogger_wwise.Log(
                eALT_WARNING,
                "Occlusion value %f is out of range, Occlusion should be between %f and %f.",
                fOcclusion, sObstructionOcclusionMin, sObstructionOcclusionMax);
        }

        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);

        if (pAKObjectData)
        {
            const AKRESULT eAKResult = AK::SoundEngine::SetObjectObstructionAndOcclusion(
                    pAKObjectData->nAKID,
                    m_defaultListenerGameObjectID,  // only set the obstruction/occlusion for the default listener for now
                    static_cast<AkReal32>(fObstruction),
                    static_cast<AkReal32>(fOcclusion));

            if (IS_WWISE_OK(eAKResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise failed to set Obstruction %f and Occlusion %f on object %llu",
                    fObstruction,
                    fOcclusion,
                    pAKObjectData->nAKID);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of SetObjectObstructionAndOcclusion");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetListenerPosition(
        IATLListenerData* const pListenerData,
        const SATLWorldPosition& oNewPosition)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKListenerData = static_cast<SATLListenerData_wwise*>(pListenerData);

        if (pAKListenerData)
        {
            AkListenerPosition oAKListenerPos;
            ATLTransformToAkTransform(oNewPosition, oAKListenerPos);

            const AKRESULT eAKResult = AK::SoundEngine::SetPosition(pAKListenerData->nAKListenerObjectId, oAKListenerPos);

            if (IS_WWISE_OK(eAKResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise SetListenerPosition failed with AKRESULT: %u", eAKResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid ATLListenerData passed to the Wwise implementation of SetListenerPosition");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ResetRtpc(IATLAudioObjectData* const pAudioObjectData, const IATLRtpcImplData* const pRtpcData)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);
        auto const pAKRtpcData = static_cast<const SATLRtpcImplData_wwise*>(pRtpcData);

        if (pAKObjectData && pAKRtpcData)
        {
            const AKRESULT eAKResult = AK::SoundEngine::ResetRTPCValue(pAKRtpcData->nAKID, pAKObjectData->nAKID);

            if (IS_WWISE_OK(eAKResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise failed to reset the Rtpc %u on object %llu",
                    pAKRtpcData->nAKID,
                    pAKObjectData->nAKID);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData or RtpcData passed to the Wwise implementation of ResetRtpc");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::RegisterInMemoryFile(SATLAudioFileEntryInfo* const pFileEntryInfo)
    {
        EAudioRequestStatus eRegisterResult = eARS_FAILURE;

        if (pFileEntryInfo)
        {
            auto const pFileDataWwise = static_cast<SATLAudioFileEntryData_wwise*>(pFileEntryInfo->pImplData);

            if (pFileDataWwise)
            {
                AkBankID nBankID = AK_INVALID_BANK_ID;

                const AKRESULT eAKResult = AK::SoundEngine::LoadBank(
                        pFileEntryInfo->pFileData,
                        static_cast<AkUInt32>(pFileEntryInfo->nSize),
                        nBankID);

                if (IS_WWISE_OK(eAKResult))
                {
                    pFileDataWwise->nAKBankID = nBankID;
                    eRegisterResult = eARS_SUCCESS;
                }
                else
                {
                    pFileDataWwise->nAKBankID = AK_INVALID_BANK_ID;
                    g_audioImplLogger_wwise.Log(eALT_ERROR, "Failed to load file %s\n", pFileEntryInfo->sFileName);
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioFileEntryData passed to the Wwise implementation of RegisterInMemoryFile");
            }
        }

        return eRegisterResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UnregisterInMemoryFile(SATLAudioFileEntryInfo* const pFileEntryInfo)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        if (pFileEntryInfo)
        {
            auto const pFileDataWwise = static_cast<SATLAudioFileEntryData_wwise*>(pFileEntryInfo->pImplData);

            if (pFileDataWwise)
            {
                const AKRESULT eAKResult = AK::SoundEngine::UnloadBank(pFileDataWwise->nAKBankID, pFileEntryInfo->pFileData);

                if (IS_WWISE_OK(eAKResult))
                {
                    eResult = eARS_SUCCESS;
                }
                else
                {
                    g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise Failed to unregister in memory file %s\n", pFileEntryInfo->sFileName);
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioFileEntryData passed to the Wwise implementation of UnregisterInMemoryFile");
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ParseAudioFileEntry(const AZ::rapidxml::xml_node<char>* audioFileEntryNode, SATLAudioFileEntryInfo* const fileEntryInfo)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        if (audioFileEntryNode && azstricmp(audioFileEntryNode->name(), WwiseXmlTags::WwiseFileTag) == 0 && fileEntryInfo)
        {
            const char* audioFileEntryName = nullptr;
            auto fileEntryNameAttr = audioFileEntryNode->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (fileEntryNameAttr)
            {
                audioFileEntryName = fileEntryNameAttr->value();
            }

            bool isLocalized = false;
            auto localizedAttr = audioFileEntryNode->first_attribute(WwiseXmlTags::WwiseLocalizedAttribute, 0, false);
            if (localizedAttr)
            {
                if (azstricmp(localizedAttr->value(), "true") == 0)
                {
                    isLocalized = true;
                }
            }

            if (audioFileEntryName && audioFileEntryName[0] != '\0')
            {
                fileEntryInfo->bLocalized = isLocalized;
                fileEntryInfo->sFileName = audioFileEntryName;
                fileEntryInfo->nMemoryBlockAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;
                fileEntryInfo->pImplData = azcreate(SATLAudioFileEntryData_wwise, (), Audio::AudioImplAllocator, "ATLAudioFileEntryData_wwise");
                result = eARS_SUCCESS;
            }
            else
            {
                fileEntryInfo->sFileName = nullptr;
                fileEntryInfo->nMemoryBlockAlignment = 0;
                fileEntryInfo->pImplData = nullptr;
            }
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioFileEntryData(IATLAudioFileEntryData* const oldAudioFileEntry)
    {
        azdestroy(oldAudioFileEntry, Audio::AudioImplAllocator, SATLAudioFileEntryData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_wwise::GetAudioFileLocation(SATLAudioFileEntryInfo* const pFileEntryInfo)
    {
        const char* sResult = nullptr;

        if (pFileEntryInfo)
        {
            sResult = pFileEntryInfo->bLocalized ? m_localizedSoundbankFolder.c_str() : m_soundbankFolder.c_str();
        }

        return sResult;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLAudioObjectData_wwise* CAudioSystemImpl_wwise::NewGlobalAudioObjectData(const TAudioObjectID nObjectID)
    {
        AZ_UNUSED(nObjectID);
        auto pNewObjectData = azcreate(SATLAudioObjectData_wwise, (AK_INVALID_GAME_OBJECT, false), Audio::AudioImplAllocator, "ATLAudioObjectData_wwise-Global");
        return pNewObjectData;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLAudioObjectData_wwise* CAudioSystemImpl_wwise::NewAudioObjectData(const TAudioObjectID nObjectID)
    {
        auto pNewObjectData = azcreate(SATLAudioObjectData_wwise, (static_cast<AkGameObjectID>(nObjectID), true), Audio::AudioImplAllocator, "ATLAudioObjectData_wwise");
        return pNewObjectData;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioObjectData(IATLAudioObjectData* const pOldObjectData)
    {
        azdestroy(pOldObjectData, Audio::AudioImplAllocator, SATLAudioObjectData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLListenerData_wwise* CAudioSystemImpl_wwise::NewDefaultAudioListenerObjectData(const TATLIDType nListenerID)
    {
        auto pNewObject = azcreate(SATLListenerData_wwise, (static_cast<AkGameObjectID>(nListenerID)), Audio::AudioImplAllocator, "ATLListenerData_wwise-Default");
        if (pNewObject)
        {
            auto listenerName = AZStd::string::format("DefaultAudioListener(%llu)", pNewObject->nAKListenerObjectId);
            AKRESULT eAKResult = AK::SoundEngine::RegisterGameObj(pNewObject->nAKListenerObjectId, listenerName.c_str());
            if (IS_WWISE_OK(eAKResult))
            {
                eAKResult = AK::SoundEngine::SetDefaultListeners(&pNewObject->nAKListenerObjectId, 1);
                if (IS_WWISE_OK(eAKResult))
                {
                    m_defaultListenerGameObjectID = pNewObject->nAKListenerObjectId;
                }
                else
                {
                    g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in SetDefaultListeners to set AkGameObjectID %llu as default with AKRESULT: %u", pNewObject->nAKListenerObjectId, eAKResult);
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in RegisterGameObj registering a DefaultAudioListener with AKRESULT: %u", eAKResult);
            }
        }

        return pNewObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLListenerData_wwise* CAudioSystemImpl_wwise::NewAudioListenerObjectData(const TATLIDType nListenerID)
    {
        auto pNewObject = azcreate(SATLListenerData_wwise, (static_cast<AkGameObjectID>(nListenerID)), Audio::AudioImplAllocator, "ATLListenerData_wwise");
        if (pNewObject)
        {
            auto listenerName = AZStd::string::format("AudioListener(%llu)", pNewObject->nAKListenerObjectId);
            AKRESULT eAKResult = AK::SoundEngine::RegisterGameObj(pNewObject->nAKListenerObjectId, listenerName.c_str());
            if (!IS_WWISE_OK(eAKResult))
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in RegisterGameObj registering an AudioListener with AKRESULT: %u", eAKResult);
            }
        }

        return pNewObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioListenerObjectData(IATLListenerData* const pOldListenerData)
    {
        auto listenerData = static_cast<SATLListenerData_wwise*>(pOldListenerData);
        if (listenerData)
        {
            AKRESULT eAKResult = AK::SoundEngine::UnregisterGameObj(listenerData->nAKListenerObjectId);
            if (IS_WWISE_OK(eAKResult))
            {
                if (listenerData->nAKListenerObjectId == m_defaultListenerGameObjectID)
                {
                    m_defaultListenerGameObjectID = AK_INVALID_GAME_OBJECT;
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in UnregisterGameObj unregistering an AudioListener(%llu) with AKRESULT: %u", listenerData->nAKListenerObjectId, eAKResult);
            }
        }

        azdestroy(pOldListenerData, Audio::AudioImplAllocator, SATLListenerData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLEventData_wwise* CAudioSystemImpl_wwise::NewAudioEventData(const TAudioEventID nEventID)
    {
        auto pNewEvent = azcreate(SATLEventData_wwise, (nEventID), Audio::AudioImplAllocator, "ATLEventData_wwise");
        return pNewEvent;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioEventData(IATLEventData* const pOldEventData)
    {
        azdestroy(pOldEventData, Audio::AudioImplAllocator, SATLEventData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::ResetAudioEventData(IATLEventData* const pEventData)
    {
        auto const pAKEventData = static_cast<SATLEventData_wwise*>(pEventData);

        if (pAKEventData)
        {
            pAKEventData->audioEventState = eAES_NONE;
            pAKEventData->nAKID = AK_INVALID_UNIQUE_ID;
            pAKEventData->nSourceId = INVALID_AUDIO_SOURCE_ID;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLTriggerImplData* CAudioSystemImpl_wwise::NewAudioTriggerImplData(const AZ::rapidxml::xml_node<char>* audioTriggerNode)
    {
        SATLTriggerImplData_wwise* newTriggerImpl = nullptr;

        if (audioTriggerNode && azstricmp(audioTriggerNode->name(), WwiseXmlTags::WwiseEventTag) == 0)
        {
            auto eventNameAttr = audioTriggerNode->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (eventNameAttr)
            {
                const char* eventName = eventNameAttr->value();
                const AkUniqueID akId = AK::SoundEngine::GetIDFromString(eventName);

                if (akId != AK_INVALID_UNIQUE_ID)
                {
                    newTriggerImpl = azcreate(SATLTriggerImplData_wwise, (akId), Audio::AudioImplAllocator, "ATLTriggerImplData_wwise");
                }
            }
        }

        return newTriggerImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioTriggerImplData(IATLTriggerImplData* const oldTriggerImplData)
    {
        azdestroy(oldTriggerImplData, Audio::AudioImplAllocator, SATLTriggerImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLRtpcImplData* CAudioSystemImpl_wwise::NewAudioRtpcImplData(const AZ::rapidxml::xml_node<char>* audioRtpcNode)
    {
        SATLRtpcImplData_wwise* newRtpcImpl = nullptr;

        AkRtpcID akRtpcId = AK_INVALID_RTPC_ID;
        float mult = 1.f;
        float shift = 0.f;

        ParseRtpcImpl(audioRtpcNode, akRtpcId, mult, shift);

        if (akRtpcId != AK_INVALID_RTPC_ID)
        {
            newRtpcImpl = azcreate(SATLRtpcImplData_wwise, (akRtpcId, mult, shift), Audio::AudioImplAllocator, "ATLRtpcImplData_wwise");
        }

        return newRtpcImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioRtpcImplData(IATLRtpcImplData* const oldRtpcImplData)
    {
        azdestroy(oldRtpcImplData, Audio::AudioImplAllocator, SATLRtpcImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLSwitchStateImplData* CAudioSystemImpl_wwise::NewAudioSwitchStateImplData(const AZ::rapidxml::xml_node<char>* audioSwitchNode)
    {
        SATLSwitchStateImplData_wwise* newSwitchImpl = nullptr;
        const char* nodeName = audioSwitchNode->name();

        if (azstricmp(nodeName, WwiseXmlTags::WwiseSwitchTag) == 0)
        {
            newSwitchImpl = ParseWwiseSwitchOrState(audioSwitchNode, eWST_SWITCH);
        }
        else if (azstricmp(nodeName, WwiseXmlTags::WwiseStateTag) == 0)
        {
            newSwitchImpl = ParseWwiseSwitchOrState(audioSwitchNode, eWST_STATE);
        }
        else if (azstricmp(nodeName, WwiseXmlTags::WwiseRtpcSwitchTag) == 0)
        {
            newSwitchImpl = ParseWwiseRtpcSwitch(audioSwitchNode);
        }

        return newSwitchImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioSwitchStateImplData(IATLSwitchStateImplData* const oldSwitchStateImplData)
    {
        azdestroy(oldSwitchStateImplData, Audio::AudioImplAllocator, SATLSwitchStateImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLEnvironmentImplData* CAudioSystemImpl_wwise::NewAudioEnvironmentImplData(const AZ::rapidxml::xml_node<char>* audioEnvironmentNode)
    {
        SATLEnvironmentImplData_wwise* newEnvironmentImpl = nullptr;

        if (azstricmp(audioEnvironmentNode->name(), WwiseXmlTags::WwiseAuxBusTag) == 0)
        {
            auto auxBusNameAttr = audioEnvironmentNode->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (auxBusNameAttr)
            {
                const char* auxBusName = auxBusNameAttr->value();
                const AkUniqueID akBusId = AK::SoundEngine::GetIDFromString(auxBusName);

                if (akBusId != AK_INVALID_AUX_ID)
                {
                    newEnvironmentImpl = azcreate(SATLEnvironmentImplData_wwise, (eWAET_AUX_BUS, static_cast<AkAuxBusID>(akBusId)), Audio::AudioImplAllocator, "ATLEnvironmentImplData_wwise");
                }
            }
        }
        else if (azstricmp(audioEnvironmentNode->name(), WwiseXmlTags::WwiseRtpcTag) == 0)
        {
            AkRtpcID akRtpcId = AK_INVALID_RTPC_ID;
            float mult = 1.f;
            float shift = 0.f;
            ParseRtpcImpl(audioEnvironmentNode, akRtpcId, mult, shift);

            if (akRtpcId != AK_INVALID_RTPC_ID)
            {
                newEnvironmentImpl = azcreate(SATLEnvironmentImplData_wwise, (eWAET_RTPC, akRtpcId, mult, shift), Audio::AudioImplAllocator, "ATLEnvironmentImplData_wwise");
            }
        }

        return newEnvironmentImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioEnvironmentImplData(IATLEnvironmentImplData* const oldEnvironmentImplData)
    {
        azdestroy(oldEnvironmentImplData, Audio::AudioImplAllocator, SATLEnvironmentImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_wwise::GetImplementationNameString() const
    {
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        return m_fullImplString.c_str();
#else
        return nullptr;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::GetMemoryInfo(SAudioImplMemoryInfo& oMemoryInfo) const
    {
        oMemoryInfo.nPrimaryPoolSize = AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().Capacity();
        oMemoryInfo.nPrimaryPoolUsedSize = oMemoryInfo.nPrimaryPoolSize - AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().GetUnAllocatedMemory();
        oMemoryInfo.nPrimaryPoolAllocations = 0;

    #if AZ_TRAIT_AUDIOENGINEWWISE_PROVIDE_IMPL_SECONDARY_POOL
        oMemoryInfo.nSecondaryPoolSize = g_audioImplMemoryPoolSecondary_wwise.MemSize();
        oMemoryInfo.nSecondaryPoolUsedSize = oMemoryInfo.nSecondaryPoolSize - g_audioImplMemoryPoolSecondary_wwise.MemFree();
        oMemoryInfo.nSecondaryPoolAllocations = g_audioImplMemoryPoolSecondary_wwise.FragmentCount();
    #else
        oMemoryInfo.nSecondaryPoolSize = 0;
        oMemoryInfo.nSecondaryPoolUsedSize = 0;
        oMemoryInfo.nSecondaryPoolAllocations = 0;
    #endif // AZ_TRAIT_AUDIOENGINEWWISE_PROVIDE_IMPL_SECONDARY_POOL
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AudioImplMemoryPoolInfo> CAudioSystemImpl_wwise::GetMemoryPoolInfo()
    {
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        // Update pools...
        for (auto& poolInfo : m_debugMemoryPoolInfo)
        {
            AKRESULT akResult = AK::MemoryMgr::CheckPoolId(poolInfo.m_poolId);
            if (IS_WWISE_OK(akResult))
            {
                AK::MemoryMgr::PoolStats poolStats;
                akResult = AK::MemoryMgr::GetPoolStats(poolInfo.m_poolId, poolStats);
                if (IS_WWISE_OK(akResult))
                {
                    poolInfo.m_memoryReserved = poolStats.uReserved;
                    poolInfo.m_memoryUsed = poolStats.uUsed;
                    poolInfo.m_peakUsed = poolStats.uPeakUsed;
                    poolInfo.m_numAllocs = poolStats.uAllocs;
                    poolInfo.m_numFrees = poolStats.uFrees;
                }
            }
        }

        // return the pool infos...
        return m_debugMemoryPoolInfo;
#else
        return AZStd::vector<AudioImplMemoryPoolInfo>();
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystemImpl_wwise::CreateAudioSource(const SAudioInputConfig& sourceConfig)
    {
        return AudioSourceManager::Get().CreateSource(sourceConfig);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DestroyAudioSource(TAudioSourceId sourceId)
    {
        AudioSourceManager::Get().DestroySource(sourceId);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::SetPanningMode(PanningMode mode)
    {
        AkPanningRule panningRule;
        switch (mode)
        {
        case PanningMode::Speakers:
            panningRule = AkPanningRule_Speakers;
            break;
        case PanningMode::Headphones:
            panningRule = AkPanningRule_Headphones;
            break;
        default:
            return;
        }

        AKRESULT akResult = AK::SoundEngine::SetPanningRule(panningRule);
        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to set Panning Rule to [%s]\n", panningRule == AkPanningRule_Speakers ? "Speakers" : "Headphones");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLSwitchStateImplData_wwise* CAudioSystemImpl_wwise::ParseWwiseSwitchOrState(const AZ::rapidxml::xml_node<char>* node, EWwiseSwitchType type)
    {
        SATLSwitchStateImplData_wwise* switchStateImpl = nullptr;

        auto switchNameAttr = node->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
        if (switchNameAttr)
        {
            const char* switchName = switchNameAttr->value();

            auto valueNode = node->first_node(WwiseXmlTags::WwiseValueTag, 0, false);
            if (valueNode)
            {
                auto valueNameAttr = valueNode->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
                if (valueNameAttr)
                {
                    const char* stateName = valueNameAttr->value();

                    const AkUniqueID akSGroupId = AK::SoundEngine::GetIDFromString(switchName);
                    const AkUniqueID akSNameId = AK::SoundEngine::GetIDFromString(stateName);

                    if (akSGroupId != AK_INVALID_UNIQUE_ID && akSNameId != AK_INVALID_UNIQUE_ID)
                    {
                        switchStateImpl = azcreate(SATLSwitchStateImplData_wwise, (type, akSGroupId, akSNameId), Audio::AudioImplAllocator, "ATLSwitchStateImplData_wwise");
                    }
                }
            }
        }

        return switchStateImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLSwitchStateImplData_wwise* CAudioSystemImpl_wwise::ParseWwiseRtpcSwitch(const AZ::rapidxml::xml_node<char>* node)
    {
        SATLSwitchStateImplData_wwise* switchStateImpl = nullptr;

        if (node && azstricmp(node->name(), WwiseXmlTags::WwiseRtpcSwitchTag) == 0)
        {
            auto rtpcNameAttr = node->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (rtpcNameAttr)
            {
                const char* rtpcName = rtpcNameAttr->value();
                float rtpcValue = 0.f;

                auto rtpcValueAttr = node->first_attribute(WwiseXmlTags::WwiseValueAttribute, 0, false);
                if (rtpcValueAttr)
                {
                    rtpcValue = AZStd::stof(AZStd::string(rtpcValueAttr->value()));

                    const AkUniqueID akRtpcId = AK::SoundEngine::GetIDFromString(rtpcName);
                    if (akRtpcId != AK_INVALID_RTPC_ID)
                    {
                        switchStateImpl = azcreate(SATLSwitchStateImplData_wwise, (eWST_RTPC, akRtpcId, akRtpcId, rtpcValue), Audio::AudioImplAllocator, "ATLSwitchStateImplData_wwise");
                    }
                }
            }
        }

        return switchStateImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::ParseRtpcImpl(const AZ::rapidxml::xml_node<char>* node, AkRtpcID& rAkRtpcId, float& rMult, float& rShift)
    {
        SATLRtpcImplData_wwise* newRtpcImpl = nullptr;

        if (node && azstricmp(node->name(), WwiseXmlTags::WwiseRtpcTag) == 0)
        {
            auto rtpcAttr = node->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (rtpcAttr)
            {
                const char* rtpcName = rtpcAttr->value();
                rAkRtpcId = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(rtpcName));

                if (rAkRtpcId != AK_INVALID_RTPC_ID)
                {
                    auto multAttr = node->first_attribute(WwiseXmlTags::WwiseMutiplierAttribute, 0, false);
                    if (multAttr)
                    {
                        rMult = AZStd::stof(AZStd::string(multAttr->value()));
                    }

                    auto shiftAttr = node->first_attribute(WwiseXmlTags::WwiseShiftAttribute, 0, false);
                    if (shiftAttr)
                    {
                        rShift = AZStd::stof(AZStd::string(shiftAttr->value()));
                    }
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PrepUnprepTriggerSync(
        const IATLTriggerImplData* const pTriggerData,
        bool bPrepare)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKTriggerImplData = static_cast<const SATLTriggerImplData_wwise*>(pTriggerData);

        if (pAKTriggerImplData)
        {
            AkUniqueID nImplAKID = pAKTriggerImplData->nAKID;

            const AKRESULT eAKResult = AK::SoundEngine::PrepareEvent(
                    bPrepare ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
                    &nImplAKID,
                    1);

            if (IS_WWISE_OK(eAKResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise PrepareEvent with %s failed for Wwise event %u with AKRESULT: %u",
                    bPrepare ? "Preparation_Load" : "Preparation_Unload",
                    nImplAKID,
                    eAKResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR,
                "Invalid ATLTriggerData or EventData passed to the Wwise implementation of %sTriggerSync",
                bPrepare ? "Prepare" : "Unprepare");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PrepUnprepTriggerAsync(
        const IATLTriggerImplData* const pTriggerData,
        IATLEventData* const pEventData,
        bool bPrepare)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        auto const pAKTriggerImplData = static_cast<const SATLTriggerImplData_wwise*>(pTriggerData);
        auto const pAKEventData = static_cast<SATLEventData_wwise*>(pEventData);

        if (pAKTriggerImplData && pAKEventData)
        {
            AkUniqueID nImplAKID = pAKTriggerImplData->nAKID;

            const AKRESULT eAKResult = AK::SoundEngine::PrepareEvent(
                    bPrepare ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
                    &nImplAKID,
                    1,
                    &PrepareEventCallback,
                    pAKEventData);

            if (IS_WWISE_OK(eAKResult))
            {
                pAKEventData->nAKID = nImplAKID;
                pAKEventData->audioEventState = eAES_UNLOADING;

                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise PrepareEvent with %s failed for Wwise event %u with AKRESULT: %u",
                    bPrepare ? "Preparation_Load" : "Preparation_Unload",
                    nImplAKID,
                    eAKResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR,
                "Invalid ATLTriggerData or EventData passed to the Wwise implementation of %sTriggerAsync",
                bPrepare ? "Prepare" : "Unprepare");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystemImpl_wwise::SEnvPairCompare::operator()(const AZStd::pair<const AkAuxBusID, float>& pair1, const AZStd::pair<const AkAuxBusID, float>& pair2) const
    {
        return (pair1.second > pair2.second);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PostEnvironmentAmounts(IATLAudioObjectData* const pAudioObjectData)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;
        auto const pAKObjectData = static_cast<SATLAudioObjectData_wwise*>(pAudioObjectData);

        if (pAKObjectData)
        {
            AkAuxSendValue aAuxValues[LY_MAX_AUX_PER_OBJ];
            AZ::u32 nAuxCount = 0;

            SATLAudioObjectData_wwise::TEnvironmentImplMap::iterator iEnvPair = pAKObjectData->cEnvironmentImplAmounts.begin();
            const SATLAudioObjectData_wwise::TEnvironmentImplMap::const_iterator iEnvStart = pAKObjectData->cEnvironmentImplAmounts.begin();
            const SATLAudioObjectData_wwise::TEnvironmentImplMap::const_iterator iEnvEnd = pAKObjectData->cEnvironmentImplAmounts.end();

            if (pAKObjectData->cEnvironmentImplAmounts.size() <= LY_MAX_AUX_PER_OBJ)
            {
                for (; iEnvPair != iEnvEnd; ++nAuxCount)
                {
                    const float fAmount = iEnvPair->second;

                    aAuxValues[nAuxCount].auxBusID = iEnvPair->first;
                    aAuxValues[nAuxCount].fControlValue = fAmount;
                    aAuxValues[nAuxCount].listenerID = m_defaultListenerGameObjectID;  // TODO: Expand api to allow specify listeners

                    // If an amount is zero, we still want to send it to the middleware, but we also want to remove it from the map.
                    if (fAmount == 0.0f)
                    {
                        iEnvPair = pAKObjectData->cEnvironmentImplAmounts.erase(iEnvPair);
                    }
                    else
                    {
                        ++iEnvPair;
                    }
                }
            }
            else
            {
                // sort the environments in order of decreasing amounts and take the first LY_MAX_AUX_PER_OBJ worth
                using TEnvPairSet = AZStd::set<SATLAudioObjectData_wwise::TEnvironmentImplMap::value_type, SEnvPairCompare, Audio::AudioImplStdAllocator>;
                TEnvPairSet cEnvPairs(iEnvStart, iEnvEnd);

                TEnvPairSet::const_iterator iSortedEnvPair = cEnvPairs.begin();
                const TEnvPairSet::const_iterator iSortedEnvEnd = cEnvPairs.end();

                for (; (iSortedEnvPair != iSortedEnvEnd) && (nAuxCount < LY_MAX_AUX_PER_OBJ); ++iSortedEnvPair, ++nAuxCount)
                {
                    aAuxValues[nAuxCount].auxBusID = iSortedEnvPair->first;
                    aAuxValues[nAuxCount].fControlValue = iSortedEnvPair->second;
                    aAuxValues[nAuxCount].listenerID = m_defaultListenerGameObjectID;      // TODO: Expand api to allow specify listeners
                }

                // remove all Environments with 0.0 amounts
                while (iEnvPair != iEnvEnd)
                {
                    if (iEnvPair->second == 0.0f)
                    {
                        iEnvPair = pAKObjectData->cEnvironmentImplAmounts.erase(iEnvPair);
                    }
                    else
                    {
                        ++iEnvPair;
                    }
                }
            }

            AZ_Assert(nAuxCount <= LY_MAX_AUX_PER_OBJ, "WwiseImpl PostEnvironmentAmounts - Exceeded the allowed number of aux environments that can be set!");

            const AKRESULT eAKResult = AK::SoundEngine::SetGameObjectAuxSendValues(pAKObjectData->nAKID, aAuxValues, nAuxCount);

            if (IS_WWISE_OK(eAKResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING,
                    "Wwise SetGameObjectAuxSendValues failed on object %llu with AKRESULT: %u",
                    pAKObjectData->nAKID,
                    eAKResult);
            }

            pAKObjectData->bNeedsToUpdateEnvironments = false;
        }

        return eResult;
    }

    //////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_wwise::GetImplSubPath() const
    {
        return sWwiseImplSubPath;
    }

    //////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::SetLanguage(const char* const sLanguage)
    {
        if (sLanguage)
        {
            AZStd::string languageSubfolder;

            if (azstricmp(sLanguage, "english") == 0)
            {
                languageSubfolder = "english(us)";
            }
            else
            {
                languageSubfolder = sLanguage;
            }

            languageSubfolder += "/";

            m_localizedSoundbankFolder = m_soundbankFolder;
            m_localizedSoundbankFolder.append(languageSubfolder);

            const AkOSChar* akLanguageFolder = nullptr;
            CONVERT_CHAR_TO_OSCHAR(languageSubfolder.c_str(), akLanguageFolder);
            m_oFileIOHandler.SetLanguageFolder(akLanguageFolder);
        }
    }
} // namespace Audio

