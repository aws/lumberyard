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

#include "StdAfx.h"

#include "3dEngine.h"
#include "Ocean.h"
#include "StatObj.h"
#include "ObjMan.h"
#include "MatMan.h"
#include "VisAreas.h"
#include "ObjectsTree.h"
#include "MergedMeshGeometry.h"

#include <ITerrain.h>
#include <MathConversion.h>
#include <StatObjBus.h>
#include <PakLoadDataUtils.h>
#include <Vegetation/StaticVegetationBus.h>


#define SIGC_ALIGNTOTERRAIN       BIT(0) // Deprecated
#define SIGC_USETERRAINCOLOR      BIT(1)
#define SIGC_HIDEABILITY          BIT(3)
#define SIGC_HIDEABILITYSECONDARY BIT(4)
#define SIGC_PROCEDURALLYANIMATED BIT(6)
#define SIGC_CASTSHADOW           BIT(7) // Deprecated
#define SIGC_RECVSHADOW           BIT(8)
#define SIGC_DYNAMICDISTANCESHADOWS BIT(9)
#define SIGC_USEALPHABLENDING     BIT(10)
#define SIGC_RANDOMROTATION       BIT(12)
#define SIGC_ALLOWINDOOR                    BIT(13)

// Bits 13-14 reserved for player hideability
#define SIGC_PLAYERHIDEABLE_LOWBIT (13)
#define SIGC_PLAYERHIDEABLE_MASK   BIT(13) | BIT(14)

#define SIGC_CASTSHADOW_MINSPEC_SHIFT (15)

// Get the number of bits needed for the maximum spec level
#define SIGC_CASTSHADOW_MINSPEC_MASK_NUM_BITS_NEEDED (IntegerLog2(uint32(END_CONFIG_SPEC_ENUM - 1)) + 1)

// Create a mask based on the number of bits needed
#define SIGC_CASTSHADOW_MINSPEC_MASK_BITS ((1 << SIGC_CASTSHADOW_MINSPEC_MASK_NUM_BITS_NEEDED) - 1)
#define SIGC_CASTSHADOW_MINSPEC_MASK  (SIGC_CASTSHADOW_MINSPEC_MASK_BITS << SIGC_CASTSHADOW_MINSPEC_SHIFT)

#define TERRAIN_NODE_CHUNK_VERSION 8


bool C3DEngine::CreateOctree(float initialWorldSize)
{
    COctreeNode* newOctreeNode = COctreeNode::Create(DEFAULT_SID, AABB(Vec3(0), Vec3(initialWorldSize)), NULL);
    if (!newOctreeNode)
    {
        Error("Failed to create octree with initial world size=%f", initialWorldSize);
        return false;
    }
    SetObjectTree(newOctreeNode);
    GetObjManager()->GetListStaticTypes().PreAllocate(1, 1);
    GetObjManager()->GetListStaticTypes()[DEFAULT_SID].Reset();
    return true;
}


void C3DEngine::DestroyOctree()
{
    if (GetObjectTree())
    {
        delete GetObjectTree();
        SetObjectTree(nullptr);
    }
}


bool C3DEngine::LoadOctree(XmlNodeRef pDoc, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable, int nSID)
{
    LOADING_TIME_PROFILE_SECTION;

    PrintMessage("===== Loading %s =====", COMPILED_OCTREE_FILE_NAME);

    // open file
    AZ::IO::HandleType fileHandle = GetPak()->FOpen(GetLevelFilePath(COMPILED_OCTREE_FILE_NAME), "rbx");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return 0;
    }

    // read header
    STerrainChunkHeader header;
    if (!GetPak()->FRead(&header, 1, fileHandle, false))
    {
        GetPak()->FClose(fileHandle);
        return 0;
    }

    SwapEndian(header, (header.nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian);

    bool loadSuccess = false;
    if (header.nChunkSize)
    {
        DestroyOctree();
        CreateOctree(static_cast<float>(header.TerrainInfo.TerrainSize()));

        //Keep record of the ocean water level.
        COcean::SetWaterLevelInfo(header.TerrainInfo.fOceanWaterLevel);
        const float fOceanWaterLevel = header.TerrainInfo.fOceanWaterLevel ? header.TerrainInfo.fOceanWaterLevel : WATER_LEVEL_UNKNOWN;
        if (GetOcean())
        {
            GetOcean()->SetWaterLevel(fOceanWaterLevel);
        }

        loadSuccess = LoadOctreeInternal(pDoc, fileHandle, header.nChunkSize - sizeof(STerrainChunkHeader), &header, ppStatObjTable, ppMatTable);

    }

    AZ_Assert(GetPak()->FEof(fileHandle), "Was expecting to be at the end of the file.");

    GetPak()->FClose(fileHandle);

    return loadSuccess;
}

bool C3DEngine::LoadOctreeInternal(XmlNodeRef pDoc, AZ::IO::HandleType fileHandle, int nDataSize, STerrainChunkHeader* pOctreeChunkHeader, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable)
{
    bool bRes;

    // in case of small data amount (console game) load entire file into memory in single operation
    if (nDataSize < 4 * 1024 * 1024)
    {
        _smart_ptr<IMemoryBlock> pMemBlock = gEnv->pCryPak->PoolAllocMemoryBlock(nDataSize + 8, "LoadOctreeInternal");
        byte* pPtr = (byte*)pMemBlock->GetData();
        while (UINT_PTR(pPtr) & 3)
        {
            pPtr++;
        }

        if (GetPak()->FReadRaw(pPtr, 1, nDataSize, fileHandle) != nDataSize)
        {
            return false;
        }

        bRes = LoadOctreeInternal_T(pDoc, pPtr, nDataSize, pOctreeChunkHeader, ppStatObjTable, ppMatTable, false, static_cast<SHotUpdateInfo*>(nullptr));
    }
    else
    {
        // in case of big data files - load data in many small blocks
        bRes = LoadOctreeInternal_T(pDoc, fileHandle, nDataSize, pOctreeChunkHeader, ppStatObjTable, ppMatTable, false, static_cast<SHotUpdateInfo*>(nullptr));
    }

    return bRes;
}

