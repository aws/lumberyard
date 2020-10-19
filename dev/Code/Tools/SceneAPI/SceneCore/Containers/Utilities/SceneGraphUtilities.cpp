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

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            AZ::Transform BuildWorldTransform(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex nodeIndex)
            {
                AZ::Transform outTransform = AZ::Transform::Identity();
                while (nodeIndex.IsValid())
                {
                    auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex,
                        graph.GetContentStorage().begin(), true);
                    auto result = AZStd::find_if(view.begin(), view.end(), Containers::DerivedTypeFilter<DataTypes::ITransform>());
                    if (result != view.end())
                    {
                        // Check if the node has any child transform node
                        const AZ::Transform& azTransform = azrtti_cast<const DataTypes::ITransform*>(result->get())->GetMatrix();
                        outTransform = azTransform * outTransform;
                    }
                    else
                    {
                        // Check if the node itself is a transform node.
                        AZStd::shared_ptr<const DataTypes::ITransform> transformData = azrtti_cast<const DataTypes::ITransform*>(graph.GetNodeContent(nodeIndex));
                        if (transformData)
                        {
                            outTransform = transformData->GetMatrix() * outTransform;
                        }
                    }

                    if (graph.HasNodeParent(nodeIndex))
                    {
                        nodeIndex = graph.GetNodeParent(nodeIndex);
                    }
                    else
                    {
                        break;
                    }
                }

                return outTransform;
            }
            
            bool ConcatenateMatricesUpwards(
                AZ::Transform& transform,
                const Containers::SceneGraph::HierarchyStorageConstIterator& nodeIterator,
                const Containers::SceneGraph& graph)
            {
                bool translated = false;

                auto view = SceneAPI::Containers::Views::MakeSceneGraphUpwardsView(graph, nodeIterator, graph.GetContentStorage().cbegin(), true);
                for (auto it = view.begin(); it != view.end(); ++it)
                {
                    const DataTypes::ITransform* nodeTransform = azrtti_cast<const DataTypes::ITransform*>(it->get());
                    if (nodeTransform)
                    {
                        transform = nodeTransform->GetMatrix() * transform;
                        translated = true;
                    }
                    else
                    {
                        bool endPointTransform = MultiplyEndPointTransforms(transform, it.GetHierarchyIterator(), graph);
                        translated = translated || endPointTransform;
                    }
                }
                return translated;
            }

            bool MultiplyEndPointTransforms(
                AZ::Transform& transform,
                const Containers::SceneGraph::HierarchyStorageConstIterator& nodeIterator,
                const Containers::SceneGraph& graph)
            {
                // If the translation is not an end point it means it's its own group as opposed to being
                //      a component of the parent, so only list end point children.
                auto view = SceneAPI::Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(graph, nodeIterator, 
                    graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(view.begin(), view.end(), Containers::DerivedTypeFilter<DataTypes::ITransform>());
                if (result != view.end())
                {
                    transform = azrtti_cast<const DataTypes::ITransform*>(result->get())->GetMatrix() * transform;
                    return true;
                }
                else
                {
                    return false;
                }
            }

            AZ::Transform GetOriginRuleTransform(
                const DataTypes::IOriginRule& originRule,
                const Containers::SceneGraph& graph)
            {
                AZ::Transform originTransform = AZ::Transform::CreateIdentity();
                if (!originRule.GetTranslation().IsZero() || !originRule.GetRotation().IsIdentity())
                {
                    originTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                        originRule.GetRotation(), originRule.GetTranslation());
                }

                if (const float scale = originRule.GetScale();
                    !AZ::IsClose(scale, 1.0f, std::numeric_limits<float>::epsilon()))
                {
                    originTransform.MultiplyByScale(AZ::Vector3(scale));
                }

                if (const AZStd::string& nodeName = originRule.GetOriginNodeName();
                    !nodeName.empty() && !originRule.UseRootAsOrigin())
                {
                    const auto index = graph.Find(nodeName);
                    if (index.IsValid())
                    {
                        AZ::Transform worldMatrix = AZ::Transform::CreateIdentity();
                        if (ConcatenateMatricesUpwards(worldMatrix, graph.ConvertToHierarchyIterator(index), graph))
                        {
                            worldMatrix.InvertFull();
                            originTransform *= worldMatrix;
                        }
                    }
                }
                return originTransform;
            }
        } // Utilities
    } // SceneAPI
} // AZ
