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

#include "CryLegacy_precompiled.h"
#include "ScriptBind_Sound.h"
#include <IAudioSystem.h>

CScriptBind_Sound::CScriptBind_Sound(IScriptSystem* pScriptSystem, ISystem* pSystem)
{
    CScriptableBase::Init(pScriptSystem, pSystem);
    SetGlobalName("Sound");

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_Sound::

        // AudioSystem
    SCRIPT_REG_TEMPLFUNC(GetAudioTriggerID, "sTriggerName");
    SCRIPT_REG_TEMPLFUNC(GetAudioSwitchID, "sSwitchName");
    SCRIPT_REG_TEMPLFUNC(GetAudioSwitchStateID, "hSwitchID, sStateName");
    SCRIPT_REG_TEMPLFUNC(GetAudioRtpcID, "sRtpcName");
    SCRIPT_REG_TEMPLFUNC(GetAudioEnvironmentID, "sEnvironmentName");
    SCRIPT_REG_TEMPLFUNC(ExecuteGlobalAudioTrigger, "hTriggerID, bStop");
}

//////////////////////////////////////////////////////////////////////////
CScriptBind_Sound::~CScriptBind_Sound()
{
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioTriggerID(IFunctionHandler* pH, const char* const sTriggerName)
{
    if (sTriggerName && (sTriggerName[0] != '\0'))
    {
        Audio::TAudioControlID nTriggerID = INVALID_AUDIO_CONTROL_ID;
        Audio::AudioSystemRequestBus::BroadcastResult(nTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, sTriggerName);
        if (nTriggerID != INVALID_AUDIO_CONTROL_ID)
        {
            // ID retrieved successfully
            return pH->EndFunction(IntToHandle(nTriggerID));
        }
        else
        {
            return pH->EndFunction();
        }
    }

    return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioSwitchID(IFunctionHandler* pH, const char* const sSwitchName)
{
    if (sSwitchName && (sSwitchName[0] != '\0'))
    {
        Audio::TAudioControlID nSwitchID = INVALID_AUDIO_CONTROL_ID;
        Audio::AudioSystemRequestBus::BroadcastResult(nSwitchID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchID, sSwitchName);
        if (nSwitchID != INVALID_AUDIO_CONTROL_ID)
        {
            // ID retrieved successfully
            return pH->EndFunction(IntToHandle(nSwitchID));
        }
        else
        {
            return pH->EndFunction();
        }
    }

    return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioSwitchStateID(IFunctionHandler* pH, const ScriptHandle hSwitchID, const char* const sSwitchStateName)
{
    if (sSwitchStateName && (sSwitchStateName[0] != '\0'))
    {
        Audio::TAudioSwitchStateID nSwitchStateID = INVALID_AUDIO_SWITCH_STATE_ID;
        Audio::TAudioControlID nSwitchID = HandleToInt<Audio::TAudioControlID>(hSwitchID);
        Audio::AudioSystemRequestBus::BroadcastResult(nSwitchStateID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, nSwitchID, sSwitchStateName);
        if (nSwitchStateID != INVALID_AUDIO_SWITCH_STATE_ID)
        {
            // ID retrieved successfully
            return pH->EndFunction(IntToHandle(nSwitchStateID));
        }
        else
        {
            return pH->EndFunction();
        }
    }

    return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioRtpcID(IFunctionHandler* pH, const char* const sRtpcName)
{
    if (sRtpcName && (sRtpcName[0] != '\0'))
    {
        Audio::TAudioControlID nRtpcID = INVALID_AUDIO_CONTROL_ID;
        Audio::AudioSystemRequestBus::BroadcastResult(nRtpcID, &Audio::AudioSystemRequestBus::Events::GetAudioRtpcID, sRtpcName);
        if (nRtpcID != INVALID_AUDIO_CONTROL_ID)
        {
            // ID retrieved successfully
            return pH->EndFunction(IntToHandle(nRtpcID));
        }
        else
        {
            return pH->EndFunction();
        }
    }

    return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioEnvironmentID(IFunctionHandler* pH, const char* const sEnvironmentName)
{
    if (sEnvironmentName && (sEnvironmentName[0] != '\0'))
    {
        Audio::TAudioEnvironmentID nEnvironmentID = INVALID_AUDIO_ENVIRONMENT_ID;
        Audio::AudioSystemRequestBus::BroadcastResult(nEnvironmentID, &Audio::AudioSystemRequestBus::Events::GetAudioEnvironmentID, sEnvironmentName);
        if (nEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID)
        {
            // ID retrieved successfully
            return pH->EndFunction(IntToHandle(nEnvironmentID));
        }
        else
        {
            return pH->EndFunction();
        }
    }

    return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::ExecuteGlobalAudioTrigger(IFunctionHandler* pH, const ScriptHandle hTriggerID, bool bStop)
{
    if (!bStop)
    {
        m_oExecuteRequestData.nTriggerID = HandleToInt<Audio::TAudioControlID>(hTriggerID);
        m_oRequest.pData = &m_oExecuteRequestData;
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_oRequest);
    }
    else
    {
        m_oStopRequestData.nTriggerID = HandleToInt<Audio::TAudioControlID>(hTriggerID);
        m_oRequest.pData = &m_oStopRequestData;
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, m_oRequest);
    }

    return pH->EndFunction();
}
