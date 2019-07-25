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

#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/DependencyRequestBus.h>
#include <Vegetation/Ebuses/DescriptorProviderRequestBus.h>
#include <Vegetation/Ebuses/DescriptorSelectorRequestBus.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/Ebuses/ModifierRequestBus.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <AzCore/Math/Random.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/set.h>
#include <Source/AreaSystemComponent.h>
#include <Source/InstanceSystemComponent.h>

// used for the mock for IStatObj
#ifndef CRYINCLUDE_CRY3DENGINE_STATOBJ_H
struct SPhysGeomArray
{
};
#endif

//////////////////////////////////////////////////////////////////////////
// mock event bus classes for testing vegetation
namespace UnitTest
{
    struct MockAreaManager
        : public Vegetation::AreaSystemRequestBus::Handler
    {
        mutable int m_count = 0;

        MockAreaManager()
        {
            Vegetation::AreaSystemRequestBus::Handler::BusConnect();
        }

        ~MockAreaManager()
        {
            Vegetation::AreaSystemRequestBus::Handler::BusDisconnect();
        }

        void RegisterArea(AZ::EntityId areaId) override
        {
            ++m_count;
        }

        void UnregisterArea(AZ::EntityId areaId) override
        {
            ++m_count;
        }

        void RefreshArea(AZ::EntityId areaId) override
        {
            ++m_count;
        }

        void RefreshAllAreas() override
        {
            ++m_count;
        }

        void ClearAllAreas() override
        {
            ++m_count;
        }

        void MuteArea(AZ::EntityId areaId) override
        {
            ++m_count;
        }

        void UnmuteArea(AZ::EntityId areaId) override
        {
            ++m_count;
        }

        AZStd::vector<Vegetation::InstanceData> m_existingInstances;
        void EnumerateInstancesInAabb(const AZ::Aabb& bounds, Vegetation::AreaSystemEnumerateCallback callback) const override
        {
            ++m_count;
            for (const auto& instanceData : m_existingInstances)
            {
                if (callback(instanceData) != Vegetation::AreaSystemEnumerateCallbackResult::KeepEnumerating)
                {
                    return;
                }
            }
        }
    };

    struct MockInstanceSystemRequestBus
        : public Vegetation::InstanceSystemRequestBus::Handler
    {
        int m_count = 0;
        int m_created = 0;
        int m_destroyed = 0;

        MockInstanceSystemRequestBus()
        {
            Vegetation::InstanceSystemRequestBus::Handler::BusConnect();
        }

        ~MockInstanceSystemRequestBus() override
        {
            Vegetation::InstanceSystemRequestBus::Handler::BusDisconnect();
        }

        Vegetation::DescriptorPtr RegisterUniqueDescriptor(const Vegetation::Descriptor& descriptor) override
        {
            m_count++;
            return Vegetation::DescriptorPtr();
        }

        void ReleaseUniqueDescriptor(Vegetation::DescriptorPtr descriptorPtr) override
        {
            m_count++;
        }

        void CreateInstance(Vegetation::InstanceData& instanceData) override
        {
            m_created++;
            m_count++;
            instanceData.m_instanceId = Vegetation::InstanceId();
        }

        void DestroyInstance(Vegetation::InstanceId instanceId) override
        {
            m_destroyed++;
            m_count++;
        }

        void DestroyAllInstances() override
        {
            m_destroyed = m_created;
            m_count++;
        }

        void Cleanup() override
        {

        }
    };

    struct MockDescriptorBus
        : public Vegetation::InstanceSystemRequestBus::Handler
    {
        AZStd::set<Vegetation::DescriptorPtr> m_descriptorSet;

        MockDescriptorBus()
        {
            Vegetation::InstanceSystemRequestBus::Handler::BusConnect();
        }

        ~MockDescriptorBus() override
        {
            Vegetation::InstanceSystemRequestBus::Handler::BusDisconnect();
        }

        Vegetation::DescriptorPtr RegisterUniqueDescriptor(const Vegetation::Descriptor& descriptor) override
        {
            Vegetation::DescriptorPtr sharedPtr = AZStd::make_shared<Vegetation::Descriptor>(descriptor);
            m_descriptorSet.insert(sharedPtr);
            return sharedPtr;
        }

        void ReleaseUniqueDescriptor(Vegetation::DescriptorPtr descriptorPtr) override
        {
            m_descriptorSet.erase(descriptorPtr);
        }

        void CreateInstance(Vegetation::InstanceData& instanceData) override
        {
            instanceData.m_instanceId = Vegetation::InstanceId();
        }

        void DestroyInstance(Vegetation::InstanceId instanceId) override {}

        void DestroyAllInstances() override {}

        void Cleanup() override {};
    };

