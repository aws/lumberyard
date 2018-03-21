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

// include the Core headers
#include "LogManager.h"
#include "LogFile.h"

#include <iostream>

namespace MCore
{
    // static mutex
    Mutex LogManager::mGlobalMutex;

    //--------------------------------------------------------------------------------------------

    // constructor
    LogCallback::LogCallback()
    {
        mLogLevels = LOGLEVEL_DEFAULT;
    }


    // set the log levels this callback will accept and pass through
    void LogCallback::SetLogLevels(ELogLevel logLevels)
    {
        mLogLevels = logLevels;
        GetLogManager().InitLogLevels();
    }

    //-------------------------------------------------------------------------------------------

    // constructor
    LogFileCallback::LogFileCallback(const char* filename)
        : LogCallback()
    {
        mLog = new LogFile(filename);
    }


    // destructor
    LogFileCallback::~LogFileCallback()
    {
        if (mLog)
        {
            delete mLog;
        }
    }


    // log callback function
    void LogFileCallback::Log(const char* text, ELogLevel logLevel)
    {
        mLog->LogMessage(text, logLevel);
    }


    // return a pointer to the log file
    LogFile* LogFileCallback::GetLogFile() const
    {
        return mLog;
    }


    // return the unique type identification number of this log file callback
    uint32 LogFileCallback::GetType() const
    {
        return TYPE_ID;
    }

    //-------------------------------------------------------------------------------------------

    // constructor
    LogManager::LogManager()
    {
        mLogCallbacks.SetMemoryCategory(MCORE_MEMCATEGORY_LOGMANAGER);

        // initialize the enabled log levels
        InitLogLevels();
    }


    // destructor
    LogManager::~LogManager()
    {
        // get rid of the callbacks
        ClearLogCallbacks();
    }


    // creates a log file callback and adds it to the stack
    LogFile* LogManager::CreateLogFile(const char* filename)
    {
        // create log file callback instance
        LogFileCallback* callback = new LogFileCallback(filename);

        // add log file callback to the stack
        AddLogCallback(callback);

        // return pointer to the log file of the callback
        return callback->GetLogFile();
    }


    // add a log file callback instance
    void LogManager::AddLogCallback(LogCallback* callback)
    {
        MCORE_ASSERT(callback);
        LockGuard lock(mMutex);

        // add the callback to the stack
        mLogCallbacks.Add(callback);

        // collect the enabled log levels
        InitLogLevels();
    }


    // remove a specific log callback from the stack
    void LogManager::RemoveLogCallback(uint32 index)
    {
        MCORE_ASSERT(mLogCallbacks.GetIsValidIndex(index));
        LockGuard lock(mMutex);

        // delete it from memory
        delete mLogCallbacks[index];

        // remove the callback from the stack
        mLogCallbacks.Remove(index);

        // collect the enabled log levels
        InitLogLevels();
    }


    // remove all given log file callbacks by file names
    void LogManager::RemoveAllByFileName(const char* fileName)
    {
        LockGuard lock(mMutex);
        // iterate through all log callbacks
        for (uint32 i = 0; i < mLogCallbacks.GetLength(); )
        {
            LogCallback* callback = mLogCallbacks[i];

            // check if we are dealing with a log file
            if (callback->GetType() == LogFileCallback::TYPE_ID)
            {
                LogFileCallback* logFileCallback = static_cast<LogFileCallback*>(callback);

                // check if the log file is valid
                if (logFileCallback->GetLogFile() == nullptr)
                {
                    continue;
                }

                // compare the file names
                if (logFileCallback->GetLogFile()->GetFileName().CheckIfIsEqual(fileName))
                {
                    // get rid of the callback instance
                    delete callback;

                    // remove the callback from the stack
                    mLogCallbacks.Remove(i);
                }
                else
                {
                    i++;
                }
            }
            else
            {
                i++;
            }
        }

        // collect the enabled log levels
        InitLogLevels();
    }


    // remove all given log callbacks by type
    void LogManager::RemoveAllByType(uint32 type)
    {
        LockGuard lock(mMutex);

        // iterate through all log callbacks
        for (uint32 i = 0; i < mLogCallbacks.GetLength(); )
        {
            LogCallback* callback = mLogCallbacks[i];

            // check if we are dealing with a log file
            if (callback->GetType() == type)
            {
                // get rid of the callback instance
                delete callback;

                // remove the callback from the stack
                mLogCallbacks.Remove(i);
            }
            else
            {
                i++;
            }
        }

        // collect the enabled log levels
        InitLogLevels();
    }


    // remove all log callbacks from the stack
    void LogManager::ClearLogCallbacks()
    {
        LockGuard lock(mMutex);

        // get rid of the callbacks
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            delete mLogCallbacks[i];
        }

        mLogCallbacks.Clear(true);

