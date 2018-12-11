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

#include "StdAfx.h"
#include "TexturePool.h"

TexturePool::TexturePool(uint32 maxElementCount)
    : m_Renderer(*gEnv->pRenderer)
    , m_Format(eTF_Unknown)
    , m_Dimension(0)
    , m_MaxElementCount(maxElementCount)
{
}

TexturePool::~TexturePool()
{
    if (Exists())
    {
        Validate();
    }
    Reset();
}

void TexturePool::Reset()
{
    ReleaseAll();
    m_FreeTextures.Clear();

    m_Format = eTF_Unknown;
    m_Dimension = 0;
}

void TexturePool::Validate()
{
    AZ_Assert(m_FreeTextures.Count() + m_Quarantine.Count() + m_UsedTextures.Count() == m_MaxElementCount, "texture pool got corrupted or used without being initialized correctly");
}

uint32 TexturePool::Allocate(const byte* data, EEndian endian)
{
    if (!m_FreeTextures.Count())
    {
        Update();
    }

    if (!m_FreeTextures.Count())
    {
        Cry3DEngineBase::Error("TexturePool::GetTexture: Pool full, can't allocate new texture.");
        return 0;
    }

    int id = m_FreeTextures.Last();

    id = m_Renderer.DownLoadToVideoMemory(
            data,               // upload data
            m_Dimension,        // width
            m_Dimension,        // height
            m_Format,           // source format
            m_Format,           // dest format
            0,                  // mipmap count
            false,              // use repeat wrapping mode
            FILTER_NONE,        // filter mode
            id,                 // existing texture id
            nullptr,            // texture name
            FT_USAGE_ALLOWREADSRGB, // flags
            endian);            // platform endian
    if (id)
    {
        m_FreeTextures.DeleteLast();
        m_UsedTextures.Add(id);
    }
    Validate();
    return id;
}

void TexturePool::Release(uint32 nTexId)
{
    if (m_UsedTextures.Delete(nTexId))
    {
        m_Quarantine.Add(nTexId);
    }
    else
    {
        AZ_Assert(false, "Attempt to release non pooled texture");
    }
    Validate();
}

void TexturePool::ReleaseAll()
{
    m_FreeTextures.AddList(m_UsedTextures);
    m_FreeTextures.AddList(m_Quarantine);
    for (int i = 0; i < m_FreeTextures.Count(); i++)
    {
        m_Renderer.RemoveTexture(m_FreeTextures[i]);
    }
    m_UsedTextures.Clear();
    m_Quarantine.Clear();
}

void TexturePool::Update()
{
    m_FreeTextures.AddList(m_Quarantine);
    m_Quarantine.Clear();
}

bool TexturePool::Exists() const
{
    return (m_FreeTextures.Count() + m_Quarantine.Count() + m_UsedTextures.Count()) > 0;
}

void TexturePool::Create(uint32 dimension, ETEX_Format format)
{
    if (m_Format != format || m_Dimension != dimension)
    {
        Reset();

        m_Format = format;
        m_Dimension = dimension;

        for (uint32 i = 0; i < m_MaxElementCount; i++)
        {
            stack_string textureName;
            textureName.Format("$TexturePool_%d_%p", i, this);
            uint32 id = m_Renderer.DownLoadToVideoMemory(
                    nullptr,                // upload data
                    dimension,              // width
                    dimension,              // height
                    format,                 // source format
                    format,                 // dest format
                    0,                      // mipmap count
                    false,                  // use wrap address mode
                    FILTER_NONE,            // filter mode
                    0,                      // existing texture id (0 = allocate new texture)
                    textureName.c_str(),    // name of texture
                    FT_USAGE_ALLOWREADSRGB, // flags
                    GetPlatformEndian(),    // platform endian
                    nullptr,                // rect region to copy
                    true);                  // async device texture creation

            m_FreeTextures.Add(id);
        }
    }

    Validate();
}