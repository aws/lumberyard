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

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

#include <QFileSystemWatcher>
#include <QDateTime>
#include <QObject>
#include <QMap>

#include <StaticDataMonitorEditorPlugin.h>

#include <StaticData/StaticDataBus.h>
#include <IStaticDataMonitor.h>

class QFileInfo;


namespace CloudCanvas
{
    namespace StaticData
    {

        class StaticDataMonitor : public QObject, public StaticDataMonitorRequestBus::Handler
        {

            Q_OBJECT
        public:
            StaticDataMonitor();
            ~StaticDataMonitor();

            void RemoveAll() override;

            void AddPath(const AZStd::string& sanitizedPath, bool isFile) override;
            void RemovePath(const AZStd::string& sanitizedPath) override;

            AZStd::string GetSanitizedName(const char* pathName) const override; // Do any sort of path sanitizing so output events line up

        signals:

            void FileChanged(const QString&);

        private slots:

            void OnFileChanged(const QString&);
            void OnDirectoryChanged(const QString&);

        private:

            QFileSystemWatcher m_watcher;

            void Monitor(const QFileInfo& fileInfo);

            AZStd::unordered_set<AZStd::string> m_monitoredFiles;
        };
    }
}