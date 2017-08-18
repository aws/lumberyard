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

#include "ManifestDataStructures.h"

#include <QFileSystemWatcher>

namespace DynamicContent
{
    class FileWatcherModel
        : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        FileWatcherModel(QObject* parent);
        ~FileWatcherModel();
 
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        Qt::DropActions supportedDragActions() const override;
        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;

        void Update(const ManifestFiles& fileList);

        static QString fileInfoConstructedByNameAndPlatform(QString fileName, QString platformType) { return fileName + "|" + platformType; };
        static QString fileNameDeconstructedfromfileInfo(QString fileinfo) { return fileinfo.split("|")[0]; };
        static QString platformTyepDeconstructedfromfileInfo(QString fileinfo) { return fileinfo.split("|")[1]; };

        static const int platformColumn = 1;
        static const int InPakColumn = 2;
        static const int PlatformRole = Qt::UserRole + 3;

    signals:
        void fileInPakChanged(QString fileName);

    private:
        ManifestFiles m_files;
        QString m_pathToAssets;
        QFileSystemWatcher* m_fileWatcher;

        QIcon m_checkBoxIcon;

        QString FullFilePath(const ManifestFile file);

    private slots:
        void OnFileChanged(const QString fileName);
    };
}