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

#include <PhysX_precompiled.h>

#include <AzTest/AzTest.h>
#include <Source/PhysXMathConversion.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <PhysXRigidBodyComponent.h>

namespace PhysX
{
    class PhysXSpecificTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
        }

        void TearDown() override
        {
        }

        float tolerance = 1e-3f;
    };

    TEST_F(PhysXSpecificTest, VectorConversion_ConvertToPxVec3_ConvertedVectorsCorrect)
    {
        AZ::Vector3 lyA(3.0f, -4.0f, 12.0f);
        AZ::Vector3 lyB(-8.0f, 1.0f, -4.0f);

        physx::PxVec3 pxA = PxVec3FromLYVec3(lyA);
        physx::PxVec3 pxB = PxVec3FromLYVec3(lyB);

        EXPECT_NEAR(pxA.magnitudeSquared(), 169.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxB.magnitudeSquared(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.dot(pxB), -76.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).x, 4.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).y, -84.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).z, -29.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, VectorConversion_ConvertToLyVec3_ConvertedVectorsCorrect)
    {
        physx::PxVec3 pxA(3.0f, -4.0f, 12.0f);
        physx::PxVec3 pxB(-8.0f, 1.0f, -4.0f);

        AZ::Vector3 lyA = LYVec3FromPxVec3(pxA);
        AZ::Vector3 lyB = LYVec3FromPxVec3(pxB);

        EXPECT_NEAR(lyA.GetLengthSq(), 169.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyB.GetLengthSq(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Dot(lyB), -76.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetX(), 4.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetY(), -84.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetZ(), -29.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, QuaternionConversion_ConvertToPxQuat_ConvertedQuatsCorrect)
    {
        AZ::Quaternion lyQ = AZ::Quaternion(9.0f, -8.0f, -4.0f, 8.0f) / 15.0f;
        physx::PxQuat pxQ = PxQuatFromLYQuat(lyQ);
        physx::PxVec3 pxV = pxQ.rotate(physx::PxVec3(-8.0f, 1.0f, -4.0f));

        EXPECT_NEAR(pxQ.magnitudeSquared(), 1.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxQ.getImaginaryPart().magnitudeSquared(), 161.0f / 225.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxQ.w, 8.0f / 15.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.magnitudeSquared(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.x, 8.0f / 9.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.y, 403.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.z, 4.0f / 45.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, QuaternionConversion_ConvertToLyQuat_ConvertedQuatsCorrect)
    {
        physx::PxQuat pxQ = physx::PxQuat(9.0f, -8.0f, -4.0f, 8.0f) * (1.0 / 15.0f);
        AZ::Quaternion lyQ = LYQuatFromPxQuat(pxQ);
        AZ::Vector3 lyV = lyQ * AZ::Vector3(-8.0f, 1.0f, -4.0f);

        EXPECT_NEAR(lyQ.GetLengthSq(), 1.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyQ.GetImaginary().GetLengthSq(), 161.0f / 225.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyQ.GetW(), 8.0f / 15.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetLengthSq(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetX(), 8.0f / 9.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetY(), 403.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetZ(), 4.0f / 45.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, TransformConversion_ConvertToPxTransform_ConvertedTransformsCorrect)
    {
        AZ::Transform lyTm;
        lyTm.SetBasisX(AZ::Vector3(39.0f, -52.0f, 0.0f) / 65.0f);
        lyTm.SetBasisY(AZ::Vector3(48.0f, 36.0f, -25.0f) / 65.0f);
        lyTm.SetBasisZ(AZ::Vector3(20.0f, 15.0f, 60.0f) / 65.0f);
        lyTm.SetPosition(AZ::Vector3(-6.0f, 5.0f, -3.0f));

        physx::PxTransform pxTm = PxTransformFromLYTransform(lyTm);
        physx::PxVec3 pxV = pxTm.transform(physx::PxVec3(8.0f, 1.0f, 4.0f));

        EXPECT_NEAR(pxV.x, 90.0f / 117.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.y, 9.0f / 117.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.z, 36.0f / 117.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, TransformConversion_ConvertToLyTransform_ConvertedTransformsCorrect)
    {
        physx::PxTransform pxTm(physx::PxVec3(2.0f, 10.0f, 9.0f), physx::PxQuat(6.0f, -8.0f, -5.0f, 10.0f) * (1.0f / 15.0f));
        AZ::Transform lyTm = LYTransformFromPxTransform(pxTm);
        AZ::Vector3 lyV = lyTm * AZ::Vector3(4.0f, -12.0f, 3.0f);

        EXPECT_NEAR(lyV.GetX(), -14.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetY(), 22.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetZ(), 4.0f / 9.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, RigidBody_GetNativeShape_ReturnsCorrectShape)
    {
        Physics::Ptr<Physics::RigidBodySettings> rigidBodySettings = Physics::RigidBodySettings::Create();
        AZ::Vector3 halfExtents(1.0f, 2.0f, 3.0f);
        rigidBodySettings->m_bodyShape = Physics::BoxShapeConfiguration::Create(halfExtents);
        Physics::Ptr<Physics::RigidBody> rigidBody = nullptr;
        Physics::SystemRequestBus::BroadcastResult(rigidBody, &Physics::SystemRequests::CreateRigidBody, rigidBodySettings);
        ASSERT_TRUE(rigidBody != nullptr);

        auto nativeShape = rigidBody->GetNativeShape();
        ASSERT_TRUE(nativeShape->m_nativePointer != nullptr);

        auto pxShape = static_cast<physx::PxShape*>(nativeShape->m_nativePointer);
        ASSERT_TRUE(pxShape->getGeometryType() == physx::PxGeometryType::eBOX);

        physx::PxBoxGeometry boxGeometry;
        pxShape->getBoxGeometry(boxGeometry);

        EXPECT_NEAR(boxGeometry.halfExtents.x, halfExtents.GetX(), PhysXSpecificTest::tolerance);
        EXPECT_NEAR(boxGeometry.halfExtents.y, halfExtents.GetY(), PhysXSpecificTest::tolerance);
        EXPECT_NEAR(boxGeometry.halfExtents.z, halfExtents.GetZ(), PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_NoTerrain_ReturnsFalse)
    {
        float height = 0.0f;
        bool result = false;
        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 0, 0, height);
        EXPECT_FALSE(result);
        EXPECT_EQ(height, 0.0f);
    }

    void UpdateTestTerrainTile(AZ::u32 x, AZ::u32 y, float minHeight)
    {
        // creates a 2 x 2 terrain tile with a diagonal slope
        float heightScale = 0.01f;
        AZ::u32 tileSize = 2;
        AZ::u32 heightMapUnitSize = 1;
        AZ::u16 heightMap[9] = { 100, 200, 0, 200, 300, 0, 0, 0, 0 };
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::UpdateTile,
            x, y, heightMap, minHeight, heightScale, tileSize, heightMapUnitSize);
    }

    void CreateTestTerrain(AZ::u32 numTiles)
    {
        // creates terrain with a diagonal slope
        AZ::u32 tileSize = 2;
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::SetNumTiles, numTiles, tileSize);
        for (AZ::u32 x = numTiles; x-- > 0;)
        {
            for (AZ::u32 y = numTiles; y-- > 0;)
            {
                float minHeight = 6.0f + 2.0f * (x + y);
                UpdateTestTerrainTile(x, y, minHeight);
            }
        }
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_TestTerrain_CorrectHeightValues)
    {
        float heightScale = 0.01f;
        CreateTestTerrain(2);

        // make some height queries
        float height = 0.0f;
        bool result = false;
        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 0, 0, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(7.0f, height, heightScale);

        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 2, 3, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(12.0f, height, heightScale);

        // clear up the terrain
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::SetNumTiles, 0, 0);
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_RequestOutsideBounds_ClampedCorrectly)
    {
        float heightScale = 0.01f;
        CreateTestTerrain(2);

        // make some queries outside the terrain to test co-ordinate clamping
        float height = 0.0f;
        bool result = false;
        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 5, 2, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(12.0f, height, heightScale);

        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 7, 1, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(11.0f, height, heightScale);

        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 6, 7, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(13.0f, height, heightScale);

        // clear up the terrain
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::SetNumTiles, 0, 0);
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_QueryAfterTileUpdate_CorrectHeightValues)
    {
        float heightScale = 0.01f;
        CreateTestTerrain(2);

        // update a tile and check the height values are still as expected
        float height = 0.0f;
        bool result = false;
        UpdateTestTerrainTile(0, 0, 4.0f);
        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 0, 0, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(5.0f, height, heightScale);

        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 0, 1, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(6.0f, height, heightScale);

        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 3, 0, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(10.0f, height, heightScale);

        // clear up the terrain
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::SetNumTiles, 0, 0);
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_QueryAfterTileUpdateWhichForcesHeightRescale_CorrectHeightValues)
    {
        float heightScale = 0.01f;
        CreateTestTerrain(2);

        // make a tile update which will force the tile to rescale because boundary values from adjacent tiles are out of its height range
        float height = 0.0f;
        bool result = false;
        UpdateTestTerrainTile(0, 0, 14.0f);
        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 0, 1, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(16.0f, height, heightScale);

        LegacyTerrain::LegacyTerrainRequestBus::BroadcastResult(result, &LegacyTerrain::LegacyTerrainRequests::GetTerrainHeight, 1, 1, height);
        EXPECT_TRUE(result);
        EXPECT_NEAR(17.0f, height, heightScale);

        // clear up the terrain
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::SetNumTiles, 0, 0);
    }

    TEST_F(PhysXSpecificTest, SetNumTiles_ResizingTerrainRepeatedly_PhysXGeometryIsClearedUp)
    {
        Physics::Ptr<Physics::World> world = nullptr;
        Physics::SystemRequestBus::BroadcastResult(world, &Physics::SystemRequests::GetDefaultWorld);
        auto nativeWorld = static_cast<physx::PxScene*>(world->GetNativePointer());
        EXPECT_EQ(0, nativeWorld->getNbActors(physx::PxActorTypeFlag::eRIGID_STATIC));

        CreateTestTerrain(2);
        EXPECT_EQ(4, nativeWorld->getNbActors(physx::PxActorTypeFlag::eRIGID_STATIC));

        CreateTestTerrain(3);
        EXPECT_EQ(9, nativeWorld->getNbActors(physx::PxActorTypeFlag::eRIGID_STATIC));

        UpdateTestTerrainTile(1, 2, 19.0f);
        EXPECT_EQ(9, nativeWorld->getNbActors(physx::PxActorTypeFlag::eRIGID_STATIC));

        // clear up the terrain
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::SetNumTiles, 0, 0);
        EXPECT_EQ(0, nativeWorld->getNbActors(physx::PxActorTypeFlag::eRIGID_STATIC));
    }

    AZ::Entity* AddUnitTestBox(const AZ::Vector3& position, const char* name = "TestBoxEntity")
    {
        auto entity = aznew AZ::Entity(name);

        AZ::TransformConfig transformConfig;
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
        entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);
        entity->CreateComponent(AZ::Uuid::CreateString("{5EDF4B9E-0D3D-40B8-8C91-5142BCFC30A6}")); // BoxShapeComponent
        entity->CreateComponent(AZ::Uuid::CreateString("{C53C7C88-7131-4EEB-A602-A7DF5B47898E}")); // PhysXColliderComponent
        PhysXRigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_motionType = Physics::MotionType::Dynamic;
        entity->CreateComponent<PhysXRigidBodyComponent>(rigidBodyConfig);
        entity->Init();
        entity->Activate();
        return entity;
    }

    TEST_F(PhysXSpecificTest, TerrainCollision_RigidBodiesFallingOnTerrain_CollideWithTerrain)
    {
        // set up a flat terrain with height 0 from x, y co-ordinates 0, 0 to 20, 20
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::SetNumTiles, 1, 4);
        AZ::u16 heightMap[25] = { 0 };
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::UpdateTile,
            0, 0, heightMap, 0.0f, 0.01f, 4, 5);

        // create some box entities inside the edges of the terrain and some beyond the edges
        AZStd::vector<AZ::Entity*> boxesInsideTerrain;
        boxesInsideTerrain.push_back(AddUnitTestBox(AZ::Vector3(1.0f, 1.0f, 2.0f), "Interior box A"));
        boxesInsideTerrain.push_back(AddUnitTestBox(AZ::Vector3(19.0f, 1.0f, 2.0f), "Interior box B"));
        boxesInsideTerrain.push_back(AddUnitTestBox(AZ::Vector3(5.0f, 18.0f, 2.0f), "Interior box C"));
        boxesInsideTerrain.push_back(AddUnitTestBox(AZ::Vector3(13.0f, 2.0f, 2.0f), "Interior box D"));

        AZStd::vector<AZ::Entity*> boxesOutsideTerrain;
        boxesOutsideTerrain.push_back(AddUnitTestBox(AZ::Vector3(-1.0f, -1.0f, 2.0f), "Exterior box A"));
        boxesOutsideTerrain.push_back(AddUnitTestBox(AZ::Vector3(1.0f, 22.0f, 2.0f), "Exterior box B"));
        boxesOutsideTerrain.push_back(AddUnitTestBox(AZ::Vector3(-2.0f, 14.0f, 2.0f), "Exterior box C"));
        boxesOutsideTerrain.push_back(AddUnitTestBox(AZ::Vector3(22.0f, 8.0f, 2.0f), "Exterior box D"));

        // run the simulation for a while
        Physics::Ptr<Physics::World> world = nullptr;
        Physics::SystemRequestBus::BroadcastResult(world, &Physics::SystemRequests::GetDefaultWorld);
        for (int timeStep = 0; timeStep < 500; timeStep++)
        {
            world->Update(1.0f / 60.0f);
        }

        // check that boxes positioned above the terrain have landed on the terrain and those outside have fallen below 0
        AZ::Vector3 position = AZ::Vector3::CreateZero();

        for (auto box : boxesInsideTerrain)
        {
            AZ::TransformBus::EventResult(position, box->GetId(), &AZ::TransformBus::Events::GetWorldTranslation);
            float z = position.GetZ();
            EXPECT_NEAR(0.5f, z, PhysXSpecificTest::tolerance) << box->GetName().c_str() << " failed to land on terrain";
            delete box;
        }

        for (auto box : boxesOutsideTerrain)
        {
            AZ::TransformBus::EventResult(position, box->GetId(), &AZ::TransformBus::Events::GetWorldTranslation);
            float z = position.GetZ();
            EXPECT_GT(0.0f, z) << box->GetName().c_str() << " did not fall below expected height";
            delete box;
        }

        // clear up the terrain
        LegacyTerrain::LegacyTerrainNotificationBus::Broadcast(&LegacyTerrain::LegacyTerrainNotifications::SetNumTiles, 0, 0);
    }

    TEST_F(PhysXSpecificTest, RigidBody_GetNativeType_ReturnsPhysXRigidBodyType)
    {
        Physics::Ptr<Physics::RigidBodySettings> rigidBodySettings = Physics::RigidBodySettings::Create();
        Physics::Ptr<Physics::RigidBody> rigidBody = nullptr;
        Physics::SystemRequestBus::BroadcastResult(rigidBody, &Physics::SystemRequests::CreateRigidBody, rigidBodySettings);
        EXPECT_EQ(rigidBody->GetNativeType(), AZ::Crc32("PhysXRigidBody"));
    }

    TEST_F(PhysXSpecificTest, RigidBody_GetNativePointer_ReturnsValidPointer)
    {
        Physics::Ptr<Physics::RigidBodySettings> rigidBodySettings = Physics::RigidBodySettings::Create();
        Physics::Ptr<Physics::RigidBody> rigidBody = nullptr;
        Physics::SystemRequestBus::BroadcastResult(rigidBody, &Physics::SystemRequests::CreateRigidBody, rigidBodySettings);
        physx::PxBase* nativePointer = static_cast<physx::PxBase*>(rigidBody->GetNativePointer());
        EXPECT_TRUE(strcmp(nativePointer->getConcreteTypeName(), "PxRigidStatic") == 0);
    }
} // namespace PhysX

