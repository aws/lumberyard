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

#ifndef AZCORELOGSINK_HEADER
#define AZCORELOGSINK_HEADER

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/containers/unordered_map.h>

#include <CryAssert.h>

/**
 * Hook Trace bus so we can funnel AZ asserts, warnings, etc to CryEngine.
 *
 * Note: This is currently owned by CrySystem, because CrySystem owns
 * the logging mechanism for which it is relevant.
 */
class AZCoreLogSink
    : public AZ::Debug::TraceMessageBus::Handler
{
public:

    inline static void Connect()
    {
        GetInstance().m_ignoredAsserts = new IgnoredAssertMap();
        GetInstance().BusConnect();
    }

    inline static void Disconnect()
    {
        GetInstance().BusDisconnect();
        delete GetInstance().m_ignoredAsserts;
    }

    static AZCoreLogSink& GetInstance()
    {
        static AZCoreLogSink s_sink;
        return s_sink;
    }

    static bool IsCryLogReady()
    {
        return gEnv && gEnv->pSystem && gEnv->pLog;
    }

    bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override
    {
        if (!IsCryLogReady())
        {
            return false;
        }

        CryLogAlways("%s", message);

#if defined(USE_CRY_ASSERT) && (defined(WIN32) || defined(DURANGO) || defined(APPLE) || defined(LINUX))
        AZ::Crc32 crc;
        crc.Add(&line, sizeof(line));
        if (fileName)
        {
            crc.Add(fileName, strlen(fileName));
        }

        bool* ignore = nullptr;
        auto foundIter = m_ignoredAsserts->find(crc);
        if (foundIter == m_ignoredAsserts->end())
        {
            ignore = &((*m_ignoredAsserts)[crc]);
            *ignore = false;
        }
        else
        {
            ignore = &((*m_ignoredAsserts)[crc]);
        }

        if (!(*ignore))
        {
            CryAssertTrace("%s", message);
            if (CryAssert("Assertion failed", fileName, line, ignore))
            {
                AZ::Debug::Trace::Break();
            }
        }

        return true;
#else
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, message);
        return true;
#endif // defined(USE_CRY_ASSERT) && (defined(WIN32) || defined(DURANGO) || defined(APPLE) || defined(LINUX))
    }

    bool OnException(const char* message) override
    {
        CryFatalError("%s", message);
        return true;
    }

    bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override
    {
        (void)window;
        return OnPreAssert(fileName, line, func, message);
    }

    bool OnWarning(const char* window, const char* message) override
    {
        (void)window;
        if (!IsCryLogReady())
        {
            return false;
        }
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", message);
        return true;
    }

    bool OnPrintf(const char* window, const char* message) override
    {
        (void)window;
        if (!IsCryLogReady())
        {
            return false;
        }
        CryLog("%s", message);
        return true;
    }

private:

    using IgnoredAssertMap = AZStd::unordered_map<AZ::Crc32, bool, AZStd::hash<AZ::Crc32>, AZStd::equal_to<AZ::Crc32>, AZ::OSStdAllocator>;
    IgnoredAssertMap* m_ignoredAsserts;
};

#endif // AZCORELOGSINK_HEADER
