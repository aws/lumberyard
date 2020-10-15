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
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>
#include <Shape/QuadShapeComponent.h>

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace
{
    const uint32_t QuadCount = 5;

    // Various transforms for quads
    const AZStd::array<Transform, QuadCount> QuadTransforms =
    {
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3( 1.0f,   2.0f,  3.0f), Transform::Axis::ZPositive),
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3(-5.0f,   3.0f, -2.0f), Transform::Axis::ZPositive),
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3( 2.0f, -10.0f,  5.0f), Transform::Axis::ZPositive),
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3(-5.0f,  -2.0f, -1.0f), Transform::Axis::ZPositive),
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3(-1.0f,  -7.0f,  2.0f), Transform::Axis::ZPositive),
    };

    // Various width/height for quads
    const AZStd::array<QuadShapeConfig, QuadCount> QuadDims =
    {
        QuadShapeConfig({0.5f, 1.0f}),
        QuadShapeConfig({2.0f, 4.0f}),
        QuadShapeConfig({3.0f, 3.0f}),
        QuadShapeConfig({4.0f, 2.0f}),
        QuadShapeConfig({1.0f, 0.5f}),
    };

    const uint32_t RayCount = 5;

    // Various normalized offset directions from center of quad along quad's surface.
    const AZStd::array<Vector3, RayCount> OffsetsFromCenter =
    {
        Vector3( 0.18f, -0.50f, 0.0f).GetNormalized(),
        Vector3(-0.08f,  0.59f, 0.0f).GetNormalized(),
        Vector3( 0.92f,  0.94f, 0.0f).GetNormalized(),
        Vector3(-0.10f, -0.99f, 0.0f).GetNormalized(),
        Vector3(-0.44f,  0.48f, 0.0f).GetNormalized(),
    };

    // Various directions away from a point on the quad's surface
    const AZStd::array<Vector3, RayCount> OffsetsFromSurface =
    {
        Vector3( 0.69f,  0.38f,  0.09f).GetNormalized(),
        Vector3(-0.98f, -0.68f, -0.28f).GetNormalized(),
        Vector3(-0.45f,  0.31f, -0.05f).GetNormalized(),
        Vector3( 0.51f, -0.75f,  0.73f).GetNormalized(),
        Vector3(-0.99f,  0.56f,  0.41f).GetNormalized(),
    };

    // Various distance away from the surface for the rays
    const AZStd::array<float, RayCount> RayDistances =
    {
        0.5f, 1.0f, 2.0f, 4.0f, 8.0f
    };
}

