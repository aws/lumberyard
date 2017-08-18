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

#ifndef CRYINCLUDE_CRYCOMMON_IBUDGETINGSYSTEM_H
#define CRYINCLUDE_CRYCOMMON_IBUDGETINGSYSTEM_H
#pragma once


struct IBudgetingSystem
{
    // <interfuscator:shuffle>
    // set budget
    virtual void SetSysMemLimit(int sysMemLimitInMB) = 0;
    virtual void SetVideoMemLimit(int videoMemLimitInMB) = 0;
    virtual void SetFrameTimeLimit(float frameLimitInMS) = 0;
    virtual void SetSoundChannelsPlayingLimit(int soundChannelsPlayingLimit) = 0;
    virtual void SetSoundMemLimit(int SoundMemLimit) = 0;
    virtual void SetSoundCPULimit(int SoundCPULimit) = 0;
    virtual void SetNumDrawCallsLimit(int numDrawCallsLimit) = 0;
    virtual void SetStreamingThroughputLimit(float streamingThroughputLimit) = 0;
    virtual void SetBudget(int sysMemLimitInMB, int videoMemLimitInMB, float frameTimeLimitInMS, int soundChannelsPlayingLimit, int SoundMemLimitInMB, int SoundCPULimit, int numDrawCallsLimit) = 0;

    // get budget
    virtual int GetSysMemLimit() const = 0;
    virtual int GetVideoMemLimit() const = 0;
    virtual float GetFrameTimeLimit() const = 0;
    virtual int GetSoundChannelsPlayingLimit() const = 0;
    virtual int GetSoundMemLimit() const = 0;
    virtual int GetSoundCPULimit() const = 0;
    virtual int GetNumDrawCallsLimit() const = 0;
    virtual float GetStreamingThroughputLimit() const = 0;
    virtual void GetBudget(int& sysMemLimitInMB, int& videoMemLimitInMB, float& frameTimeLimitInMS, int& soundChannelsPlayingLimit, int& SoundMemLimitInMB, int& SoundCPULimitInPercent, int& numDrawCallsLimit) const = 0;

    // monitoring
    virtual void MonitorBudget() = 0;
    virtual void Render(float x, float y) = 0;

    // destruction
    virtual void Release() = 0;
    // </interfuscator:shuffle>

protected:
    virtual ~IBudgetingSystem() {}
};


#endif // CRYINCLUDE_CRYCOMMON_IBUDGETINGSYSTEM_H
