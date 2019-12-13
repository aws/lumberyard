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
#include "AudioSystemImplCVars.h"
#include <IConsole.h>

#include <CryAudioImplWwise_Traits_Platform.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioWwiseImplCVars::CAudioWwiseImplCVars()
        : m_nPrimaryMemoryPoolSize(0)
        , m_nSecondaryMemoryPoolSize(0)
        , m_nPrepareEventMemoryPoolSize(0)
        , m_nStreamManagerMemoryPoolSize(0)
        , m_nStreamDeviceMemoryPoolSize(0)
        , m_nSoundEngineDefaultMemoryPoolSize(0)
        , m_nCommandQueueMemoryPoolSize(0)
        , m_nLowerEngineDefaultPoolSize(0)
    #if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        , m_nEnableCommSystem(0)
        , m_nEnableOutputCapture(0)
        , m_nMonitorMemoryPoolSize(0)
        , m_nMonitorQueueMemoryPoolSize(0)
    #endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioWwiseImplCVars::~CAudioWwiseImplCVars()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioWwiseImplCVars::RegisterVariables()
    {
        m_nCommandQueueMemoryPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_COMMAND_QUEUE_MEMORY_POOL_SIZE;
        m_nLowerEngineDefaultPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_LOWER_ENGINE_DEFAULT_POOL_SIZE;
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        m_nMonitorMemoryPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_MONITOR_MEMORY_POOL_SIZE;
        m_nMonitorQueueMemoryPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_MONITOR_QUEUE_MEMORY_POOL_SIZE;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
        m_nPrepareEventMemoryPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_PREPARE_EVENT_MEMORY_POOL_SIZE;
        m_nPrimaryMemoryPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_PRIMARY_POOL_SIZE;
        m_nSecondaryMemoryPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_SECONDARY_POOL_SIZE;
        m_nSoundEngineDefaultMemoryPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_SOUND_ENGINE_DEFAULT_MEMORY_POOL_SIZE;
        m_nStreamDeviceMemoryPoolSize = AZ_TRAIT_CRYAUDIOIMPLWWISE_STREAMER_DEVICE_MEMORY_POOL_SIZE;
        m_nStreamManagerMemoryPoolSize      = AZ_TRAIT_CRYAUDIOIMPLWWISE_STREAMER_MANAGER_MEMORY_POOL_SIZE;

        REGISTER_CVAR2("s_WwiseCommandQueueMemoryPoolSize", &m_nCommandQueueMemoryPoolSize, m_nCommandQueueMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the Wwise command queue memory pool.\n"
            "Usage: s_WwiseCommandQueueMemoryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_COMMAND_QUEUE_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_WwiseLowerEngineDefaultPoolSize", &m_nLowerEngineDefaultPoolSize, m_nLowerEngineDefaultPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the Wwise lower engine memory pool.\n"
            "Usage: s_WwiseLowerEngineDefaultPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_LOWER_ENGINE_DEFAULT_POOL_SIZE_DEFAULT_TEXT "\n");

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        REGISTER_CVAR2("s_WwiseMonitorMemoryPoolSize", &m_nMonitorMemoryPoolSize, m_nMonitorMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the Wwise monitor memory pool.\n"
            "Usage: s_WwiseMonitorMemoryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_MONITOR_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_WwiseMonitorQueueMemoryPoolSize", &m_nMonitorQueueMemoryPoolSize, m_nMonitorQueueMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the Wwise monitor queue memory pool.\n"
            "Usage: s_WwiseMonitorQueueMemoryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_MONITOR_QUEUE_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

        REGISTER_CVAR2("s_WwisePrepareEventMemoryPoolSize", &m_nPrepareEventMemoryPoolSize, m_nPrepareEventMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the Wwise prepare event memory pool.\n"
            "Usage: s_WwisePrepareEventMemoryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_PREPARE_EVENT_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_WwisePrimaryPoolSize", &m_nPrimaryMemoryPoolSize, m_nPrimaryMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the memory pool to be used by the Wwise audio system implementation.\n"
            "Usage: s_WwisePrimaryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_PRIMARY_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_WwiseSecondaryPoolSize", &m_nSecondaryMemoryPoolSize, m_nSecondaryMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the memory pool to be used by the Wwise audio system implementation.\n"
            "Usage: s_WwiseSecondaryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_SECONDARY_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_WwiseSoundEngineDefaultMemoryPoolSize", &m_nSoundEngineDefaultMemoryPoolSize, m_nSoundEngineDefaultMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the Wwise sound engine default memory pool.\n"
            "Usage: s_WwiseSoundEngineDefaultMemoryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_SOUND_ENGINE_DEFAULT_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_WwiseStreamDeviceMemoryPoolSize", &m_nStreamDeviceMemoryPoolSize, m_nStreamDeviceMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the Wwise stream device memory pool.\n"
            "Usage: s_WwiseStreamDeviceMemoryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_STREAMER_DEVICE_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_WwiseStreamManagerMemoryPoolSize", &m_nStreamManagerMemoryPoolSize, m_nStreamManagerMemoryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the Wwise stream manager memory pool.\n"
            "Usage: s_WwiseStreamManagerMemoryPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLWWISE_STREAMER_MANAGER_MEMORY_POOL_SIZE_DEFAULT_TEXT "\n");

    #if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        REGISTER_CVAR2("s_WwiseEnableCommSystem", &m_nEnableCommSystem, 0, VF_REQUIRE_APP_RESTART,
            "Specifies whether Wwise should initialize using its Comm system or not.\n"
            "This cvar is only available in non-release builds.\n"
            "Usage: s_WwiseEnableCommSystem [0/1]\n"
            "Default: 0 (off)\n");

        REGISTER_CVAR2("s_WwiseEnableOutputCapture", &m_nEnableOutputCapture, 0, VF_REQUIRE_APP_RESTART,
            "Allows for capturing the output audio to a wav file.\n"
            "This cvar is only available in non-release builds.\n"
            "Usage: s_WwiseEnableOutputCapture [0/1]\n"
            "Default: 0 (off)\n");
    #endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioWwiseImplCVars::UnregisterVariables()
    {
        IConsole* const pConsole = gEnv->pConsole;
        AZ_Assert(pConsole, "Wwise Impl CVar Unregistration - IConsole is already null!");

        pConsole->UnregisterVariable("s_WwisePrimaryPoolSize");
        pConsole->UnregisterVariable("s_WwiseSecondaryPoolSize");
        pConsole->UnregisterVariable("s_WwisePrepareEventMemoryPoolSize");
        pConsole->UnregisterVariable("s_WwiseStreamManagerMemoryPoolSize");
        pConsole->UnregisterVariable("s_WwiseStreamDeviceMemoryPoolSize");
        pConsole->UnregisterVariable("s_WwiseSoundEngineDefaultMemoryPoolSize");
        pConsole->UnregisterVariable("s_WwiseCommandQueueMemoryPoolSize");
        pConsole->UnregisterVariable("s_WwiseLowerEngineDefaultPoolSize");

    #if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
        pConsole->UnregisterVariable("s_WwiseEnableCommSystem");
        pConsole->UnregisterVariable("s_WwiseEnableOutputCapture");
        pConsole->UnregisterVariable("s_WwiseMonitorMemoryPoolSize");
        pConsole->UnregisterVariable("s_WwiseMonitorQueueMemoryPoolSize");
    #endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
    }
} // namespace Audio
