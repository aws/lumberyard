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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/SystemBus.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <MCore/Source/AzCoreConversions.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandAddRagdollJoint, EMotionFX::CommandAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(CommandRemoveRagdollJoint, EMotionFX::CommandAllocator, 0)

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const Physics::RagdollNodeConfiguration* CommandRagdollHelpers::GetNodeConfig(const Actor * actor, const AZStd::string& jointName,
        const Physics::RagdollConfiguration& ragdollConfig, AZStd::string& outResult)
    {
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* joint = skeleton->FindNodeByName(jointName);
        if (!joint)
        {
            outResult = AZStd::string::format("Cannot get node config. Joint with name '%s' does not exist.", jointName.c_str());
            return nullptr;
        }

        return ragdollConfig.FindNodeConfigByName(jointName);
    }

    Physics::RagdollNodeConfiguration* CommandRagdollHelpers::GetCreateNodeConfig(const Actor* actor, const AZStd::string& jointName,
        Physics::RagdollConfiguration& ragdollConfig, const AZStd::optional<size_t>& index, AZStd::string& outResult)
    {
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* joint = skeleton->FindNodeByName(jointName);
        if (!joint)
        {
            outResult = AZStd::string::format("Cannot add node config. Joint with name '%s' does not exist.", jointName.c_str());
            return nullptr;
        }

        Physics::RagdollNodeConfiguration* nodeConfig = ragdollConfig.FindNodeConfigByName(jointName);
        if (nodeConfig)
        {
            return nodeConfig;
        }

        Physics::RagdollNodeConfiguration newNodeConfig;
        newNodeConfig.m_debugName = jointName;

        // Create joint limit on default.
        AZStd::vector<AZ::TypeId> supportedJointLimitTypes;
        Physics::SystemRequestBus::BroadcastResult(supportedJointLimitTypes, &Physics::SystemRequests::GetSupportedJointTypes);
        if (!supportedJointLimitTypes.empty())
        {
            newNodeConfig.m_jointLimit = CommandRagdollHelpers::CreateJointLimitByType(supportedJointLimitTypes[0], skeleton, joint);
        }

        if (index)
        {
            const auto iterator = ragdollConfig.m_nodes.begin() + index.value();
            ragdollConfig.m_nodes.insert(iterator, newNodeConfig);
            return iterator;
        }
        else
        {
            ragdollConfig.m_nodes.push_back(newNodeConfig);
            return &ragdollConfig.m_nodes.back();
        }
    }

    AZStd::unique_ptr<Physics::JointLimitConfiguration> CommandRagdollHelpers::CreateJointLimitByType(
        const AZ::TypeId& typeId, const Skeleton* skeleton, const Node* node)
    {
        const Pose* bindPose = skeleton->GetBindPose();
        const Transform& nodeBindTransform = bindPose->GetModelSpaceTransform(node->GetNodeIndex());
        const Transform& parentBindTransform = node->GetParentNode()
            ? bindPose->GetModelSpaceTransform(node->GetParentIndex())
            : Transform::CreateIdentity();
        const AZ::Quaternion& nodeBindRotationWorld = nodeBindTransform.mRotation;
        const AZ::Quaternion& parentBindRotationWorld = parentBindTransform.mRotation;

        AZ::Vector3 boneDirection = AZ::Vector3::CreateAxisX();

        // if there are child nodes, point the bone direction to the average of their positions
        const uint32 numChildNodes = node->GetNumChildNodes();
        if (numChildNodes > 0)
        {
            AZ::Vector3 meanChildPosition = AZ::Vector3::CreateZero();

            // weight by the number of descendants of each child node, so that things like jiggle bones and twist bones
            // have little influence on the bone direction.
            float totalSubChildren = 0.0f;
            for (uint32 childNumber = 0; childNumber < numChildNodes; childNumber++)
            {
                const uint32 childIndex = node->GetChildIndex(childNumber);
                const Node* childNode = skeleton->GetNode(childIndex);
                const float numSubChildren = static_cast<float>(1 + childNode->GetNumChildNodesRecursive());
                totalSubChildren += numSubChildren;
                meanChildPosition += numSubChildren * (bindPose->GetModelSpaceTransform(childIndex).mPosition);
            }

            boneDirection = meanChildPosition / totalSubChildren - nodeBindTransform.mPosition;
        }
        // otherwise, point the bone direction away from the parent
        else
        {
            boneDirection = nodeBindTransform.mPosition - parentBindTransform.mPosition;
        }

        AZStd::vector<AZ::Quaternion> exampleRotationsLocal;

        AZStd::unique_ptr<Physics::JointLimitConfiguration> jointLimitConfig =
            AZ::Interface<Physics::System>::Get()->ComputeInitialJointLimitConfiguration(
                typeId, parentBindRotationWorld, nodeBindRotationWorld, boneDirection, exampleRotationsLocal);

        AZ_Assert(jointLimitConfig, "Could not create joint limit configuration with type '%s'.", typeId.ToString<AZStd::string>().c_str());
        return jointLimitConfig;
    }

    void CommandRagdollHelpers::AddJointToRagdoll(AZ::u32 actorId, const AZStd::string& jointName,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand, bool addDefaultCollider)
    {
        AddJointToRagdoll(actorId, jointName, AZStd::nullopt, AZStd::nullopt, commandGroup, executeInsideCommand, addDefaultCollider);
    }

    void CommandRagdollHelpers::AddJointToRagdoll(AZ::u32 actorId, const AZStd::string& jointName,
        const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& index,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand, bool addDefaultCollider)
    {
        // 1. Add joint to ragdoll.
        AZStd::string command = AZStd::string::format("%s -%s %d -%s \"%s\"",
                CommandAddRagdollJoint::s_commandName,
                CommandAddRagdollJoint::s_actorIdParameterName,
                actorId,
                CommandAddRagdollJoint::s_jointNameParameterName,
                jointName.c_str());

        if (contents)
        {
            command += AZStd::string::format(" -%s {", CommandAddRagdollJoint::s_contentsParameterName);
            command += contents.value();
            command += "}";
        }

        if (index)
        {
            command += AZStd::string::format(" -%s %d", CommandAddRagdollJoint::s_indexParameterName, index.value());
        }

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);

        // 2. Create default capsule collider.
        if (addDefaultCollider)
        {
            const AZ::TypeId defaultColliderType = azrtti_typeid<Physics::CapsuleShapeConfiguration>();
            CommandColliderHelpers::AddCollider(actorId, jointName,
                PhysicsSetup::Ragdoll, defaultColliderType,
                commandGroup, executeInsideCommand);
        }
    }

    void CommandRagdollHelpers::RemoveJointFromRagdoll(AZ::u32 actorId, const AZStd::string& jointName, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        // 1. Clear all ragdoll colliders for this joint.
        CommandColliderHelpers::ClearColliders(actorId, jointName,
            PhysicsSetup::Ragdoll,
            commandGroup);

        // 2. Remove joint from ragdoll.
        AZStd::string command = AZStd::string::format("%s -%s %d -%s \"%s\"",
                CommandRemoveRagdollJoint::s_commandName,
                CommandRemoveRagdollJoint::s_actorIdParameterName,
                actorId,
                CommandRemoveRagdollJoint::s_jointNameParameterName,
                jointName.c_str());

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandAddRagdollJoint::s_commandName = "AddRagdollJoint";
    const char* CommandAddRagdollJoint::s_contentsParameterName = "contents";
    const char* CommandAddRagdollJoint::s_indexParameterName = "index";

    CommandAddRagdollJoint::CommandAddRagdollJoint(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_oldIsDirty(false)
    {
    }

    CommandAddRagdollJoint::CommandAddRagdollJoint(AZ::u32 actorId, const AZStd::string& jointName, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
        , ParameterMixinJointName(jointName)
    {
    }

    void CommandAddRagdollJoint::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAddRagdollJoint, MCore::Command, ParameterMixinActorId, ParameterMixinJointName>()
            ->Version(1)
        ;
    }

    bool CommandAddRagdollJoint::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        Physics::RagdollNodeConfiguration* nodeConfig = CommandRagdollHelpers::GetCreateNodeConfig(actor, m_jointName, ragdollConfig, m_index, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        // Either in case the contents got specified via a command parameter or in case of redo.
        if (m_contents)
        {
            const AZStd::string contents = m_contents.value();
            MCore::ReflectionSerializer::Deserialize(nodeConfig, contents);
        }

        m_oldIsDirty = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAddRagdollJoint::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        const Physics::RagdollNodeConfiguration* nodeConfig = CommandRagdollHelpers::GetNodeConfig(actor, m_jointName, ragdollConfig, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        m_contents = MCore::ReflectionSerializer::Serialize(nodeConfig).GetValue();

        CommandRagdollHelpers::RemoveJointFromRagdoll(m_actorId, m_jointName, /*commandGroup*/ nullptr, true);

        actor->SetDirtyFlag(m_oldIsDirty);
        return true;
    }

    void CommandAddRagdollJoint::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(3);
        ParameterMixinActorId::InitSyntax(syntax);
        ParameterMixinJointName::InitSyntax(syntax);

        syntax.AddParameter(s_contentsParameterName, "The serialized contents (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_indexParameterName, "The index of the ragdoll node config.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }

    bool CommandAddRagdollJoint::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        ParameterMixinJointName::SetCommandParameters(parameters);

        if (parameters.CheckIfHasParameter(s_contentsParameterName))
        {
            m_contents = parameters.GetValue(s_contentsParameterName, this);
        }

        if (parameters.CheckIfHasParameter(s_indexParameterName))
        {
            m_index = parameters.GetValueAsInt(s_indexParameterName, this);
        }

        return true;
    }

    const char* CommandAddRagdollJoint::GetDescription() const
    {
        return "Add node to ragdoll.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandRemoveRagdollJoint::s_commandName = "RemoveRagdollJoint";

    CommandRemoveRagdollJoint::CommandRemoveRagdollJoint(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_oldIsDirty(false)
    {
    }

    CommandRemoveRagdollJoint::CommandRemoveRagdollJoint(AZ::u32 actorId, const AZStd::string& jointName, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
        , ParameterMixinJointName(jointName)
    {
    }

    void CommandRemoveRagdollJoint::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandRemoveRagdollJoint, MCore::Command, ParameterMixinActorId, ParameterMixinJointName>()
            ->Version(1)
        ;
    }

    bool CommandRemoveRagdollJoint::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        const Physics::RagdollNodeConfiguration* nodeConfig = CommandRagdollHelpers::GetNodeConfig(actor, m_jointName, ragdollConfig, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        m_oldContents = MCore::ReflectionSerializer::Serialize(nodeConfig).GetValue();
        m_oldIndex = ragdollConfig.FindNodeConfigIndexByName(m_jointName).GetValue();
        m_oldIsDirty = actor->GetDirtyFlag();

        ragdollConfig.RemoveNodeConfigByName(m_jointName);

        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandRemoveRagdollJoint::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        CommandRagdollHelpers::AddJointToRagdoll(m_actorId, m_jointName, m_oldContents, m_oldIndex, /*commandGroup*/ nullptr, /*executeInsideCommand*/ true, /*addDefaultCollider*/ false);
        actor->SetDirtyFlag(m_oldIsDirty);
        return true;
    }

    void CommandRemoveRagdollJoint::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(2);
        ParameterMixinActorId::InitSyntax(syntax);
        ParameterMixinJointName::InitSyntax(syntax);
    }

    bool CommandRemoveRagdollJoint::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        ParameterMixinJointName::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandRemoveRagdollJoint::GetDescription() const
    {
        return "Remove the given joint from the ragdoll.";
    }
} // namespace EMotionFX
