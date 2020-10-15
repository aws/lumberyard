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

class TexturePool
{
public:
    TexturePool(uint32 maxElementCount);
    ~TexturePool();

    void Create(uint32 dimension, ETEX_Format format);

    void Update();

    bool Exists() const;

    uint32 Allocate(const byte* data, EEndian endian);

    void Release(uint32 nTexId);

    void ReleaseAll();

    inline uint32 GetCapacity() const
    {
        return m_MaxElementCount;
    }

    inline uint32 GetUsedTextureCount() const
    {
        return m_UsedTextures.Count();
    }

private:
    void Validate();
    void Reset();

    IRenderer& m_Renderer;

    PodArray<uint32> m_FreeTextures;
    PodArray<uint32> m_UsedTextures;
    PodArray<uint32> m_Quarantine;
    uint32 m_Dimension;
    ETEX_Format m_Format;
    uint32 m_MaxElementCount;
};