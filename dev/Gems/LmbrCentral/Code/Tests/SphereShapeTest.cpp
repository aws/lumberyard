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
#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Shape/SphereShapeComponent.h>
#include <Tests/TestTypes.h>

namespace Constants = AZ::Constants;
using AZ::Entity;
using AZ::Vector3;
using AZ::TransformBus;
using AzFramework::TransformComponent;
using LmbrCentral::ShapeComponentRequestsBus;
using LmbrCentral::SphereShapeConfig;
using LmbrCentral::SphereShapeComponent;
using LmbrCentral::SphereShapeComponentRequestsBus;

class SphereShapeTest
    : public UnitTest::AllocatorsFixture
{
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformShapeComponentDescriptor;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_sphereShapeComponentDescriptor;

public:

    void SetUp() override
    {
        UnitTest::AllocatorsFixture::SetUp();
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_transformShapeComponentDescriptor.reset(TransformComponent::CreateDescriptor());
        m_transformShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        m_sphereShapeComponentDescriptor.reset(SphereShapeComponent::CreateDescriptor());
        m_sphereShapeComponentDescriptor->Reflect(&(*m_serializeContext));
    }
};

void CreateUnitSphereAtPosition(const Vector3& position, Entity& entity)
{
    entity.CreateComponent<TransformComponent>();
    entity.CreateComponent<SphereShapeComponent>();
    entity.Init();
    entity.Activate();

    TransformBus::Event(entity.GetId(), &TransformBus::Events::SetWorldTranslation, position);
}

void CreateUnitSphere(Entity& entity)
{
    CreateUnitSphereAtPosition(Vector3::CreateZero(), entity);
}

// Creates a point in a sphere using spherical coordinates
// @radius The radial distance from the center of the sphere
// @verticalAngle The angle around the sphere vertically - think top to bottom
// @horizontalAngle The angle around the sphere horizontally- think left to right
// @return A point represeting the coordinates in the sphere
Vector3 CreateSpherePoint(float radius, float verticalAngle, float horizontalAngle)
{
    return Vector3(
        radius * sinf(verticalAngle) * cosf(horizontalAngle),
        radius * sinf(verticalAngle) * sinf(horizontalAngle),
        radius * cosf(verticalAngle));
}

// Tests setting a new radius on a sphere can be retrieved from the configuration
TEST_F(SphereShapeTest, SetRadiusIsPropagatedToGetConfiguration)
{
    AZ::Entity entity;
    CreateUnitSphere(entity);

    float newRadius = 123.456f;
    SphereShapeComponentRequestsBus::Event(entity.GetId(), &SphereShapeComponentRequestsBus::Events::SetRadius, newRadius);

    SphereShapeConfig config(-1.f);
    SphereShapeComponentRequestsBus::EventResult(config, entity.GetId(), &SphereShapeComponentRequestsBus::Events::GetSphereConfiguration);

    ASSERT_FLOAT_EQ(newRadius, config.m_radius);
}

// Tests a point is inside a sphere
TEST_F(SphereShapeTest, GetPointInsideSphere)
{
    AZ::Entity entity;
    Vector3 center(1.f, 2.f, 3.f);
    CreateUnitSphereAtPosition(center, entity);

    Vector3 pos;
    TransformBus::EventResult(pos, entity.GetId(), &TransformBus::Events::GetWorldTranslation);

    Vector3 point = center + CreateSpherePoint(0.49f, Constants::Pi/4.f, Constants::Pi/4.f);
    bool isInside = false;
    ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, point);

    ASSERT_TRUE(isInside);
}

// Tests a point is outside a sphere
TEST_F(SphereShapeTest, GetPointOutsideSphere)
{
    AZ::Entity entity;
    Vector3 center(1.f, 2.f, 3.f);
    CreateUnitSphereAtPosition(center, entity);

    Vector3 pos;
    TransformBus::EventResult(pos, entity.GetId(), &TransformBus::Events::GetWorldTranslation);

    Vector3 point = center + CreateSpherePoint(0.51f, Constants::Pi / 4.f, Constants::Pi / 4.f);
    bool isInside = true;
    ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &ShapeComponentRequestsBus::Events::IsPointInside, point);

    ASSERT_FALSE(isInside);
}
