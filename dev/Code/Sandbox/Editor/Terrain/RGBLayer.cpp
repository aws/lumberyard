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
#include "RGBLayer.h"
#include "GameEngine.h"
#include "Util/PakFile.h"
#include "ITimer.h"
#include "Layer.h"
#include "CrySizer.h"
#include "Clipboard.h"

#include "Terrain/TerrainManager.h"
#include "Util/ImagePainter.h"

#include <QMessageBox>

CRGBLayer::CRGBLayer(const char* szFilename)
    : m_TerrainRGBFileName(szFilename)
    , m_dwTileResolution(0)
    , m_dwTileCountX(0)
    , m_dwTileCountY(0)
    , m_dwCurrentTileMemory(0)
    , m_bPakOpened(false)
    , m_bInfoDirty(false)
    , m_bNextSerializeForceSizeSave(false)
    , m_NeedExportTexture(false)
{
    assert(szFilename);

    FreeData();
}

CRGBLayer::~CRGBLayer()
{
    FreeData();
}

void CRGBLayer::SetSubImageStretched(const float fSrcLeft, const float fSrcTop, const float fSrcRight, const float fSrcBottom, CImageEx& rInImage, const bool bFiltered)
{
    assert(fSrcLeft >= 0.0f && fSrcLeft <= 1.0f);
    assert(fSrcTop >= 0.0f && fSrcTop <= 1.0f);
    assert(fSrcRight >= 0.0f && fSrcRight <= 1.0f);
    assert(fSrcBottom >= 0.0f && fSrcBottom <= 1.0f);
    assert(fSrcRight >= fSrcLeft);
    assert(fSrcBottom >= fSrcTop);

    float fSrcWidth = fSrcRight - fSrcLeft;
    float fSrcHeight = fSrcBottom - fSrcTop;

    uint32 dwDestWidth = rInImage.GetWidth();
    uint32 dwDestHeight = rInImage.GetHeight();

    uint32 dwTileX1 = (uint32)(fSrcLeft * m_dwTileCountX);
    uint32 dwTileY1 = (uint32)(fSrcTop * m_dwTileCountY);
    uint32 dwTileX2 = (uint32)ceil(fSrcRight * m_dwTileCountX);
    uint32 dwTileY2 = (uint32)ceil(fSrcBottom * m_dwTileCountY);

    for (uint32 dwTileY = dwTileY1; dwTileY < dwTileY2; dwTileY++)
    {
        for (uint32 dwTileX = dwTileX1; dwTileX < dwTileX2; dwTileX++)
        {
            CTerrainTextureTiles* tile = LoadTileIfNeeded(dwTileX, dwTileY);
            assert(tile);
            assert(tile->m_pTileImage);
            uint32 dwTileSize = tile->m_pTileImage->GetWidth();

            float fSx = (-fSrcLeft + float(dwTileX) / m_dwTileCountX) / fSrcWidth;
            float fSy = (-fSrcTop + float(dwTileY) / m_dwTileCountY) / fSrcHeight;

            for (uint32 y = 0; y < dwTileSize; y++)
            {
                for (uint32 x = 0; x < dwTileSize; x++)
                {
                    float fX = float(x) / dwTileSize / m_dwTileCountX / fSrcWidth + fSx;
                    float fY = float(y) / dwTileSize / m_dwTileCountY / fSrcHeight + fSy;

                    if (0.0f <= fX && fX < 1.0f && 0.0f <= fY && fY < 1.0f)
                    {
                        uint32 dwX = (uint32)(fX * dwDestWidth);
                        uint32 dwY = (uint32)(fY * dwDestHeight);

                        uint32 dwX1 = dwX + 1;
                        uint32 dwY1 = dwY + 1;

                        if (dwX1 < dwDestWidth && dwY1 < dwDestHeight)
                        {
                            float fLerpX = fX * dwDestWidth - dwX;
                            float fLerpY = fY * dwDestHeight - dwY;

                            ColorB colS[4];

                            colS[0] = rInImage.ValueAt(dwX, dwY);
                            colS[1] = rInImage.ValueAt(dwX + 1, dwY);
                            colS[2] = rInImage.ValueAt(dwX, dwY + 1);
                            colS[3] = rInImage.ValueAt(dwX + 1, dwY + 1);

                            ColorB colTop, colBottom;

                            colTop.lerpFloat(colS[0], colS[1], fLerpX);
                            colBottom.lerpFloat(colS[2], colS[3], fLerpX);

                            ColorB colRet;

                            colRet.lerpFloat(colTop, colBottom, fLerpY);

                            tile->m_pTileImage->ValueAt(x, y) = colRet.pack_abgr8888();
                        }
                        else
                        {
                            tile->m_pTileImage->ValueAt(x, y) = rInImage.ValueAt(dwX, dwY);
                        }
                    }
                }
            }
            tile->m_bDirty = true;
        }
    }
    m_NeedExportTexture = true;
}

