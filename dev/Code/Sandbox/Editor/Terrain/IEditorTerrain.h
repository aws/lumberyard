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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_ITERRAIN_H
#define CRYINCLUDE_EDITOR_TERRAIN_ITERRAIN_H
#pragma once

#include "Noise.h"
//#include "CryCommon/Cry_Vector3.h"

// Note: This type is used by the Terrain system; it isn't really a heightmap thing
struct SSectorInfo
{
    //! Type of terrain
    int type;

    //! Size of terrain unit.
    int unitSize;

    //! Sector size in meters.
    int sectorSize;

    //! Sector size in meters. Y dimension
    int sectorSizeY;

    //! Sector size in meters. Z dimension
    int sectorSizeZ;

    //! Size of texture for one sector in pixels.
    int sectorTexSize;

    //! Number of sectors on one side of terrain.
    int numSectors;

    //! Size of whole terrain surface texture.
    int surfaceTextureSize;
};

class IEditorTerrain
{
public:
    virtual void Init()=0;
    virtual void Update()=0;

    //options
    virtual bool SupportEditing()=0;
    virtual bool SupportLayers()=0;
    virtual bool SupportSerialize()=0;
    virtual bool SupportSerializeTexture()=0;
    virtual bool SupportHeightMap()=0;

    virtual size_t GetType()=0;
    virtual const char *GetTypeName()=0;
    virtual size_t GetTerrainTypeId()=0;
    virtual const char *GetTerrainTypeName()=0;

    //! Returns information about sectors on terrain.
    //! @param si Structure filled with queried data.
    virtual void GetSectorsInfo(SSectorInfo &si)=0;
    virtual Vec3i GetSectorSizeVector() const=0;
    virtual void InitSectorGrid(int numSectors)=0;
    virtual int GetNumSectors() const=0;
    virtual Vec3 SectorToWorld(const QPoint& sector) const=0;

    virtual uint64 GetWidth() const=0;
    virtual uint64 GetHeight() const=0;
    virtual uint64 GetDepth() const=0;

    virtual float GetMaxHeight() const=0;
    //! Clips heights greater than the input value (down to the input value)
    //! Minimum height = 1.0f
    virtual void SetMaxHeight(float fMaxHeight, bool scaleHeightmap=false)=0;

    virtual float GetOceanLevel() const=0;
    virtual void SetOceanLevel(float waterLevel)=0;

    //! Get size of every heightmap unit in meters.
    virtual int GetUnitSize() const=0;
    virtual void SetUnitSize(int nUnitSize)=0;

    //! Convert from world coordinates to terrain coordinates.
    virtual QPoint FromWorld(const Vec3& wp) const=0;

    //! Convert from terrain coordinates to world coordinates.
    virtual Vec3 ToWorld(const QPoint& pos) const=0;

    // Maps world bounding box to the terrain space rectangle.
    virtual QRect WorldBoundsToRect(const AABB& worldBounds) const=0;

    //! Set size of current surface texture.
    virtual void SetSurfaceTextureSize(int width, int height)=0;

//Layers
    virtual void EraseLayerID(uint8 id, uint8 replacementId)=0;

//    //! Retrieve heightmap data with optional transformations
//    //! @param pData Pointer to float array that will hold the output; it must be at least iDestWidth*iDestWidth elements large
//    //! @param iDestWidth The width of the destination
//    //! @param bSmooth Apply smoothing function before returning the data
//    //! @param bNoise Apply noise function before returning the data
//    virtual bool GetDataEx(t_hmap* pData, UINT iDestWidth, bool bSmooth=true, bool bNoise=true) const;
//
//    // Fill image data
//    // Arguments:
//    //   resolution Resolution of needed heightmap.
//    //   vTexOffset offset within hmap
//    //   trgRect Target rectangle in scaled heightmap.
//    virtual bool GetData(const QRect& trgRect, const int resolution, const QPoint& vTexOffset, CFloatImage& hmap, bool bSmooth=true, bool bNoise=true);
//
//    //////////////////////////////////////////////////////////////////////////
//    // Terrain Grid functions.
//    //////////////////////////////////////////////////////////////////////////
//    virtual void InitSectorGrid();
//    virtual CTerrainGrid* GetTerrainGrid() const { return m_terrainGrid.get(); }
//
//    //////////////////////////////////////////////////////////////////////////
//    virtual t_hmap& GetXY(uint32 x, uint32 y)=0;
//    virtual const t_hmap& GetXY(uint32 x, uint32 y) const=0;
//    virtual t_hmap& GetXYZ(uint32 x, uint32 y, uint32 z)=0;
//    virtual const t_hmap& GetXYZ(uint32 x, uint32 y, uint32 z) const=0;
//    virtual t_hmap GetSafeXY(uint32 dwX, uint32 dwY) const;
//
//    virtual void SetXY(uint32 x, uint32 y, t_hmap iVal)=0;
//    virtual void SetXYZ(uint32 x, uint32 y, uint32 z, t_hmap iVal)=0;

