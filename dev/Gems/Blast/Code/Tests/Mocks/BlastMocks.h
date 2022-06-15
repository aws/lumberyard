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

#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/Joint.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>

#include <Blast/BlastActor.h>
#include <Family/ActorTracker.h>
#include <Family/BlastFamily.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastTkActor.h>
#include <NvBlastTkAsset.h>
#include <gmock/gmock.h>

#include <AzFramework/Entity/GameEntityContextBus.h>
#include <Components/BlastMeshDataComponent.h>

#include <Actor/EntityProvider.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Scripting/SpawnerComponentBus.h>
#include <NvBlastTkFamily.h>
#include <NvBlastTkGroup.h>

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <Blast/BlastSystemBus.h>

namespace Blast
{
    class FastScopedAllocatorsBase
    {
    public:
        FastScopedAllocatorsBase()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        ~FastScopedAllocatorsBase()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    class BlastFamily;

    class FakeExtPxAsset : public Nv::Blast::ExtPxAsset
    {
    public:
        FakeExtPxAsset(
            const NvBlastActorDesc& desc, AZStd::vector<Nv::Blast::ExtPxChunk> chunks = {},
            AZStd::vector<Nv::Blast::ExtPxSubchunk> subchunks = {})
            : m_desc(desc)
            , m_chunks(chunks)
            , m_subchunks(subchunks)
        {
        }

        virtual ~FakeExtPxAsset() = default;

        NvBlastActorDesc& getDefaultActorDesc() override
        {
            return m_desc;
        }

        uint32_t getChunkCount() const override
        {
            return m_chunks.size();
        }

        const Nv::Blast::ExtPxChunk* getChunks() const override
        {
            return m_chunks.data();
        }

        uint32_t getSubchunkCount() const override
        {
            return m_subchunks.size();
        }

        const Nv::Blast::ExtPxSubchunk* getSubchunks() const override
        {
            return m_subchunks.data();
        }

        const NvBlastActorDesc& getDefaultActorDesc() const override
        {
            return m_desc;
        }

        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD0(getTkAsset, const Nv::Blast::TkAsset&());
        MOCK_METHOD1(setUniformHealth, void(bool));
        MOCK_METHOD1(setAccelerator, void(NvBlastExtDamageAccelerator*));
        MOCK_CONST_METHOD0(getAccelerator, NvBlastExtDamageAccelerator*());

        NvBlastActorDesc m_desc;
        AZStd::vector<Nv::Blast::ExtPxChunk> m_chunks;
        AZStd::vector<Nv::Blast::ExtPxSubchunk> m_subchunks;
    };

    class MockBlastMeshData : public BlastMeshData
    {
    public:
        virtual ~MockBlastMeshData() = default;

        MOCK_CONST_METHOD1(GetMeshAsset, const AZ::Data::Asset<LmbrCentral::MeshAsset>&(size_t));
        MOCK_CONST_METHOD0(GetMeshAssets, const AZStd::vector<AZ::Data::Asset<LmbrCentral::MeshAsset>>&());
    };

    class MockTkFramework : public Nv::Blast::TkFramework
    {
    public:
        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD1(getType, const Nv::Blast::TkType*(Nv::Blast::TkTypeIndex::Enum));
        MOCK_CONST_METHOD1(findObjectByID, Nv::Blast::TkIdentifiable*(const NvBlastID&));
        MOCK_CONST_METHOD1(getObjectCount, uint32_t(const Nv::Blast::TkType&));
        MOCK_CONST_METHOD4(
            getObjects, uint32_t(Nv::Blast::TkIdentifiable**, uint32_t, const Nv::Blast::TkType&, uint32_t));
        MOCK_CONST_METHOD6(
            reorderAssetDescChunks, bool(NvBlastChunkDesc*, uint32_t, NvBlastBondDesc*, uint32_t, uint32_t*, bool));
        MOCK_CONST_METHOD2(ensureAssetExactSupportCoverage, bool(NvBlastChunkDesc*, uint32_t));
        MOCK_METHOD1(createAsset, Nv::Blast::TkAsset*(const Nv::Blast::TkAssetDesc&));
        MOCK_METHOD4(
            createAsset, Nv::Blast::TkAsset*(const NvBlastAsset*, Nv::Blast::TkAssetJointDesc*, uint32_t, bool));
        MOCK_METHOD1(createGroup, Nv::Blast::TkGroup*(const Nv::Blast::TkGroupDesc&));
        MOCK_METHOD1(createActor, Nv::Blast::TkActor*(const Nv::Blast::TkActorDesc&));
        MOCK_METHOD1(createJoint, Nv::Blast::TkJoint*(const Nv::Blast::TkJointDesc&));
    };

