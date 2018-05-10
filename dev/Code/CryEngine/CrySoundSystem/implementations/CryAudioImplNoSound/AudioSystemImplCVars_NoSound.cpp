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

#include "StdAfx.h"
#include "AudioSystemImplCVars_NoSound.h"
#include <IConsole.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImplCVars_nosound::CAudioSystemImplCVars_nosound()
        : m_nPrimaryPoolSize(0)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImplCVars_nosound::~CAudioSystemImplCVars_nosound()
    {}

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImplCVars_nosound::RegisterVariables()
    {
        // ly-note: what is the justification for nosound using memory pool sizes as large as Wwise?

    #if defined(AZ_PLATFORM_WINDOWS)
        m_nPrimaryPoolSize = 128 << 10;// 128 MiB
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_APPLE_OSX)
        m_nPrimaryPoolSize = 128 << 10;// 128 MiB
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_LINUX_X64)
        m_nPrimaryPoolSize = 128 << 10;// 128 MiB
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_APPLE_IOS)
        m_nPrimaryPoolSize = 8 << 10;// 8 MiB
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_APPLE_TV)
        m_nPrimaryPoolSize = 8 << 10;// 8 MiB
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_ANDROID)
        m_nPrimaryPoolSize = 8 << 10;// 8 MiB
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(AudioSystemImplCVars_NoSound_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #else
        #error "Unsupported platform!"
    #endif

        REGISTER_CVAR2("s_AudioImplementationPrimaryPoolSize", &m_nPrimaryPoolSize, m_nPrimaryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the memory pool to be used by the Audio System Implementation."
            "Usage: s_AudioImplementationPrimaryPoolSize [0..]\n"
            "Default Windows: 131072 (128 MiB), Xbox One: 131072 (128 MiB), PS4: 131072 (128 MiB), Mac: 131072 (128 MiB), Linux: 131072 (128 MiB), iOS: 8192 (8 MiB), Android: 8192 (8 MiB)");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImplCVars_nosound::UnregisterVariables()
    {
        IConsole* const pConsole = gEnv->pConsole;
        AZ_Assert(pConsole, "NoSound Impl CVar Unregistration - IConsole is already null!");

        pConsole->UnregisterVariable("s_AudioImplementationPrimaryPoolSize");
    }
} // namespace Audio
