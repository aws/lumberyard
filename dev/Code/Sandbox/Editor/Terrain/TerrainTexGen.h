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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_TERRAINTEXGEN_H
#define CRYINCLUDE_EDITOR_TERRAIN_TERRAINTEXGEN_H
#pragma once


#include "TerrainGrid.h"

// forward declaration.
class CLayer;
struct LightingSettings;


enum ETerrainTexGenFlags
{
    ETTG_LIGHTING                                   = 0x0001,
    ETTG_SHOW_WATER                             = 0x0002,
    ETTG_STATOBJ_SHADOWS                    = 0x0010,
    ETTG_QUIET                                      = 0x0020,   // Disable all logging and progress indication.
    ETTG_INVALIDATE_LAYERS              = 0x0040,   // Invalidate stored layers information.
    ETTG_NOTEXTURE                              = 0x0080,   // Not generate texture information (Only lighting).
    ETTG_USE_LIGHTMAPS                      = 0x0100,   // Use lightmaps for sectors.
    ETTG_STATOBJ_PAINTBRIGHTNESS    = 0x0200,   // Paint brightness of vegetation objects.
    ETTG_FAST_LLIGHTING                     = 0x0400,   // Use fastest possible lighting algorithm.
    ETTG_NO_TERRAIN_SHADOWS             = 0x0800,   // Do not calculate terrain shadows (Even if specified in lighting settings)
    ETTG_BAKELIGHTING                           = 0x1000,   // enforce one result (e.g. eDynamicSun results usually in two textures)
    ETTG_PREVIEW                                    = 0x2000,   // result is intended for the preview not rthe permanent storage
};

const uint32 OCCMAP_DOWNSCALE_FACTOR = 1;       // occlusionmap resolution factor to the diffuse texture, can be 1, 2, or 4

#include "TerrainLayerTexGen.h"                 // CTerrainLayerTexGen
#include "TerrainLightGen.h"                        // CTerrainLayerGen

/** Class that generates terrain surface texture.
*/
class CTerrainTexGen
{
public:

    // default constructor with standard resolution
    CTerrainTexGen();

    /** Generate terrain surface texture.
            @param surfaceTexture Output image where texture will be stored, it must already be allocated.
                to the size of surfaceTexture.
            @param sector Terrain sector to generate texture for.
            @param rect Region on the terrain where texture must be generated within sector..
            @param pOcclusionSurfaceTexture optional occlusion surface texture
            @return true if texture was generated.
    */
    bool GenerateSectorTexture(const QPoint& sector, const QRect& rect, int flags, CImageEx& surfaceTexture);

    // might be valid (!=0, depending on mode) after GenerateSectorTexture(), release with ReleaseOcclusionSurfaceTexture()
    const CImageEx* GetOcclusionSurfaceTexture() const;

    //
    void ReleaseOcclusionSurfaceTexture();

    //! Generate whole surface texture.
    bool GenerateSurfaceTexture(int flags, CImageEx& surfaceTexture);

    //! Query layer mask for pointer to layer.
    CByteImage* GetLayerMask(CLayer* layer);

    //! Invalidate layer valid flags for all sectors..
    void InvalidateLayers();

    //! Invalidate all lighting valid flags for all sectors..
    void InvalidateLighting();

    void Init(const int resolution, const bool bFullInit);

private: // ---------------------------------------------------------------------

    CTerrainLayerTexGen             m_LayerTexGen;              //
    CTerrainLightGen                    m_LightGen;                     //
};


#endif // CRYINCLUDE_EDITOR_TERRAIN_TERRAINTEXGEN_H
