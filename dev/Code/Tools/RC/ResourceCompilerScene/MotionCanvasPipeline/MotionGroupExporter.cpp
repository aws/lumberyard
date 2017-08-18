

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

#include <RC/ResourceCompilerScene/MotionCanvasPipeline/MotionGroupExporter.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ExportContexts.h>

#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IEFXMotionGroup.h>

// TODO: Including headers directly from a Gem is not modular, and can be removed once the pipeline supports Gem-based code. 
// This is contained within #ifdef MOTIONCANVAS_GEM_ENABLED, so it won't cause compile errors when the Gem isn't present.
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <SceneAPI/SceneData/EMotionFX/Rules/MetaDataRule.h>


namespace MotionCanvasPipeline
{
    namespace SceneEvents = AZ::SceneAPI::Events;
    namespace SceneUtil = AZ::SceneAPI::Utilities;
    namespace SceneContainer = AZ::SceneAPI::Containers;
    namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

    const AZStd::string MotionGroupExporter::s_fileExtension = "motion";

    MotionGroupExporter::MotionGroupExporter()
        : CallProcessorBinder()
    {
        BindToCall(&MotionGroupExporter::ProcessContext);
        ActivateBindings();
    }

    SceneEvents::ProcessingResult MotionGroupExporter::ProcessContext(MotionGroupExportContext& context) const
    {
        if (context.m_phase != AZ::RC::Phase::Filling)
        {
            return SceneEvents::ProcessingResult::Ignored;
        }

        const AZStd::string& groupName = context.m_group.GetName();
        AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(groupName, context.m_outputDirectory, s_fileExtension);
        if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
        {
            return SceneEvents::ProcessingResult::Failure;
        }

        EMotionFX::SkeletalMotion* motion = EMotionFX::SkeletalMotion::Create(groupName.c_str());
        if (!motion)
        {
            return SceneEvents::ProcessingResult::Failure;
        }
        motion->SetUnitType(MCore::Distance::UNITTYPE_METERS);

        SceneEvents::ProcessingResultCombiner result;

        const AZ::SceneAPI::DataTypes::IEFXMotionGroup& motionGroup = context.m_group;
        MotionDataBuilderContext dataBuilderContext(context.m_scene, motionGroup, *motion, AZ::RC::Phase::Construction);
        result += SceneEvents::Process(dataBuilderContext);
        result += SceneEvents::Process<MotionDataBuilderContext>(dataBuilderContext, AZ::RC::Phase::Filling);
        result += SceneEvents::Process<MotionDataBuilderContext>(dataBuilderContext, AZ::RC::Phase::Finalizing);

        // Check if there is meta data and apply it to the motion.
        AZStd::string metaDataString;
        if (AZ::SceneAPI::SceneData::MetaDataRule::LoadMetaData(motionGroup, metaDataString))
        {
            if (!CommandSystem::MetaData::ApplyMetaDataOnMotion(motion, metaDataString))
            {
                AZ_Error("EMotionFX", false, "Applying meta data to '%s' failed.", filename.c_str());
            }
        }

        ExporterLib::SaveSkeletalMotion(filename, motion, MCore::Endian::ENDIAN_LITTLE, false);

        // The motion object served the purpose of exporting motion and is no longer needed
        MCore::Destroy(motion);
        motion = nullptr;

        return result.GetResult();
    }

} // MotionCanvasPipeline

#endif // MOTIONCANVAS_GEM_ENABLED
