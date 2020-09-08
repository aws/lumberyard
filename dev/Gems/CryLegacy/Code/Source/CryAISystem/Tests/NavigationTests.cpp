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
#include "CryLegacy_precompiled.h"
#include <AzTest/AzTest.h>
#include <gmock/gmock.h>
#include "Navigation/MNM/Voxelizer.h"
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>
#include "Mocks/ICVarMock.h"
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/Shape.h>

namespace AINavigationTests 
{
    using namespace ::testing;

    class WorldVoxelizerHelper
        : public MNM::WorldVoxelizer
    {
        friend class GTEST_TEST_CLASS_NAME_(NavigationTests, Voxelizer_GetsAzPhysicsCollidersInAabb_FT);
        friend class GTEST_TEST_CLASS_NAME_(NavigationTests, Voxelizer_RasterizesAzPhysicsGeometry_FT);
        friend class GTEST_TEST_CLASS_NAME_(NavigationTests, Voxelizer_UsesCorrectPhysicsSystem_FT);
    };

    struct MockGlobalEnvironment
    {
        MockGlobalEnvironment()
        {
            m_stubEnv.pConsole = &m_stubConsole;
            m_stubEnv.pSystem = &m_stubSystem;
            m_stubEnv.p3DEngine = nullptr;
            m_stubEnv.pPhysicalWorld = nullptr;

            gEnv = &m_stubEnv;
        }

        ~MockGlobalEnvironment()
        {
            gEnv = nullptr;
        }

        SSystemGlobalEnvironment m_stubEnv;
        NiceMock<ConsoleMock> m_stubConsole;
        NiceMock<SystemMock> m_stubSystem;
    };

    class MockShape        
        : public Physics::Shape
    {
    public:
        MOCK_METHOD1(SetMaterial, void(const AZStd::shared_ptr<Physics::Material>& material));
        MOCK_CONST_METHOD0(GetMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_METHOD1(SetCollisionLayer, void(const Physics::CollisionLayer& layer));
        MOCK_CONST_METHOD0(GetCollisionLayer, Physics::CollisionLayer());
        MOCK_METHOD1(SetCollisionGroup, void(const Physics::CollisionGroup& group));
        MOCK_CONST_METHOD0(GetCollisionGroup, Physics::CollisionGroup());
        MOCK_METHOD1(SetName, void(const char* name));
        MOCK_METHOD2(SetLocalPose, void(const AZ::Vector3& offset, const AZ::Quaternion& rotation));
        MOCK_CONST_METHOD0(GetLocalPose, AZStd::pair<AZ::Vector3, AZ::Quaternion>());
        MOCK_METHOD0(GetNativePointer, void*());
        MOCK_CONST_METHOD0(GetTag, AZ::Crc32());
        MOCK_METHOD1(AttachedToActor, void(void* actor));
        MOCK_METHOD0(DetachedFromActor, void());
        MOCK_METHOD2(RayCast, Physics::RayCastHit(const Physics::RayCastRequest& worldSpaceRequest, const AZ::Transform& worldTransform));
        MOCK_METHOD1(RayCastLocal, Physics::RayCastHit(const Physics::RayCastRequest& localSpaceRequest));

        void GetGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb* optionalBounds) override
        {
            vertices = m_geometryVertices;
        }

        AZStd::vector<AZ::Vector3> m_geometryVertices;
    };

