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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SHLOG_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SHLOG_H
#pragma once

#include <PRT/ISHLog.h>
#include <stdarg.h>
#include <stdio.h>


namespace NSH
{
    static const char* scLogFileName = "prtlog.txt";
    static const char* scErrorFileName = "prterror.txt";

    //!< logging implementation as singleton
    //!< hosts two files(prtlog.txt, prterror.txt)
    //!< always adds time
    class CSHLog
        : public ISHLog
    {
    public:
        virtual void Log(const char* cpLogText, ...);                   //!< log message
        virtual void LogError(const char* cpLogText, ...);          //!< log error
        virtual void LogWarning(const char* cpLogText, ...);    //!< log warning
        static CSHLog& Instance();                                                          //!< retrieve logging instance

        virtual ~CSHLog();              //!< destructor freeing files
        virtual void LogTime();     //!< logs the time for the log file

    private:
        unsigned int m_RecordedWarningsCount;   //!< number of recorded warnings
        unsigned int m_RecordedErrorCount;      //!< number of recorded warnings

        //!< singleton stuff
        CSHLog();
        CSHLog(const CSHLog&);
        CSHLog& operator= (const CSHLog&);

        void LogTime(FILE* pFile);      //!< logs the time for any file
    };
}

#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SHLOG_H
