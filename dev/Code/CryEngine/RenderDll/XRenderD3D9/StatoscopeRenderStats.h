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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_STATOSCOPERENDERSTATS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_STATOSCOPERENDERSTATS_H
#pragma once


#if ENABLE_STATOSCOPE
class CD3D9Renderer;

class CGPUTimesDG
    : public IStatoscopeDataGroup
{
public:
    CGPUTimesDG(CD3D9Renderer* pRenderer);

    virtual SDescription GetDescription() const;
    virtual void Enable();
    virtual void Write(IStatoscopeFrameRecord& fr);

protected:
    CD3D9Renderer* m_pRenderer;
};

class CGraphicsDG
    : public IStatoscopeDataGroup
{
public:
    CGraphicsDG(CD3D9Renderer* pRenderer);

    virtual SDescription GetDescription() const;
    virtual void Write(IStatoscopeFrameRecord& fr);
    virtual void Enable();

protected:
    void ResetGPUUsageHistory();
    void TrackGPUUsage(float gpuLoad, float frameTimeMs, int totalDPs);

    static const int GPU_HISTORY_LENGTH = 10;
    static const int SCREEN_SHOT_FREQ = 60;
    static const int GPU_TIME_THRESHOLD_MS = 40;
    static const int DP_THRESHOLD = 2200;

    CD3D9Renderer* m_pRenderer;
    std::vector<float> m_gpuUsageHistory;
    std::vector<float> m_frameTimeHistory;
    int m_nFramesGPULmited;
    float m_totFrameTime;
    int m_lastFrameScreenShotRequested;
    int m_cvarScreenCapWhenGPULimited;
};

class CPerformanceOverviewDG
    : public IStatoscopeDataGroup
{
public:
    CPerformanceOverviewDG(CD3D9Renderer* pRenderer);

    virtual SDescription GetDescription() const;
    virtual void Write(IStatoscopeFrameRecord& fr);
    virtual void Enable();

protected:
    CD3D9Renderer* m_pRenderer;
};

#endif // ENABLE_STATOSCOPE

#endif // STATOSCOPERENDERSTATS_H
