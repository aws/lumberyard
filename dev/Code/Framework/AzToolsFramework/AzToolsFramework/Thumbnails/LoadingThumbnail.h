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

#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <QMovie>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class LoadingThumbnail
            : public Thumbnail
        {
            Q_OBJECT
        public:
            explicit LoadingThumbnail(int thumbnailSize);
            void UpdateTime(float deltaTime) override;

        private:
            float m_angle;
            QMovie m_loadingMovie;
        };
    } // namespace Thumbnailer
} // namespace AzToolsFramework
