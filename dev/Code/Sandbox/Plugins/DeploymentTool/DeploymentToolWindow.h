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

#include <QScopedPointer>
#include <QtWidgets/QMainWindow>

#include <IEditor.h>

#include "BootstrapConfigContainer.h"
#include "DeploymentConfig.h"
#include "DeployNotificationsQBridge.h"
#include "RemoteLog.h"


namespace Ui
{
    class DeploymentToolWindow;
}

class DeployWorkerBase;

class QTimer;


// main window for deployment tool
class DeploymentToolWindow
    : public QMainWindow
{
    Q_OBJECT
public:
    // you are required to implement this to satisfy the unregister/registerclass requirements on "AzToolsFramework::RegisterViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {75D24BBB-F0AA-49AA-B92A-276C404BDC33}
        static const GUID guid = {
            0x75d24bbb, 0xf0aa, 0x49aa, { 0xb9, 0x2a, 0x27, 0x6c, 0x40, 0x4b, 0xdc, 0x33 }
        };
        return guid;
    }

    DeploymentToolWindow();
    ~DeploymentToolWindow();


private:
    AZ_DISABLE_COPY_MOVE(DeploymentToolWindow);

    // callbacks
    void OnDeployStatusChange(const QString& status);
    void OnDeployFinished(bool success);
    void OnRemoteLogConnectionStateChange(bool isConnected);
    void OnPlatformChanged(const QString& currentplatform);
    void OnAssetModeChanged(const QString& currentMode);
    void OnShaderCompilerUseAPToggle(int state);
    void OnDevicePollTimer();
    void OnDeployClick();

    void ToggleInteractiveUI(bool enabled);
    void UpdateIpAddressOptions();

    void UpdateUiFromSystemConfig(const char* systemConfigFile);
    bool PopulateDeploymentConfigFromUi();

    void Run();

    void InternalDeployFailure(const AZStd::string& reason);
    void LogToWindow(DeployTool::LogStream stream, DeployTool::LogSeverity logSeverity, const QString& message);

    void Reset();
    void InitializeUi();
    void SaveUiState();


    QScopedPointer<Ui::DeploymentToolWindow> m_ui;
    QMovie m_animatedSpinningIcon;

    QIcon m_connectedIcon;
    QIcon m_notConnectedIcon;

    QBrush m_defaultDeployOutputBrush;
    QBrush m_defaultRemoteDeviceOutputBrush;

    QScopedPointer<QTimer> m_devicePollTimer;
    QScopedPointer<DeployWorkerBase> m_deployWorker;

    QSettings m_deploySettings;

    DeployTool::RemoteLog m_remoteLog;
    DeployTool::NotificationsQBridge m_notificationsBridge;

    DeploymentConfig m_deploymentCfg;
    BootstrapConfigContainer m_bootstrapConfig;

    QString m_lastDeployedDevice;
};
