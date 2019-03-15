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

#include "TextureAtlas_precompiled.h"
#include "TextureAtlasImpl.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace TextureAtlasNamespace
{
    TextureAtlasImpl::TextureAtlasImpl()
    {
        m_image = nullptr;
    }

    TextureAtlasImpl::~TextureAtlasImpl() {}

    TextureAtlasImpl::TextureAtlasImpl(AtlasCoordinateSets handles)
    {
        for (int i = 0; i < handles.size(); ++i)
        {
            this->m_data[handles[i].first] = handles[i].second;
        }
        m_image = nullptr;
    }

    // Reflect The coordinates and the coordinate format
    void TextureAtlasImpl::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TextureAtlasImpl>()->Version(1)
                ->Field("Coordinate Pairs", &TextureAtlasImpl::m_data)
                ->Field("Width", &TextureAtlasImpl::m_width)
                ->Field("Height", &TextureAtlasImpl::m_height);
            AzFramework::SimpleAssetReference<TextureAtlasAsset>::Register(*serialize);
        }
        AtlasCoordinates::Reflect(context);
    }

    // Coordinates reflect their internal properties
    void AtlasCoordinates::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtlasCoordinates>()
                ->Version(1)
                ->Field("Left", &AtlasCoordinates::m_left)
                ->Field("Top", &AtlasCoordinates::m_top)
                ->Field("Width", &AtlasCoordinates::m_width)
                ->Field("Height", &AtlasCoordinates::m_height);
        }
    }

    // Retrieves the value that corresponds to a given handle in the atlas
    AtlasCoordinates TextureAtlasImpl::GetAtlasCoordinates(const AZStd::string& handle) const
    {
        AZStd::string path = handle;
        path = path.substr(0, path.find_last_of('.'));
        // Use an iterator to check if the key is being used in the hash table
        auto iterator = m_data.find(path);
        if (iterator != m_data.end())
        {
            return iterator->second;
        }
        else
        {
            return AtlasCoordinates(-1, -1, -1, -1);
        }
    }

    // Links this atlas to an image pointer
    void TextureAtlasImpl::SetTexture(ITexture* image)
    {
        // We don't need to delete the old value because the pointer is handled elsewhere
        m_image = image;
    }
    
    // Returns the image linked to this atlas
    ITexture* TextureAtlasImpl::GetTexture() const
    {
        return m_image;
    }
    
    // Internal to gem function for overwriting parameters
    void TextureAtlasImpl::OverwriteMappings(TextureAtlasImpl* source)
    {
        m_data.clear();
        m_data = source->m_data;
        m_width = source->m_width;
        m_height = source->m_height;
    }
}