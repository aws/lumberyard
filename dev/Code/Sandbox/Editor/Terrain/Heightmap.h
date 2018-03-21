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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_HEIGHTMAP_H
#define CRYINCLUDE_EDITOR_TERRAIN_HEIGHTMAP_H
#pragma once

#include "RGBLayer.h"
#include "Layer.h"

#define DEFAULT_HEIGHTMAP_SIZE 4096

// Heightmap data type
// TODO: Move this type inside CHeightmap to scope it (or use a namespace)
// Note: The code using this is mostly doing evil direct array access
typedef float t_hmap;

class CXmlArchive;
class CDynamicArray2D;
struct SNoiseParams;
class CTerrainGrid;
struct SEditorPaintBrush;

// Note: This type is used by the Terrain system; it isn't really a heightmap thing
struct SSectorInfo
{
    //! Size of terrain unit.
    int unitSize;

    //! Sector size in meters.
    int sectorSize;

    //! Size of texture for one sector in pixels.
    int sectorTexSize;

    //! Number of sectors on one side of terrain.
    int numSectors;

    //! Size of whole terrain surface texture.
    int surfaceTextureSize;
};

// Interface to access CHeightmap from gems
class IHeightmap
{
public:
    virtual void UpdateEngineTerrain(int x1, int y1, int width, int height, bool bElevation, bool bInfoBits) = 0;
    virtual void RecordUndo(int x1, int y1, int width, int height, bool bInfo) = 0;
};

// Editor data structure to keep the heights, detail layer information/holes, terrain texture
class CHeightmap : public IHeightmap
{
public:
    CHeightmap();
    CHeightmap(const CHeightmap&);
    virtual ~CHeightmap();

    uint64 GetWidth() const { return m_iWidth; }
    uint64 GetHeight() const { return m_iHeight; }
    float GetMaxHeight() const { return m_fMaxHeight; }

    //! Get size of every heightmap unit in meters.
    int GetUnitSize() const { return m_unitSize; }

    //! Clips heights greater than the input value (down to the input value)
    //! Minimum height = 1.0f
    void SetMaxHeight(float fMaxHeight, bool scaleHeightmap = false);

    void SetUnitSize(int nUnitSize) { m_unitSize = nUnitSize; }

    //! Convert from world coordinates to heightmap coordinates.
    QPoint WorldToHmap(const Vec3& wp) const;

    //! Convert from heightmap coordinates to world coordinates.
    Vec3 HmapToWorld(const QPoint& hpos) const;

    // Maps world bounding box to the heightmap space rectangle.
    QRect WorldBoundsToRect(const AABB& worldBounds) const;

    //! Set size of current surface texture.
    void SetSurfaceTextureSize(int width, int height);

    //! Returns information about sectors on terrain.
    //! @param si Structure filled with queried data.
    void GetSectorsInfo(SSectorInfo& si);

    // TODO: This unchecked buffer access needs to go
    t_hmap* GetData() { return m_pHeightmap.data(); }

    //! Retrieve heightmap data with optional transformations
    //! @param pData Pointer to float array that will hold the output; it must be at least iDestWidth*iDestWidth elements large
    //! @param iDestWidth The width of the destination
    //! @param bSmooth Apply smoothing function before returning the data
    //! @param bNoise Apply noise function before returning the data
    bool GetDataEx(t_hmap* pData, UINT iDestWidth, bool bSmooth = true, bool bNoise = true) const;

    // Fill image data
    // Arguments:
    //   resolution Resolution of needed heightmap.
    //   vTexOffset offset within hmap
    //   trgRect Target rectangle in scaled heightmap.
    bool GetData(const QRect& trgRect, const int resolution, const QPoint& vTexOffset, CFloatImage& hmap, bool bSmooth = true, bool bNoise = true);

    //! resets the height map data, ocean, and mods
    void Reset(int resolution, int unitSize);

    //////////////////////////////////////////////////////////////////////////
    // Terrain Grid functions.
    //////////////////////////////////////////////////////////////////////////
    void InitSectorGrid();
    CTerrainGrid* GetTerrainGrid() const { return m_terrainGrid.get(); }

    //////////////////////////////////////////////////////////////////////////
    void SetXY(uint32 x, uint32 y, t_hmap iVal) { m_pHeightmap[x + y * m_iWidth] = iVal; }
    t_hmap& GetXY(uint32 x, uint32 y) { return m_pHeightmap[x + y * m_iWidth]; }
    const t_hmap& GetXY(uint32 x, uint32 y) const { return m_pHeightmap[x + y * m_iWidth]; }
    t_hmap GetSafeXY(uint32 dwX, uint32 dwY) const;

    //! Calculate heightmap slope at given point.
    //! @return clamped to 0-255 range slope value.
    float GetSlope(int x, int y);

    // Returns
    //   slope in height meters per meter
    float GetAccurateSlope(const float x, const float y);

    float GetZInterpolated(const float x1, const float y1);

    float CalcHeightScale() const;

