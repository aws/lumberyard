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

#include "CloudGemDynamicContent_precompiled.h"

#include "PackagesModel.h"
#include "ManifestTableKeys.h"
#include "QDynamicContentEditorMainWindow.h"

#include <PackagesModel.moc>

namespace DynamicContent
{
    PackagesModel::PackagesModel(QDynamicContentEditorMainWindow* parent) :
        QStandardItemModel(parent),
        m_mainWindow(parent),
        m_pakMissingIcon("Editor/Icons/CloudCanvas/Pak_Not_Uploaded.png"),
        m_pakOutOfDateIcon("Editor/Icons/CloudCanvas/Pak_Reupload.png"),
        m_pakUpToDateIcon("Editor/Icons/CloudCanvas/Pak_Uploaded.png"),
        m_fileOutdatedIcon("Editor/Icons/CloudCanvas/Pak_File_Warning.png"),
        m_inProgressMovie("Editor/Icons/CloudCanvas/in_progress.gif")
    {
        // Initial progress icon setup
        if (m_inProgressMovie.loopCount() != -1)    // Ensure animation will loop
        {
            connect(&m_inProgressMovie, &QMovie::finished, &m_inProgressMovie, &QMovie::start);
        }
        m_inProgressMovie.start();
    }

    PackagesModel::~PackagesModel()
    {
    }

    void PackagesModel::Update(const ManifestFiles& vpakList, const ManifestFiles& vfileList, const QVariant& vpakStatusMap)
    {
        auto statusMap = vpakStatusMap.value<QVariantMap>();
        auto expanded = QMap<QString, bool>();

        // make a map of the files so we can append them to paks
        QMap<QString, ManifestFiles> filesInPak;
        for (auto file : vfileList)
        {
            auto pakPath = file.pakFile;
            if (!pakPath.isEmpty())
            {
                if (!filesInPak.contains(pakPath))
                {
                    filesInPak[pakPath] = ManifestFiles();
                }
                filesInPak[pakPath].append(file);
            }
        }

        beginResetModel();

        QStandardItem* root = invisibleRootItem();

        int rowCount = root->rowCount();
        for (int row = 0; row < rowCount; ++row)
        {
            QStandardItem* pakItem = root->child(row);
            // remember the existing status of the paks
            int status = item(row, ColumnName::S3Status)->data(StatusRole).toInt();
            if (!statusMap.contains(pakItem->text()))
            {
                if (status == PakStatus::New)
                {
                    statusMap[pakItem->text()] = KEY_NEW;
                }
                else if (status == PakStatus::Outdated)
                {
                    statusMap[pakItem->text()] = KEY_OUTDATED;
                }
                else if (status == PakStatus::Match)
                {
                    statusMap[pakItem->text()] = KEY_MATCH;
                }
            }
            else
            {
                // this is a little hokey but override "updated" status if this
                // is actually new
                if (status == PakStatus::New && statusMap[pakItem->text()] == KEY_OUTDATED)
                {
                    statusMap[pakItem->text()] = KEY_NEW;
                }
            }
            expanded[pakItem->text()] = m_mainWindow->treePackages->isExpanded(pakItem->index());
            pakItem->removeRows(ColumnName::PakName, pakItem->rowCount());
        }
        root->removeRows(ColumnName::PakName, rowCount);

        for (auto pak : vpakList)
        {
            auto pakName = pak.keyName;
            auto pakPath = pak.pakFile;
            QStandardItem* pakItem = new QStandardItem(pakName);
            pakItem->setDropEnabled(true);

            QStandardItem* s3StatusItem = new QStandardItem;
            s3StatusItem->setDropEnabled(false);

            if (statusMap.contains(pakName))
            {
                if (statusMap[pakName] == KEY_NEW)
                {
                    SetItemStatus(s3StatusItem, PakStatus::New);
                }
                else if (statusMap[pakName] == KEY_OUTDATED)
                {
                    SetItemStatus(s3StatusItem, PakStatus::Outdated);
                }
                else if (statusMap[pakName] == KEY_MATCH)
                {
                    SetItemStatus(s3StatusItem, PakStatus::Match);
                }
            }
            pakItem->setData(pak.keyName, NameRole);

            auto platformType = pak.platformType;
            QStandardItem* platformItem = new QStandardItem();
            platformItem->setDropEnabled(false);
            QMap<QString, QDynamicContentEditorMainWindow::PlatformInfo> platformMap = QDynamicContentEditorMainWindow::PlatformMap();
            auto pixMap = QPixmap(platformMap[pak.platformType].SmallSizeIconLink);
            platformItem->setData(pixMap, Qt::DecorationRole);
            platformItem->setData(pak.platformType, PlatformRole);
            
            int fileCount = 0;
            if (filesInPak.contains(pakPath))
            {
                fileCount = filesInPak[pakPath].count();
                for (auto file : filesInPak[pakPath])
                {
                    auto fileName = QDir(file.localFolder).filePath(file.keyName);
                    auto fileItem = new QStandardItem(fileName);
                    fileItem->setDropEnabled(false);
                    fileItem->setData(file.keyName, NameRole);

                    auto filePlatform = file.platformType;
                    auto filePlatformItem = new QStandardItem();
                    filePlatformItem->setDropEnabled(false);
                    auto pixMap = QPixmap(platformMap[filePlatform].SmallSizeIconLink);
                    filePlatformItem->setData(pixMap, Qt::DecorationRole);
                    filePlatformItem->setData(filePlatform, PlatformRole);

                    QList<QStandardItem*> fileRowItems;
                    fileRowItems << fileItem;
                    fileRowItems << filePlatformItem;
                    pakItem->appendRow(fileRowItems);
                }
            }
            QStandardItem* pakFileCountItem = new QStandardItem(QString::number(fileCount));
            pakFileCountItem->setDropEnabled(false);
            QList<QStandardItem*> rowItems;
            rowItems << pakItem;
            rowItems << platformItem;
            rowItems << pakFileCountItem;
            rowItems << s3StatusItem;
            root->appendRow(rowItems);
        }
    
        endResetModel();

        rowCount = root->rowCount();
        for (int row = 0; row < rowCount; ++row)
        {
            QStandardItem* item = root->child(row);
            if (expanded.contains(item->text()))
            {
                if (expanded[item->text()] == true)
                {
                    m_mainWindow->treePackages->expand(item->index());
                }
            }

        }

        auto tree = m_mainWindow->treePackages;
        tree->setColumnWidth(ColumnName::PakName, tree->width() * QDynamicContentEditorMainWindow::PackagesNameColumnWidth());
        tree->setColumnWidth(ColumnName::Platform, tree->width() * QDynamicContentEditorMainWindow::PackagesPlatformColumnWidth());
        tree->setColumnWidth(ColumnName::Status, tree->width() * QDynamicContentEditorMainWindow::PackagesStatusColumnWidth());
        tree->setColumnWidth(ColumnName::S3Status, tree->width() * QDynamicContentEditorMainWindow::PackagesS3StatusColumnWidth());
    }

