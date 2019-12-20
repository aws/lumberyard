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

    // returns the module name of a given address
    virtual string GetModuleNameForAddr(void* addr) { return "[unknown]"; }

    // returns the function name of a given address together with source file and line number (if available) of a given address
    virtual void GetProcNameForAddr(void* addr, string& procName, void*& baseAddr, string& filename, int& line)
    {
        filename = "[unknown]";
        line = 0;
        baseAddr = addr;
#if defined(PLATFORM_64BIT)
        procName.Format("[%016llX]", addr);
#else
        procName.Format("[%08X]", addr);
#endif
    }

    // returns current filename
    virtual string GetCurrentFilename()  { return "[unknown]"; }

    //! Dumps Current Call Stack to log.
    virtual void LogCallstack();
    //triggers a fatal error, so the DebugCallstack can create the error.log and terminate the application
    void FatalError(const char*);

    //Reports a bug and continues execution
    virtual void ReportBug(const char*) {}

    virtual void FileCreationCallback(void (* postBackupProcess)());

    static void WriteLineToLog(const char* format, ...);

    virtual void StartMemLog();
    virtual void StopMemLog();

protected:
    IDebugCallStack();
    virtual ~IDebugCallStack();

    static const char* TranslateExceptionCode(DWORD dwExcept);
    static void PutVersion(char* str, size_t length);

    static void Screenshot(const char* szFileName);

    bool m_bIsFatalError;
    static const char* const s_szFatalErrorCode;

    void (* m_postBackupProcess)();

    AZ::IO::HandleType m_memAllocFileHandle;
};



#endif // CRYINCLUDE_CRYSYSTEM_IDEBUGCALLSTACK_H
