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

#include "StdAfx.h"

#include "CloudVolumeTexture.h"

namespace CloudsGem
{
    static const size_t STAGING_BUFFER_FRAME_COUNT = 2;

    CloudVolumeTexture::~CloudVolumeTexture()
    {
#if !defined(DEDICATED_SERVER)

        if (m_texture)
        {
            gEnv->pRenderer->RemoveTexture(m_texture->GetTextureID());
        }
#endif
    }

    uint8_t* CloudVolumeTexture::GetCurrentStagingData()
    {
        const size_t sliceSize = m_stagingData.size() / STAGING_BUFFER_FRAME_COUNT;
        return &m_stagingData[m_frameIndex * sliceSize];
    }

    bool CloudVolumeTexture::Create(uint32 width, uint32 height, uint32 depth, unsigned char* pData)
    {
#if !defined(DEDICATED_SERVER)

        if (!m_texture)
        {
            static uint32_t id = 0;
            char name[128];
            name[sizeof(name) - 1] = '\0';
            azsnprintf(name, sizeof(name) - 1, "$CloudVolObj_%d", id++);

            const uint32_t totalByteCount = width * height * depth;
            m_stagingData.resize(totalByteCount * STAGING_BUFFER_FRAME_COUNT);

            if (pData)
            {
                uint8_t* currentStagingData = GetCurrentStagingData();
                memcpy(currentStagingData, pData, totalByteCount);
                pData = currentStagingData;
            }

            int flags(FT_DONT_STREAM);
            IRenderer* renderer = gEnv->pRenderer;
            m_texture = renderer->Create3DTexture(name, width, height, depth, 1, flags, pData, eTF_A8, eTF_A8);

            m_width = width;
            m_height = height;
            m_depth = depth;
        }
#endif
        return m_texture != nullptr;
    }

    bool CloudVolumeTexture::Update(uint32 width, uint32 height, uint32 depth, const unsigned char* pData)
    {
        if (CTexture::IsTextureExist(m_texture))
        {
            m_frameIndex ^= m_frameIndex;

#if !defined(DEDICATED_SERVER)
            uint8_t* stagingData = GetCurrentStagingData();
            uint32 cpyWidth = min(width, m_width);
            uint32 cpyHeight = min(height, m_height);
            uint32 cpyDepth = min(depth, m_depth);
            memcpy(stagingData, pData, cpyWidth * cpyHeight * cpyDepth);

           m_texture->UpdateTextureRegion(stagingData, 0, 0, 0, cpyWidth, cpyHeight, cpyDepth, m_texture->GetDstFormat());
#endif
           return true;
        }
        return false;
    }
}
