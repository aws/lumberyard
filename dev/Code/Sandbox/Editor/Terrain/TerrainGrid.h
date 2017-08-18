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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_TERRAINGRID_H
#define CRYINCLUDE_EDITOR_TERRAIN_TERRAINGRID_H
#pragma once


/** Represent single terrain sector.
*/
class CTerrainSector
{
public:
    CTerrainSector()
    {
        textureId = 0;
    }

public:
    //! Id of surface texture.
    unsigned int textureId;
};

/** Manages grid of terrain sectors.
*/
class CTerrainGrid
{
public:
    // constructor
    CTerrainGrid();
    // destructor
    ~CTerrainGrid();

    //! Initialize grid.
    void InitSectorGrid(int numSectors);

    //! Set target texture resolution.
    void SetResolution(int resolution);

    //! Get terrain sector.
    CTerrainSector* GetSector(const QPoint& sector);

    //! Lock texture of sector and return texture id.
    int LockSectorTexture(const QPoint& sector, const uint32 dwTextureResolution, bool& bTextureWasRecreated);
    void UnlockSectorTexture(const QPoint& sector);

    //////////////////////////////////////////////////////////////////////////
    // Coordinate conversions.
    //////////////////////////////////////////////////////////////////////////
    void GetSectorRect(const QPoint& sector, QRect& rect);
    //! Map from sector coordinates to texture coordinates.
    QPoint SectorToTexture(const QPoint& sector);
    //! Map world position to sector space.
    QPoint WorldToSector(const Vec3& wp);
    //! Map sector coordinates to world coordinates.
    Vec3 SectorToWorld(const QPoint& sector);
    //! Map world position to texture space.
    Vec2 WorldToTexture(const Vec3& wp);

    //! Return number of sectors per side.
    int GetNumSectors() const { return m_numSectors; }

    void GetMemoryUsage(ICrySizer* pSizer);

private: // ---------------------------------------------------------------

    std::vector<CTerrainSector*>    m_sectorGrid;                       // Sector grid.
    int                                                     m_numSectors;                       // Number of sectors per side.
    int                                                     m_resolution;                       // Resolution of texture and heightmap.
    int                                                     m_sectorResolution;         // Sector texture size.
    int                                                     m_sectorSize;                       // Size of sector in meters.
    float                                               m_pixelsPerMeter;               // Texture Pixels per meter.

    //! Clear all sectors.
    void ReleaseSectorGrid();
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_TERRAINGRID_H
