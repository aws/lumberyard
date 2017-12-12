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
#ifndef AZ_UNITY_BUILD
#include <AzCore/base.h>
#include <AzCore/PlatformIncl.h>

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Debug/TraceMessagesDrillerBus.h>

#include <stdio.h>
#include <stdarg.h>

#if   defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)

#   if defined(AZ_PLATFORM_ANDROID)
#       include <android/log.h>
#   endif // AZ_PLATFORM_ANDROID

#   include <AzCore/Debug/TraceCppLinux.inl>

#elif defined(AZ_PLATFORM_APPLE)
#   include <AzCore/Debug/TraceCppApple.inl>
#endif

namespace AZ {
    using namespace AZ::Debug;

    //////////////////////////////////////////////////////////////////////////
    // Globals
    const int       g_maxMessageLength = 4096;
    const char*    g_dbgSystemWnd = "System";
    Trace Debug::g_tracer;
    void* g_exceptionInfo = NULL;
#if defined(AZ_ENABLE_DEBUG_TOOLS)
    #if defined(AZ_PLATFORM_WINDOWS)
    LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo);
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousExceptionHandler = NULL;
    #endif
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)

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
    // IsDebuggerPresent
    // [8/3/2009]
    //=========================================================================
    bool
    Trace::IsDebuggerPresent()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
#   if defined(AZ_PLATFORM_WINDOWS)
        return ::IsDebuggerPresent() ? true : false;
#   elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)
        return IsDebuggerAttached();
#   elif defined(AZ_PLATFORM_APPLE)
        return AmIBeingDebugged();
#   else
        return false;
#   endif
#else // else if not defined AZ_ENABLE_DEBUG_TOOLS
        return false;
#endif// AZ_ENABLE_DEBUG_TOOLS
    }

    //=========================================================================
    // HandleExceptions
    // [8/3/2009]
    //=========================================================================
    void
    Trace::HandleExceptions(bool isEnabled)
    {
        (void)isEnabled;
        if (IsDebuggerPresent())
        {
            return;
        }

#if defined(AZ_ENABLE_DEBUG_TOOLS)
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE
        if (isEnabled)
        {
            g_previousExceptionHandler = ::SetUnhandledExceptionFilter(&ExceptionHandler);
        }
        else
        {
            ::SetUnhandledExceptionFilter(g_previousExceptionHandler);
            g_previousExceptionHandler = NULL;
        }
#endif
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)
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
#   if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE
        __debugbreak();
#   elif defined(AZ_PLATFORM_LINUX)
        DEBUG_BREAK;
#   elif defined(AZ_PLATFORM_ANDROID)
        raise(SIGINT);
#elif defined(AZ_PLATFORM_APPLE)
        __builtin_trap();
#   else
        (*((int*)0)) = 1;
#   endif // AZ_COMPILER_MSVC
#endif // AZ_ENABLE_DEBUG_TOOLS
    }

    //=========================================================================
    // Assert
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Assert(const char* fileName, int line, const char* funcName, const char* format, ...)
    {
        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength, format, mark);
        va_end(mark);

        EBUS_EVENT(TraceMessageDrillerBus, OnPreAssert, fileName, line, funcName, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPreAssert, fileName, line, funcName, message);
        if (result.m_value)
        {
            return;
        }

        Output(g_dbgSystemWnd, "\n==================================================================\n");
        azsnprintf(header, g_maxMessageLength, "Trace::Assert\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(g_dbgSystemWnd, header);
        azstrcat(message, g_maxMessageLength, "\n");
        Output(g_dbgSystemWnd, message);

        EBUS_EVENT(TraceMessageDrillerBus, OnAssert, message);
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnAssert, message);
        if (result.m_value)
        {
            Output(g_dbgSystemWnd, "==================================================================\n");
            return;
        }

        Output(g_dbgSystemWnd, "------------------------------------------------\n");
        PrintCallstack(g_dbgSystemWnd, 1);
        Output(g_dbgSystemWnd, "==================================================================\n");

        g_tracer.Break();
    }

    //=========================================================================
    // Error
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Error(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength, format, mark);
        va_end(mark);

        EBUS_EVENT(TraceMessageDrillerBus, OnPreError, window, fileName, line, funcName, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnPreError, window, fileName, line, funcName, message);
        if (result.m_value)
        {
            return;
        }

        Output(window, "\n==================================================================\n");
        azsnprintf(header, g_maxMessageLength, "Trace::Error\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        azstrcat(message, g_maxMessageLength, "\n");
        Output(window, message);

        EBUS_EVENT(TraceMessageDrillerBus, OnError, window, message);
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnError, window, message);
        if (result.m_value)
        {
            Output(window, "==================================================================\n");
            return;
        }

        Output(window, "------------------------------------------------\n");
        PrintCallstack(window, 1);
        Output(window, "==================================================================\n");
#if defined(AZ_PLATFORM_WINDOWS) && defined(AZ_DEBUG_BUILD)
        //show error message box
        char fullMsg[8 * 1024];
        _snprintf_s(fullMsg, AZ_ARRAY_SIZE(fullMsg), _TRUNCATE, "An error has occurred!\n%s(%d): '%s'\n%s\n\nPress OK to continue, or Cancel to debug\n", fileName, line, funcName, message);
        if (MessageBox(NULL, fullMsg, "Error!", MB_OKCANCEL | MB_SYSTEMMODAL) == IDCANCEL)
        {
            Break();
        }