    class MockTkActor : public Nv::Blast::TkActor
    {
    public:
        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD0(getActorLL, const NvBlastActor*());
        MOCK_CONST_METHOD0(getFamily, Nv::Blast::TkFamily&());
        MOCK_CONST_METHOD0(getIndex, uint32_t());
        MOCK_CONST_METHOD0(getGroup, Nv::Blast::TkGroup*());
        MOCK_METHOD0(removeFromGroup, Nv::Blast::TkGroup*());
        MOCK_CONST_METHOD0(getAsset, const Nv::Blast::TkAsset*());
        MOCK_CONST_METHOD0(getVisibleChunkCount, uint32_t());
        MOCK_CONST_METHOD2(getVisibleChunkIndices, uint32_t(uint32_t*, uint32_t));
        MOCK_CONST_METHOD0(getGraphNodeCount, uint32_t());
        MOCK_CONST_METHOD2(getGraphNodeIndices, uint32_t(uint32_t*, uint32_t));
        MOCK_CONST_METHOD0(getBondHealths, const float*());
        MOCK_CONST_METHOD0(getSplitMaxActorCount, uint32_t());
        MOCK_CONST_METHOD0(isPending, bool());
        MOCK_METHOD2(damage, void(const NvBlastDamageProgram&, const void*));
        MOCK_CONST_METHOD3(generateFracture, void(NvBlastFractureBuffers*, const NvBlastDamageProgram&, const void*));
        MOCK_METHOD2(applyFracture, void(NvBlastFractureBuffers*, const NvBlastFractureBuffers*));
        MOCK_CONST_METHOD0(getJointCount, uint32_t());
        MOCK_CONST_METHOD2(getJoints, uint32_t(Nv::Blast::TkJoint**, uint32_t));
        MOCK_CONST_METHOD0(isBoundToWorld, bool());
    };

    class MockTkFamily : public Nv::Blast::TkFamily
    {
    public:
        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD0(getID, const NvBlastID&());
        MOCK_METHOD1(setID, void(const NvBlastID&));
        MOCK_CONST_METHOD0(getType, const Nv::Blast::TkType&());
        MOCK_CONST_METHOD0(getFamilyLL, const NvBlastFamily*());
        MOCK_CONST_METHOD0(getAsset, const Nv::Blast::TkAsset*());
        MOCK_CONST_METHOD0(getActorCount, uint32_t());
        MOCK_CONST_METHOD3(getActors, uint32_t(Nv::Blast::TkActor**, uint32_t, uint32_t));
        MOCK_METHOD1(addListener, void(Nv::Blast::TkEventListener&));
        MOCK_METHOD1(removeListener, void(Nv::Blast::TkEventListener&));
        MOCK_METHOD1(applyFracture, void(const NvBlastFractureBuffers*));
        MOCK_METHOD2(reinitialize, void(const NvBlastFamily*, Nv::Blast::TkGroup*));
    };

    class MockTkAsset : public Nv::Blast::TkAsset
    {
    public:
        MOCK_METHOD0(release, void());
        MOCK_CONST_METHOD0(getID, const NvBlastID&());
        MOCK_METHOD1(setID, void(const NvBlastID&));
        MOCK_CONST_METHOD0(getType, const Nv::Blast::TkType&());
        MOCK_CONST_METHOD0(getAssetLL, const NvBlastAsset*());
        MOCK_CONST_METHOD0(getChunkCount, uint32_t());
        MOCK_CONST_METHOD0(getLeafChunkCount, uint32_t());
        MOCK_CONST_METHOD0(getBondCount, uint32_t());
        MOCK_CONST_METHOD0(getChunks, const NvBlastChunk*());
        MOCK_CONST_METHOD0(getBonds, const NvBlastBond*());
        MOCK_CONST_METHOD0(getGraph, const NvBlastSupportGraph());
        MOCK_CONST_METHOD0(getDataSize, uint32_t());
        MOCK_CONST_METHOD0(getJointDescCount, uint32_t());
        MOCK_CONST_METHOD0(getJointDescs, const Nv::Blast::TkAssetJointDesc*());
    };

