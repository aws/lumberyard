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
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>


namespace CommandSystem
{
    void AddCondition(const EMotionFX::AnimGraphStateTransition* transition, const AZ::TypeId& conditionType,
        const AZStd::optional<AZStd::string>& contents = AZStd::nullopt, const AZStd::optional<size_t>& insertAt = AZStd::nullopt,
        MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    class CommandAddTransitionCondition
        : public MCore::Command
        , public EMotionFX::ParameterMixinAnimGraphId
        , public EMotionFX::ParameterMixinTransitionId
    {
    public:
        AZ_RTTI(CommandAddTransitionCondition, "{617FB76A-4BE8-47EA-B7F1-2FD0B961E352}", MCore::Command, ParameterMixinAnimGraphId, ParameterMixinTransitionId)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAddTransitionCondition(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add a transition condition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAddTransitionCondition(this); }

    private:
        size_t          mOldConditionIndex;
        bool            mOldDirtyFlag;
        AZStd::string   mOldContents;
    };


    class CommandRemoveTransitionCondition
        : public MCore::Command
        , public EMotionFX::ParameterMixinAnimGraphId
        , public EMotionFX::ParameterMixinTransitionId
    {
    public:
        AZ_RTTI(CommandRemoveTransitionCondition, "{549FF52D-0A55-4094-A4F3-5A792B4D51CD}", MCore::Command, ParameterMixinAnimGraphId, ParameterMixinTransitionId)
        AZ_CLASS_ALLOCATOR_DECL

        CommandRemoveTransitionCondition(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove a transition condition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandRemoveTransitionCondition(this); }

    private:
        AZ::TypeId      mOldConditionType;
        size_t          mOldConditionIndex;
        AZStd::string   mOldContents;
        bool            mOldDirtyFlag;
    };


    void RemoveCondition(const EMotionFX::AnimGraphStateTransition* transition, const size_t conditionIndex,
        MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    class CommandAdjustTransitionCondition
        : public MCore::Command
        , public EMotionFX::ParameterMixinAnimGraphId
        , public EMotionFX::ParameterMixinTransitionId
    {
    public:
        AZ_RTTI(CommandAdjustTransitionCondition, "{D46E1922-FB4E-4FDD-8196-5980585ABE14}", MCore::Command, ParameterMixinAnimGraphId, ParameterMixinTransitionId)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAdjustTransitionCondition(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust a transition condition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAdjustTransitionCondition(this); }

    private:
        size_t          mConditionIndex;
        bool            mOldDirtyFlag;
        AZStd::string   mOldContents;
    };

    COMMANDSYSTEM_API bool GetTransitionFromParameter(const MCore::CommandLine& parameters,
        MCore::Command* command,
        EMotionFX::AnimGraph* animGraph,
        EMotionFX::AnimGraphConnectionId& outTransitionId,
        EMotionFX::AnimGraphStateTransition*& outTransition,
        AZStd::string& outResult);
} // namespace CommandSystem