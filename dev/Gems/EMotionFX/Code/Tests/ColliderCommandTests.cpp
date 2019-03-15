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

#include "ActorFixture.h"
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>


namespace EMotionFX
{
    class ColliderCommandTests
        : public ActorFixture
    {
        void SetUp()
        {
            ActorFixture::SetUp();
        }

        void TearDown()
        {
            ActorFixture::TearDown();
        }
    };

    size_t CountColliders(const Actor* actor, PhysicsSetup::ColliderConfigType colliderConfigType, bool ignoreShapeType = true, Physics::ShapeType shapeTypeToCount = Physics::ShapeType::Box)
    {
        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(colliderConfigType);
        if (!colliderConfig)
        {
            return 0;
        }

        size_t result = 0;
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : colliderConfig->m_nodes)
        {
            if (ignoreShapeType)
            {
                // Count in all colliders.
                result += nodeConfig.m_shapes.size();
            }
            else
            {
                // Count in only the given collider type.
                for (const Physics::ShapeConfigurationPair& shapeConfigPair : nodeConfig.m_shapes)
                {
                    if (shapeConfigPair.second->GetShapeType() == shapeTypeToCount)
                    {
                        result++;
                    }
                }
            }
        }

        return result;
    }

    TEST_F(ColliderCommandTests, EvaluateColliderCommands)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const AZ::u32 actorId = m_actor->GetID();
        const AZStd::vector<AZStd::string> jointNames = GetTestJointNames();
        const size_t jointCount = jointNames.size();


        // 1. Add colliders
        const AZStd::string serializedBeforeAdd = SerializePhysicsSetup(m_actor);
        for (const AZStd::string& jointName : jointNames)
        {
            CommandColliderHelpers::AddCollider(actorId, jointName, PhysicsSetup::HitDetection, azrtti_typeid<Physics::BoxShapeConfiguration>(), &commandGroup);
            CommandColliderHelpers::AddCollider(actorId, jointName, PhysicsSetup::HitDetection, azrtti_typeid<Physics::CapsuleShapeConfiguration>(), &commandGroup);
            CommandColliderHelpers::AddCollider(actorId, jointName, PhysicsSetup::HitDetection, azrtti_typeid<Physics::SphereShapeConfiguration>(), &commandGroup);
        }

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
            const AZStd::string serializedAfterAdd = SerializePhysicsSetup(m_actor);
            EXPECT_EQ(jointCount * 3, CountColliders(m_actor, PhysicsSetup::HitDetection));
            EXPECT_EQ(jointCount, CountColliders(m_actor, PhysicsSetup::HitDetection, /*ignoreShapeType*/false, Physics::ShapeType::Box));

        EXPECT_TRUE(commandManager.Undo(result));
            EXPECT_EQ(0, CountColliders(m_actor, PhysicsSetup::HitDetection));
            EXPECT_EQ(serializedBeforeAdd, SerializePhysicsSetup(m_actor));

        EXPECT_TRUE(commandManager.Redo(result));
            EXPECT_EQ(jointCount * 3, CountColliders(m_actor, PhysicsSetup::HitDetection));
            EXPECT_EQ(jointCount, CountColliders(m_actor, PhysicsSetup::HitDetection, /*ignoreShapeType*/false, Physics::ShapeType::Box));
            EXPECT_EQ(serializedAfterAdd, SerializePhysicsSetup(m_actor));


        // 2. Remove colliders
        commandGroup.RemoveAllCommands();
        const AZStd::string serializedBeforeRemove = SerializePhysicsSetup(m_actor);

        size_t colliderIndexToRemove = 1;
        for (const AZStd::string& jointName : jointNames)
        {
            CommandColliderHelpers::RemoveCollider(actorId, jointName, PhysicsSetup::HitDetection, colliderIndexToRemove, &commandGroup);
        }

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
            const AZStd::string serializedAfterRemove = SerializePhysicsSetup(m_actor);
            EXPECT_EQ(jointCount * 2, CountColliders(m_actor, PhysicsSetup::HitDetection));
            EXPECT_EQ(0, CountColliders(m_actor, PhysicsSetup::HitDetection, /*ignoreShapeType*/false, Physics::ShapeType::Capsule));

        EXPECT_TRUE(commandManager.Undo(result));
            EXPECT_EQ(jointCount * 3, CountColliders(m_actor, PhysicsSetup::HitDetection));
            EXPECT_EQ(serializedBeforeRemove, SerializePhysicsSetup(m_actor));

        EXPECT_TRUE(commandManager.Redo(result));
            EXPECT_EQ(jointCount * 2, CountColliders(m_actor, PhysicsSetup::HitDetection));
            EXPECT_EQ(0, CountColliders(m_actor, PhysicsSetup::HitDetection, /*ignoreShapeType*/false, Physics::ShapeType::Capsule));
            EXPECT_EQ(serializedAfterRemove, SerializePhysicsSetup(m_actor));
    }
} // namespace EMotionFX