    class MockPhysicsSystemRequestsHandler
        : public Physics::SystemRequestBus::Handler
        , public AZ::Interface<Physics::System>::Registrar
    {
    public:
        MockPhysicsSystemRequestsHandler()
        {
            Physics::SystemRequestBus::Handler::BusConnect();
        }
        ~MockPhysicsSystemRequestsHandler()
        {
            Physics::SystemRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(GetDefaultWorldConfiguration, const Physics::WorldConfiguration&());
        MOCK_METHOD1(SetDefaultWorldConfiguration, void(const Physics::WorldConfiguration&));
        MOCK_METHOD1(CreateWorld, AZStd::shared_ptr<Physics::World>(AZ::Crc32));
        MOCK_METHOD2(
            CreateWorldCustom, AZStd::shared_ptr<Physics::World>(AZ::Crc32, const Physics::WorldConfiguration&));
        MOCK_METHOD1(
            CreateStaticRigidBody, AZStd::unique_ptr<Physics::RigidBodyStatic>(const Physics::WorldBodyConfiguration&));
        MOCK_METHOD1(CreateRigidBody, AZStd::unique_ptr<Physics::RigidBody>(const Physics::RigidBodyConfiguration&));
        MOCK_METHOD2(
            CreateShape,
            AZStd::shared_ptr<Physics::Shape>(
                const Physics::ColliderConfiguration&, const Physics::ShapeConfiguration&));
        MOCK_METHOD4(
            AddColliderComponentToEntity,
            void(AZ::Entity*, const Physics::ColliderConfiguration&, const Physics::ShapeConfiguration&, bool));
        MOCK_METHOD1(ReleaseNativeMeshObject, void(void*));
        MOCK_METHOD1(CreateMaterial, AZStd::shared_ptr<Physics::Material>(const Physics::MaterialConfiguration&));
        MOCK_METHOD0(GetDefaultMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_METHOD1(
            CreateMaterialsFromLibrary,
            AZStd::vector<AZStd::shared_ptr<Physics::Material>>(const Physics::MaterialSelection&));
        MOCK_METHOD0(LoadDefaultMaterialLibrary, bool());
        MOCK_METHOD0(GetDefaultMaterialLibraryAssetPtr, const AZ::Data::Asset<Physics::MaterialLibraryAsset>*());
        MOCK_METHOD1(SetDefaultMaterialLibrary, void(const AZ::Data::Asset<Physics::MaterialLibraryAsset>&));
        MOCK_METHOD2(
            UpdateMaterialSelection, bool(const Physics::ShapeConfiguration&, Physics::ColliderConfiguration&));
        MOCK_METHOD0(GetSupportedJointTypes, AZStd::vector<AZ::TypeId>());
        MOCK_METHOD1(CreateJointLimitConfiguration, AZStd::shared_ptr<Physics::JointLimitConfiguration>(AZ::TypeId));
        MOCK_METHOD3(
            CreateJoint,
            AZStd::shared_ptr<Physics::Joint>(
                const AZStd::shared_ptr<Physics::JointLimitConfiguration>&, Physics::WorldBody*, Physics::WorldBody*));
        MOCK_METHOD10(
            GenerateJointLimitVisualizationData,
            void(
                const Physics::JointLimitConfiguration&, const AZ::Quaternion&, const AZ::Quaternion&, float, AZ::u32,
                AZ::u32, AZStd::vector<AZ::Vector3>&, AZStd::vector<AZ::u32>&, AZStd::vector<AZ::Vector3>&,
                AZStd::vector<bool>&));
        MOCK_METHOD5(
            ComputeInitialJointLimitConfiguration,
            AZStd::unique_ptr<Physics::JointLimitConfiguration>(
                const AZ::TypeId&, const AZ::Quaternion&, const AZ::Quaternion&, const AZ::Vector3&,
                const AZStd::vector<AZ::Quaternion>&));
        MOCK_METHOD3(CookConvexMeshToFile, bool(const AZStd::string&, const AZ::Vector3*, AZ::u32));
        MOCK_METHOD3(CookConvexMeshToMemory, bool(const AZ::Vector3*, AZ::u32, AZStd::vector<AZ::u8>&));
        MOCK_METHOD5(
            CookTriangleMeshToFile, bool(const AZStd::string&, const AZ::Vector3*, AZ::u32, const AZ::u32*, AZ ::u32));
        MOCK_METHOD5(
            CookTriangleMeshToMemory,
            bool(const AZ::Vector3*, AZ::u32, const AZ::u32*, AZ::u32, AZStd::vector<AZ::u8>&));
    };

    class MockPhysicsDefaultWorldRequestsHandler : public Physics::DefaultWorldBus::Handler
    {
    public:
        MockPhysicsDefaultWorldRequestsHandler()
        {
            Physics::DefaultWorldBus::Handler::BusConnect();
        }
        ~MockPhysicsDefaultWorldRequestsHandler()
        {
            Physics::DefaultWorldBus::Handler::BusDisconnect();
        }
        MOCK_METHOD0(GetDefaultWorld, AZStd::shared_ptr<Physics::World>());
    };

    class MockBlastListener : public BlastListener
    {
    public:
        virtual ~MockBlastListener() = default;

        MOCK_METHOD2(OnActorCreated, void(const BlastFamily&, const BlastActor&));
        MOCK_METHOD2(OnActorDestroyed, void(const BlastFamily&, const BlastActor&));
    };

    class FakeBlastActor : public BlastActor
    {
    public:
        FakeBlastActor(bool isStatic, Physics::WorldBody* worldBody, MockTkActor* tkActor)
            : m_isStatic(isStatic)
            , m_transform(worldBody->GetTransform())
            , m_worldBody(worldBody)
            , m_tkActor(tkActor)
        {
        }

        AZ::Transform GetTransform() const override
        {
            return m_transform;
        }

        Physics::WorldBody* GetWorldBody() override
        {
            return m_worldBody.get();
        }

        const Physics::WorldBody* GetWorldBody() const override
        {
            return m_worldBody.get();
        }

        const AZ::Entity* GetEntity() const override
        {
            return &m_entity;
        }

        bool IsStatic() const override
        {
            return m_isStatic;
        }

        const AZStd::vector<uint32_t>& GetChunkIndices() const override
        {
            return m_chunkIndices;
        }

        Nv::Blast::TkActor& GetTkActor() const override
        {
            return *m_tkActor;
        }

        MOCK_METHOD2(Damage, void(const NvBlastDamageProgram&, NvBlastExtProgramParams*));
        MOCK_CONST_METHOD0(GetFamily, const BlastFamily&());

        bool m_isStatic;
        AZ::Transform m_transform;
        AZStd::vector<uint32_t> m_chunkIndices;
        AZ::Entity m_entity;
        AZStd::unique_ptr<Physics::WorldBody> m_worldBody;
        AZStd::unique_ptr<MockTkActor> m_tkActor;
    };

    class MockShape : public Physics::Shape
    {
    public:
        MOCK_METHOD1(SetMaterial, void(const AZStd::shared_ptr<Physics::Material>&));
        MOCK_CONST_METHOD0(GetMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_METHOD1(SetCollisionLayer, void(const Physics::CollisionLayer&));
        MOCK_CONST_METHOD0(GetCollisionLayer, Physics::CollisionLayer());
        MOCK_METHOD1(SetCollisionGroup, void(const Physics::CollisionGroup&));
        MOCK_CONST_METHOD0(GetCollisionGroup, Physics::CollisionGroup());
        MOCK_METHOD1(SetName, void(const char*));
        MOCK_METHOD2(SetLocalPose, void(const AZ::Vector3&, const AZ::Quaternion&));
        MOCK_CONST_METHOD0(GetLocalPose, AZStd::pair<AZ::Vector3, AZ::Quaternion>());
        MOCK_CONST_METHOD0(GetRestOffset, float());
        MOCK_CONST_METHOD0(GetContactOffset, float());
        MOCK_METHOD1(SetRestOffset, void(float));
        MOCK_METHOD1(SetContactOffset, void(float));
        MOCK_METHOD0(GetNativePointer, void*());
        MOCK_CONST_METHOD0(GetTag, AZ::Crc32());
        MOCK_METHOD1(AttachedToActor, void(void*));
        MOCK_METHOD0(DetachedFromActor, void());
        MOCK_METHOD2(RayCast, Physics::RayCastHit(const Physics::RayCastRequest&, const AZ::Transform&));
        MOCK_METHOD1(RayCastLocal, Physics::RayCastHit(const Physics::RayCastRequest&));
        MOCK_CONST_METHOD1(GetAabb, AZ::Aabb(const AZ::Transform&));
        MOCK_CONST_METHOD0(GetAabbLocal, AZ::Aabb());
        MOCK_METHOD3(GetGeometry, void(AZStd::vector<AZ::Vector3>&, AZStd::vector<AZ::u32>&, AZ::Aabb*));
    };

    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")

    class FakeRigidBody : public Physics::RigidBody
    {
    public:
        FakeRigidBody(AZ::EntityId entityId = AZ::EntityId(0), AZ::Transform transform = AZ::Transform::CreateZero())
            : m_entityId(entityId)
            , m_transform(transform)
        {
        }

        void UpdateMassProperties(
            Physics::MassComputeFlags flags, const AZ::Vector3* centerOfMassOffsetOverride,
            const AZ::Matrix3x3* inertiaTensorOverride, const float* massOverride) override
        {
        }

        void AddShape(AZStd::shared_ptr<Physics::Shape> shape) override {}

        void RemoveShape(AZStd::shared_ptr<Physics::Shape> shape) override {}

        AZ::Vector3 GetCenterOfMassWorld() const override
        {
            return {};
        }

        AZ::Vector3 GetCenterOfMassLocal() const override
        {
            return {};
        }

        AZ::Matrix3x3 GetInverseInertiaWorld() const override
        {
            return {};
        }

        AZ::Matrix3x3 GetInverseInertiaLocal() const override
        {
            return {};
        }

        float GetMass() const override
        {
            return {};
        }

        float GetInverseMass() const override
        {
            return {};
        }

        void SetMass(float mass) override {}

        void SetCenterOfMassOffset(const AZ::Vector3& comOffset) override {}

        AZ::Vector3 GetLinearVelocity() const override
        {
            return {};
        }

        void SetLinearVelocity(const AZ::Vector3& velocity) override {}

        AZ::Vector3 GetAngularVelocity() const override
        {
            return {};
        }

        void SetAngularVelocity(const AZ::Vector3& angularVelocity) override {}

        AZ::Vector3 GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) override
        {
            return {};
        }

        void ApplyLinearImpulse(const AZ::Vector3& impulse) override {}

        void ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldPoint) override {}

        void ApplyAngularImpulse(const AZ::Vector3& angularImpulse) override {}

        float GetLinearDamping() const override
        {
            return {};
        }

        void SetLinearDamping(float damping) override {}

        float GetAngularDamping() const override
        {
            return {};
        }

        void SetAngularDamping(float damping) override {}

        bool IsAwake() const override
        {
            return {};
        }

        void ForceAsleep() override {}

        void ForceAwake() override {}

        float GetSleepThreshold() const override
        {
            return {};
        }

        void SetSleepThreshold(float threshold) override {}

        bool IsKinematic() const override
        {
            return {};
        }

        void SetKinematic(bool kinematic) override {}

        void SetKinematicTarget(const AZ::Transform& targetPosition) override {}

        bool IsGravityEnabled() const override
        {
            return {};
        }

        void SetGravityEnabled(bool enabled) override {}

        void SetSimulationEnabled(bool enabled) override {}

        void SetCCDEnabled(bool enabled) override {}

        void UpdateCenterOfMassAndInertia(
            bool computeCenterOfMass, const AZ::Vector3& centerOfMassOffset, bool computeInertia,
            const AZ::Matrix3x3& inertiaTensor) override
        {
        }

        AZ::EntityId GetEntityId() const override
        {
            return m_entityId;
        }

        Physics::World* GetWorld() const override
        {
            return nullptr;
        }

        AZ::Transform GetTransform() const override
        {
            return m_transform;
        }

        void SetTransform(const AZ::Transform& transform) override
        {
            m_transform = transform;
        }

        AZ::Vector3 GetPosition() const override
        {
            return m_transform.GetPosition();
        }

        AZ::Quaternion GetOrientation() const override
        {
            return AZ::Quaternion::CreateFromTransform(m_transform);
        }

        AZ::Aabb GetAabb() const override
        {
            return {};
        }

        Physics::RayCastHit RayCast(const Physics::RayCastRequest& request) override
        {
            return {};
        }

        AZ::Crc32 GetNativeType() const override
        {
            return NULL;
        }

        void* GetNativePointer() const override
        {
            return nullptr;
        }

        void AddToWorld(Physics::World&) override {}

        void RemoveFromWorld(Physics::World&) override {}

        AZ::EntityId m_entityId;
        AZ::Transform m_transform;
    };

    AZ_POP_DISABLE_WARNING

    class FakeActorFactory : public BlastActorFactory
    {
    public:
        MOCK_CONST_METHOD2(
            CalculateVisibleChunks, AZStd::vector<uint32_t>(const BlastFamily&, const Nv::Blast::TkActor&));
        MOCK_CONST_METHOD2(CalculateIsLeafChunk, bool(const Nv::Blast::TkActor&, const AZStd::vector<uint32_t>&));
        MOCK_CONST_METHOD3(
            CalculateIsStatic, bool(const BlastFamily&, const Nv::Blast::TkActor&, const AZStd::vector<uint32_t>&));
        MOCK_CONST_METHOD1(CalculateComponents, AZStd::vector<AZ::Uuid>(bool));

        FakeActorFactory(uint32_t size, bool isStatic = false)
            : m_mockActors(size)
            , m_index(0)
        {
            for (uint32_t i = 0; i < size; ++i)
            {
                m_mockActors[i] = aznew FakeBlastActor(isStatic, aznew FakeRigidBody(), new MockTkActor());
            }
        }

        ~FakeActorFactory()
        {
            for (uint32_t i = 0; i < m_mockActors.size(); ++i)
            {
                delete m_mockActors[i];
            }
            m_mockActors.clear();
        }

        BlastActor* CreateActor(const BlastActorDesc& desc) override
        {
            return m_mockActors[m_index++];
        }

        void DestroyActor([[maybe_unused]] BlastActor* actor) override {}

        std::vector<FakeBlastActor*> m_mockActors;
        int m_index;
    };

    class FakeEntityProvider : public EntityProvider
    {
    public:
        FakeEntityProvider(uint32_t entityCount)
        {
            for (int i = 0; i < entityCount; ++i)
            {
                m_entities.push_back(AZStd::make_shared<AZ::Entity>());
            }
            for (auto& entity : m_entities)
            {
                m_createdEntityIds.push_back(entity->GetId());
            }
        }

        AZStd::shared_ptr<AZ::Entity> CreateEntity(const AZStd::vector<AZ::Uuid>& components) override
        {
            auto entity = m_entities.back();
            m_entities.pop_back();
            return entity;
        }

        AZStd::vector<AZ::EntityId> m_createdEntityIds;
        AZStd::vector<AZStd::shared_ptr<AZ::Entity>> m_entities;
    };

    class MockTransformBusHandler : public AZ::TransformBus::MultiHandler
    {
    public:
        void Connect(AZ::EntityId id)
        {
            AZ::TransformBus::MultiHandler::BusConnect(id);
        }

        ~MockTransformBusHandler()
        {
            AZ::TransformBus::MultiHandler::BusDisconnect();
        }

        MOCK_METHOD0(GetLocalTM, const AZ::Transform&());
        MOCK_METHOD1(SetLocalTM, void(const AZ::Transform&));
        MOCK_METHOD0(GetWorldTM, const AZ::Transform&());
        MOCK_METHOD1(SetWorldTM, void(const AZ::Transform&));
        MOCK_METHOD2(GetLocalAndWorld, void(AZ::Transform&, AZ::Transform&));
        MOCK_METHOD1(SetWorldTranslation, void(const AZ::Vector3&));
        MOCK_METHOD1(SetLocalTranslation, void(const AZ::Vector3&));
        MOCK_METHOD0(GetWorldTranslation, AZ::Vector3());
        MOCK_METHOD0(GetLocalTranslation, AZ::Vector3());
        MOCK_METHOD1(MoveEntity, void(const AZ::Vector3&));
        MOCK_METHOD1(SetWorldX, void(float));
        MOCK_METHOD1(SetWorldY, void(float));
        MOCK_METHOD1(SetWorldZ, void(float));
        MOCK_METHOD0(GetWorldX, float());
        MOCK_METHOD0(GetWorldY, float());
        MOCK_METHOD0(GetWorldZ, float());
        MOCK_METHOD1(SetLocalX, void(float));
        MOCK_METHOD1(SetLocalY, void(float));
        MOCK_METHOD1(SetLocalZ, void(float));
        MOCK_METHOD0(GetLocalX, float());
        MOCK_METHOD0(GetLocalY, float());
        MOCK_METHOD0(GetLocalZ, float());
        MOCK_METHOD1(SetRotation, void(const AZ::Vector3&));
        MOCK_METHOD1(SetRotationX, void(float));
        MOCK_METHOD1(SetRotationY, void(float));
        MOCK_METHOD1(SetRotationZ, void(float));
        MOCK_METHOD1(SetRotationQuaternion, void(const AZ::Quaternion&));
        MOCK_METHOD1(RotateByX, void(float));
        MOCK_METHOD1(RotateByY, void(float));
        MOCK_METHOD1(RotateByZ, void(float));
        MOCK_METHOD0(GetRotationEulerRadians, AZ::Vector3());
        MOCK_METHOD0(GetRotationQuaternion, AZ::Quaternion());
        MOCK_METHOD0(GetRotationX, float());
        MOCK_METHOD0(GetRotationY, float());
        MOCK_METHOD0(GetRotationZ, float());
        MOCK_METHOD0(GetWorldRotation, AZ::Vector3());
        MOCK_METHOD0(GetWorldRotationQuaternion, AZ::Quaternion());
        MOCK_METHOD1(SetLocalRotation, void(const AZ::Vector3&));
        MOCK_METHOD1(SetLocalRotationQuaternion, void(const AZ::Quaternion&));
        MOCK_METHOD1(RotateAroundLocalX, void(float));
        MOCK_METHOD1(RotateAroundLocalY, void(float));
        MOCK_METHOD1(RotateAroundLocalZ, void(float));
        MOCK_METHOD0(GetLocalRotation, AZ::Vector3());
        MOCK_METHOD0(GetLocalRotationQuaternion, AZ::Quaternion());
        MOCK_METHOD1(SetScale, void(const AZ::Vector3&));
        MOCK_METHOD1(SetScaleX, void(float));
        MOCK_METHOD1(SetScaleY, void(float));
        MOCK_METHOD1(SetScaleZ, void(float));
        MOCK_METHOD0(GetScale, AZ::Vector3());
        MOCK_METHOD0(GetScaleX, float());
        MOCK_METHOD0(GetScaleY, float());
        MOCK_METHOD0(GetScaleZ, float());
        MOCK_METHOD1(SetLocalScale, void(const AZ::Vector3&));
        MOCK_METHOD1(SetLocalScaleX, void(float));
        MOCK_METHOD1(SetLocalScaleY, void(float));
        MOCK_METHOD1(SetLocalScaleZ, void(float));
        MOCK_METHOD0(GetLocalScale, AZ::Vector3());
        MOCK_METHOD0(GetWorldScale, AZ::Vector3());
        MOCK_METHOD0(GetParentId, AZ::EntityId());
        MOCK_METHOD0(GetParent, TransformInterface*());
        MOCK_METHOD1(SetParent, void(AZ::EntityId));
        MOCK_METHOD1(SetParentRelative, void(AZ::EntityId));
        MOCK_METHOD0(GetChildren, AZStd::vector<AZ::EntityId>());
        MOCK_METHOD0(GetAllDescendants, AZStd::vector<AZ::EntityId>());
        MOCK_METHOD0(GetEntityAndAllDescendants, AZStd::vector<AZ::EntityId>());
        MOCK_METHOD0(IsStaticTransform, bool());
        MOCK_METHOD1(SetIsStaticTransform, void(bool));
        MOCK_METHOD0(IsPositionInterpolated, bool());
        MOCK_METHOD0(IsRotationInterpolated, bool());
    };

    class MockMeshComponentRequestBusHandler : public LmbrCentral::MeshComponentRequestBus::MultiHandler
    {
    public:
        void Connect(AZ::EntityId id)
        {
            LmbrCentral::MeshComponentRequestBus::MultiHandler::BusConnect(id);
        }

        ~MockMeshComponentRequestBusHandler()
        {
            LmbrCentral::MeshComponentRequestBus::MultiHandler::BusDisconnect();
        }

        MOCK_METHOD0(GetWorldBounds, AZ::Aabb());
        MOCK_METHOD0(GetLocalBounds, AZ::Aabb());
        MOCK_METHOD1(SetMeshAsset, void(const AZ::Data::AssetId&));
        MOCK_METHOD0(GetMeshAsset, AZ::Data::Asset<AZ::Data::AssetData>());
        MOCK_METHOD0(GetVisibility, bool());
        MOCK_METHOD1(SetVisibility, void(bool));
    };

    class MockRigidBodyRequestBusHandler : public Physics::RigidBodyRequestBus::MultiHandler
    {
    public:
        void Connect(AZ::EntityId id)
        {
            Physics::RigidBodyRequestBus::MultiHandler::BusConnect(id);
        }

        ~MockRigidBodyRequestBusHandler()
        {
            Physics::RigidBodyRequestBus::MultiHandler::BusDisconnect();
        }

        MOCK_METHOD0(EnablePhysics, void());
        MOCK_METHOD0(DisablePhysics, void());
        MOCK_CONST_METHOD0(IsPhysicsEnabled, bool());
        MOCK_CONST_METHOD0(GetCenterOfMassWorld, AZ::Vector3());
        MOCK_CONST_METHOD0(GetCenterOfMassLocal, AZ::Vector3());
        MOCK_CONST_METHOD0(GetInverseInertiaWorld, AZ::Matrix3x3());
        MOCK_CONST_METHOD0(GetInverseInertiaLocal, AZ::Matrix3x3());
        MOCK_CONST_METHOD0(GetMass, float());
        MOCK_CONST_METHOD0(GetInverseMass, float());
        MOCK_METHOD1(SetMass, void(float));
        MOCK_METHOD1(SetCenterOfMassOffset, void(const AZ::Vector3&));
        MOCK_CONST_METHOD0(GetLinearVelocity, AZ::Vector3());
        MOCK_METHOD1(SetLinearVelocity, void(const AZ::Vector3&));
        MOCK_CONST_METHOD0(GetAngularVelocity, AZ::Vector3());
        MOCK_METHOD1(SetAngularVelocity, void(const AZ::Vector3&));
        MOCK_CONST_METHOD1(GetLinearVelocityAtWorldPoint, AZ::Vector3(const AZ::Vector3&));
        MOCK_METHOD1(ApplyLinearImpulse, void(const AZ::Vector3&));
        MOCK_METHOD2(ApplyLinearImpulseAtWorldPoint, void(const AZ::Vector3&, const AZ::Vector3&));
        MOCK_METHOD1(ApplyAngularImpulse, void(const AZ::Vector3&));
        MOCK_CONST_METHOD0(GetLinearDamping, float());
        MOCK_METHOD1(SetLinearDamping, void(float));
        MOCK_CONST_METHOD0(GetAngularDamping, float());
        MOCK_METHOD1(SetAngularDamping, void(float));
        MOCK_CONST_METHOD0(IsAwake, bool());
        MOCK_METHOD0(ForceAsleep, void());
        MOCK_METHOD0(ForceAwake, void());
        MOCK_CONST_METHOD0(GetSleepThreshold, float());
        MOCK_METHOD1(SetSleepThreshold, void(float));
        MOCK_CONST_METHOD0(IsKinematic, bool());
        MOCK_METHOD1(SetKinematic, void(bool));
        MOCK_METHOD1(SetKinematicTarget, void(const AZ::Transform&));
        MOCK_CONST_METHOD0(IsGravityEnabled, bool());
        MOCK_METHOD1(SetGravityEnabled, void(bool));
        MOCK_METHOD1(SetSimulationEnabled, void(bool));
        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb());
        MOCK_METHOD0(GetRigidBody, Physics::RigidBody*());
        MOCK_METHOD1(RayCast, Physics::RayCastHit(const Physics::RayCastRequest&));
    };