    class MockWorldBody
        : public Physics::WorldBody
    {
    public:
        MOCK_CONST_METHOD0(GetEntityId, AZ::EntityId());
        MOCK_CONST_METHOD0(GetWorld, Physics::World*());
        MOCK_CONST_METHOD0(GetTransform, AZ::Transform());
        MOCK_METHOD1(SetTransform, void(const AZ::Transform& transform));
        MOCK_CONST_METHOD0(GetPosition, AZ::Vector3());
        MOCK_CONST_METHOD0(GetOrientation, AZ::Quaternion());
        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb());
        MOCK_METHOD1(RayCast, Physics::RayCastHit(const Physics::RayCastRequest& request));
        MOCK_CONST_METHOD0(GetNativeType, AZ::Crc32());
        MOCK_CONST_METHOD0(GetNativePointer, void*());
        MOCK_METHOD1(AddToWorld, void(Physics::World&));
        MOCK_METHOD1(RemoveFromWorld, void(Physics::World&));
    };

    class MockWorld
        : public Physics::WorldRequestBus::Handler
    {
    public:
        MockWorld()
        {
            Physics::WorldRequestBus::Handler::BusConnect(Physics::DefaultPhysicsWorldId);
        }
        ~MockWorld() override
        {
            Physics::WorldRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(Update, void(float deltaTime));
        MOCK_METHOD1(StartSimulation, void(float deltaTime));
        MOCK_METHOD0(FinishSimulation, void());
        MOCK_METHOD1(RayCast, Physics::RayCastHit(const Physics::RayCastRequest& request));
        MOCK_METHOD1(RayCastMultiple, AZStd::vector<Physics::RayCastHit>(const Physics::RayCastRequest& request));
        MOCK_METHOD1(ShapeCast, Physics::RayCastHit(const Physics::ShapeCastRequest& request));
        MOCK_METHOD1(ShapeCastMultiple, AZStd::vector<Physics::RayCastHit>(const Physics::ShapeCastRequest& request));
        MOCK_METHOD1(Overlap, AZStd::vector<Physics::OverlapHit>(const Physics::OverlapRequest& request));
        MOCK_METHOD2(RegisterSuppressedCollision, void(const Physics::WorldBody& body0, const Physics::WorldBody& body1));
        MOCK_METHOD2(UnregisterSuppressedCollision, void(const Physics::WorldBody& body0, const Physics::WorldBody& body1));
        MOCK_METHOD1(AddBody, void(Physics::WorldBody& body));
        MOCK_METHOD1(RemoveBody, void(Physics::WorldBody& body));
        MOCK_METHOD1(SetSimFunc, void(std::function<void(void*)> func));
        MOCK_METHOD1(SetEventHandler, void(Physics::WorldEventHandler* eventHandler));
        MOCK_METHOD0(GetGravity, AZ::Vector3());
        MOCK_METHOD1(SetGravity, void(const AZ::Vector3& gravity));
        MOCK_METHOD1(SetMaxDeltaTime, void(float maxDeltaTime));
        MOCK_METHOD1(SetFixedDeltaTime, void(float fixedDeltaTime));
        MOCK_METHOD1(DeferDelete, void(AZStd::unique_ptr<Physics::WorldBody> worldBody));
        MOCK_METHOD1(SetTriggerEventCallback, void(Physics::ITriggerEventCallback* triggerCallback));
        MOCK_CONST_METHOD0(GetWorldId, AZ::Crc32());
    };

    class NavigationTests
        : public ::testing::Test
    {
    protected:
        void PrepareSingleTriangleGeometry(AZStd::vector<Physics::OverlapHit>& overlapHits)
        {
            ON_CALL(m_worldBody, GetNativePointer())
                .WillByDefault(Return(&m_worldBody));
            ON_CALL(m_worldBody, GetTransform())
                .WillByDefault(Return(AZ::Transform::CreateIdentity()));
            ON_CALL(m_shape, GetNativePointer())
                .WillByDefault(Return(&m_shape));

            // create a single triangle in a triangle list
            m_shape.m_geometryVertices.clear();
            m_shape.m_geometryVertices.push_back(AZ::Vector3(0.f, 0.f, 0.f));
            m_shape.m_geometryVertices.push_back(AZ::Vector3(1.f, 0.f, 0.f));
            m_shape.m_geometryVertices.push_back(AZ::Vector3(0.f, 1.f, 0.f));

            Physics::OverlapHit hit;
            hit.m_body = &m_worldBody;
            hit.m_shape = &m_shape;
            overlapHits.push_back(hit);

            ON_CALL(m_worldMock, Overlap(_))
                .WillByDefault(Return(overlapHits));
        }

        NiceMock<MockWorldBody> m_worldBody;
        NiceMock<MockShape> m_shape;
        NiceMock<MockGlobalEnvironment> m_mocks;
        NiceMock<MockWorld> m_worldMock;
        WorldVoxelizerHelper m_voxelizer;
    };

    TEST_F(NavigationTests, Voxelizer_GetsAzPhysicsCollidersInAabb_FT)
    {
        constexpr float radius = 100.f;
        AABB aabb{Vec3_Zero, radius};
        AZStd::vector<Physics::OverlapHit> defaultOverlapHits, editorOverlapHits;
        MockWorldBody defaultWorldBody, editorWorldBody;
        Physics::OverlapHit hit;

        // default world hits
        hit.m_body = &defaultWorldBody;
        defaultOverlapHits.push_back(hit);

        // editor world hits
        hit.m_body = &editorWorldBody;
        editorOverlapHits.push_back(hit);

        AZStd::vector<Physics::OverlapHit> overlapHits;

        // expect no hits returned when there are no colliders
        m_voxelizer.GetAZCollidersInAABB(aabb, overlapHits);
        EXPECT_TRUE(overlapHits.empty());

        // set up default physics world 
        gEnv->SetIsEditor(false);

        // return default colliders
        ON_CALL(m_worldMock, Overlap(_))
            .WillByDefault(Return(defaultOverlapHits));

        // expect 1 hit returned with the default world body
        overlapHits.clear();
        m_voxelizer.GetAZCollidersInAABB(aabb, overlapHits);
        EXPECT_EQ(overlapHits.size(),1);
        EXPECT_EQ(overlapHits[0].m_body,&defaultWorldBody);

        // set to editor world
        gEnv->SetIsEditor(true);

        // return editor colliders
        ON_CALL(m_worldMock, Overlap(_))
            .WillByDefault(Return(editorOverlapHits));

        // expect 1 hit returned with the editor world body
        overlapHits.clear();
        m_voxelizer.GetAZCollidersInAABB(aabb, overlapHits);
        EXPECT_EQ(overlapHits.size(),1);
        EXPECT_EQ(overlapHits[0].m_body,&editorWorldBody);
    }

    TEST_F(NavigationTests, Voxelizer_RasterizesAzPhysicsGeometry_FT)
    {
        constexpr float radius = 100.f;
        AABB aabb{Vec3_Zero, radius};
        m_voxelizer.m_volumeAABB = aabb;
        AZStd::vector<Physics::OverlapHit> overlapHits;

        // with no hits expect no triangles 
        size_t triCount = m_voxelizer.RasterizeAZColliderGeometry(aabb, overlapHits);
        EXPECT_EQ(triCount,0);

        // with 1 hit but no body expect no triangles 
        Physics::OverlapHit hit;
        hit.m_body = nullptr;
        overlapHits.push_back(hit);
        triCount = m_voxelizer.RasterizeAZColliderGeometry(aabb, overlapHits);
        EXPECT_EQ(triCount,0);

        // with 1 hit and a body expect triangles 
        PrepareSingleTriangleGeometry(overlapHits);
        triCount = m_voxelizer.RasterizeAZColliderGeometry(aabb, overlapHits);
        EXPECT_GE(triCount,1);
    }

    TEST_F(NavigationTests, Voxelizer_UsesCorrectPhysicsSystem_FT)
    {
        m_voxelizer.m_volumeAABB = AABB(Vec3_Zero, 100.f);

        // set up default physics world 
        gEnv->SetIsEditor(false);

        uint32 hashValueSeed{ 0 };
        uint32 hashTest{ 0 };
        uint32 hashValue{ 0 };

        AZ_Assert(gEnv->pConsole, "Global environment console system is missing");

        // setup to expect the ai_NavPhysicsMode cvar call and return 2
        NiceMock<CVarMock> cvarMock;
        ON_CALL(m_mocks.m_stubConsole, GetCVar("ai_NavPhysicsMode"))
            .WillByDefault(Return(&cvarMock));

        // set physics mode to AZ::Physics only  (2)
        ON_CALL(cvarMock, GetIVal())
            .WillByDefault(Return(2));

        size_t triCount = m_voxelizer.ProcessGeometry(hashValueSeed, hashTest, &hashValue);

        // expect we rasterized no geometry yet 
        EXPECT_EQ(triCount, 0);

        AZStd::vector<Physics::OverlapHit> overlapHits;
        PrepareSingleTriangleGeometry(overlapHits);

        triCount = m_voxelizer.ProcessGeometry(hashValueSeed, hashTest, &hashValue);
        EXPECT_EQ(triCount, 1);

        // set physics mode to CryPhysics only
        ON_CALL(cvarMock, GetIVal())
            .WillByDefault(Return(0));
        triCount = m_voxelizer.ProcessGeometry(hashValueSeed, hashTest, &hashValue);

        // expect we rasterized nothing (CryPhysics is disabled) 
        EXPECT_EQ(triCount,0);

        // set physics mode to CryPhysics and AZ::Physics 
        ON_CALL(cvarMock, GetIVal())
            .WillByDefault(Return(1));
        triCount = m_voxelizer.ProcessGeometry(hashValueSeed, hashTest, &hashValue);

        // expect we rasterized geometry
        EXPECT_EQ(triCount,1);
    }
}
