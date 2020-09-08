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
#include "PhysXTestFixtures.h"
#include "PhysXTestUtil.h"

#include <AzTest/AzTest.h>
#include <PhysX/ConfigurationBus.h>
#include <PhysX/MathConversion.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/SystemBus.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <AzFramework/Physics/TerrainBus.h>
#include <AzFramework/Physics/World.h>
#include <RigidBodyStatic.h>
#include <SphereColliderComponent.h>
#include <TerrainComponent.h>
#include <Utils.h>
#include <Physics/PhysicsTests.h>
#include <Physics/PhysicsTests.inl>
#include <PhysX/SystemComponentBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <Tests/PhysXTestCommon.h>
#include <Utils.h>

namespace PhysX
{
    class PhysXSpecificTest
        : public PhysXDefaultWorldTest
        , public UnitTest::TraceBusRedirector
    {
    protected:


        float tolerance = 1e-3f;
    };


    namespace PhysXTests
    {
        typedef EntityPtr(* EntityFactoryFunc)(const AZ::Vector3&, const char*);
    }

    class PhysXEntityFactoryParamTest
        : public PhysXSpecificTest
        , public ::testing::WithParamInterface<PhysXTests::EntityFactoryFunc>
    {
    };

    void SetCollisionLayerName(AZ::u8 index, const AZStd::string& name)
    {
        AZ::Interface<Physics::CollisionRequests>::Get()->SetCollisionLayerName(index, name);
    }

    void CreateCollisionGroup(const Physics::CollisionGroup& group, const AZStd::string& name)
    {
        AZ::Interface<Physics::CollisionRequests>::Get()->CreateCollisionGroup(name, group);
    }
    
    void SanityCheckValidFrustumParams(const AZStd::vector<AZ::Vector3>& points, float validHeight, float validBottomRadius, float validTopRadius, AZ::u8 validSubdivisions)
    {
        double rad = 0;
        const double step = AZ::Constants::TwoPi / aznumeric_cast<double>(validSubdivisions);
        const float halfHeight = validHeight * 0.5f;

        for (auto i = 0; i < points.size() / 2; i++)
        {
            // Canonical way to plot points on the circumference a cicle
            // If any attempt to refactor/optimize the implemented algorithm fails, this test will fail
            const float x = aznumeric_cast<float>(std::cos(rad));
            const float y = aznumeric_cast<float>(std::sin(rad));

            // Top face point is offset half the height along the positive z axis
            {
                const AZ::Vector3& p = points[i * 2];

                EXPECT_FLOAT_EQ(p.GetX(), x * validTopRadius);
                EXPECT_FLOAT_EQ(p.GetY(), y * validTopRadius);
                EXPECT_FLOAT_EQ(p.GetZ(), +halfHeight);
            }

            // Bottom face point is offset half the height along the negative z axis
            {
                const AZ::Vector3& p = points[i * 2 + 1];

                EXPECT_FLOAT_EQ(p.GetX(), x * validBottomRadius);
                EXPECT_FLOAT_EQ(p.GetY(), y * validBottomRadius);
                EXPECT_FLOAT_EQ(p.GetZ(), -halfHeight);
            }

            rad += step;
        }
    }


