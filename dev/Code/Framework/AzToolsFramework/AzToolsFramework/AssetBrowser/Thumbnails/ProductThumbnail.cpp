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

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <QPixmap>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnailKey::ProductThumbnailKey(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
            : ThumbnailKey()
            , m_assetId(assetId)
            , m_assetType(assetType)
        {}

        const AZ::Data::AssetId& ProductThumbnailKey::GetAssetId() const
        {
            return m_assetId;
        }

        const AZ::Data::AssetType& ProductThumbnailKey::GetAssetType() const
        {
            return m_assetType;
        }

        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnail
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnail::ProductThumbnail(SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {}

        void ProductThumbnail::LoadThread() 
        {
            auto productKey = qobject_cast<const ProductThumbnailKey*>(m_key);
            AZ_Assert(productKey, "Incorrect key type, excpected ProductThumbnailKey");

            QString iconPath;
            AZ::AssetTypeInfoBus::EventResult(iconPath, productKey->GetAssetType(), &AZ::AssetTypeInfo::GetBrowserIcon);
            AZ_Assert(!iconPath.isEmpty(), "Icon not found");
            m_pixmap = QPixmap(iconPath).scaled(m_thumbnailSize, m_thumbnailSize, Qt::KeepAspectRatio);
        }

        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnailCache::ProductThumbnailCache()
            : ThumbnailCache<ProductThumbnail, ProductKeyHash, ProductKeyEqual>() {}

        ProductThumbnailCache::~ProductThumbnailCache() = default;

        bool ProductThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            return qobject_cast<const ProductThumbnailKey*>(key.data());
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include <AssetBrowser/Thumbnails/ProductThumbnail.moc>
