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

#include <RC/ResourceCompilerScene/MotionCanvasPipeline/MotionExporter.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ExportContexts.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IEFXMotionGroup.h>

#include <AzToolsFramework/Debug/TraceContext.h>

namespace MotionCanvasPipeline
{

    namespace SceneContainers = AZ::SceneAPI::Containers;
    namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

    MotionExporter::MotionExporter()
        : CallProcessorBinder()
    {
        BindToCall(&MotionExporter::ProcessContext);
        ActivateBindings();
    }

    SceneEvents::ProcessingResult MotionExporter::ProcessContext(SceneEvents::ExportEventContext& context) const
    {
        const SceneContainers::SceneManifest& manifest = context.GetScene().GetManifest();

        auto valueStorage = manifest.GetValueStorage();
        auto view = SceneContainers::MakeDerivedFilterView<SceneDataTypes::IEFXMotionGroup>(valueStorage);

        SceneEvents::ProcessingResultCombiner result;
        for (const SceneDataTypes::IEFXMotionGroup& motionGroup : view)
        {
            AZ_TraceContext("Animation group", motionGroup.GetName());
            result += SceneEvents::Process<MotionGroupExportContext>(context, motionGroup, AZ::RC::Phase::Construction);
            result += SceneEvents::Process<MotionGroupExportContext>(context, motionGroup, AZ::RC::Phase::Filling);
            result += SceneEvents::Process<MotionGroupExportContext>(context, motionGroup, AZ::RC::Phase::Finalizing);
        }
        return result.GetResult();
    }

} // MotionCanvasPipeline

#endif // MOTIONCANVAS_GEM_ENABLED