//////////////////////////////////////////////////////////////////////////
void CRGBLayer::GetSubImageStretched(const float fSrcLeft, const float fSrcTop, const float fSrcRight,
    const float fSrcBottom, CImageEx& rOutImage, const bool bFiltered)
{
    assert(fSrcLeft >= 0.0f && fSrcLeft <= 1.0f);
    assert(fSrcTop >= 0.0f && fSrcTop <= 1.0f);
    assert(fSrcRight >= 0.0f && fSrcRight <= 1.0f);
    assert(fSrcBottom >= 0.0f && fSrcBottom <= 1.0f);
    assert(fSrcRight >= fSrcLeft);
    assert(fSrcBottom >= fSrcTop);

    uint32 dwDestWidth = rOutImage.GetWidth();
    uint32 dwDestHeight = rOutImage.GetHeight();

    // If we're using bilinear filtering, scale our source image down by 1 pixel when filling out the destination
    // image.  The right border and bottom border on the image will then get one pixel that matches the left and 
    // top of the next subimage.
    // See also InitSectorTextureSet() in terrain_sector_tex.cpp, which maps 0 to (texture size - 1) to UVs of 0-1,
    // which accounts for that 1-pixel border on the right and bottom.
    int iBorderIncluded = bFiltered ? 1 : 0;

    float fScaleX = (fSrcRight - fSrcLeft) / (float)(dwDestWidth - iBorderIncluded);
    float fScaleY = (fSrcBottom - fSrcTop) / (float)(dwDestHeight - iBorderIncluded);

    for (uint32 dwDestY = 0; dwDestY < dwDestHeight; ++dwDestY)
    {
        float fSrcY = fSrcTop + dwDestY * fScaleY;

        if (bFiltered)  // check is not in the inner loop because of performance reasons
        {
            for (uint32 dwDestX = 0; dwDestX < dwDestWidth; ++dwDestX)
            {
                float fSrcX = fSrcLeft + dwDestX * fScaleX;

                rOutImage.ValueAt(dwDestX, dwDestY) = GetFilteredValueAt(fSrcX, fSrcY);
            }
        }
        else
        {
            for (uint32 dwDestX = 0; dwDestX < dwDestWidth; ++dwDestX)
            {
                float fSrcX = fSrcLeft + dwDestX * fScaleX;

                rOutImage.ValueAt(dwDestX, dwDestY) = GetValueAt(fSrcX, fSrcY);
            }
        }
    }
}

bool CRGBLayer::ChangeTileResolution(const uint32 dwTileX, const uint32 dwTileY, uint32 dwNewSize)
{
    assert(dwTileX < m_dwTileCountX);
    assert(dwTileY < m_dwTileCountY);

    CTerrainTextureTiles* tile = LoadTileIfNeeded(dwTileX, dwTileY);
    assert(tile);

    if (!tile)
    {
        return false;
    }

    AZ_Assert(tile->m_pTileImage, "Missing tile image, even after loading tile.");

    CImageEx newImage;

    uint32 dwOldSize = tile->m_pTileImage->GetWidth();

    if (dwNewSize < 8)
    {
        return false;
    }

    if (!newImage.Allocate(dwNewSize, dwNewSize))
    {
        return false;
    }

    float fBorderX = 0;
    float fBorderY = 0;

    // copy over with filtering
    GetSubImageStretched((float)dwTileX / (float)m_dwTileCountX + fBorderX, (float)dwTileY / (float)m_dwTileCountY + fBorderY,
        (float)(dwTileX + 1) / (float)m_dwTileCountX - fBorderX, (float)(dwTileY + 1) / (float)m_dwTileCountY - fBorderY, newImage, true);

    // Make sure to reload the tile if necessary.  It's possible for it to get evicted from memory during the call 
    // to GetSubImageStretched if we're in tight memory conditions.
    tile = LoadTileIfNeeded(dwTileX, dwTileY);
    AZ_Assert(tile->m_pTileImage, "Missing tile image, even after reloading tile.");

    // Set the tile to the new image, and adjust our memory tracking to account for the new size.
    m_dwCurrentTileMemory -= tile->m_pTileImage->GetSize();
    tile->m_pTileImage->Attach(newImage);
    tile->m_dwSize = dwNewSize;
    m_dwCurrentTileMemory += tile->m_pTileImage->GetSize();

    tile->m_bDirty = true;
    m_NeedExportTexture = true;

    return true;
}

inline void CRGBLayer::ConvertWorldToTileLookups(const double xPercent, const double yPercent, uint32& xTileIndex, uint32& yTileIndex, double& xTilePercent, double& yTilePercent)
{
    AZ_Assert(xPercent >= 0.0 && xPercent <= 1.0, "Percent value is expected to be 0.0 - 1.0 inclusive");
    AZ_Assert(yPercent >= 0.0 && yPercent <= 1.0, "Percent value is expected to be 0.0 - 1.0 inclusive");

    // Get the tile indices.  Make sure to clamp at the max valid tile value for the case that we're passed in 1.0.
    xTileIndex = AZStd::GetMin(static_cast<uint32>(xPercent * m_dwTileCountX), m_dwTileCountX - 1);
    yTileIndex = AZStd::GetMin(static_cast<uint32>(yPercent * m_dwTileCountY), m_dwTileCountY - 1);

    // Get the % offset into the tile.  Note that on the edge of the image, this can equal 1.0 due to 
    // the tile index clamping above.
    xTilePercent = (xPercent * m_dwTileCountX - xTileIndex);
    yTilePercent = (yPercent * m_dwTileCountY - yTileIndex);
}

uint32 CRGBLayer::GetValueAt(const double xPercent, const double yPercent)
{
    uint32 xTileIndex, yTileIndex;
    double xTilePercent, yTilePercent;

    ConvertWorldToTileLookups(xPercent, yPercent, xTileIndex, yTileIndex, xTilePercent, yTilePercent);

    CTerrainTextureTiles* tile = LoadTileIfNeeded(xTileIndex, yTileIndex);
    AZ_Assert(tile, "Invalid tile pointer");
    if (!tile)
    {
        return 0;
    }

    uint32 tileWidth = tile->GetWidth();
    uint32 tileHeight = tile->GetHeight();

    // Convert percentage values to integer pixel indices, clamped to the tile pixel bounds
    uint32 xTilePixel = AZStd::GetMin(static_cast<uint32>(xTilePercent * tileWidth), tileWidth - 1);
    uint32 yTilePixel = AZStd::GetMin(static_cast<uint32>(yTilePercent * tileHeight), tileHeight - 1);

    return tile->m_pTileImage->ValueAt(xTilePixel, yTilePixel);
}

