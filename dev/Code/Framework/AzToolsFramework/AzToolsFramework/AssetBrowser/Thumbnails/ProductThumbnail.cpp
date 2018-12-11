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
#include <AzFramework/StringFunc/StringFunc.h>
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
        static const char* DEFAULT_PRODUCT_ICON_PATH = "Editor/Icons/AssetBrowser/DefaultProduct_16.png";

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
                // is it an embedded resource or absolute path?
                bool isUsablePath = (iconPath.startsWith(":") || (!AzFramework::StringFunc::Path::IsRelative(iconPath.toUtf8().constData())));
                
                if (!isUsablePath)
                {
                    // getting here means it needs resolution.  Can we find the real path of the file?  This also searches in gems for sources.
                    bool foundIt = false;
                    AZStd::string watchFolder;
                    AZ::Data::AssetInfo assetInfo;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, iconPath.toUtf8().constData(), assetInfo, watchFolder);

                    if (foundIt)
                    {
                        // the absolute path is join(watchfolder, relativepath); // since its relative to the watch folder.
                        AZStd::string finalPath;
                        AzFramework::StringFunc::Path::Join(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), finalPath);
                        iconPath = QString::fromUtf8(finalPath.c_str());
                    }
                }
            }
            else
            {
                // no pixmap specified - use default.
                iconPath = QString::fromUtf8(DEFAULT_PRODUCT_ICON_PATH);
            }
            m_pixmap = QPixmap(iconPath); // make sure your icons are the correct size.
            
            if (m_pixmap.isNull())
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
