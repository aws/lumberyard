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
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>


namespace EMotionFX
{
    class RagdollCommandTests
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

    size_t CountRagdollJoints(const Actor* actor, bool onlyCountJointsWithColliders = false)
    {
        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        size_t result = 0;
        for (const Physics::RagdollNodeConfiguration& nodeConfig : ragdollConfig.m_nodes)
        {
            if (onlyCountJointsWithColliders)
            {
                if (ragdollConfig.m_colliders.FindNodeConfigByName(nodeConfig.m_debugName))
                {
                    result++;
                }
            }
            else
            {
                result++;
            }
        }

        return result;
    }

    TEST_F(RagdollCommandTests, EvaluateRagdollCommands)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const AZ::u32 actorId = m_actor->GetID();
        const AZStd::vector<AZStd::string> jointNames = GetTestJointNames();
        const size_t jointCount = jointNames.size();


        // 1. Add joints to ragdoll
        const AZStd::string serializedBeforeAdd = SerializePhysicsSetup(m_actor);
        for (const AZStd::string& jointName : jointNames)
        {
            CommandRagdollHelpers::AddJointToRagdoll(actorId, jointName, &commandGroup, /*executeInsideCommand*/false, /*addDefaultCollider*/false);
        }

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
            const AZStd::string serializedAfterAdd = SerializePhysicsSetup(m_actor);
            EXPECT_EQ(jointCount, CountRagdollJoints(m_actor));

        EXPECT_TRUE(commandManager.Undo(result));
            EXPECT_EQ(0, CountRagdollJoints(m_actor));
            EXPECT_EQ(serializedBeforeAdd, SerializePhysicsSetup(m_actor));

        EXPECT_TRUE(commandManager.Redo(result));
            EXPECT_EQ(jointCount, CountRagdollJoints(m_actor));
            EXPECT_EQ(serializedAfterAdd, SerializePhysicsSetup(m_actor));


        // 2. Remove joints from ragdoll
        commandGroup.RemoveAllCommands();
        const AZStd::string serializedBeforeRemove = SerializePhysicsSetup(m_actor);

        for (const AZStd::string& jointName : jointNames)
        {
            CommandRagdollHelpers::RemoveJointFromRagdoll(actorId, jointName, &commandGroup);
        }

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
            const AZStd::string serializedAfterRemove = SerializePhysicsSetup(m_actor);
            EXPECT_EQ(0, CountRagdollJoints(m_actor));

        EXPECT_TRUE(commandManager.Undo(result));
            EXPECT_EQ(jointCount, CountRagdollJoints(m_actor));
            EXPECT_EQ(serializedBeforeRemove, SerializePhysicsSetup(m_actor));

        EXPECT_TRUE(commandManager.Redo(result));
            EXPECT_EQ(0, CountRagdollJoints(m_actor));
            EXPECT_EQ(serializedAfterRemove, SerializePhysicsSetup(m_actor));
    }
} // namespace EMotionFX