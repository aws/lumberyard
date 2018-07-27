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


#ifndef CRYINCLUDE_CRYACTION_SUBTITLEMANAGER_H
#define CRYINCLUDE_CRYACTION_SUBTITLEMANAGER_H
#pragma once

#include "ISubtitleManager.h"

class CSubtitleManager
    : public ISubtitleManager
{
public:
    CSubtitleManager();
    virtual ~CSubtitleManager();

    // ISubtitleManager
    virtual void SetHandler(ISubtitleHandler* pHandler) { m_pHandler = pHandler; }
    virtual void SetEnabled(bool bEnabled);
    virtual void SetAutoMode(bool bOn);
    virtual void ShowSubtitle(const char* subtitleLabel, bool bShow);
    // ~ISubtitleManager

    static void OnAudioTriggerStarted(const Audio::SAudioRequestInfo* const pAudioRequestInfo);
    static void OnAudioTriggerFinished(const Audio::SAudioRequestInfo* const pAudioRequestInfo);

protected:
    void ShowSubtitle(const Audio::SAudioRequestInfo* const pAudioRequestInfo, bool bShow);

    ISubtitleHandler* m_pHandler;
    bool m_bEnabled;
    bool m_bAutoMode;

    static CSubtitleManager* s_Instance;
};

#endif // CRYINCLUDE_CRYACTION_SUBTITLEMANAGER_H
