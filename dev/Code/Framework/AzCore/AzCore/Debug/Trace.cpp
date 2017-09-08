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
    const char*    g_dbgSystemWnd = "System";
    Trace Debug::g_tracer;
    void* g_exceptionInfo = NULL;
#if defined(AZ_ENABLE_DEBUG_TOOLS)
    #if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
    LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo);
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousExceptionHandler = NULL;
#endif // defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
#endif // defined(AZ_ENABLE_DEBUG_TOOLS)

    //=========================================================================
    bool Trace::IsDebuggerPresent()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
#   if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
        return ::IsDebuggerPresent() ? true : false;
#   elif defined(AZ_PLATFORM_X360)
        // Redacted
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
    void Trace::HandleExceptions(bool isEnabled)
    {
        (void)isEnabled;
        if (IsDebuggerPresent())
        {
            return;
        }

#if defined(AZ_ENABLE_DEBUG_TOOLS)
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
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
    void Trace::Break()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
#   if defined(AZ_TESTS_ENABLED)
        if (!IsDebuggerPresent())
        {
            return; // Do not break when tests are running unless debugger is present
        }
#   endif
#   if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
        __debugbreak();
#   elif defined(AZ_PLATFORM_X360)
        // Redacted
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    Debug::Result Trace::Assert(const char* expression, const char* fileName, int line, const char* funcName, const char* format, ...)
    {
        TraceMessageParameters params;
        azstrcpy(params.expression, sizeof(params.expression), expression);
        azstrcpy(params.fileName, sizeof(params.fileName), fileName);
        params.line = line;
        azstrcpy(params.funcName, sizeof(params.funcName), funcName);

        // format message

        va_list mark;
        va_start(mark, format);
        azvsnprintf(params.message, MaxMessageLength, format, mark);
        azstrcat(params.message, MaxMessageLength, "\n");
        va_end(mark);

        // handle assert

        Result handled = Result::Continue;

        EBUS_EVENT(TraceMessageDrillerBus, OnPreAssert, params.fileName, params.line, params.funcName, params.message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnPreAssert, params);

        if (handled != Result::Continue)
        {
            switch (handled)
            {

            case Result::Handled:
                return Result::Continue;

            case Result::Break:
                return IsDebuggerPresent() ? Result::Break : Result::Continue;

            default:
                return handled;

            }
        }

        // log assert

        Output(g_dbgSystemWnd, "\n==================================================================\n");
        char header[MaxMessageLength];
        azsnprintf(header, MaxMessageLength, "Trace::Assert\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(g_dbgSystemWnd, header);
        azstrcat(params.message, MaxMessageLength, "\n");
        Output(g_dbgSystemWnd, params.message);

        // handle assert

        EBUS_EVENT(TraceMessageDrillerBus, OnAssert, params.message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnAssert, params);

        if (handled != Result::Continue)
        {
            Output(g_dbgSystemWnd, "==================================================================\n");

            switch (handled)
            {

            case Result::Handled:
                return Result::Continue;

            case Result::Break:
                return IsDebuggerPresent() ? Result::Break : Result::Continue;

            default:
                return handled;

            }
        }

        Output(g_dbgSystemWnd, "------------------------------------------------\n");
        PrintCallstack(g_dbgSystemWnd, 1);
        Output(g_dbgSystemWnd, "==================================================================\n");

        return handled;
    }

    //=========================================================================
    Debug::Result Trace::Error(const char* expression, const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        TraceMessageParameters params;
        azstrcpy(params.expression, sizeof(params.expression), expression);
        azstrcpy(params.fileName, sizeof(params.fileName), fileName);
        params.line = line;
        azstrcpy(params.funcName, sizeof(params.funcName), funcName);
        azstrcpy(params.window, sizeof(params.window), window);

        // format message

        va_list mark;
        va_start(mark, format);
        azvsnprintf(params.message, MaxMessageLength, format, mark);
        azstrcat(params.message, MaxMessageLength, "\n");
        va_end(mark);

        // handle error

        Result handled = Result::Continue;

        EBUS_EVENT(TraceMessageDrillerBus, OnPreError, params.window, params.fileName, params.line, params.funcName, params.message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnPreError, params);

        if (handled != Result::Continue)
        {
            switch (handled)
            {

            case Result::Handled:
                return Result::Continue;

            case Result::Break:
                return IsDebuggerPresent() ? Result::Break : Result::Continue;

            default:
                return handled;

            }
        }

        // log error

        Output(window, "\n==================================================================\n");
        char header[MaxMessageLength];
        azsnprintf(header, MaxMessageLength, "Trace::Error\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        azstrcat(params.message, MaxMessageLength, "\n");
        Output(window, params.message);

        // handle error

        EBUS_EVENT(TraceMessageDrillerBus, OnError, params.window, params.message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnError, params);

        if (handled != Result::Continue)
        {
            Output(g_dbgSystemWnd, "==================================================================\n");

            switch (handled)
            {

            case Result::Handled:
                return Result::Continue;

            case Result::Break:
                return IsDebuggerPresent() ? Result::Break : Result::Continue;

            default:
                return handled;

            }
        }

        Output(window, "------------------------------------------------\n");
        PrintCallstack(window, 1);
        Output(window, "==================================================================\n");

        return handled;
    }

    //=========================================================================
    void Trace::Warning(const char* expression, const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        TraceMessageParameters params;
        azstrcpy(params.expression, sizeof(params.expression), expression);
        azstrcpy(params.fileName, sizeof(params.fileName), fileName);
        params.line = line;
        azstrcpy(params.funcName, sizeof(params.funcName), funcName);
        azstrcpy(params.window, sizeof(params.window), window);

        // format message

        va_list mark;
        va_start(mark, format);
        azvsnprintf(params.message, MaxMessageLength, format, mark);
        azstrcat(params.message, MaxMessageLength, "\n");
        va_end(mark);

        // handle pre warning

        Result handled = Result::Continue;

        EBUS_EVENT(TraceMessageDrillerBus, OnPreWarning, params.window, params.fileName, params.line, params.funcName, params.message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnPreWarning, params);

        if (handled != Result::Continue)
        {
            return;
        }

        // log warning

        Output(window, "\n==================================================================\n");
        char header[MaxMessageLength];
        azsnprintf(header, MaxMessageLength, "Trace::Warning\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        azstrcat(params.message, MaxMessageLength, "\n");
        Output(window, params.message);

        // handle warning

        EBUS_EVENT(TraceMessageDrillerBus, OnWarning, params.window, params.message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnWarning, params);

        if (handled != Result::Continue)
        {
            Output(window, "==================================================================\n");

            return;
        }

        Output(window, "------------------------------------------------\n");
        PrintCallstack(window, 1);
        Output(window, "==================================================================\n");
    }

    //=========================================================================
    void Trace::Printf(const char* window, const char* format, ...)
    {
        TraceMessageParameters params;
        azstrcpy(params.window, sizeof(params.window), window);

        // format message

        va_list mark;
        va_start(mark, format);
        azvsnprintf(params.message, MaxMessageLength, format, mark);
        azstrcat(params.message, MaxMessageLength, "\n");
        va_end(mark);

        // handle printf

        Result handled = Result::Continue;

        EBUS_EVENT(TraceMessageDrillerBus, OnPrintf, params.window, params.message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnPrintf, params);

        if (handled != Result::Continue)
        {
            return;
        }

        Output(window, params.message);
    }

    //=========================================================================
    void Trace::Output(const char* window, const char* message)
    {
        if (window == 0)
        {
            window = g_dbgSystemWnd;
        }

        TraceMessageParameters params;
        azstrcpy(params.window, sizeof(params.window), window);
        azstrcpy(params.message, sizeof(params.message), message);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)  /// Output to the debugger!

#   ifdef _UNICODE
        wchar_t messageW[MaxMessageLength];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, messageW, message, MaxMessageLength - 1) == 0)
        {
            OutputDebugStringW(messageW);
        }
#   else // !_UNICODE
        OutputDebugString(message);
#   endif // !_UNICODE
#endif // AZ_PLATFORM_WINDOWS || AZ_PLATFORM_XBONE

#if defined(AZ_PLATFORM_ANDROID) && !RELEASE
        __android_log_print(ANDROID_LOG_INFO, window, message);
#endif // AZ_PLATFORM_ANDROID

        Result handled = Result::Continue;

        EBUS_EVENT(TraceMessageDrillerBus, OnOutput, window, message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnOutput, params);

        if (handled != Result::Continue)
        {
            return;
        }

        printf("%s: %s", window, message);
    }

    //=========================================================================
    void Trace::PrintCallstack(const char* window, unsigned int suppressCount, void* nativeContext)
    {
        StackFrame frames[25];

        // Without StackFrame explicit alignment frames array is aligned to 4 bytes
        // which causes the stack tracing to fail.
        //size_t bla = AZStd::alignment_of<StackFrame>::value;
        //printf("Alignment value %d address 0x%08x : 0x%08x\n",bla,frames);
        SymbolStorage::StackLine lines[AZ_ARRAY_SIZE(frames)];

        if (nativeContext == NULL)
        {
            suppressCount += 1; /// If we do not provide a context we will capture in the RecordFunction, so skip us (Trace::PrinCallstack).
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
    void* Trace::GetNativeExceptionInfo()
    {
        return g_exceptionInfo;
    }

#if defined(AZ_ENABLE_DEBUG_TOOLS)
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
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

        TraceMessageParameters params;

        g_tracer.Output(g_dbgSystemWnd, "==================================================================\n");
        azsnprintf(params.message, Debug::MaxMessageLength, "Exception : 0x%X - '%s' [%p]\n", ExceptionInfo->ExceptionRecord->ExceptionCode, GetExeptionName(ExceptionInfo->ExceptionRecord->ExceptionCode), ExceptionInfo->ExceptionRecord->ExceptionAddress);
        g_tracer.Output(g_dbgSystemWnd, params.message);

        Result handled = Result::Continue;

        EBUS_EVENT(TraceMessageDrillerBus, OnException, params.message);
        EBUS_EVENT_RESULT(handled, TraceMessageBus, OnException, params);

        if (handled != Result::Continue)
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
#endif // defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)
#endif // defined(AZ_ENABLE_TRACING)
} // namspace AZ

#endif // #ifndef AZ_UNITY_BUILD
