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

#include "FileWatcherModel.h"
#include "ManifestTableKeys.h"
#include "QDynamicContentEditorMainWindow.h"
#include <FileWatcherModel.moc>

namespace DynamicContent
{
    enum class ColumnContents : int
    {
        FileName = 0,
        PlatformType = 1, 
        InPak = 2
    };
    
    FileWatcherModel::FileWatcherModel(QObject* parent) :
        QAbstractTableModel(parent),
        m_checkBoxIcon("Editor/Icons/checkmark_checked.png")
    {
        m_pathToAssets = AZ::IO::FileIOBase::GetInstance()->GetAlias("@assets@");
    }

    FileWatcherModel::~FileWatcherModel()
    {
    }

    int FileWatcherModel::rowCount(const QModelIndex &parent /*= QModelIndex()*/) const
    {
        return m_files.size();
    }

    int FileWatcherModel::columnCount(const QModelIndex &parent /*= QModelIndex()*/) const
    {
        return 3;
    }

    QVariant FileWatcherModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
    {
        if (role == Qt::DisplayRole)
        {
            auto file = m_files[index.row()];

            if(index.column() == static_cast<int>(ColumnContents::FileName))
            {
                auto fileName = file.localFolder;
                fileName = Path::FullPathToGamePath(fileName) + '/';
                fileName += file.keyName;
                return fileName;
            }
        }
        else if (role == Qt::DecorationRole)
        {
            auto file = m_files[index.row()];
            if (index.column() == static_cast<int>(ColumnContents::InPak))
            {
                auto inPack = file.pakFile;
                if (!inPack.isEmpty())
                {
                    return m_checkBoxIcon;
                }
            }
            else if (index.column() == static_cast<int>(ColumnContents::PlatformType))
            {
                QMap<QString, QDynamicContentEditorMainWindow::PlatformInfo> platformMap = QDynamicContentEditorMainWindow::PlatformMap();
                return QIcon(platformMap[file.platformType].LargeSizeIconLink);
            }
        }
        else if (role == PlatformRole)
        {
            auto file = m_files[index.row()];
            return file.platformType;
        }
        return QVariant{};
    }

    QVariant FileWatcherModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt:DisplayRole*/) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case 0:
                    return QString(tr("File Name"));
                case 1:
                    return QString(tr("Platform"));
                case 2:
                    return QString(tr("In Pak"));
                }
            }
        }
        return QVariant{};
    }

    Qt::ItemFlags FileWatcherModel::flags(const QModelIndex &index) const
    {
        auto returnFlags = QAbstractTableModel::flags(index);
        
        if (index.isValid())
        {
            returnFlags |= Qt::ItemIsDragEnabled;
            if (index.column() == (int)ColumnContents::InPak || index.column() == (int)ColumnContents::PlatformType)
            {
                return  returnFlags & ~Qt::ItemIsSelectable;
            }

        }

        return returnFlags;
    }

    Qt::DropActions FileWatcherModel::supportedDragActions() const
    {
        return Qt::CopyAction;
    }

    QStringList FileWatcherModel::mimeTypes() const
    {
        QStringList types;
        types << MIME_TYPE_TEXT;
        return types;
    }

    QMimeData* FileWatcherModel::mimeData(const QModelIndexList& indexes) const
    {
        auto mimeData = new QMimeData();
        QByteArray encodedData;
        QDataStream stream(&encodedData, QIODevice::WriteOnly);

        foreach(const QModelIndex& index, indexes)
        {
            if (index.isValid())
            {
                QString text = data(index, Qt::DisplayRole).toString();
                stream << text;
            }
        }

        mimeData->setData(MIME_TYPE_TEXT, encodedData);
        return mimeData;
    }

    void FileWatcherModel::Update(const ManifestFiles& fileList)
    {
        beginResetModel();

        m_files = fileList;

        endResetModel();

        m_fileWatcher = new QFileSystemWatcher{ this };
        for (auto file : fileList)
        {
            if (m_fileWatcher->addPath(FullFilePath(file)))
            {
                connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &FileWatcherModel::OnFileChanged);
            }
        }

        emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, columnCount() - 1));
    }

    QString FileWatcherModel::FullFilePath(const ManifestFile file)
    {
        AZStd::string fullPathToAssets = AZ::IO::FileIOBase::GetInstance()->GetAlias("@assets@");
        QString fullPath = fullPathToAssets.c_str();
        fullPath.replace("\\", "/");

        QStringList pathList = fullPath.split("/", QString::SkipEmptyParts);
        if (pathList.size() >= 2)
        {
            pathList[pathList.size() - 2] = file.platformType;
        }
        if (file.localFolder != ".")
        {
            pathList.append(file.localFolder);
        }        
        pathList.append(file.keyName);
        QString platformFullPath = pathList.join("/") ;
        return platformFullPath;
    }

    void FileWatcherModel::OnFileChanged(const QString fileName)
    {
        QString pakName;
        for (int row = 0; row < rowCount(); ++row)
        {
            auto file = m_files[row];

            QString currentRowFileName = data(createIndex(row, (int)ColumnContents::FileName), Qt::DisplayRole).toString();
            if (currentRowFileName.startsWith("./"))
            {
                currentRowFileName = currentRowFileName.right(currentRowFileName.length() - 2);
            }
            QString currentRowPlatform = data(createIndex(row, (int)ColumnContents::PlatformType), PlatformRole).toString();

            if (fileName.contains("/" + currentRowFileName) && fileName.contains("/" + currentRowPlatform + "/"))
            {
                QStringList pakDirList = file.pakFile.split('/');
                pakName = pakDirList[pakDirList.size() - 1];
                break;
            }
        }

        fileInPakChanged(pakName);
    }
}

