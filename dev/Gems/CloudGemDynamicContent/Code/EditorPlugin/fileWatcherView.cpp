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

#include <QDrag>
#include <QPainter>

#include "fileWatcherView.h"
#include "ManifestTableKeys.h"
#include "FileWatcherModel.h"
#include <fileWatcherView.moc>

namespace DynamicContent
{
    FileWatcherView::FileWatcherView(QWidget * parent):
        QTableView(parent)
    {
    }

    FileWatcherView::~FileWatcherView()
    {
    }

    void FileWatcherView::startDrag(Qt::DropActions supportedActions)
    {
        auto indexes = selectionModel()->selectedIndexes();

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData(indexes));

        auto pixMap = QPixmap("Editor/Icons/CloudCanvas/add_icon.png");

        if (m_fileCount > 1)
        {
            QPainter painter(&pixMap);
            painter.setPen(Qt::white);
            painter.drawText(QPoint(11, 22), QString::number(m_fileCount));
        }

        drag->setPixmap(pixMap);

        drag->exec(supportedActions);
    }

    QMimeData* FileWatcherView::mimeData(const QModelIndexList& indexes)
    {
        auto mimeData = new QMimeData();
        QByteArray encodedData;
        QDataStream stream(&encodedData, QIODevice::WriteOnly);

        m_fileCount = 0;
        foreach(const QModelIndex& index, indexes)
        {
            if (index.isValid())
            {
                QString name = model()->data(index, Qt::DisplayRole).toString();
                QString platform = model()->data(index, FileWatcherModel::PlatformRole).toString();
                QString text = FileWatcherModel::fileInfoConstructedByNameAndPlatform(name, platform);
                stream << text;
                m_fileCount++;
            }
        }

        mimeData->setData(MIME_TYPE_TEXT, encodedData);
        return mimeData;
    }
}