    bool GetPreviewBitmap(DWORD* pBitmapData, int width, bool bSmooth = true, bool bNoise = true, QRect* pUpdateRect = NULL, bool bShowOcean = false, bool bUseScaledRange = true);

    void GetWeightmapBlock(int x, int y, int width, int height, Weightmap& map);

    void SetWeightmapBlock(int x, int y, const Weightmap& map);

    void PaintLayerId(const float fpx, const float fpy, const SEditorPaintBrush& brush, const uint32 dwLayerId);

    bool IsHoleAt(const int x, const int y) const;

    void SetHoleAt(const int x, const int y, const bool bHole);

    LayerWeight GetLayerWeightAt(const int x, const int y) const;

    void SetLayerWeightAt(const int x, const int y, const LayerWeight& weight);

    void SetLayerWeights(const AZStd::vector<uint8>& layerIds, const CImageEx* splatMaps, size_t splatMapCount);

    void EraseLayerID(uint8 id, uint8 replacementId);

    void MarkUsedLayerIds(bool bFree[CLayer::e_undefined]) const;

    // Hold / fetch
    void Fetch();
    void Hold();
    bool Read(QString strFileName);

    // (Re)Allocate / deallocate
    void Resize(int iWidth, int iHeight, int unitSize, bool bCleanOld = true, bool bForceKeepVegetation = false);
    void CleanUp();

    // Importing / exporting
    void Serialize(CXmlArchive& xmlAr);
    void SerializeTerrain(CXmlArchive& xmlAr);
    void SaveImage(LPCSTR pszFileName) const;

    void LoadImage(const QString& fileName);
    void LoadASC(const QString& fileName);
    void LoadBT(const QString& fileName);
    void LoadTIF(const QString& fileName);

    void SaveASC(const QString& fileName);
    void SaveBT(const QString& fileName);
    void SaveTIF(const QString& fileName);

    //! Save heightmap to 16-bit image file format (PGM, ASC, BT)
    void SaveImage16Bit(const QString& fileName);

    //! Save heightmap in RAW format.
    void    SaveRAW(const QString& rawFile);
    //! Load heightmap from RAW format.
    void    LoadRAW(const QString& rawFile);

    //! Return the heightmap as type CImageEx
    //! The image will be BGR format in grayscale
    std::shared_ptr<CImageEx> GetHeightmapImageEx() const;

    //! Return the heightmap as type CFloatImage
    //! The image will be greyscale 0.0 - 1.0 range
    std::shared_ptr<CFloatImage> GetHeightmapFloatImage(bool scaleValues, ImageRotationDegrees rotationAmount) const;

    // Actions
    void Smooth();
    void Smooth(CFloatImage& hmap, const QRect& rect) const;

    void Noise();
    void Normalize();
    void Invert();
    void MakeIsle();
    void SmoothSlope();
    void Randomize();
    void LowerRange(float fFactor);
    void Flatten(float fFactor);
    void GenerateTerrain(const SNoiseParams& sParam);
    void Clear(bool bClearLayerBitmap = true);
    void InitHeight(float fHeight);

    // Drawing
    void DrawSpot(unsigned long iX, unsigned long iY,
        uint8 iWidth, float fAddVal, float fSetToHeight = -1.0f,
        bool bAddNoise = false);

    void DrawSpot2(int iX, int iY, int radius, float insideRadius, float fHeigh, float fHardness = 1.0f, bool bAddNoise = false, float noiseFreq = 1, float noiseScale = 1);
    void SmoothSpot(int iX, int iY, int radius, float fHeigh, float fHardness);
    void RiseLowerSpot(int iX, int iY, int radius, float insideRadius, float fHeigh, float fHardness = 1.0f, bool bAddNoise = false, float noiseFreq = 1, float noiseScale = 1);

    //! Make hole in the terrain.
    void MakeHole(int x1, int y1, int width, int height, bool bMake);

    //! Export terrain block.
    void ExportBlock(const QRect& rect, CXmlArchive& ar, bool bIsExportVegetation = true, std::set<int>* pLayerIds = 0, std::set<int>* pSurfaceIds = 0);
    //! Import terrain block, return offset of block to new position.
    QPoint ImportBlock(CXmlArchive& ar, const QPoint& newPos, bool bUseNewPos = true, float heightOffset = 0.0f, bool bOnlyVegetation = false, ImageRotationDegrees rotation = ImageRotationDegrees::Rotate0);

    void PasteHeightmap(CFloatImage* pHeights, const Matrix34& transform, float heightDelta);
    void PasteHeightmapPrecise(CFloatImage* pHeights, QRect rc, float heightDelta);
    void PasteLayerIds(CByteImage* pIds, const Matrix34& transform);

    //! Update terrain block in engine terrain.
    void UpdateEngineTerrain(int x1, int y1, int width, int height, bool bElevation, bool bInfoBits) override;
    //! Update all engine terrain.
    void UpdateEngineTerrain(bool bOnlyElevation = true, bool boUpdateReloadSurfacertypes = true);

