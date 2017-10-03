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

#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>

namespace AzToolsFramework
{
    using namespace Thumbnailer;

    namespace AssetBrowser
    {
        //! ProductAssetBrowserEntry thumbnail key
        class ProductThumbnailKey
            : public ThumbnailKey
        {
            Q_OBJECT
        public:
            ProductThumbnailKey(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType);
            const AZ::Data::AssetId& GetAssetId() const;
            const AZ::Data::AssetType& GetAssetType() const;

        protected:
            AZ::Data::AssetId m_assetId;
            AZ::Data::AssetType m_assetType;
        };

        class ProductThumbnail
            : public Thumbnail
        {
            Q_OBJECT
        public:
            ProductThumbnail(SharedThumbnailKey key, int thumbnailSize);

        protected:
            void LoadThread() override;
        };

        namespace
        {
            class ProductKeyHash
            {
            public:
                size_t operator() (const SharedThumbnailKey& val) const
                {
                    auto productThumbnailKey = qobject_cast<const ProductThumbnailKey*>(val.data());
                    if (!productThumbnailKey)
                    {
                        return 0;
                    }
                    return productThumbnailKey->GetAssetType().GetHash();
                }
            };

            class ProductKeyEqual
            {
            public:
                bool operator()(const SharedThumbnailKey& val1, const SharedThumbnailKey& val2) const
                {
                    auto productThumbnailKey1 = qobject_cast<const ProductThumbnailKey*>(val1.data());
                    auto productThumbnailKey2 = qobject_cast<const ProductThumbnailKey*>(val2.data());
                    if (!productThumbnailKey1 || !productThumbnailKey2)
                    {
                        return false;
                    }
                    // products displayed in Asset Browser have icons based on asset type, so multiple different products with same asset type will have same thumbnail
                    return productThumbnailKey1->GetAssetType() == productThumbnailKey2->GetAssetType();
                }
            };
        }

        //! ProductAssetBrowserEntry thumbnails
        class ProductThumbnailCache
            : public ThumbnailCache<ProductThumbnail, ProductKeyHash, ProductKeyEqual>
        {
        public:
            ProductThumbnailCache();
            ~ProductThumbnailCache() override;

        protected:
            bool IsSupportedThumbnail(SharedThumbnailKey key) const override;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