    struct MockGradientRequestHandler
        : public GradientSignal::GradientRequestBus::Handler
    {
        mutable int m_count = 0;
        AZStd::function<float()> m_valueGetter;
        float m_defaultValue = -AZ_FLT_MAX;
        AZ::Entity m_entity;

        MockGradientRequestHandler()
        {
            GradientSignal::GradientRequestBus::Handler::BusConnect(m_entity.GetId());
        }

        ~MockGradientRequestHandler()
        {
            GradientSignal::GradientRequestBus::Handler::BusDisconnect();
        }

        float GetValue(const GradientSignal::GradientSampleParams& sampleParams) const override
        {
            ++m_count;

            if (m_valueGetter)
            {
                return m_valueGetter();
            }
            return m_defaultValue;
        }

        bool IsEntityInHierarchy(const AZ::EntityId &) const override
        {
            return false;
        }
    };

    struct MockShapeComponentNotificationsBus
        : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
        AZ::Aabb m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), AZ_FLT_MAX);

        MockShapeComponentNotificationsBus()
        {
        }

        ~MockShapeComponentNotificationsBus() override
        {
        }

        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override
        {
            transform = AZ::Transform::CreateTranslation(m_aabb.GetCenter());
            bounds = m_aabb;
        }

        AZ::Crc32 GetShapeType() override
        {
            return AZ::Crc32();
        }

        AZ::Aabb GetEncompassingAabb() override
        {
            return m_aabb;
        }

        bool IsPointInside(const AZ::Vector3& point) override
        {
            return m_aabb.Contains(point);
        }

        float DistanceSquaredFromPoint(const AZ::Vector3& point) override
        {
            return m_aabb.GetDistanceSq(point);
        }
    };

    struct MockSystemConfigurationRequestBus
        : public Vegetation::SystemConfigurationRequestBus::Handler
    {
        AZ::ComponentConfig* m_lastUpdated = nullptr;
        mutable const AZ::ComponentConfig* m_lastRead = nullptr;
        Vegetation::AreaSystemConfig m_areaSystemConfig;
        Vegetation::InstanceSystemConfig m_instanceSystemConfig;

        MockSystemConfigurationRequestBus()
        {
            Vegetation::SystemConfigurationRequestBus::Handler::BusConnect();
        }

        ~MockSystemConfigurationRequestBus() override
        {
            Vegetation::SystemConfigurationRequestBus::Handler::BusDisconnect();
        }

        void UpdateSystemConfig(const AZ::ComponentConfig* config) override
        {
            if (azrtti_typeid(m_areaSystemConfig) == azrtti_typeid(*config))
            {
                m_areaSystemConfig = *azrtti_cast<const Vegetation::AreaSystemConfig*>(config);
                m_lastUpdated = &m_areaSystemConfig;
            }
            else if (azrtti_typeid(m_instanceSystemConfig) == azrtti_typeid(*config))
            {
                m_instanceSystemConfig = *azrtti_cast<const Vegetation::InstanceSystemConfig*>(config);
                m_lastUpdated = &m_instanceSystemConfig;
            }
            else
            {
                m_lastUpdated = nullptr;
                AZ_Error("vegetation", false, "Invalid AZ::ComponentConfig type updated");
            }
        }

        void GetSystemConfig(AZ::ComponentConfig* config) const
        {
            if (azrtti_typeid(m_areaSystemConfig) == azrtti_typeid(*config))
            {
                *azrtti_cast<Vegetation::AreaSystemConfig*>(config) = m_areaSystemConfig;
                m_lastRead = azrtti_cast<const Vegetation::AreaSystemConfig*>(&m_areaSystemConfig);
            }
            else if (azrtti_typeid(m_instanceSystemConfig) == azrtti_typeid(*config))
            {
                *azrtti_cast<Vegetation::InstanceSystemConfig*>(config) = m_instanceSystemConfig;
                m_lastRead = azrtti_cast<const Vegetation::InstanceSystemConfig*>(&m_instanceSystemConfig);
            }
            else
            {
                m_lastRead = nullptr;
                AZ_Error("vegetation", false, "Invalid AZ::ComponentConfig type read");
            }
        }
    };

    template<class T>
    struct MockAsset : public AZ::Data::Asset<T>
    {
        void ClearData()
        {
            m_assetData = nullptr;
        }
    };

    struct MockAssetData
        : public AZ::Data::AssetData
    {
        void SetId(const AZ::Data::AssetId& assetId)
        {
            m_assetId = assetId;
            m_status.store((int)AZ::Data::AssetData::AssetStatus::Ready);
        }
    };

    class MockShape
        : public LmbrCentral::ShapeComponentRequestsBus::Handler
    {
    public:
        AZ::Entity m_entity;
        mutable int m_count = 0;

        MockShape()
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(m_entity.GetId());
        }

        ~MockShape()
        {
            LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        }

        AZ::Crc32 GetShapeType() override
        {
            ++m_count;
            return AZ_CRC("TestShape", 0x856ca50c);
        }

        AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
        AZ::Aabb GetEncompassingAabb() override
        {
            ++m_count;
            return m_aabb;
        }

        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity();
        AZ::Aabb m_localBounds = AZ::Aabb::CreateNull();
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override
        {
            ++m_count;
            transform = m_localTransform;
            bounds = m_localBounds;
        }

        bool m_pointInside = true;
        bool IsPointInside(const AZ::Vector3& point) override
        {
            ++m_count;
            return m_pointInside;
        }

        float m_distanceSquaredFromPoint = 0.0f;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override
        {
            ++m_count;
            return m_distanceSquaredFromPoint;
        }

        AZ::Vector3 m_randomPointInside = AZ::Vector3::CreateZero();
        AZ::Vector3 GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution) override
        {
            ++m_count;
            return m_randomPointInside;
        }

        bool m_intersectRay = false;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance) override
        {
            ++m_count;
            return m_intersectRay;
        }
    };

    struct MockSurfaceHandler
        : public SurfaceData::SurfaceDataSystemRequestBus::Handler
    {
    public:
        mutable int m_count = 0;

        MockSurfaceHandler()
        {
            SurfaceData::SurfaceDataSystemRequestBus::Handler::BusConnect();
        }

        ~MockSurfaceHandler()
        {
            SurfaceData::SurfaceDataSystemRequestBus::Handler::BusDisconnect();
        }

        AZ::Vector3 m_outPosition = {};
        AZ::Vector3 m_outNormal = {};
        SurfaceData::SurfaceTagWeightMap m_outMasks;
        void GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceData::SurfaceTagVector& masks, SurfaceData::SurfacePointList& surfacePointList) const override
        {
            ++m_count;
            SurfaceData::SurfacePoint outPoint;
            outPoint.m_position = m_outPosition;
            outPoint.m_normal = m_outNormal;
            SurfaceData::AddMaxValueForMasks(outPoint.m_masks, m_outMasks);
            surfacePointList.push_back(outPoint);
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
            return SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        void UnregisterSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
            ++m_count;
        }

        void UpdateSurfaceDataProvider(const SurfaceData::SurfaceDataRegistryHandle& handle, const SurfaceData::SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride) override
        {
            ++m_count;
        }

        SurfaceData::SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryEntry& entry) override
        {
            ++m_count;
            return SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        void UnregisterSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryHandle& handle) override
        {
            ++m_count;
        }

        void UpdateSurfaceDataModifier(const SurfaceData::SurfaceDataRegistryHandle& handle, const SurfaceData::SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride) override
        {
            ++m_count;
        }
    };

    // wow IStatObj has a large interface
    struct MockStatObj
        : public IStatObj
    {
        int GetStreamableContentMemoryUsage(bool bJustForDebug = false) override
        {
            return 0;
        }
        void ReleaseStreamableContent() override
        {
        }
        void GetStreamableName(string & sName) override
        {
        }
        uint32 GetLastDrawMainFrameId() override
        {
            return uint32();
        }
        int AddRef() override
        {
            return 0;
        }
        int Release() override
        {
            return 0;
        }
        void SetFlags(int nFlags) override
        {
        }
        int GetFlags() const override
        {
            return 0;
        }
        void SetDefaultObject(bool state) override
        {
        }
        unsigned int GetVehicleOnlyPhysics() override
        {
            return 0;
        }
        int GetIDMatBreakable() override
        {
            return 0;
        }
        unsigned int GetBreakableByGame() override
        {
            return 0;
        }
        IIndexedMesh * GetIndexedMesh(bool bCreateIfNone = false) override
        {
            return nullptr;
        }
        IIndexedMesh * CreateIndexedMesh() override
        {
            return nullptr;
        }
        IRenderMesh * GetRenderMesh() override
        {
            return nullptr;
        }
        phys_geometry * GetPhysGeom(int nType = PHYS_GEOM_TYPE_DEFAULT) override
        {
            return nullptr;
        }
        int PhysicalizeFoliage(IPhysicalEntity * pTrunk, const Matrix34 & mtxWorld, IFoliage *& pRes, float lifeTime = 0.0f, int iSource = 0) override
        {
            return 0;
        }
        IStatObj * UpdateVertices(strided_pointer<Vec3> pVtx, strided_pointer<Vec3> pNormals, int iVtx0, int nVtx, int * pVtxMap = 0, float rscale = 1.f) override
        {
            return nullptr;
        }
        IStatObj * SkinVertices(strided_pointer<Vec3> pSkelVtx, const Matrix34 & mtxSkelToMesh) override
        {
            return nullptr;
        }
        void CopyFoliageData(IStatObj * pObjDst, bool bMove = false, IFoliage * pSrcFoliage = 0, int * pVtxMap = 0, primitives::box * pMovedBoxes = 0, int nMovedBoxes = -1) override
        {
        }
        void SetPhysGeom(phys_geometry * pPhysGeom, int nType = 0) override
        {
        }
        ITetrLattice * GetTetrLattice() override
        {
            return nullptr;
        }
        float GetAIVegetationRadius() const override
        {
            return 0.0f;
        }
        void SetAIVegetationRadius(float radius) override
        {
        }
        void SetMaterial(_smart_ptr<IMaterial> pMaterial) override
        {
        }
        _smart_ptr<IMaterial> GetMaterial() override
        {
            return _smart_ptr<IMaterial>();
        }
        const _smart_ptr<IMaterial> GetMaterial() const override
        {
            return _smart_ptr<IMaterial>();
        }
        Vec3 GetBoxMin() override
        {
            return Vec3();
        }
        Vec3 GetBoxMax() override
        {
            return Vec3();
        }
        const Vec3 GetVegCenter() override
        {
            return Vec3();
        }
        void SetBBoxMin(const Vec3 & vBBoxMin) override
        {
        }
        void SetBBoxMax(const Vec3 & vBBoxMax) override
        {
        }
        float GetRadius() override
        {
            return 0.0f;
        }
        void Refresh(int nFlags) override
        {
        }
        void Render(const SRendParams & rParams, const SRenderingPassInfo & passInfo) override
        {
        }
        AABB GetAABB() override
        {
            return AABB();
        }
        float GetExtent(EGeomForm eForm) override
        {
            return 0.0f;
        }
        void GetRandomPos(PosNorm & ran, EGeomForm eForm) const override
        {
        }
        IStatObj * GetLodObject(int nLodLevel, bool bReturnNearest = false) override
        {
            return nullptr;
        }
        IStatObj * GetLowestLod() override
        {
            return nullptr;
        }
        int FindNearesLoadedLOD(int nLodIn, bool bSearchUp = false) override
        {
            return 0;
        }
        int FindHighestLOD(int nBias) override
        {
            return 0;
        }
        bool LoadCGF(const char * filename, bool bLod, unsigned long nLoadingFlags, const void * pData, const int nDataSize) override
        {
            return false;
        }
        void DisableStreaming() override
        {
        }
        void TryMergeSubObjects(bool bFromStreaming) override
        {
        }
        bool IsUnloadable() const override
        {
            return false;
        }
        void SetCanUnload(bool value) override
        {
        }
        string m_GetFileName;
        string & GetFileName() override
        {
            return m_GetFileName;
        }
        const string & GetFileName() const override
        {
            return m_GetFileName;
        }
        const char * GetFilePath() const override
        {
            return m_GetFileName.c_str();
        }
        void SetFilePath(const char * szFileName) override
        {
        }
        const char * GetGeoName() override
        {
            return nullptr;
        }
        void SetGeoName(const char * szGeoName) override
        {
        }
        bool IsSameObject(const char * szFileName, const char * szGeomName) override
        {
            return false;
        }
        Vec3 GetHelperPos(const char * szHelperName) override
        {
            return Vec3();
        }
        Matrix34 m_GetHelperTM;
        const Matrix34 & GetHelperTM(const char * szHelperName) override
        {
            return m_GetHelperTM;
        }
        bool IsDefaultObject() override
        {
            return false;
        }
        void FreeIndexedMesh() override
        {
        }
        void GetMemoryUsage(ICrySizer * pSizer) const override
        {
        }
        float m_GetRadiusVert;
        float & GetRadiusVert() override
        {
            return m_GetRadiusVert;
        }
        float m_GetRadiusHors;
        float & GetRadiusHors() override
        {
            return m_GetRadiusHors;
        }
        bool IsPhysicsExist() override
        {
            return false;
        }
        void Invalidate(bool bPhysics = false, float tolerance = 0.05f) override
        {
        }
        int GetSubObjectCount() const override
        {
            return 0;
        }
        void SetSubObjectCount(int nCount) override
        {
        }
        IStatObj::SSubObject * GetSubObject(int nIndex) override
        {
            return nullptr;
        }
        bool IsSubObject() const override
        {
            return false;
        }
        IStatObj * GetParentObject() const override
        {
            return nullptr;
        }
        IStatObj * GetCloneSourceObject() const override
        {
            return nullptr;
        }
        IStatObj::SSubObject * FindSubObject(const char * sNodeName) override
        {
            return nullptr;
        }
        IStatObj::SSubObject * FindSubObject_CGA(const char * sNodeName) override
        {
            return nullptr;
        }
        IStatObj::SSubObject * FindSubObject_StrStr(const char * sNodeName) override
        {
            return nullptr;
        }
        bool RemoveSubObject(int nIndex) override
        {
            return false;
        }
        bool CopySubObject(int nToIndex, IStatObj * pFromObj, int nFromIndex) override
        {
            return false;
        }
        IStatObj::SSubObject m_AddSubObject;
        IStatObj::SSubObject& AddSubObject(IStatObj * pStatObj) override
        {
            return m_AddSubObject;
        }
        int PhysicalizeSubobjects(IPhysicalEntity * pent, const Matrix34 * pMtx, float mass, float density = 0.0f, int id0 = 0, strided_pointer<int> pJointsIdMap = 0, const char * szPropsOverride = 0) override
        {
            return 0;
        }
        int Physicalize(IPhysicalEntity * pent, pe_geomparams * pgp, int id = 0, const char * szPropsOverride = 0) override
        {
            return 0;
        }
        bool IsDeformable() override
        {
            return false;
        }
        bool SaveToCGF(const char * sFilename, IChunkFile ** pOutChunkFile = NULL, bool bHavePhysicalProxy = false) override
        {
            return false;
        }
        IStatObj * Clone(bool bCloneGeometry, bool bCloneChildren, bool bMeshesOnly) override
        {
            return nullptr;
        }
        int SetDeformationMorphTarget(IStatObj * pDeformed) override
        {
            return 0;
        }
        IStatObj * DeformMorph(const Vec3 & pt, float r, float strength, IRenderMesh * pWeights = 0) override
        {
            return nullptr;
        }
        IStatObj * HideFoliage() override
        {
            return nullptr;
        }
        int Serialize(TSerialize ser) override
        {
            return 0;
        }
        const char * GetProperties() override
        {
            return nullptr;
        }
        bool GetPhysicalProperties(float & mass, float & density) override
        {
            return false;
        }
        IStatObj * GetLastBooleanOp(float & scale) override
        {
            return nullptr;
        }
        bool m_RayIntersectionOutput = true;
        bool RayIntersection(SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl = 0) override
        {
            hitInfo.fDistance = 0.1f;
            return m_RayIntersectionOutput;
        }
        bool m_LineSegIntersection = true;
        bool LineSegIntersection(const Lineseg & lineSeg, Vec3 & hitPos, int & surfaceTypeId) override
        {
            return m_LineSegIntersection;
        }
        void DebugDraw(const SGeometryDebugDrawInfo & info, float fExtrdueScale = 0.01f) override
        {
        }
        void GetStatistics(SStatistics & stats) override
        {
        }
        uint64 GetInitialHideMask() override
        {
            return uint64();
        }
        uint64 UpdateInitialHideMask(uint64 maskAND = 0ul - 1ul, uint64 maskOR = 0) override
        {
            return uint64();
        }
        void SetStreamingDependencyFilePath(const char * szFileName) override
        {
        }
        void ComputeGeometricMean(SMeshLodInfo & lodInfo) override
        {
        }
        float GetLodDistance() const override
        {
            return 0.0f;
        }
        bool IsMeshStrippedCGF() const override
        {
            return false;
        }
        void LoadLowLODs(bool bUseStreaming, unsigned long nLoadingFlags) override
        {
        }
        bool AreLodsLoaded() const override
        {
            return false;
        }
        bool CheckGarbage() const override
        {
            return false;
        }
        void SetCheckGarbage(bool val) override
        {
        }
        int CountChildReferences() const override
        {
            return 0;
        }
        int GetUserCount() const override
        {
            return 0;
        }
        void ShutDown() override
        {
        }
        int GetMaxUsableLod() const override
        {
            return 0;
        }
        int GetMinUsableLod() const override
        {
            return 0;
        }
        SMeshBoneMapping_uint8 * GetBoneMapping() const override
        {
            return nullptr;
        }
        int GetSpineCount() const override
        {
            return 0;
        }
        SSpine * GetSpines() const override
        {
            return nullptr;
        }
        IStatObj * GetLodLevel0() override
        {
            return nullptr;
        }
        void SetLodLevel0(IStatObj * lod) override
        {
        }
        _smart_ptr<CStatObj>* GetLods() override
        {
            return nullptr;
        }
        int GetLoadedLodsNum() override
        {
            return 0;
        }
        bool UpdateStreamableComponents(float fImportance, const Matrix34A & objMatrix, bool bFullUpdate, int nNewLod) override
        {
            return false;
        }
        void RenderInternal(CRenderObject * pRenderObject, uint64 nSubObjectHideMask, const CLodValue & lodValue, const SRenderingPassInfo & passInfo, const SRendItemSorter & rendItemSorter, bool forceStaticDraw) override
        {
        }
        void RenderObjectInternal(CRenderObject * pRenderObject, int nLod, uint8 uLodDissolveRef, bool dissolveOut, const SRenderingPassInfo & passInfo, const SRendItemSorter & rendItemSorter, bool forceStaticDraw) override
        {
        }
        void RenderSubObject(CRenderObject * pRenderObject, int nLod, int nSubObjId, const Matrix34A & renderTM, const SRenderingPassInfo & passInfo, const SRendItemSorter & rendItemSorter, bool forceStaticDraw) override
        {
        }
        void RenderSubObjectInternal(CRenderObject * pRenderObject, int nLod, const SRenderingPassInfo & passInfo, const SRendItemSorter & rendItemSorter, bool forceStaticDraw) override
        {
        }
        void RenderRenderMesh(CRenderObject * pObj, SInstancingInfo * pInstInfo, const SRenderingPassInfo & passInfo, const SRendItemSorter & rendItemSorter) override
        {
        }
        SPhysGeomArray m_GetArrPhysGeomInfo = {};
        SPhysGeomArray & GetArrPhysGeomInfo() override
        {
            return m_GetArrPhysGeomInfo;
        }
        bool IsLodsAreLoadedFromSeparateFile() override
        {
            return false;
        }
        void StartStreaming(bool bFinishNow, IReadStream_AutoPtr * ppStream) override
        {
        }
        void UpdateStreamingPrioriryInternal(const Matrix34A & objMatrix, float fDistance, bool bFullUpdate) override
        {
        }
        void SetMerged(bool state) override
        {
        }
        int GetRenderMeshMemoryUsage() const override
        {
            return 0;
        }
        void SetLodObject(int nLod, IStatObj * pLod) override
        {
        }
        int GetLoadedTrisCount() const override
        {
            return 0;
        }
        int GetRenderTrisCount() const override
        {
            return 0;
        }
        int GetRenderMatIds() const override
        {
            return 0;
        }
        bool IsUnmergable() const override
        {
            return false;
        }
        void SetUnmergable(bool state) override
        {
        }
        int GetSubObjectMeshCount() const override
        {
            return 0;
        }
        void SetSubObjectMeshCount(int count) override
        {
        }
        void CleanUnusedLods() override
        {
        }
    };

    struct MockMeshAsset
        : public LmbrCentral::MeshAsset
    {
        AZ_RTTI(MockMeshAsset, "{C314B960-9B54-468D-B37C-065738E7487C}", LmbrCentral::MeshAsset);
        AZ_CLASS_ALLOCATOR(MeshAsset, AZ::SystemAllocator, 0);

        MockMeshAsset()
        {
            m_assetId = AZ::Uuid::CreateRandom();
            m_status.store(static_cast<int>(AZ::Data::AssetData::AssetStatus::Ready));
            m_statObj.reset(&m_mockStatObj);
            m_useCount.fetch_add(1);
        }

        void Release()
        {
            m_statObj.reset(nullptr);
        }

        ~MockMeshAsset() override
        {
            Release();
        }

        MockStatObj m_mockStatObj;
    };

    struct MockMeshRequestBus
        : public LmbrCentral::MeshComponentRequestBus::Handler
    {
        AZ::Aabb m_GetWorldBoundsOutput;
        AZ::Aabb GetWorldBounds() override
        {
            return m_GetWorldBoundsOutput;
        }

        AZ::Aabb m_GetLocalBoundsOutput;
        AZ::Aabb GetLocalBounds() override
        {
            return m_GetLocalBoundsOutput;
        }

        void SetMeshAsset(const AZ::Data::AssetId & id) override
        {
        }

        AZ::Data::Asset<LmbrCentral::MeshAsset> m_GetMeshAssetOutput;
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override
        {
            return m_GetMeshAssetOutput;
        }

        bool m_GetVisibilityOutput = true;
        bool GetVisibility()
        {
            return m_GetVisibilityOutput;
        }
    };

    struct MockTransformBus
        : public AZ::TransformBus::Handler
    {
        AZ::Transform m_GetLocalTMOutput;
        const AZ::Transform & GetLocalTM() override
        {
            return m_GetLocalTMOutput;
        }
        AZ::Transform m_GetWorldTMOutput;
        const AZ::Transform & GetWorldTM() override
        {
            return m_GetWorldTMOutput;
        }
        bool IsStaticTransform() override
        {
            return false;
        }
        bool IsPositionInterpolated() override
        {
            return false;
        }
        bool IsRotationInterpolated() override
        {
            return false;
        }
    };

    class MockShapeServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockShapeServiceComponent, "{E1687D77-F43F-439B-BB6D-B1457E94812A}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("VegetationDescriptorProviderService", 0x62e51209));
        }
    };

    class MockVegetationAreaServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockVegetationAreaServiceComponent, "{EF5292D8-411E-4660-9B31-EFAEED34B1EE}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("VegetationAreaService", 0x6a859504));
        }
    };

    class MockMeshServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockMeshServiceComponent, "{69547762-7EAB-4AC4-86FA-7486F1BBB115}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
        }
    };
}