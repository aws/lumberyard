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

// Description : Interface to the Subtitle Manager


#ifndef CRYINCLUDE_CRYACTION_ISUBTITLEMANAGER_H
#define CRYINCLUDE_CRYACTION_ISUBTITLEMANAGER_H
#pragma once

//////////////////////////////////////////////////////////////////////////
struct ISubtitleHandler
{
    virtual ~ISubtitleHandler(){}
    virtual void ShowSubtitle(const Audio::SAudioRequestInfo* const pAudioRequestInfo, bool bShow) = 0;
    virtual void ShowSubtitle(const char* subtitleLabel, bool bShow) = 0;
};

//////////////////////////////////////////////////////////////////////////
struct ISubtitleManager
{
    virtual ~ISubtitleManager(){}
    virtual void SetHandler(ISubtitleHandler* pHandler) = 0;

    // enables/disables subtitles manager
    virtual void SetEnabled(bool bEnabled) = 0;

    // automatic mode. Will inform the subtitleHandler about every executed/stopped audio trigger.
    // You can use this mode, if you want to drive your subtitles by started sounds and not manually.
    virtual void SetAutoMode(bool bOn) = 0;

    virtual void ShowSubtitle(const char* subtitleLabel, bool bShow) = 0;
};

#endif // CRYINCLUDE_CRYACTION_ISUBTITLEMANAGER_H
