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

#include <AzCore/Debug/Trace.h>

#include <AzCore/base.h>

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Debug/TraceMessagesDrillerBus.h>

#include <stdarg.h>

namespace AZ 
{
    namespace Debug
    {
        namespace Platform
        {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
            bool IsDebuggerPresent();
            void HandleExceptions(bool isEnabled);
            void DebugBreak();
#endif
            void Terminate(int exitCode);
            void OutputToDebugger(const char* window, const char* message);
        }
    }

    using namespace AZ::Debug;

    namespace DebugInternal
    {
        // other threads can trigger fatals and errors, but the same thread should not, to avoid stack overflow.
        // because its thread local, it does not need to be atomic.
        // its also important that this is inside the cpp file, so that its only in the trace.cpp module and not the header shared by everyone.
        static AZ_THREAD_LOCAL bool g_alreadyHandlingAssertOrFatal = false;
        AZ_THREAD_LOCAL bool g_suppressEBusCalls = false; // used when it would be dangerous to use ebus broadcasts.
    }

    //////////////////////////////////////////////////////////////////////////
    // Globals
    const int       g_maxMessageLength = 4096;
    static const char*    g_dbgSystemWnd = "System";
    Trace Debug::g_tracer;
    void* g_exceptionInfo = NULL;

    /**
     * If any listener returns true, store the result so we don't outputs detailed information.
     */
    struct TraceMessageResult
    {
        bool     m_value;
        TraceMessageResult()
            : m_value(false) {}
        void operator=(bool rhs)     { m_value = m_value || rhs; }
    };

    //=========================================================================
    const char* Trace::GetDefaultSystemWindow()
    {
        return g_dbgSystemWnd;
    }

    //=========================================================================
    // IsDebuggerPresent
    // [8/3/2009]
    //=========================================================================
    bool
    Trace::IsDebuggerPresent()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        return Platform::IsDebuggerPresent();
#else
        return false;
#endif
    }

    //=========================================================================
    // HandleExceptions
    // [8/3/2009]
    //=========================================================================
    void
    Trace::HandleExceptions(bool isEnabled)
    {
        AZ_UNUSED(isEnabled);
        if (IsDebuggerPresent())
        {
            return;
        }

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        Platform::HandleExceptions(isEnabled);
#endif
    }

    //=========================================================================
    // Break
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Break()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)

#   if defined(AZ_TESTS_ENABLED)
        if (!IsDebuggerPresent())
        {
            return; // Do not break when tests are running unless debugger is present
        }
#   endif

        Platform::DebugBreak();

