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
#pragma once

#include <AzCore/EBus/EBus.h>

#include <Cry_Math.h>
#include <smartptr.h>
#include <ITerrain.h>
#include <CryPodArray.h>
#include <IXml.h>
#include <IRenderer.h>

struct STerrainInfo; 
struct IMaterial;
struct ITerrainNode;
struct AABB;
struct ShadowMapFrustum;
struct SRenderingPassInfo;
struct SHotUpdateInfo;

namespace LegacyTerrain
{
    class CryTerrainRequests
        : public AZ::EBusTraits
    {
    public:
        /**
         * Requests a refresh of the terrain. It will be broadcast via the LegacyTerrainNotificationBus
         */
        virtual void RequestTerrainUpdate() = 0;
    };
    using CryTerrainRequestBus = AZ::EBus<CryTerrainRequests>;

    //! Requests for legacy terrain creation/destruction.
    //! This bus will be removed once the Legacy Terrain code base
    //! is moved from 3dengine into its own Gem. [LY-106934]
    //!  In the meantime
    //! this is the liaison used by the initial version of the LegacyTerrain Gem
    //! to instantiate CTerrain().
    class LegacyTerrainInstanceRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~LegacyTerrainInstanceRequests() = default;

        //! Returns true if a new legacy terrain system was created.
        //! Otherwise returns false (most likely out of memory, or a legacy
        //! terrain system already exists).
        //! Params:
        //! @octreeData Pointer to the octree data. The octree data contains the heightmap data.
        //!     If nullptr the terrain heightmap data will be read from the compiled octree data.
        //!     REMARK: Should be "const uint8*", but deep down the cry legacy code has non const
        //!     declarations.
        //! @octreeDataSize Size of the octree data buffer. If @octreeData is nullptr then this must be 0.
        virtual bool CreateTerrainSystem(AZ::u8* octreeData, size_t octreeDataSize) = 0;

        //! Creates an uninitialized terrain system. Uninitialized means it has no heightmap data at all.
        virtual bool CreateUninitializedTerrainSystem(const STerrainInfo& terrainInfo) = 0;

        //! Returns true if the legacy terrain system was created.
        virtual bool IsTerrainSystemInstantiated() const = 0;

