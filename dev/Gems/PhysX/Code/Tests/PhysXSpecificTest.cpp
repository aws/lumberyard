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

#include <StdAfx.h>

#include <AzTest/AzTest.h>
#include <Source/PhysXMathConversion.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>

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
    EBUS_EVENT_RESULT(rigidBody, Physics::SystemRequestBus, CreateRigidBody, rigidBodySettings);
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
