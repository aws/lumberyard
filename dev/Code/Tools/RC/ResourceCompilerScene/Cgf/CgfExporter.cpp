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

#include <RC/ResourceCompilerScene/Cgf/CgfExporter.h>
#include <Cry_Geo.h>
#include <ConvertContext.h>
#include <CGFContent.h>

#include <AzToolsFramework/Debug/TraceContext.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>

#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneEvents = AZ::SceneAPI::Events;

        CgfExporter::CgfExporter(IConvertContext* convertContext)
            : CallProcessorConnector()
            , m_convertContext(convertContext)
        {
        }

        SceneEvents::ProcessingResult CgfExporter::Process(SceneEvents::ICallContext* context)
        {
            SceneEvents::ExportEventContext* exportContext = azrtti_cast<SceneEvents::ExportEventContext*>(context);
            if (exportContext)
            {
                AZ_TraceContext("Scene name", exportContext->GetScene().GetName());
                AZ_TraceContext("Source file", exportContext->GetScene().GetSourceFilename());
                AZ_TraceContext("Output path", exportContext->GetOutputDirectory());

                const SceneContainers::SceneManifest& manifest = exportContext->GetScene().GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = SceneContainers::MakeDerivedFilterView<SceneDataTypes::IMeshGroup>(valueStorage);
                
                SceneEvents::ProcessingResultCombiner result;
                for (const SceneDataTypes::IMeshGroup& meshGroup : view)
                {
                    AZ_TraceContext("Mesh group", meshGroup.GetName());
                    result += SceneEvents::Process<CgfGroupExportContext>(*exportContext, meshGroup, Phase::Construction);
                    result += SceneEvents::Process<CgfGroupExportContext>(*exportContext, meshGroup, Phase::Filling);
                    result += SceneEvents::Process<CgfGroupExportContext>(*exportContext, meshGroup, Phase::Finalizing);
                }
                return result.GetResult();
            }
            else
            {
                return SceneEvents::ProcessingResult::Ignored;
            }
        }
    } // RC
} // AZ
