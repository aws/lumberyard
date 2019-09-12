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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_RGBLAYER_H
#define CRYINCLUDE_EDITOR_TERRAIN_RGBLAYER_H
#pragma once

#include <AzCore/std/string/string.h>

struct SEditorPaintBrush;

// RGB terrain texture layer
// internal structure (tiled based, loading tiles on demand) is hidden from outside
// texture does not have to be square
// cache uses "last recently used" strategy to remain within set memory bounds (during terrain texture generation)
// Data is actually stored as BGR - the video card uses this format natively, but imports / exports must be converted.
class CRGBLayer
{
public:
    CRGBLayer(const char* szFilename);
    virtual ~CRGBLayer();

    void Serialize(XmlNodeRef& node, bool bLoading);

    void AllocateTiles(const uint32 dwTileCountX, const uint32 dwTileCountY, const uint32 dwTileResolution);

    void FreeData();

    // calculated the texture resolution needed to capture all details in the texture
    uint32 CalcMinRequiredTextureExtend();

    uint32 GetTileCountX() const { return m_dwTileCountX; }
    uint32 GetTileCountY() const { return m_dwTileCountY; }

    uint32 GetTileResolution(const uint32 dwTileX, const uint32 dwTileY);

    bool ChangeTileResolution(const uint32 dwTileX, const uint32 dwTileY, uint32 dwNewSize);

    void SetSubImageStretched(const float fSrcLeft, const float fSrcTop, const float fSrcRight, const float fSrcBottom, CImageEx& rInImage, const bool bFiltered = false);

    // stretched (dwSrcWidth,dwSrcHeight)->rOutImage
    // Arguments:
    //   fSrcLeft - 0..1
    //   fSrcTop - 0..1
    //   fSrcRight - 0..1, <fSrcLeft
    //   fSrcBottom - 0..1, <fSrcTop
    //  Notice: is build as follows: blue | green << 8 | red << 16, alpha unused
    void GetSubImageStretched(const float fSrcLeft, const float fSrcTop, const float fSrcRight, const float fSrcBottom, CImageEx& rOutImage, const bool bFiltered = false);

    // Paint spot with pattern (to an RGB image)
    // Arguments:
    //   fpx - 0..1 in the texture
    //   fpy - 0..1 in the texture
    void PaintBrushWithPatternTiled(const float fpx, const float fpy, SEditorPaintBrush& brush, const CImageEx& imgPattern);

    // needed for Layer Painting
    // Arguments:
    //   fSrcLeft - usual range: [0..1] but full range is valid
    //   fSrcTop - usual range: [0..1] but full range is valid
    //   fSrcRight - usual range: [0..1] but full range is valid
    //   fSrcBottom - usual range: [0..1] but full range is valid
    // Return:
    //   might be 0 if no tile was touched
    uint32 CalcMaxLocalResolution(const float fSrcLeft, const float fSrcTop, const float fSrcRight, const float fSrcBottom);

    bool TryImportTileRect(
        const AZStd::string& filename,
        uint32_t left,
        uint32_t top,
        uint32_t right,
        uint32_t bottom);

    void ExportTileRect(
        const AZStd::string& filename,
        uint32_t left,
        uint32_t top,
        uint32_t right,
        uint32_t bottom);

    bool ExportTile(CMemoryBlock& mem, uint32 dwTileX, uint32 dwTileY, bool bCompress = true);

    void GetMemoryUsage(ICrySizer* pSizer);

    bool WouldSaveSucceed();

    void Resize(uint32 dwTileCountX, uint32 dwTileCountY, uint32 dwTileResolution);

    void CleanupCache();

    bool RefineTiles();

    uint32 GetFilteredValueAt(const double fpx, const double fpy);

    uint32 GetValueAt(const double fpx, const double fpy);

    bool GetNeedExportTexture() const
    {
        return m_NeedExportTexture;
    }
    void SetNeedExportTexture(bool needExport)
    {
        m_NeedExportTexture = needExport;
    }
    void ClosePakForLoading();

    QString GetFullFileName() const;

    void ApplyColorMultiply(float colorMultiply);

private:

    class CTerrainTextureTiles
    {
    public:
        // default constructor
        CTerrainTextureTiles()
            : m_pTileImage(0)
            , m_bDirty(false)
            , m_dwSize(0)
        {
        }

        CImageEx*          m_pTileImage;                // 0 if not loaded, otherwise pointer to the image (m_dwTileResolution,m_dwTileResolution)
        CTimeValue      m_timeLastUsed;         // needed for "last recently used" strategy
        bool                    m_bDirty;                       // true=tile needs to be written to disk, needed for "last recently used" strategy
        uint32              m_dwSize;                       // only valid if m_dwSize!=0, if not valid you need to call LoadTileIfNeeded()

        uint32 GetWidth()  
        { 
            AZ_Assert(m_pTileImage, "Invalid tile image");
            return m_pTileImage ? m_pTileImage->GetWidth() : 0;
        }

        uint32 GetHeight() 
        { 
            AZ_Assert(m_pTileImage, "Invalid tile image");
            return m_pTileImage ? m_pTileImage->GetHeight() : 0;
        }

    };

    std::vector<CTerrainTextureTiles> m_TerrainTextureTiles; // [x+y*m_dwTileCountX]

    uint32 m_dwTileCountX;
    uint32 m_dwTileCountY;
    uint32 m_dwTileResolution; // must be power of two, tiles are square in size
    uint32 m_dwCurrentTileMemory; // to detect if GarbageCollect is needed
    bool   m_bPakOpened; // to optimize redundant OpenPak an ClosePak
    bool   m_bInfoDirty; // true=save needed e.g. internal properties changed
    bool   m_bNextSerializeForceSizeSave;
    bool   m_NeedExportTexture;

    static const uint32                                 m_dwMaxTileMemory = 512 * 1024 * 1024;
    QString m_TerrainRGBFileName;
    QString m_pakFilename;

    bool OpenPakForLoading();

    bool SaveAndFreeMemory(const bool bForceFileCreation = false);

    CTerrainTextureTiles* LoadTileIfNeeded(const uint32 dwTileX, const uint32 dwTileY, bool bNoGarbageCollection = false);

    CTerrainTextureTiles* GetTilePtr(const uint32 dwTileX, const uint32 dwTileY);

    void FreeTile(CTerrainTextureTiles& rTile);

    CTerrainTextureTiles* FindOldestTileToFree();

    void ConsiderGarbageCollection();

    void ClipTileRect(QRect& inoutRect) const;

    bool IsDirty() const;

    /** Given absolute percentages of 0.0 - 1.0, return the tile index and the relative percent (0.0 - 1.0) within the tile.
            Both the absolute and relative percentages are inclusive on both sides of the range.  
        @param xPercent        input: the absolute percentage in the x direction (0.0 is left edge, 1.0 is right edge)
        @param yPercent        input: the absolute percentage in the y direction (0.0 is top edge, 1.0 is bottom edge)
        @param xTileIndex      output: the x index of the tile
        @param yTileIndex      output: the y index of the tile
        @param xTilePercent    output: the relative percentage within the tile in the x direction 
        @param yTilePercent    output: the relative percentage within the tile in the y direction
    */    
    inline void ConvertWorldToTileLookups(const double xPercent, const double yPercent, uint32& xTileIndex, uint32& yTileIndex, double& xTilePercent, double& yTilePercent);
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_RGBLAYER_H
