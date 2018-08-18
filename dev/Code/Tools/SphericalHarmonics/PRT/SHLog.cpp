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
#include "stdafx.h"
#include "SHLog.h"
#include <PRT/PRTTypes.h>


NSH::ISHLog& GetSHLog()
{
    return NSH::CSHLog::Instance();
}

NSH::CSHLog& NSH::CSHLog::Instance()
{
    static CSHLog inst;
    return inst;
}

NSH::CSHLog::CSHLog()
    : m_RecordedWarningsCount(0)
    , m_RecordedErrorCount(0)
{
    //create files
    FILE* pErrorFile = nullptr;
    azfopen(&pErrorFile, scErrorFileName, "wb");
    FILE* pLogFile = nullptr;
    azfopen(&pLogFile, scLogFileName, "wb");
    //  assert(pErrorFile);
    //  assert(pLogFile);
    if (pErrorFile)
    {
        fclose(pErrorFile);
    }
    if (pLogFile)
    {
        fclose(pLogFile);
    }
}

NSH::CSHLog::~CSHLog()
{
    if (m_RecordedWarningsCount > 0)
    {
        printf("\n%d warnings in PRT engine recorded, see log file: %s\n", m_RecordedWarningsCount, scLogFileName);
    }
    if (m_RecordedErrorCount > 0)
    {
        printf("%d erors in PRT engine recorded, see log file: %s\n", m_RecordedErrorCount, scErrorFileName);
    }
    printf("\n");
}

void NSH::CSHLog::Log(const char* cpLogText, ...)
{
    FILE* pLogFile = nullptr;
    azfopen(&pLogFile, scLogFileName, "a");
    //  assert(pLogFile);
    if (!pLogFile)
    {
        return;
    }
    char str[1024];
    va_list argptr;
    va_start(argptr, cpLogText);
    azvsprintf(str, cpLogText, argptr);
    assert(strlen(str) < 1023);
    fprintf(pLogFile, str);
    va_end(argptr);
    fclose(pLogFile);
}

void NSH::CSHLog::LogError(const char* cpLogText, ...)
{
    FILE* pErrorFile = nullptr;
    azfopen(&pErrorFile, scErrorFileName, "a");
    //  assert(pErrorFile);
    if (!pErrorFile)
    {
        return;
    }
    m_RecordedErrorCount++;
    LogTime(pErrorFile);
    fprintf(pErrorFile, "ERROR: ");
    printf("ERROR: ");
    char str[1024];
    va_list argptr;
    va_start(argptr, cpLogText);
    azvsprintf(str, cpLogText, argptr);
    assert(strlen(str) < 1023);
    fprintf(pErrorFile, str);
    printf(str);
    if (std::string(str).find("\n") == std::string::npos)
    {
        fprintf(pErrorFile, "\n");
        printf("\n");
    }
    va_end(argptr);
    fclose(pErrorFile);
}

void NSH::CSHLog::LogWarning(const char* cpLogText, ...)
{
    FILE* pLogFile = nullptr;
    azfopen(&pLogFile, scLogFileName, "a");
    //  assert(pLogFile);
    if (!pLogFile)
    {
        return;
    }
    m_RecordedWarningsCount++;
    LogTime(pLogFile);
    fprintf(pLogFile, "WARNING: ");
    char str[1024];
    va_list argptr;
    va_start(argptr, cpLogText);
    azvsprintf(str, cpLogText, argptr);
    assert(strlen(str) < 1023);
    fprintf(pLogFile, str);
    if (std::string(str).find("\n") == std::string::npos)
    {
        fprintf(pLogFile, "\n");
    }
    va_end(argptr);
    fclose(pLogFile);
}

void NSH::CSHLog::LogTime(FILE* pFile)
{
    assert(pFile);
    if (!pFile)
    {
        return;
    }
#ifdef _WIN32
    SYSTEMTIME time;
    GetLocalTime(&time);
    if (time.wMinute < 10)
    {
        fprintf(pFile, "%d.0%d: ", time.wHour, time.wMinute);
    }
    else
    {
        fprintf(pFile, "%d.%d: ", time.wHour, time.wMinute);
    }
#else
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    if (tm.tm_min < 10)
    {
        fprintf(pFile, "%d.0%d: ", tm.tm_hour, tm.tm_min);
    }
    else
    {
        fprintf(pFile, "%d.%d: ", tm.tm_hour, tm.tm_min);
    }
#endif
}

void NSH::CSHLog::LogTime()
{
    FILE* pLogFile = nullptr;
    azfopen(&pLogFile, scLogFileName, "a");
    if (!pLogFile)
    {
        return;
    }
    LogTime(pLogFile);
    fclose(pLogFile);
}
