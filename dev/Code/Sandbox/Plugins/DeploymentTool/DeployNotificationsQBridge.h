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

#include "DeployNotificationsBus.h"

#include <QObject.h>


namespace DeployTool
{
    // listener of 'uber' ebus and re-routes its messages through the Qt message pump system
    // so any events that modify UI will be processed on the UI thread.
    class NotificationsQBridge
        : public QObject
        , public Notifications::Bus::Handler
    {
        Q_OBJECT
    public:
        NotificationsQBridge(QObject* parent = nullptr);
        ~NotificationsQBridge();


        void DebugLog(LogStream stream, const AZStd::string& message) override;
        void InfoLog(LogStream stream, const AZStd::string& message) override;
        void WarningLog(LogStream stream, const AZStd::string& message) override;
        void ErrorLog(LogStream stream, const AZStd::string& message) override;

        void RemoteLogConnectionStateChange(bool isConnected) override;

        void DeployProcessStatusChange(const AZStd::string& status) override;
        void DeployProcessFinished(bool success) override;


    signals:
        void OnLog(LogStream stream, LogSeverity severity, const QString& message);
        void OnRemoteLogConnectionStateChange(bool isConnected);
        void OnDeployProcessStatusChange(const QString& status);
        void OnDeployProcessFinished(bool success);
    };
} // namespace DeployTool

// expose the enums to the Qt serializer in order to use them in a queued connection
// required to be outside the namespace
Q_DECLARE_METATYPE(DeployTool::LogStream)
Q_DECLARE_METATYPE(DeployTool::LogSeverity)
