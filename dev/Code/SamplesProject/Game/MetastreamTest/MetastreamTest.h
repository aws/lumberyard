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

#include <AzCore/Component/TickBus.h>

namespace MetastreamTest
{
    class MetastreamTestHandler;
    typedef std::shared_ptr<MetastreamTestHandler> HandlerPtr;

    class MetastreamTestHandler : public AZ::TickBus::Handler
    {
    public:
        static HandlerPtr CreateInstance();

        MetastreamTestHandler();
        ~MetastreamTestHandler();

        void Update();

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        AZ::u64 GetFreeDriveSpace(const std::string & directoryName="") const;
        AZ::u64 GetMemoryLoad() const;
        double GetCPULoadProcess() const;
        double GetCPULoadSystem() const;
        void CalculateCPUUsage();

    private:
        static HandlerPtr   m_instance;
        double              m_lastUpdate;

        AZ::u64             m_cpuPreviousProcessKernelTime;
        AZ::u64             m_cpuPreviousProcessUserTime;
        AZ::u64             m_cpuPreviousSystemKernelTime;
        AZ::u64             m_cpuPreviousSystemUserTime;
        float               m_cpuProcessUsage;
        float               m_cpuSystemUsage;
    };
}
