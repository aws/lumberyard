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

#include <AzFramework/Components/TransformComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Shape/SplineComponent.h>
#include <Shape/TubeShapeComponent.h>
#include <Tests/TestTypes.h>

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    class TubeShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_splineComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_tubeShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_tubeShapeComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(TubeShapeComponent::CreateDescriptor());
            m_tubeShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_splineComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(SplineComponent::CreateDescriptor());
            m_splineComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateTube(const Transform& transform, const float radius, Entity& entity)
    {
        entity.CreateComponent<TransformComponent>();
        entity.CreateComponent<SplineComponent>();
        entity.CreateComponent<TubeShapeComponent>();

        entity.Init();
        entity.Activate();

        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, transform);

        SplineComponentRequestBus::Event(
            entity.GetId(), &SplineComponentRequests::SetVertices,
            AZStd::vector<Vector3>
            {
                Vector3(-3.0f, 0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f),
                Vector3(1.0f, 0.0f, 0.0f), Vector3(3.0f, 0.0f, 0.0f)
            }
        );

        TubeShapeComponentRequestsBus::Event(entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    TEST_F(TubeShapeTest, GetRayIntersectTubeSuccess1)
    {
        Entity entity;
        CreateTube(
            Transform::CreateIdentity(),
            1.0f,
            entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, -3.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    // firing at end of tube
    TEST_F(TubeShapeTest, GetRayIntersectTubeSuccess2)
    {
        Entity entity;
        CreateTube(
            Transform::CreateIdentity(),
            1.0f,
            entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(6.0f, 0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    // firing at beginning of tube
    TEST_F(TubeShapeTest, GetRayIntersectTubeSuccess3)
    {
        Entity entity;
        CreateTube(
            Transform::CreateIdentity(),
            1.0f,
            entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-6.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    // transformed and scaled
    TEST_F(TubeShapeTest, GetRayIntersectTubeSuccess4)
    {
        Entity entity;
        CreateTube(
            Transform::CreateTranslation(Vector3(-40.0f, 6.0f, 1.0f)) *
            Transform::CreateScale(Vector3(2.5f, 1.0f, 1.0f)), // test max scale
            1.0f,
            entity);

        // set variable radius
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 2.0f);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-17.0f, 6.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 8.0f, 1e-2f);
    }

    // above tube
    TEST_F(TubeShapeTest, GetRayIntersectTubeFailure)
    {
        Entity entity;
        CreateTube(
            Transform::CreateIdentity(),
            1.0f,
            entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 2.0f, 2.0f), Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(TubeShapeTest, GetAabb1)
    {
        Entity entity;
        CreateTube(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(0.0f, -10.0f, 0.0f)),
            1.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-4.0f, -11.0f, -1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(4.0f, -9.0f, 1.0f)));
    }

    TEST_F(TubeShapeTest, GetAabb2)
    {
        Entity entity;
        CreateTube(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::QuarterPi) *
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisY(), Constants::QuarterPi), Vector3(-10.0f, -10.0f, 0.0f)),
            2.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-14.1213f, -13.5f, -3.5f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(-5.8786f, -6.5f, 3.5f)));
    }

    TEST_F(TubeShapeTest, GetAabb3)
    {
        Entity entity;
        CreateTube(Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::QuarterPi) *
            Quaternion::CreateFromAxisAngle(Vector3::CreateAxisY(), Constants::QuarterPi), Vector3(-10.0f, -10.0f, 0.0f)),
            2.0f, entity);

        // set variable radius
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-15.1213f, -14.5f, -4.5f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(-4.87867f, -5.5f, 4.5f)));
    }

    // variable radius and scale
    TEST_F(TubeShapeTest, GetAabb4)
    {
        Entity entity;
        CreateTube(Transform::CreateScale(Vector3(1.0f, 1.0f, 2.0f)), 1.0f, entity);

        // set variable radius
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 3.0f);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-14.0f, -8.0f, -8.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(14.0f, 8.0f, 8.0f)));
    }

    // variable radius and scale
    TEST_F(TubeShapeTest, IsPointInsideSuccess1)
    {
        Entity entity;
        CreateTube(
            Transform::CreateTranslation(Vector3(37.0f, 36.0f, 32.0f)) *
            Transform::CreateScale(Vector3(1.0f, 2.0f, 1.0f)), 1.5f, entity);

        // set variable radius
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 3.0f);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(43.6f, 35.8f, 37.86f));

        EXPECT_TRUE(inside);
    }

    // variable radius and scale
    TEST_F(TubeShapeTest, IsPointInsideSuccess2)
    {
        Entity entity;
        CreateTube(
            Transform::CreateTranslation(Vector3(37.0f, 36.0f, 32.0f)) *
            Transform::CreateRotationZ(Constants::QuarterPi) *
            Transform::CreateRotationY(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(0.8f, 1.5f, 1.5f)), 1.5f, entity);

        // set variable radius
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 1, 0.2f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 2, 0.5f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 2.0f);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(37.6f, 36.76f, 34.0f));

        EXPECT_TRUE(inside);
    }

    // distance scaled - along length
    TEST_F(TubeShapeTest, DistanceFromPoint1)
    {
        Entity entity;
        CreateTube(
            Transform::CreateTranslation(Vector3(37.0f, 36.0f, 39.0f)) *
            Transform::CreateScale(Vector3(2.0f, 1.5f, 1.5f)), 1.5f, entity);

        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 1, 0.2f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 2, 0.5f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 2.0f);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(53.0f, 36.0f, 39.0f));

        EXPECT_NEAR(distance, 3.0f, 1e-2f);
    }

    // distance scaled - along length
    TEST_F(TubeShapeTest, DistanceFromPoint2)
    {
        Entity entity;
        CreateTube(
            Transform::CreateTranslation(Vector3(37.0f, 36.0f, 39.0f)) *
            Transform::CreateScale(Vector3(2.0f, 1.5f, 1.5f)), 1.5f, entity);

        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 1, 0.2f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 2, 0.5f);
        TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 2.0f);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(39.0f, 41.0f, 39.0f));

        EXPECT_NEAR(distance, 1.0f, 1e-2f);
    }
}