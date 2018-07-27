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

#include "native/utilities/BatchApplicationManager.h"
#include "native/utilities/AssetUtilEBusHelper.h"
#include <QFileSystemWatcher>
#include <QMap>
#include <QAtomicInt>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <native/ui/MainWindow.h>
#include <QSystemTrayIcon>

class ConnectionManager;
class IniConfiguration;
class ApplicationServer;
class FileServer;
class ShaderCompilerManager;
class ShaderCompilerModel;

namespace AssetProcessor
{
    class AssetRequestHandler;
}

//! This class is the Application manager for the GUI Mode


class GUIApplicationManager
    : public BatchApplicationManager
    , public AssetProcessor::MessageInfoBus::Handler
{
    Q_OBJECT
public:
    explicit GUIApplicationManager(int* argc, char*** argv, QObject* parent = 0);
    virtual ~GUIApplicationManager();

    ApplicationManager::BeforeRunStatus BeforeRun() override;
    IniConfiguration* GetIniConfiguration() const;
    FileServer* GetFileServer() const;
    ShaderCompilerManager* GetShaderCompilerManager() const;
    ShaderCompilerModel* GetShaderCompilerModel() const;

    bool Run() override;
    ////////////////////////////////////////////////////
    ///MessageInfoBus::Listener interface///////////////
    void NegotiationFailed() override;
    void OnAssetFailed(const AZStd::string& sourceFileName) override;
    ///////////////////////////////////////////////////

    //! TraceMessageBus::Handler
    bool OnError(const char* window, const char* message) override;
    bool OnAssert(const char* message) override;

protected:
    bool Activate() override;
    bool PostActivate() override;
    void CreateQtApplication() override;
    void InitConnectionManager() override;
    void InitIniConfiguration();
    void DestroyIniConfiguration();
    void InitFileServer();
    void DestroyFileServer();
    void InitShaderCompilerManager();
    void DestroyShaderCompilerManager();
    void InitShaderCompilerModel();
    void DestroyShaderCompilerModel();
    void Destroy() override;

Q_SIGNALS:
    void ShowWindow();

protected Q_SLOTS:
    void FileChanged(QString path);
    void DirectoryChanged(QString path);
    void ShowMessageBox(QString title, QString msg, bool isCritical);
    void ShowTrayIconMessage(QString msg);
    void ShowTrayIconErrorMessage(QString msg);

private:
    bool Restart();

    IniConfiguration* m_iniConfiguration = nullptr;
    FileServer* m_fileServer = nullptr;
    ShaderCompilerManager* m_shaderCompilerManager = nullptr;
    ShaderCompilerModel* m_shaderCompilerModel = nullptr;
    QFileSystemWatcher m_fileWatcher;
    AZ::UserSettingsProvider m_localUserSettings;
    bool m_messageBoxIsVisible = false;
    bool m_startedSuccessfully = true;

    QPointer<QSystemTrayIcon> m_trayIcon;
    QPointer<MainWindow> m_mainWindow;

    AZStd::chrono::system_clock::time_point m_timeWhenLastWarningWasShown;
};
