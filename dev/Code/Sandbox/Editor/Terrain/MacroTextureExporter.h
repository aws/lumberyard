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

class CCryMemFile;
class CRGBLayer;

/*!
 * Exporter for a tiled, quadtree-based megatexture format. Source pixel information is
 * retrieved from the supplied CRGBLayer instance. Tiles are sampled and exported to a flat
 * file which can be loaded by the MacroTexture engine runtime class.
 */
class MacroTextureExporter
{
public:
    MacroTextureExporter(CRGBLayer* texture);

    //! Clamps the sampled output texture to the specified size in pixels. When exporting,
    //! the size of the current tile is extrapolated across the surface of the texture for
    //! the current mip level. If that size exceeds the supplied maximum then it is culled from
    //! export.
    inline void SetMaximumTotalSize(float sizeInPixels)
    {
        m_Settings.MaximumTotalSize = sizeInPixels;
    }

    using ProgressCB = std::function<void(uint32)>;

    //! Invokes the callback method with a progress percentage of the current export process.
    inline void SetProgressCallback(ProgressCB callback)
    {
        m_Settings.ProgressCallback = callback;
    }

    //! Invokes the export process and persists the result to the given filename.
    bool Export(const char* filename);

private:
    class Context;

    static const uint32 TileResolution = 256;

    void BuildIndexBlock(
        std::vector<int16>& indexBlock,
        uint32& outTextureId,
        const float left,
        const float bottom,
        const float size,
        const uint32 recursionDepth);

    struct Settings
    {
        ProgressCB ProgressCallback;

        uint32 MaximumTotalSize;

        CRGBLayer* Texture;
    };

    Settings m_Settings;
};
