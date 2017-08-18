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
#include "MetastreamTest.h"
#include <Metastream/MetastreamBus.h>
#include <AzCore/PlatformIncl.h>

namespace MetastreamTest
{
    const static double kUpdatePeriodMS = 100.0;          // in milliseconds
    const static std::string kDataBaseName("game");

    HandlerPtr MetastreamTestHandler::m_instance;

    HandlerPtr MetastreamTestHandler::CreateInstance()
    {
        if (m_instance == nullptr)
            m_instance = std::make_shared<MetastreamTestHandler>();

        return m_instance;
    }

    MetastreamTestHandler::MetastreamTestHandler()
        : m_lastUpdate(0.0f)
        , m_cpuPreviousProcessKernelTime(0)
        , m_cpuPreviousProcessUserTime(0)
        , m_cpuPreviousSystemKernelTime(0)
        , m_cpuPreviousSystemUserTime(0)
        , m_cpuProcessUsage(0.0f)
        , m_cpuSystemUsage(0.0f)
    {
        AZ::TickBus::Handler::BusConnect();
    }

    MetastreamTestHandler::~MetastreamTestHandler()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void MetastreamTestHandler::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        if (!gEnv || !gEnv->pGame)
        {
            return;
        }

        if (gEnv->pConsole)
        {
            const ICVar* enabled = gEnv->pConsole->GetCVar("metastream_enabled");
            if ( (enabled != nullptr) && (enabled->GetIVal() == 0) )
            {
                return;
            }
        }

        if ( (time.GetMilliseconds() - m_lastUpdate) > kUpdatePeriodMS)
        {
            CalculateCPUUsage();
            Update();
            m_lastUpdate = time.GetMilliseconds();
        }
    }

    void MetastreamTestHandler::Update()
    {
#ifdef AZ_PLATFORM_WINDOWS
        // show how to add values to a object, then add that object to the cache.
        EBUS_EVENT(Metastream::MetastreamRequestBus, AddUnsigned64ToObject, kDataBaseName.c_str(), "sysInfo", "drivespace", GetFreeDriveSpace());
        EBUS_EVENT(Metastream::MetastreamRequestBus, AddUnsigned64ToObject, kDataBaseName.c_str(), "sysInfo", "memoryload", GetMemoryLoad());
        EBUS_EVENT(Metastream::MetastreamRequestBus, AddDoubleToObject, kDataBaseName.c_str(), "sysInfo", "cpuloadsystem", GetCPULoadSystem());
        EBUS_EVENT(Metastream::MetastreamRequestBus, AddDoubleToObject, kDataBaseName.c_str(), "sysInfo", "cpuloadprocess", GetCPULoadProcess());
        EBUS_EVENT(Metastream::MetastreamRequestBus, AddUnsigned64ToObject, kDataBaseName.c_str(), "sysInfo", "tickcount", GetTickCount64());

        EBUS_EVENT(Metastream::MetastreamRequestBus, AddObjectToCache, kDataBaseName.c_str(), "systeminfo", "sysInfo");
#endif
    }

    AZ::u64 MetastreamTestHandler::GetFreeDriveSpace(const std::string & directoryName /*=""*/) const
    {
        AZ::u64 freeSpaceInBytes = 0;
        
#ifdef AZ_PLATFORM_WINDOWS
        ::GetDiskFreeSpaceEx(directoryName.empty() ? NULL : directoryName.c_str(), NULL, NULL, reinterpret_cast<PULARGE_INTEGER>(&freeSpaceInBytes));
#endif

        return freeSpaceInBytes;
    }

    AZ::u64 MetastreamTestHandler::GetMemoryLoad() const
    {
#ifdef AZ_PLATFORM_WINDOWS
        ::MEMORYSTATUSEX statex = { 0 };

        statex.dwLength = sizeof(statex);

        ::GlobalMemoryStatusEx(&statex);
        
        return static_cast<AZ::u64>(statex.dwMemoryLoad);
#else
        return 0;
#endif
    }

    double MetastreamTestHandler::GetCPULoadProcess() const
    {
        return static_cast<double>(m_cpuProcessUsage);
    }
        
    double MetastreamTestHandler::GetCPULoadSystem() const
    {
        return static_cast<double>(m_cpuSystemUsage);
    }

    void MetastreamTestHandler::CalculateCPUUsage()
    {
#ifdef AZ_PLATFORM_WINDOWS
        AZ::u64 systemIdleTime = 0;
        AZ::u64 systemKernelTime = 0;
        AZ::u64 systemUserTime = 0;
        AZ::u64 creationTime = 0;
        AZ::u64 exitTime = 0;
        AZ::u64 processKernelTime = 0;
        AZ::u64 processUserTime = 0;

        BOOL sysSuccess = ::GetSystemTimes(reinterpret_cast<LPFILETIME>(&systemIdleTime), reinterpret_cast<LPFILETIME>(&systemKernelTime), reinterpret_cast<LPFILETIME>(&systemUserTime));
        BOOL procSuccess = ::GetProcessTimes(::GetCurrentProcess(), reinterpret_cast<LPFILETIME>(&creationTime), reinterpret_cast<LPFILETIME>(&exitTime), reinterpret_cast<LPFILETIME>(&processKernelTime), reinterpret_cast<LPFILETIME>(&processUserTime));

        if (sysSuccess && procSuccess)
        {
            AZ::u64 totalSystemTime = (systemKernelTime - m_cpuPreviousSystemKernelTime) + (systemUserTime - m_cpuPreviousSystemUserTime);
           
            if (totalSystemTime > 0)
            {
                AZ::u64 totalProcessTime = (processKernelTime - m_cpuPreviousProcessKernelTime) + (processUserTime - m_cpuPreviousProcessUserTime);

                m_cpuProcessUsage = (100.0f * static_cast<float>(totalProcessTime)) / static_cast<float>(totalSystemTime);
                m_cpuSystemUsage = (100.0f * static_cast<float>(systemUserTime + systemKernelTime)) / static_cast<float>(systemIdleTime + systemKernelTime + systemUserTime);
            }

            m_cpuPreviousProcessKernelTime = processKernelTime;
            m_cpuPreviousProcessUserTime = processUserTime;
            m_cpuPreviousSystemKernelTime = systemKernelTime;
            m_cpuPreviousSystemUserTime = systemUserTime;
        }
#endif
    }

} // end namespace MetastreamTest