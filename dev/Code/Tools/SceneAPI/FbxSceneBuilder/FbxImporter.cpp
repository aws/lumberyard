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

#include <queue>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxImporter.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSDKWrapper/FbxSceneWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
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

                AZStd::queue<FbxSceneBuilder::QueueNode> nodes;
                nodes.emplace(AZStd::move(fbxRoot), scene.GetGraph().GetRoot());

                while (!nodes.empty())
                {
                    FbxSceneBuilder::QueueNode& node = nodes.front();

                    AZ_Assert(node.m_node, "Empty fbx node queued");
                    AZ_TraceContext("Source Node Name", node.m_node->GetName());

                    Containers::SceneGraph::NodeIndex newNode =
                        scene.GetGraph().AddChild(node.m_parent, node.m_node->GetName());

                    AZ_Assert(newNode.IsValid(), "Failed to add node to scene graph");
                    if (!newNode.IsValid())
                    {
                        continue;
                    }

                    FbxNodeEncounteredContext sourceNodeEncountered(scene, newNode, *m_sceneWrapper, *m_sceneSystem, *node.m_node);
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
                            AZ_TracePrintf(Utilities::WarningWindow, 
                                "One or more importers failed to create data.");
                        }

                        SceneDataPopulatedContext dataProcessed(sourceNodeEncountered, 
                            sourceNodeEncountered.m_createdData[0], node.m_node->GetName());
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
                            AZ_TracePrintf(Utilities::WarningWindow, 
                                "One or more importers failed to create data.");
                        }

                        for (size_t i = 0; i < sourceNodeEncountered.m_createdData.size(); ++i)
                        {
                            AZStd::string nodeName = node.m_node->GetName() + (i + 1);
                            Containers::SceneGraph::NodeIndex subNode =
                                scene.GetGraph().AddChild(newNode, nodeName.c_str());
                            AZ_Assert(subNode.IsValid(), "Failed to create new scene sub node");
                            SceneDataPopulatedContext dataProcessed(sourceNodeEncountered, 
                                sourceNodeEncountered.m_createdData[i], nodeName);
                            dataProcessed.m_currentGraphPosition = subNode;
                            AddDataNodeWithContexts(dataProcessed);
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

                return true;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