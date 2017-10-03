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

#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/FolderThumbnail.h>
#include <QPixmap>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        // FolderThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        FolderThumbnailKey::FolderThumbnailKey(const char* folderPath, bool isGem)
            : ThumbnailKey()
            , m_folderPath(folderPath)
            , m_isGem(isGem)
        {}

        const AZStd::string& FolderThumbnailKey::GetFolderPath() const
        {
            return m_folderPath;
        }

        bool FolderThumbnailKey::IsGem() const
        {
            return m_isGem;
        }

        //////////////////////////////////////////////////////////////////////////
        // FolderThumbnail
        //////////////////////////////////////////////////////////////////////////
        static const char* FOLDER_ICON_PATH = "Editor/Icons/AssetBrowser/folder.png";
        static const char* GEM_ICON_PATH = "Editor/Icons/AssetBrowser/gem.png";

        FolderThumbnail::FolderThumbnail(SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {}

        void FolderThumbnail::LoadThread()
        {
            auto folderKey = qobject_cast<const FolderThumbnailKey*>(m_key);
            AZ_Assert(folderKey, "Incorrect key type, excpected FolderThumbnailKey");
            m_pixmap = folderKey->IsGem() ? QPixmap(GEM_ICON_PATH) : QPixmap(FOLDER_ICON_PATH);
        }

        //////////////////////////////////////////////////////////////////////////
        // FolderThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        FolderThumbnailCache::FolderThumbnailCache()
            : ThumbnailCache<FolderThumbnail, FolderKeyHash, FolderKeyEqual>() {}

        FolderThumbnailCache::~FolderThumbnailCache() = default;

        bool FolderThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            return qobject_cast<const FolderThumbnailKey*>(key.data());
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include <AssetBrowser/Thumbnails/FolderThumbnail.moc>
