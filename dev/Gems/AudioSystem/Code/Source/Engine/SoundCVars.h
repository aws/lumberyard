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

#include <IAudioSystem.h>

struct IConsoleCmdArgs;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CSoundCVars
    {
    public:
        CSoundCVars();
        ~CSoundCVars();

        CSoundCVars(const CSoundCVars&) = delete;           // Copy protection
        CSoundCVars& operator=(const CSoundCVars&) = delete; // Copy protection

        void RegisterVariables();
        void UnregisterVariables();

        int m_nATLPoolSize;
        int m_nFileCacheManagerSize;
        int m_nAudioObjectPoolSize;
        int m_nAudioEventPoolSize;
        int m_nAudioProxiesInitType;

        float m_fOcclusionMaxDistance;
        float m_fOcclusionMaxSyncDistance;
        float m_fFullObstructionMaxDistance;
        float m_fPositionUpdateThreshold;
        float m_fVelocityTrackingThreshold;

        float m_audioListenerTranslationZOffset;
        float m_audioListenerTranslationPercentage;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        int m_nIgnoreWindowFocus;
        int m_nDrawAudioDebug;
        int m_nFileCacheManagerDebugFilter;
        int m_nAudioLoggingOptions;
        int m_nShowActiveAudioObjectsOnly;
        ICVar* m_pAudioTriggersDebugFilter;
        ICVar* m_pAudioObjectsDebugFilter;

    private:
        static void CmdExecuteTrigger(IConsoleCmdArgs* pCmdArgs);
        static void CmdStopTrigger(IConsoleCmdArgs* pCmdArgs);
        static void CmdSetRtpc(IConsoleCmdArgs* pCmdArgs);
        static void CmdSetSwitchState(IConsoleCmdArgs* pCmdArgs);
        static void CmdPlayFile(IConsoleCmdArgs* pCmdArgs);
        static void CmdMicrophone(IConsoleCmdArgs* pCmdArgs);
        static void CmdPlayExternalSource(IConsoleCmdArgs* pCmdArgs);
        static void CmdSetPanningMode(IConsoleCmdArgs* pCmdArgs);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    };

    extern CSoundCVars g_audioCVars;
} // namespace Audio
