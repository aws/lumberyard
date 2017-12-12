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
#include <QtWidgets/QMainWindow>
#include <QtWidgets/qlabel.h>
#include <IEditor.h>
#include <Include/IPlugin.h>

#include "DeploymentConfig.h"
#include "IDeploymentTool.h"

#include "DeploymentUtil.h"

namespace Ui
{
    class DeploymentToolWindow;
}

class AssetSystemListener;
class BootstrapConfigContainer;
class DeploymentUtil;

// main window for deployment tool
class DeploymentToolWindow : public QMainWindow, public IEditorNotifyListener, public IDeploymentTool
{
    Q_OBJECT
public:
    DeploymentToolWindow();
    DeploymentToolWindow(const DeploymentToolWindow& rhs) = delete;
    DeploymentToolWindow& operator=(const DeploymentToolWindow& rhs) = delete;
    DeploymentToolWindow& operator=(DeploymentToolWindow&& rhs) = delete;
    ~DeploymentToolWindow();

    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override { }

    void Success(const AZStd::string& msg) override;
    void Log(const AZStd::string& msg) override;
    void LogEndLine(const AZStd::string& msg) override;
    void Error(const AZStd::string& msg) override;

    // you are required to implement this to satisfy the unregister/registerclass requirements on "AzToolsFramework::RegisterViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {75D24BBB-F0AA-49AA-B92A-276C404BDC33}
        static const GUID guid =
        { 0x75d24bbb, 0xf0aa, 0x49aa,{ 0xb9, 0x2a, 0x27, 0x6c, 0x40, 0x4b, 0xdc, 0x33 } };

        return guid;
    }
private:

    static const char* s_settingsFileName;

    void ToggleAdvanceOptions();

    void Deploy();
    bool PopulateDeploymentConfigFromUi();
    void WaitForAssetProcessorToFinish();
    void WriteToLog(const AZStd::string& msg);
    void ResetDeployWidget();

    void SetupUi();
    void InitializeUi();

    void SaveUiState();
    void RestoreUiState();

    bool ValidateIPAddress(QString& ipAddressString);

    QScopedPointer<Ui::DeploymentToolWindow> m_ui;
    QIcon m_openIcon;
    QIcon m_closedIcon;
    QMovie m_animatedSpinningIcon;

    QSettings m_deploySettings;

    QScopedPointer<AssetSystemListener> m_assetSystemListener;

    DeploymentConfig m_deploymentCfg;
    BootstrapConfigContainer m_bootstrapConfig;

    DeploymentUtil* m_deploymentUtil;

    bool m_hideAdvancedOptions;
};