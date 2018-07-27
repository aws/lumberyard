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

#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <QtConcurrent/QtConcurrent>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // ThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        bool ThumbnailKey::IsReady() const { return m_ready; }

        bool ThumbnailKey::UpdateThumbnail()
        {
            if (!IsReady())
            {
                return false;
            }
            emit UpdateThumbnailSignal();
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        // Thumbnail
        //////////////////////////////////////////////////////////////////////////
        Thumbnail::Thumbnail(SharedThumbnailKey key, int thumbnailSize)
            : QObject()
            , m_state(State::Unloaded)
            , m_thumbnailSize(thumbnailSize)
            , m_key(key)
        {
            connect(&m_watcher, &QFutureWatcher<void>::finished, this, [this]()
                {
                    if (m_state == State::Loading)
                    {
                        m_state = State::Ready;
                    }
                    Q_EMIT Updated();
                });
        }

        Thumbnail::~Thumbnail() = default;

        bool Thumbnail::operator==(const Thumbnail& other) const
        {
            return m_key == other.m_key;
        }

        void Thumbnail::Load()
        {
            if (m_state == State::Unloaded)
            {
                m_state = State::Loading;
                QFuture<void> future = QtConcurrent::run([this](){ LoadThread(); });
                m_watcher.setFuture(future);
            }
        }

        QPixmap Thumbnail::GetPixmap() const
        {
            return m_pixmap;
        }

        SharedThumbnailKey Thumbnail::GetKey() const
        {
            return m_key;
        }

        Thumbnail::State Thumbnail::GetState() const
        {
            return m_state;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include <Thumbnails/Thumbnail.moc>