        virtual void DestroyTerrainSystem() = 0;
    };

    using LegacyTerrainInstanceRequestBus = AZ::EBus<LegacyTerrainInstanceRequests>;

    //! Requests for legacy terrain creation/destruction using Editor data.
    //! This bus definition needs to be available to game code as well as Editor code, since we still want to use
    //! unsaved Editor data for the terrain when running in Game Mode within the Editor.
    class LegacyTerrainEditorDataRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~LegacyTerrainEditorDataRequests() = default;

        //! Returns true if a new legacy terrain system was created.
        //! Otherwise returns false (most likely out of memory, or a legacy
        //! terrain system already exists).
        virtual bool CreateTerrainSystemFromEditorData() = 0;

        virtual void DestroyTerrainSystem() = 0;

        virtual void RefreshEngineMacroTexture() = 0;

        //! Warning. This method is not efficient, you should cache the results.
        virtual int GetTerrainSurfaceIdFromSurfaceTag(AZ::Crc32 tag) = 0;
    };

    using LegacyTerrainEditorDataRequestBus = AZ::EBus<LegacyTerrainEditorDataRequests>;

    namespace MacroTexture
    {
        //! Current statistics of the internal streaming engine and texture pools.
        struct TileStatistics
        {
            //! Total number of textures available.
            uint32 poolCapacity;

            //! Total texture tiles requested for paging.
            uint32 activeTotalCount;

            //! Total texture tiles resident in memory.
            uint32 residentCount;

            //! Total texture tiles currently streaming from disk.
            uint32 streamingCount;

            //! Total texture tiles waiting to be streamed (requires eviction).
            uint32 waitingCount;
        };
    }

    struct SRayTrace
    {
        float t;
        Vec3  hitPoint;
        Vec3  hitNormal;
        _smart_ptr<IMaterial> material;

        SRayTrace()
            : t(0)
            , hitPoint(0, 0, 0)
            , hitNormal(0, 0, 1)
            , material(nullptr)
        {}

        SRayTrace(float t_, Vec3 const& hitPoint_, Vec3 const& hitNormal_, _smart_ptr<IMaterial> material_)
            : t(t_)
            , hitPoint(hitPoint_)
            , hitNormal(hitNormal_)
            , material(material_)
        {}
    };

    struct MacroTextureConfiguration
    {
        AZStd::vector<int16> indexBlocks;
        AZStd::string filePath = "";
        uint32 maxElementCountPerPool = 0x1000;
        uint32 totalSectorDataSize = 0;
        uint32 tileSizeInPixels = 0;
        uint32 sectorStartDataOffset = 0;
        float colorMultiplier_deprecated = 1.0f;
        ETEX_Format texureFormat = eTF_Unknown;
        bool endian = eLittleEndian;
    };

    class LegacyTerrainDataRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //REMARK: The Legacy Terrain system is also a subclass of AzFramework::Terrain::TerrainDataRequestBus, which in turn also
        //declares its MutexType as AZStd::recursive_mutex. To avoid deadlocks the order of EnumerateCallbacks when using both EBuses
        //should be:
        //AzFramework::Terrain::TerrainDataRequestBus::EnumerateCallbacks
        //{
        //    LegacyTerrain::LegacyTerrainDataRequestBus::EnumerateCallbacks
        //    {
        //     
        //    }
        //}
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual ~LegacyTerrainDataRequests() = default;

        virtual void SetTerrainElevationAndSurfaceWeights(int left, int bottom, int areaSize, const float* heightmap, int weightmapSize, const ITerrain::SurfaceWeight* surfaceWeightSet) = 0;

        virtual void LoadTerrainSurfacesFromXML(XmlNodeRef pDoc) = 0;

        //! Gets the size of a terrain sector in meters.
        virtual int GetTerrainSectorSize() const = 0;

        virtual int GetTerrainSectorsTableSize() const = 0;

        //! Get the id of the primary surface at the given coordinates (coordinates are in meters).
        virtual int GetTerrainSurfaceId(int x, int y) const = 0;

        virtual bool IsTerrainMeshQuadFlipped(int x, int y, int nUnitSize) const = 0;

        virtual void GetTerrainMaterials(AZStd::vector<_smart_ptr<IMaterial>>& materials) = 0;

        //! Returns the number of nodes (ITerrainNode) that wrote data into pData.
        virtual int GetNodesData(byte*& pData, int& nDataSize, EEndian eEndian, SHotUpdateInfo* pExportInfo) = 0;

        virtual ITerrainNode* FindMinNodeContainingBox(const AABB& someBox) = 0;

        virtual void IntersectWithBox(const AABB& aabbBox, PodArray<ITerrainNode*>* plstResult) = 0;

        virtual void IntersectWithShadowFrustum(PodArray<ITerrainNode*>* plstResult, ShadowMapFrustum* pFrustum, const SRenderingPassInfo& passInfo) = 0;

        virtual void AddVisSector(ITerrainNode* newsec) = 0;

        virtual void MarkAllSectorsAsUncompiled() = 0;
        
        virtual void ResetTerrainVertBuffers() = 0;

        virtual int GetActiveProcObjNodesCount() const = 0;

        virtual float GetDistanceToSectorWithWater() const = 0;

        virtual void ClearVisSectors() = 0;

        virtual void UpdateNodesIncrementally(const SRenderingPassInfo& passInfo) = 0;

        virtual void CheckVis(const SRenderingPassInfo& passInfo) = 0;

        virtual void ClearTextureSetsAndDrawVisibleSectors(bool clearTextureSets, const SRenderingPassInfo& passInfo) = 0;

        virtual void UpdateSectorMeshes(const SRenderingPassInfo& passInfo) = 0;

        virtual void CheckNodesGeomUnload(const SRenderingPassInfo& passInfo) = 0;

        virtual bool IsOceanVisible() const = 0;

        virtual bool TryGetTextureStatistics(MacroTexture::TileStatistics& statistics) const = 0;

        virtual bool RayTrace(Vec3 const& vStart, Vec3 const& vEnd, SRayTrace* prt) = 0;

        virtual bool RenderArea(Vec3 vPos, float fRadius, _smart_ptr<IRenderMesh>& arrLightRenderMeshs
            , CRenderObject* pObj, _smart_ptr<IMaterial> pMaterial, const char* szComment, float* pCustomData
            , Plane* planes, const SRenderingPassInfo& passInfo) = 0;

        virtual bool IsTextureStreamingInProgress() const = 0;

        //! Returns a value between 0.0f and 255.0f;
        virtual float GetSlope(int x, int y) const = 0;

        //! Sets terrain sector texture id, and disable streaming on this sector
        virtual void SetTerrainSectorTexture(int nTexSectorX, int nTexSectorY, unsigned int textureId, unsigned int textureSizeX, unsigned int textureSizeY, bool bMergeNotAllowed) = 0;

        //! Closes terrain texture file handle and allows to replace/update it.
        virtual void CloseTerrainTextureFile() = 0;

        //! Attempts to import a macro texture file at filepath, and fills out the provided configuration with data from the file.
        //! Returns true if successful.
        virtual bool ReadMacroTextureFile(const char* filepath, MacroTextureConfiguration& configuration) const = 0;

        virtual void GetMemoryUsage(class ICrySizer* pSizer) const = 0;

        virtual void GetResourceMemoryUsage(ICrySizer* pSizer, const AABB& crstAABB) = 0;
    };
    using LegacyTerrainDataRequestBus = AZ::EBus<LegacyTerrainDataRequests>;

    /**
     * Broadcast legacy terrain notifications.
     */
    class LegacyTerrainNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~LegacyTerrainNotifications() = default;

        /**
         * Indicates that the number of terrain tiles or number of samples per tile has changed.
         * @param numTiles The number of tiles along each edge of the terrain (assumed to be equal in both directions).
         * @param tileSize The number of samples along each edge of a tile (assumed to be equal in both directions).
         */
        virtual void SetNumTiles(const AZ::u32 numTiles, const AZ::u32 tileSize) = 0;

        /**
         * Indicates that the height values for a tile have been updated.
         * The height data are assumed to have been linearly scaled with a multiplier and offset determined by
         * heightMin and heightScale.
         * Note that because of the way height values are split up between tiles in Cry, the tile boundaries furthest
         * away from the origin need to share vertices from adjacent tiles to prevent gaps in the terrain.  Therefore,
         * these boundary values will only be correct if tiles are updated after their neighbours in the +x, +y
         * directions.
         * @param tileX The x co-ordinate of the updated tile.
         * @param tileY The y co-ordinate of the updated tile.
         * @param heightMap Pointer to scaled height data.
         * @param heightMin The height corresponding to the value 0 in the scaled height data.
         * @param heightScale The change in height corresponding to 1 unit of the scaled height data.
         * @param tileSize The number of samples along each edge of a tile (assumed to be equal in both directions).
         * @param heightMapUnitSize The distance in metres between adjacent height samples.
         */
        virtual void UpdateTile(const AZ::u32 tileX, const AZ::u32 tileY, const AZ::u16* heightMap,
            const float heightMin, const float heightScale, const AZ::u32 tileSize, const AZ::u32 heightMapUnitSize) = 0;

    };

    using LegacyTerrainNotificationBus = AZ::EBus<LegacyTerrainNotifications>;
} // namespace LegacyTerrain
