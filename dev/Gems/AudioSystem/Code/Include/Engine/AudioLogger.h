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

#include <platform.h>
#include <IConsole.h>
#include <ISystem.h>
#include <ITimer.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioLogType
    {
        eALT_ERROR = 0,
        eALT_WARNING,
        eALT_COMMENT,
        eALT_ALWAYS,
        eALT_FATAL,
    };

    enum EAudioLoggingOptions
    {
        eALO_NONE     = 0,
        eALO_ERRORS   = BIT(6), // a
        eALO_WARNINGS = BIT(7), // b
        eALO_COMMENTS = BIT(8), // c
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title CAudioLogger>
    // Summary:
    //      A simpler logger wrapper that adds and audio tag and a timestamp
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioLogger
    {
    public:
        CAudioLogger() = default;
        ~CAudioLogger() = default;

        CAudioLogger(const CAudioLogger&) = delete;         // Copy protection
        CAudioLogger& operator=(const CAudioLogger&) = delete; // Copy protection

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title Log>
        // Summary:
        //      Log a message
        // Arguments:
        //      eType        - log message type (eALT_COMMENT, eALT_WARNING, eALT_ERROR or eALT_ALWAYS)
        //      sFormat, ... - printf-style format string and its arguments
        ///////////////////////////////////////////////////////////////////////////////////////////////
        void Log(const EAudioLogType eType, const char* const sFormat, ...) const
        {
        #if defined(ENABLE_AUDIO_LOGGING)
            if (!gEnv->pConsole || !gEnv->pTimer || !gEnv->pLog)
            {
                return;
            }

            const ICVar* cVar = gEnv->pConsole->GetCVar("s_AudioLoggingOptions");
            if (!cVar)
            {
                return;
            }

            if (sFormat && sFormat[0] != '\0' && gEnv->pLog->GetVerbosityLevel() != -1)
            {
                FRAME_PROFILER("CAudioLogger::Log", GetISystem(), PROFILE_AUDIO);

                char sBuffer[MAX_PATH];
                va_list ArgList;
                va_start(ArgList, sFormat);
                azvsnprintf(sBuffer, sizeof(sBuffer), sFormat, ArgList);
                sBuffer[sizeof(sBuffer) - 1] = '\0';
                va_end(ArgList);

                float fCurrTime = gEnv->pTimer->GetAsyncCurTime();

                const auto audioLoggingVerbosity = static_cast<EAudioLoggingOptions>(cVar->GetIVal());

                #define AUDIO_LOG_FORMAT    "<Audio:%.3f> %s"

                switch (eType)
                {
                    case eALT_ERROR:
                    {
                        if (audioLoggingVerbosity & eALO_ERRORS)
                        {
                            gEnv->pSystem->Warning(VALIDATOR_MODULE_AUDIO, VALIDATOR_ERROR, (VALIDATOR_FLAG_AUDIO | VALIDATOR_FLAG_IGNORE_IN_EDITOR), nullptr,
                                AUDIO_LOG_FORMAT, fCurrTime, sBuffer);
                        }
                        break;
                    }
                    case eALT_WARNING:
                    {
                        if (audioLoggingVerbosity & eALO_WARNINGS)
                        {
                            gEnv->pSystem->Warning(VALIDATOR_MODULE_AUDIO, VALIDATOR_WARNING, (VALIDATOR_FLAG_AUDIO | VALIDATOR_FLAG_IGNORE_IN_EDITOR), nullptr,
                                AUDIO_LOG_FORMAT, fCurrTime, sBuffer);
                        }
                        break;
                    }
                    case eALT_COMMENT:
                    {
                        if (audioLoggingVerbosity & eALO_COMMENTS)
                        {
                            // Use CryLog, which is tied to System verbosity level 3 (Messages)
                            CryLog(AUDIO_LOG_FORMAT, fCurrTime, sBuffer);
                        }
                        break;
                    }
                    case eALT_ALWAYS:
                    {
                        CryLogAlways(AUDIO_LOG_FORMAT, fCurrTime, sBuffer);
                        break;
                    }
                    case eALT_FATAL:
                    {
                        CryFatalError(AUDIO_LOG_FORMAT, fCurrTime, sBuffer);
                        break;
                    }
                    default:
                    {
                        AZ_ErrorOnce("AudioLogger", false, "Attempted to use Audio Logger with an invalid log type!");
                        break;
                    }
                }
            }
        #endif // ENABLE_AUDIO_LOGGING
        }
    };
} // namespace Audio
