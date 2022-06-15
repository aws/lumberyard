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

#include "LmbrCentral_precompiled.h"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Rendering/MeshAssetHandler.h>


#include "Source/Rendering/EditorMeshComponent.h"

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    // base physical entity which can be derived from to detect other specific use-cases
    struct PhysicalEntityPlaceHolder
        : public IPhysicalEntity
    {
        pe_type GetType() const override { return PE_NONE; } 
        int AddRef() override { return 0; }
        int Release() override { return 0; }
        int SetParams(const pe_params* params, int bThreadSafe = 0) override { return 0; }
        int GetParams(pe_params* params) const override { return 0; } 
        int GetStatus(pe_status* status) const override { return 0; } 
        int Action(const pe_action*, int bThreadSafe = 0) override { return 0; } 
        int AddGeometry(phys_geometry* pgeom, pe_geomparams* params, int id = -1, int bThreadSafe = 0) override { return 0; }
        void RemoveGeometry(int id, int bThreadSafe = 0) override {}
        PhysicsForeignData GetForeignData(int itype = 0) const override { return PhysicsForeignData{}; }
        int GetiForeignData() const override { return 0; }
        int GetStateSnapshot(class CStream& stm, float time_back = 0, int flags = 0) override { return 0; }
        int GetStateSnapshot(TSerialize ser, float time_back = 0, int flags = 0) override { return 0; }
        int SetStateFromSnapshot(class CStream& stm, int flags = 0) override { return 0; }
        int PostSetStateFromSnapshot() override { return 0; }
        unsigned int GetStateChecksum() override { return 0; }
        void SetNetworkAuthority(int authoritive = -1, int paused = -1) override {}
        int SetStateFromSnapshot(TSerialize ser, int flags = 0) override { return 0; }
        int SetStateFromTypedSnapshot(TSerialize ser, int type, int flags = 0) override { return 0; }
        int GetStateSnapshotTxt(char* txtbuf, int szbuf, float time_back = 0) override { return 0; }
        void SetStateFromSnapshotTxt(const char* txtbuf, int szbuf) override {}
        int DoStep(float time_interval) override { return 0; }
        int DoStep(float time_interval, int iCaller) override { return 0; }
        void StartStep(float time_interval) override {}
        void StepBack(float time_interval) override {}
#if ENABLE_CRY_PHYSICS
        IPhysicalWorld* GetWorld() const override { return nullptr; }
#endif
        void GetMemoryStatistics(ICrySizer* pSizer) const override {}
    };

    // special test fake to validate incoming pe_params
    struct PhysicalEntitySetParamsCheck
        : public PhysicalEntityPlaceHolder
    {
        int SetParams(const pe_params* params, int bThreadSafe = 0) override
        {
            if (params->type == pe_params_pos::type_id)
            {
                pe_params_pos* params_pos = (pe_params_pos*)params;

                Vec3 s;
                if (Matrix34* m34 = params_pos->pMtx3x4)
                {
                    s.Set(m34->GetColumn(0).len(), m34->GetColumn(1).len(), m34->GetColumn(2).len());
                    Matrix33 m33(m34->GetColumn(0) / s.x, m34->GetColumn(1) / s.y, m34->GetColumn(2) / s.z);
                    // ensure passed in params_pos->pMtx3x4 is orthonormal
                    // ref - see Cry_Quat.h - explicit ILINE Quat_tpl<F>(const Matrix33_tpl<F>&m)
                    m_isOrthonormal = m33.IsOrthonormalRH(0.1f);
                }
            }

            return 0;
        }

        bool m_isOrthonormal = false;
    };

    class TestEditorMeshComponent
        : public EditorMeshComponent
    {
    public:
        AZ_EDITOR_COMPONENT(TestEditorMeshComponent, "{6C6B593A-1946-4239-AE16-E8B96D9835E5}", EditorMeshComponent)

        static void Reflect(AZ::ReflectContext* context);

        TestEditorMeshComponent() = default;

#if ENABLE_CRY_PHYSICS
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override
        {
            // to stop the physics entity getting rebuilt ensure m_physScale stays in sync with world scale
            m_physScale = world.RetrieveScale();
            EditorMeshComponent::OnTransformChanged(local, world);
        }

        void OverridePhysicalEntity(IPhysicalEntity* physicalEntity)
        {
            m_physicalEntity = physicalEntity;
        }
#endif
    };

    void TestEditorMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        MeshComponentRenderNode::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            AzFramework::SimpleAssetReference<MaterialAsset>::Register(*serializeContext);

            serializeContext->Class<TestEditorMeshComponent>()
                ->Version(0)
                ->FieldFromBase<TestEditorMeshComponent>("Static Mesh Render Node", &TestEditorMeshComponent::m_mesh)
                ;
        }
    }

    class EditorMeshComponentTestFixture
        : public ToolsApplicationFixture
    {
        AZStd::unique_ptr<ComponentDescriptor> m_testMeshComponentDescriptor;

    public:
        void SetUpEditorFixtureImpl() override
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            m_testMeshComponentDescriptor =
                AZStd::unique_ptr<ComponentDescriptor>(TestEditorMeshComponent::CreateDescriptor());
            m_testMeshComponentDescriptor->Reflect(m_serializeContext);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_testMeshComponentDescriptor.reset();
        }

        AZ::SerializeContext* m_serializeContext = nullptr;
    };

    struct EditorMeshComponentAssetLoadingTestFixture
        : EditorMeshComponentTestFixture
        , AZ::Data::AssetBus::MultiHandler
    {
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            EXPECT_EQ(asset.GetId(), m_assetIdToLoad);
            m_assetIdToLoad.SetInvalid();
            m_assetLoadSemaphore.release();
            m_assetLoaded = true;
        }

        void WaitForAssetToLoad(AZ::Data::AssetId assetIdToLoad)
        {
            m_assetIdToLoad = assetIdToLoad;
            if (m_assetIdToLoad.IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_assetIdToLoad);

                const int assetLoadSleepMS = 20;
                int totalWaitTimeMS = 5000;
                bool assetLoaded = false;
                while (!m_assetLoaded && totalWaitTimeMS > 0)
                {
                    m_assetLoaded = m_assetLoaded || m_assetLoadSemaphore.try_acquire_for(AZStd::chrono::milliseconds(assetLoadSleepMS));
                    AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
                    if (!assetLoaded)
                    {
                        totalWaitTimeMS -= assetLoadSleepMS;
                    }
                }

                AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            }
        }

        bool m_assetLoaded = false;
        AZ::Data::AssetId m_assetIdToLoad;
        AZStd::binary_semaphore m_assetLoadSemaphore;
    };

    struct MeshAssetHandlerFixture
        : ScopedAllocatorSetupFixture
    {
    protected:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            Data::AssetManager::Create(Data::AssetManager::Descriptor());
            Data::AssetManager::Instance().SetAssetInfoUpgradingEnabled(false);

            m_handler.Register();
        }

        void TearDown() override
        {
            m_handler.Unregister();
            Data::AssetManager::Destroy();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        }

        MeshAssetHandler m_handler;
    };

    struct MockAssetSystemRequestHandler
        : AzFramework::AssetSystemRequestBus::Handler
    {
        MockAssetSystemRequestHandler()
        {
            BusConnect();
        }

        ~MockAssetSystemRequestHandler()
        {
            BusDisconnect();
        }

        AssetSystem::AssetStatus GetAssetStatusById(const AZ::Data::AssetId& assetId) override
        {
            m_statusRequest = true;

            return AssetSystem::AssetStatus_Queued;
        }

        MOCK_METHOD1(CompileAssetSync, AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(CompileAssetSync_FlushIO, AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(CompileAssetSyncById, AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD1(CompileAssetSyncById_FlushIO, AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD4(ConfigureSocketConnection, bool (const AZStd::string&, const AZStd::string&, const AZStd::string&, const AZStd::string&));
        MOCK_METHOD1(Connect, bool (const char*));
        MOCK_METHOD2(ConnectWithTimeout, bool (const char*, AZStd::chrono::duration<float>));
        MOCK_METHOD0(Disconnect, bool ());
        MOCK_METHOD1(EscalateAssetBySearchTerm, bool (AZStd::string_view));
        MOCK_METHOD1(EscalateAssetByUuid, bool (const AZ::Uuid&));
        MOCK_METHOD0(GetAssetProcessorPingTimeMilliseconds, float ());
        MOCK_METHOD1(GetAssetStatus, AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(GetAssetStatus_FlushIO, AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(GetAssetStatusById_FlushIO, AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD3(GetUnresolvedProductReferences, void (AZ::Data::AssetId, AZ::u32&, AZ::u32&));
        MOCK_METHOD0(SaveCatalog, bool ());
        MOCK_METHOD1(SetAssetProcessorIP, void (const AZStd::string&));
        MOCK_METHOD1(SetAssetProcessorPort, void (AZ::u16));
        MOCK_METHOD1(SetBranchToken, void (const AZStd::string&));
        MOCK_METHOD1(SetProjectName, void (const AZStd::string&));
        MOCK_METHOD0(ShowAssetProcessor, void ());
        MOCK_METHOD1(ShowInAssetProcessor, void (const AZStd::string&));

        bool m_statusRequest = false;
    };

    struct MockCatalog
        : Data::AssetCatalogRequestBus::Handler
    {
        MockCatalog()
        {
            BusConnect();
        }

        ~MockCatalog()
        {
            BusDisconnect();
        }

        AZ::Data::AssetId GetAssetIdByPath(const char*, const AZ::Data::AssetType&, bool) override
        {
            m_generatedId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 1234);

            return m_generatedId;
        }

        MOCK_METHOD1(GetAssetInfoById, AZ::Data::AssetInfo (const AZ::Data::AssetId&));
        MOCK_METHOD1(AddAssetType, void (const AZ::Data::AssetType&));
        MOCK_METHOD1(AddDeltaCatalog, bool (AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(AddExtension, void (const char*));
        MOCK_METHOD0(ClearCatalog, void ());
        MOCK_METHOD5(CreateBundleManifest, bool (const AZStd::string&, const AZStd::vector<AZStd::string>&, const AZStd::string&, int, const AZStd::vector<AZStd::string>&));
        MOCK_METHOD2(CreateDeltaCatalog, bool (const AZStd::vector<AZStd::string>&, const AZStd::string&));
        MOCK_METHOD0(DisableCatalog, void ());
        MOCK_METHOD1(EnableCatalogForAsset, void (const AZ::Data::AssetType&));
        MOCK_METHOD3(EnumerateAssets, void (BeginAssetEnumerationCB, AssetEnumerationCB, EndAssetEnumerationCB));
        MOCK_METHOD1(GenerateAssetIdTEMP, AZ::Data::AssetId (const char*));
        MOCK_METHOD1(GetAllProductDependencies, AZ::Outcome<AZStd::vector<Data::ProductDependency>, AZStd::string> (const Data::AssetId&));
        MOCK_METHOD3(GetAllProductDependenciesFilter, AZ::Outcome<AZStd::vector<Data::ProductDependency>, AZStd::string> (const Data::AssetId&, const AZStd::unordered_set<Data::AssetId>&, const AZStd::vector<AZStd::string>&));
        MOCK_METHOD1(GetAssetPathById, AZStd::string (const AZ::Data::AssetId&));
        MOCK_METHOD1(GetDirectProductDependencies, AZ::Outcome<AZStd::vector<Data::ProductDependency>, AZStd::string> (const Data::AssetId&));
        MOCK_METHOD1(GetHandledAssetTypes, void (AZStd::vector<AZ::Data::AssetType>&));
        MOCK_METHOD0(GetRegisteredAssetPaths, AZStd::vector<AZStd::string> ());
        MOCK_METHOD2(InsertDeltaCatalog, bool (AZStd::shared_ptr<AzFramework::AssetRegistry>, size_t));
        MOCK_METHOD2(InsertDeltaCatalogBefore, bool (AZStd::shared_ptr<AzFramework::AssetRegistry>, AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(LoadCatalog, bool (const char*));
        MOCK_METHOD2(RegisterAsset, void (const AZ::Data::AssetId&, AZ::Data::AssetInfo&));
        MOCK_METHOD1(RemoveDeltaCatalog, bool (AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(SaveCatalog, bool (const char*));
        MOCK_METHOD0(StartMonitoringAssets, void ());
        MOCK_METHOD0(StopMonitoringAssets, void ());
        MOCK_METHOD1(UnregisterAsset, void (const AZ::Data::AssetId&));

        AZ::Data::AssetId m_generatedId{};
    };

    struct MockAssetData
        : MeshAsset
    {
        MockAssetData(AZ::Data::AssetId assetId)
        {
            m_assetId = assetId;
        }
    };

    TEST_F(MeshAssetHandlerFixture, LoadAsset_StillInQueue_LoadsSubstituteAsset)
    {
        MockAssetSystemRequestHandler assetSystem;
        MockCatalog catalog;
        AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 0);

        Data::AssetPtr assetPointer = aznew MockAssetData(assetId);
        AZ::Data::Asset<AZ::Data::AssetData> asset = assetPointer;
        auto substituteAssetId = m_handler.AssetMissingInCatalog(asset);

        ASSERT_TRUE(assetSystem.m_statusRequest);
        ASSERT_TRUE(catalog.m_generatedId.IsValid());
        ASSERT_EQ(substituteAssetId, catalog.m_generatedId);
    }

#if ENABLE_CRY_PHYSICS
    TEST_F(EditorMeshComponentTestFixture, OrthonormalTransformIsPassedToPhysicalEntity)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // setup a simple hierarchy
        AZ::Entity parent;
        AZ::Entity child;

        parent.Init();
        child.Init();

        AZ::EntityId parentId = parent.GetId();
        AZ::EntityId childId = child.GetId();

        parent.CreateComponent<AzToolsFramework::Components::TransformComponent>();
        child.CreateComponent<AzToolsFramework::Components::TransformComponent>();

        // add test mesh component to 'sense' change
        TestEditorMeshComponent* parentTestMeshComponent =
            parent.CreateComponent<TestEditorMeshComponent>();
        TestEditorMeshComponent* childTestMeshComponent =
            child.CreateComponent<TestEditorMeshComponent>();

        parent.Activate();
        child.Activate();

        AZ::TransformBus::Event(
            childId, &AZ::TransformInterface::SetParent, parentId);

        // apply scale to entities
        AZ::TransformBus::Event(parentId, &AZ::TransformBus::Events::SetLocalScale, AZ::Vector3(2.0f, 0.5f, 3.0f));
        AZ::TransformBus::Event(childId, &AZ::TransformBus::Events::SetLocalScale, AZ::Vector3(3.0f, 2.5f, 0.2f));

        // attach face physical entity (to detect changes)
        auto parentPhysicalEntitySetParamChecker = AZStd::make_unique<PhysicalEntitySetParamsCheck>();
        parentTestMeshComponent->OverridePhysicalEntity(parentPhysicalEntitySetParamChecker.get());

        auto childPhysicalEntitySetParamChecker = AZStd::make_unique<PhysicalEntitySetParamsCheck>();
        childTestMeshComponent->OverridePhysicalEntity(childPhysicalEntitySetParamChecker.get());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const AZ::Transform rotation =
            AZ::Transform::CreateFromQuaternion(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), AZ::DegToRad(40.0f)));

        AZ::Transform childLocalTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            childLocalTM, childId, &AZ::TransformBus::Events::GetLocalTM);

        AZ::TransformBus::Event(childId, &AZ::TransformBus::Events::SetLocalTM, childLocalTM * rotation);

        // apply new scale to parent after orientating child entity
        AZ::TransformBus::Event(parentId, &AZ::TransformBus::Events::SetLocalScale, AZ::Vector3(0.6f, 0.5f, 3.0f));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        // The child should not have created a physics entity, because its transform is too skewed


        EXPECT_TRUE(childTestMeshComponent->GetPhysicalEntity() == nullptr);
        EXPECT_TRUE(parentTestMeshComponent->GetPhysicalEntity() != nullptr);
        EXPECT_TRUE(parentPhysicalEntitySetParamChecker->m_isOrthonormal);

        // clear test physical entity
        childTestMeshComponent->OverridePhysicalEntity(nullptr);
        parentTestMeshComponent->OverridePhysicalEntity(nullptr);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorMeshComponentTestFixture, IsPhysicalizableReturnsTrueForIdentity)
    {
        EXPECT_TRUE(IsPhysicalizable(AZ::Transform::Identity()));
    }

    TEST_F(EditorMeshComponentTestFixture, IsPhysicalizableReturnsTrueForPureRotationMatrices)
    {
        AZ::Transform rot1 = AZ::Transform::CreateRotationZ(AZ::DegToRad(20.0f));
        AZ::Transform rot2 = AZ::Transform::CreateRotationX(AZ::DegToRad(35.0f));
        AZ::Transform rot3 = AZ::Transform::CreateRotationY(AZ::DegToRad(60.0f));

        EXPECT_TRUE(IsPhysicalizable(rot1));
        EXPECT_TRUE(IsPhysicalizable(rot2));
        EXPECT_TRUE(IsPhysicalizable(rot3));
        EXPECT_TRUE(IsPhysicalizable(rot1 * rot2));
        EXPECT_TRUE(IsPhysicalizable(rot2 * rot3 * rot1));
    }

    TEST_F(EditorMeshComponentTestFixture, IsPhysicalizableReturnsTrueForPureScaleMatrices)
    {
        AZ::Transform scale1 = AZ::Transform::CreateDiagonal(AZ::Vector3(0.5f, 2.0f, 1.5f));
        AZ::Transform scale2 = AZ::Transform::CreateDiagonal(AZ::Vector3(2.5f, 3.0f, 0.5f));
        AZ::Transform scale3 = AZ::Transform::CreateDiagonal(AZ::Vector3(2.0f, 0.2f, 1.3f));

        EXPECT_TRUE(IsPhysicalizable(scale1));
        EXPECT_TRUE(IsPhysicalizable(scale2));
        EXPECT_TRUE(IsPhysicalizable(scale3));
        EXPECT_TRUE(IsPhysicalizable(scale1 * scale2));
        EXPECT_TRUE(IsPhysicalizable(scale3 * scale2));
    }

    TEST_F(EditorMeshComponentTestFixture, IsPhysicalizableReturnsTrueForScaleFollowedByRotationMatrices)
    {
        AZ::Transform rot1 = AZ::Transform::CreateRotationZ(AZ::DegToRad(20.0f));
        AZ::Transform rot2 = AZ::Transform::CreateRotationX(AZ::DegToRad(35.0f));
        AZ::Transform scale1 = AZ::Transform::CreateDiagonal(AZ::Vector3(0.5f, 2.0f, 1.5f));
        AZ::Transform scale2 = AZ::Transform::CreateDiagonal(AZ::Vector3(2.5f, 3.0f, 0.5f));

        EXPECT_TRUE(IsPhysicalizable(rot1 * scale1));
        EXPECT_TRUE(IsPhysicalizable(rot1 * scale2));
        EXPECT_TRUE(IsPhysicalizable(rot2 * scale1));
        EXPECT_TRUE(IsPhysicalizable(rot2 * scale2));
    }

    TEST_F(EditorMeshComponentTestFixture, IsPhysicalizableReturnsFalseForSkewMatrices)
    {
        AZ::Transform rot1 = AZ::Transform::CreateRotationZ(AZ::DegToRad(20.0f));
        AZ::Transform rot2 = AZ::Transform::CreateRotationX(AZ::DegToRad(35.0f));
        AZ::Transform scale1 = AZ::Transform::CreateDiagonal(AZ::Vector3(0.5f, 2.0f, 1.5f));
        AZ::Transform scale2 = AZ::Transform::CreateDiagonal(AZ::Vector3(2.5f, 3.0f, 0.5f));

        EXPECT_FALSE(IsPhysicalizable(scale1 * rot1));
        EXPECT_FALSE(IsPhysicalizable(scale1 * rot2));
        EXPECT_FALSE(IsPhysicalizable(scale2 * rot1));
        EXPECT_FALSE(IsPhysicalizable(scale2 * rot2));
    }
#endif

    TEST_F(EditorMeshComponentAssetLoadingTestFixture, MeshAssetCopiedThroughSerialization)
    {
        // create source and target entities
        AZ::Entity source;
        AZ::Entity target;
        AZ::EntityId sourceId = source.GetId();
        AZ::EntityId targetId = target.GetId();

        // add test mesh component to 'sense' change
        TestEditorMeshComponent* sourceTestMeshComponent = source.CreateComponent<TestEditorMeshComponent>();
        TestEditorMeshComponent* targetTestMeshComponent = target.CreateComponent<TestEditorMeshComponent>();

        // set the mesh asset of source to mock id
        MeshAssetHandler meshAssetHandler;
        meshAssetHandler.Register(); // registers self with AssetManager
        AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 0);
        auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
        if (assetCatalog)
        {
            assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<MeshAsset>::Uuid());
        }
        auto asset = Data::AssetManager::Instance().CreateAsset(assetId, azrtti_typeid<MeshAsset>());
        sourceTestMeshComponent->SetMeshAsset(assetId);

        // build InstanceDataHierarchy so we can call CopyInstanceData
        AzToolsFramework::InstanceDataHierarchy targetHierarchy;
        targetHierarchy.AddRootInstance<AZ::Entity>(&target);
        targetHierarchy.Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        AzToolsFramework::InstanceDataHierarchy sourceHierarchy;
        sourceHierarchy.AddRootInstance<AZ::Entity>(&source);
        sourceHierarchy.Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        const AzToolsFramework::InstanceDataNode* sourceNode = &sourceHierarchy;
        AzToolsFramework::InstanceDataNode* targetNode = &targetHierarchy;

        // call CopyInstanceData which should copy the asset change from source to target
        AzToolsFramework::InstanceDataHierarchy::CopyInstanceData(sourceNode, targetNode, m_serializeContext, nullptr, nullptr,
            AzToolsFramework::InstanceDataNode::Address());

        // verify target now has the mock asset data copied over from source
        EXPECT_EQ(targetTestMeshComponent->GetMeshAsset(), sourceTestMeshComponent->GetMeshAsset());

        // cleanup - remove all references to asset so asset can be released without complaints
        source.RemoveComponent(sourceTestMeshComponent);
        target.RemoveComponent(targetTestMeshComponent);
        delete sourceTestMeshComponent;
        delete targetTestMeshComponent;

        // calling CopyInstanceData actually changes target's EntityId, need to change it back for destructor not to crash
        target.SetId(targetId);

        // need to wait for asset to finish loading before destructing
        WaitForAssetToLoad(assetId);
    }
} // namespace UnitTest
