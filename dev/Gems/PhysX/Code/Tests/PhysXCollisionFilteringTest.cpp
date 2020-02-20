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

#include <PhysX_precompiled.h>
#include "PhysXTestFixtures.h"
#include "PhysXTestUtil.h"
#include "PhysXTestCommon.h"
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Components/TransformComponent.h>

#include <PhysX/ConfigurationBus.h>
#include <BoxColliderComponent.h>
#include <RigidBodyComponent.h>

namespace PhysX
{
    class PhysXCollisionFilteringTest
        : public PhysXDefaultWorldTest
    {
    protected:
        const AZStd::string DefaultLayer = "Default";
        const AZStd::string TouchBendLayer = "TouchBend";
        const AZStd::string LayerA = "LayerA";
        const AZStd::string LayerB = "LayerB";
        const AZStd::string GroupA = "GroupA";
        const AZStd::string GroupB = "GroupB";
        const AZStd::string GroupNone = "None";
        const AZStd::string LeftCollider = "LeftCollider";
        const AZStd::string RightCollider = "RightCollider";
        const int FramesToUpdate = 25;

        void SetUp() override 
        {
            PhysXDefaultWorldTest::SetUp();

            // Test layers
            AZStd::vector<AZStd::string> TestCollisionLayers =
            {
                DefaultLayer,
                TouchBendLayer, // This is needed here as placeholder as collision events are disabled on this layer.
                LayerA,
                LayerB
            };

            // Test groups
            AZStd::vector<AZStd::pair<AZStd::string, AZStd::vector<AZStd::string>>> TestCollisionGroups =
            {
                {GroupA, {DefaultLayer, LayerA}}, // "A" only collides with 'LayerA'
                {GroupB, {DefaultLayer, LayerB}}, // "B" only collides with 'LayerB'
            };

            // Setup test collision layers to use in the tests
            for (int i = 0; i < TestCollisionLayers.size(); ++i)
            {
                Physics::CollisionRequestBus::Broadcast(&Physics::CollisionRequests::SetCollisionLayerName, i, TestCollisionLayers[i]);
            }

            // Setup test collision groups to use in the tests
            for (int i = 0; i < TestCollisionGroups.size(); ++i)
            {
                const auto& groupName = TestCollisionGroups[i].first;
                auto group = CreateGroupFromLayerNames(TestCollisionGroups[i].second);
                Physics::CollisionRequestBus::Broadcast(&Physics::CollisionRequests::CreateCollisionGroup, groupName, group);
            }
        }

        Physics::CollisionGroup CreateGroupFromLayerNames(const AZStd::vector<AZStd::string>& layerNames)
        {
            Physics::CollisionGroup group = Physics::CollisionGroup::None;
            for (const auto& layerName : layerNames)
            {
                Physics::CollisionLayer layer(layerName);
                group.SetLayer(layer, true);
            }
            return group;
        }
    };