#ifndef LY_TERRAIN_LEGACY_RUNTIME
template <class T>
int C3DEngine::SkipTerrainData_T(T& f, int& nDataSize, const STerrainInfo& terrainInfo, bool bHotUpdate, bool bHMap, bool bSectorPalettes, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
    // get terrain settings
    int unitSize;
    float invUnitSize;
    int terrainSize;
    int meterToUnitBitShift;
    int terrainSizeDiv;
    int sectorSize;
    int sectorsTableSize;
    int unitToSectorBitShift;

    // get terrain settings
    terrainInfo.LoadTerrainSettings(unitSize, invUnitSize, terrainSize,
        meterToUnitBitShift, terrainSizeDiv,
        sectorSize, sectorsTableSize, unitToSectorBitShift);

    const int sectorSizeInUnits = (1 << unitToSectorBitShift) + 1;

    // Calculates the number of terrain nodes as it runs through the number of floors (or levels) in the "would-have-been" Quadtree.
    // Levels saved without terrain still have one dummy root node, so make sure to always have at least one terrain node in the count.
    int nodeSize = terrainSize;
    int quadtreeFloorIndex = 0;
    int numTerrainNodes = 0;
    do
    {
        numTerrainNodes += (1 << quadtreeFloorIndex)*(1 << quadtreeFloorIndex);
        nodeSize = (nodeSize >> 1);
        quadtreeFloorIndex++;
    } while ((nodeSize >= sectorSize) && (nodeSize > 0));

    for (int i = 0; i < numTerrainNodes; i++)
    {
        STerrainNodeChunk chunk;
        if (!PakLoadDataUtils::LoadDataFromFile(&chunk, 1, f, nDataSize, eEndian))
        {
            continue;
        }

        AZ_Assert(chunk.nChunkVersion == TERRAIN_NODE_CHUNK_VERSION, "Unexpected STerrainNodeChunk.nChunkVersion");
        if (chunk.nChunkVersion != TERRAIN_NODE_CHUNK_VERSION)
        {
            continue;
        }

        if (chunk.nSize)
        {
            AZ_Assert(chunk.nSize == sectorSizeInUnits, "sectorSizeInUnits doesn't match its value in STerrainNodeChunk.nSize");

            const int ChunkSizeSqr = chunk.nSize * chunk.nSize;
            if (!PakLoadDataUtils::LoadDataFromFile_Seek(ChunkSizeSqr * sizeof(uint16), f, nDataSize, eEndian))
            {
                continue;
            }

            PakLoadDataUtils::LoadDataFromFile_FixAlignment(f, nDataSize);

            if (!PakLoadDataUtils::LoadDataFromFile_Seek(ChunkSizeSqr *sizeof(ITerrain::SurfaceWeight), f, nDataSize, eEndian))
            {
                continue;
            }

            PakLoadDataUtils::LoadDataFromFile_FixAlignment(f, nDataSize);
        }

        // Skip LOD errors
        if (!PakLoadDataUtils::LoadDataFromFile_Seek(unitToSectorBitShift * sizeof(float), f, nDataSize, eEndian))
        {
            continue;
        }

        if (chunk.nSurfaceTypesNum)
        {
            if (!PakLoadDataUtils::LoadDataFromFile_Seek(chunk.nSurfaceTypesNum, f, nDataSize, eEndian))
            {
                continue;
            }
            PakLoadDataUtils::LoadDataFromFile_FixAlignment(f, nDataSize);
        }
    }

    return numTerrainNodes;
}


void C3DEngine::GetEmptyTerrainCompiledData(byte*& pData, int& nDataSize, EEndian eEndian)
{
    if (!pData)
    {
        nDataSize += sizeof(STerrainNodeChunk);
        return;
    }

    STerrainNodeChunk* pChunk = (STerrainNodeChunk*)pData;
    pChunk->nChunkVersion = TERRAIN_NODE_CHUNK_VERSION;
    pChunk->boxHeightmap = AABB(Vec3(0.0f));
    pChunk->bHasHoles = true;
    pChunk->fOffset = 0.0;
    pChunk->fRange = 0.0;
    pChunk->nSize = 0;
    pChunk->nSurfaceTypesNum = 0;

    SwapEndian(*pChunk, eEndian);
    UPDATE_PTR_AND_SIZE(pData, nDataSize, sizeof(STerrainNodeChunk));
}
#endif //#ifndef LY_TERRAIN_LEGACY_RUNTIME


void C3DEngine::CreateTerrainInternal(const STerrainInfo& TerrainInfo)
{
#ifdef LY_TERRAIN_LEGACY_RUNTIME
    SAFE_DELETE(m_pTerrain);
    m_pTerrain = new CTerrain(TerrainInfo);
    float terrainSize = (float)m_pTerrain->GetTerrainSize() - TERRAIN_AABB_PADDING;
    if (terrainSize < TERRAIN_AABB_PADDING)
    {
        terrainSize = TERRAIN_AABB_PADDING;
    }
    m_terrainAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(TERRAIN_AABB_PADDING, TERRAIN_AABB_PADDING, -FLT_MAX), AZ::Vector3(terrainSize, terrainSize, FLT_MAX));
#endif //#ifdef LY_TERRAIN_LEGACY_RUNTIME
}


