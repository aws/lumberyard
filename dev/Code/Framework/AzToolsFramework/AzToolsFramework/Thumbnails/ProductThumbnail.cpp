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

#include <AzCore/Asset/AssetManagerBus.h>
#include <AssetBrowser/EBusFindAssetTypeByName.h>
#include <Thumbnails/ProductThumbnail.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnailKey::ProductThumbnailKey(const AZ::Data::AssetId& assetId)
            : ThumbnailKey()
            , m_assetId(assetId)
        {
            AZ::Data::AssetInfo info;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_assetId);
            m_assetType = info.m_assetType;
        }

        const AZ::Data::AssetId& ProductThumbnailKey::GetAssetId() const { return m_assetId; }

        const AZ::Data::AssetType& ProductThumbnailKey::GetAssetType() const { return m_assetType; }

        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnail::ProductThumbnail(SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {
            auto assetIdThumbnailKey = qobject_cast<const ProductThumbnailKey*>(m_key);
            AZ_Assert(assetIdThumbnailKey, "Incorrect key type, excpected ProductThumbnailKey");
            m_assetId = assetIdThumbnailKey->GetAssetId();
            m_assetType = assetIdThumbnailKey->GetAssetType();
            BusConnect(m_assetId);
        }

        ProductThumbnail::~ProductThumbnail()
        {
            BusDisconnect();
        }

        void ProductThumbnail::ThumbnailRendered(QPixmap& thumbnailImage)
        {
            m_pixmap = thumbnailImage;
            m_renderWait.release();
        }

        void ProductThumbnail::ThumbnailFailedToRender()
        {
            m_state = State::Failed;
            m_renderWait.release();
        }

        void ProductThumbnail::LoadThread()
        {
            bool installed = false;
            ThumbnailerRendererRequestBus::EventResult(installed, m_assetType, &ThumbnailerRendererRequests::Installed);
            if (installed)
            {
                ThumbnailerRendererRequestBus::QueueEvent(m_assetType, &ThumbnailerRendererRequests::RenderThumbnail, m_assetId, m_thumbnailSize);
                // wait for response from thumbnail renderer
                m_renderWait.acquire();
            }
            else
            {
                m_state = State::Failed;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnailCache::ProductThumbnailCache()
            : ThumbnailCache<ProductThumbnail, ProductKeyHash, ProductKeyEqual>()
        {
        }

        ProductThumbnailCache::~ProductThumbnailCache() = default;

        bool ProductThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            // thumbnail key is managed by this provider if its both a ProductThumbnailKey type and its asset type is texture asset
            auto productThumbnailKey = qobject_cast<const ProductThumbnailKey*>(key.data());
            if (!productThumbnailKey)
            {
                return false;
            }
            return true;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework
#include <Thumbnails/ProductThumbnail.moc>

