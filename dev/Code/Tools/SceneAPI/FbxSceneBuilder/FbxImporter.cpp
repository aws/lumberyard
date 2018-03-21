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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxImporter.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSDKWrapper/FbxSceneWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            struct QueueNode
            {
                std::shared_ptr<FbxSDKWrapper::FbxNodeWrapper> m_node;
                Containers::SceneGraph::NodeIndex m_parent;

                QueueNode() = default;
                QueueNode(std::shared_ptr<FbxSDKWrapper::FbxNodeWrapper>&& node, Containers::SceneGraph::NodeIndex parent)
                    : m_node(std::move(node))
                    , m_parent(parent)
                {
                }
            };

            FbxImporter::FbxImporter()
                : m_sceneWrapper(new FbxSDKWrapper::FbxSceneWrapper())
                , m_sceneSystem(new FbxSceneSystem())
            {
                BindToCall(&FbxImporter::ImportProcessing);
            }

            void FbxImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult FbxImporter::ImportProcessing(Events::ImportEventContext& context)
            {
                m_sceneWrapper->Clear();

                if (!m_sceneWrapper->LoadSceneFromFile(context.GetInputDirectory().c_str()))
                {
                    return Events::ProcessingResult::Failure;
                }
                m_sceneSystem->Set(*m_sceneWrapper);

                if (ConvertFbxSceneContext(context.GetScene()))
                {
                    return Events::ProcessingResult::Success;
                }
                else
                {
                    return Events::ProcessingResult::Failure;
                }
            }

            bool FbxImporter::ConvertFbxSceneContext(Containers::Scene& scene) const
            {
                std::shared_ptr<FbxSDKWrapper::FbxNodeWrapper> fbxRoot = m_sceneWrapper->GetRootNode();
                if (!fbxRoot)
                {
                    return false;
                }

                int sign = 0;
                FbxSDKWrapper::FbxAxisSystemWrapper::UpVector upVector = m_sceneWrapper->GetAxisSystem()->GetUpVector(sign);
                AZ_Assert(sign != 0, "sign failed to populate which is a failure in GetUpVector");

                if (upVector == FbxSDKWrapper::FbxAxisSystemWrapper::UpVector::Z)
                {
                    if (sign > 0)
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::ZUp);
                    }
                    else
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::NegZUp);
                        AZ_Assert(false, "Negative Z Up scene orientation is not a currently supported orientation.");
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative Z Up scene orientation is not a currently supported orientation.");
                    }
                }
                else if (upVector == FbxSDKWrapper::FbxAxisSystemWrapper::UpVector::Y)
                {
                    if (sign > 0)
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::YUp);
                    }
                    else
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::NegYUp);
                        AZ_Assert(false, "Negative Y Up scene orientation is not a currently supported orientation.");
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative Y Up scene orientation is not a currently supported orientation.");
                    }
                }
                else if (upVector == FbxSDKWrapper::FbxAxisSystemWrapper::UpVector::X)
                {
                    if (sign > 0)
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::XUp);
                        AZ_Assert(false, "Positive X Up scene orientation is not a currently supported orientation.");
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Positive X Up scene orientation is not a currently supported orientation.");
                    }
                    else
                    {
                        scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::NegXUp);
                        AZ_Assert(false, "Negative X Up scene orientation is not a currently supported orientation.");
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative X Up scene orientation is not a currently supported orientation.");
                    }
                }

                AZStd::queue<FbxSceneBuilder::QueueNode> nodes;
                nodes.emplace(AZStd::move(fbxRoot), scene.GetGraph().GetRoot());

                RenamedNodesMap nodeNameMap;
                
                while (!nodes.empty())
                {
                    FbxSceneBuilder::QueueNode& node = nodes.front();

                    AZ_Assert(node.m_node, "Empty fbx node queued");
                    
                    if (!nodeNameMap.RegisterNode(node.m_node, scene.GetGraph(), node.m_parent))
                    {
                        AZ_TracePrintf(Utilities::ErrorWindow, "Failed to register fbx node in name table.");
                        continue;
                    }
                    AZStd::string nodeName = nodeNameMap.GetNodeName(node.m_node);
                    AZ_TraceContext("SceneAPI Node Name", nodeName);

                    Containers::SceneGraph::NodeIndex newNode = scene.GetGraph().AddChild(node.m_parent, nodeName.c_str());
                    AZ_Assert(newNode.IsValid(), "Failed to add node to scene graph");
                    if (!newNode.IsValid())
                    {
                        continue;
                    }

                    FbxNodeEncounteredContext sourceNodeEncountered(scene, newNode, *m_sceneWrapper, *m_sceneSystem, nodeNameMap, *node.m_node);
                    Events::ProcessingResultCombiner nodeResult;
                    nodeResult += Events::Process(sourceNodeEncountered);

                    // If no importer created data, we still create an empty node that may eventually contain a transform
                    if (sourceNodeEncountered.m_createdData.empty())
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Success,
                            "Importers returned success but no data was created");
                        AZStd::shared_ptr<DataTypes::IGraphObject> nullData(nullptr);
                        sourceNodeEncountered.m_createdData.emplace_back(nullData);
                        nodeResult += Events::ProcessingResult::Success;
                    }

                    // Create single node since only one piece of graph data was created
                    if (sourceNodeEncountered.m_createdData.size() == 1)
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored, 
                            "An importer created data, but did not return success");
                        if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                        }

                        SceneDataPopulatedContext dataProcessed(sourceNodeEncountered, 
                            sourceNodeEncountered.m_createdData[0], nodeName.c_str());
                        Events::ProcessingResult result = AddDataNodeWithContexts(dataProcessed);
                        if (result != Events::ProcessingResult::Failure)
                        {
                            newNode = dataProcessed.m_currentGraphPosition;
                        }
                    }
                    // Create an empty parent node and place all data under it. The remaining
                    // tree will be built off of this as the logical parent
                    else
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored,
                            "%i importers created data, but did not return success", 
                            sourceNodeEncountered.m_createdData.size());
                        if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                        }

                        size_t offset = nodeName.length();
                        for (size_t i = 0; i < sourceNodeEncountered.m_createdData.size(); ++i)
                        {
                            nodeName += '_';
                            nodeName += AZStd::to_string(aznumeric_cast<AZ::u64>(i + 1));

                            Containers::SceneGraph::NodeIndex subNode =
                                scene.GetGraph().AddChild(newNode, nodeName.c_str());
                            AZ_Assert(subNode.IsValid(), "Failed to create new scene sub node");
                            SceneDataPopulatedContext dataProcessed(sourceNodeEncountered, 
                                sourceNodeEncountered.m_createdData[i], nodeName);
                            dataProcessed.m_currentGraphPosition = subNode;
                            AddDataNodeWithContexts(dataProcessed);

                            // Remove the temporary extension again.
                            nodeName.erase(offset, nodeName.length() - offset);
                        }
                    }

                    AZ_Assert(nodeResult.GetResult() == Events::ProcessingResult::Success, 
                        "No importers successfully added processed scene data.");
                    AZ_Assert(newNode != node.m_parent, 
                        "Failed to update current graph position during data processing.");

                    int childCount = node.m_node->GetChildCount();
                    for (int i = 0; i < childCount; ++i)
                    {
                        std::shared_ptr<FbxSDKWrapper::FbxNodeWrapper> child = node.m_node->GetChild(i);
                        if (child)
                        {
                            nodes.emplace(AZStd::move(child), newNode);
                        }
                    }

                    nodes.pop();
                }

                Events::ProcessingResult result = Events::Process<FinalizeSceneContext>(scene, *m_sceneWrapper, *m_sceneSystem, nodeNameMap);
                if (result == Events::ProcessingResult::Failure)
                {
                    return false;
                }

                return true;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