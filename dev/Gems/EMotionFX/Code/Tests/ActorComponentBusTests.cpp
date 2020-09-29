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

#include <AzFramework/Components/TransformComponent.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Integration/Components/ActorComponent.h>
#include <Include/Integration/ActorComponentBus.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <MCore/Source/AttributeFloat.h>
#include <Tests/Integration/EntityComponentFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/TestActorAssets.h>

namespace EMotionFX
{
    class ActorComponentNotificationTestBus
        : Integration::ActorComponentNotificationBus::Handler
    {
    public:
        ActorComponentNotificationTestBus(AZ::EntityId entityId)
        {
            Integration::ActorComponentNotificationBus::Handler::BusConnect(entityId);
        }

        ~ActorComponentNotificationTestBus()
        {
            Integration::ActorComponentNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(OnActorInstanceCreated, void(EMotionFX::ActorInstance*));
        MOCK_METHOD1(OnActorInstanceDestroyed, void(EMotionFX::ActorInstance*));
    };

    TEST_F(EntityComponentFixture, ActorComponentNotificationBusTest)
    {
        AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>();
        AZ::EntityId entityId = AZ::EntityId(740216387);
        entity->SetId(entityId);

        ActorComponentNotificationTestBus testBus(entityId);

        auto transformComponent = entity->CreateComponent<AzFramework::TransformComponent>();
        Integration::ActorComponent* actorComponent = entity->CreateComponent<Integration::ActorComponent>();

        entity->Init();

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
        AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));

        actorComponent->OnAssetReady(actorAsset);

        EXPECT_CALL(testBus, OnActorInstanceCreated(testing::_));
        entity->Activate();

        EXPECT_CALL(testBus, OnActorInstanceDestroyed(testing::_));
        entity->Deactivate();
    }
} // end namespace EMotionFX
