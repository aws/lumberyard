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

#ifndef CRYINCLUDE_CRYSYSTEM_BUDGETINGSYSTEM_H
#define CRYINCLUDE_CRYSYSTEM_BUDGETINGSYSTEM_H
#pragma once



#include <IBudgetingSystem.h>


struct IRenderer;
struct IRenderAuxGeom;
struct ITimer;
namespace Audio {
    struct IAudioSystem;
}
struct IStreamEngine;


class CBudgetingSystem
    : public IBudgetingSystem
{
public:
    CBudgetingSystem();

    // interface
    virtual void SetSysMemLimit(int sysMemLimitInMB);
    virtual void SetVideoMemLimit(int videoMemLimitInMB);
    virtual void SetFrameTimeLimit(float frameLimitInMS);
    virtual void SetSoundChannelsPlayingLimit(int soundChannelsPlayingLimit);
    virtual void SetSoundMemLimit(int SoundMemLimit);
    virtual void SetSoundCPULimit(int SoundCPULimit);
    virtual void SetNumDrawCallsLimit(int numDrawCallsLimit);
    virtual void SetStreamingThroughputLimit(float streamingThroughputLimit);
    virtual void SetBudget(int sysMemLimitInMB, int videoMemLimitInMB,
        float frameTimeLimitInMS, int soundChannelsPlayingLimit, int SoundMemLimitInMB, int SoundCPULimit, int numDrawCallsLimit);

    virtual int GetSysMemLimit() const;
    virtual int GetVideoMemLimit() const;
    virtual float GetFrameTimeLimit() const;
    virtual int GetSoundChannelsPlayingLimit() const;
    virtual int GetSoundMemLimit() const;
    virtual int GetSoundCPULimit() const;
    virtual int GetNumDrawCallsLimit() const;
    virtual float GetStreamingThroughputLimit() const;
    virtual void GetBudget(int& sysMemLimitInMB, int& videoMemLimitInMB,
        float& frameTimeLimitInMS, int& soundChannelsPlayingLimit, int& SoundMemLimitInMB, int& SoundCPULimitInPercent, int& numDrawCallsLimit) const;

    virtual void MonitorBudget();
    virtual void Render(float x, float y);

    virtual void Release();

protected:
    virtual ~CBudgetingSystem();

    void DrawText(float& x, float& y, float* pColor, const char* format, ...) PRINTF_PARAMS(5, 6);

    void MonitorSystemMemory(float& x, float& y);
    void MonitorVideoMemory(float& x, float& y);
    void MonitorFrameTime(float& x, float& y);
    void MonitorSoundChannels(float& x, float& y);
    void MonitorSoundMemory(float& x, float& y);
    void MonitorSoundCPU(float& x, float& y);
    void MonitorDrawCalls(float& x, float& y);
    void MonitorPolyCount(float& x, float& y);
    void MonitorStreaming(float& x, float& y);

    void GetColor(float scale, float* pColor);
    void DrawMeter(float& x, float& y, float scale);

    void RegisterWithPerfHUD();

    //static perfhud render callback
    static void PerfHudRender(float x, float y);

protected:
    IRenderer* m_pRenderer;
    IRenderAuxGeom* m_pAuxRenderer;
    ITimer* m_pTimer;
    IStreamEngine* m_pStreamEngine;

    int m_sysMemLimitInMB;
    int m_videoMemLimitInMB;
    float m_frameTimeLimitInMS;
    int m_soundChannelsPlayingLimit;
    int m_soundMemLimitInMB;
    int m_soundCPULimitInPercent;
    int m_numDrawCallsLimit;
    int m_numPolysLimit;
    float m_streamingThroughputLimit;
    int m_width;
    int m_height;
};


#endif // CRYINCLUDE_CRYSYSTEM_BUDGETINGSYSTEM_H
