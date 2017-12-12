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

#include <QFileSystemWatcher>
#include <QDateTime>
#include <QObject>
#include <QMap>

class QFileInfo;

class FileChangeMonitor
    : public QObject
{
    Q_OBJECT
             Q_ENUMS(MonitoredFileType)

public:

    FileChangeMonitor();
    ~FileChangeMonitor();

    enum class MonitoredFileType
    {
        ProjectSettings,
        UserSettings,
        ProjectTemplate,
        DeploymentTemplate,
        DeploymentAccessTemplate,
        ResourceGroupTemplate,
        AWSCredentials,
        GUIRefresh,
        GemsFile
    };

    void BeginMonitoring(MonitoredFileType fileType, const QString& fileName);

signals:

    void FileChanged(FileChangeMonitor::MonitoredFileType fileType, const QString&);

private slots:

    void OnFileChanged(const QString&);
    void OnDirectoryChanged(const QString&);

private:

    struct MonitoredFile
    {
        MonitoredFile(MonitoredFileType _fileType, QDateTime _modificationTime)
            : fileType{_fileType}
            , lastModified{_modificationTime}
        {}

        MonitoredFile(const MonitoredFile&) = default;
        MonitoredFile& operator=(const MonitoredFile&) = default;

        MonitoredFileType fileType;
        QDateTime lastModified;
    };

    QFileSystemWatcher m_watcher;
    QMap<QString, MonitoredFile> m_files;

    void Monitor(const QFileInfo& fileInfo);
};

Q_DECLARE_METATYPE(FileChangeMonitor::MonitoredFileType);