    void PackagesModel::UpdateBucketDiffInfo(const QVariant& vBucketDiffInfo)
    {
        QVariantMap itemMap = vBucketDiffInfo.value<QVariantMap>();

        auto newItems = GetKeyList(itemMap, KEY_NEW);
        auto outdatedItems = GetKeyList(itemMap, KEY_OUTDATED);
        auto matchItems = GetKeyList(itemMap, KEY_MATCH);

        QStandardItem* root = invisibleRootItem();
        int rowCount = root->rowCount();
        for (int row = 0; row < rowCount; ++row)
        {
            QStandardItem* pakItem = root->child(row);
            auto itemName = pakItem->data(NameRole).toString();
            if (newItems.contains(itemName))
            {
                SetItemStatus(item(row, ColumnName::S3Status), PakStatus::New);
            }
            else if (outdatedItems.contains(itemName))
            {
                if (item(row, ColumnName::S3Status)->data(StatusRole).toInt() != PakStatus::New)
                {
                    SetItemStatus(item(row, ColumnName::S3Status), PakStatus::Outdated);
                }
            }
        }
    }

    void PackagesModel::UpdateLocalDiffInfo(const QVariant& vLocalDiffInfo)
    {
        auto updateItems = vLocalDiffInfo.toStringList();
        QStandardItem* root = invisibleRootItem();
        int pakRowCount = root->rowCount();
        for (int pakRow = 0; pakRow < pakRowCount; ++pakRow)
        {
            QStandardItem* item = root->child(pakRow);
            int fileRowCount = item->rowCount();
            for (int fileRow = 0; fileRow < fileRowCount; ++fileRow)
            {
                QStandardItem* file = item->child(fileRow);
                auto fileName = file->text();
                if (updateItems.contains(fileName))
                {
                    
                    SetItemStatus(file, PakStatus::FileOutdated);
                }
                else
                {
                    SetItemStatus(file, PakStatus::FileCurrent);
                }
            }
        }
    }

    Qt::DropActions PackagesModel::supportedDropActions() const
    {
        return Qt::CopyAction;
    }

