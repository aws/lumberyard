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

// Description : A multiplatform base class for handling errors and collecting call stacks

#ifndef CRYINCLUDE_CRYSYSTEM_IDEBUGCALLSTACK_H
#define CRYINCLUDE_CRYSYSTEM_IDEBUGCALLSTACK_H
#pragma once

#include "System.h"

#if AZ_LEGACY_CRYSYSTEM_TRAIT_FORWARD_EXCEPTION_POINTERS
struct EXCEPTION_POINTERS;
#endif
//! Limits the maximal number of functions in call stack.
enum
{
    MAX_DEBUG_STACK_ENTRIES = 80
};

class IDebugCallStack
{
public:
    // Returns single instance of DebugStack
    static IDebugCallStack* instance();

    virtual int handleException(EXCEPTION_POINTERS* exception_pointer){return 0; }
    virtual void CollectCurrentCallStack(int maxStackEntries = MAX_DEBUG_STACK_ENTRIES){}
    // Collects the low level callstack frames.
    // Returns number of collected stack frames.
    virtual int CollectCallStackFrames(void** pCallstack, int maxStackEntries) { return 0; }

    // collects low level callstack for given thread handle
    virtual int CollectCallStack(HANDLE thread, void** pCallstack, int maxStackEntries) { return 0; }

    // returns the module name of a given address
    virtual string GetModuleNameForAddr(void* addr) { return "[unknown]"; }

    // returns the function name of a given address together with source file and line number (if available) of a given address
    virtual bool GetProcNameForAddr(void* addr, string& procName, void*& baseAddr, string& filename, int& line)
    {
        filename = "[unknown]";
        line = 0;
        baseAddr = addr;
#if defined(PLATFORM_64BIT)
        procName.Format("[%016llX]", addr);
#else
        procName.Format("[%08X]", addr);
#endif
        return false;
    }

    // returns current filename
    virtual string GetCurrentFilename()  { return "[unknown]"; }

    // initialize symbols
    virtual void InitSymbols() {}
    virtual void DoneSymbols() {}

    //! Dumps Current Call Stack to log.
    virtual void LogCallstack();
    //triggers a fatal error, so the DebugCallstack can create the error.log and terminate the application
    void FatalError(const char*);

    //Reports a bug and continues execution
    virtual void ReportBug(const char*) {}

    virtual void FileCreationCallback(void (* postBackupProcess)());

    //! Get current call stack information.
    void getCallStack(std::vector<string>& functions) { functions = m_functions; }

    static void WriteLineToLog(const char* format, ...);

    virtual void StartMemLog();
    virtual void StopMemLog();

protected:
    IDebugCallStack();
    virtual ~IDebugCallStack();

    static const char* TranslateExceptionCode(DWORD dwExcept);
    static void PutVersion(char* str);

    static void Screenshot(const char* szFileName);

    bool m_bIsFatalError;
    static const char* const s_szFatalErrorCode;

    std::vector<string> m_functions;
    void (* m_postBackupProcess)();

    AZ::IO::HandleType m_memAllocFileHandle;
};



#endif // CRYINCLUDE_CRYSYSTEM_IDEBUGCALLSTACK_H
