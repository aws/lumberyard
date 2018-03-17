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

#include <platform_impl.h>      // include this once per module!
#include <IEngineModule.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <AudioSystem.h>
#include <SoundCVars.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    #include "../CryAction/IViewSystem.h"
    #include <IGameFramework.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE


namespace Audio
{
    // Define global objects.
    CSoundCVars g_audioCVars;
    CAudioLogger g_audioLogger;

#define MAX_MODULE_NAME_LENGTH 256

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CSystemEventListener_Audio
        : public ISystemEventListener
        , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
    {
    public:
        ~CSystemEventListener_Audio()
        {
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override
        {
            switch (event)
            {
                case ESYSTEM_EVENT_LEVEL_LOAD_START:
                {
                    AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GarbageCollect();
                    break;
                }
                case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
                {
                    AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GarbageCollect();
                    break;
                }
                case ESYSTEM_EVENT_RANDOM_SEED:
                {
                    cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
                    break;
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////
        void OnApplicationConstrained(Event /*lastEvent*/) override
        {
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, m_oLoseFocusRequest);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////
        void OnApplicationUnconstrained(Event /*lastEvent*/) override
        {
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, m_oGetFocusRequest);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////
        void InitRequestData()
        {
            m_oLoseFocusRequest.nFlags = eARF_PRIORITY_HIGH;
            m_oLoseFocusRequest.pData = &m_oAMLoseFocusData;

            m_oGetFocusRequest.nFlags = eARF_PRIORITY_HIGH;
            m_oGetFocusRequest.pData = &m_oAMGetFocusData;

            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
        }

    private:
        SAudioRequest m_oLoseFocusRequest;
        SAudioRequest m_oGetFocusRequest;
        SAudioManagerRequestData<eAMRT_LOSE_FOCUS> m_oAMLoseFocusData;
        SAudioManagerRequestData<eAMRT_GET_FOCUS> m_oAMGetFocusData;
    };

    static CSystemEventListener_Audio g_system_event_listener_audio;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CreateAudioSystem(SSystemGlobalEnvironment& rEnv)
    {
        bool bSuccess = false;

        AZ_Assert(!AudioSystemRequestBus::HasHandlers(), "CreateAudioSystem - The AudioSystemRequestBus is already connected!");

        // The AudioSystem must be the first object that is allocated from the audio memory pool after it has been created and the last that is freed from it!
        auto audioSystemNew = azcreate(CAudioSystem, (), Audio::AudioSystemAllocator, "AudioSystem");

        if (audioSystemNew)
        {
            AudioSystemRequestBus::BroadcastResult(bSuccess, &AudioSystemRequestBus::Events::Initialize);
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Could not create an instance of CAudioSystem! Keeping the default AudioSystem!\n");
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void PrepareAudioSystem()
    {
        // This is called when a new audio implementation has been set,
        // so update the controls path before we start loading data...
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::UpdateControlsPath);

        // Must be blocking requests.
        SAudioRequest oAudioRequestData;
        oAudioRequestData.nFlags = eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING;

        const char* controlsPath = nullptr;
        AudioSystemRequestBus::BroadcastResult(controlsPath, &AudioSystemRequestBus::Events::GetControlsPath);
        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sTemp(controlsPath);

        SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA> oAMData(sTemp.c_str(), eADS_GLOBAL);
        oAudioRequestData.pData = &oAMData;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

        SAudioManagerRequestData<eAMRT_PARSE_PRELOADS_DATA> oAMData2(sTemp.c_str(), eADS_GLOBAL);
        oAudioRequestData.pData = &oAMData2;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

        SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST> oAMData3(SATLInternalControlIDs::nGlobalPreloadRequestID);
        oAudioRequestData.pData = &oAMData3;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);
    }

} // namespace Audio


///////////////////////////////////////////////////////////////////////////////////////////////////
class CEngineModule_CrySoundSystem
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CrySoundSystem, AUDIO_SYSTEM_MODULE_NAME, 0xec73cf4362ca4a7f, 0x8b451076dc6fdb8b)

    const char* GetName() const override
    {
        return "CrySoundSystem";
    }
    const char* GetCategory() const override
    {
        return "CryEngine";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
    {
        using namespace Audio;
        bool bSuccess = false;

        // initialize memory pools
        if (!AZ::AllocatorInstance<Audio::AudioSystemAllocator>::IsReady())
        {
            const size_t poolSize = g_audioCVars.m_nATLPoolSize << 10;

            Audio::AudioSystemAllocator::Descriptor allocDesc;

            // Generic Allocator:
            allocDesc.m_allocationRecords = true;
            allocDesc.m_heap.m_numMemoryBlocks = 1;
            allocDesc.m_heap.m_memoryBlocksByteSize[0] = poolSize;

            allocDesc.m_heap.m_memoryBlocks[0] = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(allocDesc.m_heap.m_memoryBlocksByteSize[0], allocDesc.m_heap.m_memoryBlockAlignment);

            // Note: This allocator is destroyed in CAudioSystem::Release() after the CAudioSystem has been freed.
            AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Create(allocDesc);
        }

        if (CreateAudioSystem(env))
        {

            g_audioLogger.Log(eALT_ALWAYS, "%s loaded!", GetName());

            // Load the Audio Implementation Module.  If it fails to load, don't consider CrySoundSystem as a failure too.
            s_currentModuleName = m_cvAudioSystemImplementationName->GetString();
            if (env.pSystem->InitializeEngineModule(s_currentModuleName.c_str(), s_currentModuleName.c_str(), initParams))
            {
                PrepareAudioSystem();

                g_system_event_listener_audio.InitRequestData();
                env.pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_audio);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Failed to load Audio System Implementation Module named: %s\n"
                    "Check the spelling of the module (case sensitive!), and make sure it's correct.", s_currentModuleName.c_str());
                // Reset to empty string if loading failed, next time around we won't try to unload a module that isn't there.
                s_currentModuleName.clear();
            }

            bSuccess = true;
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Could not create AudioSystemImplementation %s!",
                m_cvAudioSystemImplementationName->GetString());
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    static void OnAudioImplChanged(ICVar* pAudioSystemImplementationName)
    {
    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        using namespace Audio;
        // First set the NULL implementation. This will release the currently running implementation.
        SAudioRequest oAudioRequestData;
        oAudioRequestData.nFlags = eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING;

        SAudioManagerRequestData<eAMRT_RELEASE_AUDIO_IMPL> oAMData;
        oAudioRequestData.pData = &oAMData;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

        // Then unload the previous module.
        if (!s_currentModuleName.empty())
        {
            if (!GetISystem()->UnloadEngineModule(s_currentModuleName.c_str(), s_currentModuleName.c_str()))
            {
                g_audioLogger.Log(eALT_WARNING, "Failed to unload Audio System Implementation Module named: %s", s_currentModuleName.c_str());
            }
        }

        // Then initialize the new engine module.
        SSystemInitParams oSystemInitParams;
        s_currentModuleName = pAudioSystemImplementationName->GetString();
        if (!GetISystem()->InitializeEngineModule(s_currentModuleName.c_str(), s_currentModuleName.c_str(), oSystemInitParams))
        {
            g_audioLogger.Log(eALT_ERROR, "Failed to load Audio System Implementation Module named: %s\n"
                "Check the spelling of the module (case sensitive!), and make sure it's correct.", s_currentModuleName.c_str());
            // Reset to empty string if loading failed, next time around we won't try to unload a module that isn't there.
            s_currentModuleName.clear();
            return;
        }

        // Then load global controls data and preloads.
        PrepareAudioSystem();

        // Then load level specific controls data and preloads.
        const string szLevelName = PathUtil::GetFileName(gEnv->pGame->GetIGameFramework()->GetLevelName());

        if (!szLevelName.empty() && szLevelName.compareNoCase("Untitled") != 0)
        {
            const char* controlsPath = nullptr;
            AudioSystemRequestBus::BroadcastResult(controlsPath, &AudioSystemRequestBus::Events::GetControlsPath);
            string szAudioLevelPath(controlsPath);
            szAudioLevelPath += "levels/";
            szAudioLevelPath += szLevelName;
            SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA> oAMData2(szAudioLevelPath, eADS_LEVEL_SPECIFIC);
            SAudioRequest audioRequestData;
            audioRequestData.nFlags = eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING; // Needs to be blocking so data is available for next preloading request!
            audioRequestData.pData = &oAMData2;
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, audioRequestData);

            SAudioManagerRequestData<eAMRT_PARSE_PRELOADS_DATA> oAMData3(szAudioLevelPath, eADS_LEVEL_SPECIFIC);
            audioRequestData.pData = &oAMData3;
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, audioRequestData);

            TAudioPreloadRequestID nPreloadRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID;

            AudioSystemRequestBus::BroadcastResult(nPreloadRequestID, &AudioSystemRequestBus::Events::GetAudioPreloadRequestID, szLevelName.c_str());
            if (nPreloadRequestID != INVALID_AUDIO_PRELOAD_REQUEST_ID)
            {
                SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST> oAMData4(nPreloadRequestID, true);
                audioRequestData.pData = &oAMData4;
                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, audioRequestData);
            }
        }