#endif // AZ_ENABLE_DEBUG_TOOLS
    }

    void Debug::Trace::Terminate(int exitCode)
    {
        Platform::Terminate(exitCode);
    }

    //=========================================================================
    // Assert
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Assert(const char* fileName, int line, const char* funcName, const char* format, ...)
    {
        using namespace DebugInternal;

        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        if (g_alreadyHandlingAssertOrFatal)
        {
            return;
        }

        g_alreadyHandlingAssertOrFatal = true;

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength, format, mark);
        va_end(mark);

        EBUS_EVENT(TraceMessageDrillerBus, OnPreAssert, fileName, line, funcName, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPreAssert, fileName, line, funcName, message);
        if (result.m_value)
        {
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        Output(g_dbgSystemWnd, "\n==================================================================\n");
        azsnprintf(header, g_maxMessageLength, "Trace::Assert\n %s(%d): (%tu) '%s'\n", fileName, line, (uintptr_t)(AZStd::this_thread::get_id().m_id), funcName);
        Output(g_dbgSystemWnd, header);
        azstrcat(message, g_maxMessageLength, "\n");
        Output(g_dbgSystemWnd, message);

        EBUS_EVENT(TraceMessageDrillerBus, OnAssert, message);
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnAssert, message);
        if (result.m_value)
        {
            Output(g_dbgSystemWnd, "==================================================================\n");
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        Output(g_dbgSystemWnd, "------------------------------------------------\n");
        PrintCallstack(g_dbgSystemWnd, 1);
        Output(g_dbgSystemWnd, "==================================================================\n");
        g_alreadyHandlingAssertOrFatal = false;

        g_tracer.Break();
    }

    //=========================================================================
    // Error
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Error(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        using namespace DebugInternal;
        if (!window)
        {
            window = g_dbgSystemWnd;
        }

        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        if (g_alreadyHandlingAssertOrFatal)
        {
            return;
        }
        g_alreadyHandlingAssertOrFatal = true;

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength, format, mark);
        va_end(mark);

        EBUS_EVENT(TraceMessageDrillerBus, OnPreError, window, fileName, line, funcName, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPreError, window, fileName, line, funcName, message);
        if (result.m_value)
        {
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        Output(window, "\n==================================================================\n");
        azsnprintf(header, g_maxMessageLength, "Trace::Error\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        azstrcat(message, g_maxMessageLength, "\n");
        Output(window, message);

        EBUS_EVENT(TraceMessageDrillerBus, OnError, window, message);
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnError, window, message);
        Output(window, "==================================================================\n");
        if (result.m_value)
        {
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        g_alreadyHandlingAssertOrFatal = false;
    }
    //=========================================================================
    // Warning
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Warning(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength, format, mark);
        va_end(mark);

        EBUS_EVENT(TraceMessageDrillerBus, OnPreWarning, window, fileName, line, funcName, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPreWarning, window, fileName, line, funcName, message);
        if (result.m_value)
        {
            return;
        }

        Output(window, "\n==================================================================\n");
        azsnprintf(header, g_maxMessageLength, "Trace::Warning\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        azstrcat(message, g_maxMessageLength, "\n");
        Output(window, message);

        EBUS_EVENT(TraceMessageDrillerBus, OnWarning, window, message);
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnWarning, window, message);
        Output(window, "==================================================================\n");
    }

    //=========================================================================
    // Printf
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Printf(const char* window, const char* format, ...)
    {
        if (!window)
        {
            window = g_dbgSystemWnd;
        }

        char message[g_maxMessageLength];

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength, format, mark);
        va_end(mark);

        EBUS_EVENT(TraceMessageDrillerBus, OnPrintf, window, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPrintf, window, message);
        if (result.m_value)
        {
            return;
        }

        Output(window, message);
    }

    //=========================================================================
    // Output
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Output(const char* window, const char* message)
    {
        if (!window)
        {
            window = g_dbgSystemWnd;
        }

        Platform::OutputToDebugger(window, message);
        
        if (!DebugInternal::g_suppressEBusCalls)
        {
            // only call into Ebusses if we are not in a recursive-exception situation as that
            // would likely just lead to even more exceptions.
            
            EBUS_EVENT(TraceMessageDrillerBus, OnOutput, window, message);
            TraceMessageResult result;
            EBUS_EVENT_RESULT(result, TraceMessageBus, OnOutput, window, message);
            if (result.m_value)
            {
                return;
            }
        }

        printf("%s: %s", window, message);
    }

    //=========================================================================
    // PrintCallstack
    // [8/3/2009]
    //=========================================================================
    void
    Trace::PrintCallstack(const char* window, unsigned int suppressCount, void* nativeContext)
    {
        StackFrame frames[25];

        // Without StackFrame explicit alignment frames array is aligned to 4 bytes
        // which causes the stack tracing to fail.
        //size_t bla = AZStd::alignment_of<StackFrame>::value;
        //printf("Alignment value %d address 0x%08x : 0x%08x\n",bla,frames);
        SymbolStorage::StackLine lines[AZ_ARRAY_SIZE(frames)];

        if (nativeContext == NULL)
        {
            suppressCount += 1; /// If we don't provide a context we will capture in the RecordFunction, so skip us (Trace::PrinCallstack).
        }
        unsigned int numFrames = StackRecorder::Record(frames, AZ_ARRAY_SIZE(frames), suppressCount, nativeContext);
        if (numFrames)
        {
            SymbolStorage::DecodeFrames(frames, numFrames, lines);
            for (unsigned int i = 0; i < numFrames; ++i)
            {
                if (lines[i][0] == 0)
                {
                    continue;
                }

                azstrcat(lines[i], AZ_ARRAY_SIZE(lines[i]), "\n");
                AZ_Printf(window, "%s", lines[i]); // feed back into the trace system so that listeners can get it.
            }
        }
    }

    //=========================================================================
    // GetExceptionInfo
    // [4/2/2012]
    //=========================================================================
    void*
    Trace::GetNativeExceptionInfo()
    {
        return g_exceptionInfo;
    }

} // namspace AZ
