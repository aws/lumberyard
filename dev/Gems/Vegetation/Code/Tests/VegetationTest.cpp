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
#include "Vegetation_precompiled.h"

#include <AzTest/AzTest.h>
#include <Tests/TestTypes.h>
#include "VegetationTest.h"
#include "VegetationMocks.h"

//////////////////////////////////////////////////////////////////////////

#include <AzCore/Component/Entity.h>
#include <Source/Components/AreaBlenderComponent.h>

#include <Source/Components/AreaBlenderComponent.h>
#include <Source/Components/BlockerComponent.h>
#include <Source/Components/DescriptorListCombinerComponent.h>
#include <Source/Components/DescriptorListComponent.h>
#include <Source/Components/DescriptorWeightSelectorComponent.h>
#include <Source/Components/DistanceBetweenFilterComponent.h>
#include <Source/Components/DistributionFilterComponent.h>
#include <Source/Components/LevelSettingsComponent.h>
#include <Source/Components/MeshBlockerComponent.h>
#include <Source/Components/PositionModifierComponent.h>
#include <Source/Components/ReferenceShapeComponent.h>
#include <Source/Components/RotationModifierComponent.h>
#include <Source/Components/ScaleModifierComponent.h>
#include <Source/Components/ShapeIntersectionFilterComponent.h>
#include <Source/Components/SlopeAlignmentModifierComponent.h>
#include <Source/Components/SpawnerComponent.h>
#include <Source/Components/StaticVegetationBlockerComponent.h>
#include <Source/Components/SurfaceAltitudeFilterComponent.h>
#include <Source/Components/SurfaceMaskDepthFilterComponent.h>
#include <Source/Components/SurfaceMaskFilterComponent.h>
#include <Source/Components/SurfaceSlopeFilterComponent.h>

namespace UnitTest
{
    struct VegetationComponentTestsBasics
        : public VegetationComponentTests
    {
        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockShapeServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockVegetationAreaServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockMeshServiceComponent::CreateDescriptor());
        }

        template <typename Component, typename MockComponent1>
        void CreateWith()
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());

            AZ::Entity entity;
            entity.CreateComponent<Component>();
            entity.CreateComponent<MockComponent1>();

            entity.Init();
            ASSERT_TRUE(entity.GetState() == AZ::Entity::ES_INIT);

            entity.Activate();
            ASSERT_TRUE(entity.GetState() == AZ::Entity::ES_ACTIVE);

            entity.Deactivate();
            ASSERT_TRUE(entity.GetState() == AZ::Entity::ES_INIT);
        }
    };

    TEST_F(VegetationComponentTestsBasics, CreateEach)
    {
        CreateWith<Vegetation::AreaBlenderComponent, MockShapeServiceComponent>();
        CreateWith<Vegetation::BlockerComponent, MockShapeServiceComponent>();
        CreateWith<Vegetation::DescriptorListCombinerComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::DescriptorListComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::DescriptorWeightSelectorComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::DistanceBetweenFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::DistributionFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::LevelSettingsComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::MeshBlockerComponent, MockMeshServiceComponent>();
        CreateWith<Vegetation::PositionModifierComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::ReferenceShapeComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::RotationModifierComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::ScaleModifierComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::ShapeIntersectionFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SlopeAlignmentModifierComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SpawnerComponent, MockShapeServiceComponent>();
        CreateWith<Vegetation::StaticVegetationBlockerComponent, MockShapeServiceComponent>();
        CreateWith<Vegetation::SurfaceAltitudeFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SurfaceMaskDepthFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SurfaceMaskFilterComponent, MockVegetationAreaServiceComponent>();
        CreateWith<Vegetation::SurfaceSlopeFilterComponent, MockVegetationAreaServiceComponent>();
    }

    TEST_F(VegetationComponentTestsBasics, LevelSettingsComponent)
    {
        MockSystemConfigurationRequestBus mockSystemConfigurationRequestBus;

        struct TestLevelSettingsComponent
            : public Vegetation::LevelSettingsComponent
        {
            const Vegetation::LevelSettingsConfig& Config() const
            {
                return m_configuration;
            }
        };

        // Provide a default configuration to the system component
        constexpr int defaultProcessTime = 7;
        Vegetation::InstanceSystemConfig defaultSystemConfig;
        defaultSystemConfig.m_maxInstanceProcessTimeMicroseconds = defaultProcessTime;

        mockSystemConfigurationRequestBus.UpdateSystemConfig(&defaultSystemConfig);
        const auto* instConfig = azrtti_cast<const Vegetation::InstanceSystemConfig*>(mockSystemConfigurationRequestBus.m_lastUpdated);
        ASSERT_TRUE(instConfig != nullptr);
        EXPECT_EQ(defaultProcessTime, instConfig->m_maxInstanceProcessTimeMicroseconds);

        {
            // Create a level settings component with a different config that should override the system component
            Vegetation::LevelSettingsComponent* component = nullptr;
            Vegetation::LevelSettingsConfig config;
            config.m_instanceSystemConfig.m_maxInstanceProcessTimeMicroseconds = 13;

            auto entity = CreateEntity(config, &component);

            const auto* instConfig = azrtti_cast<const Vegetation::InstanceSystemConfig*>(mockSystemConfigurationRequestBus.m_lastUpdated);
            ASSERT_TRUE(instConfig != nullptr);
            EXPECT_EQ(13, instConfig->m_maxInstanceProcessTimeMicroseconds);

            TestLevelSettingsComponent* tester = static_cast<TestLevelSettingsComponent*>(component);
            EXPECT_EQ(13, tester->Config().m_instanceSystemConfig.m_maxInstanceProcessTimeMicroseconds);
        }

        // Entity should be out of scope now (destroyed), so the default settings should be restored
        instConfig = azrtti_cast<const Vegetation::InstanceSystemConfig*>(mockSystemConfigurationRequestBus.m_lastUpdated);
        EXPECT_EQ(defaultProcessTime, instConfig->m_maxInstanceProcessTimeMicroseconds);
    }

    TEST_F(VegetationComponentTestsBasics, ReferenceShapeComponent)
    {
        UnitTest::MockShape testShape;

        Vegetation::ReferenceShapeConfig config;
        config.m_shapeEntityId = testShape.m_entity.GetId();

        Vegetation::ReferenceShapeComponent* component;
        auto entity = CreateEntity(config, &component);

        AZ::RandomDistributionType randomDistribution = AZ::RandomDistributionType::Normal;
        AZ::Vector3 randPos = AZ::Vector3::CreateOne();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(randPos, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, randomDistribution);
        EXPECT_EQ(AZ::Vector3::CreateZero(), randPos);

        testShape.m_aabb = AZ::Aabb::CreateFromPoint(AZ::Vector3(1.0f, 21.0f, 31.0f));
        AZ::Aabb resultAABB;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(resultAABB, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        EXPECT_EQ(testShape.m_aabb, resultAABB);

        AZ::Crc32 resultCRC = {};
        LmbrCentral::ShapeComponentRequestsBus::EventResult(resultCRC, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);
        EXPECT_EQ(AZ_CRC("TestShape", 0x856ca50c), resultCRC);

        testShape.m_localBounds = AZ::Aabb::CreateFromPoint(AZ::Vector3(1.0f, 21.0f, 31.0f));
        testShape.m_localTransform = AZ::Transform::CreateTranslation(testShape.m_localBounds.GetCenter());
        AZ::Transform resultTransform = {};
        AZ::Aabb resultBounds = {};
        LmbrCentral::ShapeComponentRequestsBus::Event(entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, resultTransform, resultBounds);
        EXPECT_EQ(testShape.m_localTransform, resultTransform);
        EXPECT_EQ(testShape.m_localBounds, resultBounds);

        testShape.m_pointInside = true;
        bool resultPointInside = false;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(resultPointInside, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, AZ::Vector3::CreateZero());
        EXPECT_EQ(testShape.m_pointInside, resultPointInside);

        testShape.m_distanceSquaredFromPoint = 456.0f;
        float resultdistanceSquaredFromPoint = 0;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(resultdistanceSquaredFromPoint, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceSquaredFromPoint, AZ::Vector3::CreateZero());
        EXPECT_EQ(testShape.m_distanceSquaredFromPoint, resultdistanceSquaredFromPoint);

        testShape.m_intersectRay = false;
        bool resultIntersectRay = false;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(resultIntersectRay, entity->GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), AZ::VectorFloat() );
        EXPECT_TRUE(testShape.m_intersectRay == resultIntersectRay);
    }
}

//////////////////////////////////////////////////////////////////////////

AZ_UNIT_TEST_HOOK();
