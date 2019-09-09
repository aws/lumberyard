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

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Actor;
    class AnimGraph;
    class AnimGraphNode;
    class AnimGraphStateTransition;

    class ParameterMixinActorId
    {
    public:
        AZ_RTTI(ParameterMixinActorId, "{EE5FAA4B-FC04-4323-820F-FFE46EFC8038}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinActorId() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_actorIdParameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetActorId(AZ::u32 actorId) { m_actorId = actorId; }
        AZ::u32 GetActorId() const { return m_actorId; }

        Actor* GetActor(MCore::Command* command, AZStd::string& outResult) const;
    protected:
        AZ::u32 m_actorId = MCORE_INVALIDINDEX32;
    };

    class ParameterMixinJointName
    {
    public:
        AZ_RTTI(ParameterMixinJointName, "{9EFF81B2-4720-449F-8B7E-59C9F437E7E3}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinJointName() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_jointNameParameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetJointName(const AZStd::string& jointName) { m_jointName = jointName; }
        const AZStd::string& GetJointName() const { return m_jointName; }
    protected:
        AZStd::string m_jointName;
    };

    class ParameterMixinAnimGraphId
    {
    public:
        AZ_RTTI(ParameterMixinAnimGraphId, "{3F48199E-6566-471F-A7EA-ADF67CAC4DCD}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinAnimGraphId() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_parameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetAnimGraphId(AZ::u32 animGraphId) { m_animGraphId = animGraphId; }
        AZ::u32 GetAnimGraphId() const { return m_animGraphId; }

        AnimGraph* GetAnimGraph(MCore::Command* command, AZStd::string& outResult) const;

    protected:
        AZ::u32 m_animGraphId = MCORE_INVALIDINDEX32;
    };

    class ParameterMixinTransitionId
    {
    public:
        AZ_RTTI(ParameterMixinTransitionId, "{B70F34AB-CE3B-4AF2-B6D8-2D38F1CEBBC8}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinTransitionId() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_parameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetTransitionId(AnimGraphConnectionId transitionId) { m_transitionId = transitionId; }
        AnimGraphConnectionId GetTransitionId() const { return m_transitionId; }

        AnimGraphStateTransition* GetTransition(const AnimGraph* animGraph, const MCore::Command* command, AZStd::string& outResult) const;

    protected:
        AnimGraphConnectionId m_transitionId;
    };

    class ParameterMixinAnimGraphNodeId
    {
    public:
        AZ_RTTI(ParameterMixinAnimGraphNodeId, "{D5329E34-B20C-43D2-A169-A0E446CC244E}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinAnimGraphNodeId() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_parameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetNodeId(AnimGraphNodeId nodeId) { m_nodeId = nodeId; }
        AnimGraphNodeId GetNodeId() const { return m_nodeId; }

        AnimGraphNode* GetNode(const AnimGraph* animGraph, const MCore::Command* command, AZStd::string& outResult) const;

    protected:
        AnimGraphNodeId m_nodeId;
    };
} // namespace EMotionFX