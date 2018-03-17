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

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>

#include <QApplication>
#include <QPainter>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const int EntryDelegate::SPACING_PIXELS = 5;
        const int EntryDelegate::MARGIN_PIXELS = 2;

        EntryDelegate::EntryDelegate(QWidget* parent)
            : QStyledItemDelegate(parent)
            , m_iconSize(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize))
        {
        }

        EntryDelegate::~EntryDelegate() = default;

        QSize EntryDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            QSize baseHint = QStyledItemDelegate::sizeHint(option, index);
            if (baseHint.height() < m_iconSize)
            {
                baseHint.setHeight(m_iconSize);
            }
            return baseHint;
        }

        void EntryDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            auto data = index.data(AssetBrowserModel::Roles::EntryRole);
            if (data.canConvert<const AssetBrowserEntry*>())
            {
                if (option.state & QStyle::State_Selected)
                {
                    painter->fillRect(option.rect, option.palette.highlight());
                }

                auto entry = qvariant_cast<const AssetBrowserEntry*>(data);

                int x = option.rect.x();
                int y = option.rect.y();

                // Draw source control icon
                if (m_showSourceControl)
                {
                    auto sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
                    if (sourceEntry)
                    {
                        DrawThumbnail(painter, QPoint(x - m_iconSize / 2, y + m_iconSize / 2), QSize(m_iconSize, m_iconSize) / 2, sourceEntry->GetSourceControlThumbnailKey());
                    }
                }
                
                // Draw main entry thumbnail
                x += DrawThumbnail(painter, QPoint(x, y), QSize(m_iconSize, m_iconSize), entry->GetThumbnailKey());
                x += SPACING_PIXELS;

                // Draw entry name
                DrawText(painter, QPoint(x, y), option.rect.x() + option.rect.width() - m_iconSize - SPACING_PIXELS - MARGIN_PIXELS, entry->GetDisplayName().c_str());
            }
        }

        void EntryDelegate::SetThumbnailContext(const char* thumbnailContext)
        {
            m_thumbnailContext = thumbnailContext;
        }

        void EntryDelegate::SetShowSourceControlIcons(bool showSourceControl)
        {
            m_showSourceControl = showSourceControl;
        }

        int EntryDelegate::DrawThumbnail(QPainter* painter, const QPoint& point, const QSize& size, Thumbnailer::SharedThumbnailKey thumbnailKey) const
        {
            SharedThumbnail thumbnail;
            ThumbnailerRequestsBus::BroadcastResult(thumbnail, &ThumbnailerRequests::GetThumbnail, thumbnailKey, m_thumbnailContext.c_str());
            AZ_Assert(thumbnail, "Could not get thumbnail");
            if (thumbnail->GetState() == Thumbnail::State::Failed)
            {
                return 0;
            }
            QPixmap pixmap = thumbnail->GetPixmap();
            painter->drawPixmap(point.x(), point.y(), size.width(), size.height(), pixmap);
            return m_iconSize;
        }

        int EntryDelegate::DrawText(QPainter* painter, const QPoint& point, int maxX, const char* text) const
        {
            QFontMetrics fm = painter->fontMetrics();
            int width = qMin(fm.width(text), maxX - point.x());
            QRect textRect(point.x(), point.y(), width, m_iconSize);
            QTextOption textOption;
            textOption.setWrapMode(QTextOption::NoWrap);
            painter->drawText(textRect, text, textOption);
            return width;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include <AssetBrowser/Views/EntryDelegate.moc>
