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

#include <NvCloth_precompiled.h>

#include <AzToolsFramework/Debug/TraceContext.h>

#include <Cry_Geo.h> // Needed for CGFContent.h
#include <CGFContent.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Color.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IClothRule.h>

#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>

#include <Pipeline/RCExt/CgfClothExporter.h>

namespace NvCloth
{
    namespace Pipeline
    {
        namespace
        {
            // Index for the Vertex color stream that contains the cloth inverse masses.
            const int ClothVertexBufferStreamIndex = 1;

            const SMeshColor DefaultInverseMassColor(255, 255, 255, 255);
        }

        void BuildClothMesh(CMesh& mesh,
            const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshVertexColorData>& vertexColorData);

        CgfClothExporter::CgfClothExporter()
        {
            // Binding the processing functions so when exporters call
            // SceneAPI::Events::Process<Context>() these functions will
            // get called if their Context was used.
            BindToCall(&CgfClothExporter::ProcessMeshNodeContext);
            BindToCall(&CgfClothExporter::ProcessContainerContext);
        }

        void CgfClothExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<CgfClothExporter, AZ::SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        AZ::SceneAPI::Events::ProcessingResult CgfClothExporter::ProcessContainerContext(AZ::RC::ContainerExportContext& context) const
        {
            if (!context.m_group.GetRuleContainerConst().ContainsRuleOfType<AZ::SceneAPI::DataTypes::IClothRule>())
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            if (context.m_phase == AZ::RC::Phase::Finalizing)
            {
                if (context.m_container.GetExportInfo()->bMergeAllNodes)
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow,
                        "Mesh group '%s' has cloth rules and trying to merge all nodes.",
                        context.m_group.GetName().c_str());
                    return AZ::SceneAPI::Events::ProcessingResult::Failure;
                }
            }
            else
            {
                // If the current mesh group contains a cloth rule it should not merge all the nodes.
                context.m_container.GetExportInfo()->bMergeAllNodes = false;
            }

            return AZ::SceneAPI::Events::ProcessingResult::Success;
        }

        AZ::SceneAPI::Events::ProcessingResult CgfClothExporter::ProcessMeshNodeContext(AZ::RC::MeshNodeExportContext& context) const
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            const auto& graph = context.m_scene.GetGraph();
            const auto& group = context.m_group;

            // Go through all cloth rules in the group to check if any chose the current mesh node.
            const auto& groupRules = group.GetRuleContainerConst();
            for (size_t ruleIndex = 0; ruleIndex < groupRules.GetRuleCount(); ++ruleIndex)
            {
                const AZ::SceneAPI::DataTypes::IClothRule* rule = azrtti_cast<const AZ::SceneAPI::DataTypes::IClothRule*>(groupRules.GetRule(ruleIndex).get());
                if (!rule)
                {
                    continue;
                }

                // Reached a cloth rule for this mesh node?
                if (context.m_nodeName != rule->GetMeshNodeName())
                {
                    continue;
                }

                // If a mesh color stream is already present in the mesh it means there are more than 1 cloth rules with the same mesh.
                // That is not allowed, using the first one and ignoring the others.
                if (context.m_mesh.GetStreamPtr<SMeshColor>(CMesh::COLORS, ClothVertexBufferStreamIndex) != nullptr)
                {
                    AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Different cloth rules used the same mesh node but selected different color streams, that's not allowed.");
                    continue;
                }

                // Find the Vertex Color Data for the cloth
                AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshVertexColorData> vertexColorData;
                if (!rule->IsVertexColorStreamDisabled() &&
                    !rule->GetVertexColorStreamName().empty())
                {
                    AZ::SceneAPI::Containers::SceneGraph::NodeIndex vertexColorNodeIndex = graph.Find(context.m_nodeIndex, rule->GetVertexColorStreamName());
                    vertexColorData = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshVertexColorData*>(graph.GetNodeContent(vertexColorNodeIndex));
                    if (vertexColorData)
                    {
                        if (context.m_mesh.GetVertexCount() != vertexColorData->GetCount())
                        {
                            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow,
                                "Number of vertices in the mesh (%i) don't match with the number of stored vertex color stream (%i).",
                                context.m_mesh.GetVertexCount(), vertexColorData->GetCount());
                            AZ::SceneAPI::Events::ProcessingResult::Failure;
                        }
                    }
                    else
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow,
                            "Vertex color stream '%s' not found for mesh node '%s'.",
                            rule->GetVertexColorStreamName().c_str(),
                            rule->GetMeshNodeName().c_str());
                    }
                }

                BuildClothMesh(context.m_mesh, vertexColorData);
            }

            return AZ::SceneAPI::Events::ProcessingResult::Success;
        }

        void BuildClothMesh(CMesh& mesh,
            const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshVertexColorData>& vertexColorData)
        {
            // Allocate and get the vertex color stream for cloth
            mesh.ReallocStream(CMesh::COLORS, ClothVertexBufferStreamIndex, mesh.GetVertexCount());
            auto meshColorStream = mesh.GetStreamPtr<SMeshColor>(CMesh::COLORS, ClothVertexBufferStreamIndex);
            AZ_Assert(meshColorStream, "Mesh color stream is invalid");

            // Set the data to the mesh.
            if (vertexColorData)
            {
                for (int i = 0; i < mesh.GetVertexCount(); ++i)
                {
                    const auto& color = vertexColorData->GetColor(i);

                    AZ::Color inverseMassColor(
                        AZ::GetClamp<float>(color.red, 0.0f, 1.0f),
                        AZ::GetClamp<float>(color.green, 0.0f, 1.0f),
                        AZ::GetClamp<float>(color.blue, 0.0f, 1.0f),
                        AZ::GetClamp<float>(color.alpha, 0.0f, 1.0f));

                    meshColorStream[i] = SMeshColor(
                        inverseMassColor.GetR8(),
                        inverseMassColor.GetG8(),
                        inverseMassColor.GetB8(),
                        inverseMassColor.GetA8());
                }
            }
            else
            {
                for (int i = 0; i < mesh.GetVertexCount(); ++i)
                {
                    meshColorStream[i] = DefaultInverseMassColor;
                }
            }
        }
    } // namespace Pipeline
} // namespace NvCloth