#endif // RR_PLATFORM_WIN32
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
        if (result.m_value)
        {
            Output(window, "==================================================================\n");
            return;
        }

        Output(window, "------------------------------------------------\n");
        PrintCallstack(window, 1);
        Output(window, "==================================================================\n");
    }

    //=========================================================================
    // Printf
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Printf(const char* window, const char* format, ...)
    {
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
        if (window == 0)
        {
            window = g_dbgSystemWnd;
        }

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)  /// Output to the debugger! // ACCEPTED_USE

#   ifdef _UNICODE
        wchar_t messageW[g_maxMessageLength];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, messageW, message, g_maxMessageLength - 1) == 0)
        {
            OutputDebugStringW(messageW);
        }
#   else // !_UNICODE
        OutputDebugString(message);
#   endif // !_UNICODE
#endif

#if defined(AZ_PLATFORM_ANDROID) && !RELEASE
        __android_log_print(ANDROID_LOG_INFO, window, message);
#endif // AZ_PLATFORM_ANDROID

        EBUS_EVENT(TraceMessageDrillerBus, OnOutput, window, message);
        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnOutput, window, message);
        if (result.m_value)
        {
            return;
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
                Output(window, lines[i]);
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

#if defined(AZ_ENABLE_DEBUG_TOOLS)
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE
    //=========================================================================
    // GetExeptionName
    // [8/3/2011]
    //=========================================================================
    const char* GetExeptionName(DWORD code)
    {
    #define RETNAME(exc)    case exc: \
    return (#exc);
        switch (code)
        {
            RETNAME(EXCEPTION_ACCESS_VIOLATION);
            RETNAME(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
            RETNAME(EXCEPTION_BREAKPOINT);
            RETNAME(EXCEPTION_DATATYPE_MISALIGNMENT);
            RETNAME(EXCEPTION_FLT_DENORMAL_OPERAND);
            RETNAME(EXCEPTION_FLT_DIVIDE_BY_ZERO);
            RETNAME(EXCEPTION_FLT_INEXACT_RESULT);
            RETNAME(EXCEPTION_FLT_INVALID_OPERATION);
            RETNAME(EXCEPTION_FLT_OVERFLOW);
            RETNAME(EXCEPTION_FLT_STACK_CHECK);
            RETNAME(EXCEPTION_FLT_UNDERFLOW);
            RETNAME(EXCEPTION_ILLEGAL_INSTRUCTION);
            RETNAME(EXCEPTION_IN_PAGE_ERROR);
            RETNAME(EXCEPTION_INT_DIVIDE_BY_ZERO);
            RETNAME(EXCEPTION_INT_OVERFLOW);
            RETNAME(EXCEPTION_INVALID_DISPOSITION);
            RETNAME(EXCEPTION_NONCONTINUABLE_EXCEPTION);
            RETNAME(EXCEPTION_PRIV_INSTRUCTION);
            RETNAME(EXCEPTION_SINGLE_STEP);
            RETNAME(EXCEPTION_STACK_OVERFLOW);
            RETNAME(EXCEPTION_INVALID_HANDLE);
        default:
            return (char*)("Unknown exception");
        }
    #undef RETNAME
    }

    //=========================================================================
    // ExceptionHandler
    // [8/3/2011]
    //=========================================================================
    LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
    {
        static bool volatile isInExeption = false;
        if (isInExeption)
        {
            g_tracer.Output(g_dbgSystemWnd, "Exception handler loop!");
            if (g_tracer.IsDebuggerPresent())
            {
                g_tracer.Break();
            }
            else
            {
                exit(1);
            }
        }

        isInExeption = true;
        g_exceptionInfo = (void*)ExceptionInfo;

        char message[g_maxMessageLength];
        g_tracer.Output(g_dbgSystemWnd, "==================================================================\n");
        azsnprintf(message, g_maxMessageLength, "Exception : 0x%X - '%s' [%p]\n", ExceptionInfo->ExceptionRecord->ExceptionCode, GetExeptionName(ExceptionInfo->ExceptionRecord->ExceptionCode), ExceptionInfo->ExceptionRecord->ExceptionAddress);
        g_tracer.Output(g_dbgSystemWnd, message);

        EBUS_EVENT(TraceMessageDrillerBus, OnException, message);

        TraceMessageResult result;
        EBUS_EVENT_RESULT(result, TraceMessageBus, OnException, message);
        if (result.m_value)
        {
            g_tracer.Output(g_dbgSystemWnd, "==================================================================\n");
            g_exceptionInfo = NULL;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        g_tracer.PrintCallstack(g_dbgSystemWnd, 0, ExceptionInfo->ContextRecord);
        g_tracer.Output(g_dbgSystemWnd, "==================================================================\n");

        LONG lReturn = EXCEPTION_CONTINUE_SEARCH;
#if defined(AZ_PLATFORM_WINDOWS)
        int Result = MessageBox(NULL, "Abort to quit\nRetry with debugger\nIgnore to continue execution", "Exception encountered",
                MB_ABORTRETRYIGNORE | MB_DEFBUTTON3 | MB_ICONSTOP | MB_APPLMODAL);
        switch (Result)
        {
        case IDABORT:
            exit(1);
        case IDRETRY:
            g_tracer.Break();
        case IDIGNORE:
            lReturn = EXCEPTION_CONTINUE_EXECUTION;
            break;
        }
#else // else non-windows-platform
        g_tracer.Break();
#endif
        isInExeption = false;
        g_exceptionInfo = NULL;
        return lReturn;
    }
#endif // defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE
#endif // defined(AZ_ENABLE_TRACING)
} // namspace AZ

#endif // #ifndef AZ_UNITY_BUILD
