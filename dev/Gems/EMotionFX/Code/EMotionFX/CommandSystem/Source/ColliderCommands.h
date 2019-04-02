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

#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Physics/Character.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Actor;

    class CommandColliderHelpers
    {
    public:
        static Physics::CharacterColliderNodeConfiguration* GetNodeConfig(const Actor* actor, const AZStd::string& jointName, const Physics::CharacterColliderConfiguration& colliderConfig, AZStd::string& outResult);
        static Physics::CharacterColliderNodeConfiguration* GetCreateNodeConfig(const Actor* actor, const AZStd::string& jointName, Physics::CharacterColliderConfiguration& colliderConfig, AZStd::string& outResult);

        static bool AddCollider(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            const AZ::TypeId& colliderType,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        static bool AddCollider(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            const AZStd::string& contents, const AZStd::optional<size_t>& insertAtIndex = AZStd::nullopt,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        static bool AddCollider(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            const AZStd::optional<AZ::TypeId>& colliderType, const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& insertAtIndex = AZStd::nullopt,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        static bool RemoveCollider(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            size_t colliderIndex,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        static bool ClearColliders(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            MCore::CommandGroup* commandGroup = nullptr);
    };

    class CommandAddCollider
        : public MCore::Command
        , public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(CommandAddCollider, "{15DF4EB1-5FC2-457D-A2F5-0D94C9702926}", MCore::Command, ParameterMixinActorId, ParameterMixinJointName)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAddCollider(MCore::Command* orgCommand = nullptr);
        CommandAddCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, const AZ::TypeId& colliderType);
        CommandAddCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, const AZStd::string& contents, size_t insertAtIndex);
        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        static const char* s_colliderConfigTypeParameterName;
        static const char* s_colliderTypeParameterName;
        static const char* s_contentsParameterName;
        static const char* s_insertAtIndexParameterName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add collider"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAddCollider(this); }

    private:
        PhysicsSetup::ColliderConfigType m_configType;
        AZStd::optional<AZ::TypeId> m_colliderType;
        AZStd::optional<AZStd::string> m_contents;
        AZStd::optional<size_t> m_insertAtIndex;

        bool m_oldIsDirty;
        AZStd::optional<size_t> m_oldColliderIndex;
    };

    class CommandRemoveCollider
        : public MCore::Command
        , public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(CommandRemoveCollider, "{A8F70A9E-2021-473E-BF2D-AE1859D9201D}", MCore::Command, ParameterMixinActorId, ParameterMixinJointName)
        AZ_CLASS_ALLOCATOR_DECL

        CommandRemoveCollider(MCore::Command* orgCommand = nullptr);
        CommandRemoveCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, size_t colliderIndex);
        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        static const char* s_colliderConfigTypeParameterName;
        static const char* s_colliderIndexParameterName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove collider"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandRemoveCollider(this); }

    private:
        PhysicsSetup::ColliderConfigType m_configType;
        size_t m_colliderIndex;

        bool m_oldIsDirty;
        AZStd::string m_oldContents;
    };
} // namespace EMotionFX