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
#include <CryAudioImplNoSound_Traits_Platform.h>

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

        m_nPrimaryPoolSize = AZ_TRAIT_CRYAUDIOIMPLNOSOUND_PRIMARY_POOL_SIZE;

        REGISTER_CVAR2("s_AudioImplementationPrimaryPoolSize", &m_nPrimaryPoolSize, m_nPrimaryPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the memory pool to be used by the Audio System Implementation."
            "Usage: s_AudioImplementationPrimaryPoolSize [0..]\n"
            "Default: " AZ_TRAIT_CRYAUDIOIMPLNOSOUND_PRIMARY_POOL_SIZE_DEFAULT_TEXT );
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImplCVars_nosound::UnregisterVariables()
    {
        IConsole* const pConsole = gEnv->pConsole;
        AZ_Assert(pConsole, "NoSound Impl CVar Unregistration - IConsole is already null!");

        pConsole->UnregisterVariable("s_AudioImplementationPrimaryPoolSize");
    }
} // namespace Audio