uint32 CRGBLayer::GetFilteredValueAt(const double xPercent, const double yPercent)
{
    // This method uses a 2x2 sampling kernel to get an interpolated pixel value.
    ColorF pixelColor[2][2];

    uint32 xTileIndex, yTileIndex, xTilePixel, yTilePixel;
    double xTilePercent, yTilePercent;
    ConvertWorldToTileLookups(xPercent, yPercent, xTileIndex, yTileIndex, xTilePercent, yTilePercent);

    // Load the starting tile
    CTerrainTextureTiles* tile = LoadTileIfNeeded(xTileIndex, yTileIndex);
    AZ_Assert(tile, "Invalid tile pointer");
    uint32 tileWidth = tile->GetWidth();
    uint32 tileHeight = tile->GetHeight();

    // Convert tile-relative percentage values to integer pixel indices, clamped to the tile pixel bounds
    xTilePixel = AZStd::GetMin(static_cast<uint32>(xTilePercent * tileWidth), tileWidth - 1);
    yTilePixel = AZStd::GetMin(static_cast<uint32>(yTilePercent * tileHeight), tileHeight - 1);

    // The lerp factors are the fractional part of the pixel values, describing how much to move into the next pixel
    float xLerp = (xTilePercent * tileWidth) - xTilePixel; // 0..1
    float yLerp = (yTilePercent * tileHeight) - yTilePixel; // 0..1

    // To find the correct locations of each adjacent pixel, we'll increment by world percentages,
    // based on the resolution of our starting tile.  We work in percentages instead of pixel values
    // because adjacent tiles might have different resolutions, which would make pixel-based math harder. 
    double xPercentIncrement = 1.0 / (tileWidth * m_dwTileCountX);
    double yPercentIncrement = 1.0 / (tileHeight * m_dwTileCountY);

    // Sample our pixels.
    for (int y = 0; y < 2; y++)
    {
        for (int x = 0; x < 2; x++)
        {
            // Calculate the world percentage for each pixel and look up the pixel value.
            // (We use doubles instead of floats to avoid precision loss for large terrain texture resolutions)
            double worldXPercent = AZStd::GetMin(xPercent + (xPercentIncrement * static_cast<double>(x)), 1.0);
            double worldYPercent = AZStd::GetMin(yPercent + (yPercentIncrement * static_cast<double>(y)), 1.0);
            pixelColor[x][y] = GetValueAt(worldXPercent, worldYPercent);
        }
    }
    // Perform the bilinear filter of our 4 color values

    ColorF topColor, bottomColor, finalColor;

    // lerp between left and right for the top pixels
    topColor.lerpFloat(pixelColor[0][0], pixelColor[1][0], xLerp);
    // lerp between left and right for the bottom pixels
    bottomColor.lerpFloat(pixelColor[0][1], pixelColor[1][1], xLerp);

    // lerp bewteen the lerped top and bottom pixels
    finalColor.lerpFloat(topColor, bottomColor, yLerp);

    return finalColor.pack_abgr8888();
}

void CRGBLayer::FreeTile(CTerrainTextureTiles& rTile)
{
    if (rTile.m_pTileImage)
    {
        AZ_Assert(m_dwCurrentTileMemory >= rTile.m_pTileImage->GetSize(), "Terrain tile memory tracking is out of sync with tile memory usage.");
        m_dwCurrentTileMemory -= rTile.m_pTileImage->GetSize();
    }

    delete rTile.m_pTileImage;
    rTile.m_pTileImage = 0;

    rTile.m_bDirty = false;
    rTile.m_timeLastUsed = CTimeValue();
}


void CRGBLayer::ConsiderGarbageCollection()
{
    while (m_dwCurrentTileMemory > m_dwMaxTileMemory)
    {
        CTerrainTextureTiles* pOldestTile = FindOldestTileToFree();

        if (pOldestTile)
        {
            FreeTile(*pOldestTile);
        }
        else
        {
            return;
        }
    }
}

void CRGBLayer::FreeData()
{
    ClosePakForLoading();

    std::vector<CTerrainTextureTiles>::iterator it;

    for (it = m_TerrainTextureTiles.begin(); it != m_TerrainTextureTiles.end(); ++it)
    {
        CTerrainTextureTiles& ref = *it;

        FreeTile(ref);
    }

    AZ_Assert(m_dwCurrentTileMemory == 0, "Non-zero terrain tile memory, a leak has occurred.");

    m_TerrainTextureTiles.clear();

    m_dwTileCountX = 0;
    m_dwTileCountY = 0;
    m_dwTileResolution = 0;
    m_dwCurrentTileMemory = 0;
}


CRGBLayer::CTerrainTextureTiles* CRGBLayer::FindOldestTileToFree()
{
    std::vector<CTerrainTextureTiles>::iterator it;
    uint32 dwI = 0;

    CTerrainTextureTiles* pRet = 0;

    for (it = m_TerrainTextureTiles.begin(); it != m_TerrainTextureTiles.end(); ++it, ++dwI)
    {
        CTerrainTextureTiles& ref = *it;

        if (ref.m_pTileImage)
        {
            if (!ref.m_bDirty)
            {
                if (pRet == 0 || ref.m_timeLastUsed < pRet->m_timeLastUsed)
                {
                    pRet = &ref;
                }
            }
        }
    }

    return pRet;
}

bool CRGBLayer::OpenPakForLoading()
{
    if (m_bPakOpened)
    {
        return true;
    }

    m_pakFilename = GetFullFileName();

    ICryPak* pIPak = GetIEditor()->GetSystem()->GetIPak();
    m_bPakOpened = pIPak->OpenPack(m_pakFilename.toUtf8().data());

    // if the pak file wasn't created yet
    if (!m_bPakOpened)
    {
        // create PAK file so we don't get errors when loading
        SaveAndFreeMemory(true);

        m_bPakOpened = pIPak->OpenPack(m_pakFilename.toUtf8().data());
    }

    return m_bPakOpened;
}


void CRGBLayer::ClosePakForLoading()
{
    if (!GetIEditor() || !GetIEditor()->GetSystem() || m_pakFilename.isEmpty())
    {
        return;
    }

    ICryPak* pIPak = GetIEditor()->GetSystem()->GetIPak();

    m_bPakOpened = !pIPak->ClosePack(m_pakFilename.toUtf8().data());

    assert(!m_bPakOpened);
}


