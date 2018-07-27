/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file LogMacros.hpp
 * @brief
 */

#pragma once

#include "util/Core_EXPORTS.hpp"

#include "util/logging/LogLevel.hpp"
#include "util/logging/Logging.hpp"
#include "util/logging/LogSystemInterface.hpp"
#include "util/memory/stl/StringStream.hpp"

// While macros are usually grotty, using them here lets us have a simple function call interface for logging that
//
//  (1) Can be compiled out completely, so you don't even have to pay the cost to check the log level (which will be a virtual function call and a std::atomic<> read) if you don't want any AWS logging
//  (2) If you use logging and the log statement doesn't pass the conditional log filter level, not only do you not pay the cost of building the log string, you don't pay the cost for allocating or
//      getting any of the values used in building the log string, as they're in a scope (if-statement) that never gets entered.

#ifdef DISABLE_AWS_LOGGING

    #define AWS_LOG(level, tag, ...) 
    #define AWS_LOG_FATAL(tag, ...) 
    #define AWS_LOG_ERROR(tag, ...) 
    #define AWS_LOG_WARN(tag, ...) 
    #define AWS_LOG_INFO(tag, ...) 
    #define AWS_LOG_DEBUG(tag, ...) 
    #define AWS_LOG_TRACE(tag, ...) 

    #define AWS_LOGSTREAM(level, tag, streamExpression) 
    #define AWS_LOGSTREAM_FATAL(tag, streamExpression)
    #define AWS_LOGSTREAM_ERROR(tag, streamExpression)
    #define AWS_LOGSTREAM_WARN(tag, streamExpression)
    #define AWS_LOGSTREAM_INFO(tag, streamExpression)
    #define AWS_LOGSTREAM_DEBUG(tag, streamExpression)
    #define AWS_LOGSTREAM_TRACE(tag, streamExpression)

#else

    #define AWS_LOG(level, tag, ...) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= level ) \
            { \
                logSystem->Log(level, tag, __VA_ARGS__); \
            } \
        }

    #define AWS_LOG_FATAL(tag, ...) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Fatal ) \
            { \
                logSystem->Log(awsiotsdk::util::Logging::LogLevel::Fatal, tag, __VA_ARGS__); \
            } \
        }

    #define AWS_LOG_ERROR(tag, ...) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Error ) \
            { \
                logSystem->Log(awsiotsdk::util::Logging::LogLevel::Error, tag, __VA_ARGS__); \
            } \
        }

    #define AWS_LOG_WARN(tag, ...) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Warn ) \
            { \
                logSystem->Log(awsiotsdk::util::Logging::LogLevel::Warn, tag, __VA_ARGS__); \
            } \
        }

    #define AWS_LOG_INFO(tag, ...) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Info ) \
            { \
                logSystem->Log(awsiotsdk::util::Logging::LogLevel::Info, tag, __VA_ARGS__); \
            } \
        }

    #define AWS_LOG_DEBUG(tag, ...) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Debug ) \
            { \
                logSystem->Log(awsiotsdk::util::Logging::LogLevel::Debug, tag, __VA_ARGS__); \
            } \
        }

    #define AWS_LOG_TRACE(tag, ...) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Trace ) \
            { \
                logSystem->Log(awsiotsdk::util::Logging::LogLevel::Trace, tag, __VA_ARGS__); \
            } \
        }

    #define AWS_LOGSTREAM(level, tag, streamExpression) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= level ) \
            { \
                awsiotsdk::OStringStream logStream; \
                logStream << streamExpression; \
                logSystem->LogStream( logLevel, tag, logStream ); \
            } \
        }

    #define AWS_LOGSTREAM_FATAL(tag, streamExpression) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Fatal ) \
            { \
                awsiotsdk::OStringStream logStream; \
                logStream << streamExpression; \
                logSystem->LogStream( awsiotsdk::util::Logging::LogLevel::Fatal, tag, logStream ); \
            } \
        }

    #define AWS_LOGSTREAM_ERROR(tag, streamExpression) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Error ) \
            { \
                awsiotsdk::OStringStream logStream; \
                logStream << streamExpression; \
                logSystem->LogStream( awsiotsdk::util::Logging::LogLevel::Error, tag, logStream ); \
            } \
        }

    #define AWS_LOGSTREAM_WARN(tag, streamExpression) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Warn ) \
            { \
                awsiotsdk::OStringStream logStream; \
                logStream << streamExpression; \
                logSystem->LogStream( awsiotsdk::util::Logging::LogLevel::Warn, tag, logStream ); \
            } \
        }

    #define AWS_LOGSTREAM_INFO(tag, streamExpression) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Info ) \
            { \
                awsiotsdk::OStringStream logStream; \
                logStream << streamExpression; \
                logSystem->LogStream( awsiotsdk::util::Logging::LogLevel::Info, tag, logStream ); \
            } \
        }

    #define AWS_LOGSTREAM_DEBUG(tag, streamExpression) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Debug ) \
            { \
                awsiotsdk::OStringStream logStream; \
                logStream << streamExpression; \
                logSystem->LogStream( awsiotsdk::util::Logging::LogLevel::Debug, tag, logStream ); \
            } \
        }

    #define AWS_LOGSTREAM_TRACE(tag, streamExpression) \
        { \
            awsiotsdk::util::Logging::LogSystemInterface* logSystem = awsiotsdk::util::Logging::GetLogSystem(); \
            if ( logSystem && logSystem->GetLogLevel() >= awsiotsdk::util::Logging::LogLevel::Trace ) \
            { \
                awsiotsdk::OStringStream logStream; \
                logStream << streamExpression; \
                logSystem->LogStream( awsiotsdk::util::Logging::LogLevel::Trace, tag, logStream ); \
            } \
        }

#endif // DISABLE_AWS_LOGGING
