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

#include <AzToolsFramework/MaterialBrowser/MaterialThumbnail.h>

#include <QPixmap>

namespace AzToolsFramework
{
    namespace MaterialBrowser
    {
        static const char* SIMPLE_MATERIAL_ICON_PATH = ":/MaterialBrowser/images/material_04.png";
        static const char* MULTI_MATERIAL_ICON_PATH = ":/MaterialBrowser/images/material_06.png";

        //////////////////////////////////////////////////////////////////////////
        // MaterialThumbnail
        //////////////////////////////////////////////////////////////////////////
        MaterialThumbnail::MaterialThumbnail(Thumbnailer::SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {
            auto productKey = qobject_cast<const Thumbnailer::ProductThumbnailKey*>(m_key);
            AZ_Assert(productKey, "Incorrect key type, excpected ProductThumbnailKey");

            bool multiMat = false;
            MaterialBrowserRequestBus::BroadcastResult(multiMat, &MaterialBrowserRequests::IsMultiMaterial, productKey->GetAssetId());

            QString iconPath = multiMat ? MULTI_MATERIAL_ICON_PATH : SIMPLE_MATERIAL_ICON_PATH;
            m_pixmap = QPixmap(iconPath).scaled(m_thumbnailSize, m_thumbnailSize, Qt::KeepAspectRatio);
        }

        //////////////////////////////////////////////////////////////////////////
        // MaterialThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        MaterialThumbnailCache::MaterialThumbnailCache()
            : ThumbnailCache<MaterialThumbnail, MaterialKeyHash, MaterialKeyEqual>() {}

        MaterialThumbnailCache::~MaterialThumbnailCache() = default;

        bool MaterialThumbnailCache::IsSupportedThumbnail(Thumbnailer::SharedThumbnailKey key) const
        {
            return qobject_cast<const Thumbnailer::ProductThumbnailKey*>(key.data());
        }
    } // namespace MaterialBrowser
} // namespace AzToolsFramework

#include <MaterialBrowser/MaterialThumbnail.moc>
