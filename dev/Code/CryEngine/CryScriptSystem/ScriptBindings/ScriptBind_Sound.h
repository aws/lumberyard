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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_SOUND_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_SOUND_H
#pragma once


#include <IScriptSystem.h>
#include <IAudioSystem.h>

/*! <remarks>These function will never be called from C-Code. They're script-exclusive.</remarks>*/
class CScriptBind_Sound
    : public CScriptableBase
{
public:

    CScriptBind_Sound(IScriptSystem* pScriptSystem, ISystem* pSystem);
    virtual ~CScriptBind_Sound();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    //! <code>Sound.GetAudioTriggerID(char const* const sTriggerName)</code>
    //! <description>Get the trigger TAudioControlID (wrapped into a ScriptHandle).</description>
    //!     <param name="sTriggerName">unique name of an audio trigger</param>
    //! <returns>ScriptHandle with the TAudioControlID value, or nil if the sTriggerName is not found.</returns>
    int GetAudioTriggerID(IFunctionHandler* pH, const char* const sTriggerName);

    //! <code>Sound.GetAudioSwitchID(char const* const sSwitchName)</code>
    //! <description>Get the switch TAudioControlID (wrapped into a ScriptHandle).</description>
    //!     <param name="sSwitchName">unique name of an audio switch</param>
    //! <returns>ScriptHandle with the TAudioControlID value, or nil if the sSwitchName is not found.</returns>
    int GetAudioSwitchID(IFunctionHandler* pH, const char* const sSwitchName);

    //! <code>Sound.GetAudioSwitchStateID(ScriptHandle const hSwitchID, char const* const sSwitchStateName)</code>
    //! <description>Get the SwitchState TAudioSwitchStatelID (wrapped into a ScriptHandle).</description>
    //!     <param name="sSwitchStateName">unique name of an audio switch state</param>
    //! <returns>ScriptHandle with the TAudioSwitchStateID value, or nil if the sSwitchStateName is not found.</returns>
    int GetAudioSwitchStateID(IFunctionHandler* pH, const ScriptHandle hSwitchID, const char* const sSwitchStateName);

    //! <code>Sound.GetAudioRtpcID(char const* const sRtpcName)</code>
    //! <description>Get the RTPC TAudioControlID (wrapped into a ScriptHandle).</description>
    //!     <param name="sRtpcName">unique name of an audio RTPC</param>
    //! <returns>ScriptHandle with the TAudioControlID value, or nil if the sRtpcName is not found.</returns>
    int GetAudioRtpcID(IFunctionHandler* pH, const char* const sRtpcName);

    //! <code>Sound.GetAudioEnvironmentID(char const* const sEnvironmentName)</code>
    //! <description>Get the Audio Environment TAudioEnvironmentID (wrapped into a ScriptHandle).</description>
    //!     <param name="sEnvironmentName">unique name of an Audio Environment</param>
    //! <returns>ScriptHandle with the TAudioEnvironmentID value, or nil if the sEnvironmentName is not found.</returns>
    int GetAudioEnvironmentID(IFunctionHandler* pH, const char* const sEnvironmentName);

    // ly-todo: update these docs to match others in this file
    // <title ExecuteGlobalAudioTrigger>
    // Syntax: Sound.ExecuteGlobalAudioTrigger(AudioUtils.LookupTriggerID("Play_sound"), false)
    // Description:
    //    Executes a global audio trigger without needing an entity
    // Arguments:
    //    hTriggerID - unique name of an Audio Trigger
    //    bStop - stop the audio trigger or don't stop audio trigger
    // Return:
    //    nothing
    int ExecuteGlobalAudioTrigger(IFunctionHandler* pH, const ScriptHandle hTriggerID, bool bStop);

private:
    Audio::SAudioRequest m_oRequest;
    Audio::SAudioObjectRequestData<Audio::eAORT_EXECUTE_TRIGGER> m_oExecuteRequestData;
    Audio::SAudioObjectRequestData<Audio::eAORT_STOP_TRIGGER> m_oStopRequestData;
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_SOUND_H
