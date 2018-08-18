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
#include "StdAfx.h"
#include "D3DGPUParticleProfiler.h"
#include "GPUTimer.h"

// Constants
static const int s_GPUParticles_GPUTimersCount = CGPUParticleProfiler::COUNT;
static const int s_GPUParticles_GPUTimersFrames = 2;
static const int s_GPUParticles_GPUTimersTotalBufferCount = s_GPUParticles_GPUTimersCount * s_GPUParticles_GPUTimersFrames;

#define GPU_PARTICLE_PROFILER_CURRENT_FRAME ((timersCurrentFrame + 1) % (s_GPUParticles_GPUTimersFrames - 1))
#define GPU_PARTICLE_PROFILER_PREVIOUS_FRAME ((timersCurrentFrame) % (s_GPUParticles_GPUTimersFrames - 1))
#define GPU_PROFILER_PREVIOUS_INDEX(index) (index + GPU_PARTICLE_PROFILER_PREVIOUS_FRAME * s_GPUParticles_GPUTimersCount)
#define GPU_PROFILER_CURRENT_INDEX(index) (index + GPU_PARTICLE_PROFILER_CURRENT_FRAME * s_GPUParticles_GPUTimersCount)

static const char* s_GPUParticles_GPUTimerIndexes[] = { "GPU RENDER", "GPU EMIT", "GPU BEGIN", "GPU UPDATE", "GPU GATHER SORT", "GPU SORT TOTAL", "GPU SORT PASS" };


class CGPUParticleProfilerData
{
public:
    CGPUParticleProfilerData()
    {
        // Regardless of state, GPU Particle Profiler can only sample performance when GPU timers are enabled.
        // Because profiling has a specific use-case (and not intended for regular operation), 
        // we explicitly enable timers as a design-exception.
        CD3DProfilingGPUTimer::EnableTiming();

        timersCurrentFrame = 0;
        for (unsigned int i = 0; i < s_GPUParticles_GPUTimersTotalBufferCount; ++i)
        {
            timers[i].Init();
        }
        for (unsigned int i = 0; i < s_GPUParticles_GPUTimersFrames; ++i)
        {
            timersActive[i] = 0;
        }
    }

    ~CGPUParticleProfilerData()
    {
        for (unsigned int i = 0; i < s_GPUParticles_GPUTimersTotalBufferCount; ++i)
        {
            timers[i].Release();
        }
    }

    FORCEINLINE void Start(const int index)
    {
        SetCurrentBufferTimerActive(index);
        timers[GPU_PROFILER_CURRENT_INDEX(index)].Start(s_GPUParticles_GPUTimerIndexes[index]);
    }

    FORCEINLINE void Stop(const int index)
    {
        timers[GPU_PROFILER_CURRENT_INDEX(index)].Stop();
    }

    FORCEINLINE void SetCurrentBufferTimerActive(const int index)
    {
        timersActive[GPU_PARTICLE_PROFILER_CURRENT_FRAME] |= (1 << index);
    }

    FORCEINLINE void SetPreviousBufferTimerInactive(const int index)
    {
        timersActive[GPU_PARTICLE_PROFILER_PREVIOUS_FRAME] &= (~(1 << index));
    }

    FORCEINLINE bool IsPreviousBufferTimerActive(const int index)
    {
        return (timersActive[GPU_PARTICLE_PROFILER_PREVIOUS_FRAME] & (1 << index)) != 0;
    }

    FORCEINLINE const float GetTime(const int index)
    {
        if (IsPreviousBufferTimerActive(index))
        {
            const int previousIndex = GPU_PROFILER_PREVIOUS_INDEX(index);
            while (timers[previousIndex].HasPendingQueries())
            {
                timers[previousIndex].UpdateTime();
            }

            return timers[previousIndex].GetTime();
        }

        timersCurrentFrame++;

        return 0.0f;
    }

private:
    CD3DProfilingGPUTimer timers[s_GPUParticles_GPUTimersTotalBufferCount];
    int timersActive[s_GPUParticles_GPUTimersFrames];
    int timersCurrentFrame;
};

CGPUParticleProfiler::CGPUParticleProfiler()
{
    m_particleProfilerData = new CGPUParticleProfilerData();
}

CGPUParticleProfiler::~CGPUParticleProfiler()
{
    delete m_particleProfilerData;
}

void CGPUParticleProfiler::Start(const int index)
{
    m_particleProfilerData->Start(index);
}

void CGPUParticleProfiler::Stop(const int index)
{
    m_particleProfilerData->Stop(index);
}

const float CGPUParticleProfiler::GetTime(const int index)
{
    return m_particleProfilerData->GetTime(index);
}

const char* const CGPUParticleProfiler::GetTimerIndexEnumName(const int index)
{
    return s_GPUParticles_GPUTimerIndexes[index];
}