    EntityPtr CreateDynamicMultiCollider(const AZStd::string& leftColliderTag, const AZStd::string& rightColliderTag)
    {
        auto entityPtr = AZStd::make_shared<AZ::Entity>("MultiCollider");
        entityPtr->CreateComponent<AzFramework::TransformComponent>();

        auto leftBoxConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>();
        auto leftColliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        leftColliderConfig->m_position = AZ::Vector3(-1.0f, 0.0f, 0.0f);
        leftColliderConfig->m_tag = leftColliderTag;
        
        auto rightBoxConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>();
        auto rightColliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        rightColliderConfig->m_position = AZ::Vector3(1.0f, 0.0f, 0.0f);
        rightColliderConfig->m_tag = rightColliderTag;

        auto leftCollider = entityPtr->CreateComponent<BoxColliderComponent>();
        leftCollider->SetShapeConfigurationList({ AZStd::make_pair(leftColliderConfig, leftBoxConfig) });

        auto rightCollider = entityPtr->CreateComponent<BoxColliderComponent>();
        rightCollider->SetShapeConfigurationList({ AZStd::make_pair(rightColliderConfig, rightBoxConfig) });

        entityPtr->CreateComponent<RigidBodyComponent>();

        entityPtr->Init();
        entityPtr->Activate();
        return AZStd::move(entityPtr);
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetColliderLayerOnStaticObject)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto ground = CreateStaticBoxEntity(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 0.5f));
        auto fallingBox = CreateBoxEntity(Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f));

        // GroupB does not collider with LayerA, no collision expected.
        SetCollisionLayer(ground, LayerA);
        SetCollisionGroup(fallingBox, GroupB);

        CollisionCallbacksListener collisionEvents(ground->GetId());

        UpdateWorld(FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetColliderLayerOnDynamicObject)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto ground = CreateStaticBoxEntity(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 0.5f));
        auto fallingBox = CreateBoxEntity(Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f));

        SetCollisionGroup(ground, GroupB);
        SetCollisionLayer(fallingBox, LayerA);

        CollisionCallbacksListener collisionEvents(ground->GetId());

        UpdateWorld(FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithGroupOnDynamicObject)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto ground = CreateStaticBoxEntity(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 0.5f));
        auto fallingBox = CreateBoxEntity(Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f));

        SetCollisionGroup(fallingBox, GroupNone);

        CollisionCallbacksListener collisionEvents(ground->GetId());

        UpdateWorld(FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithGroupOnStaticObject)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto ground = CreateStaticBoxEntity(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 0.5f));
        auto fallingBox = CreateBoxEntity(Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f));

        SetCollisionGroup(ground, GroupNone);

        CollisionCallbacksListener collisionEvents(ground->GetId());

        UpdateWorld(FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetColliderLayerOnFilteredCollider)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto leftStatic = CreateStaticBoxEntity(Vector3(-1.0f, 0.0f, -1.5f), Vector3(1.0f, 1.0f, 1.0f));
        auto rightStatic = CreateStaticBoxEntity(Vector3(1.0f, 0.0f, -1.5f), Vector3(1.0f, 1.0f, 1.0f));
        auto fallingMultiCollider = CreateDynamicMultiCollider(LeftCollider, RightCollider);

        SetCollisionGroup(leftStatic, GroupA);
        SetCollisionGroup(rightStatic, GroupB);

        SetCollisionLayer(fallingMultiCollider, LayerB, LeftCollider);
        SetCollisionLayer(fallingMultiCollider, LayerA, RightCollider);
        
        CollisionCallbacksListener collisionEvents(fallingMultiCollider->GetId());

        UpdateWorld(FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithGroupOnFilteredCollider)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto leftStatic = CreateStaticBoxEntity(Vector3(-1.0f, 0.0f, -1.5f), Vector3(1.0f, 1.0f, 1.0f));
        auto rightStatic = CreateStaticBoxEntity(Vector3(1.0f, 0.0f, -1.5f), Vector3(1.0f, 1.0f, 1.0f));
        auto fallingMultiCollider = CreateDynamicMultiCollider(LeftCollider, RightCollider);

        SetCollisionLayer(leftStatic, LayerA);
        SetCollisionLayer(rightStatic, LayerB);

        SetCollisionGroup(fallingMultiCollider, GroupB, LeftCollider);
        SetCollisionGroup(fallingMultiCollider, GroupA, RightCollider);

        CollisionCallbacksListener collisionEvents(fallingMultiCollider->GetId());

        UpdateWorld(FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithLayer)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto ground = CreateStaticBoxEntity(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));
        auto fallingBox = CreateBoxEntity(Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f));

        SetCollisionLayer(fallingBox, LayerA);
        ToggleCollisionLayer(ground, LayerA, false);

        CollisionCallbacksListener collisionEvents(fallingBox->GetId());

        UpdateWorld(FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithLayerFiltered)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto leftStatic = CreateStaticBoxEntity(Vector3(-1.0f, 0.0f, -1.5f), Vector3(1.0f, 1.0f, 1.0f));
        auto rightStatic = CreateStaticBoxEntity(Vector3(1.0f, 0.0f, -1.5f), Vector3(1.0f, 1.0f, 1.0f));
        auto fallingMultiCollider = CreateDynamicMultiCollider(LeftCollider, RightCollider);

        SetCollisionLayer(leftStatic, LayerA);
        SetCollisionLayer(rightStatic, LayerB);

        ToggleCollisionLayer(fallingMultiCollider, LayerA, false, LeftCollider);
        ToggleCollisionLayer(fallingMultiCollider, LayerB, false, RightCollider);

        CollisionCallbacksListener collisionEvents(fallingMultiCollider->GetId());

        UpdateWorld(FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestGetCollisionLayerName)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto staticBody = CreateStaticBoxEntity(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));

        SetCollisionLayer(staticBody, LayerA);

        AZStd::string collisionLayerName;
        Physics::CollisionFilteringRequestBus::EventResult(collisionLayerName, staticBody->GetId(), &Physics::CollisionFilteringRequests::GetCollisionLayerName);

        EXPECT_EQ(collisionLayerName, LayerA);
    }

    TEST_F(PhysXCollisionFilteringTest, TestGetCollisionGroupName)
    {
        using namespace Physics;
        using namespace AZ;
        using namespace TestUtils;

        auto staticBody = CreateStaticBoxEntity(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));

        SetCollisionGroup(staticBody, GroupA);

        AZStd::string collisionGroupName;
        Physics::CollisionFilteringRequestBus::EventResult(collisionGroupName, staticBody->GetId(), &Physics::CollisionFilteringRequests::GetCollisionGroupName);

        EXPECT_EQ(collisionGroupName, GroupA);
    }
}