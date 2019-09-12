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
#include "TerrainGrid.h"

#include "Terrain/Heightmap.h"
#include "TerrainTexGen.h"

#include <I3DEngine.h>

//////////////////////////////////////////////////////////////////////////
CTerrainGrid::CTerrainGrid()
{
    m_numSectors = 0;
    m_resolution = 0;
    m_sectorSize = 0;
    m_sectorResolution = 0;
    m_pixelsPerMeter = 0;
}

//////////////////////////////////////////////////////////////////////////
CTerrainGrid::~CTerrainGrid()
{
    ReleaseSectorGrid();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainGrid::InitSectorGrid(int numSectors)
{
    ReleaseSectorGrid();
    m_numSectors = numSectors;
    m_sectorGrid.resize(m_numSectors * m_numSectors);
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        m_sectorGrid[i] = 0;
    }

    SSectorInfo si;
    GetIEditor()->GetHeightmap()->GetSectorsInfo(si);
    m_sectorSize = si.sectorSize;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainGrid::ReleaseSectorGrid()
{
    IRenderer* pRenderer = GetIEditor()->GetRenderer();

    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        if (m_sectorGrid[i])
        {
            pRenderer->RemoveTexture(m_sectorGrid[i]->textureId);

            delete m_sectorGrid[i];
        }
    }
    m_sectorGrid.clear();
    m_numSectors = 0;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainGrid::SetResolution(int resolution)
{
    m_resolution = resolution;

    if (abs(m_numSectors) > 0)
    {
        m_sectorResolution = m_resolution / m_numSectors;
    }
    else
    {
        m_sectorResolution = 0;
    }

    if (abs(m_sectorSize) > 0)
    {
        m_pixelsPerMeter = ((float)m_sectorResolution) / m_sectorSize;
    }
    else
    {
        m_pixelsPerMeter = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
CTerrainSector* CTerrainGrid::GetSector(const QPoint& sector)
{
    assert(sector.x() >= 0 && sector.x() < m_numSectors && sector.y() >= 0 && sector.y() < m_numSectors);
    int index = sector.x() + sector.y() * m_numSectors;
    if (!m_sectorGrid[index])
    {
        m_sectorGrid[index] = new CTerrainSector;
    }
    return m_sectorGrid[index];
}

//////////////////////////////////////////////////////////////////////////
QPoint CTerrainGrid::SectorToTexture(const QPoint& sector)
{
    return sector * m_sectorResolution;
}

//////////////////////////////////////////////////////////////////////////
QPoint CTerrainGrid::WorldToSector(const Vec3& wp)
{
    //swap x/y
    return QPoint(int(wp.y / m_sectorSize), int(wp.x / m_sectorSize));
}

//////////////////////////////////////////////////////////////////////////
Vec3 CTerrainGrid::SectorToWorld(const QPoint& sector)
{
    //swap x/y
    return Vec3(sector.y() * m_sectorSize, sector.x() * m_sectorSize, 0);
}

//////////////////////////////////////////////////////////////////////////
Vec2 CTerrainGrid::WorldToTexture(const Vec3& wp)
{
    //swap x/y
    return Vec2(wp.y * m_pixelsPerMeter, wp.x * m_pixelsPerMeter);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainGrid::GetSectorRect(const QPoint& sector, QRect& rect)
{
    rect.setTopLeft(sector * m_sectorResolution);
    rect.setSize(QSize(m_sectorResolution, m_sectorResolution));
}

//////////////////////////////////////////////////////////////////////////
int CTerrainGrid::LockSectorTexture(const QPoint& sector, const uint32 dwTextureResolution, bool& bTextureWasRecreated)
{
    CTerrainSector* st = GetSector(sector);
    assert(st);
    GetIEditor()->GetHeightmap()->AddModSector(sector.x(), sector.y());
    bTextureWasRecreated = false;

    I3DEngine* p3Engine = GetIEditor()->Get3DEngine();
    IRenderer* pRenderer = GetIEditor()->GetRenderer();

    // if the texture existis already - make sure the size fits
    {
        ITexture* pTex = pRenderer->EF_GetTextureByID(st->textureId);

        if (pTex)
        {
            if (pTex->GetWidth() != dwTextureResolution || pTex->GetHeight() != dwTextureResolution)
            {
                pRenderer->RemoveTexture(st->textureId);
                pTex = 0;

                p3Engine->SetTerrainSectorTexture(sector.y(), sector.x(), 0, 0, 0);
                st->textureId = 0;
                bTextureWasRecreated = true;
            }
        }
    }

    if (!st->textureId)
    {
        st->textureId = pRenderer->DownLoadToVideoMemory(0, dwTextureResolution, dwTextureResolution,
                eTF_B8G8R8A8, eTF_B8G8R8A8, 0, false, FILTER_LINEAR, 0, 0, FT_USAGE_ALLOWREADSRGB);

        p3Engine->SetTerrainSectorTexture(sector.y(), sector.x(), st->textureId, dwTextureResolution, dwTextureResolution);
    }

    return st->textureId;
}

void CTerrainGrid::UnlockSectorTexture(const QPoint& sector)
{
    CTerrainSector* st = GetSector(sector);
    assert(st);

    I3DEngine* p3Engine = GetIEditor()->Get3DEngine();
    IRenderer* pRenderer = GetIEditor()->GetRenderer();
    ITexture* pTex = pRenderer->EF_GetTextureByID(st->textureId);

    if (pTex)
    {
        pRenderer->RemoveTexture(st->textureId);
        pTex = 0;

        p3Engine->SetTerrainSectorTexture(sector.y(), sector.x(), 0, 0, 0);
        st->textureId = 0;
    }
}


//////////////////////////////////////////////////////////////////////////
void CTerrainGrid::GetMemoryUsage(ICrySizer* pSizer)
{
    pSizer->Add(*this);

    pSizer->Add(m_sectorGrid);

    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        if (m_sectorGrid[i])
        {
            pSizer->Add(*m_sectorGrid[i]);
        }
    }
}