    class FakeBlastFamily : public BlastFamily
    {
    public:
        FakeBlastFamily()
            : m_pxAsset(NvBlastActorDesc{1, nullptr, 1, nullptr})
        {
        }

        const Nv::Blast::TkFamily* GetTkFamily() const override
        {
            return &m_tkFamily;
        }

        Nv::Blast::TkFamily* GetTkFamily() override
        {
            return &m_tkFamily;
        }

        const Nv::Blast::ExtPxAsset& GetPxAsset() const override
        {
            return m_pxAsset;
        }

        const BlastActorConfiguration& GetActorConfiguration() const override
        {
            return m_actorConfiguration;
        }

        MOCK_METHOD1(Spawn, bool(const AZ::Transform&));
        MOCK_METHOD0(Despawn, void());
        MOCK_METHOD2(HandleEvents, void(const Nv::Blast::TkEvent*, uint32_t));
        MOCK_METHOD1(RegisterListener, void(BlastListener&));
        MOCK_METHOD1(UnregisterListener, void(BlastListener&));
        MOCK_METHOD1(DestroyActor, void(BlastActor*));
        MOCK_METHOD0(GetActorTracker, ActorTracker&());
        MOCK_METHOD3(FillDebugRender, void(DebugRenderBuffer&, DebugRenderMode, float));

