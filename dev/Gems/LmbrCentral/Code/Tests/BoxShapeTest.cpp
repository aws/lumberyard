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
#include <AzCore/Math/Random.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Tests/TestTypes.h>

#include <Shape/BoxShapeComponent.h>

class BoxShapeTest
    : public UnitTest::AllocatorsFixture
{
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_boxShapeComponentDescriptor;

public:
    void SetUp() override
    {
        UnitTest::AllocatorsFixture::SetUp();
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
        m_boxShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::BoxShapeComponent::CreateDescriptor());
        m_boxShapeComponentDescriptor->Reflect(&(*m_serializeContext));
    }

    void TearDown() override
    {
        m_serializeContext.reset();
        UnitTest::AllocatorsFixture::TearDown();
    }
};

void SetupBoxComponent(const AZ::Transform& transform, AZ::Entity& entity)
{
    LmbrCentral::BoxShapeComponent* boxShapeComponent = entity.CreateComponent<LmbrCentral::BoxShapeComponent>();
    entity.CreateComponent<AzFramework::TransformComponent>();

    entity.Init();
    entity.Activate();

    //Use identity transform so that we test points against an AABB
    AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);

    AZ::Vector3 boxDimensions(10.0f, 10.0f, 10.0f);
    LmbrCentral::BoxShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::BoxShapeComponentRequestsBus::Events::SetBoxDimensions, boxDimensions);
}

bool RandomPointsAreInBox(const AZ::Entity& entity, const AZ::Transform& entityTransform, const AZ::RandomDistributionType distributionType)
{
    const AZ::u32 testPoints = 10000;
    AZ::Vector3 testPoint;
    bool testPointInVolume = false;

    //Test a bunch of random points generated with a random distribution type 
    //They should all end up in the volume
    for (AZ::u32 i = 0; i < testPoints; ++i)
    {
        LmbrCentral::ShapeComponentRequestsBus::EventResult(testPoint, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, distributionType);

        LmbrCentral::ShapeComponentRequestsBus::EventResult(testPointInVolume, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, testPoint);

        if (!testPointInVolume)
        {
            return false;
        }
    }

    return true;
}

TEST_F(BoxShapeTest, NormalDistributionRandomPointsAreInAABB)
{
    bool allRandomPointsInVolume = false;

    //Don't rotate transform so that this is an AABB
    AZ::Transform transform = AZ::Transform::CreateIdentity();
    transform.SetPosition(AZ::Vector3(5.0f, 5.0f, 5.0f));

    AZ::Entity entity;
    SetupBoxComponent(transform, entity);

    allRandomPointsInVolume = RandomPointsAreInBox(entity, transform, AZ::RandomDistributionType::Normal);

    ASSERT_TRUE(allRandomPointsInVolume);
}

TEST_F(BoxShapeTest, UniformRealDistributionRandomPointsAreInAABB)
{
    bool allRandomPointsInVolume = false;

    //Don't rotate transform so that this is an AABB
    AZ::Transform transform = AZ::Transform::CreateIdentity();
    transform.SetPosition(AZ::Vector3(5.0f, 5.0f, 5.0f));

    AZ::Entity entity;
    SetupBoxComponent(transform, entity);

    allRandomPointsInVolume = RandomPointsAreInBox(entity, transform, AZ::RandomDistributionType::UniformReal);

    ASSERT_TRUE(allRandomPointsInVolume);
}

TEST_F(BoxShapeTest, NormalDistributionRandomPointsAreInOBB)
{
    bool allRandomPointsInVolume = false;

    //Rotate to end up with an OBB
    float angle = AZ::Constants::HalfPi * 0.5f;

    AZ::Quaternion rotation = AZ::Quaternion::CreateRotationY(angle);
    AZ::Transform transform = AZ::Transform::CreateFromQuaternion(rotation);

    transform.SetPosition(AZ::Vector3(5.0f, 5.0f, 5.0f));

    AZ::Entity entity;
    SetupBoxComponent(transform, entity);

    allRandomPointsInVolume = RandomPointsAreInBox(entity, transform, AZ::RandomDistributionType::Normal);

    ASSERT_TRUE(allRandomPointsInVolume);
}

TEST_F(BoxShapeTest, UniformRealDistributionRandomPointsAreInOBB)
{
    bool allRandomPointsInVolume = false;

    //Rotate to end up with an OBB
    float angle = AZ::Constants::HalfPi * 0.5f;

    AZ::Quaternion rotation = AZ::Quaternion::CreateRotationY(angle);
    AZ::Transform transform = AZ::Transform::CreateFromQuaternion(rotation);

    transform.SetPosition(AZ::Vector3(5.0f, 5.0f, 5.0f));

    AZ::Entity entity;
    SetupBoxComponent(transform, entity);

    allRandomPointsInVolume = RandomPointsAreInBox(entity, transform, AZ::RandomDistributionType::UniformReal);

    ASSERT_TRUE(allRandomPointsInVolume);
}