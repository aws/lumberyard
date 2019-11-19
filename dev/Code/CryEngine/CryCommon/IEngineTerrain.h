#ifndef _Cry3DEngine_IEngineTerrain_h_
#define _Cry3DEngine_IEngineTerrain_h_
#pragma once

#include "ITerrain.h"
#include <ISerialize.h>
#include <TerrainFactory.h>

#include <Terrain/Texture/MacroTexture.h>
#include <Terrain/Texture/TexturePool.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <AzCore/std/function/function_fwd.h> // for callbacks
#include "Environment/OceanEnvironmentBus.h"

class COcean;
class CTerrainUpdateDispatcher;

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

class IEngineTerrain:public ITerrain
{
public:
    using Meter=int;
    using MeterF=float;
    using Unit=int;

    virtual ~IEngineTerrain() {}

    virtual bool HasRootNode() = 0;
    virtual CTerrainNode* GetRootNode()=0;
    virtual AABB GetRootBBoxVirtual()=0;
//    virtual CTerrainNode *GetLeafNodeAt(Meter x, Meter y)=0;

    //stats
    virtual int GetTerrainSize() const = 0;
    virtual int GetSectorSize() const = 0;
    virtual int GetHeightMapUnitSize() const = 0;
    virtual int GetTerrainTextureNodeSizeMeters() = 0;
    virtual void GetResourceMemoryUsage(ICrySizer* pSizer, const AABB &crstAABB) = 0;

//    virtual const int GetUnitToSectorBitShift() const=0;
//    virtual const int GetMeterToUnitBitShift() const=0;
    //edit
    virtual float GetBilinearZ(MeterF xWS, MeterF yWS) const = 0;
    virtual float GetZ(Meter x, Meter y) const = 0;
    virtual bool IsHole(Meter x, Meter y) const = 0;
//    virtual bool Recompile_Modified_Incrementaly_RoadRenderNodes() = 0;
//    virtual void RemoveAllStaticObjects() = 0;
//    virtual bool RemoveObjectsInArea(Vec3 vExploPos, float fExploRadius)=0;
    virtual void MarkAllSectorsAsUncompiled()=0;
    //vis
    virtual void CheckVis(const SRenderingPassInfo &passInfo) = 0;
    virtual void AddVisSector(CTerrainNode* newsec)=0;
    virtual void ClearVisSectors() = 0;
    virtual void UpdateNodesIncrementaly(const SRenderingPassInfo &passInfo) = 0;

    virtual void DrawVisibleSectors(const SRenderingPassInfo &passInfo) = 0;
    virtual void UpdateSectorMeshes(const SRenderingPassInfo &passInfo) = 0;
    virtual void CheckNodesGeomUnload(const SRenderingPassInfo &passInfo) = 0;

    virtual bool RenderArea(Vec3 vPos, float fRadius, _smart_ptr<IRenderMesh> &pRenderMesh,
        CRenderObject *pObj, _smart_ptr<IMaterial> pMaterial, const char *szComment, float *pCustomData,
        Plane *planes, const SRenderingPassInfo &passInfo)=0;
    virtual void IntersectWithShadowFrustum(PodArray<CTerrainNode*>* plstResult, ShadowMapFrustum* pFrustum, const SRenderingPassInfo& passInfo)=0;
    virtual void IntersectWithBox(const AABB& aabbBox, PodArray<CTerrainNode*>* plstResult)=0;
    virtual CTerrainNode* FindMinNodeContainingBox(const AABB& someBox)=0;

    virtual bool RayTrace(Vec3 const& vStart, Vec3 const& vEnd, SRayTrace* prt)=0;
    //ocean
    virtual bool IsOceanVisible() const = 0;
    virtual COcean* GetOcean()=0;
    virtual float GetWaterLevel() = 0;
    virtual float GetDistanceToSectorWithWater() const = 0;
    virtual int UpdateOcean(const SRenderingPassInfo &passInfo) = 0;
    virtual int RenderOcean(const SRenderingPassInfo &passInfo) = 0;
//    virtual void InitTerrainWater(_smart_ptr<IMaterial> pTerrainWaterShader)=0;
    
    //texture
    virtual void ClearTextureSets() = 0;
    virtual bool IsTextureStreamingInProgress() const = 0;
    virtual bool TryGetTextureStatistics(MacroTexture::TileStatistics &statistics) const = 0;
//    virtual MacroTexture *GetMacroTexture()=0;
//    virtual int GetWhiteTexId() const=0;

    //terrain
    virtual void CloseTerrainTextureFile() = 0;
    virtual void SetTerrainSectorTexture(int nTexSectorX, int nTexSectorY, unsigned int textureId, bool bMergeNotAllowed) = 0;
//    virtual Vec3 GetTerrainSurfaceNormal(Vec3 vPos, float fRange) = 0;
//    virtual bool IsMeshQuadFlipped(const Meter x, const Meter y, const Meter nUnitSize) const=0;
//    virtual void GetTerrainAlignmentMatrix(const Vec3& vPos, const float amount, Matrix33& matrix33)=0;
//    virtual ITerrain::SurfaceWeight GetSurfaceWeight(Meter x, Meter y) const=0;

//    virtual CTerrainUpdateDispatcher *GetTerrainUpdateDispatcher() const=0;
    //streams
    virtual int GetActiveProcObjNodesCount() = 0;
//    virtual void ActivateNodeProcObj(CTerrainNode* pNode)=0;
//    virtual void SetNodePyramid(int treeLevel, int x, int y, CTerrainNode *node)=0;

    //
    virtual void LoadSurfaceTypesFromXML(XmlNodeRef pDoc)=0;
    virtual void UpdateSurfaceTypes()=0;

    //
    virtual void InitHeightfieldPhysics()=0;
    virtual void ResetTerrainVertBuffers()=0;

    //
    virtual bool LoadHandle(AZ::IO::HandleType fileHandle, int nDataSize, STerrainChunkHeader* pTerrainChunkHeader, STerrainInfo* pTerrainInfo, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable)=0;
//    virtual bool Load(const char *, STerrainInfo *)=0;

    virtual void RemoveAllStaticObjects()=0;
    virtual bool RemoveObjectsInArea(Vec3 vExploPos, float fExploRadius)=0;
    virtual void GetObjectsAround(Vec3 vPos, float fRadius, PodArray<struct SRNInfo>* pEntList, bool bSkip_ERF_NO_DECALNODE_DECALS, bool bSkipDynamicObjects)=0;
    virtual bool Recompile_Modified_Incrementaly_RoadRenderNodes()=0;
};

#endif//_Cry3DEngine_IEngineTerrain_h_