template <class T>
bool C3DEngine::LoadOctreeInternal_T(XmlNodeRef pDoc, T& f, int& nDataSize, STerrainChunkHeader* pOctreeChunkHeader, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable, bool bHotUpdate, SHotUpdateInfo* pExportInfo, bool loadTerrainMacroTexture)
{
    LOADING_TIME_PROFILE_SECTION;

    AZ_Assert(pOctreeChunkHeader->nVersion == OCTREE_CHUNK_VERSION, "Invalid Octree Chunk Version");
    if (pOctreeChunkHeader->nVersion != OCTREE_CHUNK_VERSION)
    {
        Error("C3DEngine::LoadOctreeInternal_T: version of file is %d, expected version is %d", pOctreeChunkHeader->nVersion, (int)OCTREE_CHUNK_VERSION);
        return 0;
    }

    EEndian eEndian = (pOctreeChunkHeader->nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian;
    bool bSectorPalettes = (pOctreeChunkHeader->nFlags & SERIALIZATION_FLAG_SECTOR_PALETTES) != 0;

    bool bHMap(!pExportInfo || pExportInfo->nHeigtmap);
    bool bObjs(!pExportInfo || pExportInfo->nObjTypeMask);
    AABB* pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

    if (bHotUpdate)
    {
        PrintMessage("Importing outdoor data (%s, %.2f MB) ...",
            (bHMap && bObjs) ? "Objects and heightmap" : (bHMap ? "Heightmap" : (bObjs ? "Objects" : "Nothing")), ((float)nDataSize) / 1024.f / 1024.f);
    }

    if (pOctreeChunkHeader->nChunkSize != nDataSize + sizeof(STerrainChunkHeader))
    {
        return 0;
    }

    if (bHotUpdate)
    {
        m_bAreaActivationInUse = false;
    }
    else
    {
        m_bAreaActivationInUse = (pOctreeChunkHeader->nFlags2 & TCH_FLAG2_AREA_ACTIVATION_IN_USE) != 0;
    }

    if (IsAreaActivationInUse())
    {
        PrintMessage("Object layers control in use");
    }

    const int nTerrainSize = pOctreeChunkHeader->TerrainInfo.TerrainSize();

    // setup physics grid
    if (!m_bEditor && !bHotUpdate)
    {
        LOADING_TIME_PROFILE_SECTION_NAMED("SetupEntityGrid");

        //CryPhysics under performs if physicsEntityGridSize < nTerrainSize.
        int physicsEntityGridSize = nTerrainSize;
        if (physicsEntityGridSize <= 0)
        {
            physicsEntityGridSize = GetCVars()->e_PhysEntityGridSizeDefault;
        }

        int nCellSize = physicsEntityGridSize > 2048 ? physicsEntityGridSize >> 10 : 2;
        nCellSize = max(nCellSize, GetCVars()->e_PhysMinCellSize);
        int log2PODGridSize = 0;
        if (nCellSize == 2)
        {
            log2PODGridSize = 2;
        }
        else if (nCellSize == 4)
        {
            log2PODGridSize = 1;
        }
        GetPhysicalWorld()->SetupEntityGrid(2, Vec3(0, 0, 0), // this call will destroy all physicalized stuff
            physicsEntityGridSize / nCellSize, physicsEntityGridSize / nCellSize, (float)nCellSize, (float)nCellSize, log2PODGridSize);
    }

    std::vector<_smart_ptr<IMaterial> >* pMatTable = NULL;
    std::vector<IStatObj*>* pStatObjTable = NULL;

    if (bObjs)
    {
        PodArray<StatInstGroupChunk> lstStatInstGroupChunkFileChunks;

        { // get vegetation objects count
            LOADING_TIME_PROFILE_SECTION_NAMED("Vegetation");

            int nObjectsCount = 0;
            if (!PakLoadDataUtils::LoadDataFromFile(&nObjectsCount, 1, f, nDataSize, eEndian))
            {
                return 0;
            }

            if (!bHotUpdate)
            {
                PrintMessage("===== Loading %d vegetation models =====", nObjectsCount);
            }

            // load chunks into temporary array
            PodArray<StatInstGroupChunk>& lstFileChunks = lstStatInstGroupChunkFileChunks;
            lstFileChunks.PreAllocate(nObjectsCount, nObjectsCount);
            if (!PakLoadDataUtils::LoadDataFromFile(lstFileChunks.GetElements(), lstFileChunks.Count(), f, nDataSize, eEndian))
            {
                return 0;
            }

            // get vegetation objects table
            if (!m_bEditor || bHotUpdate)
            {
                // preallocate real array
                PodArray<StatInstGroup>& rTable = GetObjManager()->GetListStaticTypes()[DEFAULT_SID];
                rTable.resize(nObjectsCount);
                StatInstGroupEventBus::Broadcast(&StatInstGroupEventBus::Events::ReserveStatInstGroupIdRange, StatInstGroupId(0), StatInstGroupId(nObjectsCount));

                // init struct values and load cgf's
                for (uint32 i = 0; i < rTable.size(); i++)
                {
                    LoadVegetationData(rTable, lstFileChunks, i);
                }
            }
        }

        pStatObjTable = new std::vector < IStatObj* >;

        { // get brush objects count
            LOADING_TIME_PROFILE_SECTION_NAMED("Brushes");

            int nObjectsCount = 0;
            if (!PakLoadDataUtils::LoadDataFromFile(&nObjectsCount, 1, f, nDataSize, eEndian))
            {
                delete pStatObjTable;
                pStatObjTable = nullptr;
                return 0;
            }

            if (!bHotUpdate)
            {
                PrintMessage("===== Loading %d brush models ===== ", nObjectsCount);
            }

            PodArray<SNameChunk> lstFileChunks;
            lstFileChunks.PreAllocate(nObjectsCount, nObjectsCount);
            if (!PakLoadDataUtils::LoadDataFromFile(lstFileChunks.GetElements(), lstFileChunks.Count(), f, nDataSize, eEndian))
            {
                delete pStatObjTable;
                pStatObjTable = nullptr;
                return 0;
            }

            // get brush objects table
            if (!m_bEditor || bHotUpdate)
            {
                pStatObjTable->resize(nObjectsCount);//PreAllocate(nObjectsCount,nObjectsCount);

                // load cgf's
                for (uint32 i = 0; i < pStatObjTable->size(); i++)
                {
                    if (lstFileChunks[i].szFileName[0])
                    {
                        (*pStatObjTable)[i] = GetObjManager()->LoadStatObjUnsafeManualRef(lstFileChunks[i].szFileName);
                    }

                    SLICE_AND_SLEEP();
                }
            }
        }

        pMatTable = new std::vector < _smart_ptr<IMaterial> >;

        { // get brush materials count
            LOADING_TIME_PROFILE_SECTION_NAMED("BrushMaterials");

            int nObjectsCount = 0;
            if (!PakLoadDataUtils::LoadDataFromFile(&nObjectsCount, 1, f, nDataSize, eEndian))
            {
                return 0;
            }

            // get vegetation objects table
            if (!m_bEditor || bHotUpdate)
            {
                if (!bHotUpdate)
                {
                    PrintMessage("===== Loading %d brush materials ===== ", nObjectsCount);
                }

                std::vector<_smart_ptr<IMaterial> >& rTable = *pMatTable;
                rTable.clear();
                rTable.resize(nObjectsCount);//PreAllocate(nObjectsCount,nObjectsCount);

                const uint32 cTableCount = rTable.size();
                for (uint32 tableIndex = 0; tableIndex < cTableCount; ++tableIndex)
                {
                    SNameChunk matName;
                    if (!PakLoadDataUtils::LoadDataFromFile(&matName, 1, f, nDataSize, eEndian))
                    {
                        return 0;
                    }
                    CryPathString sMtlName = matName.szFileName;
                    sMtlName = PathUtil::MakeGamePath(sMtlName);
                    rTable[tableIndex] = matName.szFileName[0] ? GetMatMan()->LoadMaterial(sMtlName).get() : NULL;

                    SLICE_AND_SLEEP();
                }

                // assign real material to vegetation group
                PodArray<StatInstGroup>& rStaticTypes = GetObjManager()->GetListStaticTypes()[DEFAULT_SID];
                for (uint32 i = 0; i < rStaticTypes.size(); i++)
                {
                    if (lstStatInstGroupChunkFileChunks[i].nMaterialId >= 0)
                    {
                        rStaticTypes[i].pMaterial = rTable[lstStatInstGroupChunkFileChunks[i].nMaterialId];
                    }
                    else
                    {
                        rStaticTypes[i].pMaterial = NULL;
                    }
                }
            }
            else
            { // skip data
                SNameChunk matInfoTmp;
                for (int tableIndex = 0; tableIndex < nObjectsCount; ++tableIndex)
                {
                    if (!PakLoadDataUtils::LoadDataFromFile(&matInfoTmp, 1, f, nDataSize, eEndian))
                    {
                        return 0;
                    }
                }
            }
        }
    }

    // set nodes data
    int nNodesLoaded = 1;

#ifdef LY_TERRAIN_LEGACY_RUNTIME
    {
        LOADING_TIME_PROFILE_SECTION_NAMED("Terrain");
        CreateTerrainInternal(pOctreeChunkHeader->TerrainInfo);
        AZ_Assert(m_pTerrain, "Failed to create terrain.");
        nNodesLoaded = m_pTerrain->Load_T(pDoc, f, nDataSize, pOctreeChunkHeader->TerrainInfo, bHotUpdate, bHMap, bSectorPalettes, eEndian, pExportInfo, loadTerrainMacroTexture);
    }
#else
    nNodesLoaded = SkipTerrainData_T(f, nDataSize, pOctreeChunkHeader->TerrainInfo, bHotUpdate, bHMap, bSectorPalettes, eEndian, pExportInfo);
#endif //#ifdef LY_TERRAIN_LEGACY_RUNTIME

    if (bObjs)
    {
        if (!bHotUpdate)
        {
            PrintMessage("===== Loading outdoor instances ===== ");
        }

        LOADING_TIME_PROFILE_SECTION_NAMED("ObjectInstances");

        PodArray<IRenderNode*> arrUnregisteredObjects;
        GetObjectTree()->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, true);

        // load object instances (in case of editor just check input data do no create object instances)
        int nOcNodesLoaded = 0;
        if (NULL != pExportInfo && NULL != pExportInfo->pVisibleLayerMask && NULL != pExportInfo->pLayerIdTranslation)
        {
            SLayerVisibility visInfo;
            visInfo.pLayerIdTranslation = pExportInfo->pLayerIdTranslation;
            visInfo.pLayerVisibilityMask = pExportInfo->pVisibleLayerMask;
            nOcNodesLoaded = GetObjectTree()->Load(f, nDataSize, pStatObjTable, pMatTable, eEndian, pBox, &visInfo);
        }
        else
        {
            nOcNodesLoaded = GetObjectTree()->Load(f, nDataSize, pStatObjTable, pMatTable, eEndian, pBox, NULL);
        }

        for (int i = 0; i < arrUnregisteredObjects.Count(); i++)
        {
            arrUnregisteredObjects[i]->ReleaseNode();
        }
        arrUnregisteredObjects.Reset();
    }

    if (m_bEditor && !bHotUpdate && Get3DEngine()->IsObjectTreeReady())
    { // editor will re-insert all objects
        AABB aabb = GetObjectTree()->GetNodeBox();
        delete GetObjectTree();
        SetObjectTree(COctreeNode::Create(DEFAULT_SID, aabb, 0));
    }

    if (bObjs)
    {
        LOADING_TIME_PROFILE_SECTION_NAMED("PostObj");

        if (ppStatObjTable)
        {
            *ppStatObjTable = pStatObjTable;
        }
        else
        {
            SAFE_DELETE(pStatObjTable);
        }

        if (ppMatTable)
        {
            *ppMatTable = pMatTable;
        }
        else
        {
            SAFE_DELETE(pMatTable);
        }
    }

    m_bAreaActivationInUse = (pOctreeChunkHeader->nFlags2 & TCH_FLAG2_AREA_ACTIVATION_IN_USE) != 0;

    AZ_Assert(nNodesLoaded && nDataSize == 0, "nNodesLoaded must be different than zero if bytes left to read is 0.");
    return (nNodesLoaded && nDataSize == 0);
}


