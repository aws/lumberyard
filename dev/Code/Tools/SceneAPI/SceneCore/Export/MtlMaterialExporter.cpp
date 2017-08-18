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

#include <memory>
#include <SceneAPI/SceneCore/Export/MtlMaterialExporter.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <GFxFramework/MaterialIO/Material.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMeshAdvancedRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IPhysicsRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Export
        {
            MtlMaterialExporter::SaveMaterialResult MtlMaterialExporter::SaveMaterialGroup(const DataTypes::ISceneNodeGroup& sceneNodeGroup,
                const Containers::Scene& scene, const AZStd::string& textureRootPath)
            {
                m_textureRootPath = textureRootPath;
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, m_textureRootPath);
                return BuildMaterialGroup(sceneNodeGroup, scene, m_materialGroup);
            }

            bool MtlMaterialExporter::WriteToFile(const char* filePath, const Containers::Scene& scene)
            {
                if (AZ::IO::SystemFile::Exists(filePath) && !AZ::IO::SystemFile::IsWritable(filePath))
                {
                    return false;
                }
                return WriteMaterialFile(filePath, m_materialGroup);
            }

            MtlMaterialExporter::SaveMaterialResult MtlMaterialExporter::BuildMaterialGroup(const DataTypes::ISceneNodeGroup& sceneNodeGroup, const Containers::Scene& scene,
                MaterialGroup& materialGroup) const
            {
                //Default rule settings for materials.
                materialGroup.m_materials.clear();
                materialGroup.m_removeMaterials = false;
                materialGroup.m_updateMaterials = false;

                const AZ::SceneAPI::Containers::RuleContainer& rules = sceneNodeGroup.GetRuleContainerConst();

                AZStd::shared_ptr<const DataTypes::IPhysicsRule> physicsRule = rules.FindFirstByType<DataTypes::IPhysicsRule>();
                AZStd::shared_ptr<const DataTypes::IMaterialRule> materialRule = rules.FindFirstByType<DataTypes::IMaterialRule>();

                if (materialRule)
                {
                    materialGroup.m_removeMaterials = materialRule->RemoveUnusedMaterials();
                    materialGroup.m_updateMaterials = materialRule->UpdateMaterials();
                }

                AZStd::unordered_set<AZStd::string> usedMaterials;
                const Containers::SceneGraph& sceneGraph = scene.GetGraph();

                AZStd::vector<AZStd::string> physTargetNodes;

                if (physicsRule)
                {
                    physTargetNodes = Utilities::SceneGraphSelector::GenerateTargetNodes(sceneGraph, physicsRule->GetSceneNodeSelectionList(), Utilities::SceneGraphSelector::IsMesh);
                    for (auto& nodeName : physTargetNodes)
                    {
                        auto index = sceneGraph.Find(nodeName);
                        //if we find any valid nodes add a physics rule and stop.
                        if (index.IsValid())
                        {
                            MaterialInfo info;
                            info.m_name = GFxFramework::MaterialExport::g_stringPhysicsNoDraw;
                            info.m_materialData = nullptr;
                            info.m_usesVertexColoring = false;
                            info.m_physicalize = true;
                            materialGroup.m_materials.push_back(info);
                            break;
                        }
                    }
                }

                //If we have a material rule process materials
                if (materialRule)
                {
                    // create materials for render nodes
                    AZStd::vector<AZStd::string> renderTargetNodes = Utilities::SceneGraphSelector::GenerateTargetNodes(sceneGraph, sceneNodeGroup.GetSceneNodeSelectionList(), Utilities::SceneGraphSelector::IsMesh);
                    for (auto& nodeName : renderTargetNodes)
                    {
                        Containers::SceneGraph::NodeIndex index = sceneGraph.Find(nodeName);
                        if (index.IsValid())
                        {
                            auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(sceneGraph, index,
                                sceneGraph.GetContentStorage().begin(), true);
                            for (auto it = view.begin(); it != view.end(); ++it)
                            {
                                if ((*it) && (*it)->RTTI_IsTypeOf(DataTypes::IMaterialData::TYPEINFO_Uuid()))
                                {
                                    const Containers::SceneGraph::Name& name =
                                        sceneGraph.GetNodeName(sceneGraph.ConvertToNodeIndex(it.GetHierarchyIterator()));
                                    if (usedMaterials.find(name.GetName()) == usedMaterials.end())
                                    {
                                        AZStd::string nodeName = name.GetName();
                                        MaterialInfo info;
                                        info.m_name = nodeName;
                                        info.m_materialData = azrtti_cast<const DataTypes::IMaterialData*>(*it);
                                        info.m_usesVertexColoring = UsesVertexColoring(sceneNodeGroup, scene, it.GetHierarchyIterator());
                                        info.m_physicalize = false;
                                        materialGroup.m_materials.push_back(info);
                                        usedMaterials.insert(AZStd::move(nodeName));
                                    }
                                }
                            }
                        }
                    }
                }

                return materialGroup.m_materials.size() == 0 ? SaveMaterialResult::Skipped : SaveMaterialResult::Success;
            }


            // Check if there's a mesh advanced rule of the given scene node group that
            //      specifically controls vertex coloring. If no rule exists for group, check if there are any
            //      vertex color streams, which would automatically enable the vertex coloring feature.
            bool MtlMaterialExporter::UsesVertexColoring(const DataTypes::ISceneNodeGroup& sceneNodeGroup, const Containers::Scene& scene,
                Containers::SceneGraph::HierarchyStorageConstIterator materialNode) const
            {
                const Containers::SceneGraph& graph = scene.GetGraph();
                Containers::SceneGraph::NodeIndex meshNodeIndex = graph.GetNodeParent(graph.ConvertToNodeIndex(materialNode));

                AZStd::shared_ptr<const DataTypes::IMeshAdvancedRule> rule = sceneNodeGroup.GetRuleContainerConst().FindFirstByType<DataTypes::IMeshAdvancedRule>();
                if (rule)
                {
                    return !rule->IsVertexColorStreamDisabled() && !rule->GetVertexColorStreamName().empty();
                }
                else
                {
                    if (DoesMeshNodeHaveColorStreamChild(scene, meshNodeIndex))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool MtlMaterialExporter::WriteMaterialFile(const char* filePath, MaterialGroup& materialGroup) const
            {
                if (materialGroup.m_materials.size() == 0)
                {
                    // Nothing to write to return true.
                    return true;
                }

                AZ::GFxFramework::MaterialGroup matGroup;
                AZ::GFxFramework::MaterialGroup doNotRemoveGroup;

                //Open the MTL file for read if it exists 
                bool fileRead = matGroup.ReadMtlFile(filePath);
                AZ_TraceContext("MTL File Name", filePath);
                if (fileRead)
                {
                    AZ_TracePrintf(Utilities::LogWindow, "MTL File found and will be updated as needed");
                }
                else
                {
                    AZ_TracePrintf(Utilities::LogWindow, "No existing MTL file found. A new one will be generated.");
                }

                bool hasPhysicalMaterial = false;

                for (const auto& material : materialGroup.m_materials)
                {
                    AZStd::shared_ptr<AZ::GFxFramework::IMaterial> mat = AZStd::make_shared<AZ::GFxFramework::Material>();
                    mat->EnableUseVertexColor(material.m_usesVertexColoring);
                    mat->EnablePhysicalMaterial(material.m_physicalize);
                    hasPhysicalMaterial |= material.m_physicalize;
                    mat->SetName(material.m_name);
                    if (material.m_materialData)
                    {
                        mat->SetTexture(AZ::GFxFramework::TextureMapType::Diffuse,
                            Utilities::FileUtilities::GetRelativePath(
                                material.m_materialData->GetTexture(DataTypes::IMaterialData::TextureMapType::Diffuse)
                                , m_textureRootPath));

                        mat->SetTexture(AZ::GFxFramework::TextureMapType::Specular,
                            Utilities::FileUtilities::GetRelativePath(
                                material.m_materialData->GetTexture(DataTypes::IMaterialData::TextureMapType::Specular)
                                , m_textureRootPath));

                        mat->SetTexture(AZ::GFxFramework::TextureMapType::Bump,
                            Utilities::FileUtilities::GetRelativePath(
                                material.m_materialData->GetTexture(DataTypes::IMaterialData::TextureMapType::Bump)
                                , m_textureRootPath));

                        mat->SetDiffuseColor(material.m_materialData->GetDiffuseColor());
                        mat->SetSpecularColor(material.m_materialData->GetSpecularColor());
                        mat->SetEmissiveColor(material.m_materialData->GetEmissiveColor());
                        mat->SetOpacity(material.m_materialData->GetOpacity());
                        mat->SetShininess(material.m_materialData->GetShininess());
                    }
                    size_t matIndex = matGroup.FindMaterialIndex(material.m_name);
                    if (materialGroup.m_updateMaterials && matIndex != GFxFramework::MaterialExport::g_materialNotFound)
                    {
                        AZStd::shared_ptr<GFxFramework::IMaterial> origMat = matGroup.GetMaterial(matIndex);
                        origMat->SetName(mat->GetName());
                        origMat->EnableUseVertexColor(mat->UseVertexColor());
                        origMat->EnablePhysicalMaterial(mat->IsPhysicalMaterial());
                        origMat->SetTexture(GFxFramework::TextureMapType::Diffuse, mat->GetTexture(GFxFramework::TextureMapType::Diffuse));
                        origMat->SetTexture(GFxFramework::TextureMapType::Specular, mat->GetTexture(GFxFramework::TextureMapType::Specular));
                        origMat->SetTexture(GFxFramework::TextureMapType::Bump, mat->GetTexture(GFxFramework::TextureMapType::Bump));
                    }
                    //This rule could change independently of an update material flag as it is set in the advanced rule.
                    else if (matIndex != GFxFramework::MaterialExport::g_materialNotFound)
                    {
                        AZStd::shared_ptr<GFxFramework::IMaterial> origMat = matGroup.GetMaterial(matIndex);
                        origMat->EnableUseVertexColor(mat->UseVertexColor());
                    }
                    else
                    {
                        matGroup.AddMaterial(mat);
                    }

                    if (materialGroup.m_removeMaterials)
                    {
                        doNotRemoveGroup.AddMaterial(mat);
                    }
                }

                //Remove a physical material if one had been added previously. 
                if (!hasPhysicalMaterial)
                {
                    matGroup.RemoveMaterial(GFxFramework::MaterialExport::g_stringPhysicsNoDraw);
                }

                //Remove any unused materials
                if (materialGroup.m_removeMaterials)
                {
                    AZStd::vector<AZStd::string> removeNames;
                    for (size_t i = 0; i < matGroup.GetMaterialCount(); ++i)
                    {
                        if (doNotRemoveGroup.FindMaterialIndex(matGroup.GetMaterial(i)->GetName()) == GFxFramework::MaterialExport::g_materialNotFound)
                        {
                            removeNames.push_back(matGroup.GetMaterial(i)->GetName());
                        }
                    }

                    for (const auto& name : removeNames)
                    {
                        matGroup.RemoveMaterial(name);
                    }
                }

                matGroup.WriteMtlFile(filePath);

                return true;
            }

            bool MtlMaterialExporter::DoesMeshNodeHaveColorStreamChild(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex meshNode) const
            {
                const Containers::SceneGraph& graph = scene.GetGraph();

                auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(graph, meshNode,
                    graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(view.begin(), view.end(), Containers::DerivedTypeFilter<DataTypes::IMeshVertexColorData>());
                return result != view.end();
            }
        } // namespace Export
    } // namespace SceneAPI
} // namespace AZ
