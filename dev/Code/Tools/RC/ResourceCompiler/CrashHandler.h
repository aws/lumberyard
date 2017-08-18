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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Based on DebugCallStack code.


#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CRASHHANDLER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CRASHHANDLER_H
#pragma once


#ifdef WIN32


//!============================================================================
//!
//! CrashHandler class, captures call stack information from symbol files,
//! writes callstack & minidump files.
//!
//!============================================================================
class CrashHandler
{
private:
    enum ELogDestination
    {
        eDefaultLog,
        eCrashLog,
        eNull
    };

public:
    CrashHandler();
    ~CrashHandler();

    //! Sets files used for logging and crash dumping.
    void SetFiles(const char* logFilename0, const char* logFilename1, const char* dumpFilename);

    //! Dumps Current Call Stack to log.
    void LogCallStack();

    //! Exception handler.
    int HandleException(EXCEPTION_POINTERS* pex);

private:
    void LogLine(ELogDestination logDestination, const char* format, ...) const;

    bool InitSymbols(ELogDestination logDestination);
    void DoneSymbols();

    void LogCallStack(ELogDestination logDestination, const std::vector<string>& funcs);
    int  UpdateCallStack(EXCEPTION_POINTERS* pex);
    void FillStackTrace(int maxStackEntries = MAX_DEBUG_STACK_ENTRIES, HANDLE hThread = GetCurrentThread());
    void LogExceptionInfo(EXCEPTION_POINTERS* pex);

    void WriteMinidump(EXCEPTION_POINTERS* pex);

private:
    //! Limits the maximal number of functions in call stack.
    static const int MAX_DEBUG_STACK_ENTRIES = 30;

    int m_nSkipNumFunctions;
    std::vector<string> m_functions;

    char m_excModule[MAX_PATH];

    bool m_bSymbols;

    HANDLE m_hThread;
    CONTEXT m_context;

    void* m_prevExceptionHandler;
};

#endif //WIN32

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CRASHHANDLER_H