void C3DEngine::LoadVegetationData(PodArray<StatInstGroup>& rTable, PodArray<StatInstGroupChunk>& lstFileChunks, int i)
{
    cry_strcpy(rTable[i].szFileName, lstFileChunks[i].szFileName);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fBending);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fSpriteDistRatio);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fLodDistRatio);
    if (rTable[i].fLodDistRatio == 0)
    {
        rTable[i].fLodDistRatio = 1.f; // set default value if it was not exported
    }
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fShadowDistRatio);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fMaxViewDistRatio);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fBrightness);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], nRotationRangeToTerrainNormal);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fAlignToTerrainCoefficient);

    rTable[i].bUseTerrainColor = (lstFileChunks[i].nFlags & SIGC_USETERRAINCOLOR) != 0;
    rTable[i].bAllowIndoor = (lstFileChunks[i].nFlags & SIGC_ALLOWINDOOR) != 0;
    rTable[i].bHideability = (lstFileChunks[i].nFlags & SIGC_HIDEABILITY) != 0;
    rTable[i].bHideabilitySecondary = (lstFileChunks[i].nFlags & SIGC_HIDEABILITYSECONDARY) != 0;
    rTable[i].bAutoMerged = (lstFileChunks[i].nFlags & SIGC_PROCEDURALLYANIMATED) != 0;
    rTable[i].bRecvShadow = (lstFileChunks[i].nFlags & SIGC_RECVSHADOW) != 0;
    rTable[i].bDynamicDistanceShadows = (lstFileChunks[i].nFlags & SIGC_DYNAMICDISTANCESHADOWS) != 0;
    rTable[i].bUseAlphaBlending = (lstFileChunks[i].nFlags & SIGC_USEALPHABLENDING) != 0;
    rTable[i].bRandomRotation = (lstFileChunks[i].nFlags & SIGC_RANDOMROTATION) != 0;

    int nCastShadowMinSpec = (lstFileChunks[i].nFlags & SIGC_CASTSHADOW_MINSPEC_MASK) >> SIGC_CASTSHADOW_MINSPEC_SHIFT;

    bool bCastShadowLegacy = (lstFileChunks[i].nFlags & SIGC_CASTSHADOW) != 0;  // deprecated, should be always false on re-export
    nCastShadowMinSpec = (bCastShadowLegacy && !nCastShadowMinSpec) ? CONFIG_LOW_SPEC : nCastShadowMinSpec;

    rTable[i].nCastShadowMinSpec = (nCastShadowMinSpec) ? nCastShadowMinSpec : END_CONFIG_SPEC_ENUM;

    rTable[i].nPlayerHideable = ((lstFileChunks[i].nFlags & SIGC_PLAYERHIDEABLE_MASK) >> SIGC_PLAYERHIDEABLE_LOWBIT);

    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], nMaterialLayers);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fDensity);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fElevationMax);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fElevationMin);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fSize);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fSizeVar);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fSlopeMax);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fSlopeMin);

    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fStiffness);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fDamping);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fVariance);
    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], fAirResistance);

    COPY_MEMBER_LOAD(&rTable[i], &lstFileChunks[i], m_dwRndFlags);
    rTable[i].nID = lstFileChunks[i].nIDPlusOne - 1;

    int nMinSpec = (rTable[i].m_dwRndFlags & ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT;
    rTable[i].minConfigSpec = (ESystemConfigSpec)nMinSpec;

    if (rTable[i].szFileName[0])
    {
        ReleaseOwnership(rTable[i].pStatObj);
        rTable[i].pStatObj = GetObjManager()->LoadStatObjAutoRef(rTable[i].szFileName, NULL, NULL, !rTable[i].bAutoMerged);//,NULL,NULL,false);
    }

    rTable[i].Update(GetCVars(),GetGeomDetailScreenRes());

    SLICE_AND_SLEEP();
}