    TEST_F(PhysXSpecificTest, VectorConversion_ConvertToPxVec3_ConvertedVectorsCorrect)
    {
        AZ::Vector3 lyA(3.0f, -4.0f, 12.0f);
        AZ::Vector3 lyB(-8.0f, 1.0f, -4.0f);

        physx::PxVec3 pxA = PxMathConvert(lyA);
        physx::PxVec3 pxB = PxMathConvert(lyB);

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

        AZ::Vector3 lyA = PxMathConvert(pxA);
        AZ::Vector3 lyB = PxMathConvert(pxB);

        EXPECT_NEAR(lyA.GetLengthSq(), 169.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyB.GetLengthSq(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Dot(lyB), -76.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetX(), 4.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetY(), -84.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyA.Cross(lyB).GetZ(), -29.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, ExtendedVectorConversion_ConvertToPxExtendedVec3_ConvertedVectorsCorrect)
    {
        AZ::Vector3 lyA(3.0f, -4.0f, 12.0f);
        AZ::Vector3 lyB(-8.0f, 1.0f, -4.0f);

        physx::PxExtendedVec3 pxA = PxMathConvertExtended(lyA);
        physx::PxExtendedVec3 pxB = PxMathConvertExtended(lyB);

        EXPECT_NEAR(pxA.magnitudeSquared(), 169.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxB.magnitudeSquared(), 81.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).x, 4.0, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).y, -84.0, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxA.cross(pxB).z, -29.0, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, ExtendedVectorConversion_ConvertToLyVec3_ConvertedVectorsCorrect)
    {
        physx::PxExtendedVec3 pxA(3.0, -4.0, 12.0);
        physx::PxExtendedVec3 pxB(-8.0, 1.0, -4.0);

        AZ::Vector3 lyA = PxMathConvertExtended(pxA);
        AZ::Vector3 lyB = PxMathConvertExtended(pxB);

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
        physx::PxQuat pxQ = PxMathConvert(lyQ);
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
        physx::PxQuat pxQ = physx::PxQuat(9.0f, -8.0f, -4.0f, 8.0f) * (1.0f / 15.0f);
        AZ::Quaternion lyQ = PxMathConvert(pxQ);
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

        physx::PxTransform pxTm = PxMathConvert(lyTm);
        physx::PxVec3 pxV = pxTm.transform(physx::PxVec3(8.0f, 1.0f, 4.0f));

        EXPECT_NEAR(pxV.x, 90.0f / 117.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.y, 9.0f / 117.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(pxV.z, 36.0f / 117.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, TransformConversion_ConvertToLyTransform_ConvertedTransformsCorrect)
    {
        physx::PxTransform pxTm(physx::PxVec3(2.0f, 10.0f, 9.0f), physx::PxQuat(6.0f, -8.0f, -5.0f, 10.0f) * (1.0f / 15.0f));
        AZ::Transform lyTm = PxMathConvert(pxTm);
        AZ::Vector3 lyV = lyTm * AZ::Vector3(4.0f, -12.0f, 3.0f);

        EXPECT_NEAR(lyV.GetX(), -14.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetY(), 22.0f / 45.0f, PhysXSpecificTest::tolerance);
        EXPECT_NEAR(lyV.GetZ(), 4.0f / 9.0f, PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, RigidBody_GetNativeShape_ReturnsCorrectShape)
    {
        Physics::RigidBodyConfiguration rigidBodyConfiguration;
        AZ::Vector3 halfExtents(1.0f, 2.0f, 3.0f);

        AZStd::unique_ptr<Physics::RigidBody> rigidBody = AZ::Interface<Physics::System>::Get()->CreateRigidBody(rigidBodyConfiguration);
        ASSERT_TRUE(rigidBody != nullptr);

        Physics::BoxShapeConfiguration shapeConfig(halfExtents * 2.0f);
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi);
        AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        rigidBody->AddShape(shape);

        auto nativeShape = rigidBody->GetShape(0);
        ASSERT_TRUE(nativeShape != nullptr);

        auto pxShape = AZStd::rtti_pointer_cast<PhysX::Shape>(shape);
        ASSERT_TRUE(pxShape->GetPxShape()->getGeometryType() == physx::PxGeometryType::eBOX);

        physx::PxBoxGeometry boxGeometry;
        pxShape->GetPxShape()->getBoxGeometry(boxGeometry);

        EXPECT_NEAR(boxGeometry.halfExtents.x, halfExtents.GetX(), PhysXSpecificTest::tolerance);
        EXPECT_NEAR(boxGeometry.halfExtents.y, halfExtents.GetY(), PhysXSpecificTest::tolerance);
        EXPECT_NEAR(boxGeometry.halfExtents.z, halfExtents.GetZ(), PhysXSpecificTest::tolerance);
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_NoTerrain_ReturnsFalse)
    {
        float height = 0.0f;
        Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, 0.0f, 0.0f);
        EXPECT_EQ(height, 0.0f);
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_TestTerrain_CorrectHeightValues)
    {
        auto terrain = TestUtils::CreateSlopedTestTerrain();

        // make some height queries
        float heightScale = 0.01f;
        float height = 0.0f;
        for (float x = 0; x < 3; x++)
        {
            for (float y = 0; y < 3; y++)
            {
                Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, x, y);
                float expected = x + y;
                EXPECT_NEAR(expected, height, heightScale);
            }
        }
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_RequestNonGridPoint_InterpolatesCorrectly)
    {
        auto terrain = TestUtils::CreateSlopedTestTerrain();

        float height = 0.0f;
        Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, 0.0f, 1.5f);
        EXPECT_NEAR(height, 1.5f, 0.01f);

        Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, 1.5f, 1.5f);
        EXPECT_NEAR(height, 3.0f, 0.01f);
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_ScaledTerrain_ScaledHeightValues)
    {
        float scaleX = 2.0f;
        float scaleY = 1.0f;
        float scaleZ = 10.0f;
        auto terrain = TestUtils::CreateSlopedTestTerrain(scaleX, scaleY, scaleZ);

        // make some height queries
        float error = 0.01f;
        float height = 0.0f;
        for (float x = 0; x < 3.0f; x++)
        {
            for (float y = 0; y < 3.0f; y++)
            {
                Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, x, y);
                float expected = (x / scaleX + y / scaleY) * scaleZ;
                EXPECT_NEAR(expected, height, error);
            }
        }
    }

    TEST_F(PhysXSpecificTest, GetTerrainHeight_RequestOutsideBounds_ClampedCorrectly)
    {
        float heightScale = 0.01f;
        auto terrain = TestUtils::CreateSlopedTestTerrain();

        // make some queries outside the terrain to test co-ordinate clamping
        // All queries will return 0.0f if outside heightmap
        float height = 0.0f;
        Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, 5.0f, 2.0f);
        EXPECT_NEAR(4.0f, height, heightScale);

        Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, 7.0f, 1.0f);
        EXPECT_NEAR(3.0f, height, heightScale);

        Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, 6.0f, 7.0f);
        EXPECT_NEAR(4.0f, height, heightScale);

        Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, -1.0f, -2.0f);
        EXPECT_NEAR(0.0f, height, heightScale);

        Physics::TerrainRequestBus::BroadcastResult(height, &Physics::TerrainRequests::GetHeight, -1.0f, 1.0f);
        EXPECT_NEAR(1.0f, height, heightScale);
    }

    TEST_P(PhysXEntityFactoryParamTest, TerrainCollision_RigidBodiesFallingOnTerrain_CollideWithTerrain)
    {
        // set up a flat terrain with height 0 from x, y co-ordinates 0, 0 to 20, 20
        auto terrain = TestUtils::CreateFlatTestTerrain(20, 20);

        auto testEntityFactory = GetParam();

        // create some box entities inside the edges of the terrain and some beyond the edges
        AZStd::vector<EntityPtr> boxesInsideTerrain;
        boxesInsideTerrain.push_back(testEntityFactory(AZ::Vector3(1.0f, 1.0f, 2.0f), "Interior box A"));
        boxesInsideTerrain.push_back(testEntityFactory(AZ::Vector3(19.0f, 1.0f, 2.0f), "Interior box B"));
        boxesInsideTerrain.push_back(testEntityFactory(AZ::Vector3(5.0f, 18.0f, 2.0f), "Interior box C"));
        boxesInsideTerrain.push_back(testEntityFactory(AZ::Vector3(13.0f, 2.0f, 2.0f), "Interior box D"));

        AZStd::vector<EntityPtr> boxesOutsideTerrain;
        boxesOutsideTerrain.push_back(testEntityFactory(AZ::Vector3(-1.0f, -1.0f, 2.0f), "Exterior box A"));
        boxesOutsideTerrain.push_back(testEntityFactory(AZ::Vector3(1.0f, 22.0f, 2.0f), "Exterior box B"));
        boxesOutsideTerrain.push_back(testEntityFactory(AZ::Vector3(-2.0f, 14.0f, 2.0f), "Exterior box C"));
        boxesOutsideTerrain.push_back(testEntityFactory(AZ::Vector3(22.0f, 8.0f, 2.0f), "Exterior box D"));

        // run the simulation for a while
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
        for (int timeStep = 0; timeStep < 150; timeStep++)
        {
            world->Update(1.0f / 60.0f);
        }

        // check that boxes positioned above the terrain have landed on the terrain and those outside have fallen below 0
        AZ::Vector3 position = AZ::Vector3::CreateZero();

        for (const auto& box : boxesInsideTerrain)
        {
            AZ::TransformBus::EventResult(position, box->GetId(), &AZ::TransformBus::Events::GetWorldTranslation);
            float z = position.GetZ();
            EXPECT_NEAR(0.5f, z, PhysXSpecificTest::tolerance) << box->GetName().c_str() << " failed to land on terrain";
        }

        for (const auto& box : boxesOutsideTerrain)
        {
            AZ::TransformBus::EventResult(position, box->GetId(), &AZ::TransformBus::Events::GetWorldTranslation);
            float z = position.GetZ();
            EXPECT_GT(0.0f, z) << box->GetName().c_str() << " did not fall below expected height";
        }
    }

    auto entityFactories = { TestUtils::AddUnitTestObject<BoxColliderComponent>, TestUtils::AddUnitTestBoxComponentsMix };
    INSTANTIATE_TEST_CASE_P(DifferentBoxes, PhysXEntityFactoryParamTest, ::testing::ValuesIn(entityFactories));

    TEST_F(PhysXSpecificTest, RigidBody_GetNativeType_ReturnsPhysXRigidBodyType)
    {
        Physics::RigidBodyConfiguration rigidBodyConfiguration;
        AZStd::unique_ptr<Physics::RigidBody> rigidBody = AZ::Interface<Physics::System>::Get()->CreateRigidBody(rigidBodyConfiguration);
        EXPECT_EQ(rigidBody->GetNativeType(), AZ::Crc32("PhysXRigidBody"));
    }

    TEST_F(PhysXSpecificTest, RigidBody_GetNativePointer_ReturnsValidPointer)
    {
        Physics::RigidBodyConfiguration rigidBodyConfiguration;
        AZStd::unique_ptr<Physics::RigidBody> rigidBody = AZ::Interface<Physics::System>::Get()->CreateRigidBody(rigidBodyConfiguration);
        physx::PxBase* nativePointer = static_cast<physx::PxBase*>(rigidBody->GetNativePointer());
        EXPECT_TRUE(strcmp(nativePointer->getConcreteTypeName(), "PxRigidDynamic") == 0);
    }

    TEST_F(PhysXSpecificTest, TriggerArea_RigidBodyEnteringAndLeavingTrigger_EnterLeaveCallbackCalled)
    {
        // set up a trigger box
        auto triggerBox = TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 12.0f));
        auto triggerBody = triggerBox->FindComponent<BoxColliderComponent>()->GetStaticRigidBody();
        auto triggerShape = triggerBody->GetShape(0);

        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(triggerBox->GetId());

        // Create a test box above the trigger so when it falls down it'd enter and leave the trigger box
        auto testBox = TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 16.0f), "TestBox");
        auto testBoxBody = testBox->FindComponent<RigidBodyComponent>()->GetRigidBody();
        auto testBoxShape = testBoxBody->GetShape(0);

        // run the simulation for a while
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

        for (int timeStep = 0; timeStep < 500; timeStep++)
        {
            world->Update(1.0f / 60.0f);
        }

        const auto& enteredEvents = testTriggerAreaNotificationListener.GetEnteredEvents();
        const auto& exitedEvents = testTriggerAreaNotificationListener.GetExitedEvents();

        ASSERT_EQ(enteredEvents.size(), 1);
        ASSERT_EQ(exitedEvents.size(), 1);

        EXPECT_EQ(enteredEvents[0].m_triggerBody, triggerBody);
        EXPECT_EQ(enteredEvents[0].m_triggerShape, triggerShape.get());
        EXPECT_EQ(enteredEvents[0].m_otherBody, testBoxBody);
        EXPECT_EQ(enteredEvents[0].m_otherShape, testBoxShape.get());

        EXPECT_EQ(exitedEvents[0].m_triggerBody, triggerBody);
        EXPECT_EQ(exitedEvents[0].m_triggerShape, triggerShape.get());
        EXPECT_EQ(exitedEvents[0].m_otherBody, testBoxBody);
        EXPECT_EQ(exitedEvents[0].m_otherShape, testBoxShape.get());
    }

    TEST_F(PhysXSpecificTest, TriggerArea_RigidBodiesEnteringAndLeavingTriggers_EnterLeaveCallbackCalled)
    {
        // set up triggers
        AZStd::vector<EntityPtr> triggers =
        {
            TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 12.0f)),
            TestUtils::CreateTriggerAtPosition<SphereColliderComponent>(AZ::Vector3(0.0f, 0.0f, 8.0f))
        };

        // set up dynamic objs
        AZStd::vector<EntityPtr> testBoxes =
        {
            TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 16.0f), "TestBox"),
            TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 18.0f), "TestBox2")
        };

        // set up listeners on triggers
        TestTriggerAreaNotificationListener testTriggerBoxNotificationListener(triggers[0]->GetId());
        TestTriggerAreaNotificationListener testTriggerSphereNotificationListener(triggers[1]->GetId());

        // run the simulation for a while
        UpdateWorld(500);

        for (const auto triggerListener : {&testTriggerBoxNotificationListener, &testTriggerSphereNotificationListener})
        {
            const auto& enteredEvents = triggerListener->GetEnteredEvents();
            ASSERT_EQ(2, enteredEvents.size());
            EXPECT_EQ(enteredEvents[0].m_otherBody, testBoxes[0]->FindComponent<RigidBodyComponent>()->GetRigidBody());
            EXPECT_EQ(enteredEvents[0].m_otherShape, testBoxes[0]->FindComponent<RigidBodyComponent>()->GetRigidBody()->GetShape(0).get());
            EXPECT_EQ(enteredEvents[1].m_otherBody, testBoxes[1]->FindComponent<RigidBodyComponent>()->GetRigidBody());
            EXPECT_EQ(enteredEvents[1].m_otherShape, testBoxes[1]->FindComponent<RigidBodyComponent>()->GetRigidBody()->GetShape(0).get());

            const auto& exitedEvents = triggerListener->GetExitedEvents();
            ASSERT_EQ(2, enteredEvents.size());
            EXPECT_EQ(exitedEvents[0].m_otherBody, testBoxes[0]->FindComponent<RigidBodyComponent>()->GetRigidBody());
            EXPECT_EQ(exitedEvents[0].m_otherShape, testBoxes[0]->FindComponent<RigidBodyComponent>()->GetRigidBody()->GetShape(0).get());
            EXPECT_EQ(exitedEvents[1].m_otherBody, testBoxes[1]->FindComponent<RigidBodyComponent>()->GetRigidBody());
            EXPECT_EQ(exitedEvents[1].m_otherShape, testBoxes[1]->FindComponent<RigidBodyComponent>()->GetRigidBody()->GetShape(0).get());
        }
    }

    TEST_F(PhysXSpecificTest, RigidBody_CollisionCallback_SimpleCallbackOfTwoSpheres)
    {
        auto obj01 = TestUtils::AddUnitTestObject<SphereColliderComponent>(AZ::Vector3(0.0f, 0.0f, 10.0f), "TestSphere01");
        auto obj02 = TestUtils::AddUnitTestObject<SphereColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f), "TestSphere01");

        auto body01 = obj01->FindComponent<RigidBodyComponent>()->GetRigidBody();
        auto body02 = obj02->FindComponent<RigidBodyComponent>()->GetRigidBody();

        auto shape01 = body01->GetShape(0).get();
        auto shape02 = body02->GetShape(0).get();

        CollisionCallbacksListener listener01(obj01->GetId());
        CollisionCallbacksListener listener02(obj02->GetId());

        Physics::RigidBodyRequestBus::Event(obj02->GetId(), &Physics::RigidBodyRequestBus::Events::ApplyLinearImpulse, AZ::Vector3(0.0f, 0.0f, 50.0f));

        // run the simulation for a while
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

        for (int timeStep = 0; timeStep < 500; timeStep++)
        {
            world->Update(1.0f / 60.0f);
        }

        // We expect to have two (CollisionBegin and CollisionEnd) events for both objects
        ASSERT_EQ(listener01.m_beginCollisions.size(), 1);
        ASSERT_EQ(listener01.m_endCollisions.size(), 1);
        ASSERT_EQ(listener02.m_beginCollisions.size(), 1);
        ASSERT_EQ(listener02.m_endCollisions.size(), 1);

        // First collision recorded is CollisionBegin event
        auto collisionBegin01 = listener01.m_beginCollisions[0];
        EXPECT_EQ(collisionBegin01.m_body2->GetEntityId(), obj02->GetId());
        EXPECT_EQ(collisionBegin01.m_body2, body02);
        EXPECT_EQ(collisionBegin01.m_shape2, shape02);

        // Checkes one of the collision point details
        ASSERT_EQ(collisionBegin01.m_contacts.size(), 1);
        EXPECT_NEAR(collisionBegin01.m_contacts[0].m_impulse.GetZ(), -37.12f, 0.01f);
        float dotNormal = collisionBegin01.m_contacts[0].m_normal.Dot(AZ::Vector3(0.0f, 0.0f, -1.0f));
        EXPECT_NEAR(dotNormal, 1.0f, 0.01f);
        EXPECT_NEAR(collisionBegin01.m_contacts[0].m_separation, -0.12, 0.01f);

        // Second collision recorded is CollisionExit event
        auto collisionEnd01 = listener01.m_endCollisions[0];
        EXPECT_EQ(collisionEnd01.m_body2->GetEntityId(), obj02->GetId());
        EXPECT_EQ(collisionEnd01.m_body2, body02);
        EXPECT_EQ(collisionEnd01.m_shape2, shape02);

        // Some checks for the second sphere
        auto collisionBegin02 = listener02.m_beginCollisions[0];
        EXPECT_EQ(collisionBegin02.m_body2->GetEntityId(), obj01->GetId());
        EXPECT_EQ(collisionBegin02.m_body2, body01);
        EXPECT_EQ(collisionBegin02.m_shape2, shape01);

        auto collisionEnd02 = listener02.m_endCollisions[0];
        EXPECT_EQ(collisionEnd02.m_body2->GetEntityId(), obj01->GetId());
        EXPECT_EQ(collisionEnd02.m_body2, body01);
        EXPECT_EQ(collisionEnd02.m_shape2, shape01);
    }

    TEST_F(PhysXSpecificTest, RigidBody_CollisionCallback_SimpleCallbackSphereFallingOnStaticBox)
    {
        auto obj01 = TestUtils::AddUnitTestObject<SphereColliderComponent>(AZ::Vector3(0.0f, 0.0f, 10.0f), "TestSphere01");
        auto obj02 = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f), "TestBox01");

        auto body01 = obj01->FindComponent<RigidBodyComponent>()->GetRigidBody();
        auto body02 = obj02->FindComponent<BoxColliderComponent>()->GetStaticRigidBody();

        auto shape01 = body01->GetShape(0).get();
        auto shape02 = body02->GetShape(0).get();

        CollisionCallbacksListener listener01(obj01->GetId());
        CollisionCallbacksListener listener02(obj02->GetId());

        // run the simulation for a while
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

        for (int timeStep = 0; timeStep < 500; timeStep++)
        {
            world->Update(1.0f / 60.0f);
        }

        // Ball should bounce at least 2 times, generating CollisionBegin and CollisionEnd events
        ASSERT_GE(listener01.m_beginCollisions.size(), 2);
        ASSERT_GE(listener01.m_endCollisions.size(), 2);
        ASSERT_GE(listener02.m_beginCollisions.size(), 2);
        ASSERT_GE(listener02.m_endCollisions.size(), 2);

        EXPECT_EQ(listener01.m_beginCollisions[0].m_body2->GetEntityId(), obj02->GetId());
        EXPECT_EQ(listener01.m_beginCollisions[0].m_body2, body02);
        EXPECT_EQ(listener01.m_beginCollisions[0].m_shape2, shape02);

        EXPECT_EQ(listener02.m_beginCollisions[0].m_body2->GetEntityId(), obj01->GetId());
        EXPECT_EQ(listener02.m_beginCollisions[0].m_body2, body01);
        EXPECT_EQ(listener02.m_beginCollisions[0].m_shape2, shape01);
    }

    TEST_F(PhysXSpecificTest, RigidBody_CollisionCallback_SimpleCallbackSphereFallingOnTerrain)
    {
        // Create terrain
        auto terrain = TestUtils::CreateSlopedTestTerrain();

        // Create sphere
        auto sphere = TestUtils::AddUnitTestObject<SphereColliderComponent>(AZ::Vector3(0.0f, 0.0f, 10.0f), "TestSphere77");

        Physics::RigidBodyStatic* terrainBody;
        Physics::TerrainRequestBus::BroadcastResult(terrainBody, &Physics::TerrainRequests::GetTerrainTile, 0.0f, 0.0f);

        auto terrainShape = terrainBody->GetShape(0);

        CollisionCallbacksListener listener01(sphere->GetId());

        UpdateWorld(360);

        ASSERT_GE(listener01.m_beginCollisions.size(), 1);
        ASSERT_GE(listener01.m_endCollisions.size(), 1);

        EXPECT_EQ(listener01.m_beginCollisions[0].m_body2, terrainBody);
        EXPECT_EQ(listener01.m_beginCollisions[0].m_shape2, terrainShape.get());

        EXPECT_EQ(listener01.m_endCollisions[0].m_body2, terrainBody);
        EXPECT_EQ(listener01.m_endCollisions[0].m_shape2, terrainShape.get());
    }

    TEST_F(PhysXSpecificTest, Terrain_Raycast_ReturnsHit)
    {
        // Create terrain
        auto terrain = TestUtils::CreateFlatTestTerrain();
        Physics::RigidBodyStatic* terrainBody;
        Physics::TerrainRequestBus::BroadcastResult(terrainBody, &Physics::TerrainRequests::GetTerrainTile, 0.0f, 0.0f);

        Physics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.5f, 0.5f, 1.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, -1.0f);
        request.m_distance = 2;

        Physics::RayCastHit hit;
        Physics::WorldRequestBus::BroadcastResult(hit, &Physics::WorldRequests::RayCast, request);

        ASSERT_EQ(hit, true);
        EXPECT_EQ(hit.m_body, terrainBody);
        EXPECT_EQ(hit.m_shape, terrainBody->GetShape(0).get());
    }

    TEST_F(PhysXSpecificTest, Terrain_RaycastCustomFilter_ReturnsHit)
    {
        // Create terrain
        auto terrain = TestUtils::CreateFlatTestTerrain();
        Physics::RigidBodyStatic* terrainBody;
        Physics::TerrainRequestBus::BroadcastResult(terrainBody, &Physics::TerrainRequests::GetTerrainTile, 0.0f, 0.0f);
        Physics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.5f, 0.5f, 1.0f);
        request.m_direction = AZ::Vector3(0.0f, 0.0f, -1.0f);
        request.m_distance = 2.0f;
        request.m_filterCallback = [](const Physics::WorldBody* body, const Physics::Shape* shape)
        {
            return Physics::QueryHitType::Block;
        };

        Physics::RayCastHit hit;
        Physics::WorldRequestBus::BroadcastResult(hit, &Physics::WorldRequests::RayCast, request);

        ASSERT_EQ(hit, true);
        EXPECT_EQ(hit.m_body, terrainBody);
        EXPECT_EQ(hit.m_shape, terrainBody->GetShape(0).get());
    }

    TEST_F(PhysXSpecificTest, CollisionFiltering_CollisionLayers_CombineLayersIntoGroup)
    {
        // Start with empty group
        Physics::CollisionGroup group = Physics::CollisionGroup::None;
        Physics::CollisionLayer layer1(1);
        Physics::CollisionLayer layer2(2);

        // Check nothing is set
        EXPECT_FALSE(group.IsSet(layer1));
        EXPECT_FALSE(group.IsSet(layer2));

        // Combine layers into group
        group = layer1 | layer2;

        // Check they are set
        EXPECT_TRUE(group.IsSet(layer1));
        EXPECT_TRUE(group.IsSet(layer2));
    }

    TEST_F(PhysXSpecificTest, CollisionFiltering_CollisionLayers_ConstructLayerByName)
    {
        // Set layer names
        SetCollisionLayerName(1, "Layer1");
        SetCollisionLayerName(2, "Layer2");
        SetCollisionLayerName(3, "Layer3");

        // Lookup layers by name
        Physics::CollisionLayer layer1("Layer1");
        Physics::CollisionLayer layer2("Layer2");
        Physics::CollisionLayer layer3("Layer3");

        // Check they match what was set before
        EXPECT_EQ(1, layer1.GetIndex());
        EXPECT_EQ(2, layer2.GetIndex());
        EXPECT_EQ(3, layer3.GetIndex());
    }

    TEST_F(PhysXSpecificTest, CollisionFiltering_CollisionGroups_AppendLayerToGroup)
    {
        // Start with empty group
        Physics::CollisionGroup group = Physics::CollisionGroup::None;
        Physics::CollisionLayer layer1(1);

        EXPECT_FALSE(group.IsSet(layer1));

        // Append layer to group
        group = group | layer1;

        // Check its set
        EXPECT_TRUE(group.IsSet(layer1));
    }

    TEST_F(PhysXSpecificTest, CollisionFiltering_CollisionGroups_ConstructGroupByName)
    {
        // Create a collision group preset from layers
        CreateCollisionGroup(Physics::CollisionLayer(5) | Physics::CollisionLayer(13), "TestGroup");

        // Lookup the group by name
        Physics::CollisionGroup group("TestGroup");

        // Check it looks correct
        EXPECT_TRUE(group.IsSet(Physics::CollisionLayer(5)));
        EXPECT_TRUE(group.IsSet(Physics::CollisionLayer(13)));
    }

    TEST_F(PhysXSpecificTest, RigidBody_CenterOfMassOffsetComputed)
    {
        Physics::RigidBodyConfiguration rigidBodyConfiguration;
        AZ::Vector3 halfExtents(1.0f, 2.0f, 3.0f);
        rigidBodyConfiguration.m_computeCenterOfMass = true;
        rigidBodyConfiguration.m_computeInertiaTensor = true;

        AZStd::unique_ptr<Physics::RigidBody> rigidBody = AZ::Interface<Physics::System>::Get()->CreateRigidBody(rigidBodyConfiguration);
        ASSERT_TRUE(rigidBody != nullptr);

        Physics::BoxShapeConfiguration shapeConfig(halfExtents * 2.0f);
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi);
        AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        rigidBody->AddShape(shape);

        auto com = rigidBody->GetCenterOfMassLocal();
        EXPECT_TRUE(com.IsClose(AZ::Vector3::CreateZero(), PhysXSpecificTest::tolerance));
    }

    TEST_F(PhysXSpecificTest, RigidBody_CenterOfMassOffsetSpecified)
    {
        Physics::RigidBodyConfiguration rigidBodyConfiguration;
        AZ::Vector3 halfExtents(1.0f, 2.0f, 3.0f);
        rigidBodyConfiguration.m_computeCenterOfMass = false;
        rigidBodyConfiguration.m_centerOfMassOffset = AZ::Vector3::CreateOne();
        rigidBodyConfiguration.m_computeInertiaTensor = true;

        AZStd::unique_ptr<Physics::RigidBody> rigidBody = AZ::Interface<Physics::System>::Get()->CreateRigidBody(rigidBodyConfiguration);
        ASSERT_TRUE(rigidBody != nullptr);

        Physics::BoxShapeConfiguration shapeConfig(halfExtents * 2.0f);
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi);
        AZStd::shared_ptr<Physics::Shape> shape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        rigidBody->AddShape(shape);

        auto com = rigidBody->GetCenterOfMassLocal();
        EXPECT_TRUE(com.IsClose(AZ::Vector3::CreateOne(), PhysXSpecificTest::tolerance));
    }

    TEST_F(PhysXSpecificTest, TriggerArea_BodyDestroyedInsideTrigger_OnTriggerExitEventRaised)
    {
        // set up a trigger box
        auto triggerBox = TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto triggerBody = triggerBox->FindComponent<BoxColliderComponent>()->GetStaticRigidBody();

        // Create a test box above the trigger so when it falls down it'd enter and leave the trigger box
        auto testBox = TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 1.5f), "TestBox");
        auto testBoxBody = testBox->FindComponent<RigidBodyComponent>()->GetRigidBody();

        // Listen for trigger events on the box
        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(triggerBox->GetId());

        // run the simulation for a while
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

        const auto& enteredEvents = testTriggerAreaNotificationListener.GetEnteredEvents();
        const auto& exitedEvents = testTriggerAreaNotificationListener.GetExitedEvents();

        for (int timeStep = 0; timeStep < 100; timeStep++)
        {
            world->Update(1.0f / 60.0f);

            // Body entered the trigger area, kill it!!!
            if (enteredEvents.size() > 0 && testBox != nullptr)
            {
                testBox.reset();
            }
        }

        ASSERT_EQ(testBox, nullptr);
        ASSERT_EQ(enteredEvents.size(), 1);
        ASSERT_EQ(exitedEvents.size(), 1);

        EXPECT_EQ(enteredEvents[0].m_triggerBody, triggerBody);
        EXPECT_EQ(enteredEvents[0].m_otherBody, testBoxBody);

        EXPECT_EQ(exitedEvents[0].m_triggerBody, triggerBody);
        EXPECT_EQ(exitedEvents[0].m_otherBody, testBoxBody);
    }

    TEST_F(PhysXSpecificTest, TriggerArea_StaticBodyDestroyedInsideDynamicTrigger_OnTriggerExitEventRaised)
    {
        // Set up a static non trigger box
        auto staticBox = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto staticBody = staticBox->FindComponent<BoxColliderComponent>()->GetStaticRigidBody();

        // Create a test trigger box above the static box so when it falls down it'd enter and leave the trigger box
        auto dynamicTrigger = TestUtils::CreateDynamicTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 5.0f));
        auto dynamicBody = dynamicTrigger->FindComponent<RigidBodyComponent>()->GetRigidBody();

        // Listen for trigger events on the box
        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(dynamicTrigger->GetId());

        // run the simulation for a while
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

        const auto& enteredEvents = testTriggerAreaNotificationListener.GetEnteredEvents();
        const auto& exitedEvents = testTriggerAreaNotificationListener.GetExitedEvents();

        for (int timeStep = 0; timeStep < 100; timeStep++)
        {
            world->Update(1.0f / 60.0f);

            // Body entered the trigger area, kill it!!!
            if (enteredEvents.size() > 0 && staticBox != nullptr)
            {
                staticBox.reset();
            }
        }

        ASSERT_EQ(staticBox, nullptr);
        ASSERT_EQ(enteredEvents.size(), 1);
        ASSERT_EQ(exitedEvents.size(), 1);

        EXPECT_EQ(enteredEvents[0].m_triggerBody, dynamicBody);
        EXPECT_EQ(enteredEvents[0].m_otherBody, staticBody);

        EXPECT_EQ(exitedEvents[0].m_triggerBody, dynamicBody);
        EXPECT_EQ(exitedEvents[0].m_otherBody, staticBody);
    }

    TEST_F(PhysXSpecificTest, TriggerArea_BodyDestroyedOnTriggerEnter_DoesNotCrash)
    {
        // Given a rigid body falling into a trigger.
        auto triggerBox = TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto testBox = TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on trigger enter event.
        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(triggerBox->GetId());
        testTriggerAreaNotificationListener.m_onTriggerEnter = [&](const Physics::TriggerEvent& triggerEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        UpdateWorld(30, 1.0f / 30.0f);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, TriggerArea_BodyDestroyedOnTriggerExit_DoesNotCrash)
    {
        // Given a rigid body falling into a trigger.
        auto triggerBox = TestUtils::CreateTriggerAtPosition<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f));
        auto testBox = TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on trigger enter event.
        TestTriggerAreaNotificationListener testTriggerAreaNotificationListener(triggerBox->GetId());
        testTriggerAreaNotificationListener.m_onTriggerExit = [&](const Physics::TriggerEvent& triggerEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        UpdateWorld(30, 1.0f / 30.0f);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, CollisionEvents_BodyDestroyedOnCollisionBegin_DoesNotCrash)
    {
        // Given a rigid body falling onto a static box.
        auto staticBox = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f), "StaticTestBox");
        auto testBox = TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on collision begin event.
        CollisionCallbacksListener collisionListener(testBox->GetId());
        collisionListener.m_onCollisionBegin = [&](const Physics::CollisionEvent& collisionEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        UpdateWorld(30, 1.0f / 30.0f);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, CollisionEvents_BodyDestroyedOnCollisionPersist_DoesNotCrash)
    {
        // Given a rigid body falling onto a static box.
        auto staticBox = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f), "StaticTestBox");
        auto testBox = TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on collision begin event.
        CollisionCallbacksListener collisionListener(testBox->GetId());
        collisionListener.m_onCollisionPersist = [&](const Physics::CollisionEvent& collisionEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        UpdateWorld(30, 1.0f / 30.0f);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, CollisionEvents_BodyDestroyedOnCollisionEnd_DoesNotCrash)
    {
        // Given a rigid body falling onto a static box.
        auto staticBox = TestUtils::AddStaticUnitTestObject<BoxColliderComponent>(AZ::Vector3(0.0f, 0.0f, 0.0f), "StaticTestBox");
        auto testBox = TestUtils::AddUnitTestObject(AZ::Vector3(0.0f, 0.0f, 1.2f), "TestBox");

        // When the rigid body is deleted inside on collision begin event.
        CollisionCallbacksListener collisionListener(testBox->GetId());
        collisionListener.m_onCollisionEnd = [&](const Physics::CollisionEvent& collisionEvent)
        {
            testBox.reset();
        };

        // Update the world. This should not crash.
        UpdateWorld(30, 1.0f / 30.0f);

        /// Then the program does not crash (If you made it this far the test passed).
        ASSERT_TRUE(true);
    }

    TEST_F(PhysXSpecificTest, RigidBody_ConvexRigidBodyCreatedFromCookedMesh_CachedMeshObjectCreated)
    {
        // Create rigid body
        Physics::RigidBodyConfiguration rigidBodyConfiguration;
        AZStd::unique_ptr<Physics::RigidBody> rigidBody = AZ::Interface<Physics::System>::Get()->CreateRigidBody(rigidBodyConfiguration);
        ASSERT_TRUE(rigidBody != nullptr);

        // Generate input data
        const PointList testPoints = TestUtils::GeneratePyramidPoints(1.0f);
        AZStd::vector<AZ::u8> cookedData;
        bool cookingResult = false;
        PhysX::SystemRequestsBus::BroadcastResult(cookingResult, &PhysX::SystemRequests::CookConvexMeshToMemory,
            testPoints.data(), static_cast<AZ::u32>(testPoints.size()), cookedData);
        EXPECT_TRUE(cookingResult);

        // Setup shape & collider configurations
        Physics::CookedMeshShapeConfiguration shapeConfig;
        shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(), 
            Physics::CookedMeshShapeConfiguration::MeshType::Convex);

        Physics::ColliderConfiguration colliderConfig;

        // Create the first shape
        AZStd::shared_ptr<Physics::Shape> firstShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        ASSERT_TRUE(firstShape != nullptr);

        rigidBody->AddShape(firstShape);

        // Validate the cached mesh is there
        EXPECT_NE(shapeConfig.GetCachedNativeMesh(), nullptr);

        // Make some changes in the configuration for the second shape
        colliderConfig.m_position.SetX(1.0f);
        shapeConfig.m_scale = AZ::Vector3(2.0f, 2.0f, 2.0f);

        // Create the second shape
        AZStd::shared_ptr<Physics::Shape> secondShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        ASSERT_TRUE(secondShape != nullptr);
        
        rigidBody->AddShape(secondShape);

        // Add the rigid body to the physics world
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
        world->AddBody(*rigidBody);

        AZ::Vector3 initialPosition = rigidBody->GetPosition();

        // Tick the world
        for (int timeStep = 0; timeStep < 20; timeStep++)
        {
            world->Update(1.0f / 60.0f);
        }

        // Verify the actor has moved
        EXPECT_NE(rigidBody->GetPosition(), initialPosition);

        // Clean up
        world->RemoveBody(*rigidBody);
        rigidBody = nullptr;
    }

    TEST_F(PhysXSpecificTest, RigidBody_TriangleMeshRigidBodyCreatedFromCookedMesh_CachedMeshObjectCreated)
    {
        // Create static rigid body
        Physics::RigidBodyConfiguration rigidBodyConfiguration;
        AZStd::unique_ptr<Physics::RigidBodyStatic> rigidBody = AZ::Interface<Physics::System>::Get()->CreateStaticRigidBody(rigidBodyConfiguration);

        // Generate input data
        VertexIndexData cubeMeshData = TestUtils::GenerateCubeMeshData(3.0f);
        AZStd::vector<AZ::u8> cookedData;
        bool cookingResult = false;
        PhysX::SystemRequestsBus::BroadcastResult(cookingResult, &PhysX::SystemRequests::CookTriangleMeshToMemory,
            cubeMeshData.first.data(), static_cast<AZ::u32>(cubeMeshData.first.size()),
            cubeMeshData.second.data(), static_cast<AZ::u32>(cubeMeshData.second.size()),
            cookedData);
        EXPECT_TRUE(cookingResult);

        // Setup shape & collider configurations
        Physics::CookedMeshShapeConfiguration shapeConfig;
        shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(),
            Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);

        Physics::ColliderConfiguration colliderConfig;

        // Create the first shape
        AZStd::shared_ptr<Physics::Shape> firstShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        AZ_Assert(firstShape != nullptr, "Failed to create a shape from cooked data");

        rigidBody->AddShape(firstShape);

        // Validate the cached mesh is there
        EXPECT_NE(shapeConfig.GetCachedNativeMesh(), nullptr);

        // Make some changes in the configuration for the second shape
        colliderConfig.m_position.SetX(4.0f);
        shapeConfig.m_scale = AZ::Vector3(2.0f, 2.0f, 2.0f);

        // Create the second shape
        AZStd::shared_ptr<Physics::Shape> secondShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
        AZ_Assert(secondShape != nullptr, "Failed to create a shape from cooked data");
        
        rigidBody->AddShape(secondShape);

        // Add the rigid body to the physics world
        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
        world->AddBody(*rigidBody);

        // Drop a sphere
        auto sphereActor = TestUtils::AddUnitTestObject<SphereColliderComponent>(AZ::Vector3(0.0f, 0.0f, 8.0f), "TestSphere01");
        Physics::RigidBody* sphereRigidBody = sphereActor->FindComponent<RigidBodyComponent>()->GetRigidBody();

        // Tick the world
        for (int timeStep = 0; timeStep < 120; timeStep++)
        {
            world->Update(1.0f / 60.0f);
        }

        // Verify the sphere is lying on top of the mesh
        AZ::Vector3 spherePosition = sphereRigidBody->GetPosition();
        EXPECT_TRUE(spherePosition.GetZ().IsClose(6.5f));

        // Clean up
        world->RemoveBody(*rigidBody);
        rigidBody = nullptr;
    }

    TEST_F(PhysXSpecificTest, Shape_ConstructorDestructor_PxShapeReferenceCounterIsCorrect)
    {
        // Create physx::PxShape object
        Physics::CollisionGroup assignedCollisionGroup = Physics::CollisionGroup::None;
        physx::PxShape* shape = Utils::CreatePxShapeFromConfig(
            Physics::ColliderConfiguration(), Physics::BoxShapeConfiguration(), assignedCollisionGroup);

        // physx::PxShape object ref count is expected to be 1 after creation
        EXPECT_EQ(shape->getReferenceCount(), 1);

        // Create PhysX::Shape wrapper object and verify physx::PxShape ref count is increased to 2
        AZStd::unique_ptr<Shape> shapeWrapper = AZStd::make_unique<Shape>(shape);
        EXPECT_EQ(shape->getReferenceCount(), 2);

        // Destroy PhysX::Shape wrapper object and verify physx::PxShape ref count is back to 1
        shapeWrapper = nullptr;
        EXPECT_EQ(shape->getReferenceCount(), 1);

        // Clean up
        shape->release();
    }
    
    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateWithInvalidHeight_ReturnsEmpty)
    {
        // Given a frustum with an invalid height
        float invalidHeight = 0.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions; 

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(invalidHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // The frustum creation will be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateWithInvalidBottomRadius_ReturnsEmpty)
    {
        // Given a frustum with an invalid bottom radius
        float validHeight = 1.0f;
        float invalidBottomRadius = -1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, invalidBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateFromInvalidTopRadius_ReturnsEmpty)
    {
        // Given a frustum with an invalid top radius
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float invalidTopRadius = -1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, invalidTopRadius, validSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }


    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateFromInvalidBottomAndTopRadius_ReturnsEmpty)
    {
        // Given a frustum with an invalid bottom and top radius
        float validHeight = 1.0f;
        float invalidBottomRadius = 0.0f; 
        float invalidTopRadius = 0.0f;    
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, invalidBottomRadius, invalidTopRadius, validSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateFromInvalidMinSubdivisions_ReturnsEmpty)
    {
        // Given a frustum with an invalid minimum subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 invalidMinSubdivisions = Utils::MinFrustumSubdivisions - 1;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, invalidMinSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_CreateFromInvalidMaxSubdivisions_ReturnsEmpty)
    {
        // Given a frustum with an invalid maximum subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 invalidMaxSubdivisions = Utils::MaxFrustumSubdivisions + 1;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, invalidMaxSubdivisions);

        // Expect the frustum creation to be unsuccessful
        EXPECT_FALSE(points.has_value());
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create3SidedFrustum_ReturnsPoints)
    {
        // Given a valid unit frustum with MinSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create3SidedBottomCone_ReturnsPoints)
    {
        // Given a valid unit frustum with MinSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 0.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create3SidedTopCone_ReturnsPoints)
    {
        // Given a valid unit frustum with MinSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 0.0f;
        AZ::u8 validSubdivisions = Utils::MinFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create125SidedFrustum_ReturnsPoints)
    {
        // Given a valid unit frustum with MaxSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MaxFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create125SidedBottomCone_ReturnsPoints)
    {
        // Given a valid unit frustum with MaxSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 0.0f;
        float validTopRadius = 1.0f;
        AZ::u8 validSubdivisions = Utils::MaxFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }
    
    TEST_F(PhysXSpecificTest, FrustumCreatePoints_Create125SidedTopCone_ReturnsPoints)
    {
        // Given a valid unit frustum with MaxSubdivisions subdivisions
        float validHeight = 1.0f;
        float validBottomRadius = 1.0f;
        float validTopRadius = 0.0f;
        AZ::u8 validSubdivisions = Utils::MaxFrustumSubdivisions;

        // Attempt to create a frustum point list from the given parameters
        auto points = Utils::CreatePointsAtFrustumExtents(validHeight, validBottomRadius, validTopRadius, validSubdivisions);

        // Expect the frustum creation to be successful
        EXPECT_TRUE(points.has_value());

        // Expect each generated point to be equal to the canonical frustum plotting algorithm
        SanityCheckValidFrustumParams(points.value(), validHeight, validBottomRadius, validTopRadius, validSubdivisions);
    }

    TEST_F(PhysXSpecificTest, RayCast_CastFromInsidTriangleMesh_ReturnsHitsBasedOnHitFlags)
    {
        // Add a cube to the scene
        AZStd::unique_ptr<Physics::RigidBodyStatic> rigidBody = TestUtils::CreateStaticTriangleMeshCube(3.0f);
        AZStd::shared_ptr<Physics::World> world = GetDefaultWorld();
        world->AddBody(*rigidBody);

        // Do a simple raycast from the inside of the cube
        Physics::RayCastRequest request;
        request.m_start = AZ::Vector3(0.0f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 20.0f;
        request.m_hitFlags = Physics::HitFlags::Position;

        // Verify no hit is detected
        Physics::RayCastHit hit = world->RayCast(request);
        EXPECT_FALSE(hit);

        // Set the flags to count hits for both sides of the mesh
        request.m_hitFlags = Physics::HitFlags::Position | Physics::HitFlags::MeshBothSides;

        // Verify now the hit is detected and it's indeed the cube
        AZStd::vector<Physics::RayCastHit> hits = world->RayCastMultiple(request);
        EXPECT_EQ(hits.size(), 1);
        EXPECT_TRUE(hits[0].m_body == rigidBody.get());

        // Shift the ray start position outside of the mesh
        request.m_start.SetX(-4.0f);

        // Set the flags to include multiple hits per object
        request.m_hitFlags = Physics::HitFlags::Position | Physics::HitFlags::MeshBothSides | Physics::HitFlags::MeshMultiple;

        // Verify now we have 4 hits: outside + inside of 1st side and inside + outside of the 2nd side
        hits = world->RayCastMultiple(request);
        EXPECT_EQ(hits.size(), 4);

        // Verify all hits are for the test cube
        bool testBodyInAllHits = AZStd::all_of(hits.begin(), hits.end(), [&rigidBody](const Physics::RayCastHit& hit)
            {
                return hit.m_body == rigidBody.get();
            });
        EXPECT_TRUE(testBodyInAllHits);
    }
} // namespace PhysX

