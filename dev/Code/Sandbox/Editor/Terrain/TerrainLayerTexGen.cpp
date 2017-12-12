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
#include "TerrainLayerTexGen.h"

#include "CryEditDoc.h"
#include "TerrainLighting.h"
#include "TerrainGrid.h"
#include "Layer.h"
#include "Sky Accessibility/HeightmapAccessibility.h"
#include "TerrainTexGen.h"

#include "Terrain/TerrainManager.h"

// Sector flags.
enum
{
    eSectorLayersValid  = 0x04
};


//////////////////////////////////////////////////////////////////////////
CTerrainLayerTexGen::CTerrainLayerTexGen(const int resolution)
{
    m_bLog = true;

    Init(resolution);
}

CTerrainLayerTexGen::CTerrainLayerTexGen()
{
    m_bLog = true;

    SSectorInfo si;
    GetIEditor()->GetHeightmap()->GetSectorsInfo(si);
    Init(si.surfaceTextureSize);
}

//////////////////////////////////////////////////////////////////////////
CTerrainLayerTexGen::~CTerrainLayerTexGen()
{
    // Release masks for all layers to save memory.
    int numLayers = GetLayerCount();

    for (int i = 0; i < numLayers; i++)
    {
        if (m_layers[i].m_pLayerMask)
        {
            delete m_layers[i].m_pLayerMask;
        }

        CLayer* pLayer = GetLayer(i);

        pLayer->ReleaseTempResources();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainLayerTexGen::Init(const int resolution)
{
    int i;

    m_resolution = resolution;

    // Fill layers array.
    ClearLayers();

    int numLayers = GetIEditor()->GetTerrainManager()->GetLayerCount();
    m_layers.reserve(numLayers + 2);   // Leave some space for water layers.
    for (i = 0; i < numLayers; i++)
    {
        SLayerInfo li;
        li.m_pLayer = GetIEditor()->GetTerrainManager()->GetLayer(i);
        m_layers.push_back(li);
    }

    SSectorInfo si;
    GetIEditor()->GetHeightmap()->GetSectorsInfo(si);

    m_numSectors = si.numSectors;
    if (m_numSectors > 0)
    {
        m_sectorResolution = resolution / m_numSectors;
    }

    // Invalidate all sectors.
    m_sectorGrid.resize(m_numSectors * m_numSectors);
    if (m_sectorGrid.size() > 0)
    {
        memset(&m_sectorGrid[0], 0, m_sectorGrid.size() * sizeof(m_sectorGrid[0]));
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainLayerTexGen::UpdateSectorLayers(const QPoint& sector)
{
    QRect sectorRect;
    GetSectorRect(sector, sectorRect);

    // Local heightmap (scaled up to the target resolution)
    CFloatImage hmap;

    QRect recHMap;
    GetSectorRect(sector, recHMap);

    const int iBorder = 2;

    // Increase rectangle by .. pixel..
    recHMap.adjust(-iBorder, -iBorder, iBorder, iBorder);

    // Update heightmap for that sector.
    {
        int sectorFlags = GetSectorFlags(sector);

        // Allocate heightmap big enough.
        {
            if (!hmap.Allocate(recHMap.width(), recHMap.height()))
            {
                return false;
            }

            hmap.Clear();

            // Invalidate all sectors.
            m_sectorGrid.resize(m_numSectors * m_numSectors);
            memset(&m_sectorGrid[0], 0, m_sectorGrid.size() * sizeof(m_sectorGrid[0]));
        }

        GetIEditor()->GetHeightmap()->GetData(recHMap, m_resolution, recHMap.topLeft(), hmap, true, true);
    }

    // For this sector all layers are valid.
    SetSectorFlags(sector, eSectorLayersValid);

    return true;
}





////////////////////////////////////////////////////////////////////////
// Generate the surface texture with the current layer and lighting
// configuration and write the result to surfaceTexture.
// Also give out the results of the terrain lighting if pLightingBit is not NULL.
////////////////////////////////////////////////////////////////////////
bool CTerrainLayerTexGen::GenerateSectorTexture(const QPoint& sector, const QRect& rect, int flags, CImageEx& surfaceTexture)
{
    // set flags.
    bool bShowWater = flags & ETTG_SHOW_WATER;
    bool bNoTexture = flags & ETTG_NOTEXTURE;
    bool bUseLightmap = flags & ETTG_USE_LIGHTMAPS;
    m_bLog = !(flags & ETTG_QUIET);

    if (flags & ETTG_INVALIDATE_LAYERS)
    {
        InvalidateLayers();
    }

    uint32 i;

    CCryEditDoc* pDocument = GetIEditor()->GetDocument();
    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    CTexSectorInfo& texsectorInfo = GetCTexSectorInfo(sector);
    int sectorFlags = texsectorInfo.m_Flags;

    assert(pDocument);
    assert(pHeightmap);

    float waterLevel = pHeightmap->GetWaterLevel();

    if (bNoTexture)
    {
        // fill texture with white color if there is no texture present
        surfaceTexture.Fill(255);
    }
    else
    {
        // Enable texturing.
        //////////////////////////////////////////////////////////////////////////


        ////////////////////////////////////////////////////////////////////////
        // Generate the layer masks
        ////////////////////////////////////////////////////////////////////////

        // if lLayers not valid for this sector, update them.
        // Update layers for this sector.
        if (!(sectorFlags & eSectorLayersValid))
        {
            if (!UpdateSectorLayers(sector))
            {
                return false;
            }
        }

        bool bFirstLayer = true;

        // Calculate sector rectangle.
        QRect sectorRect;
        GetSectorRect(sector, sectorRect);

        // Calculate surface texture rectangle.
        QRect layerRc(QPoint(sectorRect.left() + rect.left(), sectorRect.top() + rect.top()),
                      QPoint(sectorRect.left() + rect.right(), sectorRect.top() + rect.bottom()));

        CByteImage layerMask;

        ////////////////////////////////////////////////////////////////////////
        // Generate the masks and the texture.
        ////////////////////////////////////////////////////////////////////////
        int numLayers = GetLayerCount();
        for (i = 0; i < (int)numLayers; i++)
        {
            CLayer* pLayer = GetLayer(i);

            // Skip the layer if it is not in use
            if (!pLayer->IsInUse() || !pLayer->HasTexture())
            {
                continue;
            }

            // Set the write pointer (will be incremented) for the surface data
            unsigned int* pTex =    surfaceTexture.GetData();

            uint32 layerWidth = pLayer->GetTextureWidth();
            uint32 layerHeight = pLayer->GetTextureHeight();

            if (bFirstLayer)
            {
                bFirstLayer = false;

                // Draw the first layer, without layer mask.
                // Qt rect bottom right is width, height - 1, need to add 1 when looping
                for (int y = layerRc.top(); y < (layerRc.bottom() + 1); y++)
                {
                    uint32 layerY = y & (layerHeight - 1);
                    for (int x = layerRc.left(); x < (layerRc.right() + 1); x++)
                    {
                        uint32 layerX = x & (layerWidth - 1);

                        COLORREF clr;

                        // Get the color of the tiling texture at this position
                        clr = pLayer->GetTexturePixel(layerX, layerY);

                        *pTex++ = clr;
                    }
                }
            }
            else
            {
                if (!m_layers[i].m_pLayerMask || !m_layers[i].m_pLayerMask->IsValid())
                {
                    continue;
                }

                layerMask.Attach(*m_layers[i].m_pLayerMask);

                uint32 iBlend;
                COLORREF clr;

                // Draw the current layer with layer mask.
                // Qt rect bottom right is width, height - 1, need to add 1 when looping
                for (int y = layerRc.top(); y < (layerRc.bottom() + 1); y++)
                {
                    uint32 layerY = y & (layerHeight - 1);
                    for (int x = layerRc.left(); x < (layerRc.right() + 1); x++)
                    {
                        uint32 layerX = x & (layerWidth - 1);

                        // Scale the current preview coordinate to the layer mask and get the value.
                        iBlend = layerMask.ValueAt(x, y);
                        // Check if this pixel should be drawn.
                        if (iBlend == 0)
                        {
                            pTex++;
                            continue;
                        }

                        // Get the color of the tiling texture at this position
                        clr = pLayer->GetTexturePixel(layerX, layerY);

                        // Just overdraw when the opaqueness of the new layer is maximum
                        if (iBlend == 255)
                        {
                            *pTex = clr;
                        }
                        else
                        {
                            // Blend the layer into the existing color, iBlend is the blend factor taken from the layer
                            int iBlendSrc = 255 - iBlend;
                            // WAT_EDIT
                            *pTex = RGB(((iBlendSrc * GetRValue(*pTex)) + (GetRValue(clr) * iBlend)) >> 8,
                                        ((iBlendSrc * GetGValue(*pTex)) + (GetGValue(clr) * iBlend)) >> 8,
                                        ((iBlendSrc * GetBValue(*pTex)) + (GetBValue(clr) * iBlend)) >> 8);
                        }
                        pTex++;
                    }
                }
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainLayerTexGen::GetSectorRect(const QPoint& sector, QRect& rect)
{
    rect.setTopLeft(sector * m_sectorResolution);
    rect.setSize(QSize(m_sectorResolution, m_sectorResolution));
}





//////////////////////////////////////////////////////////////////////////
void CTerrainLayerTexGen::Log(const char* format, ...)
{
    if (!m_bLog)
    {
        return;
    }

    va_list ArgList;
    char        szBuffer[1024];

    va_start(ArgList, format);
    vsprintf(szBuffer, format, ArgList);
    va_end(ArgList);

    GetIEditor()->SetStatusText(szBuffer);
    CLogFile::WriteLine(szBuffer);
}


//////////////////////////////////////////////////////////////////////////
int CTerrainLayerTexGen::GetSectorFlags(const QPoint& sector)
{
    assert(sector.x() >= 0 && sector.x() < m_numSectors && sector.y() >= 0 && sector.y() < m_numSectors);
    int index = sector.x() + sector.y() * m_numSectors;
    return m_sectorGrid[index].m_Flags;
}

//////////////////////////////////////////////////////////////////////////
CTerrainLayerTexGen::CTexSectorInfo& CTerrainLayerTexGen::GetCTexSectorInfo(const QPoint& sector)
{
    assert(sector.x() >= 0 && sector.x() < m_numSectors && sector.y() >= 0 && sector.y() < m_numSectors);
    int index = sector.x() + sector.y() * m_numSectors;
    return m_sectorGrid[index];
}

//////////////////////////////////////////////////////////////////////////
void CTerrainLayerTexGen::SetSectorFlags(const QPoint& sector, int flags)
{
    assert(sector.x() >= 0 && sector.x() < m_numSectors && sector.y() >= 0 && sector.y() < m_numSectors);
    m_sectorGrid[sector.x() + sector.y() * m_numSectors].m_Flags |= flags;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainLayerTexGen::InvalidateLayers()
{
    for (int i = 0; i < m_numSectors * m_numSectors; i++)
    {
        m_sectorGrid[i].m_Flags &= ~eSectorLayersValid;
    }
}


//////////////////////////////////////////////////////////////////////////
int CTerrainLayerTexGen::GetLayerCount() const
{
    return m_layers.size();
}

//////////////////////////////////////////////////////////////////////////
CLayer* CTerrainLayerTexGen::GetLayer(int index) const
{
    assert(index >= 0 && index < m_layers.size());
    return m_layers[index].m_pLayer;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainLayerTexGen::ClearLayers()
{
    //  for (int i = 0; i < m_layers.size(); i++)
    //  {
    //  }
    m_layers.clear();
}

//////////////////////////////////////////////////////////////////////////
CByteImage* CTerrainLayerTexGen::GetLayerMask(CLayer* layer)
{
    for (int i = 0; i < m_layers.size(); i++)
    {
        SLayerInfo& ref = m_layers[i];

        if (ref.m_pLayer == layer)
        {
            return ref.m_pLayerMask;
        }
    }
    return 0;
}


