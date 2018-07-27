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

#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <AzToolsFramework/Thumbnails/MissingThumbnail.h>
#include <AzToolsFramework/Thumbnails/LoadingThumbnail.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        ThumbnailContext::ThumbnailContext(int thumbnailSize)
            : m_missingThumbnail(new MissingThumbnail(thumbnailSize))
            , m_loadingThumbnail(new LoadingThumbnail(thumbnailSize))
            , m_thumbnailSize(thumbnailSize)
        {
        }

        ThumbnailContext::~ThumbnailContext() = default;

        SharedThumbnail ThumbnailContext::GetThumbnail(SharedThumbnailKey key)
        {
            SharedThumbnail thumbnail;
            // find provider who can handle supplied key
            for (auto& provider : m_providers)
            {
                if (provider->GetThumbnail(key, thumbnail))
                {
                    // if thumbnail is ready return it
                    if (thumbnail->GetState() == Thumbnail::State::Ready)
                    {
                        return thumbnail;
                    }
                    // if thumbnail is not loaded, start loading it, meanwhile return loading thumbnail
                    if (thumbnail->GetState() == Thumbnail::State::Unloaded)
                    {
                        // listen to the loading signal, so the anyone using it will update loading animation
                        connect(m_loadingThumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                        // once the thumbnail is loaded, disconnect it from loading thumbnail
                        connect(thumbnail.data(), &Thumbnail::Updated, this , [this, key, thumbnail]()
                            {
                                disconnect(m_loadingThumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                                thumbnail->disconnect();
                                connect(thumbnail.data(), &Thumbnail::Updated, key.data(), &ThumbnailKey::ThumbnailUpdatedSignal);
                                connect(key.data(), &ThumbnailKey::UpdateThumbnailSignal, thumbnail.data(), &Thumbnail::Update);
                                key->m_ready = true;
                                Q_EMIT key->ThumbnailUpdatedSignal();
                            });
                        thumbnail->Load();
                    }
                    if (thumbnail->GetState() == Thumbnail::State::Failed)
                    {
                        return m_missingThumbnail;
                    }
                    return m_loadingThumbnail;
                }
            }
            return m_missingThumbnail;
        }

        void ThumbnailContext::RegisterThumbnailProvider(SharedThumbnailProvider provider)
        {
            provider->SetThumbnailSize(m_thumbnailSize);
            m_providers.append(provider);
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include <Thumbnails/ThumbnailContext.moc>