        // Then force the view system to update the listener position.
        if (gEnv->IsEditor() && !gEnv->IsEditorGameMode())
        {
            if (IGame* pIGame = gEnv->pGame)
            {
                if (IGameFramework* pIGameFramework = pIGame->GetIGameFramework())
                {
                    if (IViewSystem* pIViewSystem = pIGameFramework->GetIViewSystem())
                    {
                        pIViewSystem->UpdateSoundListeners();
                    }
                }
            }
        }

        // And finally re-trigger all active audio controls to restart previously playing sounds.
        SAudioManagerRequestData<eAMRT_RETRIGGER_AUDIO_CONTROLS> oAMData5;
        oAudioRequestData.pData = &oAMData5;
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

        GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_IMPLEMENTATION_LOADED, 0, 0);
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

private:
    ICVar* m_cvAudioSystemImplementationName;
    static CryFixedStringT<MAX_MODULE_NAME_LENGTH> s_currentModuleName;
};

CryFixedStringT<MAX_MODULE_NAME_LENGTH> CEngineModule_CrySoundSystem::s_currentModuleName;
CRYREGISTER_SINGLETON_CLASS(CEngineModule_CrySoundSystem)

///////////////////////////////////////////////////////////////////////////////////////////////////
CEngineModule_CrySoundSystem::CEngineModule_CrySoundSystem()
{
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(CrySoundSystem_cpp)
#else
#define CRYSOUNDSYSTEM_CPP_TRAIT_DISABLE_AUDIO 0
#endif
#if CRYSOUNDSYSTEM_CPP_TRAIT_DISABLE_AUDIO || defined(AZ_TESTS_ENABLED)
    #define DEFAULT_AUDIO_SYSTEM_IMPLEMENTATION_NAME    "CryAudioImplNoSound"
#else
    #define DEFAULT_AUDIO_SYSTEM_IMPLEMENTATION_NAME    "CryAudioImplWwise"
#endif

    // Register audio implementation name cvar
    m_cvAudioSystemImplementationName = REGISTER_STRING_CB(
            "s_AudioSystemImplementationName", DEFAULT_AUDIO_SYSTEM_IMPLEMENTATION_NAME, VF_CHEAT | VF_CHEAT_NOCHECK,
            "Holds the name of the AudioSystemImplementation library to be used.\n"
            "Usage: s_AudioSystemImplementationName <name of the library without extension>\n"
            "Default: " DEFAULT_AUDIO_SYSTEM_IMPLEMENTATION_NAME "\n",
            CEngineModule_CrySoundSystem::OnAudioImplChanged
            );

    Audio::g_audioCVars.RegisterVariables();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CEngineModule_CrySoundSystem::~CEngineModule_CrySoundSystem()
{
}

#include <CrtDebugStats.h>
