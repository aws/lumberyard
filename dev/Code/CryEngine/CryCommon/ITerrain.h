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

#pragma once

#define COMPILED_HEIGHT_MAP_FILE_NAME "terrain/terrain.dat"
#define COMPILED_TERRAIN_TEXTURE_FILE_NAME "terrain/cover.ctc"

#pragma pack(push,4)

struct SHotUpdateInfo;
struct IRenderNode;
struct IStatObj;
struct IMaterial;
struct IStatInstGroup;

struct STerrainInfo
{
    int nHeightMapSize_InUnits;
    int nUnitSize_InMeters;
    int nSectorSize_InMeters;

    int nSectorsTableSize_InSectors;
    float fHeightmapZRatio;
    float fOceanWaterLevel;

    AUTO_STRUCT_INFO
};

#define TERRAIN_CHUNK_VERSION 29

struct STerrainChunkHeader
{
    int8 nVersion;
    int8 nDummy;
    int8 nFlags;
    int8 nFlags2;
    int32 nChunkSize;
    STerrainInfo TerrainInfo;

    AUTO_STRUCT_INFO
};

struct ITerrain
{
    virtual ~ITerrain() {}

    struct SurfaceWeight
    {
        static const int WeightCount = 3;
        static const uint8 Undefined = 127;
        static const uint8 Hole = 128;

        SurfaceWeight()
        {
            Ids[0] = Ids[1] = Ids[2] = Undefined;
            Weights[0] = Weights[1] = Weights[2] = 0;
        }

        inline uint8 PrimaryId() const
        {
            return Ids[0];
        }

        inline uint8 PrimaryWeight() const
        {
            return Weights[0];
        }

        uint8 Ids[WeightCount];

        uint8 Weights[WeightCount];

        AUTO_STRUCT_INFO
    };

    virtual bool SetCompiledData(byte* pData, int nDataSize, std::vector<IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial>>** ppMatTable, bool bHotUpdate = false, SHotUpdateInfo* pExportInfo = nullptr) = 0;

    virtual bool GetCompiledData(byte* pData, int nDataSize, std::vector<IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial>>** ppMatTable, std::vector<struct IStatInstGroup*>** ppStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo = nullptr) = 0;

    virtual int GetCompiledDataSize(SHotUpdateInfo* pExportInfo = nullptr) = 0;

    virtual void GetStatObjAndMatTables(DynArray<IStatObj*>* pStatObjTable, DynArray<_smart_ptr<IMaterial>>* pMatTable, DynArray<IStatInstGroup*>* pStatInstGroupTable, uint32 nObjTypeMask) = 0;

    virtual void SetTerrainElevation(int left, int bottom, int areaSize, const float* heightmap, int weightmapSize, const SurfaceWeight* surfaceWeightSet) = 0;

    virtual void SetOceanWaterLevel(float fOceanWaterLevel) = 0;

    virtual void ChangeOceanMaterial(_smart_ptr<IMaterial> pMat) = 0;

    virtual void InitTerrainWater(_smart_ptr<IMaterial> pTerrainWaterMat) = 0;

    virtual IRenderNode* AddVegetationInstance(int nStaticGroupID, const Vec3& vPos, const float fScale, uint8 ucBright, uint8 angle, uint8 angleX = 0, uint8 angleY = 0) = 0;

    virtual float GetZ(int x, int y) const = 0;
    virtual float GetBilinearZ(float x1, float y1) const = 0;

    virtual SurfaceWeight GetSurfaceWeight(int x, int y) const = 0;

    virtual Vec3 GetTerrainSurfaceNormal(Vec3 vPos, float fRange) = 0;
    virtual void GetTerrainAlignmentMatrix(const Vec3& vPos, const float amount, Matrix33& matrix33) = 0;

    virtual bool IsHole(int x, int y) const = 0;
    virtual bool IsMeshQuadFlipped(const int x, const int y, const int nUnitSize) const = 0;

};

//==============================================================================================

#define FILEVERSION_TERRAIN_TEXTURE_FILE 10

// Summary:
//   Common header for binary files used by 3dengine
struct SCommonFileHeader
{
    char    signature[4];   // File signature, should be "CRY "
    uint8   file_type;      // File type
    uint8   flags;          // File common flags
    uint16  version;        // File version

    AUTO_STRUCT_INFO
};

// Summary:
// Sub header for terrain texture file
struct STerrainTextureFileHeader
{
    uint16  LayerCount;
    uint16  Flags;
    float   ColorMultiplier_deprecated;

    AUTO_STRUCT_INFO
};

// Summary:
//   Layer header for terrain texture file (for each layer)
struct STerrainTextureLayerFileHeader
{
    uint16      SectorSizeInPixels; //
    uint16      nReserved;          // ensure padding and for later usage
    ETEX_Format eTexFormat;         // typically eTF_BC3
    uint32      SectorSizeInBytes;  // redundant information for more convenient loading code

    AUTO_STRUCT_INFO
};

#pragma pack(pop)