bool C3DEngine::GetOctreeCompiledData(byte* pData, int nDataSize, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable, std::vector<struct IStatInstGroup*>** ppStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
# if !ENGINE_ENABLE_COMPILATION
    CryFatalError("serialization code removed, please enable ENGINE_ENABLE_COMPILATION in Cry3DEngine/StdAfx.h");
    return false;
# else

    if (!GetObjectTree() || !GetVisAreaManager())
    {
        //The octree doesn't exist yet.
        return false;
    }

    bool bHMap(!pExportInfo || pExportInfo->nHeigtmap);
    bool bObjs(!pExportInfo || pExportInfo->nObjTypeMask);

    //PrintMessage("Exporting terrain data (%s, %.2f MB) ...",
    //  (bHMap && bObjs) ? "Objects and heightmap" : (bHMap ? "Heightmap" : (bObjs ? "Objects" : "Nothing")), ((float)nDataSize)/1024.f/1024.f);

    // write header
    STerrainChunkHeader* pOctreeChunkHeader = (STerrainChunkHeader*)pData;
    pOctreeChunkHeader->nVersion = OCTREE_CHUNK_VERSION;
    pOctreeChunkHeader->nDummy = 0;

    pOctreeChunkHeader->nFlags = (eEndian == eBigEndian) ? SERIALIZATION_FLAG_BIG_ENDIAN : 0;
    pOctreeChunkHeader->nFlags |= SERIALIZATION_FLAG_SECTOR_PALETTES;

    pOctreeChunkHeader->nFlags2 = (Get3DEngine()->m_bAreaActivationInUse ? TCH_FLAG2_AREA_ACTIVATION_IN_USE : 0);
    pOctreeChunkHeader->nChunkSize = nDataSize;
#ifdef LY_TERRAIN_LEGACY_RUNTIME
    if (m_pTerrain)
    {
        pOctreeChunkHeader->TerrainInfo.nHeightMapSize_InUnits = m_pTerrain->GetSectorSize() * CTerrain::GetSectorsTableSize() / m_pTerrain->GetHeightMapUnitSize();
        pOctreeChunkHeader->TerrainInfo.nUnitSize_InMeters = m_pTerrain->GetHeightMapUnitSize();
        pOctreeChunkHeader->TerrainInfo.nSectorSize_InMeters = m_pTerrain->GetSectorSize();
        pOctreeChunkHeader->TerrainInfo.nSectorsTableSize_InSectors = CTerrain::GetSectorsTableSize();
    }
    else
#endif //#ifdef LY_TERRAIN_LEGACY_RUNTIME
    {

        pOctreeChunkHeader->TerrainInfo.nHeightMapSize_InUnits = 0;
        pOctreeChunkHeader->TerrainInfo.nUnitSize_InMeters = 0;
        pOctreeChunkHeader->TerrainInfo.nSectorSize_InMeters = 0;
        pOctreeChunkHeader->TerrainInfo.nSectorsTableSize_InSectors = 0;
    }

    pOctreeChunkHeader->TerrainInfo.fHeightmapZRatio = 0;
    float fOceanWaterLevel = m_pOcean ? m_pOcean->GetWaterLevel() : WATER_LEVEL_UNKNOWN;
    pOctreeChunkHeader->TerrainInfo.fOceanWaterLevel = (fOceanWaterLevel > WATER_LEVEL_UNKNOWN) ? fOceanWaterLevel : 0;

    SwapEndian(*pOctreeChunkHeader, eEndian);
    UPDATE_PTR_AND_SIZE(pData, nDataSize, sizeof(STerrainChunkHeader));

    std::vector<struct IStatObj*>* pStatObjTable = NULL;
    std::vector< _smart_ptr<IMaterial> >* pMatTable = NULL;
    std::vector<struct IStatInstGroup*>* pStatInstGroupTable = NULL;

    if (bObjs)
    {
        SaveTables(pData, nDataSize, pStatObjTable, pMatTable, pStatInstGroupTable, eEndian, pExportInfo);
    }

    int nNodesLoaded = 1;
#ifdef LY_TERRAIN_LEGACY_RUNTIME
    // get nodes data
    if (m_pTerrain)
    {
        nNodesLoaded = bHMap ? m_pTerrain->GetNodesData(pData, nDataSize, eEndian, pExportInfo) : 1;
    }
#else
    GetEmptyTerrainCompiledData(pData, nDataSize, eEndian);
#endif //#ifdef LY_TERRAIN_LEGACY_RUNTIME


    if (bObjs)
    {
        GetObjectTree()->CleanUpTree();

        GetObjectTree()->GetData(pData, nDataSize, pStatObjTable, pMatTable, pStatInstGroupTable, eEndian, pExportInfo);

        if (ppStatObjTable)
        {
            *ppStatObjTable = pStatObjTable;
        }
        else
        {
            SAFE_DELETE(pStatObjTable);
        }

        if (ppMatTable)
        {
            *ppMatTable = pMatTable;
        }
        else
        {
            SAFE_DELETE(pMatTable);
        }

        if (ppStatInstGroupTable)
        {
            *ppStatInstGroupTable = pStatInstGroupTable;
        }
        else
        {
            SAFE_DELETE(pStatInstGroupTable);
        }
    }

    //PrintMessagePlus(" done in %.2f sec", GetCurAsyncTimeSec()-fStartTime );

    AZ_Assert(nNodesLoaded&& nDataSize == 0, "nNodesLoaded must be different than zero if bytes left to read is 0.");
    return (nNodesLoaded && nDataSize == 0);
# endif
}

