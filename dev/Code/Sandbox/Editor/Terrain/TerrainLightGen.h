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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_TERRAINLIGHTGEN_H
#define CRYINCLUDE_EDITOR_TERRAIN_TERRAINLIGHTGEN_H
#pragma once


// forward declaration.
class CLayer;
struct LightingSettings;
class CHeightmap;


/** Class that generates terrain surface texture for lighting
*/
class CTerrainLightGen
{
public: // -----------------------------------------------------------------------

    // default constructor
    CTerrainLightGen
    (
        const int cApplySS = 1,
        CPakFile* m_pLevelPakFile = NULL
    );
    // destructor
    ~CTerrainLightGen();

    // Generate terrain surface texture.
    // @param surfaceTexture Output image where texture will be stored, it must already be allocated.
    // to the size of surfaceTexture.
    // @param sector Terrain sector to generate texture for.
    // @param rect Region on the terrain where texture must be generated within sector..
    // @param pOcclusionSurfaceTexture optional occlusion surface texture
    // @return true if texture was generated.
    bool GenerateSectorTexture(const QPoint& sector, const QRect& rect, int flags, CImageEx& surfaceTexture);

    // might be valid (!=0, depending on mode) after GenerateSectorTexture(), release with ReleaseOcclusionSurfaceTexture()
    const CImageEx* GetOcclusionSurfaceTexture() const;

    //
    void ReleaseOcclusionSurfaceTexture();

    //! Invalidate all lighting valid flags for all sectors..
    void InvalidateLighting();

    //
    // Arguments:
    //   bFullInit - only needed (true) if m_hmap is used (not for exporting but for preview) - should be removed one day
    void Init(const int resolution, const bool bFullInit);

    void GetSubImageStretched(const float fSrcLeft, const float fSrcTop, const float fSrcRight, const float fSrcBottom, CImageEx& rOutImage, const int genFlags);

private: // ---------------------------------------------------------------------


    //////////////////////////////////////////////////////////////////////////
    // Sectors.
    //////////////////////////////////////////////////////////////////////////
    //! Get rectangle for this sector.
    void GetSectorRect(const QPoint& sector, QRect& rect);
    //! Get terrain sector info.
    int& GetCLightGenSectorFlag(const QPoint& sector);
    //! Get terrain sector.
    int GetSectorFlags(const QPoint& sector);
    //! Add flags to sector.
    void SetSectorFlags(const QPoint& sector, int flags);

    //////////////////////////////////////////////////////////////////////////


    //! calculate the shadow (only hills)
    //! \param inpHeightmapData must not be 0
    //! \param iniResolution
    //! \param iniShift
    //! \param iniX [0..iniWidth-1]
    //! \param iniY [0..iniHeight-1]
    //! \param invSunVector normalized vector to the sun
    //! \param infShadowBlur defines the slope blurring, 0=no blurring, .. (angle would be better but is slower)
    //! \return 0..1
    float GetSunAmount(const float* inpHeightmapData, int iniX, int iniY, float infInvHeightScale,
        const Vec3& vSunShadowVector, const float infShadowBlur) const;

    //! use precalculated data, bilinear filtered
    //! \param iniX [0..m_resolution-1]
    //! \param iniY [0..m_resolution-1]
    //! \return 0.0f=no hemisphere visible .. 1.0f =full hemisphere visible
    float GetSkyAccessibilityFast(const int iniX, const int iniY) const;

    //! use precalculated data, bilinear filtered
    //! \param iniX [0..m_resolution-1]
    //! \param iniY [0..m_resolution-1]
    //! \return 0.0f=no sun visible .. 1.0f =full sun visible
    float GetSunAccessibilityFast(const int iniX, const int iniY) const;

    //! use precalculated data, bilinear filtered
    //! \param fX [0..1]
    //! \param fY [0..1]
    //! \return 0.0f=no hemisphere visible .. 1.0f =full hemisphere visible
    float GetSkyAccessibilityFloat(const float fX, const float fY) const;

