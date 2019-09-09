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
#include <Shape/CylinderShapeComponent.h>
#include <Tests/TestTypes.h>

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    class CylinderShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_cylinderShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_cylinderShapeComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(CylinderShapeComponent::CreateDescriptor());
            m_cylinderShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_cylinderShapeComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateCylinder(const Transform& transform, float radius, float height, Entity& entity)
    {
        entity.CreateComponent<CylinderShapeComponent>();
        entity.CreateComponent<TransformComponent>();

        entity.Init();
        entity.Activate();

        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, transform);

        CylinderShapeComponentRequestsBus::Event(entity.GetId(), &CylinderShapeComponentRequestsBus::Events::SetHeight, height);
        CylinderShapeComponentRequestsBus::Event(entity.GetId(), &CylinderShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    void CreateDefaultCylinder(const Transform& transform, Entity& entity)
    {
        CreateCylinder(transform, 10.0f, 10.0f, entity);
    }

    bool RandomPointsAreInCylinder(const RandomDistributionType distributionType)
    {
        //Apply a simple transform to put the Cylinder off the origin
        Transform transform = Transform::CreateIdentity();
        transform.SetRotationPartFromQuaternion(Quaternion::CreateRotationY(Constants::HalfPi));
        transform.SetTranslation(Vector3(5.0f, 5.0f, 5.0f));

        Entity entity;
        CreateDefaultCylinder(transform, entity);

        const u32 testPoints = 10000;
        Vector3 testPoint;
        bool testPointInVolume = false;

        //Test a bunch of random points generated with the Normal random distribution type
        //They should all end up in the volume
        for (u32 i = 0; i < testPoints; ++i)
        {
            ShapeComponentRequestsBus::EventResult(testPoint, entity.GetId(), &ShapeComponentRequestsBus::Events::GenerateRandomPointInside, distributionType);

            ShapeComponentRequestsBus::EventResult(testPointInVolume, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, testPoint);

            if (!testPointInVolume)
            {
                return false;
            }
        }

        return true;
    }

    TEST_F(CylinderShapeTest, NormalDistributionRandomPointsAreInVolume)
    {
        EXPECT_TRUE(RandomPointsAreInCylinder(RandomDistributionType::Normal));
    }

    TEST_F(CylinderShapeTest, UniformRealDistributionRandomPointsAreInVolume)
    {
        EXPECT_TRUE(RandomPointsAreInCylinder(RandomDistributionType::UniformReal));
    }

    TEST_F(CylinderShapeTest, GetRayIntersectCylinderSuccess1)
    {
        Entity entity;
        CreateCylinder(Transform::CreateFromQuaternionAndTranslation(
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

    TEST_F(CylinderShapeTest, GetRayIntersectCylinderSuccess2)
    {
        Entity entity;
        CreateCylinder(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi), Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 5.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-10.0f, -20.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 7.5f, 1e-2f);
    }

    TEST_F(CylinderShapeTest, GetRayIntersectCylinderSuccess3)
    {
        Entity entity;
        CreateCylinder(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi), Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 5.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-10.0f, -10.0f, -10.0f), Vector3(0.0f, 0.0f, 1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 9.0f, 1e-2f);
    }

    // transform scaled
    TEST_F(CylinderShapeTest, GetRayIntersectCylinderSuccess4)
    {
        Entity entity;
        CreateCylinder(
            Transform::CreateTranslation(Vector3(-14.0f, -14.0f, -1.0f)) *
            Transform::CreateRotationY(Constants::HalfPi) *
            Transform::CreateRotationZ(Constants::HalfPi) *
            Transform::CreateScale(Vector3(4.0f)),
            1.0f, 1.25f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-9.0f, -14.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 2.5f, 1e-2f);
    }

    TEST_F(CylinderShapeTest, GetRayIntersectCylinderFailure)
    {
        Entity entity;
        CreateCylinder(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(CylinderShapeTest, GetAabb1)
    {
        Entity entity;
        CreateCylinder(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-5.0f, -15.0f, -0.5f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(5.0f, -5.0f, 0.5f)));
    }

    TEST_F(CylinderShapeTest, GetAabb2)
    {
        Entity entity;
        CreateCylinder(
            Transform::CreateTranslation(Vector3(-10.0f, -10.0f, 0.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::QuarterPi),
            1.0f, 5.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-12.4748f, -12.4748f, -1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(-7.52512f, -7.52512f, 1.0f)));
    }

    // transformed scaled
    TEST_F(CylinderShapeTest, GetAabb3)
    {
        Entity entity;
        CreateCylinder(
            Transform::CreateTranslation(Vector3(-10.0f, -10.0f, 10.0f)) *
            Transform::CreateScale(Vector3(3.5f)),
            1.0f, 5.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-13.5f, -13.5f, 1.25f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(-6.5f, -6.5f, 18.75f)));
    }

    TEST_F(CylinderShapeTest, GetTransformAndLocalBounds1)
    {
        Entity entity;
        Transform transformIn = Transform::CreateIdentity();
        CreateCylinder(transformIn, 5.0f, 1.0f, entity);

        Transform transformOut;
        Aabb aabb;
        ShapeComponentRequestsBus::Event(entity.GetId(), &ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transformIn));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-5.0f, -5.0f, -0.5f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(5.0f, 5.0f, 0.5f)));
    }

    TEST_F(CylinderShapeTest, GetTransformAndLocalBounds2)
    {
        Entity entity;
        Transform transformIn = Transform::CreateTranslation(Vector3(-10.0f, -10.0f, 10.0f)) * Transform::CreateScale(Vector3(3.5f));
        CreateCylinder(transformIn, 5.0f, 5.0f, entity);

        Transform transformOut;
        Aabb aabb;
        ShapeComponentRequestsBus::Event(entity.GetId(), &ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transformIn));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-5.0f, -5.0f, -2.5f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(5.0f, 5.0f, 2.5f)));
    }

    // point inside scaled
    TEST_F(CylinderShapeTest, IsPointInsideSuccess1)
    {
        Entity entity;
        CreateCylinder(
            Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateScale(Vector3(2.5f, 1.0f, 1.0f)), // test max scale
            0.5f, 2.0f, entity);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(27.0f, 28.5f, 40.0f));

        EXPECT_TRUE(inside);
    }

    // point inside scaled
    TEST_F(CylinderShapeTest, IsPointInsideSuccess2)
    {
        Entity entity;
        CreateCylinder(
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
    TEST_F(CylinderShapeTest, DistanceFromPoint1)
    {
        Entity entity;
        CreateCylinder(
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
    TEST_F(CylinderShapeTest, DistanceFromPoint2)
    {
        Entity entity;
        CreateCylinder(
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