void C3DEngine::SaveTables(byte*& pData, int& nDataSize, std::vector<struct IStatObj*>*& pStatObjTable, std::vector<_smart_ptr<IMaterial> >*& pMatTable, std::vector<IStatInstGroup*>*& pStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
    pMatTable = new std::vector < _smart_ptr<IMaterial> >;
    pStatObjTable = new std::vector < struct IStatObj* >;
    pStatInstGroupTable = new std::vector < struct IStatInstGroup* >;

    GetVegetationMaterials(pMatTable);

    Get3DEngine()->GetObjectTree()->GenerateStatObjAndMatTables(pStatObjTable, pMatTable, pStatInstGroupTable, pExportInfo);
    GetVisAreaManager()->GenerateStatObjAndMatTables(pStatObjTable, pMatTable, pStatInstGroupTable, pExportInfo);

    {
        { // get vegetation objects count
            std::vector<IStatInstGroup*>& rTable = *pStatInstGroupTable;
            int nObjectsCount = rTable.size();

            // prepare temporary chunks array for saving
            PodArray<StatInstGroupChunk> lstFileChunks;
            lstFileChunks.PreAllocate(nObjectsCount, nObjectsCount);

            // init struct values and load cgf's
            for (uint32 i = 0; i < rTable.size(); i++)
            {
                cry_strcpy(lstFileChunks[i].szFileName, rTable[i]->szFileName);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fBending);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fSpriteDistRatio);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fLodDistRatio);
                if (lstFileChunks[i].fLodDistRatio < 0.001f) // not allow to export value of 0 because it would mean that variable is not set
                {
                    lstFileChunks[i].fLodDistRatio = 0.001f;
                }
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fShadowDistRatio);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fMaxViewDistRatio);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fBrightness);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], nRotationRangeToTerrainNormal);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fAlignToTerrainCoefficient);

                lstFileChunks[i].nFlags = 0;
                if (rTable[i]->bUseTerrainColor)
                {
                    lstFileChunks[i].nFlags |= SIGC_USETERRAINCOLOR;
                }
                if (rTable[i]->bAllowIndoor)
                {
                    lstFileChunks[i].nFlags |= SIGC_ALLOWINDOOR;
                }
                if (rTable[i]->bHideability)
                {
                    lstFileChunks[i].nFlags |= SIGC_HIDEABILITY;
                }
                if (rTable[i]->bHideabilitySecondary)
                {
                    lstFileChunks[i].nFlags |= SIGC_HIDEABILITYSECONDARY;
                }
                if (rTable[i]->bAutoMerged)
                {
                    lstFileChunks[i].nFlags |= SIGC_PROCEDURALLYANIMATED;
                }
                if (rTable[i]->bRecvShadow)
                {
                    lstFileChunks[i].nFlags |= SIGC_RECVSHADOW;
                }
                if (rTable[i]->bDynamicDistanceShadows)
                {
                    lstFileChunks[i].nFlags |= SIGC_DYNAMICDISTANCESHADOWS;
                }
                if (rTable[i]->bUseAlphaBlending)
                {
                    lstFileChunks[i].nFlags |= SIGC_USEALPHABLENDING;
                }
                if (rTable[i]->bRandomRotation)
                {
                    lstFileChunks[i].nFlags |= SIGC_RANDOMROTATION;
                }

                lstFileChunks[i].nFlags |= ((rTable[i]->nCastShadowMinSpec << SIGC_CASTSHADOW_MINSPEC_SHIFT) & SIGC_CASTSHADOW_MINSPEC_MASK);
                lstFileChunks[i].nFlags |= ((rTable[i]->nPlayerHideable << SIGC_PLAYERHIDEABLE_LOWBIT) & SIGC_PLAYERHIDEABLE_MASK);

                lstFileChunks[i].nMaterialId = -1;
                if (rTable[i]->pMaterial)
                {
                    lstFileChunks[i].nMaterialId = CObjManager::GetItemId(pMatTable, (_smart_ptr<IMaterial>)rTable[i]->pMaterial, false);
                    if (lstFileChunks[i].nMaterialId < 0)
                    {
                        lstFileChunks[i].nMaterialId = pMatTable->size();
                        pMatTable->push_back(rTable[i]->pMaterial);
                    }
                }

                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], nMaterialLayers);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fDensity);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fElevationMax);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fElevationMin);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fSize);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fSizeVar);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fSlopeMax);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fSlopeMin);

                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], m_dwRndFlags);

                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fStiffness);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fDamping);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fVariance);
                COPY_MEMBER_SAVE(&lstFileChunks[i], rTable[i], fAirResistance);

                lstFileChunks[i].nIDPlusOne = rTable[i]->nID + 1;
            }

            // get count
            AddToPtr(pData, nDataSize, nObjectsCount, eEndian);

            // get table content
            for (int i = 0; i < lstFileChunks.Count(); i++)
            {
                AddToPtr(pData, nDataSize, lstFileChunks[i], eEndian);
            }
        }

        { // get brush objects count
            int nObjectsCount = pStatObjTable->size();
            AddToPtr(pData, nDataSize, nObjectsCount, eEndian);

            PodArray<SNameChunk> lstFileChunks;
            lstFileChunks.PreAllocate(pStatObjTable->size(), pStatObjTable->size());

            for (uint32 i = 0; i < pStatObjTable->size(); i++)
            {
                if ((*pStatObjTable)[i])
                {
                    cry_strcpy(lstFileChunks[i].szFileName, (*pStatObjTable)[i]->GetFilePath());
                }
                else
                {
                    lstFileChunks[i].szFileName[0] = 0;
                }
                AddToPtr(pData, nDataSize, lstFileChunks[i], eEndian);
            }
        }

        { // get brush materials count
            std::vector<_smart_ptr<IMaterial> >& rTable = *pMatTable;
            int nObjectsCount = rTable.size();

            // count
            AddToPtr(pData, nDataSize, nObjectsCount, eEndian);

            // get table content
            for (int dwI = 0; dwI < nObjectsCount; ++dwI)
            {
                SNameChunk tmp;
                AZ_Assert(strlen(rTable[dwI] ? rTable[dwI]->GetName() : "") < sizeof(tmp.szFileName), "table name is too large.");
                azstrcpy(tmp.szFileName, AZ_ARRAY_SIZE(tmp.szFileName), rTable[dwI] ? rTable[dwI]->GetName() : "");
                AddToPtr(pData, nDataSize, tmp, eEndian);
            }
        }
    }
}

