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
#pragma once
#include <ILog.h>

#pragma warning( push )
#pragma warning( disable: 4373 )      // virtual function overrides differ only by const/volatile qualifiers, mock issue


class LogMock
    : public ILog
{
public:
    MOCK_METHOD3(LogV,
        void(const ELogType nType, const char* szFormat, va_list args));
    MOCK_METHOD4(LogV,
        void(const ELogType nType, int flags, const char* szFormat, va_list args));
    MOCK_METHOD0(Release,
        void());
    MOCK_METHOD2(SetFileName,
        bool(const char* fileNameOrFullPath, bool doBackups));
    MOCK_METHOD0(GetFileName,
        const char*());
    MOCK_METHOD0(GetBackupFileName,
        const char*());

    virtual void Log(const char* szCommand, ...) override {}
    virtual void LogWarning(const char* szCommand, ...) override {}
    virtual void LogError(const char* szCommand, ...) override {}
    virtual void LogAlways(const char* szCommand, ...) override {}
    virtual void LogPlus(const char* command, ...) override {}
    virtual void LogToFile(const char* command, ...) override {}
    virtual void LogToFilePlus(const char* command, ...) override {}
    virtual void LogToConsole(const char* command, ...) override {}
    virtual void LogToConsolePlus(const char* command, ...) override {}
    virtual void UpdateLoadingScreen(const char* command, ...) override {}

    MOCK_METHOD1(SetVerbosity,
        void(int verbosity));
    MOCK_METHOD0(GetVerbosityLevel,
        int());
    MOCK_METHOD1(AddCallback,
        void(ILogCallback * pCallback));
    MOCK_METHOD1(RemoveCallback,
        void(ILogCallback * pCallback));
    MOCK_METHOD0(Update,
        void());
    MOCK_METHOD0(GetModuleFilter,
        const char*());
    MOCK_CONST_METHOD1(GetMemoryUsage,
        void(ICrySizer * pSizer));
    MOCK_METHOD1(Indent,
        void(class CLogIndenter * indenter));
    MOCK_METHOD1(Unindent,
        void(class CLogIndenter * indenter));
    MOCK_METHOD0(FlushAndClose,
        void());
};

#pragma warning( pop )
