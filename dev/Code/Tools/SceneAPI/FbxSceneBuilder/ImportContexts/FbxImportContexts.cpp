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

#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxImportContext::FbxImportContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition,  const FbxSDKWrapper::FbxSceneWrapper& sourceScene, 
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode)
                : m_scene(scene)
                , m_currentGraphPosition(currentGraphPosition)
                , m_sourceScene(sourceScene)
                , m_sourceSceneSystem(sourceSceneSystem)
                , m_sourceNode(sourceNode)
                , m_nodeNameMap(nodeNameMap)
            {
            }

            FbxNodeEncounteredContext::FbxNodeEncounteredContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode)
                : FbxImportContext(scene, currentGraphPosition, sourceScene, sourceSceneSystem, nodeNameMap, sourceNode)
            {
            }

            FbxNodeEncounteredContext::FbxNodeEncounteredContext(
                Events::ImportEventContext& parent, Containers::SceneGraph::NodeIndex currentGraphPosition,
                const FbxSDKWrapper::FbxSceneWrapper& sourceScene, const FbxSceneSystem& sourceSceneSystem,
                RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode)
                : FbxImportContext(parent.GetScene(), currentGraphPosition, sourceScene, sourceSceneSystem, nodeNameMap, sourceNode)
            {
            }

            SceneDataPopulatedContext::SceneDataPopulatedContext(FbxNodeEncounteredContext& parent,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& graphData, const AZStd::string& dataName)
                : FbxImportContext(parent)
                , m_graphData(graphData)
                , m_dataName(dataName)
            {
            }

            SceneDataPopulatedContext::SceneDataPopulatedContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData, const AZStd::string& dataName)
                : FbxImportContext(scene, currentGraphPosition, sourceScene, sourceSceneSystem, nodeNameMap, sourceNode)
                , m_graphData(nodeData)
                , m_dataName(dataName)
            {
            }

            SceneNodeAppendedContext::SceneNodeAppendedContext(SceneDataPopulatedContext& parent,
                Containers::SceneGraph::NodeIndex newIndex)
                : FbxImportContext(parent.m_scene, newIndex, parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_nodeNameMap, parent.m_sourceNode)
            {
            }

            SceneNodeAppendedContext::SceneNodeAppendedContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode)
                : FbxImportContext(scene, currentGraphPosition, sourceScene, sourceSceneSystem, nodeNameMap, sourceNode)
            {
            }

            SceneAttributeDataPopulatedContext::SceneAttributeDataPopulatedContext(SceneNodeAppendedContext& parent,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                const Containers::SceneGraph::NodeIndex attributeNodeIndex, const AZStd::string& dataName)
                : FbxImportContext(parent.m_scene, attributeNodeIndex, parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_nodeNameMap, parent.m_sourceNode)
                , m_graphData(nodeData)
                , m_dataName(dataName)
            {
            }

            SceneAttributeNodeAppendedContext::SceneAttributeNodeAppendedContext(
                SceneAttributeDataPopulatedContext& parent, Containers::SceneGraph::NodeIndex newIndex)
                : FbxImportContext(parent.m_scene, newIndex, parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_nodeNameMap, parent.m_sourceNode)
            {
            }

            SceneNodeAddedAttributesContext::SceneNodeAddedAttributesContext(SceneNodeAppendedContext& parent)
                : FbxImportContext(parent)
            {
            }

            SceneNodeFinalizeContext::SceneNodeFinalizeContext(SceneNodeAddedAttributesContext& parent)
                : FbxImportContext(parent)
            {
            }

            FinalizeSceneContext::FinalizeSceneContext(Containers::Scene& scene, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap)
                : m_scene(scene)
                , m_sourceScene(sourceScene)
                , m_sourceSceneSystem(sourceSceneSystem)
                , m_nodeNameMap(nodeNameMap)
            {
            }
        } // namespace SceneAPI
    } // namespace FbxSceneBuilder
} // namespace AZ