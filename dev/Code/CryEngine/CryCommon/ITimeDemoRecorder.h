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

// Description : Provides interface for TimeDemo  CryAction
//               Can attach listener to perform game specific stuff on Record Play


#ifndef __ITIMEDEMORECORDER_H__
#define __ITIMEDEMORECORDER_H__

#pragma once


struct STimeDemoFrameRecord
{
    Vec3 playerPosition;
    Quat playerRotation;
    Quat playerViewRotation;
    Vec3 hmdPositionOffset;
    Quat hmdViewRotation;
};

struct ITimeDemoListener
{
    virtual ~ITimeDemoListener(){}
    virtual void OnRecord(bool bEnable) = 0;
    virtual void OnPlayback(bool bEnable) = 0;
    virtual void OnPlayFrame() = 0;
};

struct ITimeDemoRecorder
{
    virtual ~ITimeDemoRecorder(){}

    virtual bool IsRecording() const = 0;
    virtual bool IsPlaying() const = 0;

    virtual void RegisterListener(ITimeDemoListener* pListener) = 0;
    virtual void UnregisterListener(ITimeDemoListener* pListener) = 0;
    virtual void GetCurrentFrameRecord(STimeDemoFrameRecord& record) const = 0;
};


#endif //__ITIMEDEMORECORDER_H__
