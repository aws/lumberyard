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

// include Core headers
#include "StandardHeaders.h"
#include "DiskTextFile.h"
#include "LogManager.h"


namespace MCore
{
    /**
     * LogFile class for writing debug/log data to files.
     * Should not be used directly, but through the LogManager class.
     */
    class MCORE_API LogFile
        : public DiskTextFile
    {
        friend class LogManager;

    public:
        /**
         * Constructor. Called by LogManager.
         * @param filename The filename of the log file.
         */
        LogFile(const char* filename);

        /**
         * Destructor.
         */
        ~LogFile();

        /**
         * Logs a message to log file.
         * @param message The message to write into the log file.
         * @param logLevel The log level (debug, info, error, etc).
         */
        void LogMessage(const char* message, LogCallback::ELogLevel logLevel);
    };
}   // namespace MCore
