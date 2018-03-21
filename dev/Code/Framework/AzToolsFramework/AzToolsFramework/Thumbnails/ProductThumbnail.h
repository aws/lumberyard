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

#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>


namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class ProductThumbnailKey
            : public ThumbnailKey
        {
            Q_OBJECT
        public:
            explicit ProductThumbnailKey(const AZ::Data::AssetId& assetId);
            const AZ::Data::AssetId& GetAssetId() const;
            const AZ::Data::AssetType& GetAssetType() const;

        protected:
            AZ::Data::AssetId m_assetId;
            AZ::Data::AssetType m_assetType;
        };

        //! A thumbnail type for processed assets
        class ProductThumbnail
            : public Thumbnail
            , public ThumbnailerRendererNotificationBus::Handler
        {
            Q_OBJECT
        public:
            ProductThumbnail(SharedThumbnailKey key, int thumbnailSize);
            ~ProductThumbnail() override;

            //////////////////////////////////////////////////////////////////////////
            // ThumbnailerRendererNotifications
            //////////////////////////////////////////////////////////////////////////
            void ThumbnailRendered(QPixmap& thumbnailImage) override;
            void ThumbnailFailedToRender();

        protected:
            AZ::Data::AssetId m_assetId;
            AZ::Data::AssetType m_assetType;

            void LoadThread() override;

        private:
            AZStd::binary_semaphore m_renderWait;
        };

        namespace
        {
            class ProductKeyHash
            {
            public:
                size_t operator() (const SharedThumbnailKey& val) const
                {
                    auto key = qobject_cast<const ProductThumbnailKey*>(val.data());
                    if (!key)
                    {
                        return 0;
                    }
                    return key->GetAssetId().m_guid.GetHash();
                }
            };

            class ProductKeyEqual
            {
            public:
                bool operator()(const SharedThumbnailKey& val1, const SharedThumbnailKey& val2) const
                {
                    auto key1 = qobject_cast<const ProductThumbnailKey*>(val1.data());
                    auto key2 = qobject_cast<const ProductThumbnailKey*>(val2.data());
                    if (!key1 || !key2)
                    {
                        return false;
                    }
                    // in this case product keys are unique if they have different assetIds
                    return key1->GetAssetId() == key2->GetAssetId();
                }
            };
        }

        //! Stores products' thumbnails
        class ProductThumbnailCache
            : public ThumbnailCache<ProductThumbnail, ProductKeyHash, ProductKeyEqual>
        {
        public:
            ProductThumbnailCache();
            ~ProductThumbnailCache() override;

        protected:
            bool IsSupportedThumbnail(SharedThumbnailKey key) const override;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework