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

#include "StdAfx.h"
#include "GPUTimer.h"
#include "DriverD3D.h"
#include <AzCore/Debug/EventTraceDrillerBus.h>

bool CSimpleGPUTimer::s_bTimingEnabled = false;
bool CSimpleGPUTimer::s_bTimingAllowed = true;

namespace EventTrace
{
    const AZStd::thread_id GpuThreadId{ (AZStd::native_thread_id_type)1 };
    const char* GpuThreadName = "GPU";
    const char* GpuCategory = "GPU";
}

void CSimpleGPUTimer::EnableTiming()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
    if (!s_bTimingEnabled && s_bTimingAllowed)
    {
        s_bTimingEnabled = true;
    }
#endif
}


void CSimpleGPUTimer::DisableTiming()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
    if (s_bTimingEnabled)
    {
        s_bTimingEnabled = false;
    }
#endif
}


void CSimpleGPUTimer::AllowTiming()
{
    s_bTimingAllowed = true;
}


void CSimpleGPUTimer::DisallowTiming()
{
    s_bTimingAllowed = false;
    if (gcpRendD3D->m_pPipelineProfiler)
    {
        gcpRendD3D->m_pPipelineProfiler->ReleaseGPUTimers();
    }
    DisableTiming();
}


CSimpleGPUTimer::CSimpleGPUTimer()
{
    m_time = 0.f;
    m_smoothedTime = 0.f;
    m_bInitialized = false;
    m_bWaiting = false;
    m_bStarted = false;

#if GPUTIMER_CPP_TRAIT_CSIMPLEGPUTIMER_SETQUERYSTART
    m_pQueryStart = m_pQueryStop = m_pQueryFreq = NULL;
#endif
}


CSimpleGPUTimer::~CSimpleGPUTimer()
{
    Release();
}

void CSimpleGPUTimer::Release()
{
#if GPUTIMER_CPP_TRAIT_RELEASE_RELEASEQUERY
    SAFE_RELEASE(m_pQueryStart);
    SAFE_RELEASE(m_pQueryStop);
    SAFE_RELEASE(m_pQueryFreq);
#endif
    m_bInitialized = false;
    m_bWaiting = false;
    m_bStarted = false;
    m_smoothedTime = 0.f;
}

bool CSimpleGPUTimer::Init()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
    if (!m_bInitialized)
    {
    #if GPUTIMER_CPP_TRAIT_INIT_CREATEQUERY
        D3D11_QUERY_DESC stampDisjointDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
        D3D11_QUERY_DESC stampDesc = { D3D11_QUERY_TIMESTAMP, 0 };

        if (gcpRendD3D->GetDevice().CreateQuery(&stampDisjointDesc, &m_pQueryFreq) == S_OK &&
            gcpRendD3D->GetDevice().CreateQuery(&stampDesc, &m_pQueryStart) == S_OK &&
            gcpRendD3D->GetDevice().CreateQuery(&stampDesc, &m_pQueryStop) == S_OK)
        {
            m_bInitialized = true;
        }
    #endif

        EBUS_EVENT(AZ::Debug::EventTraceDrillerSetupBus, SetThreadName, EventTrace::GpuThreadId, EventTrace::GpuThreadName);
    }
#endif

    return m_bInitialized;
}


void CSimpleGPUTimer::Start(const char* name)
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
    if (s_bTimingEnabled && !m_bWaiting && Init())
    {
        m_Name = name;

    #if GPUTIMER_CPP_TRAIT_START_PRIMEQUERY
        // TODO: The d3d docs suggest that the disjoint query should be done once per frame at most
        gcpRendD3D->GetDeviceContext().Begin(m_pQueryFreq);
        gcpRendD3D->GetDeviceContext().End(m_pQueryStart);
        m_bStarted = true;
    #endif
    }
#endif
}

void CSimpleGPUTimer::Stop()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
    if (s_bTimingEnabled && m_bStarted && m_bInitialized)
    {
    #if GPUTIMER_CPP_TRAIT_STOP_ENDQUERY
        gcpRendD3D->GetDeviceContext().End(m_pQueryStop);
        gcpRendD3D->GetDeviceContext().End(m_pQueryFreq);
        m_bStarted = false;
        m_bWaiting = true;
    #endif
    }
#endif
}

void CSimpleGPUTimer::UpdateTime()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
    if (s_bTimingEnabled && m_bWaiting && m_bInitialized)
    {
    #if GPUTIMER_CPP_TRAIT_UPDATETIME_GETDATA
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
        UINT64 timeStart, timeStop;

        ID3D11DeviceContext& context = gcpRendD3D->GetDeviceContext();
        if (context.GetData(m_pQueryFreq, &disjointData, m_pQueryFreq->GetDataSize(), 0) == S_OK &&
            context.GetData(m_pQueryStart, &timeStart, m_pQueryStart->GetDataSize(), 0) == S_OK &&
            context.GetData(m_pQueryStop, &timeStop, m_pQueryStop->GetDataSize(), 0) == S_OK)
        {
            if (!disjointData.Disjoint)
            {
                float time = (timeStop - timeStart) / (float)(disjointData.Frequency / 1000);
                if (time >= 0.f && time < 1000.f)
                {
                    m_time = time;                                 // Filter out wrong insane values that get reported sometimes

                    RecordSlice(timeStart, timeStop, disjointData.Frequency);
                }
            }
            m_bWaiting = false;
        }
    #endif

        if (!m_bWaiting)
        {
            m_smoothedTime = m_smoothedTime * 0.7f + m_time * 0.3f;
        }
    }
    else
    {
        // Reset timers when not used in frame
        m_time = 0.0f;
        m_smoothedTime = 0.0f;
    }
#endif
}

void CSimpleGPUTimer::RecordSlice(AZ::u64 timeStart, AZ::u64 timeStop, AZ::u64 frequency)
{
#if defined(CRY_USE_DX12)
    {
        AZ::u64 cpuTimeStart = gcpRendD3D->GetDeviceContext().MakeCpuTimestampMicroseconds(timeStart);
        AZ::u32 durationInMicroseconds = (AZ::u32)((1000000 * (timeStop - timeStart)) / frequency);

        // Cannot queue event because string is not deep copied (and lifetime is not guaranteed until next update).
        EBUS_EVENT(AZ::Debug::EventTraceDrillerBus, RecordSlice, m_Name.c_str(), EventTrace::GpuCategory, EventTrace::GpuThreadId, cpuTimeStart, durationInMicroseconds);
    }
#endif
}