        // collect the enabled log levels
        InitLogLevels();
    }


    // retrieve a pointer to the given log callback
    LogCallback* LogManager::GetLogCallback(uint32 index)
    {
        return mLogCallbacks[index];
    }


    // find the first log file callback by filename
    LogFileCallback* LogManager::FindByFileName(const char* filename)
    {
        LockGuard lock(mMutex);

        // get the number of callbacks and iterate through them
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            // get the current log callback and check if it is a log file
            LogCallback* logCallback = mLogCallbacks[i];
            if (logCallback->GetType() == LogFileCallback::TYPE_ID)
            {
                // typecast to a log file callback
                LogFileCallback* logFileCallback    = (LogFileCallback*)logCallback;
                LogFile*        logFile             = logFileCallback->GetLogFile();

                // check if we are dealing with the callback we are searching for
                if (filename == logFile->GetFileName())
                {
                    return logFileCallback;
                }
            }
        }

        // nothing has been found
        return nullptr;
    }


    // return number of log callbacks in the stack
    uint32 LogManager::GetNumLogCallbacks() const
    {
        return mLogCallbacks.GetLength();
    }


    // collect all enabled log levels
    void LogManager::InitLogLevels()
    {
        // reset the loglevels
        int32 logLevels = LogCallback::LOGLEVEL_NONE;

        // enable all log levels that are enabled by any of the callbacks
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            logLevels |= (int32)mLogCallbacks[i]->GetLogLevels();
        }

        mLogLevels = (LogCallback::ELogLevel)logLevels;
    }


    // force set the given log levels to all callbacks
    void LogManager::SetLogLevels(LogCallback::ELogLevel logLevels)
    {
        // iterate through all log callbacks and set it to the given log levels
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            mLogCallbacks[i]->SetLogLevels(logLevels);
        }

        // force set the log manager's log levels to the given one as well
        mLogLevels = logLevels;
    }


    // the main logging method
    void LogManager::LogMessage(const char* message, LogCallback::ELogLevel logLevel)
    {
        LockGuard lock(mMutex);

        // iterate through all callbacks
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            if (mLogCallbacks[i]->GetLogLevels() & logLevel)
            {
                mLogCallbacks[i]->Log(message, logLevel);
            }
        }
    }


    // find the index of a given callback
    uint32 LogManager::FindLogCallback(LogCallback* callback) const
    {
        // iterate through all callbacks
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            if (mLogCallbacks[i] == callback)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    void LogFatalError(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_FATAL)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            //FormatWString(textBuf, 4096, what, args);
        #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
            vsnprintf_s(textBuf, 4096, what, args);
        #else
            vsnprintf(textBuf, 4096, what, args);
        #endif
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_FATAL);
        }
    }


    void LogError(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_ERROR)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            //FormatWString(textBuf, 4096, what, args);

        #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
            vsnprintf_s(textBuf, 4096, what, args);
        #else
            vsnprintf(textBuf, 4096, what, args);
        #endif

            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_ERROR);
        }
    }


    void LogWarning(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_WARNING)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            //FormatWString(textBuf, 4096, what, args);

        #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
            vsnprintf_s(textBuf, 4096, what, args);
        #else
            vsnprintf(textBuf, 4096, what, args);
        #endif
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_WARNING);
        }
    }


    void LogInfo(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_INFO)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            //FormatWString(textBuf, 4096, what, args);
        #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
            vsnprintf_s(textBuf, 4096, what, args);
        #else
            vsnprintf(textBuf, 4096, what, args);
        #endif

            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_INFO);
        }
    }


    void LogDetailedInfo(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_DETAILEDINFO)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            //FormatWString(textBuf, 4096, what, args);

        #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
            vsnprintf_s(textBuf, 4096, what, args);
        #else
            vsnprintf(textBuf, 4096, what, args);
        #endif

            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_DETAILEDINFO);
        }
    }


    void LogDebug(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_DEBUG)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            //FormatWString(textBuf, 4096, what, args);

        #if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
            vsnprintf_s(textBuf, 4096, what, args);
        #else
            vsnprintf(textBuf, 4096, what, args);
        #endif
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_DEBUG);
        }
    }


    // print a debug line to the visual studio output, or console output, etc
    void Print(const char* message)
    {
        // output to the Visual Studio debug window
    #if (defined(MCORE_PLATFORM_WINDOWS))
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
    #elif (defined(MCORE_PLATFORM_ANDROID))
        __android_log_print(ANDROID_LOG_INFO, "MCore", message);
    #else
        std::cout << message << "\n";
    #endif
    }


    // format an std string
    AZStd::string FormatStdString(const char* fmt, ...)
    {
        int size = ((int)strlen(fmt)) * 2 + 128; // guess an initial size
        AZStd::string result;
        va_list ap;
        for (;; )
        {
            result.resize(size);
            va_start(ap, fmt);
            const int n = azvsnprintf((char*)result.data(), size, fmt, ap);
            va_end(ap);

            if (n > -1 && n < size)
            {
                result.resize(n);
                return result;
            }

            if (n > -1)     // needed size returned
            {
                size = n + 1; // +1 for null char
            }
            else
            {
                size *= 2;  // guess at a larger size (OS specific)
            }
        }
    }
}   // namespace MCore