        FakeExtPxAsset m_pxAsset;
        MockTkFamily m_tkFamily;
        BlastActorConfiguration m_actorConfiguration;
    };

    class MockBlastSystemBusHandler : public AZ::Interface<Blast::BlastSystemRequests>::Registrar
    {
    public:
        MOCK_CONST_METHOD0(GetTkFramework, Nv::Blast::TkFramework*());
        MOCK_CONST_METHOD0(GetExtSerialization, Nv::Blast::ExtSerialization*());
        MOCK_METHOD0(GetTkGroup, Nv::Blast::TkGroup*());
        MOCK_CONST_METHOD0(GetGlobalConfiguration, const BlastGlobalConfiguration&());
        MOCK_METHOD1(SetGlobalConfiguration, void(const BlastGlobalConfiguration&));
        MOCK_METHOD0(InitPhysics, void());
        MOCK_METHOD0(DeactivatePhysics, void());
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtRadialDamageDesc>));
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtCapsuleRadialDamageDesc>));
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtShearDamageDesc>));
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtTriangleIntersectionDamageDesc>));
        MOCK_METHOD1(AddDamageDesc, void(AZStd::unique_ptr<NvBlastExtImpactSpreadDamageDesc>));
        MOCK_METHOD1(AddProgramParams, void(AZStd::unique_ptr<NvBlastExtProgramParams>));
        MOCK_METHOD1(SetDebugRenderMode, void(DebugRenderMode));
    };
} // namespace Blast
