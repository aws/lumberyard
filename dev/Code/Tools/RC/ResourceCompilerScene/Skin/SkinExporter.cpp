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

#include <RC/ResourceCompilerScene/Skin/SkinExporter.h>
#include <Cry_Geo.h>
#include <ConvertContext.h>
#include <CGFContent.h>

#include <AzToolsFramework/Debug/TraceContext.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <RC/ResourceCompilerScene/Skin/SkinExportContexts.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneEvents = AZ::SceneAPI::Events;

        SkinExporter::SkinExporter(IConvertContext* convertContext)
            : CallProcessorConnector()
            , m_convertContext(convertContext)
        {
        }

        SceneEvents::ProcessingResult SkinExporter::Process(SceneEvents::ICallContext* context)
        {
            SceneEvents::ExportEventContext* exportContext = azrtti_cast<SceneEvents::ExportEventContext*>(context);
            if (exportContext)
            {
                const SceneContainers::SceneManifest& manifest = exportContext->GetScene().GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = SceneContainers::MakeDerivedFilterView<SceneDataTypes::ISkinGroup>(valueStorage);

                SceneEvents::ProcessingResultCombiner result;
                for (const SceneDataTypes::ISkinGroup& skinGroup : view)
                {
                    AZ_TraceContext("Skin Group", skinGroup.GetName());
                    result += SceneEvents::Process<SkinGroupExportContext>(*exportContext, skinGroup, Phase::Construction);
                    result += SceneEvents::Process<SkinGroupExportContext>(*exportContext, skinGroup, Phase::Filling);
                    result += SceneEvents::Process<SkinGroupExportContext>(*exportContext, skinGroup, Phase::Finalizing);
                }
                return result.GetResult();
            }
            else
            {
                return SceneEvents::ProcessingResult::Ignored;
            }
        }
    }
}
