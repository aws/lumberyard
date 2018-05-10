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
#include <AzFramework/Components/TransformComponent.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <Shape/SphereShapeComponent.h>
#include <Tests/TestTypes.h>

namespace Constants = AZ::Constants;

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    class SphereShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformShapeComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_sphereShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();
            m_transformShapeComponentDescriptor.reset(TransformComponent::CreateDescriptor());
            m_transformShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_sphereShapeComponentDescriptor.reset(SphereShapeComponent::CreateDescriptor());
            m_sphereShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }
    };

    void CreateSphere(const Transform& transform, const float radius, Entity& entity)
    {
        entity.CreateComponent<TransformComponent>();
        entity.CreateComponent<SphereShapeComponent>();

        entity.Init();
        entity.Activate();

        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, transform);
        SphereShapeComponentRequestsBus::Event(entity.GetId(), &SphereShapeComponentRequests::SetRadius, radius);
    }

    void CreateUnitSphere(const Vector3& position, Entity& entity)
    {
        CreateSphere(Transform::CreateTranslation(position), 0.5f, entity);
    }

    void CreateUnitSphereAtOrigin(Entity& entity)
    {
        CreateUnitSphere(Vector3::CreateZero(), entity);
    }

    /** 
     * @brief Creates a point in a sphere using spherical coordinates
     * @radius The radial distance from the center of the sphere
     * @verticalAngle The angle around the sphere vertically - think top to bottom
     * @horizontalAngle The angle around the sphere horizontally- think left to right
     * @return A point represeting the coordinates in the sphere
     */
    Vector3 CreateSpherePoint(float radius, float verticalAngle, float horizontalAngle)
    {
        return Vector3(
            radius * sinf(verticalAngle) * cosf(horizontalAngle),
            radius * sinf(verticalAngle) * sinf(horizontalAngle),
            radius * cosf(verticalAngle));
    }

    TEST_F(SphereShapeTest, SetRadiusIsPropagatedToGetConfiguration)
    {
        Entity entity;
        CreateUnitSphereAtOrigin(entity);

        float newRadius = 123.456f;
        SphereShapeComponentRequestsBus::Event(entity.GetId(), &SphereShapeComponentRequestsBus::Events::SetRadius, newRadius);

        SphereShapeConfig config(-1.0f);
        SphereShapeComponentRequestsBus::EventResult(config, entity.GetId(), &SphereShapeComponentRequestsBus::Events::GetSphereConfiguration);

        EXPECT_FLOAT_EQ(newRadius, config.m_radius);
    }

    TEST_F(SphereShapeTest, GetPointInsideSphere)
    {
        Entity entity;
        const Vector3 center(1.0f, 2.0f, 3.0f);
        CreateUnitSphere(center, entity);

        Vector3 point = center + CreateSpherePoint(0.49f, AZ::Constants::Pi / 4.0f, AZ::Constants::Pi / 4.0f);
        bool isInside = false;
        ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, point);

        EXPECT_TRUE(isInside);
    }

    TEST_F(SphereShapeTest, GetPointOutsideSphere)
    {
        Entity entity;
        const Vector3 center(1.0f, 2.0f, 3.0f);
        CreateUnitSphere(center, entity);

        Vector3 point = center + CreateSpherePoint(0.51f, AZ::Constants::Pi / 4.0f, AZ::Constants::Pi / 4.0f);
        bool isInside = true;
        ShapeComponentRequestsBus::EventResult(
            isInside, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, point);

        EXPECT_FALSE(isInside);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereSuccess1)
    {
        Entity entity;
        CreateUnitSphere(Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 5.0f, 5.0f), Vector3(0.0f, -1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.5f, 1e-4f);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereSuccess2)
    {
        Entity entity;
        CreateSphere(Transform::CreateTranslation(Vector3(-10.0f, -10.0f, -10.0f)), 2.5f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-10.0f, -10.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 7.5f, 1e-4f);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereSuccess3)
    {
        Entity entity;
        CreateSphere(Transform::CreateTranslation(Vector3(5.0f, 0.0f, 0.0f)), 1.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(6.0f, 10.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 10.0f, 1e-4f);
    }

    // transformed scaled
    TEST_F(SphereShapeTest, GetRayIntersectSphereSuccess4)
    {
        Entity entity;
        CreateSphere(
            Transform::CreateTranslation(Vector3(-8.0f, -15.0f, 5.0f)) *
            Transform::CreateScale(Vector3(5.0f)),
            0.25f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-5.0f, -15.0f, 5.0f), Vector3(-1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 1.75f, 1e-4f);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereFailure)
    {
        Entity entity;
        CreateSphere(Transform::CreateTranslation(Vector3(0.0f, 0.0f, 0.0f)), 2.0f, entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(3.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    // not transformed
    TEST_F(SphereShapeTest, GetAabb1)
    {
        Entity entity;
        CreateSphere(Transform::CreateTranslation(Vector3::CreateZero()), 2.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-2.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(2.0f)));
    }

    // transformed
    TEST_F(SphereShapeTest, GetAabb2)
    {
        Entity entity;
        CreateSphere(Transform::CreateTranslation(Vector3(200.0f, 150.0f, 60.0f)), 2.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(198.0f, 148.0f, 58.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(202.0f, 152.0f, 62.0f)));
    }

    // transform scaled
    TEST_F(SphereShapeTest, GetAabb3)
    {
        Entity entity;
        CreateSphere(
            Transform::CreateTranslation(Vector3(100.0f, 200.0f, 300.0f)) *
            Transform::CreateScale(Vector3(2.5f)),
            0.5f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(98.75f, 198.75f, 298.75f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(101.25f, 201.25f, 301.25f)));
    }

    // point inside scaled
    TEST_F(SphereShapeTest, IsPointInsideSuccess1)
    {
        Entity entity;
        CreateSphere(
            Transform::CreateTranslation(Vector3(-30.0f, -30.0f, 22.0f)) *
            Transform::CreateScale(Vector3(2.0f)),
            1.2f, entity);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(-30.0f, -30.0f, 20.0f));

        EXPECT_TRUE(inside);
    }

    // point inside scaled
    TEST_F(SphereShapeTest, IsPointInsideSuccess2)
    {
        Entity entity;
        CreateSphere(
            Transform::CreateTranslation(Vector3(-30.0f, -30.0f, 22.0f)) *
            Transform::CreateScale(Vector3(1.5f)),
            1.6f, entity);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(-31.0f, -32.0f, 21.2f));

        EXPECT_TRUE(inside);
    }

    // distance scaled
    TEST_F(SphereShapeTest, DistanceFromPoint1)
    {
        Entity entity;
        CreateSphere(
            Transform::CreateTranslation(Vector3(19.0f, 34.0f, 37.0f)) *
            Transform::CreateScale(Vector3(2.0f)),
            1.0f, entity);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(13.0f, 34.0f, 37.2f));

        EXPECT_NEAR(distance, 4.0f, 1e-2f);
    }

    // distance scaled
    TEST_F(SphereShapeTest, DistanceFromPoint2)
    {
        Entity entity;
        CreateSphere(
            Transform::CreateTranslation(Vector3(19.0f, 34.0f, 37.0f)) *
            Transform::CreateScale(Vector3(0.5f)),
            1.0f, entity);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(19.0f, 37.0f, 37.2f));

        EXPECT_NEAR(distance, 2.5f, 1e-2f);
    }
}