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
#include "DeploymentTool_precompiled.h"
#include "DeployNotificationsQBridge.h"


namespace DeployTool
{
    NotificationsQBridge::NotificationsQBridge(QObject* parent)
        : QObject(parent)
    {
        Notifications::Bus::Handler::BusConnect();
    }

    NotificationsQBridge::~NotificationsQBridge()
    {
        Notifications::Bus::Handler::BusDisconnect();
    }

    void NotificationsQBridge::DebugLog(LogStream stream, const AZStd::string& message)
    {
        emit OnLog(stream, LogSeverity::Debug, QString(message.c_str()));
    }

    void NotificationsQBridge::InfoLog(LogStream stream, const AZStd::string& message)
    {
        emit OnLog(stream, LogSeverity::Info, QString(message.c_str()));
    }

    void NotificationsQBridge::WarningLog(LogStream stream, const AZStd::string& message)
    {
        emit OnLog(stream, LogSeverity::Warn, QString(message.c_str()));
    }

    void NotificationsQBridge::ErrorLog(LogStream stream, const AZStd::string& message)
    {
        emit OnLog(stream, LogSeverity::Error, QString(message.c_str()));
    }

    void NotificationsQBridge::RemoteLogConnectionStateChange(bool isConnected)
    {
        emit OnRemoteLogConnectionStateChange(isConnected);
    }

    void NotificationsQBridge::DeployProcessStatusChange(const AZStd::string& status)
    {
        emit OnDeployProcessStatusChange(QString(status.c_str()));
    }

    void NotificationsQBridge::DeployProcessFinished(bool success)
    {
        emit OnDeployProcessFinished(success);
    }
} // namespace DeployTool

#include <DeployNotificationsQBridge.moc>
