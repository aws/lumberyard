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

// Description : Subtitle Manager Implementation


#include "CryLegacy_precompiled.h"
#include "SubtitleManager.h"
#include "ISubtitleManager.h"
#include "DialogSystem/DialogActorContext.h"
#include <IAudioSystem.h>

CSubtitleManager* CSubtitleManager::s_Instance = nullptr;

//////////////////////////////////////////////////////////////////////////
CSubtitleManager::CSubtitleManager()
{
    m_bEnabled = false;
    m_bAutoMode = true;
    m_pHandler = NULL;

    s_Instance = this;
}

//////////////////////////////////////////////////////////////////////////
CSubtitleManager::~CSubtitleManager()
{
    SetEnabled(false);
    s_Instance = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::SetEnabled(bool bEnabled)
{
    if (bEnabled != m_bEnabled)
    {
        m_bEnabled = bEnabled;

        if (m_bAutoMode)
        {
            if (bEnabled)
            {
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
                    &CSubtitleManager::OnAudioTriggerStarted,
                    nullptr,
                    Audio::eART_AUDIO_OBJECT_REQUEST,
                    Audio::eAORT_EXECUTE_TRIGGER);

                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
                    &CSubtitleManager::OnAudioTriggerFinished,
                    nullptr,
                    Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST,
                    Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE);
            }
            else
            {
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener, &CSubtitleManager::OnAudioTriggerStarted, nullptr);
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener, &CSubtitleManager::OnAudioTriggerFinished, nullptr);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::SetAutoMode(bool bOn)
{
    if (bOn != m_bAutoMode)
    {
        if (m_bEnabled)
        {
            SetEnabled(false);  //force refresh for add/remove as audio listener
            m_bAutoMode = bOn;
            SetEnabled(true);
        }
        else
        {
            m_bAutoMode = bOn;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::ShowSubtitle(const char* subtitleLabel, bool bShow)
{
    if (m_bEnabled && m_pHandler)
    {
        m_pHandler->ShowSubtitle(subtitleLabel, bShow);
    }
}
//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::ShowSubtitle(const Audio::SAudioRequestInfo* const pAudioRequestInfo, bool bShow)
{
    if (m_bEnabled && m_pHandler)
    {
        m_pHandler->ShowSubtitle(pAudioRequestInfo, bShow);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::OnAudioTriggerStarted(const Audio::SAudioRequestInfo* const pAudioRequestInfo)
{
    CSubtitleManager::s_Instance->ShowSubtitle(pAudioRequestInfo, true);
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::OnAudioTriggerFinished(const Audio::SAudioRequestInfo* const pAudioRequestInfo)
{
    CSubtitleManager::s_Instance->ShowSubtitle(pAudioRequestInfo, false);
}