namespace UnitTest
{
    class QuadShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformShapeComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_quadShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();
            m_transformShapeComponentDescriptor.reset(TransformComponent::CreateDescriptor());
            m_transformShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_quadShapeComponentDescriptor.reset(QuadShapeComponent::CreateDescriptor());
            m_quadShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformShapeComponentDescriptor.reset();
            m_quadShapeComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }

    };

    void CreateQuad(const Transform& transform, float width, float height, Entity& entity)
    {
        entity.CreateComponent<TransformComponent>();
        entity.CreateComponent<QuadShapeComponent>();

        entity.Init();
        entity.Activate();

        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, transform);
        QuadShapeComponentRequestBus::Event(entity.GetId(), &QuadShapeComponentRequests::SetQuadWidth, width);
        QuadShapeComponentRequestBus::Event(entity.GetId(), &QuadShapeComponentRequests::SetQuadHeight, height);
    }

    void CreateUnitQuad(const Vector3& position, Entity& entity)
    {
        CreateQuad(Transform::CreateTranslation(position), 0.5f, 0.5f, entity);
    }

    void CreateUnitQuadAtOrigin(Entity& entity)
    {
        CreateUnitQuad(Vector3::CreateZero(), entity);
    }

    void CheckQuadDistance(const Entity& entity, const Transform& transform, const Vector3& point, float expectedDistance, float epsilon = 0.01f)
    {
        // Check distance between quad and point at center of quad.
        float distance = -1.0f;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, transform * point);
        EXPECT_NEAR(distance, expectedDistance, epsilon);
    }

    // Tests

    TEST_F(QuadShapeTest, SetWidthHeightIsPropagatedToGetConfiguration)
    {
        Entity entity;
        CreateUnitQuadAtOrigin(entity);

        const float newWidth = 123.456f;
        const float newHeight = 654.321f;
        QuadShapeComponentRequestBus::Event(entity.GetId(), &QuadShapeComponentRequests::SetQuadWidth, newWidth);
        QuadShapeComponentRequestBus::Event(entity.GetId(), &QuadShapeComponentRequests::SetQuadHeight, newHeight);

        QuadShapeConfig config { -1.0f, -1.0f };
        QuadShapeComponentRequestBus::EventResult(config, entity.GetId(), &QuadShapeComponentRequestBus::Events::GetQuadConfiguration);

        EXPECT_FLOAT_EQ(newWidth, config.m_width);
        EXPECT_FLOAT_EQ(newHeight, config.m_height);
    }

    TEST_F(QuadShapeTest, IsPointInsideQuad)
    {
        Entity entity;
        const Vector3 center(1.0f, 2.0f, 3.0f);
        const Vector3 origin = Vector3::CreateZero();
        CreateUnitQuad(center, entity);

        bool isInside = true; // Initialize to opposite of what's expected to ensure the bus call runs.

        // Check point outside of quad
        ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, origin);
        EXPECT_FALSE(isInside);

        // Check point at center of quad (should also return false since a quad is 2D has no inside.
        isInside = true;
        ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, center);
        EXPECT_FALSE(isInside);
    }

    TEST_F(QuadShapeTest, GetRayIntersectQuadSuccess)
    {
        // Check simple case - a quad with normal facing down the Z axis intesecting with a ray going down the Z axis

        Entity entity;
        CreateUnitQuad(Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 0.0f, 10.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 5.0f, 1e-4f);

        // More complicated cases - construct rays that should intersect by starting from hit points already on the quads and working backwards

        // Create quad entities with test data in test class.
        Entity quadEntities[QuadCount];
        for (uint32_t i = 0; i < QuadCount; ++i)
        {
            CreateQuad(QuadTransforms[i], QuadDims[i].m_width, QuadDims[i].m_height, quadEntities[i]);
        }

        // Construct rays and test against the different quads
        for (uint32_t quadIndex = 0; quadIndex < QuadCount; ++quadIndex)
        {
            for (uint32_t rayIndex = 0; rayIndex < RayCount; ++rayIndex)
            {
                // OffsetsFromCenter are all less than 1, so scale by the dimensions of the quad.
                Vector3 scaledWidthHeight = Vector3(QuadDims[quadIndex].m_width, QuadDims[quadIndex].m_height, 0.0f);
                // Scale the offset and multiply by 0.5 because distance from center is half the width/height
                Vector3 scaledOffsetFromCenter = OffsetsFromCenter[rayIndex] * scaledWidthHeight * 0.5f;
                Vector3 positionOnQuadSurface = QuadTransforms[quadIndex] * scaledOffsetFromCenter;
                Vector3 rayOrigin = positionOnQuadSurface + OffsetsFromSurface[rayIndex] * RayDistances[rayIndex];

                bool rayHit = false;
                VectorFloat distance;
                ShapeComponentRequestsBus::EventResult(
                    rayHit, quadEntities[quadIndex].GetId(), &ShapeComponentRequests::IntersectRay,
                    rayOrigin, -OffsetsFromSurface[rayIndex], distance);

                EXPECT_TRUE(rayHit);
                EXPECT_NEAR(distance, RayDistances[rayIndex], 1e-4f);
            }
        }

    }

    TEST_F(QuadShapeTest, GetRayIntersectQuadFail)
    {
        // Check simple case - a quad with normal facing down the Z axis intesecting with a ray going down the Z axis, but the ray is offset enough to miss.

        Entity entity;
        CreateUnitQuad(Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 2.0f, 10.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_FALSE(rayHit);

        // More complicated cases - construct rays that should not intersect by starting from points on the quad plane but outside the quad, and working backwards

        // Create quad entities with test data in test class.
        AZStd::array<Entity, QuadCount> quadEntities;
        for (uint32_t i = 0; i < QuadCount; ++i)
        {
            CreateQuad(QuadTransforms[i], QuadDims[i].m_width, QuadDims[i].m_height, quadEntities[i]);
        }

        // Construct rays and test against the different quads
        for (uint32_t quadIndex = 0; quadIndex < QuadCount; ++quadIndex)
        {
            for (uint32_t rayIndex = 0; rayIndex < RayCount; ++rayIndex)
            {
                // OffsetsFromCenter are all less than 1, so scale by the dimensions of the quad.
                Vector3 scaledWidthHeight = Vector3(QuadDims[quadIndex].m_width, QuadDims[quadIndex].m_height, 0.0f);
                // Scale the offset and add 1.0 to OffsetsFromCenter to ensure the point is outside the quad.
                Vector3 scaledOffsetFromCenter = (Vector3::CreateOne() + OffsetsFromCenter[rayIndex]) * scaledWidthHeight; 
                Vector3 positionOnQuadSurface = QuadTransforms[quadIndex] * scaledOffsetFromCenter;
                Vector3 rayOrigin = positionOnQuadSurface + OffsetsFromSurface[rayIndex] * RayDistances[rayIndex];

                bool rayHit = false;
                VectorFloat distance;
                ShapeComponentRequestsBus::EventResult(
                    rayHit, quadEntities[quadIndex].GetId(), &ShapeComponentRequests::IntersectRay,
                    rayOrigin, -OffsetsFromSurface[rayIndex], distance);

                EXPECT_FALSE(rayHit);
            }
        }

    }

    TEST_F(QuadShapeTest, GetAabbNotTransformed)
    {
        Entity entity;
        CreateQuad(Transform::CreateTranslation(Vector3::CreateZero()), 2.0f, 4.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-1.0f, -2.0f, 0.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(1.0f, 2.0f, 0.0f)));
    }

    TEST_F(QuadShapeTest, GetAabbTranslated)
    {
        Entity entity;
        CreateQuad(Transform::CreateTranslation(Vector3(2.0f, 3.0f, 4.0f)), 2.0f, 4.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(1.0f, 1.0f, 4.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(3.0f, 5.0f, 4.0f)));
    }

    TEST_F(QuadShapeTest, GetAabbTranslatedScaled)
    {
        Entity entity;
        CreateQuad(
            Transform::CreateTranslation(Vector3(100.0f, 200.0f, 300.0f)) *
            Transform::CreateScale(Vector3(2.5f)),
            1.0f, 2.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3( 98.75f, 197.50f, 300.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(101.25f, 202.50f, 300.0f)));
    }

    TEST_F(QuadShapeTest, GetAabbRotated)
    {
        const QuadShapeConfig quadShape { 2.0f /*width*/, 3.0f /*height*/};

        Entity entity;
        Transform transform = Transform::CreateLookAt(Vector3::CreateZero(), Vector3(1.0f, 2.0f, 3.0f), Transform::Axis::ZPositive);
        CreateQuad(transform, quadShape.m_width, quadShape.m_height, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        // Test against an Aabb made by sampling points at corners.
        Aabb encompasingAabb = Aabb::CreateNull();
        AZStd::array<Vector3, 4> corners = quadShape.GetCorners();
        for (uint32_t i = 0; i < corners.size(); ++i)
        {
            encompasingAabb.AddPoint(transform * corners[i]);
        }

        EXPECT_TRUE(aabb.GetMin().IsClose(encompasingAabb.GetMin()));
        EXPECT_TRUE(aabb.GetMax().IsClose(encompasingAabb.GetMax()));
    }

    TEST_F(QuadShapeTest, IsPointInsideAlwaysFail)
    {
        // Shapes implement the concept of inside strictly, where a point on the surface is not counted
        // as being inside. Therefore a 2D shape like quad has no inside and should always return false.

        Entity entity;
        bool inside;
        CreateUnitQuadAtOrigin(entity);

        // Check a point at the center of the quad
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3::CreateZero());
        EXPECT_FALSE(inside);

        // Check a point clearly outside the quad
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(100.f, 10.0f, 10.0f));
        EXPECT_FALSE(inside);
    }

    TEST_F(QuadShapeTest, DistanceFromPoint)
    {
        float distance = 0.0f;

        const uint32_t dimCount = 2;
        const uint32_t transformCount = 3;
        const Vector2 dims[dimCount] = { Vector2 { 0.5f, 2.0f }, Vector2 { 1.5f, 0.25f } };
        Transform transforms[transformCount] =
        {
            Transform::CreateIdentity(),
            Transform::CreateLookAt(Vector3::CreateZero(), Vector3( 1.0f,  2.0f,  3.0f), Transform::Axis::ZPositive),
            Transform::CreateLookAt(Vector3::CreateZero(), Vector3(-3.0f, -2.0f, -1.0f), Transform::Axis::ZPositive),
        };

        for (uint32_t dimIndex = 0; dimIndex < dimCount; ++dimIndex)
        {
            const Vector2 dim = dims[dimIndex];

            for (uint32_t transformIndex = 0; transformIndex < transformCount; ++transformIndex)
            {
                const Transform& transform = transforms[transformIndex];

                Entity entity;
                CreateQuad(transform, dim.GetX(), dim.GetY(), entity);

                Vector2 offset = dim * 0.5f;

                // Check distance between quad and point at center of quad.
                CheckQuadDistance(entity, transform, Vector3(0.0f, 0.0f, 0.0f), 0.0f);

                // Check distance between quad and points on edge of quad.
                CheckQuadDistance(entity, transform, Vector3(offset.GetX(), 0.0f, 0.0f), 0.0f);
                CheckQuadDistance(entity, transform, Vector3(0.0f, -offset.GetY(), 0.0f), 0.0f);
                CheckQuadDistance(entity, transform, Vector3(-offset.GetX(), offset.GetY(), 0.0f), 0.0f);

                // Check distance between quad and point 1 unit directly in front of it.
                CheckQuadDistance(entity, transform, Vector3(0.0f, 0.0f, 1.0f), 1.0f);

                // Check distance between quad and point 1 unit directly to the side of the edge
                CheckQuadDistance(entity, transform, Vector3(0.0f, offset.GetY() + 1.0f, 0.0f), 1.0f);
                CheckQuadDistance(entity, transform, Vector3(offset.GetX() + 1.0f, 0.0f, 0.0f), 1.0f);
                CheckQuadDistance(entity, transform, Vector3(offset.GetX() + 1.0f, offset.GetY() + 1.0f, 0.0f), sqrtf(2.0f)); // offset 1 in both x and y from corner = sqrt(1*1 + 1*1)

                // Check distance between quad and a point 1 up and 1 to the sides and corner of it
                CheckQuadDistance(entity, transform, Vector3(0.0f, offset.GetY() + 1.0f, 1.0f), sqrtf(2.0f));
                CheckQuadDistance(entity, transform, Vector3(offset.GetX() + 1.0f, 0.0f, 1.0f), sqrtf(2.0f));
                CheckQuadDistance(entity, transform, Vector3(offset.GetX() + 1.0f, offset.GetY() + 1.0f, 1.0f), sqrtf(3.0f)); // sqrt(1*1 + 1*1 + 1*1)

                // Check distance between quad and a point 1 up and 3 to the side of it
                CheckQuadDistance(entity, transform, Vector3(0.0f, offset.GetY() + 3.0f, 1.0f), sqrtf(10.0f)); // sqrt(3*3 + 1*1)
                CheckQuadDistance(entity, transform, Vector3(offset.GetX() + 3.0f, 0.0f, 1.0f), sqrtf(10.0f));
                CheckQuadDistance(entity, transform, Vector3(offset.GetX() + 3.0f, offset.GetY() + 3.0f, 1.0f), sqrtf(19.0f)); // sqrt(3*3 + 3*3 + 1*1)
            }
        }
    }
} // namespace UnitTest
