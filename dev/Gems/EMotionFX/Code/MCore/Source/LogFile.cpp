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
#include "LogFile.h"
#include <iostream>


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
        MCORE_UNUSED(logLevel);

        // output to the Visual Studio debug window
    #if (defined(MCORE_PLATFORM_WINDOWS))
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/LogFile_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/LogFile_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif (defined(MCORE_PLATFORM_ANDROID))
        switch (logLevel)
        {
        case LogCallback::LOGLEVEL_INFO:
            __android_log_print(ANDROID_LOG_INFO,   "MCore", message);
            break;
        case LogCallback::LOGLEVEL_DETAILEDINFO:
            __android_log_print(ANDROID_LOG_VERBOSE, "MCore", message);
            break;
        case LogCallback::LOGLEVEL_ERROR:
            __android_log_print(ANDROID_LOG_ERROR,  "MCore", message);
            break;
        case LogCallback::LOGLEVEL_WARNING:
            __android_log_print(ANDROID_LOG_WARN,   "MCore", message);
            break;
        case LogCallback::LOGLEVEL_DEBUG:
            __android_log_print(ANDROID_LOG_DEBUG,  "MCore", message);
            break;
        case LogCallback::LOGLEVEL_NONE:
            ;
            break;
        default:
            __android_log_print(ANDROID_LOG_INFO, "MCore", message);
            break;
        }
        ;
    #else
        std::cout << message << "\n";
    #endif

        // write the log line
        if (GetIsOpen())
        {
            WriteString(message);
            WriteString("\n");
            Flush(); // flush the file, force it to write to disk immediately and not keep it cached
        }
    }
}   // namespace MCore