CRGBLayer::CTerrainTextureTiles* CRGBLayer::GetTilePtr(const uint32 dwTileX, const uint32 dwTileY)
{
    assert(dwTileX < m_dwTileCountX);
    assert(dwTileY < m_dwTileCountY);

    if (dwTileX >= m_dwTileCountX || dwTileY >= m_dwTileCountY)
    {
        return 0;
    }

    return &m_TerrainTextureTiles[dwTileX + dwTileY * m_dwTileCountX];
}

namespace
{
    CryCriticalSection csLoadTileIfNeeded;
}

CRGBLayer::CTerrainTextureTiles* CRGBLayer::LoadTileIfNeeded(const uint32 dwTileX, const uint32 dwTileY, bool bNoGarbageCollection)
{
    CTerrainTextureTiles* pTile = GetTilePtr(dwTileX, dwTileY);

    if (!pTile)
    {
        return 0;
    }

    if (!pTile->m_pTileImage)
    {
        AUTO_LOCK(csLoadTileIfNeeded);

        pTile->m_timeLastUsed = GetIEditor()->GetSystem()->GetITimer()->GetAsyncCurTime();

        if (!pTile->m_pTileImage)
        {
            CLogFile::FormatLine("Loading RGB Layer Tile: TilePos=(%d, %d) MemUsage=%.1fMB", dwTileX, dwTileY, float(m_dwCurrentTileMemory) / 1024.f / 1024.f);     // debugging

            if (!bNoGarbageCollection)
            {
                ConsiderGarbageCollection();
            }

            QString pathRel = GetIEditor()->GetGameEngine()->GetLevelPath();

            CImageEx* pTileImage = pTile->m_pTileImage;

            if (OpenPakForLoading())
            {
                CMemoryBlock mem;
                uint32 dwWidth = 0, dwHeight = 0;
                uint8* pSrc8 = 0;
                int bpp = 3;

                CCryFile file;

                char szTileName[128];

                sprintf_s(szTileName, "Tile%d_%d.raw", dwTileX, dwTileY);

                if (file.Open((Path::AddPathSlash(pathRel) + szTileName).toUtf8().data(), "rb"))
                {
                    if (file.ReadType(&dwWidth)
                        && file.ReadType(&dwHeight)
                        && dwWidth > 0 && dwHeight > 0)
                    {
                        assert(dwWidth == dwHeight);

                        if (mem.Allocate(dwWidth * dwHeight * bpp))
                        {
                            if (!file.ReadRaw(mem.GetBuffer(), dwWidth * dwHeight * bpp))
                            {
                                CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Error reading tile raw data from %s!", szTileName);
                                dwWidth = 0;
                            }
                            pSrc8 = (uint8*)mem.GetBuffer();
                        }
                        else
                        {
                            // error
                            assert(0);
                        }
                    }
                    else
                    {
                        // error
                        assert(0);
                    }

                    file.Close();
                }

                if (pSrc8 && dwWidth > 0 && dwHeight > 0)
                {
                    if (!pTileImage)
                    {
                        pTileImage = new CImageEx();
                    }

                    pTile->m_dwSize = dwWidth;

                    if (pTileImage->Allocate(dwWidth, dwHeight))
                    {
                        uint8* pDst8 = (uint8*)pTileImage->GetData();

                        for (uint32 dwI = 0; dwI < dwWidth * dwHeight; ++dwI)
                        {
                            *pDst8++ = *pSrc8++;
                            *pDst8++ = *pSrc8++;
                            *pDst8++ = *pSrc8++;
                            *pDst8++ = 0;
                        }
                    }
                    else
                    {
                        // error
                        assert(0);
                    }
                }
            }

            // still not there - then create an empty tile
            if (!pTileImage)
            {
                pTileImage = new CImageEx();

                pTile->m_dwSize = m_dwTileResolution;

                pTileImage->Allocate(m_dwTileResolution, m_dwTileResolution);
                pTileImage->Fill(0xff);

                // for more convenience in the beginning:
                CLayer* pLayer = GetIEditor()->GetTerrainManager()->GetLayer(0);

                if (pLayer)
                {
                    pLayer->PrecacheTexture();

                    CImagePainter painter;

                    uint32 dwTileSize = pTileImage->GetWidth();
                    uint32 dwTileSizeReduced = dwTileSize - 1;
                    uint32 dwTileOffsetX = dwTileX * dwTileSizeReduced;
                    uint32 dwTileOffsetY = dwTileY * dwTileSizeReduced;

                    painter.FillWithPattern(*pTileImage, dwTileOffsetX, dwTileOffsetY, pLayer->m_texture);
                }
            }

            pTile->m_bDirty = false;

            pTile->m_pTileImage = pTileImage;
            m_dwCurrentTileMemory += pTile->m_pTileImage->GetSize();
        }
    }

    return pTile;
}

