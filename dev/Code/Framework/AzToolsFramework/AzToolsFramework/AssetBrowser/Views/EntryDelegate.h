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

#include <AzCore/std/function/function_fwd.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <QStyledItemDelegate>
#include <QPointer>

class QWidget;
class QPainter;
class QStyleOptionViewItem;
class QModelIndex;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;

        //! EntryDelegate draws a single item in AssetBrowser
        class EntryDelegate
            : public QStyledItemDelegate
        {
            Q_OBJECT
        public:
            explicit EntryDelegate(QWidget* parent = nullptr);
            ~EntryDelegate() override;

            QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            //! Set location where thumbnails are located for this instance of asset browser
            void SetThumbnailContext(const char* thumbnailContext);
            //! Set whether to show source control icons, this is still temporary mainly to support existing functionality of material browser
            void SetShowSourceControlIcons(bool showSourceControl);

        protected:
            //! Distance between UI elements 
            static const int SPACING_PIXELS;
            //! Left and right margins
            static const int MARGIN_PIXELS;

            int m_iconSize;
            AZStd::string m_thumbnailContext;
            bool m_showSourceControl = false;
            //! Draw a thumbnail and return its width
            int DrawThumbnail(QPainter* painter, const QPoint& point, const QSize& size, Thumbnailer::SharedThumbnailKey thumbnailKey) const;
            //! Draw text (culled by maxWidth) and return its width
            int DrawText(QPainter* painter, const QPoint& point, int maxWidth, const char* text) const;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
