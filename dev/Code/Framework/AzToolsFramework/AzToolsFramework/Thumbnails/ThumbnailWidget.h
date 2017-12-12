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

#include <AzToolsFramework/Thumbnails/Thumbnail.h>

#include <QWidget>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //! A widget used to display thumbnail. To display thumbnails within item views, use ThumbnailDelegate
        class ThumbnailWidget
            : public QWidget
        {
            Q_OBJECT
        public:
            explicit ThumbnailWidget(QWidget* parent = nullptr);
            ~ThumbnailWidget() override = default;

            //! Call this to set what thumbnail widget will display
            void SetThumbnailKey(SharedThumbnailKey key, const char* contextName = "Default");
            //! Remove current thumbnail
            void ClearThumbnail();

        protected:
            void paintEvent(QPaintEvent* event) override;

        private:
            SharedThumbnailKey m_key;
            AZStd::string m_contextName;

        private Q_SLOTS:
            void KeyUpdatedSlot();
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework