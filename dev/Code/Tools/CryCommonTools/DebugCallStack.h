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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_DEBUGCALLSTACK_H
#define CRYINCLUDE_CRYCOMMONTOOLS_DEBUGCALLSTACK_H

#pragma once

class ILogger;

//! Limits the maximal number of functions in call stack.
const int MAX_DEBUG_STACK_ENTRIES = 30;

//!============================================================================
//!
//! DebugCallStack class, capture call stack information from symbol files.
//!
//!============================================================================
class DebugCallStack
{
public:
    // Returns single instance of DebugStack
    static DebugCallStack*  instance();

    void SetLog(ILogger* log);

    //! Get current call stack information.
    void getCallStack(std::vector<string>& functions);

    int  handleException(void* exception_pointer);

    struct ErrorHandlerScope
    {
        ErrorHandlerScope(ILogger* log);
        ~ErrorHandlerScope();
    };

private:
    DebugCallStack();
    virtual ~DebugCallStack();

    bool initSymbols();
    void doneSymbols();

    string  LookupFunctionName(void* adderss, bool fileInfo);
    int         updateCallStack(void* exception_pointer);
    void        FillStackTrace(int maxStackEntries = MAX_DEBUG_STACK_ENTRIES);

    static  int unhandledExceptionHandler(void* exception_pointer);

    //! Return name of module where exception happened.
    const char* getExceptionModule() { return m_excModule; }
    const char* getExceptionLine() { return m_excLine; }

    void installErrorHandler();
    void uninstallErrorHandler();

    void PrintException(EXCEPTION_POINTERS* pex);

    std::vector<string> m_functions;
    static DebugCallStack* m_instance;

    char m_excLine[256];
    char m_excModule[128];

    void* m_pVectoredExceptionHandlerHandle;

    bool    m_symbols;

    int m_nSkipNumFunctions;
    CONTEXT  m_context;

    ILogger* m_pLog;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_DEBUGCALLSTACK_H
