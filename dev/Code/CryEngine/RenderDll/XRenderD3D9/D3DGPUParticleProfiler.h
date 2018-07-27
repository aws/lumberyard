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
#pragma once

// enable GPU particle profiler by setting PROFILE_GPU_PARTICLES to 1
#define PROFILE_GPU_PARTICLES 0

class CGPUParticleProfilerData;
class CGPUParticleProfiler
{
public:
    enum GPUTimerIndex
    {
        RENDER,
        EMIT,
        BEGIN,
        UPDATE,
        GATHER_SORT,
        SORT_TOTAL,
        SORT_PASS,
        COUNT
    };

public:
    CGPUParticleProfiler();
    ~CGPUParticleProfiler();

    void Start(const int index);
    void Stop(const int index);
    const float GetTime(const int index);

    const char* const GetTimerIndexEnumName(const int index);

private:
    CGPUParticleProfilerData* m_particleProfilerData;
};

// defines to measure particle performance on the GPU, when we don't compile in profile mode, these defines will resolve to nothing
#if PROFILE_GPU_PARTICLES

// Defines
#define GPU_PARTICLE_PROFILER_START(emitterData, index)             emitterData->gpuParticleProfiler.Start(index)
#define GPU_PARTICLE_PROFILER_END(emitterData, index)               emitterData->gpuParticleProfiler.Stop(index)
#define GPU_PARTICLE_PROFILER_GET_TIME(emitterData, index)          emitterData->gpuParticleProfiler.GetTime(index)
#define GPU_PARTICLE_PROFILER_GET_INDEX_NAME(emitterData, index)    emitterData->gpuParticleProfiler.GetTimerIndexEnumName(index)

#else

#define GPU_PARTICLE_PROFILER_START(emitterData, index)
#define GPU_PARTICLE_PROFILER_END(emitterData, index)
#define GPU_PARTICLE_PROFILER_GET_TIME(emitterData, index)
#define GPU_PARTICLE_PROFILER_GET_INDEX_NAME(emitterData, index)

#endif