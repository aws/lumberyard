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
#include "AudioSystemImpl_wwise.h"

#include <AzCore/std/containers/set.h>

#include <IAudioSystem.h>

#include <AudioSourceManager.h>
#include <AudioSystemImplCVars.h>
#include <Common_wwise.h>
#include <FileIOHandler_wwise.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>        // Sound engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>        // Music Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>          // Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>             // Default memory and stream managers

#include <PluginRegistration_wwise.h>                   // Registration of default set of plugins, customize this header to your needs.

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define AUDIOSYSTEMIMPL_WWISE_CPP_SECTION_1 1
#define AUDIOSYSTEMIMPL_WWISE_CPP_SECTION_2 2
#define AUDIOSYSTEMIMPL_WWISE_CPP_SECTION_3 3
#endif

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
//                              MEMORY HOOKS SETUP
//
//                             ##### IMPORTANT #####
//
// These custom alloc/free functions are declared as "extern" in AkMemoryMgr.h
// and MUST be defined by the game developer.
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

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION AUDIOSYSTEMIMPL_WWISE_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AudioSystemImpl_wwise_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AudioSystemImpl_wwise_cpp_provo.inl"
    #endif
#endif
}



namespace Audio
{
    const char* const CAudioSystemImpl_wwise::sWwiseImplSubPath = "wwise/";
    const char* const CAudioSystemImpl_wwise::sWwiseEventTag = "WwiseEvent";
    const char* const CAudioSystemImpl_wwise::sWwiseRtpcTag = "WwiseRtpc";
    const char* const CAudioSystemImpl_wwise::sWwiseSwitchTag = "WwiseSwitch";
    const char* const CAudioSystemImpl_wwise::sWwiseStateTag = "WwiseState";
    const char* const CAudioSystemImpl_wwise::sWwiseRtpcSwitchTag = "WwiseRtpc";
    const char* const CAudioSystemImpl_wwise::sWwiseFileTag = "WwiseFile";
    const char* const CAudioSystemImpl_wwise::sWwiseAuxBusTag = "WwiseAuxBus";
    const char* const CAudioSystemImpl_wwise::sWwiseValueTag = "WwiseValue";
    const char* const CAudioSystemImpl_wwise::sWwiseNameAttribute = "wwise_name";
    const char* const CAudioSystemImpl_wwise::sWwiseValueAttribute = "wwise_value";
    const char* const CAudioSystemImpl_wwise::sWwiseMutiplierAttribute = "atl_mult";
    const char* const CAudioSystemImpl_wwise::sWwiseShiftAttribute = "atl_shift";
    const char* const CAudioSystemImpl_wwise::sWwiseLocalisedAttribute = "wwise_localised";
    const char* const CAudioSystemImpl_wwise::sWwiseGlobalAudioObjectName = "LY-GlobalAudioObject";
    const float CAudioSystemImpl_wwise::sObstructionOcclusionMin = 0.0f;
    const float CAudioSystemImpl_wwise::sObstructionOcclusionMax = 1.0f;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AK callbacks
    void EndEventCallback(AkCallbackType eType, AkCallbackInfo* pCallbackInfo)
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
            "<Wwise> %s ErrorCode: %d PlayingID: %u GameObjID: %" PRISIZE_T, sTemp, in_eErrorCode, in_playingID, in_gameObjID);
    }
#endif // WWISE_FOR_RELEASE


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImpl_wwise::CAudioSystemImpl_wwise()
        : m_globalGameObjectID(static_cast<AkGameObjectID>(GLOBAL_AUDIO_OBJECT_ID))
        , m_defaultListenerGameObjectID(AK_INVALID_GAME_OBJECT)
        , m_nInitBankID(AK_INVALID_BANK_ID)
#if !defined(WWISE_FOR_RELEASE)
        , m_bCommSystemInitialized(false)
