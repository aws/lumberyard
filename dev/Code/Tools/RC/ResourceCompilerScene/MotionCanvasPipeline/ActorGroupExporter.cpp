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

#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ActorGroupExporter.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ExportContexts.h>

#include <AzFramework/StringFunc/StringFunc.h>

// TODO: Including headers from gem is not right. Need to fix this.
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>

#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IActorGroup.h>
#include <SceneAPI/SceneData/EMotionFX/Rules/MetaDataRule.h>


namespace MotionCanvasPipeline
{
    namespace SceneEvents = AZ::SceneAPI::Events;
    namespace SceneUtil = AZ::SceneAPI::Utilities;
    namespace SceneContainer = AZ::SceneAPI::Containers;
    namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

    ActorGroupExporter::ActorGroupExporter()
        : CallProcessorBinder()
    {
        BindToCall(&ActorGroupExporter::ProcessContext);
        ActivateBindings();
    }

    SceneEvents::ProcessingResult ActorGroupExporter::ProcessContext(ActorGroupExportContext& context) const
    {
        if (context.m_phase != AZ::RC::Phase::Filling)
        {
            return SceneEvents::ProcessingResult::Ignored;
        }

        const AZStd::string& groupName = context.m_group.GetName();
        AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(groupName, context.m_outputDirectory, "");
        if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
        {
            return SceneEvents::ProcessingResult::Failure;
        }

        EMotionFX::Actor* actor = EMotionFX::Actor::Create(groupName.c_str());
        if (!actor)
        {
            return SceneEvents::ProcessingResult::Failure;
        }

        SceneEvents::ProcessingResultCombiner result;

        const AZ::SceneAPI::DataTypes::IActorGroup& actorGroup = context.m_group;
        ActorBuilderContext actorBuilderContext(context.m_scene, context.m_outputDirectory, actorGroup, actor, AZ::RC::Phase::Construction);
        result += SceneEvents::Process(actorBuilderContext);
        result += SceneEvents::Process<ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Filling);
        result += SceneEvents::Process<ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Finalizing);

        // Check if there is meta data and apply it to the actor.
        AZStd::string metaDataString;
        if (AZ::SceneAPI::SceneData::MetaDataRule::LoadMetaData(actorGroup, metaDataString))
        {
            if (!CommandSystem::MetaData::ApplyMetaDataOnActor(actor, metaDataString))
            {
                AZ_Error("EMotionFX", false, "Applying meta data to '%s' failed.", filename.c_str());
            }
        }

        ExporterLib::SaveActor(filename, actor, MCore::Endian::ENDIAN_LITTLE);

#ifdef EMOTIONFX_ACTOR_DEBUG
        // Use there line to create a log file and inspect detail debug info
        AZStd::string folderPath;
        AzFramework::StringFunc::Path::GetFolderPath(filename.c_str(), folderPath);
        AZStd::string logFilename = folderPath;
        logFilename += "MotionCanvasExporter_Log.txt";
        MCore::GetLogManager().CreateLogFile(logFilename.c_str());
        EMotionFX::GetImporter().SetLogDetails(true);
        filename += ".xac";

        // use this line to load the actor from the saved actor file
        EMotionFX::Actor* testLoadingActor = EMotionFX::GetImporter().LoadActor(MCore::String(filename.c_str()));
        MCore::Destroy(testLoadingActor);
#endif // EMOTIONFX_ACTOR_DEBUG

        // Destroy the actor after save.
        MCore::Destroy(actor);

        return result.GetResult();
    }

} // MotionCanvasPipeline

#endif // MOTIONCANVAS_GEM_ENABLED