    //! use precalculated data, bilinear filtered
    //! \param fX [0..1]
    //! \param fY [0..1]
    //! \return 0.0f=no sun visible .. 1.0f =full sun visible
    float GetSunAccessibilityFloat(const float fX, const float fY) const;

    // Calculate lightmap for sector.
    bool GenerateLightmap(const QPoint& sector, LightingSettings* ls, CImageEx& lightmap, int genFlags);
    void GenerateShadowmap(const QPoint& sector, CByteImage& shadowmap, float shadowAmmount, const Vec3& sunVector);
    void UpdateSectorHeightmap(const QPoint& sector);
    bool UpdateWholeHeightmap();
    //! Caluclate max terrain height (Optimizes calculation of shadows and sky accessibility).
    void CalcTerrainMaxZ();

    //! Log generation progress.
    void Log(const char* format, ...);

    //! you have to do it for the whole map and that can take a min but using the result is just a lookup
    //! upate m_SkyAccessiblity and m_SunAccessiblity
    //! \return true=success, false otherwise
    bool RefreshAccessibility(const LightingSettings* inpLS, int genFlags);

    float CalcHeightScaleForLighting(const LightingSettings* pSettings, const DWORD indwTargetResolution) const;

    //////////////////////////////////////////////////////////////////////////
    // Vars.
    //////////////////////////////////////////////////////////////////////////

    bool                            m_bLog;                                                     // true if ETTG_QUIET was not specified
    bool                            m_bNotValid;
    unsigned int            m_resolution;                                           // target texture resolution

    unsigned int            m_resolutionShift;                              // (1 << m_resolutionShift) == m_resolition.
    int                             m_sectorResolution;                             // Resolution of sector.
    int                             m_numSectors;                                           // Number of sectors per side.
    float                           m_pixelSizeInMeters;                            // Size of one pixel on generated texture in meters.
    float                           m_terrainMaxZ;                                      // Highest point on terrain. use CalcTerrainMaxZ() to update this value

    CHeightmap*            m_heightmap;                                             // pointer to the editor heihgtmap (not scaled up to the target resolution)
    CImageEx                        m_OcclusionSurfaceTexture;              // might be valid (!=0, depending on mode) after GenerateSectorTexture(), release with ReleaseOcclusionSurfaceTexture()

    std::vector<int>    m_sectorGrid;                               // Sector grid.

    CFloatImage             m_hmap;                                                     // Local heightmap (scaled up to the target resolution)

    // Sky Accessiblity
    CByteImage              m_SkyAccessiblity;                              // Amount of sky that is visible for each point (4MB for 2048x2048) (not scaled up from source, but might be scaled down), 0=no sky accessible, 255= full sky accessible
    int                             m_iCachedSkyAccessiblityQuality;    // to detect changes to refresh m_SkyAccessiblity
    //  int                             m_iCachedSkyBrightening;                    // to detect changes to refresh m_SkyAccessiblity

    // Sun Accessibility
    CByteImage              m_SunAccessiblity;                              // Amount of sun that is visible for each point (4MB for 2048x2048) (not scaled up from source, but might be scaled down), 0=no sun accessible, 255= full sun accessible
    //  int                             m_iCachedSunBlurLevel;                      // to detect changes to refresh m_SunAccessiblity
    Vec3                            m_vCachedSunDirection;                      // to detect changes to refresh m_SunAccessiblity

    CPakFile* m_pLevelPakFile;                                                      // reference to PAK file

    uint32                      m_TerrainAccMapRes;                             // resolution of terrain accessibility map
    uint8* m_pTerrainAccMap;                                                        // terrain accessibility map generated by indirect lighting
    int                             m_ApplySS;                                              // quality for indirect lighting: apply super sampling to terrain occl
    //  float                           m_TerrainGIClamp;                                   // clamp value for terrain IL, 0..1

    friend class CTerrainTexGen;
};


#endif // CRYINCLUDE_EDITOR_TERRAIN_TERRAINLIGHTGEN_H
