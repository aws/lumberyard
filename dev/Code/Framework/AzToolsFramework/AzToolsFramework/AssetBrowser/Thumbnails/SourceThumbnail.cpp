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

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <QString>
#include <QFileIconProvider>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        SourceThumbnailKey::SourceThumbnailKey(const char* fileName)
            : ThumbnailKey()
            , m_fileName(fileName)
        {
            AzFramework::StringFunc::Path::Split(fileName, nullptr, nullptr, nullptr, &m_extension);
        }

        const AZStd::string& SourceThumbnailKey::GetFileName() const
        {
            return m_fileName;
        }

        const AZStd::string& SourceThumbnailKey::GetExtension() const
        {
            return m_extension;
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnail
        //////////////////////////////////////////////////////////////////////////
        QMutex SourceThumbnail::m_mutex;

        SourceThumbnail::SourceThumbnail(SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {
        }

        void SourceThumbnail::LoadThread()
        {
            auto sourceKey = azrtti_cast<const SourceThumbnailKey*>(m_key.data());
            AZ_Assert(sourceKey, "Incorrect key type, excpected SourceThumbnailKey");

            QString finalPath = QString::fromUtf8(sourceKey->GetFileName().c_str());
            QFileInfo fileInfo(finalPath);
            QFileIconProvider iconProvider;
            QIcon fileIcon;
            if (!fileInfo.exists())
            {
                fileIcon = iconProvider.icon(fileInfo);
            }
            else
            {
                fileIcon = iconProvider.icon(QFileIconProvider::IconType::File);
            }
            // fileIcon is not thread safe. Accessing it simultaneously from multiple threads produces incorrect results.
            m_mutex.lock();
            m_pixmap = fileIcon.pixmap(16).copy(0, 0, m_thumbnailSize, m_thumbnailSize);
            m_mutex.unlock();
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        SourceThumbnailCache::SourceThumbnailCache()
            : ThumbnailCache<SourceThumbnail, SourceKeyHash, SourceKeyEqual>() {}

        SourceThumbnailCache::~SourceThumbnailCache() = default;

        bool SourceThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            return azrtti_istypeof<const SourceThumbnailKey*>(key.data());
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include <AssetBrowser/Thumbnails/SourceThumbnail.moc>
