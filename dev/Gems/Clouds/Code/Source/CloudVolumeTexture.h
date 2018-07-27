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
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#pragma once

#include "VertexFormats.h"

namespace CloudsGem
{
    /**
     * CloudVolumeTexture
     */
    class CloudVolumeTexture : public IVolumeTexture
    {
    public:

        CloudVolumeTexture() {}
        ~CloudVolumeTexture();

        void Release() override {};
        bool Create(uint32 width, uint32 height, uint32 depth, unsigned char* pData) override;
        bool Update(uint32 width, uint32 height, uint32 depth, const unsigned char* pData) override;
        int32 GetTexID() const override { return m_texture ? m_texture->GetTextureID() : 0; }
        uint32 GetWidth() const override { return m_width; }
        uint32 GetHeight() const override { return m_height; }
        uint32 GetDepth() const override { return m_depth; }
        ITexture* GetTexture() const override { return m_texture; }

    private:

        /** Gets current staging data */
        uint8_t* GetCurrentStagingData();

        // Members
        uint32 m_width{ 0 };
        uint32 m_height{ 0 };
        uint32 m_depth{ 0 };
        ITexture* m_texture{ nullptr };
        uint8_t m_frameIndex{ 0 };
        AZStd::vector<uint8_t> m_stagingData;
    };
}