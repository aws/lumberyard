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

#include <AzCore/std/string/string.h>

#include <AzToolsFramework/Thumbnails/ThumbnailDelegate.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QVariant>
#include <QPainter>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        ThumbnailDelegate::ThumbnailDelegate(QWidget* parent)
            : QStyledItemDelegate(parent)
        {}

        ThumbnailDelegate::~ThumbnailDelegate() = default;

        void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            auto data = index.data(Qt::DecorationRole);
            if (data.canConvert<SharedThumbnailKey>())
            {
                auto key = qvariant_cast<SharedThumbnailKey>(data);
                SharedThumbnail thumbnail;
                ThumbnailerRequestsBus::BroadcastResult(thumbnail, &ThumbnailerRequests::GetThumbnail, key, m_thumbnailContext.c_str());

                AZ_Assert(thumbnail, "Could not get thumbnail");

                if (option.state & QStyle::State_Selected)
                {
                    painter->fillRect(option.rect, option.palette.highlight());
                }

                QPixmap pixmap = thumbnail->GetPixmap();
                painter->drawPixmap(option.rect.x(), option.rect.y(), pixmap);
            }

            QStyledItemDelegate::paint(painter, option, index);
        }

        QWidget* ThumbnailDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            return QStyledItemDelegate::createEditor(parent, option, index);
        }

        void ThumbnailDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
        {
            QStyledItemDelegate::setEditorData(editor, index);
        }

        void ThumbnailDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
        {
            QStyledItemDelegate::setModelData(editor, model, index);
        }

        void ThumbnailDelegate::SetThumbnailContext(const char* thumbnailContext)
        {
            m_thumbnailContext = thumbnailContext;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include <Thumbnails/ThumbnailDelegate.moc>
