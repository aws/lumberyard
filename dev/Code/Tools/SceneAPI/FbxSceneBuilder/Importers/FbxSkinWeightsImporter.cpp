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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxSkinWeightsImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinWeightData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            const AZStd::string FbxSkinWeightsImporter::s_skinWeightName = "SkinWeight_";

            FbxSkinWeightsImporter::FbxSkinWeightsImporter()
            {
                BindToCall(&FbxSkinWeightsImporter::ImportSkinWeights);
            }

            void FbxSkinWeightsImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxSkinWeightsImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult FbxSkinWeightsImporter::ImportSkinWeights(SceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Skin Weights");

                if (!IsSkinnedMesh(context.m_sourceNode))
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedSkinWeightsResult;

                for (int deformerIndex = 0; deformerIndex < context.m_sourceNode.GetMesh()->GetDeformerCount(FbxDeformer::eSkin); ++deformerIndex)
                {
                    AZ_TraceContext("Deformer Index", deformerIndex);
                    AZStd::shared_ptr<const FbxSDKWrapper::FbxSkinWrapper> fbxSkin = 
                        context.m_sourceNode.GetMesh()->GetSkin(deformerIndex);
                    if (!fbxSkin)
                    {
                        return Events::ProcessingResult::Failure;
                    }
                    AZStd::string skinWeightName = s_skinWeightName;
                    skinWeightName += AZStd::to_string(deformerIndex);

                    AZStd::shared_ptr<SceneData::GraphData::SkinWeightData> skinDeformer =
                        BuildSkinWeightData(context.m_sourceNode.GetMesh(), deformerIndex);
                
                    AZ_Assert(skinDeformer, "Failed to allocate skin weighting data.");
                    if (!skinDeformer)
                    {
                        combinedSkinWeightsResult += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Containers::SceneGraph::NodeIndex newIndex =
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, skinWeightName.c_str());

                    AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                    if (!newIndex.IsValid())
                    {
                        combinedSkinWeightsResult += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Events::ProcessingResult skinWeightsResult;
                    SceneAttributeDataPopulatedContext dataPopulated(context, skinDeformer, newIndex, skinWeightName);
                    skinWeightsResult = Events::Process(dataPopulated);

                    if (skinWeightsResult != Events::ProcessingResult::Failure)
                    {
                        skinWeightsResult = AddAttributeDataNodeWithContexts(dataPopulated);
                    }
                
                    combinedSkinWeightsResult += skinWeightsResult;
                }

                return combinedSkinWeightsResult.GetResult();
            }

            AZStd::shared_ptr<SceneData::GraphData::SkinWeightData> FbxSkinWeightsImporter::BuildSkinWeightData(
                const std::shared_ptr<const FbxSDKWrapper::FbxMeshWrapper>& fbxMesh, int skinIndex)
            {
                const AZStd::shared_ptr<const FbxSDKWrapper::FbxSkinWrapper> fbxSkin = fbxMesh->GetSkin(skinIndex);
                AZ_Assert(fbxSkin, "BuildSkinWeightData was called for index %i which doesn't contain a skin deformer.", 
                    skinIndex);

                if (!fbxSkin)
                {
                    return nullptr;
                }
                AZStd::shared_ptr<SceneData::GraphData::SkinWeightData> skinWeightData =
                    AZStd::make_shared<SceneData::GraphData::SkinWeightData>();

                int controlPointCount = fbxMesh->GetControlPointsCount();
                skinWeightData->ResizeContainerSpace(controlPointCount);

                int clusterCount = fbxSkin->GetClusterCount();

                for (int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
                {
                    int controlPointCount = fbxSkin->GetClusterControlPointIndicesCount(clusterIndex);
                    AZStd::shared_ptr<const FbxSDKWrapper::FbxNodeWrapper> fbxLink = fbxSkin->GetClusterLink(clusterIndex);
                    
                    if (!fbxLink)
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "FBX data contains null skin cluster link at index %i", clusterIndex);
                        continue;
                    }

                    AZStd::string boneName = fbxLink->GetName();
                    int boneId = skinWeightData->GetBoneId(boneName);

                    for (int pointIndex = 0; pointIndex < controlPointCount; ++pointIndex)
                    {
                        SceneAPI::DataTypes::ISkinWeightData::Link link;
                        link.boneId = boneId;
                        link.weight = aznumeric_caster(fbxSkin->GetClusterControlPointWeight(clusterIndex, pointIndex));
                        skinWeightData->AppendLink(fbxSkin->GetClusterControlPointIndex(clusterIndex, pointIndex), link);
                    }
                }

                return skinWeightData;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