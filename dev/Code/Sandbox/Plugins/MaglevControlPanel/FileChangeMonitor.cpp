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

#include "stdafx.h"
#include "FileChangeMonitor.h"

#include <FileChangeMonitor.moc>

#include <QFileInfo>
#include <QDir>
#include <QDebug>

FileChangeMonitor::FileChangeMonitor()
    : QObject{}
{
    qRegisterMetaType<FileChangeMonitor::MonitoredFileType>("FileChangeMonitor::MonitoredFileType");
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &FileChangeMonitor::OnFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileChangeMonitor::OnDirectoryChanged);
}

void FileChangeMonitor::BeginMonitoring(MonitoredFileType fileType, const QString& fileName)
{
    QFileInfo fileInfo {
        fileName
    };

    if (!m_files.contains(fileInfo.absoluteFilePath())) // only monitor a given file once
    {
        qDebug() << "BeginMonitoring" << fileName;
        QString key = QDir::toNativeSeparators(fileInfo.absoluteFilePath());
#ifdef WIN32
        // The drive letter has been observed to switch from lower to upper case when
        // changes are reported by the QFileSystemWatcher. For windows we force the
        // entire path to be lowercase so it doesn't matter.
        key = key.toLower();
#endif
        m_files.insert(key, MonitoredFile {fileType, fileInfo.lastModified()});
        Monitor(fileInfo);
    }
}

FileChangeMonitor::~FileChangeMonitor()
{
}

void FileChangeMonitor::Monitor(const QFileInfo& fileInfo)
{
    if (fileInfo.exists())
    {
        QString path = QDir::toNativeSeparators(fileInfo.absoluteFilePath());
        qDebug() << "Monitoring File" << path;
        m_watcher.addPath(path);
    }
    else
    {
        QDir dir = fileInfo.dir();
        QDir lastDir = dir;
        // The while loop is here for the situation where the target folder hasn't been
        // created yet, and we want to watch for its creation. In this case, the
        // function will walk back up the path until if finds a valid directory and
        // will monitor that for changes allowing a signal when the new directory
        // is created. Note that if there are a depth of directories created this is
        // correctly handled in OnFileChanged as it compares the change in the monitored
        // directory to see if intermediate directories are created and puts a new
        // monitor on that folder
        while (!dir.exists())
        {
            dir.cdUp();
            // cdUp can in some cases fail and not change the directory.
            // We want to bail in these cases
            // Note that the return value of cdUp can't be used as it's valid for the directory
            // to not exist yet (as per the above comment) but we still want to check for
            // the validity of a directory, for example, if we attempt to watch a folder
            // on a drive that doesn't exist
            if (dir == lastDir)
            {
                QString outPath = QDir::toNativeSeparators(dir.absolutePath());
                qDebug() << "Failed to monitor " << outPath;
                return;
            }
            lastDir = dir;
        }

        QString path = QDir::toNativeSeparators(dir.absolutePath());
        qDebug() << "Monitoring Dir" << path;
        m_watcher.addPath(path);
    }
}

void FileChangeMonitor::OnFileChanged(const QString& fileName)
{
    qDebug() << "OnFileChanged" << fileName;

    QString key = fileName;
#ifdef WIN32
    // The drive letter has been observed to switch from lower to upper case when
    // changes are reported by the QFileSystemWatcher. For windows we force the
    // entire path to be lowercase so it doesn't matter.
    key = key.toLower();
#endif

    auto it = m_files.find(key);
    if (it != m_files.end())
    {
        MonitoredFile& monitoredFile = it.value();
        QFileInfo fileInfo {
            fileName
        };

        if (fileInfo.exists())
        {
            qDebug() << "File exists" << monitoredFile.lastModified << fileInfo.lastModified();
            if (monitoredFile.lastModified != fileInfo.lastModified())
            {
                // If the file didn't exist before but does now, start monitoring it.
                if (monitoredFile.lastModified.isNull())
                {
                    Monitor(fileInfo);
                }
                monitoredFile.lastModified = fileInfo.lastModified();
                qDebug() << "emit FileChanged" << fileName;
                FileChanged(monitoredFile.fileType, fileName);
            }
        }
        else
        {
            qDebug() << "File doesn't exist" << monitoredFile.lastModified << fileInfo.lastModified();
            // If previously got a modification time, file must have been deleted.
            if (!monitoredFile.lastModified.isNull())
            {
                monitoredFile.lastModified = QDateTime {};
                qDebug() << "emit FileChanged" << fileName;
                FileChanged(monitoredFile.fileType, fileName);
            }
            // Ensure that we are monitoring the file's directory so we know if the
            // file is recreated. We do this every time in case intermediate
            // directories have been created.
            Monitor(fileInfo);
        }
    }
}

void FileChangeMonitor::OnDirectoryChanged(const QString& directoryName)
{
    QString keyPrefix = directoryName;
#ifdef WIN32
    // The drive letter has been observed to switch from lower to upper case when
    // changes are reported by the QFileSystemWatcher. For windows we force the
    // entire path to be lowercase so it doesn't matter.
    keyPrefix = keyPrefix.toLower();
#endif

    for (auto it = m_files.constBegin(); it != m_files.constEnd(); ++it)
    {
        // If file path starts with directory path, process as file change
        if (it.key().startsWith(keyPrefix))
        {
            OnFileChanged(it.key());
        }
    }
}


