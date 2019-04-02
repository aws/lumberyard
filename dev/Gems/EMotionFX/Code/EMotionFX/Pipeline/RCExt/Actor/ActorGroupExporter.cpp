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

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <SceneAPIExt/Rules/ActorPhysicsSetupRule.h>

#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <RCExt/Actor/ActorGroupExporter.h>
#include <RCExt/ExportContexts.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainer = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        ActorGroupExporter::ActorGroupExporter()
        {
            BindToCall(&ActorGroupExporter::ProcessContext);
        }

        void ActorGroupExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ActorGroupExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
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

            const Group::IActorGroup& actorGroup = context.m_group;
            ActorBuilderContext actorBuilderContext(context.m_scene, context.m_outputDirectory, actorGroup, actor, AZ::RC::Phase::Construction);
            result += SceneEvents::Process(actorBuilderContext);
            result += SceneEvents::Process<ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Filling);
            result += SceneEvents::Process<ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Finalizing);

            // Check if there is meta data and apply it to the actor.
            AZStd::string metaDataString;
            if (Rule::MetaDataRule::LoadMetaData(actorGroup, metaDataString))
            {
                if (!CommandSystem::MetaData::ApplyMetaDataOnActor(actor, metaDataString))
                {
                    AZ_Error("EMotionFX", false, "Applying meta data to '%s' failed.", filename.c_str());
                }
            }

            AZStd::shared_ptr<EMotionFX::PhysicsSetup> physicsSetup;
            if (EMotionFX::Pipeline::Rule::LoadGromGroup<EMotionFX::Pipeline::Rule::ActorPhysicsSetupRule, AZStd::shared_ptr<EMotionFX::PhysicsSetup>>(actorGroup, physicsSetup))
            {
                actor->SetPhysicsSetup(physicsSetup);
            }

            ExporterLib::SaveActor(filename, actor, MCore::Endian::ENDIAN_LITTLE);

#ifdef EMOTIONFX_ACTOR_DEBUG
            // Use there line to create a log file and inspect detail debug info
            AZStd::string folderPath;
            AzFramework::StringFunc::Path::GetFolderPath(filename.c_str(), folderPath);
            AZStd::string logFilename = folderPath;
            logFilename += "EMotionFXExporter_Log.txt";
            MCore::GetLogManager().CreateLogFile(logFilename.c_str());
            EMotionFX::GetImporter().SetLogDetails(true);
            filename += ".xac";

            // use this line to load the actor from the saved actor file
            EMotionFX::Actor* testLoadingActor = EMotionFX::GetImporter().LoadActor(AZStd::string(filename.c_str()));
            MCore::Destroy(testLoadingActor);
#endif // EMOTIONFX_ACTOR_DEBUG

            static AZ::Data::AssetType emotionFXActorAssetType("{F67CC648-EA51-464C-9F5D-4A9CE41A7F86}"); // from ActorAsset.h in EMotionFX Gem
            context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), emotionFXActorAssetType);

            // Destroy the actor after save.
            MCore::Destroy(actor);

            return result.GetResult();
        }
    } // namespace Pipeline
} // namespace EMotionFX
