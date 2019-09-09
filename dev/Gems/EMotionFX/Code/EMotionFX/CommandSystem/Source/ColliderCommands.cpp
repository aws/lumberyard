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
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/PhysicsSetup.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandAddCollider, EMotionFX::CommandAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(CommandRemoveCollider, EMotionFX::CommandAllocator, 0)

    Physics::CharacterColliderNodeConfiguration* CommandColliderHelpers::GetNodeConfig(const Actor * actor, const AZStd::string& jointName, const Physics::CharacterColliderConfiguration& colliderConfig, AZStd::string& outResult)
    {
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* joint = skeleton->FindNodeByName(jointName);
        if (!joint)
        {
            outResult = AZStd::string::format("Cannot get node config. Joint with name '%s' does not exist.", jointName.c_str());
            return nullptr;
        }

        return colliderConfig.FindNodeConfigByName(jointName);
    }

    Physics::CharacterColliderNodeConfiguration* CommandColliderHelpers::GetCreateNodeConfig(const Actor* actor, const AZStd::string& jointName, Physics::CharacterColliderConfiguration& colliderConfig, AZStd::string& outResult)
    {
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* joint = skeleton->FindNodeByName(jointName);
        if (!joint)
        {
            outResult = AZStd::string::format("Cannot add node config. Joint with name '%s' does not exist.", jointName.c_str());
            return nullptr;
        }

        Physics::CharacterColliderNodeConfiguration* nodeConfig = colliderConfig.FindNodeConfigByName(jointName);
        if (nodeConfig)
        {
            return nodeConfig;
        }

        Physics::CharacterColliderNodeConfiguration newNodeConfig;
        newNodeConfig.m_name = jointName;

        colliderConfig.m_nodes.push_back(newNodeConfig);
        return &colliderConfig.m_nodes.back();
    }

    bool CommandColliderHelpers::AddCollider(AZ::u32 actorId, const AZStd::string& jointName, const PhysicsSetup::ColliderConfigType& configType, const AZ::TypeId& colliderType, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        return AddCollider(actorId, jointName, configType, colliderType, AZStd::nullopt, AZStd::nullopt, commandGroup, executeInsideCommand);
    }

    bool CommandColliderHelpers::AddCollider(AZ::u32 actorId, const AZStd::string& jointName, const PhysicsSetup::ColliderConfigType& configType, const AZStd::string& contents, const AZStd::optional<size_t>& insertAtIndex, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        return AddCollider(actorId, jointName, configType, AZStd::nullopt, contents, insertAtIndex, commandGroup, executeInsideCommand);
    }

    bool CommandColliderHelpers::AddCollider(AZ::u32 actorId, const AZStd::string& jointName, const PhysicsSetup::ColliderConfigType& configType, const AZStd::optional<AZ::TypeId>& colliderType, const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& insertAtIndex, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %d -%s \"%s\" -%s \"%s\"",
            CommandAddCollider::s_commandName,
            CommandAddCollider::s_actorIdParameterName,
            actorId,
            CommandAddCollider::s_colliderConfigTypeParameterName,
            PhysicsSetup::GetStringForColliderConfigType(configType),
            CommandAddCollider::s_jointNameParameterName,
            jointName.c_str());

        if (colliderType)
        {
            command += AZStd::string::format(" -%s \"%s\"", CommandAddCollider::s_colliderTypeParameterName, colliderType.value().ToString<AZStd::string>().c_str());
        }

        if (contents)
        {
            command += AZStd::string::format(" -%s {", CommandAddCollider::s_contentsParameterName);
            command += contents.value();
            command += "}";
        }

        if (insertAtIndex)
        {
            command += AZStd::string::format(" -%s %d", CommandAddCollider::s_insertAtIndexParameterName, insertAtIndex.value());
        }

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    bool CommandColliderHelpers::RemoveCollider(AZ::u32 actorId, const AZStd::string& jointName, const PhysicsSetup::ColliderConfigType& configType, size_t colliderIndex, MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        const AZStd::string command = AZStd::string::format("%s -%s %d -%s \"%s\" -%s \"%s\" -%s %d",
            CommandRemoveCollider::s_commandName,
            CommandRemoveCollider::s_actorIdParameterName,
            actorId,
            CommandAddCollider::s_colliderConfigTypeParameterName,
            PhysicsSetup::GetStringForColliderConfigType(configType),
            CommandRemoveCollider::s_jointNameParameterName,
            jointName.c_str(),
            CommandRemoveCollider::s_colliderIndexParameterName,
            colliderIndex);

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    bool CommandColliderHelpers::ClearColliders(AZ::u32 actorId, const AZStd::string& jointName, const PhysicsSetup::ColliderConfigType& configType, MCore::CommandGroup* commandGroup)
    {
        Actor* actor = GetEMotionFX().GetActorManager()->FindActorByID(actorId);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(configType);
        if (!colliderConfig)
        {
            return false;
        }

        AZStd::string result;
        Physics::CharacterColliderNodeConfiguration* nodeConfig = CommandColliderHelpers::GetNodeConfig(actor, jointName, *colliderConfig, result);
        if (!nodeConfig)
        {
            // No colliders assigned to this joint. We can return directly.
            return true;
        }

        MCore::CommandGroup newCommandGroup("Clear colliders");
        if (!commandGroup)
        {
            commandGroup = &newCommandGroup;
        }

        // Remove the colliders back to front
        const size_t shapeCount = nodeConfig->m_shapes.size();
        for (size_t i = 0; i < shapeCount; ++i)
        {
            const size_t colliderIndex = shapeCount - 1 - i;
            RemoveCollider(actorId, jointName, configType, colliderIndex, commandGroup);
        }

        if (!commandGroup)
        {
            if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(*commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
                return false;
            }
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandAddCollider::s_commandName = "AddCollider";
    const char* CommandAddCollider::s_colliderConfigTypeParameterName = "colliderConfigType";
    const char* CommandAddCollider::s_colliderTypeParameterName = "colliderType";
    const char* CommandAddCollider::s_contentsParameterName = "contents";
    const char* CommandAddCollider::s_insertAtIndexParameterName = "insertAtIndex";

    CommandAddCollider::CommandAddCollider(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_oldIsDirty(false)
    {
    }

    CommandAddCollider::CommandAddCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, const AZ::TypeId& colliderType)
        : CommandAddCollider(nullptr)
    {
        m_actorId = actorId;
        m_jointName = jointName;
        m_configType = configType;
        m_colliderType = colliderType;
    }

    CommandAddCollider::CommandAddCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, const AZStd::string& contents, size_t insertAtIndex)
        : CommandAddCollider(nullptr)
    {
        m_actorId = actorId;
        m_jointName = jointName;
        m_configType = configType;
        m_contents = contents;
        m_insertAtIndex = insertAtIndex;
    }

    void CommandAddCollider::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAddCollider, MCore::Command, ParameterMixinActorId, ParameterMixinJointName>()
            ->Version(2)
            ->Field("configType", &CommandAddCollider::m_configType)
            ->Field("colliderType", &CommandAddCollider::m_colliderType)
            ->Field("contents", &CommandAddCollider::m_contents)
            ->Field("insertAtIndex", &CommandAddCollider::m_insertAtIndex)
        ;
    }

    bool CommandAddCollider::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(m_configType);
        if (!colliderConfig)
        {
            outResult = "Cannot get collider configuration. Invalid type specified.";
            return false;
        }

        Physics::CharacterColliderNodeConfiguration* nodeConfig = CommandColliderHelpers::GetCreateNodeConfig(actor, m_jointName, *colliderConfig, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        Physics::ShapeConfigurationPair newCollider;

        // Either in case the contents got specified via a command parameter or in case of redo.
        if (m_contents)
        {
            const AZStd::string contents = m_contents.value();
            // Deserialize the contents directly, else we might be overwriting things in the end.
            MCore::ReflectionSerializer::Deserialize(&newCollider, contents);
        }
        else if (m_colliderType)
        {
            // Create new collider.
            AZ::Outcome<Physics::ShapeConfigurationPair> colliderOutcome = PhysicsSetup::CreateColliderByType(m_colliderType.value(), outResult);
            if (!colliderOutcome.IsSuccess())
            {
                return false;
            }

            newCollider = colliderOutcome.GetValue();

            // Auto size
            const Skeleton* skeleton = actor->GetSkeleton();
            const Node* joint = skeleton->FindNodeByName(m_jointName);
            PhysicsSetup::AutoSizeCollider(newCollider, actor, joint);
        }
        else
        {
            outResult = "Cannot add collider. Neither the collider type nor contents are specified.";
            return false;
        }

        if (m_configType == PhysicsSetup::HitDetection)
        {
            // Collider needs to be exclusive in order to be movable.
            newCollider.first->m_isExclusive = true;
        }
        if (m_configType == PhysicsSetup::Cloth)
        {
            // Cloth specific settings
            newCollider.first->SetPropertyVisibility(Physics::ColliderConfiguration::CollisionLayer, false);
            newCollider.first->SetPropertyVisibility(Physics::ColliderConfiguration::MaterialSelection, false);
            newCollider.first->SetPropertyVisibility(Physics::ColliderConfiguration::IsTrigger, false);
        }

        if (m_insertAtIndex)
        {
            nodeConfig->m_shapes.insert(nodeConfig->m_shapes.begin() + m_insertAtIndex.value(), newCollider);
            m_oldColliderIndex = m_insertAtIndex;
        }
        else if (m_oldColliderIndex)
        {
            nodeConfig->m_shapes.insert(nodeConfig->m_shapes.begin() + m_oldColliderIndex.value(), newCollider);
        }
        else
        {
            nodeConfig->m_shapes.emplace_back(newCollider);
            m_oldColliderIndex = nodeConfig->m_shapes.size() - 1;
        }

        m_oldIsDirty = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAddCollider::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(m_configType);
        if (!colliderConfig)
        {
            outResult = "Cannot get collider configuration. Invalid type specified.";
            return false;
        }

        Physics::CharacterColliderNodeConfiguration* nodeConfig = CommandColliderHelpers::GetNodeConfig(actor, m_jointName, *colliderConfig, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        const size_t shapeCount = nodeConfig->m_shapes.size();
        if (!m_oldColliderIndex || m_oldColliderIndex.value() >= shapeCount)
        {
            outResult = AZStd::string::format("Cannot undo adding collider. The joint '%s' is only holding %d colliders and the index %d is out of range.", m_jointName.c_str(), shapeCount, m_oldColliderIndex.value());
            return false;
        }

        const Physics::ShapeConfigurationPair& collider = nodeConfig->m_shapes[m_oldColliderIndex.value()];
        m_contents = MCore::ReflectionSerializer::Serialize(&collider).GetValue();

        CommandColliderHelpers::RemoveCollider(m_actorId, m_jointName, m_configType, m_oldColliderIndex.value(), /*commandGroup*/ nullptr, true);

        actor->SetDirtyFlag(m_oldIsDirty);
        return true;
    }

    void CommandAddCollider::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(6);
        ParameterMixinActorId::InitSyntax(syntax);
        ParameterMixinJointName::InitSyntax(syntax);

        syntax.AddRequiredParameter(s_colliderConfigTypeParameterName, "The config to which the collider shall be added to. [HitDetection, Ragdoll, Cloth]", MCore::CommandSyntax::PARAMTYPE_STRING);
        syntax.AddParameter(s_colliderTypeParameterName, "Collider type UUID in the registry format.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_contentsParameterName, "The serialized contents of the collider (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_insertAtIndexParameterName, "The index at which collider will be added.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }

    bool CommandAddCollider::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        ParameterMixinJointName::SetCommandParameters(parameters);
        m_configType = PhysicsSetup::GetColliderConfigTypeFromString(parameters.GetValue(s_colliderConfigTypeParameterName, this));

        if (parameters.CheckIfHasParameter(s_contentsParameterName))
        {
            m_contents = parameters.GetValue(s_contentsParameterName, this);
        }

        if (parameters.CheckIfHasParameter(s_colliderTypeParameterName))
        {
            AZStd::string colliderTypeString = parameters.GetValue(s_colliderTypeParameterName, this);
            m_colliderType = AZ::TypeId::CreateString(colliderTypeString.c_str(), colliderTypeString.size());
        }

        if (parameters.CheckIfHasParameter(s_insertAtIndexParameterName))
        {
            m_insertAtIndex = parameters.GetValueAsInt(s_insertAtIndexParameterName, this);
        }

        return true;
    }

    const char* CommandAddCollider::GetDescription() const
    {
        return "Add collider of the given type.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandRemoveCollider::s_commandName = "RemoveCollider";
    const char* CommandRemoveCollider::s_colliderConfigTypeParameterName = "colliderConfigType";
    const char* CommandRemoveCollider::s_colliderIndexParameterName = "colliderIndex";

    CommandRemoveCollider::CommandRemoveCollider(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_oldIsDirty(false)
    {
    }

    CommandRemoveCollider::CommandRemoveCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, size_t colliderIndex)
        : CommandRemoveCollider(nullptr)
    {
        m_actorId = actorId;
        m_jointName = jointName;
        m_configType = configType;
        m_colliderIndex = colliderIndex;
    }

    void CommandRemoveCollider::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandRemoveCollider, MCore::Command, ParameterMixinActorId, ParameterMixinJointName>()
            ->Version(2)
            ->Field("configType", &CommandRemoveCollider::m_configType)
            ->Field("colliderIndex", &CommandRemoveCollider::m_colliderIndex)
        ;
    }

    bool CommandRemoveCollider::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(m_configType);
        if (!colliderConfig)
        {
            outResult = "Cannot get collider configuration. Invalid type specified.";
            return false;
        }

        Physics::CharacterColliderNodeConfiguration* nodeConfig = CommandColliderHelpers::GetNodeConfig(actor, m_jointName, *colliderConfig, outResult);
        if (!nodeConfig)
        {
            return false;
        }

        const size_t shapeCount = nodeConfig->m_shapes.size();
        if (m_colliderIndex >= shapeCount)
        {
            outResult = AZStd::string::format("Cannot remove collider. The joint '%s' is only holding %d colliders and the index %d is out of range.", m_jointName.c_str(), shapeCount, m_colliderIndex);
            return false;
        }

        m_oldContents = MCore::ReflectionSerializer::Serialize(&nodeConfig->m_shapes[m_colliderIndex]).GetValue();
        m_oldIsDirty = actor->GetDirtyFlag();

        nodeConfig->m_shapes.erase(nodeConfig->m_shapes.begin() + m_colliderIndex);

        // Remove the whole node config in case there are no shapes anymore.
        if (nodeConfig->m_shapes.empty())
        {
            colliderConfig->RemoveNodeConfigByName(nodeConfig->m_name);
        }

        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandRemoveCollider::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        CommandColliderHelpers::AddCollider(m_actorId, m_jointName, m_configType, m_oldContents, m_colliderIndex, /*commandGroup*/ nullptr, true);
        actor->SetDirtyFlag(m_oldIsDirty);
        return true;
    }

    void CommandRemoveCollider::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(4);
        ParameterMixinActorId::InitSyntax(syntax);
        ParameterMixinJointName::InitSyntax(syntax);

        syntax.AddRequiredParameter(s_colliderConfigTypeParameterName, "The config to which the collider shall be added to. [HitDetection, Ragdoll, Cloth]", MCore::CommandSyntax::PARAMTYPE_STRING);
        syntax.AddRequiredParameter(s_colliderIndexParameterName, "Collider index to be removed.", MCore::CommandSyntax::PARAMTYPE_INT);
    }

    bool CommandRemoveCollider::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        ParameterMixinJointName::SetCommandParameters(parameters);
        m_configType = PhysicsSetup::GetColliderConfigTypeFromString(parameters.GetValue(s_colliderConfigTypeParameterName, this));
        m_colliderIndex = parameters.GetValueAsInt(s_colliderIndexParameterName, this);
        return true;
    }

    const char* CommandRemoveCollider::GetDescription() const
    {
        return "Remove the collider of the given index.";
    }
} // namespace EMotionFX