#endif // !WWISE_FOR_RELEASE
    {
        m_sRegularSoundBankFolder = WWISE_IMPL_BANK_FULL_PATH;

        m_sLocalizedSoundBankFolder = m_sRegularSoundBankFolder;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        m_sFullImplString = WWISE_IMPL_VERSION_STRING " (" WWISE_IMPL_BASE_PATH ")";
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
    // AZ::Component
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::Init()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::Activate()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::Deactivate()
    {
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioSystemImplementationNotificationBus
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemLoseFocus()
    {
        // With Wwise we drive this via events.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemGetFocus()
    {
        // With Wwise we drive this via events.
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

        AkStreamMgrSettings oStreamSettings;
        AK::StreamMgr::GetDefaultSettings(oStreamSettings);
        oStreamSettings.uMemorySize = g_audioImplCVars_wwise.m_nStreamManagerMemoryPoolSize << 10; // 64 KiB is the default value!

        if (AK::StreamMgr::Create(oStreamSettings) == nullptr)
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::StreamMgr::Create() failed!\n");
            ShutDown();
            return eARS_FAILURE;
        }

        AkDeviceSettings oDeviceSettings;
        AK::StreamMgr::GetDefaultDeviceSettings(oDeviceSettings);
        oDeviceSettings.uIOMemorySize = g_audioImplCVars_wwise.m_nStreamDeviceMemoryPoolSize << 10; // 2 MiB is the default value!

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION AUDIOSYSTEMIMPL_WWISE_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AudioSystemImpl_wwise_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AudioSystemImpl_wwise_cpp_provo.inl"
    #endif
#endif

        eResult = m_oFileIOHandler.Init(oDeviceSettings);

        if (!IS_WWISE_OK(eResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "m_oFileIOHandler.Init() returned AKRESULT %d", eResult);
            ShutDown();
            return eARS_FAILURE;
        }

        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sTemp(WWISE_IMPL_BASE_PATH);
        const AkOSChar* pTemp = nullptr;
        CONVERT_CHAR_TO_OSCHAR(sTemp.c_str(), pTemp);
        m_oFileIOHandler.SetBankPath(pTemp);

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

#if defined(AZ_PLATFORM_WINDOWS)
        // Turn off XAudio2 output type due to rare startup crashes.  Prefers WASAPI or DirectSound.
        oPlatformInitSettings.eAudioAPI = static_cast<AkAudioAPI>(oPlatformInitSettings.eAudioAPI & ~AkAPI_XAudio2);
        oPlatformInitSettings.threadBankManager.dwAffinityMask = 0;
        oPlatformInitSettings.threadLEngine.dwAffinityMask = 0;
        oPlatformInitSettings.threadMonitor.dwAffinityMask = 0;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION AUDIOSYSTEMIMPL_WWISE_CPP_SECTION_3
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/AudioSystemImpl_wwise_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/AudioSystemImpl_wwise_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_APPLE_OSX)
#elif defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
        oInitSettings.uDefaultPoolSize = 1.5 * 1024 * 1024;
        oPlatformInitSettings.uLEngineDefaultPoolSize = 1.5 * 1024 * 1024;
#elif defined(AZ_PLATFORM_ANDROID)

        JNIEnv* jniEnv = AZ::Android::AndroidEnv::Get()->GetJniEnv();
        jobject javaActivity = AZ::Android::AndroidEnv::Get()->GetActivityRef();
        JavaVM* javaVM = nullptr;
        if (jniEnv)
        {
            jniEnv->GetJavaVM(&javaVM);
        }

        oPlatformInitSettings.pJavaVM = javaVM;
        oPlatformInitSettings.jNativeActivity = javaActivity;

#elif defined(AZ_PLATFORM_LINUX_X64)
#else
    #error "Unsupported platform."
#endif

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

#if !defined(WWISE_FOR_RELEASE)
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
#endif // !WWISE_FOR_RELEASE

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
        azdestroy(this, Audio::AudioImplAllocator, CAudioSystemImpl_wwise);

        if (AZ::AllocatorInstance<Audio::AudioImplAllocator>::IsReady())
        {
            AZ::AllocatorInstance<Audio::AudioImplAllocator>::Destroy();
        }

        g_audioImplCVars_wwise.UnregisterVariables();

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
        const TAudioSourceId sourceId)
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

            const AkPlayingID nAKPlayingID = AK::SoundEngine::PostEvent(
                    pAKTriggerImplData->nAKID,
                    nAKObjectID,
                    AK_EndOfEvent,
                    &EndEventCallback,
                    pAKEventData);


            if (nAKPlayingID != AK_INVALID_PLAYING_ID)
            {
                if (sourceId != INVALID_AUDIO_SOURCE_ID)
                {
                    // Activate the audio input source (associates sourceId w/ playingId)...
                    AudioSourceManager::Get().ActivateSource(sourceId, nAKPlayingID);
                    pAKEventData->nSourceId = sourceId;
                }

                pAKEventData->audioEventState = eAES_PLAYING;
                pAKEventData->nAKID = nAKPlayingID;
                eResult = eARS_SUCCESS;
            }
            else
            {
                // if Posting an Event failed, try to prepare it, if it isn't prepared already
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Failed to Post Wwise event %" PRISIZE_T, pAKEventData->nAKID);
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
            ATLTransformToAKTransform(sWorldPosition, sAkSoundPos);

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
                    const float fCurrentAmount = stl::find_in_map(
                            pAKObjectData->cEnvironmentImplAmounts,
                            pAKEnvironmentData->nAKBusID,
                            -1.0f);

                    if ((fCurrentAmount == -1.0f) || (fabs(fCurrentAmount - fAmount) > sEnvEpsilon))
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
                    "Wwise failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
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
                            "Wwise failed to set the switch group %" PRISIZE_T " to state %" PRISIZE_T " on object %" PRISIZE_T,
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
                            "Wwise failed to set the state group %" PRISIZE_T "to state %" PRISIZE_T,
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
                            "Wwise failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
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
                    g_audioImplLogger_wwise.Log(eALT_WARNING, "Unknown EWwiseSwitchType: %" PRISIZE_T, pAKSwitchStateData->eType);
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
                "Obstruction value %f is out of range, fObstruction should be between %f and %f.",
                fObstruction, sObstructionOcclusionMin, sObstructionOcclusionMax);
        }

        if (fOcclusion < sObstructionOcclusionMin || fOcclusion > sObstructionOcclusionMax)
        {
            g_audioImplLogger_wwise.Log(
                eALT_WARNING,
                "Occlusion value %f is out of range, fOcclusion should be between %f and %f " PRISIZE_T,
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
                    "Wwise failed to set Obstruction %f and Occlusion %f on object %" PRISIZE_T,
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
            ATLTransformToAKTransform(oNewPosition, oAKListenerPos);

            const AKRESULT eAKResult = AK::SoundEngine::SetPosition(pAKListenerData->nAKListenerObjectId, oAKListenerPos);

            if (IS_WWISE_OK(eAKResult))
            {
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise SetListenerPosition failed with AKRESULT: %" PRISIZE_T, eAKResult);
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
                    "Wwise failed to reset the Rtpc %" PRISIZE_T " on object %" PRISIZE_T,
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
    EAudioRequestStatus CAudioSystemImpl_wwise::ParseAudioFileEntry(const XmlNodeRef pAudioFileEntryNode, SATLAudioFileEntryInfo* const pFileEntryInfo)
    {
        static CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sPath;

        EAudioRequestStatus eResult = eARS_FAILURE;

        if (pFileEntryInfo && azstricmp(pAudioFileEntryNode->getTag(), sWwiseFileTag) == 0)
        {
            const char* const sWwiseAudioFileEntryName = pAudioFileEntryNode->getAttr(sWwiseNameAttribute);

            if (sWwiseAudioFileEntryName && sWwiseAudioFileEntryName[0] != '\0')
            {
                const char* const sWwiseLocalised = pAudioFileEntryNode->getAttr(sWwiseLocalisedAttribute);
                pFileEntryInfo->bLocalized = (sWwiseLocalised != nullptr) && (azstricmp(sWwiseLocalised, "true") == 0);

                pFileEntryInfo->sFileName = sWwiseAudioFileEntryName;

                pFileEntryInfo->nMemoryBlockAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;

                pFileEntryInfo->pImplData = azcreate(SATLAudioFileEntryData_wwise, (), Audio::AudioImplAllocator, "ATLAudioFileEntryData_wwise");

                eResult = eARS_SUCCESS;
            }
            else
            {
                pFileEntryInfo->sFileName = nullptr;
                pFileEntryInfo->nMemoryBlockAlignment = 0;
                pFileEntryInfo->pImplData = nullptr;
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioFileEntryData(IATLAudioFileEntryData* const pOldAudioFileEntry)
    {
        azdestroy(pOldAudioFileEntry, Audio::AudioImplAllocator, SATLAudioFileEntryData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_wwise::GetAudioFileLocation(SATLAudioFileEntryInfo* const pFileEntryInfo)
    {
        const char* sResult = nullptr;

        if (pFileEntryInfo)
        {
            sResult = pFileEntryInfo->bLocalized ? m_sLocalizedSoundBankFolder.c_str() : m_sRegularSoundBankFolder.c_str();
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
            auto listenerName = AZStd::string::format("DefaultAudioListener(%" PRISIZE_T ")", pNewObject->nAKListenerObjectId);
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
                    g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in SetDefaultListeners to set AkGameObjectID %" PRISIZE_T " as default with AKRESULT: %d", pNewObject->nAKListenerObjectId, eAKResult);
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in RegisterGameObj registering a DefaultAudioListener with AKRESULT: %d", eAKResult);
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
            auto listenerName = AZStd::string::format("AudioListener(%" PRISIZE_T ")", pNewObject->nAKListenerObjectId);
            AKRESULT eAKResult = AK::SoundEngine::RegisterGameObj(pNewObject->nAKListenerObjectId, listenerName.c_str());
            if (!IS_WWISE_OK(eAKResult))
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in RegisterGameObj registering an AudioListener with AKRESULT: %d", eAKResult);
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
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in UnregisterGameObj unregistering an AudioListener(%" PRISIZE_T ") with AKRESULT: %d", listenerData->nAKListenerObjectId, eAKResult);
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
    const SATLTriggerImplData_wwise* CAudioSystemImpl_wwise::NewAudioTriggerImplData(const XmlNodeRef pAudioTriggerImplNode)
    {
        SATLTriggerImplData_wwise* pNewTriggerImpl = nullptr;

        if (azstricmp(pAudioTriggerImplNode->getTag(), sWwiseEventTag) == 0)
        {
            const char* const sWwiseEventName = pAudioTriggerImplNode->getAttr(sWwiseNameAttribute);
            const AkUniqueID nAKID = AK::SoundEngine::GetIDFromString(sWwiseEventName);//Does not check if the string represents an event!!!!

            if (nAKID != AK_INVALID_UNIQUE_ID)
            {
                pNewTriggerImpl = azcreate(SATLTriggerImplData_wwise, (nAKID), Audio::AudioImplAllocator, "ATLTriggerImplData_wwise");
            }
            else
            {
                AZ_Assert(false, "<Wwise> Failed to get a unique ID from string '%s'.", sWwiseEventName);
            }
        }

        return pNewTriggerImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioTriggerImplData(const IATLTriggerImplData* const pOldTriggerImplData)
    {
        azdestroy(const_cast<IATLTriggerImplData*>(pOldTriggerImplData), Audio::AudioImplAllocator, SATLTriggerImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const SATLRtpcImplData_wwise* CAudioSystemImpl_wwise::NewAudioRtpcImplData(const XmlNodeRef pAudioRtpcNode)
    {
        SATLRtpcImplData_wwise* pNewRtpcImpl = nullptr;

        AkRtpcID nAKRtpcID = AK_INVALID_RTPC_ID;
        float fMult = 1.0f;
        float fShift = 0.0f;

        ParseRtpcImpl(pAudioRtpcNode, nAKRtpcID, fMult, fShift);

        if (nAKRtpcID != AK_INVALID_RTPC_ID)
        {
            pNewRtpcImpl = azcreate(SATLRtpcImplData_wwise, (nAKRtpcID, fMult, fShift), Audio::AudioImplAllocator, "ATLRtpcImplData_wwise");
        }

        return pNewRtpcImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioRtpcImplData(const IATLRtpcImplData* const pOldRtpcImplData)
    {
        azdestroy(const_cast<IATLRtpcImplData*>(pOldRtpcImplData), Audio::AudioImplAllocator, SATLRtpcImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const SATLSwitchStateImplData_wwise* CAudioSystemImpl_wwise::NewAudioSwitchStateImplData(const XmlNodeRef pAudioSwitchNode)
    {
        const char* const sWwiseSwitchNodeTag = pAudioSwitchNode->getTag();
        const SATLSwitchStateImplData_wwise* pNewSwitchImpl = nullptr;

        if (azstricmp(sWwiseSwitchNodeTag, sWwiseSwitchTag) == 0)
        {
            pNewSwitchImpl = ParseWwiseSwitchOrState(pAudioSwitchNode, eWST_SWITCH);
        }
        else if (azstricmp(sWwiseSwitchNodeTag, sWwiseStateTag) == 0)
        {
            pNewSwitchImpl = ParseWwiseSwitchOrState(pAudioSwitchNode, eWST_STATE);
        }
        else if (azstricmp(sWwiseSwitchNodeTag, sWwiseRtpcSwitchTag) == 0)
        {
            pNewSwitchImpl = ParseWwiseRtpcSwitch(pAudioSwitchNode);
        }
        else
        {
            // Unknown Wwise switch tag!
            g_audioImplLogger_wwise.Log(eALT_WARNING, "Unknown Wwise Switch Tag: %s" PRISIZE_T, sWwiseSwitchNodeTag);
            AZ_Assert(false, "<Wwise> Unknown Wwise Switch Tag.");
        }

        return pNewSwitchImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioSwitchStateImplData(const IATLSwitchStateImplData* const pOldAudioSwitchStateImplData)
    {
        azdestroy(const_cast<IATLSwitchStateImplData*>(pOldAudioSwitchStateImplData), Audio::AudioImplAllocator, SATLSwitchStateImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const SATLEnvironmentImplData_wwise* CAudioSystemImpl_wwise::NewAudioEnvironmentImplData(const XmlNodeRef pAudioEnvironmentNode)
    {
        SATLEnvironmentImplData_wwise* pNewEnvironmentImpl = nullptr;

        if (azstricmp(pAudioEnvironmentNode->getTag(), sWwiseAuxBusTag) == 0)
        {
            const char* const sWwiseAuxBusName = pAudioEnvironmentNode->getAttr(sWwiseNameAttribute);
            const AkUniqueID nAKBusID = AK::SoundEngine::GetIDFromString(sWwiseAuxBusName); //Does not check if the string represents an event!!!!

            if (nAKBusID != AK_INVALID_AUX_ID)
            {
                pNewEnvironmentImpl = azcreate(SATLEnvironmentImplData_wwise, (eWAET_AUX_BUS, static_cast<AkAuxBusID>(nAKBusID)), Audio::AudioImplAllocator, "ATLEnvironmentImplData_wwise");
            }
            else
            {
                AZ_Assert(false, "<Wwise> Unknown Aux Bus ID.");
            }
        }
        else if (azstricmp(pAudioEnvironmentNode->getTag(), sWwiseRtpcTag) == 0)
        {
            AkRtpcID nAKRtpcID = AK_INVALID_RTPC_ID;
            float fMult = 1.0f;
            float fShift = 0.0f;

            ParseRtpcImpl(pAudioEnvironmentNode, nAKRtpcID, fMult, fShift);

            if (nAKRtpcID != AK_INVALID_RTPC_ID)
            {
                pNewEnvironmentImpl = azcreate(SATLEnvironmentImplData_wwise, (eWAET_RTPC, nAKRtpcID, fMult, fShift), Audio::AudioImplAllocator, "ATLEnvironmentImplData_wwise");
            }
            else
            {
                AZ_Assert(false, "<Wwise> Unknown RTPC ID.");
            }
        }

        return pNewEnvironmentImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioEnvironmentImplData(const IATLEnvironmentImplData* const pOldEnvironmentImplData)
    {
        azdestroy(const_cast<IATLEnvironmentImplData*>(pOldEnvironmentImplData), Audio::AudioImplAllocator, SATLEnvironmentImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_wwise::GetImplementationNameString() const
    {
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        return m_sFullImplString.c_str();
#else
        return nullptr;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::GetMemoryInfo(SAudioImplMemoryInfo& oMemoryInfo) const
    {
        oMemoryInfo.nPrimaryPoolSize = AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().Capacity();
        oMemoryInfo.nPrimaryPoolUsedSize = AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().NumAllocatedBytes();
        oMemoryInfo.nPrimaryPoolAllocations = 0;

    #if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
        oMemoryInfo.nSecondaryPoolSize = g_audioImplMemoryPoolSecondary_wwise.MemSize();
        oMemoryInfo.nSecondaryPoolUsedSize = oMemoryInfo.nSecondaryPoolSize - g_audioImplMemoryPoolSecondary_wwise.MemFree();
        oMemoryInfo.nSecondaryPoolAllocations = g_audioImplMemoryPoolSecondary_wwise.FragmentCount();
    #else
        oMemoryInfo.nSecondaryPoolSize = 0;
        oMemoryInfo.nSecondaryPoolUsedSize = 0;
        oMemoryInfo.nSecondaryPoolAllocations = 0;
    #endif // PROVIDE_WWISE_IMPL_SECONDARY_POOL
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
    const SATLSwitchStateImplData_wwise* CAudioSystemImpl_wwise::ParseWwiseSwitchOrState(
        XmlNodeRef pNode,
        EWwiseSwitchType eType)
    {
        SATLSwitchStateImplData_wwise* pSwitchStateImpl = nullptr;

        const char* const sWwiseSwitchNodeName = pNode->getAttr(sWwiseNameAttribute);
        if (sWwiseSwitchNodeName && (sWwiseSwitchNodeName[0] != '\0') && (pNode->getChildCount() == 1))
        {
            const XmlNodeRef pValueNode(pNode->getChild(0));

            if (pValueNode && azstricmp(pValueNode->getTag(), sWwiseValueTag) == 0)
            {
                const char* sWwiseSwitchStateName = pValueNode->getAttr(sWwiseNameAttribute);

                if (sWwiseSwitchStateName && (sWwiseSwitchStateName[0] != '\0'))
                {
                    const AkUniqueID nAKSwitchID = AK::SoundEngine::GetIDFromString(sWwiseSwitchNodeName);
                    const AkUniqueID nAKSwitchStateID = AK::SoundEngine::GetIDFromString(sWwiseSwitchStateName);
                    pSwitchStateImpl = azcreate(SATLSwitchStateImplData_wwise, (eType, nAKSwitchID, nAKSwitchStateID), Audio::AudioImplAllocator, "ATLSwitchStateImplData_wwise");
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(
                eALT_WARNING,
                "A Wwise Switch or State %s inside ATLSwitchState needs to have exactly one WwiseValue.",
                sWwiseSwitchNodeName);
        }

        return pSwitchStateImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const SATLSwitchStateImplData_wwise* CAudioSystemImpl_wwise::ParseWwiseRtpcSwitch(XmlNodeRef pNode)
    {
        SATLSwitchStateImplData_wwise* pSwitchStateImpl = nullptr;

        const char* const sWwiseRtpcNodeName = pNode->getAttr(sWwiseNameAttribute);
        if (sWwiseRtpcNodeName && sWwiseRtpcNodeName[0] != '\0' && pNode->getChildCount() == 0)
        {
            float fRtpcValue = 0.f;
            if (pNode->getAttr(sWwiseValueAttribute, fRtpcValue))
            {
                const AkUniqueID nAKRtpcID = AK::SoundEngine::GetIDFromString(sWwiseRtpcNodeName);
                pSwitchStateImpl = azcreate(SATLSwitchStateImplData_wwise, (eWST_RTPC, nAKRtpcID, nAKRtpcID, fRtpcValue), Audio::AudioImplAllocator, "ATLSwitchStateImplData_wwise");
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(
                eALT_WARNING,
                "A Wwise Rtpc '%s' inside ATLSwitchState shouldn't have any children.",
                sWwiseRtpcNodeName);
        }

        return pSwitchStateImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::ParseRtpcImpl(XmlNodeRef pNode, AkRtpcID& rAkRtpcID, float& rMult, float& rShift)
    {
        SATLRtpcImplData_wwise* pNewRtpcImpl = nullptr;

        if (azstricmp(pNode->getTag(), sWwiseRtpcTag) == 0)
        {
            const char* const sWwiseRtpcName = pNode->getAttr(sWwiseNameAttribute);
            rAkRtpcID = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(sWwiseRtpcName));

            if (rAkRtpcID != AK_INVALID_RTPC_ID)
            {
                //the Wwise name is supplied
                pNode->getAttr(sWwiseMutiplierAttribute, rMult);
                pNode->getAttr(sWwiseShiftAttribute, rShift);
            }
            else
            {
                // Invalid Wwise RTPC name!
                AZ_Assert(false, "<Wwise> Invalid RTPC name: '%s'", sWwiseRtpcName);
            }
        }
        else
        {
            // Unknown Wwise RTPC tag!
            AZ_Assert(false, "<Wwise> Unknown RTPC tag: '%s'", sWwiseRtpcTag);
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
                    "Wwise PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %" PRISIZE_T,
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
                    "Wwise PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %d",
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
            uint32 nAuxCount = 0;

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
                    "Wwise SetGameObjectAuxSendValues failed on object %" PRISIZE_T " with AKRESULT: %d",
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
            CryFixedStringT<MAX_AUDIO_FILE_NAME_LENGTH> sLanguageFolder;

            if (azstricmp(sLanguage, "english") == 0)
            {
                sLanguageFolder = "english(us)";
            }
            else
            {
                sLanguageFolder = sLanguage;// TODO: handle the other possible language variations
            }

            sLanguageFolder += "/";

            m_sLocalizedSoundBankFolder = m_sRegularSoundBankFolder;
            m_sLocalizedSoundBankFolder += sLanguageFolder.c_str();

            const AkOSChar* pTemp = nullptr;
            CONVERT_CHAR_TO_OSCHAR(sLanguageFolder.c_str(), pTemp);
            m_oFileIOHandler.SetLanguageFolder(pTemp);
        }
    }
} // namespace Audio