void C3DEngine::GetVegetationMaterials(std::vector<_smart_ptr<IMaterial> >*& pMatTable)
{
    if (!pMatTable)
    {
        return;
    }

    { // get vegetation objects materials
        PodArray<StatInstGroup>& rTable = GetObjManager()->GetListStaticTypes()[0];
        int nObjectsCount = rTable.size();

        // init struct values and load cgf's
        for (uint32 i = 0; i < rTable.size(); i++)
        {
            int nMaterialId = -1;
            if (rTable[i].pMaterial)
            {
                nMaterialId = CObjManager::GetItemId(pMatTable, (_smart_ptr<IMaterial>)rTable[i].pMaterial, false);
                if (nMaterialId < 0)
                {
                    nMaterialId = pMatTable->size();
                    pMatTable->push_back(rTable[i].pMaterial);
                }
            }
        }
    }
}

int C3DEngine::GetTablesSize(SHotUpdateInfo* pExportInfo)
{
    int nDataSize = 0;

    nDataSize += sizeof(int);

    // get brush objects table size
    std::vector<IStatObj*> brushTypes;
    std::vector<_smart_ptr<IMaterial> > usedMats;
    std::vector<IStatInstGroup*> instGroups;

    std::vector<_smart_ptr<IMaterial> >* pMatTable = &usedMats;
    GetVegetationMaterials(pMatTable);
    if (IsObjectTreeReady())
    {
        GetObjectTree()->GenerateStatObjAndMatTables(&brushTypes, &usedMats, &instGroups, pExportInfo);
    }
    GetVisAreaManager()->GenerateStatObjAndMatTables(&brushTypes, &usedMats, &instGroups, pExportInfo);

    nDataSize += instGroups.size() * sizeof(StatInstGroupChunk);

    nDataSize += sizeof(int);
    nDataSize += brushTypes.size() * sizeof(SNameChunk);
    brushTypes.clear();

    // get custom materials table size
    nDataSize += sizeof(int);
    nDataSize += usedMats.size() * sizeof(SNameChunk);

    return nDataSize;
}