bool CRGBLayer::IsDirty() const
{
    if (m_bInfoDirty)
    {
        return true;
    }

    std::vector<CTerrainTextureTiles>::const_iterator it;

    for (it = m_TerrainTextureTiles.begin(); it != m_TerrainTextureTiles.end(); ++it)
    {
        const CTerrainTextureTiles& ref = *it;

        if (ref.m_pTileImage)
        {
            if (ref.m_bDirty)
            {
                return true;
            }
        }
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////
QString CRGBLayer::GetFullFileName() const
{
    QString pathRel = GetIEditor()->GetGameEngine()->GetLevelPath();
    QString pathPak = Path::AddSlash(pathRel) + m_TerrainRGBFileName;

    return pathPak;
}


//////////////////////////////////////////////////////////////////////////
bool CRGBLayer::WouldSaveSucceed()
{
    ClosePakForLoading();

    CPakFile pakFile;

    if (!IsDirty())
    {
        return true;
    }

    if (!CFileUtil::OverwriteFile(GetFullFileName()))
    {
        return false;
    }

    // create pak file
    if (!pakFile.Open(GetFullFileName().toUtf8().data()))
    {
        return false;
    }

    pakFile.Close();

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CRGBLayer::Resize(uint32 dwTileCountX, uint32 dwTileCountY, uint32 dwTileResolution)
{
    CImageEx** pImages = new CImageEx*[dwTileCountX * dwTileCountY];
    uint32 x, y;
    for (y = 0; y < dwTileCountY; ++y)
    {
        for (x = 0; x < dwTileCountX; ++x)
        {
            CImageEx*& pImg = pImages[y * dwTileCountX + x];
            pImg = new CImageEx();
            pImg->Allocate(dwTileResolution, dwTileResolution);
            float l = ((float)x) / dwTileCountX;
            float t = ((float)y) / dwTileCountY;
            float r = ((float)x + 1) / dwTileCountX;
            float b = ((float)y + 1) / dwTileCountY;
            GetSubImageStretched(l, t, r, b, *pImg);
        }
    }
    FreeData();
    AllocateTiles(dwTileCountX, dwTileCountY, dwTileResolution);
    for (y = 0; y < dwTileCountY; ++y)
    {
        for (x = 0; x < dwTileCountX; ++x)
        {
            CTerrainTextureTiles* pTile = GetTilePtr(x, y);
            pTile->m_pTileImage = pImages[y * dwTileCountX + x];
            pTile->m_dwSize = dwTileResolution;
            pTile->m_bDirty = true;
        }
    }
    m_NeedExportTexture = true;
    delete[] pImages;
}

//////////////////////////////////////////////////////////////////////////
void CRGBLayer::CleanupCache()
{
    ClosePakForLoading();

    std::vector<CTerrainTextureTiles>::iterator it;

    for (it = m_TerrainTextureTiles.begin(); it != m_TerrainTextureTiles.end(); ++it)
    {
        CTerrainTextureTiles& ref = *it;

        if (ref.m_pTileImage)
        {
            if (!ref.m_bDirty)
            {
                FreeTile(ref);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CRGBLayer::SaveAndFreeMemory(const bool bForceFileCreation)
{
    ClosePakForLoading();

    assert(WouldSaveSucceed());

    if (!bForceFileCreation && !IsDirty())
    {
        return true;
    }

    // create pak file
    CPakFile pakFile;
    if (!pakFile.Open(GetFullFileName().toUtf8().data()))
    {
        // error
        assert(0);
        return false;
    }

    std::vector<CTerrainTextureTiles>::iterator it;
    uint32 dwI = 0;

    for (it = m_TerrainTextureTiles.begin(); it != m_TerrainTextureTiles.end(); ++it, ++dwI)
    {
        CTerrainTextureTiles& ref = *it;

        uint32 dwTileX = dwI % m_dwTileCountX;
        uint32 dwTileY = dwI / m_dwTileCountX;

        if (ref.m_pTileImage)
        {
            if (ref.m_bDirty)
            {
                CMemoryBlock mem;
                ExportTile(mem, dwTileX, dwTileY, false);

                char szTileName[20];

                sprintf_s(szTileName, "Tile%d_%d.raw", dwTileX, dwTileY);

                if (!pakFile.UpdateFile((Path::AddPathSlash(GetIEditor()->GetGameEngine()->GetLevelPath()) + szTileName).toUtf8().data(), mem))
                {
                    // error
                    assert(0);
                }
            }

            // free tile
            FreeTile(ref);
        }
    }

    AZ_Assert(m_dwCurrentTileMemory == 0, "Terrain tile memory is non-zero, a memory leak has occurred.");

    pakFile.Close();

    return true;
}

void CRGBLayer::ClipTileRect(QRect& inoutRect) const
{
    if ((int)(inoutRect.left()) < 0)
    {
        inoutRect.setLeft(0);
    }

    if ((int)(inoutRect.top()) < 0)
    {
        inoutRect.setTop(0);
    }

    // Qt rect bottom right is width, height - 1, need to add 1
    if ((int)(inoutRect.right() + 1) > m_dwTileCountX)
    {
        inoutRect.setRight(m_dwTileCountX - 1);
    }

    // Qt rect bottom right is width, height - 1, need to add 1
    if ((int)(inoutRect.bottom() + 1) > m_dwTileCountY)
    {
        inoutRect.setBottom(m_dwTileCountY - 1);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRGBLayer::PaintBrushWithPatternTiled(const float fpx, const float fpy, SEditorPaintBrush& brush, const CImageEx& imgPattern)
{
    assert(fpx >= 0.0f && fpx <= 1.0f);
    assert(fpy >= 0.0f && fpy <= 1.0f);
    assert(brush.fRadius >= 0.0f && brush.fRadius <= 1.0f);

    float fOldRadius = brush.fRadius;

    uint32 dwMaxRes = CalcMaxLocalResolution(0, 0, 1, 1);

    QRect recTiles = QRect(
            QPoint((uint32)floor((fpx - brush.fRadius - 2.0f / dwMaxRes) * m_dwTileCountX),
                (uint32)floor((fpy - brush.fRadius - 2.0f / dwMaxRes) * m_dwTileCountY)),
            QPoint((uint32)ceil((fpx + brush.fRadius + 2.0f / dwMaxRes) * m_dwTileCountX),
                (uint32)ceil((fpy + brush.fRadius + 2.0f / dwMaxRes) * m_dwTileCountY)) - QPoint(1, 1));

    ClipTileRect(recTiles);

    CImagePainter painter;

    // In Qt, the rect's bottom and right values are inclusive, not exclusive, which is why we use
    // <= in the loops instead of just <
    for (uint32 dwTileY = recTiles.top(); dwTileY <= recTiles.bottom(); ++dwTileY)
    {
        for (uint32 dwTileX = recTiles.left(); dwTileX <= recTiles.right(); ++dwTileX)
        {
            CTerrainTextureTiles* tile = LoadTileIfNeeded(dwTileX, dwTileY);
            assert(tile);

            assert(tile->m_pTileImage);

            tile->m_bDirty = true;

            uint32 tileWidth = tile->m_pTileImage->GetWidth();
            uint32 tileHeight = tile->m_pTileImage->GetHeight();
            uint32 dwTileOffsetX = dwTileX * tileWidth;
            uint32 dwTileOffsetY = dwTileY * tileHeight;

            float fScaleX = (tileWidth * m_dwTileCountX), fScaleY = (tileHeight * m_dwTileCountY);

            // NOTE: If tile width is ever different from tile height, we would need to revisit the logic
            // for how to calculate the brush radius
            AZ_Assert(tileWidth == tileHeight, "Tile width and height are different.  If this is valid, the RGB layer painting logic will need to be adjusted.");
            brush.fRadius = fOldRadius * (tileWidth * m_dwTileCountX);
            painter.PaintBrushWithPattern(fpx, fpy, *tile->m_pTileImage, dwTileOffsetX, dwTileOffsetY, fScaleX, fScaleY, brush, imgPattern);
        }
    }

    brush.fRadius = fOldRadius;
    m_NeedExportTexture = true;
}


uint32 CRGBLayer::GetTileResolution(const uint32 dwTileX, const uint32 dwTileY)
{
    CTerrainTextureTiles* tile = GetTilePtr(dwTileX, dwTileY);
    assert(tile);

    if (!tile->m_dwSize)        // not size info yet - load the tile
    {
        tile = LoadTileIfNeeded(dwTileX, dwTileY);
        assert(tile);

        m_bInfoDirty = true;        // save is required to update the dwSizee

        assert(tile->m_pTileImage);
    }

    assert(tile->m_dwSize);

    return tile->m_dwSize;
}


uint32 CRGBLayer::CalcMaxLocalResolution(const float fSrcLeft, const float fSrcTop, const float fSrcRight, const float fSrcBottom)
{
    assert(fSrcRight >= fSrcLeft);
    assert(fSrcBottom >= fSrcTop);

    // Qt rect bottom right is width, height - 1, need to subtract 1 when setting bottomRight in the constructor to QRect
    QRect recTiles = QRect(QPoint((uint32)floor(fSrcLeft * m_dwTileCountX), (uint32)floor(fSrcTop * m_dwTileCountY)),
            QPoint((uint32)ceil(fSrcRight * m_dwTileCountX), (uint32)ceil(fSrcBottom * m_dwTileCountY)) - QPoint(1, 1));

    ClipTileRect(recTiles);

    uint32 dwRet = 0;

    // Qt rect bottom right is width, height - 1, need to add 1
    for (uint32 dwTileY = recTiles.top(); dwTileY < (recTiles.bottom() + 1); ++dwTileY)
    {
        for (uint32 dwTileX = recTiles.left(); dwTileX < (recTiles.right() + 1); ++dwTileX)
        {
            uint32 dwSize = GetTileResolution(dwTileX, dwTileY);
            assert(dwSize);

            uint32 dwLocalWidth = dwSize * m_dwTileCountX;
            uint32 dwLocalHeight = dwSize * m_dwTileCountY;

            if (dwRet < dwLocalWidth)
            {
                dwRet = dwLocalWidth;
            }
            if (dwRet < dwLocalHeight)
            {
                dwRet = dwLocalHeight;
            }
        }
    }

    return dwRet;
}

void CRGBLayer::Serialize(XmlNodeRef& node, bool bLoading)
{
    if (bLoading)
    {
        m_dwTileCountX = m_dwTileCountY = m_dwTileResolution = 0;

        // Texture
        node->getAttr("TileCountX", m_dwTileCountX);
        node->getAttr("TileCountY", m_dwTileCountY);
        node->getAttr("TileResolution", m_dwTileResolution);

        if (m_dwTileCountX && m_dwTileCountY && m_dwTileResolution)                 // if info is valid
        {
            AllocateTiles(m_dwTileCountX, m_dwTileCountY, m_dwTileResolution);
        }
        else
        {
            FreeData();
        }

        XmlNodeRef mainNode = node->findChild("RGBLayer");

        if (mainNode)       // old nodes might not have this info
        {
            XmlNodeRef tiles = mainNode->findChild("Tiles");
            if (tiles)
            {
                int numObjects = tiles->getChildCount();

                for (int i = 0; i < numObjects; i++)
                {
                    XmlNodeRef tile = tiles->getChild(i);

                    uint32 dwX = 0, dwY = 0, dwSize = 0;

                    tile->getAttr("X", dwX);
                    tile->getAttr("Y", dwY);
                    tile->getAttr("Size", dwSize);

                    CTerrainTextureTiles* pPtr = GetTilePtr(dwX, dwY);
                    assert(pPtr);

                    if (pPtr)
                    {
                        pPtr->m_dwSize = dwSize;
                    }
                }
            }
        }

        m_pakFilename = GetFullFileName();
    }
    else
    {
        // Storing
        node = XmlHelpers::CreateXmlNode("TerrainTexture");

        // Texture
        node->setAttr("TileCountX", m_dwTileCountX);
        node->setAttr("TileCountY", m_dwTileCountY);
        node->setAttr("TileResolution", m_dwTileResolution);

        SaveAndFreeMemory();

        XmlNodeRef mainNode = node->newChild("RGBLayer");
        XmlNodeRef tiles = mainNode->newChild("Tiles");

        for (uint32 dwY = 0; dwY < m_dwTileCountY; ++dwY)
        {
            for (uint32 dwX = 0; dwX < m_dwTileCountX; ++dwX)
            {
                XmlNodeRef obj = tiles->newChild("tile");

                CTerrainTextureTiles* pPtr = GetTilePtr(dwX, dwY);
                assert(pPtr);

                uint32 dwSize = pPtr->m_dwSize;

                if (dwSize || m_bNextSerializeForceSizeSave)
                {
                    obj->setAttr("X", dwX);
                    obj->setAttr("Y", dwY);
                    obj->setAttr("Size", dwSize);
                }
            }
        }

        m_bNextSerializeForceSizeSave = false;

        m_bInfoDirty = false;
    }
}

void CRGBLayer::AllocateTiles(const uint32 dwTileCountX, const uint32 dwTileCountY, const uint32 dwTileResolution)
{
    assert(dwTileCountX);
    assert(dwTileCountY);

    // free
    FreeData();
    m_TerrainTextureTiles.resize(dwTileCountX * dwTileCountY);

    m_dwTileCountX = dwTileCountX;
    m_dwTileCountY = dwTileCountY;
    m_dwTileResolution = dwTileResolution;

    CLogFile::FormatLine("CRGBLayer::AllocateTiles %dx%d tiles %d => %dx%d texture", dwTileCountX, dwTileCountY, dwTileResolution, dwTileCountX * dwTileResolution, dwTileCountY * dwTileResolution);       // debugging
}

//////////////////////////////////////////////////////////////////////////
void CRGBLayer::GetMemoryUsage(ICrySizer* pSizer)
{
    std::vector<CTerrainTextureTiles>::iterator it;

    for (it = m_TerrainTextureTiles.begin(); it != m_TerrainTextureTiles.end(); ++it)
    {
        CTerrainTextureTiles& ref = *it;

        if (ref.m_pTileImage)                                   // something to free
        {
            pSizer->Add((char*)ref.m_pTileImage->GetData(), ref.m_pTileImage->GetSize());
        }
    }

    pSizer->Add(m_TerrainTextureTiles);

    pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////
uint32 CRGBLayer::CalcMinRequiredTextureExtend()
{
    uint32 dwMaxLocalExtend = 0;

    for (uint32 dwTileY = 0; dwTileY < m_dwTileCountY; ++dwTileY)
    {
        for (uint32 dwTileX = 0; dwTileX < m_dwTileCountX; ++dwTileX)
        {
            dwMaxLocalExtend = max(dwMaxLocalExtend, GetTileResolution(dwTileX, dwTileY));
        }
    }

    return max(m_dwTileCountX, m_dwTileCountY) * dwMaxLocalExtend;
}

//////////////////////////////////////////////////////////////////////////
bool CRGBLayer::RefineTiles()
{
    CRGBLayer out("TerrainTexture2.pak");

    assert(m_dwTileCountX);
    assert(m_dwTileCountY);
    assert(m_dwTileResolution / 2);

    out.AllocateTiles(m_dwTileCountX * 2, m_dwTileCountY * 2, m_dwTileResolution / 2);

    std::vector<CTerrainTextureTiles>::iterator it, end = m_TerrainTextureTiles.end();
    uint32 dwI = 0;

    for (it = m_TerrainTextureTiles.begin(); it != end; ++it, ++dwI)
    {
        CTerrainTextureTiles& ref = *it;

        uint32 dwTileX = dwI % m_dwTileCountX;
        uint32 dwTileY = dwI / m_dwTileCountY;

        LoadTileIfNeeded(dwTileX, dwTileY);

        assert(ref.m_dwSize);

        for (uint32 dwY = 0; dwY < 2; ++dwY)
        {
            for (uint32 dwX = 0; dwX < 2; ++dwX)
            {
                CTerrainTextureTiles* pOutTile = out.GetTilePtr(dwTileX * 2 + dwX, dwTileY * 2 + dwY);
                assert(pOutTile);

                pOutTile->m_dwSize = ref.m_dwSize;
                pOutTile->m_pTileImage = new CImageEx();
                pOutTile->m_pTileImage->Allocate(pOutTile->m_dwSize, pOutTile->m_dwSize);
                pOutTile->m_timeLastUsed = GetIEditor()->GetSystem()->GetITimer()->GetAsyncCurTime();
                pOutTile->m_bDirty = true;
                out.m_dwCurrentTileMemory += pOutTile->m_pTileImage->GetSize();
                m_bInfoDirty = true;

                float fSrcLeft = (float)(dwTileX * 2 + dwX) / (float)(m_dwTileCountX * 2);
                float fSrcTop = (float)(dwTileY * 2 + dwY) / (float)(m_dwTileCountY * 2);
                float fSrcRight = (float)(dwTileX * 2 + dwX + 1) / (float)(m_dwTileCountX * 2);
                float fSrcBottom = (float)(dwTileY * 2 + dwY + 1) / (float)(m_dwTileCountY * 2);

                GetSubImageStretched(fSrcLeft, fSrcTop, fSrcRight, fSrcBottom, *pOutTile->m_pTileImage, false);
            }
        }

        if (!out.SaveAndFreeMemory(true))
        {
            assert(0);
            return false;
        }
    }

    ClosePakForLoading();

    QString path = GetFullFileName();
    QString path2 = out.GetFullFileName();

    QFile::rename(path, PathUtil::ReplaceExtension(path.toUtf8().data(), "bak").c_str());
    QFile::rename(path2, path);

    AllocateTiles(m_dwTileCountX * 2, m_dwTileCountY * 2, m_dwTileResolution);
    m_bNextSerializeForceSizeSave = true;
    m_NeedExportTexture = true;

    return true;
}

bool CRGBLayer::TryImportTileRect(
    const AZStd::string& filename,
    uint32_t left,
    uint32_t top,
    uint32_t right,
    uint32_t bottom)
{
    if (left >= right || top >= bottom)
    {
        AZ_Assert(false, "Bad rect passed to TryImportTileRect");
        return false;
    }

    SaveAndFreeMemory(true);

    CImageEx srcImage;
    if (!filename.empty())
    {
        CImageEx newImage;
        if (!CImageUtil::LoadBmp(filename.c_str(), newImage))
        {
            QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Error: Can't load BMP file. Probably out of memory."));
            return false;
        }
        // Rotating by 270-degrees to match loaded PGM heightmap and orientation of the image in content creation tools
        srcImage.RotateOrt(newImage, ImageRotationDegrees::Rotate270);
        srcImage.SwapRedAndBlue(); // Swap loaded image to BGR format.
    }
    else
    {
        CClipboard cb(nullptr);
        if (!cb.GetImage(srcImage))
        {
            return false;
        }
    }

    const uint32_t tileCountX = GetTileCountX();
    const uint32_t tileCountY = GetTileCountY();

    SetSubImageStretched(
        (float)left / tileCountX,
        (float)top / tileCountY,
        (float)right / tileCountX,
        (float)bottom / tileCountY,
        srcImage,
        true);

    return true;
}

void CRGBLayer::ExportTileRect(
    const AZStd::string& filename,
    uint32_t left,
    uint32_t top,
    uint32_t right,
    uint32_t bottom)
{
    const uint32_t tileCountX = GetTileCountX();
    const uint32_t tileCountY = GetTileCountY();

    if (left >= right || top >= bottom)
    {
        AZ_Assert(false, "Bad rect passed to ExportTileRect");
        return;
    }

    // [Min, Max] are inclusive. Add 1 to get size.
    const uint32_t tileBoxCountX = right - left;
    const uint32_t tileBoxCountY = bottom - top;
    const uint32_t tileBoxCountTotal = tileBoxCountX * tileBoxCountY;

    // Compute maximum resolution of all tiles.
    uint32 tileResolutionMax = 0;
    for (uint32_t x = left; x < right; x++)
    {
        for (uint32_t y = top; y < bottom; y++)
        {
            tileResolutionMax = max(GetTileResolution(x, y), tileResolutionMax);
        }
    }

    CImageEx dstImage;
    dstImage.Allocate(tileBoxCountX * tileResolutionMax, tileBoxCountY * tileResolutionMax);

    CImageEx stagingImage;
    stagingImage.Allocate(tileResolutionMax, tileResolutionMax);

    uint32_t currentTileIndex = 0;
    CWaitProgress progress("Export Terrain Surface Texture");
    for (uint32_t tileX = left; tileX < right; tileX++)
    {
        const uint32_t localTileX = tileX - left;

        for (uint32_t tileY = top; tileY < bottom; tileY++)
        {
            const uint32_t localTileY = tileY - top;

            // update progress bar
            progress.Step((100 * currentTileIndex) / tileBoxCountTotal);
            currentTileIndex++;

            CTerrainTextureTiles* tile = LoadTileIfNeeded(tileX, tileY);
            if (!tile)
            {
                continue;
            }
            AZ_Assert(tile->m_pTileImage, "Tile image is null");
            CImageEx* tileImage = tile->m_pTileImage;

            const bool bStagedExport = GetTileResolution(tileX, tileY) != tileResolutionMax;
            if (bStagedExport)
            {
                tileImage = &stagingImage;

                GetSubImageStretched(
                    (float)tileX / tileCountX,
                    (float)tileY / tileCountY,
                    (float)(tileX + 1) / tileCountX,
                    (float)(tileY + 1) / tileCountY,
                    *tileImage,
                    true);
            }

            for (uint32_t x = 0; x < tileResolutionMax; x++)
            {
                const uint32_t localImageX = localTileX * tileResolutionMax + x;

                for (uint32_t y = 0; y < tileResolutionMax; y++)
                {
                    const uint32_t localImageY = localTileY * tileResolutionMax + y;

                    dstImage.ValueAt(localImageX, localImageY) = tileImage->ValueAt(x, y);
                }
            }
        }
    }

    if (!filename.empty())
    {
        dstImage.SwapRedAndBlue(); // swap image back to RGB before saving.
        CImageEx newImage;
        newImage.RotateOrt(dstImage, ImageRotationDegrees::Rotate90);
        CImageUtil::SaveBitmap(filename.c_str(), newImage);
    }
    else
    {
        CClipboard cb(nullptr);
        cb.PutImage(dstImage);
    }
}

bool CRGBLayer::ExportTile(CMemoryBlock& mem, uint32 dwTileX, uint32 dwTileY, bool bCompress)
{
    CTerrainTextureTiles* pTile = LoadTileIfNeeded(dwTileX, dwTileY);
    if (!pTile || !pTile->m_pTileImage)
    {
        assert(0);
        return false;
    }

    uint32 dwWidth = pTile->m_pTileImage->GetWidth(), dwHeight = pTile->m_pTileImage->GetHeight();

    assert(dwWidth);
    assert(dwHeight);

    CMemoryBlock tmpMem;
    CMemoryBlock& tmp = bCompress ? tmpMem : mem;

    const int bpp = 3;
    tmp.Allocate(sizeof(uint32) + sizeof(uint32) + dwWidth * dwHeight * bpp);

    uint32* pMem32 = (uint32*)tmp.GetBuffer();
    uint8* pDst8 = (uint8*)tmp.GetBuffer() + sizeof(uint32) + sizeof(uint32);
    uint8* pSrc8 = (uint8*)pTile->m_pTileImage->GetData();

    pMem32[0] = dwWidth;
    pMem32[1] = dwHeight;

    for (uint32 j = 0; j < dwWidth * dwHeight; ++j)
    {
        *pDst8++ = *pSrc8++;
        *pDst8++ = *pSrc8++;
        *pDst8++ = *pSrc8++;
        pSrc8++;
    }

    if (bCompress)
    {
        tmp.Compress(mem);
    }

    return true;
}

void CRGBLayer::ApplyColorMultiply(float colorMultiply)
{

    for (uint32 dwTileY = 0; dwTileY < m_dwTileCountY; ++dwTileY)
    {
        for (uint32 dwTileX = 0; dwTileX <m_dwTileCountX; ++dwTileX)
        {
            CTerrainTextureTiles* tile = LoadTileIfNeeded(dwTileX, dwTileY);
            assert(tile);
            assert(tile->m_pTileImage);
            tile->m_bDirty = true;

            const uint tileWidth = tile->m_pTileImage->GetWidth();
            const uint tileHeight = tile->m_pTileImage->GetHeight();

            for (uint32 y = 0; y < tileWidth; y++)
            {
                for (uint32 x = 0; x < tileHeight; x++)
                {
                    ColorB color = tile->m_pTileImage->ValueAt(x, y);
                    color.r *= colorMultiply;
                    color.g *= colorMultiply;
                    color.b *= colorMultiply;
                    tile->m_pTileImage->ValueAt(x, y) = color.pack_rgb888();
                }
            }
        }
    }

}
