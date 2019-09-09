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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <AzToolsFramework/Thumbnails/ProductThumbnail.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        const int DEFAULT_THUMBNAIL_SIZE = 100;

        ThumbnailerComponent::ThumbnailerComponent()
        {
        }

        ThumbnailerComponent::~ThumbnailerComponent() = default;

        void ThumbnailerComponent::Activate()
        {
            RegisterContext("Default", DEFAULT_THUMBNAIL_SIZE);
            RegisterThumbnailProvider(MAKE_TCACHE(ProductThumbnailCache), "Default");
            BusConnect();
        }

        void ThumbnailerComponent::Deactivate()
        {
            BusDisconnect();
        }

        void ThumbnailerComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ThumbnailerComponent, AZ::Component>();
            }
        }

        void ThumbnailerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ThumbnailerService", 0x65422b97));
        }

        void ThumbnailerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ThumbnailerService", 0x65422b97));
        }

        void ThumbnailerComponent::RegisterContext(const char* contextName, int thumbnailSize)
        {
            AZ_Assert(m_thumbnails.find(contextName) == m_thumbnails.end(), "Context %s already registered", contextName);
            m_thumbnails[contextName] = AZStd::make_shared<ThumbnailContext>(thumbnailSize);
        }

        void ThumbnailerComponent::RegisterThumbnailProvider(SharedThumbnailProvider provider, const char* contextName)
        {
            auto it = m_thumbnails.find(contextName);
            AZ_Assert(it != m_thumbnails.end(), "Context %s not registered", contextName);
            it->second->RegisterThumbnailProvider(provider);
        }

        SharedThumbnail ThumbnailerComponent::GetThumbnail(SharedThumbnailKey key, const char* contextName)
        {
            auto it = m_thumbnails.find(contextName);
            AZ_Assert(it != m_thumbnails.end(), "Context %s not registered", contextName);
            return it->second->GetThumbnail(key);
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework
#include <Thumbnails/ThumbnailerComponent.moc>
