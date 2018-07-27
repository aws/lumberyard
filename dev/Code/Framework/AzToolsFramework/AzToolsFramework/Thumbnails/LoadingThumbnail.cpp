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

#include <AzToolsFramework/Thumbnails/LoadingThumbnail.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // LoadingThumbnail
        //////////////////////////////////////////////////////////////////////////
        static const char* LOADING_ICON_PATH = "Editor/Icons/AssetBrowser/in_progress.gif";

        LoadingThumbnail::LoadingThumbnail(int thumbnailSize)
            : Thumbnail(MAKE_TKEY(ThumbnailKey), thumbnailSize)
            , m_angle(0)
        {
            m_loadingMovie.setFileName(LOADING_ICON_PATH);
            m_loadingMovie.setCacheMode(QMovie::CacheMode::CacheAll);
            m_loadingMovie.setScaledSize(QSize(m_thumbnailSize, m_thumbnailSize));
            m_loadingMovie.start();
            m_pixmap = m_loadingMovie.currentPixmap();
            m_state = State::Ready;

            BusConnect();
        }

        LoadingThumbnail::~LoadingThumbnail()
        {
            BusDisconnect();
        }

        void LoadingThumbnail::UpdateTime(float)
        {
            m_pixmap = m_loadingMovie.currentPixmap();
            Q_EMIT Updated();
        }

        void LoadingThumbnail::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
        {
            UpdateTime(deltaTime);
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include <Thumbnails/LoadingThumbnail.moc>