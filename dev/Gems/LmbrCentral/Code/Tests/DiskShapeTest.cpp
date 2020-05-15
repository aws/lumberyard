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
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <Shape/DiskShapeComponent.h>

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace
{
    const uint32_t DiskCount = 5;

    // Various transforms for disks
    const AZStd::array<Transform, DiskCount> DiskTransforms =
    {
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3(1.0, 2.0, 3.0), Transform::Axis::ZPositive),
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3(-5.0, 3.0, -2.0), Transform::Axis::ZPositive),
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3(2.0, -10.0, 5.0), Transform::Axis::ZPositive),
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3(-5.0, -2.0, -1.0), Transform::Axis::ZPositive),
        Transform::CreateLookAt(Vector3::CreateZero(), Vector3(-1.0, -7.0, 2.0), Transform::Axis::ZPositive),
    };

    // Various radii for disks
    const AZStd::array<float, DiskCount> DiskRadii =
    {
        0.5f, 1.0f, 2.0f, 4.0f, 8.0f,
    };

    const uint32_t RayCount = 5;

    // Various normalized offset directions from center of disk along disk's surface.
    const AZStd::array<Vector3, RayCount> OffsetsFromCenter =
    {
        Vector3(0.18f, -0.50f, 0.0f).GetNormalized(),
        Vector3(-0.08f,  0.59f, 0.0f).GetNormalized(),
        Vector3(0.92f,  0.94f, 0.0f).GetNormalized(),
        Vector3(-0.10f, -0.99f, 0.0f).GetNormalized(),
        Vector3(-0.44f,  0.48f, 0.0f).GetNormalized(),
    };

    // Various directions away from a point on the disk's surface
    const AZStd::array<Vector3, RayCount> OffsetsFromSurface =
    {
        Vector3(0.69f,  0.38f,  0.09f).GetNormalized(),
        Vector3(-0.98f, -0.68f, -0.28f).GetNormalized(),
        Vector3(-0.45f,  0.31f, -0.05f).GetNormalized(),
        Vector3(0.51f, -0.75f,  0.73f).GetNormalized(),
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
    class DiskShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformShapeComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_diskShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();
            m_transformShapeComponentDescriptor.reset(TransformComponent::CreateDescriptor());
            m_transformShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_diskShapeComponentDescriptor.reset(DiskShapeComponent::CreateDescriptor());
            m_diskShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformShapeComponentDescriptor.reset();
            m_diskShapeComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }

    };

    void CreateDisk(const Transform& transform, const float radius, Entity& entity)
    {
        entity.CreateComponent<TransformComponent>();
        entity.CreateComponent<DiskShapeComponent>();

        entity.Init();
        entity.Activate();

        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, transform);
        DiskShapeComponentRequestBus::Event(entity.GetId(), &DiskShapeComponentRequests::SetRadius, radius);
    }

    void CreateUnitDisk(const Vector3& position, Entity& entity)
    {
        CreateDisk(Transform::CreateTranslation(position), 0.5f, entity);
    }

    void CreateUnitDiskAtOrigin(Entity& entity)
    {
        CreateUnitDisk(Vector3::CreateZero(), entity);
    }

    void CheckDistance(const Entity& entity, const Transform& transform, const Vector3& point, float expectedDistance, float epsilon = 0.001f)
    {
        // Check distance between disk and point at center of disk.
        float distance = -1.0f;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, transform * point);
        EXPECT_NEAR(distance, expectedDistance, epsilon);
    }

    // Tests

    TEST_F(DiskShapeTest, SetRadiusIsPropagatedToGetConfiguration)
    {
        Entity entity;
        CreateUnitDiskAtOrigin(entity);

        const float newRadius = 123.456f;
        DiskShapeComponentRequestBus::Event(entity.GetId(), &DiskShapeComponentRequests::SetRadius, newRadius);

        DiskShapeConfig config(-1.0f);
        DiskShapeComponentRequestBus::EventResult(config, entity.GetId(), &DiskShapeComponentRequestBus::Events::GetDiskConfiguration);

        EXPECT_FLOAT_EQ(newRadius, config.m_radius);
    }

    TEST_F(DiskShapeTest, IsPointInsideDisk)
    {
        Entity entity;
        const Vector3 center(1.0f, 2.0f, 3.0f);
        const Vector3 origin = Vector3::CreateZero();
        CreateUnitDisk(center, entity);

        bool isInside = true; // Initialize to opposite of what's expected to ensure the bus call runs.

        // Check point outside of disk
        ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, origin);
        EXPECT_FALSE(isInside);

        // Check point at center of disk (should also return false since a disk is 2D has no inside.
        isInside = true;
        ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, center);
        EXPECT_FALSE(isInside);
    }
    
    TEST_F(DiskShapeTest, GetRayIntersectDiskSuccess)
    {
        // Check simple case - a disk with normal facing down the Z axis intesecting with a ray going down the Z axis

        Entity entity;
        CreateUnitDisk(Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 0.0f, 10.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 5.0f, 1e-4f);

        // More complicated cases - construct rays that should intersect by starting from hit points already on the disks and working backwards

        // Create disk entities with test data in test class.
        Entity diskEntities[DiskCount];
        for (uint32_t i = 0; i < DiskCount; ++i)
        {
            CreateDisk(DiskTransforms[i], DiskRadii[i], diskEntities[i]);
        }

        // Offsets from center scaled down from the disk edge so that all the rays should hit
        const AZStd::array<float, RayCount> offsetFromCenterScale =
        {
            0.8f,
            0.2f,
            0.5f,
            0.9f,
            0.1f,
        };

        // Construct rays and test against the different disks
        for (uint32_t diskIndex = 0; diskIndex < DiskCount; ++diskIndex)
        {
            for (uint32_t rayIndex = 0; rayIndex < RayCount; ++rayIndex)
            {
                Vector3 scaledOffsetFromCenter = OffsetsFromCenter[rayIndex] * DiskRadii[diskIndex] * offsetFromCenterScale[rayIndex];
                Vector3 positionOnDiskSurface = DiskTransforms[diskIndex] * scaledOffsetFromCenter;
                Vector3 rayOrigin = positionOnDiskSurface + OffsetsFromSurface[rayIndex] * RayDistances[rayIndex];

                bool rayHit = false;
                VectorFloat distance;
                ShapeComponentRequestsBus::EventResult(
                    rayHit, diskEntities[diskIndex].GetId(), &ShapeComponentRequests::IntersectRay,
                    rayOrigin, -OffsetsFromSurface[rayIndex], distance);

                EXPECT_TRUE(rayHit);
                EXPECT_NEAR(distance, RayDistances[rayIndex], 1e-4f);
            }
        }

    }

    TEST_F(DiskShapeTest, GetRayIntersectDiskFail)
    {
        // Check simple case - a disk with normal facing down the Z axis intesecting with a ray going down the Z axis, but the ray is offset enough to miss.

        Entity entity;
        CreateUnitDisk(Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(0.0f, 2.0f, 10.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_FALSE(rayHit);

        // More complicated cases - construct rays that should not intersect by starting from points on the disk plane but outside the disk, and working backwards

        // Create disk entities with test data in test class.
        AZStd::array<Entity, DiskCount> diskEntities;
        for (uint32_t i = 0; i < DiskCount; ++i)
        {
            CreateDisk(DiskTransforms[i], DiskRadii[i], diskEntities[i]);
        }

        // Offsets from center scaled up from the disk edge so that all the rays should miss
        const AZStd::array<float, RayCount> offsetFromCenterScale =
        {
            1.8f,
            1.2f,
            1.5f,
            1.9f,
            1.1f,
        };

        // Construct rays and test against the different disks
        for (uint32_t diskIndex = 0; diskIndex < DiskCount; ++diskIndex)
        {
            for (uint32_t rayIndex = 0; rayIndex < RayCount; ++rayIndex)
            {
                Vector3 scaledOffsetFromCenter = OffsetsFromCenter[rayIndex] * DiskRadii[diskIndex] * offsetFromCenterScale[rayIndex];
                Vector3 positionOnDiskSurface = DiskTransforms[diskIndex] * scaledOffsetFromCenter;
                Vector3 rayOrigin = positionOnDiskSurface + OffsetsFromSurface[rayIndex] * RayDistances[rayIndex];

                bool rayHit = false;
                VectorFloat distance;
                ShapeComponentRequestsBus::EventResult(
                    rayHit, diskEntities[diskIndex].GetId(), &ShapeComponentRequests::IntersectRay,
                    rayOrigin, -OffsetsFromSurface[rayIndex], distance);

                EXPECT_FALSE(rayHit);
            }
        }

    }

    TEST_F(DiskShapeTest, GetAabbNotTransformed)
    {
        Entity entity;
        CreateDisk(Transform::CreateTranslation(Vector3::CreateZero()), 2.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-2.0f, -2.0f, 0.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(2.0f, 2.0f, 0.0f)));
    }

    TEST_F(DiskShapeTest, GetAabbTranslated)
    {
        Entity entity;
        CreateDisk(Transform::CreateTranslation(Vector3(2.0f, 3.0f, 4.0f)), 2.0f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(0.0f, 1.0f, 4.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(4.0f, 5.0f, 4.0f)));
    }

    TEST_F(DiskShapeTest, GetAabbTranslatedScaled)
    {
        Entity entity;
        CreateDisk(
            Transform::CreateTranslation(Vector3(100.0f, 200.0f, 300.0f)) *
            Transform::CreateScale(Vector3(2.5f)),
            0.5f, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(98.75f, 198.75f, 300.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(101.25f, 201.25f, 300.0f)));
    }

    TEST_F(DiskShapeTest, GetAabbRotated)
    {
        const float radius = 0.5f;
        Entity entity;
        Transform transform = Transform::CreateLookAt(Vector3::CreateZero(), Vector3(1.0, 2.0, 3.0), Transform::Axis::ZPositive);
        CreateDisk(transform, radius, entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        // Test against an Aabb made by sampling many points along the edge.
        Aabb encompasingAabb = Aabb::CreateNull();
        const uint32_t numSamples = 1000;
        for (uint32_t i = 0; i < numSamples; ++i)
        {
            const float angle = (aznumeric_cast<float>(i) / aznumeric_cast<float>(numSamples)) * Constants::TwoPi;
            Vector3 offsetFromCenter = Vector3(cos(angle), sin(angle), 0.0f) * radius;
            offsetFromCenter = transform * offsetFromCenter;
            encompasingAabb.AddPoint(offsetFromCenter);
        }

        EXPECT_TRUE(aabb.GetMin().IsClose(encompasingAabb.GetMin()));
        EXPECT_TRUE(aabb.GetMax().IsClose(encompasingAabb.GetMax()));
    }

    TEST_F(DiskShapeTest, IsPointInsideAlwaysFail)
    {
        // Shapes implement the concept of inside strictly, where a point on the surface is not counted
        // as being inside. Therefore a 2D shape like disk has no inside and should always return false.

        Entity entity;
        bool inside;
        CreateUnitDiskAtOrigin(entity);

        // Check a point at the center of the disk
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3::CreateZero());
        EXPECT_FALSE(inside);

        // Check a point clearly outside the disk
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(100.f, 10.0f, 10.0f));
        EXPECT_FALSE(inside);
    }

    TEST_F(DiskShapeTest, DistanceFromPoint)
    {
        float distance = 0.0f;
        const float epsilon = 0.001f;

        const uint32_t radiiCount = 2;
        const uint32_t transformCount = 3;
        const float radii[radiiCount] = { 0.5f, 2.0f };
        Transform transforms[transformCount] =
        {
            Transform::CreateIdentity(),
            Transform::CreateLookAt(Vector3::CreateZero(), Vector3(1.0, 2.0, 3.0), Transform::Axis::ZPositive),
            Transform::CreateLookAt(Vector3::CreateZero(), Vector3(-3.0, -2.0, -1.0), Transform::Axis::ZPositive),
        };

        for (uint32_t radiusIndex = 0; radiusIndex < radiiCount; ++radiusIndex)
        {
            const float radius = radii[radiusIndex];

            for (uint32_t transformIndex = 0; transformIndex < transformCount; ++transformIndex)
            {
                const Transform& transform = transforms[transformIndex];

                Entity entity;
                CreateDisk(transform, radius, entity);

                // Check distance between disk and point at center of disk.
                CheckDistance(entity, transform, Vector3(0.0f, 0.0f, 0.0f), 0.0f);

                // Check distance between disk and point on edge of disk.
                CheckDistance(entity, transform, Vector3(0.0f, radius, 0.0f), 0.0f);

                // Check distance between disk and point 1 unit directly in front of it.
                CheckDistance(entity, transform, Vector3(0.0f, 0.0f, 1.0f), 1.0f);

                // Check distance between disk and point 1 unit directly to the side of the edge
                CheckDistance(entity, transform, Vector3(0.0f, radius + 1.0f, 0.0f), 1.0f);

                // Check distance between disk and a point 1 up and 1 to the side of it
                CheckDistance(entity, transform, Vector3(0.0f, radius + 1.0f, 1.0f), sqrt(2.0f));

                // Check distance between disk and a point 1 up and 3 to the side of it
                CheckDistance(entity, transform, Vector3(0.0f, radius + 3.0f, 1.0f), sqrt(10.0f));

                // Check distance between disk and a point 1 up and 1 to the side of it in x and y
                float a = radius + 1.0f;
                float b = radius + 1.0f;
                float distAlongPlaneFromEdge = sqrt(a * a + b * b) - radius;
                float diagonalDist = sqrt(distAlongPlaneFromEdge * distAlongPlaneFromEdge + 1.0f);
                CheckDistance(entity, transform, Vector3(radius + 1.0f, radius + 1.0f, 1.0f), diagonalDist);
                distance = -1.0f;
                ShapeComponentRequestsBus::EventResult(
                    distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, transform * Vector3(radius + 1.0f, radius + 1.0f, 1.0f));
                EXPECT_NEAR(distance, diagonalDist, epsilon);
            }
        }
    }
} // namespace UnitTest