    //! Update layer textures in video memory.
    void UpdateLayerTexture(const QRect& rect);

    //! Synchronize engine hole bit with bit stored in editor heightmap.
    void UpdateEngineHole(int x1, int y1, int width, int height);

    void SetOceanLevel(float oceanLevel);
    float GetOceanLevel() const;

    void CopyData(t_hmap* pDataOut) const
    {
        memcpy(pDataOut, m_pHeightmap.data(), sizeof(t_hmap) * m_iWidth * m_iHeight);
    }

    //! Dump to log sizes of all layers.
    //! @return Total size allocated for layers.
    int LogLayerSizes();

    float GetShortPrecisionScale() const { return (256.f * 256.f - 1.0f) / m_fMaxHeight; }
    float GetBytePrecisionScale() const { return 255.f / m_fMaxHeight; }

    void GetMemoryUsage(ICrySizer* pSizer);

    void RecordUndo(int x1, int y1, int width, int height, bool bInfo = false) override;

    CRGBLayer* GetRGBLayer() { return &m_TerrainBGRTexture; }

    // Arguments:
    //   texsector - make sure the values are in valid range
    void UpdateSectorTexture(const QPoint& texsector, const float fGlobalMinX, const float fGlobalMinY, const float fGlobalMaxX, const float fGlobalMaxY);

    void InitTerrain();

    void AddModSector(int x, int y);
    void ClearModSectors();
    void UpdateModSectors();
    void UnlockSectorsTexture(const QRect& rc);

    int GetNoiseSize() const;
    float GetNoise(int x, int y) const;

    bool IsAllocated();


    void SetUseTerrain(bool useTerrain);
    bool GetUseTerrain();

private:
    void CopyFrom(t_hmap* prevHeightmap, LayerWeight* prevWeightmap, int prevSize);
    void CopyFromInterpolate(t_hmap* prevHeightmap, LayerWeight* prevWeightmap, int resolution, int prevUnitSize);

    bool ProcessLoadedImage(const QString& fileName, const CFloatImage& tmpImage, bool atWorldScale, ImageRotationDegrees rotationAmount);

    __inline float ExpCurve(float v, float fCover, float fSharpness);

    // Verify internal class state
    __inline void Verify()
    {
        assert(m_iWidth && m_iHeight);
        assert(m_pHeightmap.size() >= m_iWidth * m_iHeight);
    }

    // Initialization
    void InitNoise() const;

    float m_fOceanLevel;
    float m_fMaxHeight;

    std::vector<t_hmap> m_pHeightmap;

    // The noise data is initialized lazily; it is thus necessary to modify it
    // inside some methods that would otherwise be "const" -- we're making this
    // member mutable to hide this implementation detail.
    mutable std::unique_ptr<CDynamicArray2D> m_pNoise;

    Weightmap m_Weightmap;

    uint64 m_iWidth;
    uint64 m_iHeight;

    int m_textureSize; // Size of surface texture.
    int m_numSectors;  // Number of sectors per grid side.
    int m_unitSize;

    std::unique_ptr<CTerrainGrid> m_terrainGrid;

    CRGBLayer m_TerrainBGRTexture; // Terrain RGB texture

    // Changing mod sectors to a set for efficiency, using std::pair instead of Vec2i for built-in lexicographic operator<
    std::set<std::pair<int, int> > m_modSectors;
    bool m_updateModSectors;

    // When set this flag allows CHeightmap instances to be used without triggering
    // global events in the Editor. If this is unset CHeightmap will behave in the
    // "normal" fashion (firing UI events etc...).
    bool m_standaloneMode;

    bool m_useTerrain;

    // Raises an event indicating the heightmap was modified.
    void NotifyModified(int x = 0, int y = 0, int width = 0, int height = 0);

    friend class CUndoHeightmapInfo;
};


//////////////////////////////////////////////////////////////////////////
// Inlined implementation of get slope.
inline float CHeightmap::GetSlope(int x, int y)
{
    if (x <= 0 || y <= 0 || x >= m_iWidth - 1 || y >= m_iHeight - 1)
    {
        return 0;
    }

    t_hmap* p = &m_pHeightmap.data()[x + y * m_iWidth];
    float h = *p;
    int w = m_iWidth;
    float fs = (fabs_tpl(*(p + 1) - h) +
                fabs_tpl(*(p - 1) - h) +
                fabs_tpl(*(p + w) - h) +
                fabs_tpl(*(p - w) - h) +
                fabs_tpl(*(p + w + 1) - h) +
                fabs_tpl(*(p - w - 1) - h) +
                fabs_tpl(*(p + w - 1) - h) +
                fabs_tpl(*(p - w + 1) - h));
    fs = fs * 8.0f;
    if (fs > 255.0f)
    {
        fs = 255.0f;
    }
    return fs;
}

#endif // CRYINCLUDE_EDITOR_TERRAIN_HEIGHTMAP_H
