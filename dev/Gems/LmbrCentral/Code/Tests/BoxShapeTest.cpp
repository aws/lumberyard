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
#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Shape/BoxShapeComponent.h>
#include <Tests/TestTypes.h>

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    class BoxShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_boxShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_boxShapeComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(BoxShapeComponent::CreateDescriptor());
            m_boxShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateBox(const Transform& transform, const Vector3& dimensions, Entity& entity)
    {
        entity.CreateComponent<BoxShapeComponent>();
        entity.CreateComponent<TransformComponent>();

        entity.Init();
        entity.Activate();

        // use identity transform so that we test points against an AABB
        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, transform);
        BoxShapeComponentRequestsBus::Event(entity.GetId(), &BoxShapeComponentRequestsBus::Events::SetBoxDimensions, dimensions);
    }

    void CreateDefaultBox(const Transform& transform, Entity& entity)
    {
        CreateBox(transform, Vector3(10.0f, 10.0f, 10.0f), entity);
    }

    bool RandomPointsAreInBox(const Entity& entity, const RandomDistributionType distributionType)
    {
        const size_t testPoints = 10000;

        Vector3 testPoint;
        bool testPointInVolume = false;

        // test a bunch of random points generated with a random distribution type
        // they should all end up in the volume
        for (size_t i = 0; i < testPoints; ++i)
        {
            ShapeComponentRequestsBus::EventResult(
                testPoint, entity.GetId(), &ShapeComponentRequestsBus::Events::GenerateRandomPointInside, distributionType);

            ShapeComponentRequestsBus::EventResult(
                testPointInVolume, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, testPoint);

            if (!testPointInVolume)
            {
                return false;
            }
        }

        return true;
    }

    TEST_F(BoxShapeTest, NormalDistributionRandomPointsAreInAABB)
    {
        // don't rotate transform so that this is an AABB
        const Transform transform = Transform::CreateTranslation(Vector3(5.0f, 5.0f, 5.0f));

        Entity entity;
        CreateDefaultBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, RandomDistributionType::Normal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, UniformRealDistributionRandomPointsAreInAABB)
    {
        // don't rotate transform so that this is an AABB
        const Transform transform = Transform::CreateTranslation(Vector3(5.0f, 5.0f, 5.0f));

        Entity entity;
        CreateDefaultBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, RandomDistributionType::UniformReal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, NormalDistributionRandomPointsAreInOBB)
    {
        // rotate to end up with an OBB
        const Transform transform = Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateRotationY(Constants::QuarterPi), Vector3(5.0f, 5.0f, 5.0f));

        Entity entity;
        CreateDefaultBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, RandomDistributionType::Normal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, UniformRealDistributionRandomPointsAreInOBB)
    {
        // rotate to end up with an OBB
        const Transform transform = Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateRotationY(Constants::QuarterPi), Vector3(5.0f, 5.0f, 5.0f));

        Entity entity;
        CreateDefaultBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, RandomDistributionType::UniformReal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxSuccess1)
    {
        Entity entity;
        CreateBox(
            Transform::CreateTranslation(Vector3(0.0f, 0.0f, 5.0f)) *
            Transform::CreateRotationZ(Constants::QuarterPi),
            Vector3(1.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 5.0f, 5.0f), Vector3(0.0f, -1.0f, 0.0f), distance);

        // 5.0 - 0.707 ~= 4.29 (box rotated by 45 degrees)
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.29f, 1e-2f);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxSuccess2)
    {
        Entity entity;
        CreateBox(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi) *
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisZ(), Constants::QuarterPi), Vector3(-10.0f, -10.0f, -10.0f)),
            Vector3(4.0f, 4.0f, 2.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-10.0f, -10.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), distance);
        
        // 0.70710678 * 4 = 2.8284271
        // 10.0 - 2.8284271 ~= 7.17157287
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 7.17f, 1e-2f);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxSuccess3)
    {
        Entity entity;
        CreateBox(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(100.0f, 100.0f, 0.0f)),
            Vector3(5.0f, 5.0f, 5.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(100.0f, 100.0f, -100.0f), Vector3(0.0f, 0.0f, 1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 97.5f, 1e-2f);
    }

    // transformed scaled
    TEST_F(BoxShapeTest, GetRayIntersectBoxSuccess4)
    {
        Entity entity;
        CreateBox(
            Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateFromAxisAngle(Vector3::CreateAxisY(), Constants::QuarterPi),
                Vector3(0.0f, 0.0f, 5.0f)) *
            Transform::CreateScale(Vector3(3.0f)),
            Vector3(2.0f, 4.0f, 1.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(1.0f, -10.0f, 4.0f), Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.0f, 1e-2f);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxFailure)
    {
        Entity entity;
        CreateBox(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(0.0f, -10.0f, 0.0f)),
            Vector3(2.0f, 6.0f, 4.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3::CreateZero(), Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(BoxShapeTest, GetAabb1)
    {
        // not rotated - AABB input
        Entity entity;
        CreateBox(Transform::CreateIdentity(), Vector3(1.5f, 3.5f, 5.5f), entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-0.75f, -1.75f, -2.75f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(0.75f, 1.75f, 2.75f)));
    }

    TEST_F(BoxShapeTest, GetAabb2)
    {
        // rotated - OBB input
        Entity entity;
        CreateDefaultBox(
            Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateRotationY(Constants::QuarterPi),
                Vector3(5.0f, 5.0f, 5.0f)), entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-2.07106f, 0.0f, -2.07106f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(12.07106f, 10.0f, 12.07106f)));
    }

    TEST_F(BoxShapeTest, GetAabb3)
    {
        // rotated - OBB input
        Entity entity;
        CreateBox(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::QuarterPi) *
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisY(), Constants::QuarterPi), Vector3(0.0f, 0.0f, 0.0f)),
            Vector3(2.0f, 5.0f, 1.0f), entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-1.06066f, -2.517766f, -2.517766f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(1.06066f, 2.517766f, 2.517766f)));
    }

    // transformed
    TEST_F(BoxShapeTest, GetAabb4)
    {
        // not rotated - AABB input
        Entity entity;
        CreateBox(Transform::CreateTranslation(
            Vector3(100.0f, 70.0f, 30.0f)), Vector3(1.8f, 3.5f, 5.2f), entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(99.1f, 68.25f, 27.4f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(100.9f, 71.75f, 32.6f)));
    }

    // transformed scaled
    TEST_F(BoxShapeTest, GetAabb5)
    {
        Entity entity;
        CreateBox(
            Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateFromAxisAngle(Vector3::CreateAxisY(), Constants::QuarterPi),
                Vector3::CreateZero()) *
            Transform::CreateScale(Vector3(3.0f)),
            Vector3(2.0f, 4.0f, 1.0f), entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-3.18f, -6.0f, -3.18f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(3.18f, 6.0f, 3.18f)));
    }

    // point inside scaled
    TEST_F(BoxShapeTest, IsPointInsideSuccess1)
    {
        Entity entity;
        CreateBox(
            Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateFromAxisAngle(Vector3::CreateAxisZ(), Constants::QuarterPi),
                Vector3(23.0f, 12.0f, 40.0f)) *
            Transform::CreateScale(Vector3(3.0f)),
            Vector3(2.0f, 6.0f, 3.5f), entity);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(27.0f, 5.0f, 36.0f));

        EXPECT_TRUE(inside);
    }

    // point inside scaled
    TEST_F(BoxShapeTest, IsPointInsideSuccess2)
    {
        Entity entity;
        CreateBox(
            Transform::CreateTranslation(Vector3(23.0f, 12.0f, 40.0f)) *
            Transform::CreateRotationX(-Constants::QuarterPi) *
            Transform::CreateRotationZ(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(2.0f)),
            Vector3(4.0f, 7.0f, 3.5f), entity);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(18.0f, 16.0f, 40.0f));

        EXPECT_TRUE(inside);
    }

    // distance scaled
    TEST_F(BoxShapeTest, DistanceFromPoint1)
    {
        Entity entity;
        CreateBox(
            Transform::CreateTranslation(Vector3(10.0f, 37.0f, 32.0f)) *
            Transform::CreateRotationZ(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(3.0f, 1.0f, 1.0f)),
            Vector3(4.0f, 2.0f, 10.0f), entity);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(3.6356f, 30.636f, 40.0f));

        EXPECT_NEAR(distance, 3.0f, 1e-2f);
    }

    // distance scaled
    TEST_F(BoxShapeTest, DistanceFromPoint2)
    {
        Entity entity;
        CreateBox(
            Transform::CreateTranslation(Vector3(10.0f, 37.0f, 32.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::HalfPi) *
            Transform::CreateScale(Vector3(3.0f, 1.0f, 1.0f)),
            Vector3(4.0f, 2.0f, 10.0f), entity);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(10.0f, 37.0f, 48.0f));

        EXPECT_NEAR(distance, 13.0f, 1e-2f);
    }
}