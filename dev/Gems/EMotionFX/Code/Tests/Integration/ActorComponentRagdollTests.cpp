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

#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/World.h>
#include <Integration/Components/ActorComponent.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Node.h>
#include <Tests/Integration/EntityComponentFixture.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    class TestRagdoll
        : public Physics::Ragdoll
    {
    public:
        AZ_RTTI(TestRagdoll, "{A8FCEA6D-DC28-4D7D-9284-D98AD771E944}", Physics::Ragdoll)

        MOCK_METHOD1(EnableSimulation, void(const Physics::RagdollState&));
        MOCK_METHOD0(DisableSimulation, void());

        MOCK_METHOD0(IsSimulated, bool());

        MOCK_CONST_METHOD1(GetState, void(Physics::RagdollState&));
        MOCK_METHOD1(SetState, void(const Physics::RagdollState&));

        MOCK_CONST_METHOD2(GetNodeState, void(size_t, Physics::RagdollNodeState&));
        MOCK_METHOD2(SetNodeState, void(size_t, const Physics::RagdollNodeState&));

        MOCK_CONST_METHOD1(GetNode, Physics::RagdollNode* (size_t));
        MOCK_CONST_METHOD0(GetNumNodes, size_t());

        MOCK_CONST_METHOD0(GetWorldId, AZ::Crc32());

        // WorldBody
        MOCK_CONST_METHOD0(GetEntityId, AZ::EntityId());
        MOCK_CONST_METHOD0(GetWorld, Physics::World*());

        MOCK_CONST_METHOD0(GetTransform, AZ::Transform());
        MOCK_METHOD1(SetTransform, void(const AZ::Transform&));

        MOCK_CONST_METHOD0(GetPosition, AZ::Vector3());
        MOCK_CONST_METHOD0(GetOrientation, AZ::Quaternion());

        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb());

        void RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) const override {}

        MOCK_CONST_METHOD0(GetNativeType, AZ::Crc32());
        MOCK_CONST_METHOD0(GetNativePointer, void*());

        MOCK_METHOD1(AddToWorld, void(Physics::World&));
        MOCK_METHOD1(RemoveFromWorld, void(Physics::World&));
    };

    class TestRagdollPhysicsRequestHandler
        : public AzFramework::RagdollPhysicsRequestBus::Handler
    {
    public:
        TestRagdollPhysicsRequestHandler(Physics::Ragdoll* ragdoll, const AZ::EntityId& entityId)
        {
            m_ragdoll = ragdoll;
            AzFramework::RagdollPhysicsRequestBus::Handler::BusConnect(entityId);
        }

        ~TestRagdollPhysicsRequestHandler()
        {
            AzFramework::RagdollPhysicsRequestBus::Handler::BusDisconnect();
        }

        void EnableSimulation(const Physics::RagdollState& initialState) override {}
        void DisableSimulation() override {}

        Physics::Ragdoll* GetRagdoll() override { return m_ragdoll; }

        void GetState(Physics::RagdollState& ragdollState) const override {}
        void SetState(const Physics::RagdollState& ragdollState) override {}

        void GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const override {}
        void SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState) override {}

        Physics::RagdollNode* GetNode(size_t nodeIndex) const override { return nullptr; }

        void EnterRagdoll() override {}
        void ExitRagdoll() override {}

    private:
        Physics::Ragdoll* m_ragdoll = nullptr;
    };

    TEST_F(EntityComponentFixture, ActorComponent_ActivateRagdoll)
    {
        AZ::EntityId entityId(740216387);
        AZ::Crc32 worldId(174592);

        TestRagdoll testRagdoll;
        TestRagdollPhysicsRequestHandler ragdollPhysicsRequestHandler(&testRagdoll, entityId);

        EXPECT_CALL(testRagdoll, GetState(::testing::_)).Times(::testing::AnyNumber());
        EXPECT_CALL(testRagdoll, GetNumNodes()).WillRepeatedly(::testing::Return(1));
        EXPECT_CALL(testRagdoll, IsSimulated()).WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(testRagdoll, GetWorldId()).WillRepeatedly(::testing::Return(worldId));
        EXPECT_CALL(testRagdoll, GetEntityId()).WillRepeatedly(::testing::Return(entityId));
        EXPECT_CALL(testRagdoll, GetPosition()).WillRepeatedly(::testing::Return(AZ::Vector3::CreateZero()));
        EXPECT_CALL(testRagdoll, GetOrientation()).WillRepeatedly(::testing::Return(AZ::Quaternion::CreateIdentity()));

        auto gameEntity = AZStd::make_unique<AZ::Entity>();
        gameEntity->SetId(entityId);

        auto transformComponent = gameEntity->CreateComponent<AzFramework::TransformComponent>();
        auto actorComponent = gameEntity->CreateComponent<Integration::ActorComponent>();

        gameEntity->Init();
        gameEntity->Activate();

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
        AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));

        actorComponent->OnAssetReady(actorAsset);
        EXPECT_FALSE(actorComponent->IsWorldNotificationBusConnected(worldId))
            << "World notification bus should not be connected directly after creating the actor instance.";

        actorComponent->OnRagdollActivated();
        EXPECT_TRUE(actorComponent->IsWorldNotificationBusConnected(worldId))
            << "World notification bus should be connected after activating the ragdoll.";

        actorComponent->OnRagdollDeactivated();
        EXPECT_FALSE(actorComponent->IsWorldNotificationBusConnected(worldId))
            << "World notification bus should not be connected after deactivating the ragdoll.";

        actorComponent->OnRagdollActivated();
        EXPECT_TRUE(actorComponent->IsWorldNotificationBusConnected(worldId))
            << "World notification bus should be connected after activating the ragdoll.";

        gameEntity->Deactivate();
        EXPECT_FALSE(actorComponent->IsWorldNotificationBusConnected(worldId))
            << "World notification bus should not be connected anymore after deactivating the entire entity.";
    }
}
