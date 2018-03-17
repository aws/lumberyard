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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <TraceMessageHook.h>

namespace AssetBuilder
{
    TraceMessageHook::TraceMessageHook()
        : m_stacks(nullptr)
        , m_inDebugMode(false)
        , m_skipErrorsCount(0)
        , m_skipWarningsCount(0)
        , m_skipPrintfsCount(0)
        , m_totalWarningCount(0)
        , m_totalErrorCount(0)
    {
        AssetBuilderSDK::AssetBuilderTraceBus::Handler::BusConnect();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    TraceMessageHook::~TraceMessageHook()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        AssetBuilderSDK::AssetBuilderTraceBus::Handler::BusDisconnect();
        delete m_stacks;
        m_stacks = nullptr;
    }

    void TraceMessageHook::EnableTraceContext(bool enable)
    {
        if (enable)
        {
            if (!m_stacks)
            {
                m_stacks = new AzToolsFramework::Debug::TraceContextMultiStackHandler();
            }
        }
        else
        {
            delete m_stacks;
            m_stacks = nullptr;
        }
    }

    void TraceMessageHook::EnableDebugMode(bool enable)
    {
        m_inDebugMode = enable;
    }

    bool TraceMessageHook::OnAssert(const char* message)
    {
        if (m_skipErrorsCount == 0)
        {
            DumpTraceContext(stderr);
            CleanMessage(stderr, "E", message, true);
            std::fflush(stderr);
            ++m_totalErrorCount;
        }
        else
        {
            --m_skipErrorsCount;
        }

        return !m_inDebugMode;
    }

    bool TraceMessageHook::OnError(const char* /*window*/, const char* message)
    {
        if (m_skipErrorsCount == 0)
        {
            DumpTraceContext(stderr);
            CleanMessage(stderr, "E", message, true);
            ++m_totalErrorCount;
        }
        else
        {
            --m_skipErrorsCount;
        }

        return !m_inDebugMode;
    }

    bool TraceMessageHook::OnWarning(const char* /*window*/, const char* message)
    {
        if (m_skipWarningsCount == 0)
        {
            DumpTraceContext(stdout);
            CleanMessage(stdout, "W", message, true);
            ++m_totalWarningCount;
        }
        else
        {
            --m_skipWarningsCount;
        }

        return !m_inDebugMode;
    }

    bool TraceMessageHook::OnException(const char* message)
    {
        m_isInException = true;
        DumpTraceContext(stderr);
        CleanMessage(stderr, "E", message, true);
        ++m_totalErrorCount;
        AZ::Debug::Trace::HandleExceptions(false);
        AZ::Debug::Trace::PrintCallstack("", 3); // Skip all the Trace.cpp function calls
        // note that the above call ultimately results in a whole bunch of TracePrint/Outputs, which will end up in OnOutput below.

        std::fflush(stderr);
        std::fflush(stdout);

        // if we don't terminate here, the user may get a dialog box from the OS saying that the program crashed.
        // we don't want this, because in this case, the program is one of potentially many, many background worker processes
        // that are continuously starting/stopping and they'd get flooded by those message boxes.
        AZ::Debug::Trace::Terminate(1);

        return false;
    }

bool TraceMessageHook::OnOutput(const char* window, const char* message)
{
    if (m_isInException) // all messages that occur during an exception should be considered an error.
    {
        CleanMessage(stderr, "E: ", message, true);
    }
    else
    {
        CleanMessage(stdout, "", message, false);
    }

    return true;
}

    bool TraceMessageHook::OnPrintf(const char* window, const char* message)
    {
        if (m_skipPrintfsCount == 0)
        {
            DumpTraceContext(stdout);
            CleanMessage(stdout, window, message, false);
        }
        else
        {
            --m_skipPrintfsCount;
        }
        
        return true;
    }

    void TraceMessageHook::IgnoreNextErrors(AZ::u32 count)
    {
        m_skipErrorsCount += count;
    }

    void TraceMessageHook::IgnoreNextWarning(AZ::u32 count)
    {
        m_skipWarningsCount += count;
    }

    void TraceMessageHook::IgnoreNextPrintf(AZ::u32 count)
    {
        m_skipPrintfsCount += count;
    }

    void TraceMessageHook::ResetWarningCount()
    {
        m_totalWarningCount = 0;
    }

    void TraceMessageHook::ResetErrorCount()
    {
        m_totalErrorCount = 0;
    }

    AZ::u32 TraceMessageHook::GetWarningCount()
    {
        return m_totalWarningCount;
    }

    AZ::u32 TraceMessageHook::GetErrorCount()
    {
        return m_totalErrorCount;
    }

    void TraceMessageHook::DumpTraceContext(FILE* stream)
    {
        AZStd::shared_ptr<const AzToolsFramework::Debug::TraceContextStack> stack = m_stacks->GetCurrentStack();
        if (stack)
        {
            AZStd::string line;
            size_t stackSize = stack->GetStackCount();
            for (size_t i = 0; i < stackSize; ++i)
            {
                line.clear();
                AzToolsFramework::Debug::TraceContextLogFormatter::PrintLine(line, *stack, i);
                CleanMessage(stream, "C", line.c_str(), false);
            }
        }
    }

    void TraceMessageHook::CleanMessage(FILE* stream, const char* prefix, const char* message, bool forceFlush)
    {
        if (message && message[0] != 0)
        {
            fprintf(stream, "%s", prefix);
            fprintf(stream, ": ");
            fprintf(stream, "%s", message);

            // Make sure the message ends with a newline
            if (message[AzFramework::StringFunc::StringLength(message) - 1] != '\n')
            {
                fprintf(stream, "\n");
            }

            if (forceFlush)
            {
                fflush(stream);
            }
        }
    }
} // namespace AssetBuilder