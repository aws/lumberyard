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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>

#include <AzFramework/StringFunc/StringFunc.h>


// uncomment to enable verbose deploy logs
//#define ENABLE_DEPLOY_DEBUG_LOG


namespace DeployTool
{
    enum class LogStream : unsigned char
    {
        DeployOutput,
        RemoteDeviceOutput
    };

    enum class LogSeverity : unsigned char
    {
        Debug,
        Info,
        Warn,
        Error
    };


    // thread safe 'uber' ebus for dispatching messages to the deploy tool main window
    class Notifications
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<Notifications>;

        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        using MutexType = AZStd::mutex;

        virtual ~Notifications() {}

        virtual void DebugLog(LogStream stream, const AZStd::string& message) = 0;
        virtual void InfoLog(LogStream stream, const AZStd::string& message) = 0;
        virtual void WarningLog(LogStream stream, const AZStd::string& message) = 0;
        virtual void ErrorLog(LogStream stream, const AZStd::string& message) = 0;

        virtual void RemoteLogConnectionStateChange(bool isConnected) = 0;

        virtual void DeployProcessStatusChange(const AZStd::string& status) = 0;
        virtual void DeployProcessFinished(bool success) = 0;
    };
} // namespace DeployTool


#define _DEPLOY_LOG_ARGS_(_type, _stream, ...) \
    { \
        AZStd::string message = AZStd::move(AZStd::string::format(__VA_ARGS__)); \
        AzFramework::StringFunc::TrimWhiteSpace(message, /*leading =*/ false, /*trailing =*/ true); \
        DeployTool::Notifications::Bus::Broadcast(_type, _stream, message); \
    }

#define _DEPLOY_LOG_(_type, _stream, _message) \
    { \
        AZStd::string message(_message); \
        AzFramework::StringFunc::TrimWhiteSpace(message, /*leading =*/ false, /*trailing =*/ true); \
        DeployTool::Notifications::Bus::Broadcast(_type, _stream, message); \
    }


#if defined(ENABLE_DEPLOY_DEBUG_LOG)
    #define DEPLOY_LOG_DEBUG(...) _DEPLOY_LOG_ARGS_(&DeployTool::Notifications::DebugLog, DeployTool::LogStream::DeployOutput, __VA_ARGS__)
    #define REMOTE_LOG_DEBUG(...) _DEPLOY_LOG_ARGS_(&DeployTool::Notifications::DebugLog, DeployTool::LogStream::RemoteDeviceOutput, __VA_ARGS__)
#else
    #define DEPLOY_LOG_DEBUG(...)
    #define REMOTE_LOG_DEBUG(...)
#endif // defined(ENABLE_DEPLOY_DEBUG_LOG)

#define DEPLOY_LOG_INFO(...)  _DEPLOY_LOG_ARGS_(&DeployTool::Notifications::InfoLog,    DeployTool::LogStream::DeployOutput, __VA_ARGS__)
#define DEPLOY_LOG_WARN(...)  _DEPLOY_LOG_ARGS_(&DeployTool::Notifications::WarningLog, DeployTool::LogStream::DeployOutput, __VA_ARGS__)
#define DEPLOY_LOG_ERROR(...) _DEPLOY_LOG_ARGS_(&DeployTool::Notifications::ErrorLog,   DeployTool::LogStream::DeployOutput, __VA_ARGS__)

#define REMOTE_LOG_INFO(_message)  _DEPLOY_LOG_(&DeployTool::Notifications::InfoLog,    DeployTool::LogStream::RemoteDeviceOutput, _message)
#define REMOTE_LOG_WARN(_message)  _DEPLOY_LOG_(&DeployTool::Notifications::WarningLog, DeployTool::LogStream::RemoteDeviceOutput, _message)
#define REMOTE_LOG_ERROR(_message) _DEPLOY_LOG_(&DeployTool::Notifications::ErrorLog,   DeployTool::LogStream::RemoteDeviceOutput, _message)
