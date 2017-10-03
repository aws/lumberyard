

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

#include "stdafx.h"
#include <CGFContent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <RC/ResourceCompilerPC/CGF/LoaderCAF.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Caf/CafExportContexts.h>
#include <RC/ResourceCompilerScene/Caf/CafGroupExporter.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainer = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        const AZStd::string CafGroupExporter::s_fileExtension = "i_caf";
        const float CafGroupExporter::s_defaultFrameRate = 30.0f;

        CafGroupExporter::CafGroupExporter(IAssetWriter* writer, IConvertContext* convertContext)
            : m_assetWriter(writer)
            , m_convertContext(convertContext)
        {
            BindToCall(&CafGroupExporter::ProcessContext);
            ActivateBindings();
        }

        SceneEvents::ProcessingResult CafGroupExporter::ProcessContext(CafGroupExportContext& context) const
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(context.m_group.GetName(), context.m_outputDirectory, s_fileExtension);
            if (filename.empty() || !SceneUtil::FileUtilities::EnsureTargetFolderExists(filename))
            {
                return SceneEvents::ProcessingResult::Failure;
            }

            SceneEvents::ProcessingResultCombiner result;

            CContentCGF cgfContent(filename.c_str());
            CInternalSkinningInfo controllerSkinningInfo;
            SaveAnimationConfiguration(context, controllerSkinningInfo);

            const SceneAPI::DataTypes::IAnimationGroup& animationGroup = context.m_group;
            AnimationExportContext animationContextConstruction(context.m_scene, animationGroup, controllerSkinningInfo, Phase::Construction);
            ConfigureCafContent(cgfContent);
            result += SceneEvents::Process(animationContextConstruction);
            result += SceneEvents::Process<AnimationExportContext>(animationContextConstruction, Phase::Filling);
            result += SceneEvents::Process<AnimationExportContext>(animationContextConstruction, Phase::Finalizing);

            if (m_assetWriter)
            {
                if (m_assetWriter->WriteCAF(&cgfContent, &context.m_group, &controllerSkinningInfo, m_convertContext))
                {
                    // The Caf writer expects a i_caf as input in order to produce an optimized and compressed .caf file.
                    AzFramework::StringFunc::Path::ReplaceExtension(filename, ".caf");

                    // This is a new guid as caf doesn't have type as it's an input to Mannequin.
                    static const AZ::Data::AssetType cryAnimationAssetType("{10121AD6-2B3E-442C-AF5B-6F563614ABC6}");
                    context.m_products.AddProduct(AZStd::move(filename), context.m_group.GetId(), cryAnimationAssetType);
                }
                else
                {
                    result += SceneEvents::ProcessingResult::Failure;
                }
            }
            else
            {
                result += SceneEvents::ProcessingResult::Failure;
            }
            return result.GetResult();
        }

        //
        // Implemented by referencing to ColladaScene::CreateControllerSkinningInfo
        //
        void CafGroupExporter::SaveAnimationConfiguration(CafGroupExportContext& context, CInternalSkinningInfo& controllerSkinningInfo) const
        {
            const SceneAPI::DataTypes::IAnimationGroup& animationGroup = context.m_group; 
            controllerSkinningInfo.m_nStart = animationGroup.GetStartFrame();
            controllerSkinningInfo.m_nEnd = animationGroup.GetEndFrame();
            controllerSkinningInfo.m_nTicksPerFrame = TICKS_CONVERT;
            controllerSkinningInfo.m_secsPerTick = 1.0f / s_defaultFrameRate / TICKS_CONVERT;
        }

        void CafGroupExporter::ConfigureCafContent(CContentCGF& content) const
        {
            CExportInfoCGF* exportInfo = content.GetExportInfo();

            exportInfo->bMergeAllNodes = true;
            exportInfo->bUseCustomNormals = false;
            exportInfo->bCompiledCGF = false;
            exportInfo->bHavePhysicsProxy = false;
            exportInfo->bHaveAutoLods = false;
            exportInfo->bNoMesh = true;
            exportInfo->b8WeightsPerVertex = false;
            exportInfo->bWantF32Vertices = false;
            exportInfo->authorToolVersion = 1;
        }
    }
}
