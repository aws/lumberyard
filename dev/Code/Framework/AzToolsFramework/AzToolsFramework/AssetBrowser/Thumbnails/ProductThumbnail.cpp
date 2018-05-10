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
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <QPixmap>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnail
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnail::ProductThumbnail(Thumbnailer::SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {}

        void ProductThumbnail::LoadThread() 
        {
            auto productKey = azrtti_cast<const Thumbnailer::ProductThumbnailKey*>(m_key.data());
            AZ_Assert(productKey, "Incorrect key type, excpected ProductThumbnailKey");

            QString iconPath;
            AZ::AssetTypeInfoBus::EventResult(iconPath, productKey->GetAssetType(), &AZ::AssetTypeInfo::GetBrowserIcon);
            if (!iconPath.isEmpty())
            {
                // Use absolute path for the icon if possible
                AZStd::string iconFullPath;
                bool pathFound = false;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(pathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, iconPath.toUtf8().constBegin(), iconFullPath);
                if (!pathFound)
                {
                    // Special case:  If the path cannot be resolved, and starts with a relative "Editor/" path and app root is not the engine folder, then use the engine root to
                    // build an absolute path to it
                    if (iconPath.startsWith("Editor/"))
                    {
                        bool isEngineExternal = false;
                        AzFramework::ApplicationRequests::Bus::BroadcastResult(isEngineExternal, &AzFramework::ApplicationRequests::IsEngineExternal);
                        if (isEngineExternal)
                        {
                            // For icon paths in external projects that are looking for Editor/, convert the relative icon path to the engine absolute one
                            AZStd::string engineRootPath;
                            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRootPath, &AzFramework::ApplicationRequests::GetEngineRoot);
                            iconPath = QString(engineRootPath.c_str()) + iconPath;
                        }
                    }
                    else
                    {
                        m_state = State::Failed;
                    }
                }
                else
                {
                    iconPath = QString(iconFullPath.c_str());
                }
                m_pixmap = QPixmap(iconPath).scaled(m_thumbnailSize, m_thumbnailSize, Qt::KeepAspectRatio);
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
            : ThumbnailCache<ProductThumbnail, ProductKeyHash, ProductKeyEqual>() {}

        ProductThumbnailCache::~ProductThumbnailCache() = default;

        bool ProductThumbnailCache::IsSupportedThumbnail(Thumbnailer::SharedThumbnailKey key) const
        {
            return azrtti_istypeof<const Thumbnailer::ProductThumbnailKey*>(key.data());
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include <AssetBrowser/Thumbnails/ProductThumbnail.moc>
