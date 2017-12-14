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
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <QtConcurrent/QtConcurrent>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        SourceControlThumbnailKey::SourceControlThumbnailKey(const char* fileName)
            : ThumbnailKey()
            , m_fileName(fileName)
        {
        }

        const AZStd::string& SourceControlThumbnailKey::GetFileName() const
        {
            return m_fileName;
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnail
        //////////////////////////////////////////////////////////////////////////
        const float SourceControlThumbnail::UPDATE_INTERVAL_S = 30.0f;
        const char* SourceControlThumbnail::CHECKEDOUT_ICON_PATH = "Editor/Icons/AssetBrowser/checkedOut.png";
        const char* SourceControlThumbnail::READONLY_ICON_PATH = "Editor/Icons/AssetBrowser/notCheckedOut.png";

        SourceControlThumbnail::SourceControlThumbnail(SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
            , m_timeToUpdate(UPDATE_INTERVAL_S)
        {
            BusConnect();
        }

        SourceControlThumbnail::~SourceControlThumbnail()
        {
            BusDisconnect();
        }

        void SourceControlThumbnail::UpdateTime(float deltaTime)
        {
            // update source control status every UPDATE_INTERVAL_S secounds
            if (m_readyToUpdate)
            {
                if (m_timeToUpdate > 0)
                {
                    m_timeToUpdate -= deltaTime;
                }
                else
                {
                    m_readyToUpdate = false;
                    m_timeToUpdate = UPDATE_INTERVAL_S;
                    QFuture<void> future = QtConcurrent::run([this]() { LoadThread(); });
                    m_watcher.setFuture(future);
                }
            }
        }

        void SourceControlThumbnail::FileStatusChanged(const char* filename)
        {
            // when file status is changed, force instant update
            auto sourceControlKey = qobject_cast<const SourceControlThumbnailKey*>(m_key);
            AZ_Assert(sourceControlKey, "Incorrect key type, excpected SourceControlThumbnailKey");

            AZStd::string myFileName(sourceControlKey->GetFileName());
            AzFramework::StringFunc::Path::Normalize(myFileName);
            if (AzFramework::StringFunc::Equal(myFileName.c_str(), filename))
            {
                m_timeToUpdate = 0;
            }
        }

        void SourceControlThumbnail::LoadThread()
        {
            auto sourceControlKey = qobject_cast<const SourceControlThumbnailKey*>(m_key);
            AZ_Assert(sourceControlKey, "Incorrect key type, excpected SourceControlThumbnailKey");

            bool isSourceControlActive = false;
            SourceControlConnectionRequestBus::BroadcastResult(isSourceControlActive, &SourceControlConnectionRequests::IsActive);
            if (isSourceControlActive && SourceControlCommandBus::FindFirstHandler() != nullptr)
            {
                SourceControlCommandBus::Broadcast(&SourceControlCommands::GetFileInfo, sourceControlKey->GetFileName().c_str(),
                    AZStd::bind(&SourceControlThumbnail::SourceControlFileInfoUpdated, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
                m_sourceControlWait.acquire();
            }
            else
            {
                m_pixmap = QPixmap();
            }
        }

        void SourceControlThumbnail::SourceControlFileInfoUpdated(bool succeeded, const SourceControlFileInfo& fileInfo)
        {
            if (succeeded)
            {
                if (fileInfo.HasFlag(AzToolsFramework::SCF_OpenByUser))
                {
                    m_pixmap = QPixmap(CHECKEDOUT_ICON_PATH);
                }
                else
                {
                    m_pixmap = QPixmap(READONLY_ICON_PATH);
                }
            }
            else
            {
                m_pixmap = QPixmap();
            }
            m_sourceControlWait.release();
            m_readyToUpdate = true;
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        SourceControlThumbnailCache::SourceControlThumbnailCache()
            : ThumbnailCache<SourceControlThumbnail, SourceControlKeyHash, SourceControlKeyEqual>()
        {
        }

        SourceControlThumbnailCache::~SourceControlThumbnailCache() = default;

        bool SourceControlThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            auto productThumbnailKey = qobject_cast<const SourceControlThumbnailKey*>(key.data());
            if (!productThumbnailKey)
            {
                return false;
            }
            return true;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework
#include <Thumbnails/SourceControlThumbnail.moc>

