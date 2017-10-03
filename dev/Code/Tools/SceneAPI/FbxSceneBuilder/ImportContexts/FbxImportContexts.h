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

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxSceneWrapper;
        class FbxNodeWrapper;
    }
    
    namespace SceneAPI
    {
        class FbxSceneSystem;

        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IGraphObject;
        }

        namespace Events
        {
            class ImportEventContext;
        }

        namespace FbxSceneBuilder
        {
            class RenamedNodesMap;

            //  FbxImportContext
            //  Base structure containing common data needed for all import contexts
            //  Member Variables:
            //      m_scene - The scene being populated
            //      m_currentGraphPosition - in/out variable used to provide the parent
            //          node in the scene graph, or return the node index of a newly created
            //          node.
            //      m_sourceScene - Basic scene data extracted from the FBX Scene. Used to
            //          transform data.
            //      m_sourceNode - FBX node being used for data processing.
            struct FbxImportContext
                : public Events::ICallContext
            {
                AZ_RTTI(FbxNodeEncounteredContext, "{C8D665D5-E871-41AD-90E7-C84CF6842BCF}", Events::ICallContext);

                FbxImportContext(Containers::Scene& scene, Containers::SceneGraph::NodeIndex currentGraphPosition,
                    const FbxSDKWrapper::FbxSceneWrapper& sourceScene, const FbxSceneSystem& sourceSceneSystem, 
                    RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode);

                Containers::Scene& m_scene;
                Containers::SceneGraph::NodeIndex m_currentGraphPosition;
                const FbxSDKWrapper::FbxSceneWrapper& m_sourceScene;
                const FbxSceneSystem& m_sourceSceneSystem; // Needed for unit and axis conversion
                FbxSDKWrapper::FbxNodeWrapper& m_sourceNode;
                RenamedNodesMap& m_nodeNameMap; // Map of the nodes that have received a new name.
            };

            //  FbxNodeEncounteredContext
            //  Context pushed to indicate that a new FBX Node has been found and any
            //  importers that have means to process the contained data should do so
            //  Member Variables:
            //      m_createdData - out container that importers must add their created data
            //          to.
            struct FbxNodeEncounteredContext
                : public FbxImportContext
            {
                AZ_RTTI(FbxNodeEncounteredContext, "{BE21E324-6745-41FD-A79C-A6CA7AB15A7A}", FbxImportContext);

                FbxNodeEncounteredContext(Containers::Scene& scene, 
                    Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                    const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode);

                FbxNodeEncounteredContext(Events::ImportEventContext& parent, 
                    Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                    const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode);

                AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>> m_createdData;
            };

            //  SceneDataPopulatedContext
            //  Context pushed to indicate that a piece of scene data has been fully 
            //  processed and any importers that wish to place it within the scene graph
            //  may now do so. This may be triggered by processing a FbxNodeEncounteredContext
            //  (for base data, e.g. bones, meshes) or from a SceneNodeAppendedContext 
            //  (for attribute data, e.g. UV Maps, materials)
            //  Member Variables:
            //      m_graphData - the piece of data that should be inserted in the graph
            //      m_dataName - the name that should be used as the basis for the scene node
            //          name
            //      m_isAttribute - Indicates whether the graph data is an attribute
            struct SceneDataPopulatedContext
                : public FbxImportContext
            {
                AZ_RTTI(SceneDataPopulatedContext, "{DF17306C-FE28-4BEB-9CF0-88CF0472B8A8}", FbxImportContext);

                SceneDataPopulatedContext(FbxNodeEncounteredContext& parent,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                    const AZStd::string& dataName);

                SceneDataPopulatedContext(Containers::Scene& scene, 
                    Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene, 
                    const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData, const AZStd::string& dataName);

                const AZStd::shared_ptr<DataTypes::IGraphObject>& m_graphData;
                const AZStd::string m_dataName;
            };

            //  SceneNodeAppendedContext
            //  Context pushed to indicate that data has been added to the scene graph. 
            //  Generally created due to the insertion of a node during SceneDataPopulatedContext
            //  processing.
            struct SceneNodeAppendedContext
                : public FbxImportContext
            {
                AZ_RTTI(SceneNodeAppendedContext, "{72C1C37A-C6ED-4CB7-B929-DA03AA44131C}", FbxImportContext);

                SceneNodeAppendedContext(SceneDataPopulatedContext& parent, Containers::SceneGraph::NodeIndex newIndex);
                SceneNodeAppendedContext(Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene, 
                    const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode);
            };

            // SceneAttributeDataPopulatedContext
            // Context pushed to indicate that attribute data has been found and processed
            struct SceneAttributeDataPopulatedContext
                : public FbxImportContext
            {
                AZ_RTTI(SceneAttributeDataPopulatedContext, "{93E67C26-5A40-4385-8189-947A626E3CDA}", FbxImportContext);

                SceneAttributeDataPopulatedContext(SceneNodeAppendedContext& parent,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                    const Containers::SceneGraph::NodeIndex attributeNodeIndex, const AZStd::string& dataName);

                const AZStd::shared_ptr<DataTypes::IGraphObject>& m_graphData;
                const AZStd::string m_dataName;
            };

            // SceneAttributeNodeAppendedContext
            // Context pushed to indicate that an attribute node has been added to the scene graph
            struct SceneAttributeNodeAppendedContext
                : public FbxImportContext
            {
                AZ_RTTI(SceneAttributeNodeAppendedContext, "{C0DD4F39-5C61-4CA0-96C5-9EA3AC40D98B}", FbxImportContext);

                SceneAttributeNodeAppendedContext(SceneAttributeDataPopulatedContext& parent, 
                    Containers::SceneGraph::NodeIndex newIndex);
            };

            //  SceneNodeAddedAttributesContext
            //  Context pushed to indicate that all attribute processors have completed their
            //  work for a specific data node.
            struct SceneNodeAddedAttributesContext
                : public FbxImportContext
            {
                AZ_RTTI(SceneNodeAddedAttributesContext, "{1601900C-5109-4D37-83F1-22317A4D7C78}", FbxImportContext);

                SceneNodeAddedAttributesContext(SceneNodeAppendedContext& parent);
            };

            //  SceneNodeFinalizeContext
            //  Context pushed last after all other contexts for a scene node to allow any
            //  post-processing needed for an importer.
            struct SceneNodeFinalizeContext
                : public FbxImportContext
            {
                AZ_RTTI(SceneNodeFinalizeContext, "{D1D9839A-EA48-425D-BB7A-A9AEA65B8B7A}", FbxImportContext);

                SceneNodeFinalizeContext(SceneNodeAddedAttributesContext& parent);
            };

            //  FinalizeSceneContext
            //  Context pushed after the scene has been fully created. This can be used to finalize pending work
            //  such as resolving named links.
            struct FinalizeSceneContext
                : public Events::ICallContext
            {
                AZ_RTTI(FinalizeSceneContext, "{C8D665D5-E871-41AD-90E7-C84CF6842BCF}", Events::ICallContext);

                FinalizeSceneContext(Containers::Scene& scene, const FbxSDKWrapper::FbxSceneWrapper& sourceScene, 
                    const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap);

                Containers::Scene& m_scene;
                const FbxSDKWrapper::FbxSceneWrapper& m_sourceScene;
                const FbxSceneSystem& m_sourceSceneSystem; // Needed for unit and axis conversion
                RenamedNodesMap& m_nodeNameMap;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ

