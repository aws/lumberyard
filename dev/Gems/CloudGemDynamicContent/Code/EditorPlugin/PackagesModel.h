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

#include <QMovie>

#include "ManifestDataStructures.h"

namespace DynamicContent
{
    class QDynamicContentEditorMainWindow;
    
    class PackagesModel
        : public QStandardItemModel
    {
        Q_OBJECT

    public:
        PackagesModel(QDynamicContentEditorMainWindow* parent);
        ~PackagesModel();

        void Update(const ManifestFiles& vpakList, const ManifestFiles& vfileList, const QVariant& vpakStatusMap);
        void UpdateBucketDiffInfo(const QVariant& vBucketDiffInfo);
        void UpdateLocalDiffInfo(const QVariant& vLocalDiffInfo);
        
        struct PakStatus {
            static const int New = 1;
            static const int Outdated = 2;
            static const int Match = 3;
            static const int FileOutdated = 4;
            static const int FileCurrent = 5;
        };

        struct ColumnName {
            static const int PakName = 0;
            static const int Platform = 1;
            static const int Status = 2;
            static const int S3Status = 3;
        };

        static const int PlatformRole = Qt::UserRole + 3;
        static const int S3StatusRole = Qt::UserRole + 4;

        Qt::DropActions supportedDropActions() const override;
        bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
        bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
        QStringList selectedItems(const QMimeData *data) const;

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

        void SetPakStatus(const QString& key, int status);

        void startS3StatusAnimation(QString key);
        void stopS3StatusAnimation(QString message); // ACCEPTED_USE

    private:
        const int NameRole = Qt::UserRole + 1;
        const int StatusRole = Qt::UserRole + 2;
        
        QStringList GetKeyList(const QVariantMap& bucketInfo, const QString& key);
        
        void SetItemStatus(QStandardItem* item, int status);

        void SetUpIcon();
        void OnProgressIndicatorAnimationUpdate(int frame);

        QDynamicContentEditorMainWindow* m_mainWindow;
        QIcon m_pakMissingIcon;
        QIcon m_pakOutOfDateIcon;
        QIcon m_pakUpToDateIcon;
        QIcon m_fileOutdatedIcon;
        QMovie m_inProgressMovie;
        QStandardItem* updatingItem;
        bool m_isUpdating = false;
    };
}