    bool PackagesModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
    {
        if(!canDropMimeData(data, action, row, column, parent))
        {
            return false;
        }

        if (action == Qt::IgnoreAction)
        {
            return true;
        }
        
        QStringList newItems = selectedItems(data);
        m_mainWindow->AddFilesToPackageFromFileInfoList(newItems, parent);

        return true;
    }

    bool PackagesModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
    {
        Q_UNUSED(action);
        Q_UNUSED(row);
        Q_UNUSED(column);

        if (!data->hasFormat(MIME_TYPE_TEXT))
        {
            return false;
        }

        // paks are top level parents, so their parent should be invalid
        if (parent.parent().isValid())
        {
            return false;
        }

        QStringList newItems = selectedItems(data);
        if (!m_mainWindow->CheckPlatformType(newItems, parent))
        {
            return false;
        }

        return true;
    }

    QStringList PackagesModel::selectedItems(const QMimeData *data) const
    {
        QByteArray encodedData = data->data(MIME_TYPE_TEXT);
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QStringList newItems;
        while (!stream.atEnd())
        {
            QString text;
            stream >> text;
            newItems << text;
        }

        return newItems;
    }

    QVariant PackagesModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) 
            {
            case ColumnName::PakName:
                return "Package / File Name";
            case ColumnName::Platform:
                return "Platform";
            case ColumnName::Status:
                return "Files";
            case ColumnName::S3Status:
                return "S3 Status";
            }

        }
        return QVariant{};
    }

    void PackagesModel::SetPakStatus(const QString& key, int status)
    {
        QStandardItem* root = invisibleRootItem();
        int rowCount = root->rowCount();
        for (int row = 0; row < rowCount; ++row)
        {
            QStandardItem* pakItem = root->child(row);
            auto itemName = pakItem->data(NameRole).toString();
            if (itemName == key)
            {
                SetItemStatus(item(row, ColumnName::S3Status), status);
            }
        }
    }

    void PackagesModel::SetItemStatus(QStandardItem* item, int status)
    {
        item->setData(status, StatusRole);

        switch (status)
        {
        case PakStatus::New:
            item->setIcon(m_pakMissingIcon);
            item->setData(PakStatus::New, S3StatusRole);
            break;
        case PakStatus::Match:
            item->setIcon(m_pakUpToDateIcon);
            item->setData(PakStatus::Match, S3StatusRole);
            break;
        case PakStatus::Outdated:
            item->setIcon(m_pakOutOfDateIcon);
            item->setData(PakStatus::Outdated, S3StatusRole);
            break;
        case PakStatus::FileOutdated:
            item->setIcon(m_fileOutdatedIcon);
            item->setData(PakStatus::FileOutdated, S3StatusRole);
            break;
        case PakStatus::FileCurrent:
            item->setIcon(QIcon()); 
            item->setData(PakStatus::FileCurrent, S3StatusRole);
            break;
        }
    }

    QStringList PackagesModel::GetKeyList(const QVariantMap& bucketInfo, const QString& key)
    {
        QStringList items;
        QVariantList vItems = bucketInfo[key].value<QVariantList>();
        for (auto&& vItem : vItems)
        {
            items.append(vItem.toString());
        }
        return items;
    }

    void PackagesModel::startS3StatusAnimation(QString key)
    {
        QStandardItem* root = invisibleRootItem();
        int rowCount = root->rowCount();
        for (int row = 0; row < rowCount; ++row)
        {
            QStandardItem* pakItem = root->child(row);
            auto itemName = pakItem->data(NameRole).toString();
            if (itemName == key)
            {
                m_isUpdating = true;
                updatingItem = item(row, ColumnName::S3Status);
                SetUpIcon();
            }
        }
    }

    void PackagesModel::SetUpIcon()
    {
        QIcon iconToSet = QIcon{m_inProgressMovie.currentPixmap()};
        updatingItem->setIcon(iconToSet);
        
        connect(&m_inProgressMovie, &QMovie::frameChanged, this, &PackagesModel::OnProgressIndicatorAnimationUpdate);
    }

    void  PackagesModel::OnProgressIndicatorAnimationUpdate(int frame)
    {
        if (m_isUpdating)
        {
            updatingItem->setIcon(QIcon{m_inProgressMovie.currentPixmap()});
        }
        else
        {
            updatingItem->setIcon(m_pakUpToDateIcon);
            disconnect(&m_inProgressMovie, &QMovie::frameChanged, this, &PackagesModel::OnProgressIndicatorAnimationUpdate);
        }
    }

    void PackagesModel::stopS3StatusAnimation(QString message) // ACCEPTED_USE
    {
        m_isUpdating = false;
    }
}
