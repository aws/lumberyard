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
#include <AzCore/UnitTest/TestTypes.h>

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

    using CylinderParams = std::tuple<AZ::Transform, float, float>;
    using BoundingBoxResult = std::tuple<Vector3, Vector3>;
    using BoundingBoxParams = std::tuple<CylinderParams, BoundingBoxResult>;
    using IsPointInsideParams = std::tuple<CylinderParams, Vector3, bool>;
    using RayParams = std::tuple<Vector3, Vector3>;
    using RayIntersectResult = std::tuple<bool, float, float>;
    using RayIntersectParams = std::tuple<RayParams, CylinderParams, RayIntersectResult>;
    using DistanceResultParams = std::tuple<float, float>;
    using DistanceFromPointParams = std::tuple<CylinderParams, Vector3, DistanceResultParams>;

    class CylinderShapeRayIntersectTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<RayIntersectParams>
    {
    public:
        static const std::vector<RayIntersectParams> ShouldPass;
        static const std::vector<RayIntersectParams> ShouldFail;
    };

    class CylinderShapeAABBTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<BoundingBoxParams>
    {
    public:
        static const std::vector<BoundingBoxParams> ShouldPass;
    };

    class CylinderShapeTransformAndLocalBoundsTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<BoundingBoxParams>
    {
    public:
        static const std::vector<BoundingBoxParams> ShouldPass;
    };

    class CylinderShapeIsPointInsideTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<IsPointInsideParams>
    {
    public:
        static const std::vector<IsPointInsideParams> ShouldPass;
        static const std::vector<IsPointInsideParams> ShouldFail;
    }; 

    class CylinderShapeDistanceFromPointTest
        : public CylinderShapeTest
        , public testing::WithParamInterface<DistanceFromPointParams>
    {
    public:
        static const std::vector<DistanceFromPointParams> ShouldPass;
    };

    const std::vector<RayIntersectParams> CylinderShapeRayIntersectTest::ShouldPass = {
        // Test case 0
        {   // Ray: src, dir
            { Vector3(0.0f, 5.0f, 5.0f), Vector3(0.0f, -1.0f, 0.0f) },
             // Cylinder: transform, radius, height
            { Transform::CreateTranslation(
             Vector3(0.0f, 0.0f, 5.0f)),
              0.5f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 4.5f, 1e-4f }   },
        // Test case 1
        {   // Ray: src, dir
            { Vector3(-10.0f, -20.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi), Vector3(-10.0f, -10.0f, 0.0f)),
                1.0f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 7.5f, 1e-2f }   },
        // Test case 2
        {// Ray: src, dir
            { Vector3(-10.0f, -10.0f, -10.0f), Vector3(0.0f, 0.0f, 1.0f) },
            // Cylinder: transform, radius, height
            { Transform::CreateFromQuaternionAndTranslation(
                    Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi), Vector3(-10.0f, -10.0f, 0.0f)),
                    1.0f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 9.0f, 1e-2f }   },
        // Test case 3
        {
            // Ray: src, dir
            { Vector3(-9.0f, -14.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(-14.0f, -14.0f, -1.0f)) *
                    Transform::CreateRotationY(Constants::HalfPi) *
                    Transform::CreateRotationZ(Constants::HalfPi) *
                    Transform::CreateScale(Vector3(4.0f)),
                    1.0f, 1.25f },
            // Result: hit, distance, epsilon
            { true, 2.5f, 1e-2f }
        },
        // Test case 4
        {   // Ray: src, dir
            { Vector3(0.0f, 5.0f, 5.0f), Vector3(0.0f, -1.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { Transform::CreateTranslation(
              Vector3(0.0f, 0.0f, 5.0f)),
              0.0f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 0.0f, 1e-4f }   },
        // Test case 5
        {   // Ray: src, dir
            { Vector3(0.0f, 5.0f, 5.0f), Vector3(0.0f, -1.0f, 0.0f) },
            // Cylinder: transform, radius, height
              { Transform::CreateTranslation(
              Vector3(0.0f, 0.0f, 5.0f)),
              0.0f, 5.0f },
            // Result: hit, distance, epsilon
            { true, 0.0f, 1e-4f }  },
        // Test case 6
        {   // Ray: src, dir
            { Vector3(0.0f, 5.0f, 5.0f), Vector3(0.0f, -1.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { Transform::CreateTranslation(
              Vector3(0.0f, 0.0f, 5.0f)),
              0.0f, 0.0f },
            // Result: hit, distance, epsilon
            { true, 0.0f, 1e-4f }   }
    };

    const std::vector<RayIntersectParams> CylinderShapeRayIntersectTest::ShouldFail = {
        // Test case 0
        {   // Ray: src, dir
            { Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
            // Cylinder: transform, radius, height
            { Transform::CreateTranslation(
              Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f },
            // Result: hit, distance, epsilon
            { false, 0.0f, 0.0f }   }
    }; 
        
    const std::vector<BoundingBoxParams> CylinderShapeAABBTest::ShouldPass = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(
              Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f },
            // AABB: min, max
            { Vector3(-5.0f, -15.0f, -0.5f), Vector3(5.0f, -5.0f, 0.5f) }   },
        // Test case 1
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(-10.0f, -10.0f, 0.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::QuarterPi),
            1.0f, 5.0f },
            // AABB: min, max
            { Vector3(-12.4748f, -12.4748f, -1.0f), Vector3(-7.52512f, -7.52512f, 1.0f) }   },
        // Test case 2
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(-10.0f, -10.0f, 10.0f)) *
            Transform::CreateScale(Vector3(3.5f)),
            1.0f, 5.0f },
            // AABB: min, max
            { Vector3(-13.5f, -13.5f, 1.25f), Vector3(-6.5f, -6.5f, 18.75f) }   },
        // Test case 3
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(0.0f, 0.0f, 0.0f)),
            0.0f, 1.0f },
            // AABB: min, max
            { Vector3(0.0f, 0.0f, -0.5f), Vector3(0.0f, 0.0f,-0.5f) }    },
        // Test case 4
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(0.0f, 0.0f, 0.0f)),
            1.0f, 0.0f },
            // AABB: min, max
            { Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f) }    },
        // Test case 5
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(0.0f, 0.0f, 0.0f)),
            0.0f, 0.0f },
            // AABB: min, max
            { Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f) }    }
    };

    const std::vector<BoundingBoxParams> CylinderShapeTransformAndLocalBoundsTest::ShouldPass = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            { Transform::CreateIdentity(),
            5.0f, 1.0f },
            // Local bounds: min, max
            { Vector3(-5.0f, -5.0f, -0.5f), Vector3(5.0f, 5.0f, 0.5f) } },
        // Test case 1
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(-10.0f, -10.0f, 10.0f)) * Transform::CreateScale(Vector3(3.5f)),
            5.0f, 5.0f },
            // Local bounds: min, max
            { Vector3(-5.0f, -5.0f, -2.5f), Vector3(5.0f, 5.0f, 2.5f) } },
        // Test case 2
        {   // Cylinder: transform, radius, height
            { Transform::CreateIdentity(),
            0.0f, 5.0f },
            // Local bounds: min, max
            { Vector3(0.0f, 0.0f, -2.5f), Vector3(0.0f, 0.0f, 2.5f) }   },
        // Test case 3
        {   // Cylinder: transform, radius, height
            { Transform::CreateIdentity(),
            5.0f, 0.0f },
            // Local bounds: min, max
            { Vector3(-5.0f, -5.0f, -0.0f), Vector3(5.0f, 5.0f, 0.0f) } },
        // Test case 4
        {   // Cylinder: transform, radius, height  
            { Transform::CreateIdentity(),
            0.0f, 0.0f },
            // Local bounds: min, max
            { Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f) }    },
    };

    const std::vector<IsPointInsideParams> CylinderShapeIsPointInsideTest::ShouldPass = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            {Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateScale(Vector3(2.5f, 1.0f, 1.0f)), // test max scale
            0.5f, 2.0f},
            // Point
            Vector3(27.0f, 28.5f, 40.0f),
            // Result
            true    },
        // Test case 1
        {   // Cylinder: transform, radius, height
            {Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(0.5f)),
            0.5f, 2.0f},
            // Point
            Vector3(27.0f, 28.155f, 37.82f),
            // Result
            true    }
    };

    const std::vector<IsPointInsideParams> CylinderShapeIsPointInsideTest::ShouldFail = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            {Transform::CreateTranslation(Vector3(0, 0.0f, 0.0f)),
            0.0f, 1.0f},
            // Point
            Vector3(0.0f, 0.0f, 0.0f),
            // Result
            false   },
        // Test case 1
        {   // Cylinder: transform, radius, height
            {Transform::CreateTranslation(Vector3(0, 0.0f, 0.0f)),
            1.0f, 0.0f},
            // Point
            Vector3(0.0f, 0.0f, 0.0f),
            // Result
            false   },
        // Test case 2
        {   // Cylinder: transform, radius, height
            {Transform::CreateTranslation(Vector3(0, 0.0f, 0.0f)),
            0.0f, 0.0f},
            // Point
            Vector3(0.0f, 0.0f, 0.0f),
            // Result
            false   }
    }; 

    const std::vector<DistanceFromPointParams> CylinderShapeDistanceFromPointTest::ShouldPass = {
        // Test case 0
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(2.0f)),
            0.5f, 4.0f },
            // Point
            Vector3(27.0f, 28.0f, 41.0f),
            // Result: distance, epsilon
            { 2.0f, 1e-2f } },
        // Test case 1
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(27.0f, 28.0f, 38.0f)) *
            Transform::CreateRotationX(Constants::HalfPi) *
            Transform::CreateRotationY(Constants::QuarterPi) *
            Transform::CreateScale(Vector3(2.0f)),
            0.5f, 4.0f },
            // Point
            Vector3(22.757f, 32.243f, 38.0f),
            // Result: distance, epsilon
            { 2.0f, 1e-2f } },
        // Test case 2
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(0.0f, 0.0f, 0.0f)),
            0.0f, 1.0f },
            // Point
            Vector3(0.0f, 5.0f, 0.0f),
            // Result: distance, epsilon
            { 5.0f, 1e-1f } },
        // Test case 3
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(0.0f, 0.0f, 0.0f)),
            1.0f, 0.0f },
            // Point
            Vector3(0.0f, 5.0f, 0.0f),
            // Result: distance, epsilon
            { 5.0f, 1e-2f } },
        // Test case 4
        {   // Cylinder: transform, radius, height
            { Transform::CreateTranslation(Vector3(0.0f, 0.0f, 0.0f)),
            0.0f, 0.0f },
            // Point
            Vector3(0.0f, 5.0f, 0.0f),
            // Result: distance, epsilon
            { 5.0f, 1e-2f } }
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

    TEST_P(CylinderShapeRayIntersectTest, GetRayIntersectCylinder)
    {      
        const auto& [ray, cylinder, result] = GetParam();
        const auto& [src, dir] = ray;
        const auto& [transform, radius, height] = cylinder;
        const auto& [expectedHit, expectedDistance, epsilon] = result;

        Entity entity;
        CreateCylinder(transform, radius, height, entity);

        Transform::CreateFromQuaternionAndTranslation(
            Quaternion::CreateIdentity(), Vector3(0.0f, 0.0f, 5.0f));
        
        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            src, dir, distance);
        
        EXPECT_EQ(rayHit, expectedHit);

        if (expectedHit)
        {
            EXPECT_NEAR(distance, expectedDistance, epsilon);
        }
    }

    INSTANTIATE_TEST_CASE_P(ValidIntersections,
        CylinderShapeRayIntersectTest,
        ::testing::ValuesIn(CylinderShapeRayIntersectTest::ShouldPass)
    );

    INSTANTIATE_TEST_CASE_P(InvalidIntersections,
        CylinderShapeRayIntersectTest,
        ::testing::ValuesIn(CylinderShapeRayIntersectTest::ShouldFail)
    );
    
    TEST_P(CylinderShapeAABBTest, GetAabb)
    {
        const auto& [cylinder, AABB] = GetParam();
        const auto& [transform, radius, height] = cylinder;
        const auto& [minExtent, maxExtent] = AABB;

        Entity entity;
        CreateCylinder(transform, radius, height, entity);
    
        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);
    
        EXPECT_TRUE(aabb.GetMin().IsClose(minExtent));
        EXPECT_TRUE(aabb.GetMax().IsClose(maxExtent));
    }

    INSTANTIATE_TEST_CASE_P(AABB,
        CylinderShapeAABBTest,
        ::testing::ValuesIn(CylinderShapeAABBTest::ShouldPass)
    );

    TEST_P(CylinderShapeTransformAndLocalBoundsTest, GetTransformAndLocalBounds)
    {
        const auto& [cylinder, boundingBox] = GetParam();
        const auto& [transform, radius, height] = cylinder;
        const auto& [minExtent, maxExtent] = boundingBox;

        Entity entity;
        CreateCylinder(transform, radius, height, entity);

        Transform transformOut;
        Aabb aabb;
        ShapeComponentRequestsBus::Event(entity.GetId(), &ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transform));
        EXPECT_TRUE(aabb.GetMin().IsClose(minExtent));
        EXPECT_TRUE(aabb.GetMax().IsClose(maxExtent));
    }

    INSTANTIATE_TEST_CASE_P(TransformAndLocalBounds,
        CylinderShapeTransformAndLocalBoundsTest,
        ::testing::ValuesIn(CylinderShapeTransformAndLocalBoundsTest::ShouldPass)
    );

    // point inside scaled
    TEST_P(CylinderShapeIsPointInsideTest, IsPointInside)
    {
        const auto& [cylinder, point, expectedInside] = GetParam();
        const auto& [transform, radius, height] = cylinder;

        Entity entity;
        CreateCylinder(transform, radius, height, entity);

        bool inside;
        ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, point);

        EXPECT_EQ(inside, expectedInside);
    }

    INSTANTIATE_TEST_CASE_P(ValidIsPointInside,
        CylinderShapeIsPointInsideTest,
        ::testing::ValuesIn(CylinderShapeIsPointInsideTest::ShouldPass)
    );


    INSTANTIATE_TEST_CASE_P(InvalidIsPointInside,
        CylinderShapeIsPointInsideTest,
        ::testing::ValuesIn(CylinderShapeIsPointInsideTest::ShouldFail)
    );

    // distance scaled - along length
    TEST_P(CylinderShapeDistanceFromPointTest, DistanceFromPoint)
    {
        const auto& [cylinder, point, result] = GetParam();
        const auto& [transform, radius, height] = cylinder;
        const auto& [expectedDistance, epsilon] = result;

        Entity entity;
        CreateCylinder(transform, radius, height, entity);

        float distance;
        ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, point);

        EXPECT_NEAR(distance, expectedDistance, epsilon);
    }

    INSTANTIATE_TEST_CASE_P(ValidIsDistanceFromPoint,
        CylinderShapeDistanceFromPointTest,
        ::testing::ValuesIn(CylinderShapeDistanceFromPointTest::ShouldPass)
    );
}
