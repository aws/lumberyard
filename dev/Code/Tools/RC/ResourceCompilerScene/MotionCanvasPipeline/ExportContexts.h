#pragma once

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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <RC/ResourceCompilerScene/Common/ExportContextGlobal.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IEFXMotionGroup;
            class IActorGroup;
        }
    }
}


namespace EMotionFX
{
    class SkeletalMotion;
    class Actor;
}

namespace MotionCanvasPipeline
{
    // Context structure to export a specific Animation (Motion) Group
    struct MotionGroupExportContext
        : public AZ::SceneAPI::Events::ICallContext
    {
        AZ_RTTI(MotionGroupExportContext, "{03B84A87-D1C2-4392-B78B-AC1174CA296E}", AZ::SceneAPI::Events::ICallContext);

        MotionGroupExportContext(AZ::SceneAPI::Events::ExportEventContext& parent,
            const AZ::SceneAPI::DataTypes::IEFXMotionGroup& group, AZ::RC::Phase phase);
        MotionGroupExportContext(const AZ::SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
            const AZ::SceneAPI::DataTypes::IEFXMotionGroup& group, AZ::RC::Phase phase);
        MotionGroupExportContext(const MotionGroupExportContext& copyContent, AZ::RC::Phase phase);
        MotionGroupExportContext(const MotionGroupExportContext& copyContent) = delete;
        ~MotionGroupExportContext() override = default;

        MotionGroupExportContext& operator=(const MotionGroupExportContext& other) = delete;

        const AZ::SceneAPI::Containers::Scene&          m_scene;
        const AZStd::string&                            m_outputDirectory;
        const AZ::SceneAPI::DataTypes::IEFXMotionGroup& m_group;
        const AZ::RC::Phase                             m_phase;
    };

    // Context structure for building the motion data structure for the purpose of exporting.
    struct MotionDataBuilderContext
        : public AZ::SceneAPI::Events::ICallContext
    {
        AZ_RTTI(MotionDataBuilderContext, "{1C5795BB-2130-499E-96AD-50926EFC8CE9}", AZ::SceneAPI::Events::ICallContext);

        MotionDataBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IEFXMotionGroup& motionGroup,
            EMotionFX::SkeletalMotion& motion, AZ::RC::Phase phase);
        MotionDataBuilderContext(const MotionDataBuilderContext& copyContext, AZ::RC::Phase phase);
        MotionDataBuilderContext(const MotionDataBuilderContext& copyContext) = delete;
        ~MotionDataBuilderContext() override = default;

        MotionDataBuilderContext& operator=(const MotionDataBuilderContext& other) = delete;

        const AZ::SceneAPI::Containers::Scene&          m_scene;
        const AZ::SceneAPI::DataTypes::IEFXMotionGroup& m_group;
        EMotionFX::SkeletalMotion&                      m_motion;
        const AZ::RC::Phase                             m_phase;
    };

    // Context structure to export a specific Actor Group
    struct ActorGroupExportContext
        : public AZ::SceneAPI::Events::ICallContext
    {
        AZ_RTTI(ActorGroupExportContext, "{9FBECA5A-8EDB-4178-8A66-793A5F55B194}", AZ::SceneAPI::Events::ICallContext);

        ActorGroupExportContext(AZ::SceneAPI::Events::ExportEventContext& parent,
            const AZ::SceneAPI::DataTypes::IActorGroup& group, AZ::RC::Phase phase);
        ActorGroupExportContext(const AZ::SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
            const AZ::SceneAPI::DataTypes::IActorGroup& group, AZ::RC::Phase phase);
        ActorGroupExportContext(const ActorGroupExportContext& copyContext, AZ::RC::Phase phase);
        ActorGroupExportContext(const ActorGroupExportContext& copyContext) = delete;
        ~ActorGroupExportContext() override = default;

        ActorGroupExportContext& operator=(const ActorGroupExportContext& other) = delete;

        const AZ::SceneAPI::Containers::Scene&          m_scene;
        const AZStd::string&                            m_outputDirectory;
        const AZ::SceneAPI::DataTypes::IActorGroup&     m_group;
        const AZ::RC::Phase                             m_phase;
    };

    // Context structure for building the actor data structure for the purpose of exporting.
    struct ActorBuilderContext
        : public AZ::SceneAPI::Events::ICallContext
    {
        AZ_RTTI(ActorBuilderContext, "{92048988-F567-4E6C-B6BD-3EFD2A5B6AA1}", AZ::SceneAPI::Events::ICallContext);

        ActorBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
            const AZ::SceneAPI::DataTypes::IActorGroup& actorGroup, EMotionFX::Actor* actor, AZ::RC::Phase phase);
        ActorBuilderContext(const ActorBuilderContext& copyContext, AZ::RC::Phase phase);
        ActorBuilderContext(const ActorBuilderContext& copyContext) = delete;
        ~ActorBuilderContext() override = default;

        ActorBuilderContext& operator=(const ActorBuilderContext& other) = delete;

        const AZ::SceneAPI::Containers::Scene&          m_scene;
        const AZStd::string&                            m_outputDirectory;
        EMotionFX::Actor*                               m_actor;
        const AZ::SceneAPI::DataTypes::IActorGroup&     m_group;
        const AZ::RC::Phase                             m_phase;
    };

} // MotionCanvasPipeline
#endif //MOTIONCANVAS_GEM_ENABLED