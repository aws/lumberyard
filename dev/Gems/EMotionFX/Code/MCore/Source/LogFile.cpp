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

// include the required headers
#include <AzCore/PlatformIncl.h>
#include "LogFile.h"


namespace MCore
{
    // the constructor
    LogFile::LogFile(const char* filename)
    {
        Open(filename, WRITE);
    }


    // the destructor
    LogFile::~LogFile()
    {
        Close();
    }


    // log the message
    void LogFile::LogMessage(const char* message, LogCallback::ELogLevel logLevel)
    {
        switch (logLevel)
        {
        case LogCallback::LOGLEVEL_FATAL:
        case LogCallback::LOGLEVEL_ERROR:
            AZ_Error("EMotionFX", false, "%s\n", message);
            break;
        case LogCallback::LOGLEVEL_WARNING:
            AZ_Warning("EMotionFX", false, "%s\n", message);
            break;
        case LogCallback::LOGLEVEL_INFO:
        case LogCallback::LOGLEVEL_DETAILEDINFO:
        case LogCallback::LOGLEVEL_DEBUG:
        default:
            AZ_TracePrintf("EMotionFX", "%s\n", message);
            break;
        }

        // write the log line
        if (GetIsOpen())
        {
            WriteString(message);
            WriteString("\n");
            Flush(); // flush the file, force it to write to disk immediately and not keep it cached
        }
    }
}   // namespace MCore
