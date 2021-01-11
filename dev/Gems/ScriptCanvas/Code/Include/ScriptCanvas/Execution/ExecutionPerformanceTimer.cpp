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
#include "ExecutionPerformanceTimer.h"

namespace ScriptCanvas
{
    ExecutionPerformanceTimer::ExecutionPerformanceTimer()
        : m_latentStartHandler([this](size_t id) { this->OnLatentStart(id); })
        , m_latentStopHandler([this](size_t id) { this->OnLatentStop(id); })
    {
        m_readyStart = m_instantStart = AZStd::chrono::system_clock::time_point::max();
        m_readyTime = m_instantTime = m_latentTime = 0;
        m_scriptCanvasId = AZ::EntityId();
    }

    ExecutionPerformanceTimer::~ExecutionPerformanceTimer()
    {
        DisconnectTimer();
    }

    void ExecutionPerformanceTimer::ConnectTimer(const ScriptCanvasId& scriptCanvasId)
    {
        m_scriptCanvasId = scriptCanvasId;
        m_latentStartHandler.Connect(m_latentStartEvent);
        m_latentStopHandler.Connect(m_latentStopEvent);
        ExecutionTimingNotificationsBus::Handler::BusConnect(m_scriptCanvasId);
    }

    void ExecutionPerformanceTimer::DisconnectTimer()
    {
        m_latentStartHandler.Disconnect();
        m_latentStopHandler.Disconnect();
        ExecutionTimingNotificationsBus::Handler::BusDisconnect(m_scriptCanvasId);
    }

    void ExecutionPerformanceTimer::Reset()
    {
        m_readyStart = m_instantStart = AZStd::chrono::system_clock::time_point::max();
        m_readyTime = m_instantTime = m_latentTime = 0;
        m_scriptCanvasId = AZ::EntityId();
        m_latentStartTracker.clear();
    }

    AZStd::sys_time_t ExecutionPerformanceTimer::GetInstantDurationInMicroseconds()
    {
        return m_instantTime;
    }

    double ExecutionPerformanceTimer::GetInstantDurationInMilliseconds()
    {
        return m_instantTime / 1000.0;
    }

    AZStd::sys_time_t ExecutionPerformanceTimer::GetLatentDurationInMicroseconds()
    {
        return m_latentTime.load();
    }

    double ExecutionPerformanceTimer::GetLatentDurationInMilliseconds()
    {
        return m_latentTime.load() / 1000.0;
    }

    AZStd::sys_time_t ExecutionPerformanceTimer::GetReadyDurationInMicroseconds()
    {
        return m_readyTime;
    }

    double ExecutionPerformanceTimer::GetReadyDurationInMilliseconds()
    {
        return m_readyTime / 1000.0;
    }

    AZ::Event<size_t>* ExecutionPerformanceTimer::GetLatentStartTimerEvent()
    {
#if defined(SC_EXECUTION_TRACE_ENABLED)
        return &m_latentStartEvent;
#else
        return nullptr;
#endif
    }

    AZ::Event<size_t>* ExecutionPerformanceTimer::GetLatentStopTimerEvent()
    {
#if defined(SC_EXECUTION_TRACE_ENABLED)
        return &m_latentStopEvent;
#else
        return nullptr;
#endif
    }

    void ExecutionPerformanceTimer::OnInstantStart()
    {
#if defined(SC_EXECUTION_TRACE_ENABLED)
        m_instantStart = AZStd::chrono::system_clock::now();
#endif
    }

    void ExecutionPerformanceTimer::OnInstantStop()
    {
#if defined(SC_EXECUTION_TRACE_ENABLED)
        const auto instantStop = AZStd::chrono::system_clock::now();
        if (instantStop >= m_instantStart)
        {
            m_instantTime += AZStd::chrono::microseconds(instantStop - m_instantStart).count();
        }
        m_instantStart = AZStd::chrono::system_clock::time_point::max();
#endif
    }

    void ExecutionPerformanceTimer::OnLatentStart(size_t latentId)
    {
#if defined(SC_EXECUTION_TRACE_ENABLED)
        const auto latentStart = AZStd::chrono::system_clock::now();
        auto latentTimingIter = m_latentStartTracker.find(latentId);
        if (latentTimingIter != m_latentStartTracker.end())
        {
            latentTimingIter->second = latentStart;
        }
        else
        {
            m_latentStartTracker.emplace(latentId, latentStart);
        }
#else
        AZ_UNUSED(latentId);
#endif
    }

    void ExecutionPerformanceTimer::OnLatentStop(size_t latentId)
    {
#if defined(SC_EXECUTION_TRACE_ENABLED)
        const auto latentStop = AZStd::chrono::system_clock::now();
        auto latentTimingIter = m_latentStartTracker.find(latentId);
        if (latentTimingIter != m_latentStartTracker.end())
        {
            if (latentStop > latentTimingIter->second)
            {
                const auto latentTime = AZStd::chrono::microseconds(latentStop - latentTimingIter->second).count();
                m_latentTime += latentTime;
            }
            m_latentStartTracker.erase(latentTimingIter);
        }
#else
        AZ_UNUSED(latentId);
#endif
    }

    void ExecutionPerformanceTimer::OnReadyStart()
    {
#if defined(SC_EXECUTION_TRACE_ENABLED)
        m_readyStart = AZStd::chrono::system_clock::now();
#endif
    }

    void ExecutionPerformanceTimer::OnReadyStop()
    {
#if defined(SC_EXECUTION_TRACE_ENABLED)
        const auto readyStop = AZStd::chrono::system_clock::now();
        if (readyStop >= m_readyStart)
        {
            m_readyTime += AZStd::chrono::microseconds(readyStop - m_readyStart).count();
        }
        m_readyStart = AZStd::chrono::system_clock::time_point::max();
#endif
    }
}
