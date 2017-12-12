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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QObject>
#include <QList>

class QString;
class QPixmap;

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class ThumbnailProvider;

        //! ThumbnailContext provides distinct thumbnail location for specific context
        /*
            There can be any number of contexts for every unique feature that may need different types of thumbnails.
            For example 'AssetBrowser' context provides thumbnails specific to Asset Browser
            'PreviewContext' may provide thumbnails for Preview Widget
            'MaterialBrowser' may provide thumbnails for Material Browser
            etc.
        */
        class ThumbnailContext
            : public QObject
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(ThumbnailContext, AZ::SystemAllocator, 0)

            explicit ThumbnailContext(int thumbnailSize);
            ~ThumbnailContext() override;

            //! Retrieve thumbnail by key, generate one if needed
            SharedThumbnail GetThumbnail(SharedThumbnailKey key);
            //! Add new thumbnail cache
            void RegisterThumbnailProvider(SharedThumbnailProvider provider);

        private:
            //! Collection of thumbnail caches provided by this context
            QList<SharedThumbnailProvider> m_providers;
            //! Default missing thumbnail used when no thumbnail for given key can be found within this context
            SharedThumbnail m_missingThumbnail;
            //! Default loading thumbnail used when thumbnail is found by is not yet generated
            SharedThumbnail m_loadingThumbnail;
            //! Thumbnail size (width and height in pixels)
            int m_thumbnailSize;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
