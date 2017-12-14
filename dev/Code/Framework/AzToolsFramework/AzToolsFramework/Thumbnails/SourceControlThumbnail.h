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
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnailBus.h>

namespace AzToolsFramework
{
    struct SourceControlFileInfo;

    namespace Thumbnailer
    {
        class SourceControlThumbnailKey
            : public ThumbnailKey
        {
            Q_OBJECT
        public:
            explicit SourceControlThumbnailKey(const char* fileName);
            const AZStd::string& GetFileName() const;
        protected:
            //! absolute path
            AZStd::string m_fileName;
        };

        //! SourceControlThumbnail currently replicates the source control functionality within Material Browser
        //! This thumbnail refreshes source control status for each individual file every UPDATE_INTERVAL_S
        //! Additionally source control status is refreshed whenever an operation is performed through context menu
        class SourceControlThumbnail
            : public Thumbnail
            , public SourceControlThumbnailRequestBus::Handler
        {
            Q_OBJECT
        public:
            SourceControlThumbnail(SharedThumbnailKey key, int thumbnailSize);
            ~SourceControlThumbnail() override;

            void UpdateTime(float deltaTime) override;

            //////////////////////////////////////////////////////////////////////////
            // SourceControlNotificationBus
            //////////////////////////////////////////////////////////////////////////
            void FileStatusChanged(const char* filename) override;

        protected:
            void LoadThread() override;

        private:
            //! How often to check status
            static const float UPDATE_INTERVAL_S;
            static const char* CHECKEDOUT_ICON_PATH;
            static const char* READONLY_ICON_PATH;

            AZStd::binary_semaphore m_sourceControlWait;            
            //! How long before next update
            float m_timeToUpdate;
            bool m_readyToUpdate = false;

            void SourceControlFileInfoUpdated(bool succeeded, const SourceControlFileInfo& fileInfo);
        };

        namespace
        {
            class SourceControlKeyHash
            {
            public:
                size_t operator() (const SharedThumbnailKey& /*val*/) const
                {
                    return 0;
                }
            };

            class SourceControlKeyEqual
            {
            public:
                bool operator()(const SharedThumbnailKey& val1, const SharedThumbnailKey& val2) const
                {
                    auto sourceThumbnailKey1 = qobject_cast<const SourceControlThumbnailKey*>(val1.data());
                    auto sourceThumbnailKey2 = qobject_cast<const SourceControlThumbnailKey*>(val2.data());
                    if (!sourceThumbnailKey1 || !sourceThumbnailKey2)
                    {
                        return false;
                    }
                    return sourceThumbnailKey1->GetFileName() == sourceThumbnailKey2->GetFileName();
                }
            };
        }

        //! Stores products' thumbnails
        class SourceControlThumbnailCache
            : public ThumbnailCache<SourceControlThumbnail, SourceControlKeyHash, SourceControlKeyEqual>
        {
        public:
            SourceControlThumbnailCache();
            ~SourceControlThumbnailCache() override;

        protected:
            bool IsSupportedThumbnail(SharedThumbnailKey key) const override;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework