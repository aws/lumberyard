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

#include <Shape/CylinderShapeComponent.h>

class CylinderShapeTest
    : public UnitTest::AllocatorsFixture
{
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_cylinderShapeComponentDescriptor;

public:
    void SetUp() override
    {
        UnitTest::AllocatorsFixture::SetUp();
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
        m_cylinderShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::CylinderShapeComponent::CreateDescriptor());
        m_cylinderShapeComponentDescriptor->Reflect(&(*m_serializeContext));
    }

    void TearDown() override
    {
        m_serializeContext.reset();
        UnitTest::AllocatorsFixture::TearDown();
    }
};

void SetupCylinderComponent(const AZ::Transform& transform, AZ::Entity& entity)
{
    LmbrCentral::CylinderShapeComponent* cylinderShapeComponent = entity.CreateComponent<LmbrCentral::CylinderShapeComponent>();
    entity.CreateComponent<AzFramework::TransformComponent>();

    entity.Init();
    entity.Activate();

    AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);

    LmbrCentral::CylinderShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::CylinderShapeComponentRequestsBus::Events::SetHeight, 10.0f);
    LmbrCentral::CylinderShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::CylinderShapeComponentRequestsBus::Events::SetRadius, 10.0f);
}

bool RandomPointsAreInCylinder(const AZ::RandomDistributionType distributionType)
{
    //Apply a simple transform to put the Cylinder off the origin
    AZ::Transform transform = AZ::Transform::CreateIdentity();
    transform.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationY(AZ::Constants::HalfPi));
    transform.SetTranslation(AZ::Vector3(5, 5, 5));

    AZ::Entity entity;
    SetupCylinderComponent(transform, entity);

    const AZ::u32 testPoints = 10000;
    AZ::Vector3 testPoint;
    bool testPointInVolume = false;

    //Test a bunch of random points generated with the Normal random distribution type 
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

TEST_F(CylinderShapeTest, NormalDistributionRandomPointsAreInVolume)
{
    bool allRandomPointsInVolume = false;

    allRandomPointsInVolume = RandomPointsAreInCylinder(AZ::RandomDistributionType::Normal);

    ASSERT_TRUE(allRandomPointsInVolume);
}

TEST_F(CylinderShapeTest, UniformRealDistributionRandomPointsAreInVolume)
{
    bool allRandomPointsInVolume = false;

    allRandomPointsInVolume = RandomPointsAreInCylinder(AZ::RandomDistributionType::UniformReal);

    ASSERT_TRUE(allRandomPointsInVolume);
}