    virtual bool IsAllocated()=0;
    virtual void GetMemoryUsage(ICrySizer* pSizer)=0;

    // (Re)Allocate / deallocate
    virtual void Resize(int width, int height, int unitSize, bool cleanOld=true, bool forceKeepVegetation=false)=0;
    virtual void Resize(int width, int height, int depth, int unitSize, bool cleanOld=true, bool forceKeepVegetation=false)=0;
    virtual void CleanUp()=0;

    virtual void Update(bool bOnlyElevation, bool boUpdateReloadSurfacertypes)=0;
    virtual void UpdateSectors()=0;

    // Importing / exporting
    virtual void Serialize(CXmlArchive& xmlAr)=0;
    virtual void SerializeTerrain(CXmlArchive& xmlAr)=0;

    virtual void ClearTerrain()=0;

//    virtual void ExportBlock(const QRect& rect, CXmlArchive& ar, bool bIsExportVegetation=true, std::set<int>* pLayerIds=0, std::set<int>* pSurfaceIds=0)=0;
//    //! Import terrain block, return offset of block to new position.
//    virtual QPoint ImportBlock(CXmlArchive& ar, const QPoint& newPos, bool bUseNewPos=true, float heightOffset=0.0f, bool bOnlyVegetation=false, ImageRotationDegrees rotation=ImageRotationDegrees::Rotate0)=0;

private:
};

class IEditableTerrain:public IEditorTerrain
{
public:
    ~IEditableTerrain() {}

    bool SupportEditing() override { return true; }

    virtual void LoadImage(const QString& fileName)=0;
    virtual void LoadASC(const QString& fileName)=0;
    virtual void LoadBT(const QString& fileName)=0;
    virtual void LoadTIF(const QString& fileName)=0;
    virtual void LoadRAW(const QString& rawFile)=0;

    virtual void SaveImage(LPCSTR pszFileName) const=0;
    virtual void SaveASC(const QString& fileName)=0;
    virtual void SaveBT(const QString& fileName)=0;
    virtual void SaveTIF(const QString& fileName)=0;
    virtual void SaveRAW(const QString& rawFile)=0;
    virtual void SaveImage16Bit(const QString& fileName)=0;

    virtual bool GetPreviewBitmap(DWORD* pBitmapData, int width, bool bSmooth=true, bool bNoise=true, QRect* pUpdateRect=NULL, bool bShowOcean=false, bool bUseScaledRange=true)=0;

    virtual void Smooth()=0;
    virtual void Smooth(CFloatImage& hmap, const QRect& rect) const=0;

    virtual void Noise()=0;
    virtual void Normalize()=0;
    virtual void Invert()=0;
    virtual void MakeIsle()=0;
    virtual void SmoothSlope()=0;
    virtual void Randomize()=0;
    virtual void LowerRange(float fFactor)=0;
    virtual void Flatten(float fFactor)=0;
    virtual void GenerateTerrain(const SNoiseParams& sParam)=0;
    virtual void Clear(bool bClearLayerBitmap=true)=0;
    virtual void InitHeight(float fHeight)=0;

    virtual void Fetch()=0;
    virtual void Hold()=0;
    virtual bool Read(QString strFileName)=0;
};

// Interface to access CHeightmap from gems
class IHeightmap:public IEditableTerrain
{
public:
    ~IHeightmap() {}

    bool SupportHeightMap() override { return true; }

    virtual void UpdateEngineTerrain(int x1, int y1, int width, int height, bool bElevation, bool bInfoBits)=0;
    virtual void RecordAzUndoBatchTerrainModify(AZ::u32 x, AZ::u32 y, AZ::u32 width, AZ::u32 height)=0;
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_ITERRAIN_H
