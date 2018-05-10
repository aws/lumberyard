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
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Shape/PolygonPrismShapeComponent.h>
#include <Tests/TestTypes.h>

using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    class PolygonPrismShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_polygonPrismShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_polygonPrismShapeComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(PolygonPrismShapeComponent::CreateDescriptor());
            m_polygonPrismShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreatePolygonPrism(
        const Transform& transform, const float height, const AZStd::vector<Vector2>& vertices, Entity& entity)
    {
        entity.CreateComponent<PolygonPrismShapeComponent>();
        entity.CreateComponent<TransformComponent>();

        entity.Init();
        entity.Activate();

        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, transform);

        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetHeight, height);
        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetVertices, vertices);
    }


    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_IsPointInside)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateIdentity(), 10.0f,
            AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(0.0f, 10.0f),
                Vector2(10.0f, 10.0f),
                Vector2(10.0f, 0.0f)
            }), 
            entity);

        // verify point inside returns true
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(5.0f, 5.0f, 5.0f));
            EXPECT_TRUE(inside);
        }

        // verify point outside return false
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(5.0f, 5.0f, 20.0f));
            EXPECT_TRUE(!inside);
        }

        // verify points at polygon edge return true
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(0.0f, 0.0f, 0.0f));
            EXPECT_TRUE(inside);
        }

        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(0.0f, 10.0f, 0.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(10.0f, 10.0f, 0.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(10.0f, 0.0f, 0.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(5.0f, 10.0f, 0.0f));
            EXPECT_TRUE(inside);
        }

        // verify point lies just inside
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(5.0f, 9.5f, 0.0f));
            EXPECT_TRUE(inside);
        }
        // verify point lies just outside
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(5.0f, 10.1f, 0.0f));
            EXPECT_FALSE(inside);
        }

        // note: the shape and positions/transforms were defined in the editor and replicated here - this
        // gave a good way to create various test cases and replicate them here
        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, Transform::CreateFromMatrix3x3AndTranslation(Matrix3x3::CreateIdentity(), Vector3(497.0f, 595.0f, 32.0f)));
        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 9.0f),
                Vector2(6.5f, 6.5f),
                Vector2(9.0f, 0.0f),
                Vector2(6.5f, -6.5f),
                Vector2(0.0f, -9.0f),
                Vector2(-6.5f, -6.5f),
                Vector2(-9.0f, 0.0f),
                Vector2(-6.5f, 6.5f)
            }
        ));

        // verify point inside aabb but not inside polygon returns false
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(488.62f, 588.88f, 32.0f));
            EXPECT_FALSE(inside);
        }

        // verify point inside aabb and inside polygon returns true - when intersecting two vertices
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(496.62f, 595.0f, 32.0f));
            EXPECT_TRUE(inside);
        }

        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(10.0f, 0.0f),
                Vector2(5.0f, 10.0f)
            }
        ));

        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(496.62f, 595.0f, 32.0f));
            EXPECT_FALSE(inside);
        }

        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 10.0f),
                Vector2(10.0f, 10.0f),
                Vector2(5.0f, 0.0f)
            }
        ));

        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(496.62f, 595.0f, 32.0f));
            EXPECT_FALSE(inside);
        }

        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(10.0f, 0.0f),
                Vector2(5.0f, -10.0f)
            }
        ));

        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(496.62f, 595.0f, 32.0f));
            EXPECT_FALSE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(502.0f, 585.0f, 32.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(499.62f, 595.0f, 32.0f));
            EXPECT_TRUE(inside);
        }

        // U shape
        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, Transform::CreateIdentity());
        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(0.0f, 10.0f),
                Vector2(5.0f, 10.0f),
                Vector2(5.0f, 5.0f),
                Vector2(10.0f, 5.0f),
                Vector2(10.0f, 10.0f),
                Vector2(15.0f, 15.0f),
                Vector2(15.0f, 0.0f),
            }
        ));

        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(7.5f, 7.5f, 0.0f));
            EXPECT_FALSE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(12.5f, 7.5f, 0.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(12.5f, 7.5f, 12.0f));
            EXPECT_FALSE(inside);
        }

        // check polygon prism with rotation
        TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTM, Transform::CreateRotationX(DegToRad(45.0f)));
        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetHeight, 10.0f);
        PolygonPrismShapeComponentRequestBus::Event(entity.GetId(), &PolygonPrismShapeComponentRequests::SetVertices,
            AZStd::vector<Vector2>(
            {
                Vector2(-5.0f, -5.0f),
                Vector2(-5.0f, 5.0f),
                Vector2(5.0f, 5.0f),
                Vector2(5.0f, -5.0f)
            }
        ));

        // check below
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(2.0f, 3.5f, 2.0f));
            EXPECT_FALSE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(2.0f, -8.0f, -2.0f));
            EXPECT_FALSE(inside);
        }
        // check above
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(2.0f, -8.0f, 8.0f));
            EXPECT_FALSE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(2.0f, 2.0f, 8.0f));
            EXPECT_FALSE(inside);
        }
        // check inside
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(2.0f, -3.0f, 8.0f));
            EXPECT_TRUE(inside);
        }
        {
            bool inside;
            ShapeComponentRequestsBus::EventResult(inside, entity.GetId(), &ShapeComponentRequests::IsPointInside, Vector3(2.0f, -3.0f, -2.0f));
            EXPECT_TRUE(inside);
        }
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_DistanceFromPoint)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateIdentity(), 
            10.0f, AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(0.0f, 10.0f),
                Vector2(10.0f, 10.0f),
                Vector2(10.0f, 0.0f)
            }), 
            entity);

        {
            float distance;
            ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(15.0f, 5.0f, 0.0f));
            EXPECT_TRUE(IsCloseMag(distance, 5.0f));
        }

        {
            float distance;
            ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(5.0f, 5.0f, 5.0f));
            EXPECT_TRUE(IsCloseMag(distance, 0.0f));
        }

        {
            float distance;
            ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(5.0f, 5.0f, 0.0f));
            EXPECT_TRUE(IsCloseMag(distance, 0.0f));
        }

        {
            float distance;
            ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(1.0f, 1.0f, -1.0f));
            EXPECT_TRUE(IsCloseMag(distance, 1.0f));
        }

        {
            float distance;
            ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(10.0f, 10.0f, 10.0f));
            EXPECT_TRUE(IsCloseMag(distance, 0.0f));
        }

        {
            float distance;
            ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(5.0f, 5.0f, 15.0f));
            EXPECT_TRUE(IsCloseMag(distance, 5.0f));
        }

        {
            float distance;
            ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(5.0f, 5.0f, -10.0f));
            EXPECT_TRUE(IsCloseMag(distance, 10.0f));
        }

        {
            float distance;
            ShapeComponentRequestsBus::EventResult(distance, entity.GetId(), &ShapeComponentRequests::DistanceFromPoint, Vector3(5.0f, 13.0f, 14.0f));
            EXPECT_TRUE(IsCloseMag(distance, 5.0f));
        }
    }

    // ccw
    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismSuccess1)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateIdentity(),
            10.0f, AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(0.0f, 10.0f),
                Vector2(10.0f, 10.0f),
                Vector2(10.0f, 0.0f)
            }),
            entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(5.0f, 5.0f, 15.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 5.0f, 1e-2f);
    }

    // cw
    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismSuccess2)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateIdentity(),
            10.0f, AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(10.0f, 0.0f),
                Vector2(10.0f, 10.0f),
                Vector2(0.0f, 10.0f),
            }),
            entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(5.0f, 5.0f, 15.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 5.0f, 1e-2f);
    }

    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismSuccess3)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::HalfPi),
                Vector3(2.0f, 0.0f, 5.0f)),
            2.0f, 
            AZStd::vector<Vector2>(
            {
                Vector2(1.0f, 0.0f),
                Vector2(-1.0f, -2.0f),
                Vector2(-4.0f, -2.0f),
                Vector2(-6.0f, 0.0f),
                Vector2(-4.0f, 2.0f),
                Vector2(-1.0f, 2.0f)
            }),
            entity);

        {
            bool rayHit = false;
            VectorFloat distance;
            ShapeComponentRequestsBus::EventResult(
                rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
                Vector3(0.0f, 5.0f, 5.0f), Vector3(0.0f, -1.0f, 0.0f), distance);

            EXPECT_TRUE(rayHit);
            EXPECT_NEAR(distance, 5.0f, 1e-2f);
        }

        {
            bool rayHit = false;
            VectorFloat distance;
            ShapeComponentRequestsBus::EventResult(
                rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
                Vector3(0.0f, -1.0f, 9.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

            EXPECT_TRUE(rayHit);
            EXPECT_NEAR(distance, 2.0f, 1e-2f);
        }
    }

    // transformed scaled
    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismSuccess4)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateTranslation(Vector3(5.0f, 15.0f, 40.0f)) *
            Transform::CreateScale(Vector3(3.0f)), 2.0f,
            AZStd::vector<Vector2>(
            {
                Vector2(-2.0f, -2.0f),
                Vector2(2.0f, -2.0f),
                Vector2(2.0f, 2.0f),
                Vector2(-2.0f, 2.0f)
            }),
            entity);

        {
            bool rayHit = false;
            VectorFloat distance;
            ShapeComponentRequestsBus::EventResult(
                rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
                Vector3(5.0f, 15.0f, 51.0f), Vector3(0.0f, 0.0f, -1.0f), distance);

            EXPECT_TRUE(rayHit);
            EXPECT_NEAR(distance, 5.0f, 1e-2f);
        }

        {
            bool rayHit = false;
            VectorFloat distance;
            ShapeComponentRequestsBus::EventResult(
                rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
                Vector3(15.0f, 15.0f, 43.0f), Vector3(-1.0f, 0.0f, 0.0f), distance);

            EXPECT_TRUE(rayHit);
            EXPECT_NEAR(distance, 4.0f, 1e-2f);
        }
    }

    TEST_F(PolygonPrismShapeTest, GetRayIntersectPolygonPrismFailure)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateIdentity(),
            1.0f, AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(0.0f, 10.0f),
                Vector2(10.0f, 10.0f),
                Vector2(10.0f, 0.0f)
            }),
            entity);

        bool rayHit = false;
        VectorFloat distance;
        ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &ShapeComponentRequests::IntersectRay,
            Vector3(-3.0f, -1.0f, 2.0f), Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_GetAabb1)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateTranslation(Vector3(5.0f, 5.0f, 5.0f)), 10.0f,
            AZStd::vector<Vector2>(
            {
                Vector2(0.0f, 0.0f),
                Vector2(0.0f, 10.0f),
                Vector2(10.0f, 10.0f),
                Vector2(10.0f, 0.0f)
            }),
            entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(5.0f, 5.0f, 5.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(15.0f, 15.0f, 15.0f)));
    }

    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_GetAabb2)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateFromAxisAngle(Vector3::CreateAxisX(), Constants::QuarterPi) *
                Quaternion::CreateFromAxisAngle(Vector3::CreateAxisY(), Constants::QuarterPi),
                Vector3(5.0f, 15.0f, 20.0f)), 5.0f,
                AZStd::vector<Vector2>(
                {
                    Vector2(-2.0f, -2.0f),
                    Vector2(2.0f, -2.0f),
                    Vector2(2.0f, 2.0f),
                    Vector2(-2.0f, 2.0f)
                }),
            entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(3.5857f, 10.08578f, 17.5857f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(9.9497f, 17.41413f, 24.9142f)));
    }

    // transformed scaled
    TEST_F(PolygonPrismShapeTest, PolygonShapeComponent_GetAabb3)
    {
        Entity entity;
        CreatePolygonPrism(
            Transform::CreateTranslation(Vector3(5.0f, 15.0f, 40.0f)) *
            Transform::CreateScale(Vector3(3.0f)), 1.5f,
            AZStd::vector<Vector2>(
            {
                Vector2(-2.0f, -2.0f),
                Vector2(2.0f, -2.0f),
                Vector2(2.0f, 2.0f),
                Vector2(-2.0f, 2.0f)
            }),
            entity);

        Aabb aabb;
        ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-1.0f, 9.0f, 40.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(11.0f, 21.0f, 44.5f)));
    }
}