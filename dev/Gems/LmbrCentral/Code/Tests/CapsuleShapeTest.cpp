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
#include <AzCore/Math/Random.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Shape/CapsuleShapeComponent.h>
#include <Tests/TestTypes.h>

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    class CapsuleShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_capsuleShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_capsuleShapeComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(CapsuleShapeComponent::CreateDescriptor());
            m_capsuleShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateCapsule(const Transform& transform, const float radius, const float height, Entity& entity)
    {
        entity.CreateComponent<CapsuleShapeComponent>();
        entity.CreateComponent<TransformComponent>();

        entity.Init();
        entity.Activate();

        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, transform);

        CapsuleShapeComponentRequestsBus::Event(
            entity.GetId(), &CapsuleShapeComponentRequestsBus::Events::SetHeight, height);
        CapsuleShapeComponentRequestsBus::Event(
            entity.GetId(), &CapsuleShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess1)
    {
        Entity entity;
        CreateCapsule(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(0.0f, 0.0f, 5.0f)),
            0.5f, 5.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 5.0f, 5.0f), Vector3(0.0f, -1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.5f, 1e-4f);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess2)
    {
        Entity entity;
        CreateCapsule(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi),
            Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 5.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-10.0f, -20.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 7.5f, 1e-2f);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess3)
    {
        Entity entity;
        CreateCapsule(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi),
            Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 5.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-10.0f, -10.0f, -10.0f), Vector3(0.0f, 0.0f, 1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 9.0f, 1e-2f);
    }

    // test sphere case
    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess4)
    {
        Entity entity;
        CreateCapsule(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi),
            Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 0.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-10.0f, -10.0f, -10.0f), Vector3(0.0f, 0.0f, 1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 9.0f, 1e-2f);
    }

    // transformed scaled
    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess5)
    {
        Entity entity;
        CreateCapsule(
            Transform::CreateTranslation(Vector3(-4.0f, -12.0f, -3.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateScale(Vector3(6.0f)),
            0.25f, 1.5f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-4.0f, -21.0f, -3.0f), Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.5f, 1e-2f);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleFailure)
    {
        Entity entity;
        CreateCapsule(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(CapsuleShapeTest, GetAabb1)
    {
        Entity entity;
        CreateCapsule(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-5.0f, -15.0f, -5.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(5.0f, -5.0f, 5.0f)));
    }

    TEST_F(CapsuleShapeTest, GetAabb2)
    {
        Entity entity;
        CreateCapsule(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi) *
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisY(), Constants::QuarterPi), Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 5.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-12.0606f, -12.0606f, -1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(-7.9393f, -7.9393f, 1.0f)));
    }

    // test with scale
    TEST_F(CapsuleShapeTest, GetAabb3)
    {
        Entity entity;
        CreateCapsule(Transform::CreateScale(Vector3(3.5f)), 2.0f, 4.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-7.0f, -7.0f, -7.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(7.0f, 7.0f, 7.0f)));
    }

    // test with scale and translation
    TEST_F(CapsuleShapeTest, GetAabb4)
    {
        Entity entity;
        CreateCapsule(
            Transform::CreateTranslation(Vector3(5.0f, 20.0f, 0.0f)) *
            Transform::CreateScale(Vector3(2.5f)), 1.0f, 5.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(2.5f, 17.5f, -6.25f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(7.5f, 22.5f, 6.25f)));
    }

    // point inside scaled
    TEST_F(CapsuleShapeTest, IsPointInsideSuccess1)
    {
        Entity entity;
        CreateCapsule(
            Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateScale(Vector3(2.5f, 1.0f, 1.0f)), // test max scale
            0.5f, 2.0f, entity);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(27.0f, 28.5f, 40.0f));

        EXPECT_TRUE(inside);
    }

    // point inside scaled
    TEST_F(CapsuleShapeTest, IsPointInsideSuccess2)
    {
        Entity entity;
        CreateCapsule(
            Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(0.5f)),
            0.5f, 2.0f, entity);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(27.0f, 28.155f, 37.82f));

        EXPECT_TRUE(inside);
    }

    // distance scaled - along length
    TEST_F(CapsuleShapeTest, DistanceFromPoint1)
    {
        Entity entity;
        CreateCapsule(
            Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(2.0f)),
            0.5f, 4.0f, entity);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(27.0f, 28.0f, 41.0f));

        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    // distance scaled - from end
    TEST_F(CapsuleShapeTest, DistanceFromPoint2)
    {
        Entity entity;
        CreateCapsule(
            Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(2.0f)),
            0.5f, 4.0f, entity);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(22.757f, 32.243f, 38.0f));

        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }
}