int C3DEngine::GetOctreeCompiledDataSize(SHotUpdateInfo* pExportInfo)
{
# if !ENGINE_ENABLE_COMPILATION
    CryFatalError("serialization code removed, please enable ENGINE_ENABLE_COMPILATION in Cry3DEngine/StdAfx.h");
    return 0;
# else

    if (!GetObjectTree() || !GetVisAreaManager())
    {
        //The octree doesn't exist yet.
        return 0;
    }

    int nDataSize = 0;
    byte* pData = NULL;

    bool bHMap(!pExportInfo || pExportInfo->nHeigtmap);
    bool bObjs(!pExportInfo || pExportInfo->nObjTypeMask);

    if (bObjs)
    {
        GetObjectTree()->CleanUpTree();
        GetObjectTree()->GetData(pData, nDataSize, NULL, NULL, NULL, eLittleEndian, pExportInfo);
    }

    if (bHMap)
    {
#ifdef LY_TERRAIN_LEGACY_RUNTIME
        if (m_pTerrain)
        {
            m_pTerrain->GetNodesData(pData, nDataSize, GetPlatformEndian(), pExportInfo);
        }
#else
        GetEmptyTerrainCompiledData(pData, nDataSize, GetPlatformEndian());
#endif //#ifdef LY_TERRAIN_LEGACY_RUNTIME
    }

    // get header size
    nDataSize += sizeof(STerrainChunkHeader);

    // get vegetation objects table size
    if (bObjs)
    {
        nDataSize += GetTablesSize(pExportInfo);
    }

    return nDataSize;
# endif
}



bool C3DEngine::SetOctreeCompiledData(byte* pData, int nDataSize, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable, bool bHotUpdate, SHotUpdateInfo* pExportInfo, bool loadTerrainMacroTexture)
{
    STerrainChunkHeader* pOctreeChunkHeader = (STerrainChunkHeader*)pData;
    SwapEndian(*pOctreeChunkHeader, eLittleEndian);

    pData += sizeof(STerrainChunkHeader);
    nDataSize -= sizeof(STerrainChunkHeader);
    return LoadOctreeInternal_T(XmlNodeRef(static_cast<IXmlNode*>(nullptr)), pData, nDataSize, pOctreeChunkHeader, ppStatObjTable, ppMatTable, bHotUpdate, pExportInfo, loadTerrainMacroTexture);
}


#define RAD2BYTE(x) ((x)* 255.0f / float(g_PI2))
#define BYTE2RAD(x) ((x)* float(g_PI2) / 255.0f)

IRenderNode* C3DEngine::AddVegetationInstance(int nStaticGroupID, const Vec3& vPos, const float fScale, uint8 ucBright,
    uint8 angle, uint8 angleX, uint8 angleY)
{
    // Don't add instances that are extremely tiny in scale
    if (fScale * VEGETATION_CONV_FACTOR < 1.f)
    {
        return 0;
    }

#ifdef LY_TERRAIN_LEGACY_RUNTIME
    if (m_pTerrain)
    {
        if (vPos.x <= 0 || vPos.y <= 0 || vPos.x >= CTerrain::GetTerrainSize() || vPos.y >= CTerrain::GetTerrainSize())
        {
            return 0;
        }
    }
#endif //#ifdef LY_TERRAIN_LEGACY_RUNTIME

    IRenderNode* renderNode = NULL;

    AZ_Assert(DEFAULT_SID >= 0 && DEFAULT_SID < GetObjManager()->GetListStaticTypes().Count(), "DEFAULT_SID is out of range.");

    if (nStaticGroupID < 0 || nStaticGroupID >= GetObjManager()->GetListStaticTypes()[DEFAULT_SID].Count())
    {
        return 0;
    }

    AZ::Aabb aabb;
    StatInstGroup& group = GetObjManager()->GetListStaticTypes()[DEFAULT_SID][nStaticGroupID];
    if (!group.GetStatObj())
    {
        Warning("I3DEngine::AddStaticObject: Attempt to add object of undefined type");
        return 0;
    }
    if (!group.bAutoMerged)
    {
        CVegetation* pEnt = (CVegetation*)Get3DEngine()->CreateRenderNode(eERType_Vegetation);
        pEnt->SetScale(fScale);
        pEnt->m_vPos = vPos;
        pEnt->SetStatObjGroupIndex(nStaticGroupID);
        pEnt->m_ucAngle = angle;
        pEnt->m_ucAngleX = angleX;
        pEnt->m_ucAngleY = angleY;
        aabb = LyAABBToAZAabb(pEnt->CalcBBox());

        float fEntLengthSquared = pEnt->GetBBox().GetSize().GetLengthSquared();
        if (fEntLengthSquared > MAX_VALID_OBJECT_VOLUME || !_finite(fEntLengthSquared) || fEntLengthSquared <= 0)
        {
            Warning("CTerrain::AddVegetationInstance: Object has invalid bbox: %s,%s, GetRadius() = %.2f",
                pEnt->GetName(), pEnt->GetEntityClassName(), sqrt_tpl(fEntLengthSquared) * 0.5f);
        }

        pEnt->Physicalize();
        RegisterEntity(pEnt);
        renderNode = pEnt;
    }
    else
    {
        SProcVegSample sample;
        sample.InstGroupId = nStaticGroupID;
        sample.pos = vPos;
        sample.scale = (uint8)SATURATEB(fScale * VEGETATION_CONV_FACTOR);
        Matrix33 mr = Matrix33::CreateRotationXYZ(Ang3(BYTE2RAD(angleX), BYTE2RAD(angleY), BYTE2RAD(angle)));

        if (group.GetAlignToTerrainAmount() != 0.f)
        {
            Matrix33 m33;
#ifdef LY_TERRAIN_LEGACY_RUNTIME
            GetTerrain()->GetTerrainAlignmentMatrix(vPos, group.GetAlignToTerrainAmount(), m33);
#else
            m33 = Matrix33::CreateIdentity();
#endif
            sample.q = Quat(m33) * Quat(mr);
        }
        else
        {
            sample.q = Quat(mr);
        }
        sample.q.NormalizeSafe();
        renderNode = m_pMergedMeshesManager->AddInstance(sample);

        AZ::Transform transform = LYTransformToAZTransform(mr) * AZ::Transform::CreateScale(AZ::Vector3(fScale));
        transform.SetTranslation(LYVec3ToAZVec3(sample.pos));

        aabb = LyAABBToAZAabb(group.GetStatObj()->GetAABB());
        aabb.ApplyTransform(transform);
    }

    Vegetation::StaticVegetationNotificationBus::Broadcast(&Vegetation::StaticVegetationNotificationBus::Events::InstanceAdded, renderNode, aabb);

    return renderNode;
}

#include "TypeInfo_impl.h"
#include "vegetation_compile_info.h"

