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

    virtual AZ::Debug::Result OnPreAssert(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        if (!IsCryLogReady())
        {
            return AZ::Debug::Result::Continue;
        }

        CryLogAlways("%s", parameters.message);

#if defined(USE_CRY_ASSERT) && (defined(WIN32) || defined(DURANGO) || defined(APPLE) || defined(LINUX))
        AZ::Crc32 crc;
        crc.Add(&parameters.line, sizeof(parameters.line));
        if (parameters.fileName)
        {
            crc.Add(parameters.fileName, strlen(parameters.fileName));
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
            CryAssertTrace("%s", parameters.message);
            if (CryAssert("Assertion failed", parameters.fileName, parameters.line, ignore))
            {
                return AZ::Debug::Result::Break;
            }
        }

        return AZ::Debug::Result::Handled;
#else
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, parameters.message);
        return AZ::Debug::Result::Handled;
#endif // defined(USE_CRY_ASSERT) && (defined(WIN32) || defined(DURANGO) || defined(APPLE) || defined(LINUX))
    }

    virtual AZ::Debug::Result OnException(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        CryFatalError("%s", parameters.message);
        return AZ::Debug::Result::Handled;
    }

    virtual AZ::Debug::Result OnPreError(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        return OnPreAssert(parameters);
    }

    virtual AZ::Debug::Result OnWarning(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        if (!IsCryLogReady())
        {
            return AZ::Debug::Result::Continue;
        }
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "%s", parameters.message);
        return AZ::Debug::Result::Handled;
    }

    virtual AZ::Debug::Result OnPrintf(const AZ::Debug::TraceMessageParameters& parameters) override
    {
        if (!IsCryLogReady())
        {
            return AZ::Debug::Result::Continue;
        }
        CryLog("%s", parameters.message);
        return AZ::Debug::Result::Handled;
    }

private:

    using IgnoredAssertMap = AZStd::unordered_map<AZ::Crc32, bool, AZStd::hash<AZ::Crc32>, AZStd::equal_to<AZ::Crc32>, AZ::OSStdAllocator>;
    IgnoredAssertMap* m_ignoredAsserts;
};

#endif // AZCORELOGSINK_HEADER
