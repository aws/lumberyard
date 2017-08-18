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

#ifndef EXCLUDE_OCULUS_SDK

#include "OculusLog.h"


//////////////////////////////////////////////////////////////////////////
// OculusLog


OculusLog::OculusLog()
    : m_pLog(0)
{
    m_pLog = gEnv->pLog;
}


OculusLog& OculusLog::GetAccess()
{
    static OculusLog s_inst;
    return s_inst;
}


OculusLog::~OculusLog()
{
}


void OculusLog::LogMessageVarg(OVR::LogMessageType messageType, const char* pFmt, va_list argList)
{
    char logBuf[1024];
    {
        const char prefix[] = "<Oculus> ";

        const size_t sizeLogBuf = sizeof(logBuf);
        const size_t lenPrefix = sizeof(prefix) - 1;

        char* pStr = logBuf;

        // prefix
        {
            COMPILE_TIME_ASSERT(lenPrefix < sizeLogBuf);

            _snprintf(pStr, sizeLogBuf, prefix);
            pStr += lenPrefix;
        }

        // msg
        {
            const size_t sizeMsg = sizeLogBuf - lenPrefix;

            int written = _vsnprintf(pStr, sizeMsg, pFmt, argList);
            if (written < 0 || written >= sizeMsg)
            {
                pStr[sizeMsg - 1] = '\0';
                written = sizeMsg - 1;
            }
            if (written && pStr[written - 1] == '\n')
            {
                pStr[written - 1] = '\0';
                --written;
            }
            pStr += (size_t) written;
        }
    }

    switch (messageType)
    {
    case OVR::Log_Error:
    {
        m_pLog->LogError("%s", logBuf);
        break;
    }
    default:
    {
        m_pLog->Log("%s", logBuf);
        break;
    }
    }
}


#endif // #ifndef EXCLUDE_OCULUS_SDK
