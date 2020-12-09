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

#include <ScriptCanvas/Execution/ExecutionBus.h>

namespace ScriptCanvas
{
    class ExecutionPerformanceTimer
        : public ScriptCanvas::ExecutionTimingNotificationsBus::Handler
    {
    public:
        ExecutionPerformanceTimer();
        ~ExecutionPerformanceTimer();

        void ConnectTimer(const ScriptCanvasId& scriptCanvasId);
        void DisconnectTimer();
        void Reset();

        void OnInstantStart();
        void OnInstantStop();
        void OnReadyStart();
        void OnReadyStop();

        // ExecutionTimingNotificationsBus::Handler
        AZStd::sys_time_t GetInstantDurationInMicroseconds() override;
        double GetInstantDurationInMilliseconds() override;

        AZStd::sys_time_t GetLatentDurationInMicroseconds() override;
        double GetLatentDurationInMilliseconds() override;

        AZStd::sys_time_t GetReadyDurationInMicroseconds() override;
        double GetReadyDurationInMilliseconds() override;

        AZ::Event<size_t>* GetLatentStartTimerEvent() override;
        AZ::Event<size_t>* GetLatentStopTimerEvent() override;

        void OnLatentStart(size_t latentId) override;
        void OnLatentStop(size_t latentId) override;

    private:
        ScriptCanvasId m_scriptCanvasId;

        AZStd::sys_time_t m_readyTime;
        AZStd::chrono::system_clock::time_point m_readyStart;

        AZStd::sys_time_t m_instantTime;
        AZStd::chrono::system_clock::time_point m_instantStart;

        AZStd::atomic<AZStd::sys_time_t> m_latentTime;
        AZStd::unordered_map<size_t, AZStd::chrono::system_clock::time_point> m_latentStartTracker;

        AZ::Event<size_t> m_latentStartEvent;
        AZ::Event<size_t> m_latentStopEvent;

        AZ::Event<size_t>::Handler m_latentStartHandler;
        AZ::Event<size_t>::Handler m_latentStopHandler;
    };
}
