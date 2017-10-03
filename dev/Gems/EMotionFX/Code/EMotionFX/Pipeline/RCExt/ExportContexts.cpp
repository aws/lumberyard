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

#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ExportContexts.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IEFXMotionGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IActorGroup.h>

namespace MotionCanvasPipeline
{
    //==========================================================================
    //  Motion
    //==========================================================================

    MotionGroupExportContext::MotionGroupExportContext(AZ::SceneAPI::Events::ExportEventContext& parent,
        const AZ::SceneAPI::DataTypes::IEFXMotionGroup& group, AZ::RC::Phase phase)
        : m_scene(parent.GetScene())
        , m_outputDirectory(parent.GetOutputDirectory())
        , m_group(group)
        , m_phase(phase)
    {
    }

    MotionGroupExportContext::MotionGroupExportContext(const AZ::SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
        const AZ::SceneAPI::DataTypes::IEFXMotionGroup& group, AZ::RC::Phase phase)
        : m_scene(scene)
        , m_outputDirectory(outputDirectory)
        , m_group(group)
        , m_phase(phase)
    {
    }

    MotionGroupExportContext::MotionGroupExportContext(const MotionGroupExportContext& copyContext, AZ::RC::Phase phase)
        : m_scene(copyContext.m_scene)
        , m_outputDirectory(copyContext.m_outputDirectory)
        , m_group(copyContext.m_group)
        , m_phase(phase)
    {
    }

    //==========================================================================

    MotionDataBuilderContext::MotionDataBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IEFXMotionGroup& motionGroup,
        EMotionFX::SkeletalMotion& motion, AZ::RC::Phase phase)
        : m_scene(scene)
        , m_group(motionGroup)
        , m_motion(motion)
        , m_phase(phase)
    {
    }

    MotionDataBuilderContext::MotionDataBuilderContext(const MotionDataBuilderContext& copyContext, AZ::RC::Phase phase)
        : m_scene(copyContext.m_scene)
        , m_group(copyContext.m_group)
        , m_motion(copyContext.m_motion)
        , m_phase(phase)
    {
    }

    //==========================================================================
    //  Actor
    //==========================================================================

    ActorGroupExportContext::ActorGroupExportContext(AZ::SceneAPI::Events::ExportEventContext& parent,
        const AZ::SceneAPI::DataTypes::IActorGroup& group, AZ::RC::Phase phase)
        : m_scene(parent.GetScene())
        , m_outputDirectory(parent.GetOutputDirectory())
        , m_group(group)
        , m_phase(phase)
    {
    }

    ActorGroupExportContext::ActorGroupExportContext(const AZ::SceneAPI::Containers::Scene & scene, const AZStd::string& outputDirectory,
        const AZ::SceneAPI::DataTypes::IActorGroup & group, AZ::RC::Phase phase)
        : m_scene(scene)
        , m_outputDirectory(outputDirectory)
        , m_group(group)
        , m_phase(phase)
    {
    }

    ActorGroupExportContext::ActorGroupExportContext(const ActorGroupExportContext & copyContext, AZ::RC::Phase phase)
        : m_scene(copyContext.m_scene)
        , m_outputDirectory(copyContext.m_outputDirectory)
        , m_group(copyContext.m_group)
        , m_phase(phase)
    {
    }

    //==========================================================================

    ActorBuilderContext::ActorBuilderContext(const AZ::SceneAPI::Containers::Scene& scene,
        const AZStd::string& outputDirectory, const AZ::SceneAPI::DataTypes::IActorGroup& actorGroup,
        EMotionFX::Actor* actor, AZ::RC::Phase phase)
        : m_scene(scene)
        , m_outputDirectory(outputDirectory)
        , m_group(actorGroup)
        , m_actor(actor)
        , m_phase(phase)
    {
    }

    ActorBuilderContext::ActorBuilderContext(const ActorBuilderContext & copyContext, AZ::RC::Phase phase)
        : m_scene(copyContext.m_scene)
        , m_outputDirectory(copyContext.m_outputDirectory)
        , m_group(copyContext.m_group)
        , m_actor(copyContext.m_actor)
        , m_phase(phase)
    {
    }

} // MotionCanvasPipeline
#endif //MOTIONCANVAS_GEM_ENABLED