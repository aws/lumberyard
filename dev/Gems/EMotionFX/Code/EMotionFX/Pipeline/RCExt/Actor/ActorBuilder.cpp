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
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>

#include <SceneAPIExt/Rules/IMeshRule.h>
#include <SceneAPIExt/Rules/ISkinRule.h>
#include <SceneAPIExt/Rules/IActorScaleRule.h>
#include <SceneAPIExt/Rules/CoordinateSystemRule.h>
#include <SceneAPIExt/Groups/ActorGroup.h>
#include <RCExt/Actor/ActorBuilder.h>
#include <RCExt/ExportContexts.h>
#include <RCExt/CoordinateSystemConverter.h>

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MeshBuilder.h>
#include <EMotionFX/Source/MeshBuilderSkinningInfo.h>
#include <EMotionFX/Source/StandardMaterial.h>
#include <EMotionFX/Source/SoftSkinDeformer.h>
#include <EMotionFX/Source/DualQuatSkinDeformer.h>
#include <EMotionFX/Source/MeshDeformerStack.h>
#include <EMotionFX/Source/SoftSkinManager.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <MCore/Source/AzCoreConversions.h>

#include <GFxFramework/MaterialIO/Material.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;


        AZ_FORCE_INLINE EMotionFX::Transform AzTransformToEmfxTransformConverted(const AZ::Transform& azTransform, const CoordinateSystemConverter& coordSysConverter)
        {
            return EMotionFX::Transform(
                coordSysConverter.ConvertVector3(azTransform.GetTranslation()),
                MCore::AzQuatToEmfxQuat(coordSysConverter.ConvertQuaternion(AZ::Quaternion::CreateFromTransform(azTransform))),
                coordSysConverter.ConvertScale(azTransform.RetrieveScale()));
        }


        ActorBuilder::ActorBuilder()
        {
            BindToCall(&ActorBuilder::BuildActor);
        }

        void ActorBuilder::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ActorBuilder, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult ActorBuilder::BuildActor(ActorBuilderContext& context)
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            ActorSettings actorSettings;
            ExtractActorSettings(context.m_group, actorSettings);

            NodeIndexSet selectedMeshNodeIndices;
            GetNodeIndicesOfSelectedMeshes(context, selectedMeshNodeIndices);

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            const Group::IActorGroup& actorGroup = context.m_group;
            const char* rootBoneName = actorGroup.GetSelectedRootBone().c_str();
            AZ_TraceContext("Root bone", rootBoneName);
            SceneContainers::SceneGraph::NodeIndex rootBoneNodeIndex = graph.Find(rootBoneName);
            if (!rootBoneNodeIndex.IsValid())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone cannot be found.\n");
                return SceneEvents::ProcessingResult::Failure;
            }

            // Get the coordinate system conversion rule.
            CoordinateSystemConverter ruleCoordSysConverter;
            AZStd::shared_ptr<Rule::CoordinateSystemRule> coordinateSystemRule = actorGroup.GetRuleContainerConst().FindFirstByType<Rule::CoordinateSystemRule>();
            if (coordinateSystemRule)
            {
                coordinateSystemRule->UpdateCoordinateSystemConverter();
                ruleCoordSysConverter = coordinateSystemRule->GetCoordinateSystemConverter();
            }

            CoordinateSystemConverter coordSysConverter = ruleCoordSysConverter;
            if (context.m_scene.GetOriginalSceneOrientation() != SceneContainers::Scene::SceneOrientation::ZUp)
            {
                AZ::Transform orientedTarget = ruleCoordSysConverter.GetTargetTransform();
                AZ::Transform rotationZ = AZ::Transform::CreateRotationZ(-AZ::Constants::Pi);
                orientedTarget = orientedTarget * rotationZ;

                //same as rule
                // X, Y and Z are all at the same indices inside the target coordinate system, compared to the source coordinate system.
                const AZ::u32 targetBasisIndices[3] = { 0, 1, 2 };
                coordSysConverter = CoordinateSystemConverter::CreateFromTransforms(ruleCoordSysConverter.GetSourceTransform(), orientedTarget, targetBasisIndices);
            }

            // Collect the node indices that emfx cares about and construct the boneNameEmfxIndex map for quick search.
            AZStd::vector<SceneContainers::SceneGraph::NodeIndex> nodeIndices;
            BoneNameEmfxIndexMap boneNameEmfxIndexMap;
            BuildPreExportStructure(graph, rootBoneNodeIndex, selectedMeshNodeIndices, nodeIndices, boneNameEmfxIndexMap);

            EMotionFX::Actor* actor = context.m_actor;
            EMotionFX::Skeleton* actorSkeleton = actor->GetSkeleton();
            const AZ::u32 exfxNodeCount = aznumeric_cast<AZ::u32>(nodeIndices.size());
            actor->SetNumNodes(aznumeric_cast<AZ::u32>(exfxNodeCount));
            actor->ResizeTransformData();

            // Add a standard material
            // This material is used within the existing EMotionFX GL window. The engine will use a native engine material at runtime. The GL window will also be replaced by a native engine viewport
            EMotionFX::StandardMaterial* defaultMat = EMotionFX::StandardMaterial::Create("Default");
            defaultMat->SetAmbient(MCore::RGBAColor(0.0f, 0.0f, 0.0f));
            defaultMat->SetDiffuse(MCore::RGBAColor(1.0f, 1.0f, 1.0f));
            defaultMat->SetSpecular(MCore::RGBAColor(1.0f, 1.0f, 1.0f));
            defaultMat->SetShine(100.0f);
            actor->AddMaterial(0, defaultMat);

            EMotionFX::Pose* bindPose = actor->GetBindPose();
            AZ_Assert(bindPose, "BindPose not available for actor");
            AZStd::unordered_map<const SceneContainers::SceneGraph::NodeIndex::IndexType, EMotionFX::Node*> actorSkeletonLookup;
            for (AZ::u32 emfxNodeIndex = 0; emfxNodeIndex < exfxNodeCount; ++emfxNodeIndex)
            {
                const SceneContainers::SceneGraph::NodeIndex& nodeIndex = nodeIndices[emfxNodeIndex];
                const char* nodeName = graph.GetNodeName(nodeIndex).GetName();
                EMotionFX::Node* emfxNode = EMotionFX::Node::Create(nodeName, actorSkeleton);

                emfxNode->SetNodeIndex(emfxNodeIndex);

                // Add the emfx node to the actor
                actorSkeleton->SetNode(emfxNodeIndex, emfxNode);
                actorSkeletonLookup.insert(AZStd::make_pair(nodeIndex.AsNumber(), emfxNode));

                // Set the parent, and add this node as child inside the parent
                // Only if this node has a parent and the parent node is valid
                EMotionFX::Node* emfxParentNode = nullptr;
                if (graph.HasNodeParent(nodeIndex) && graph.GetNodeParent(nodeIndex) != graph.GetRoot())
                {
                    const SceneContainers::SceneGraph::NodeIndex nodeParent = graph.GetNodeParent(nodeIndex);
                    if (actorSkeletonLookup.find(nodeParent.AsNumber()) != actorSkeletonLookup.end())
                    {
                        emfxParentNode = actorSkeletonLookup[nodeParent.AsNumber()];
                    }
                }

                if (emfxParentNode)
                {
                    emfxNode->SetParentIndex(emfxParentNode->GetNodeIndex());
                    emfxParentNode->AddChild(emfxNodeIndex);
                }
                else
                {
                    // If this node has no parent, then it should be added as root node
                    actorSkeleton->AddRootNode(emfxNodeIndex);
                }

                // Set the decomposed bind pose local transformation
                EMotionFX::Transform outTransform;
                auto view = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(graph, nodeIndex,
                        graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(view.begin(), view.end(), SceneContainers::DerivedTypeFilter<SceneDataTypes::ITransform>());
                if (result != view.end())
                {
                    // Check if the node contain any transform node.
                    const AZ::Transform azTransform = azrtti_cast<const SceneDataTypes::ITransform*>(result->get())->GetMatrix();
                    outTransform = AzTransformToEmfxTransformConverted(azTransform, coordSysConverter);
                }
                else
                {
                    // This node itself could be a transform node.
                    AZStd::shared_ptr<const SceneDataTypes::ITransform> transformData = azrtti_cast<const SceneDataTypes::ITransform*>(graph.GetNodeContent(nodeIndex));
                    if (transformData)
                    {
                        const AZ::Transform transform = coordSysConverter.ConvertTransform(transformData->GetMatrix());
                        outTransform = AzTransformToEmfxTransformConverted(transform, coordSysConverter);
                    }
                }
                bindPose->SetLocalTransform(emfxNodeIndex, outTransform);
            }
            SceneEvents::ProcessingResultCombiner result;
            if (actorSettings.m_loadMeshes && !selectedMeshNodeIndices.empty())
            {
                GetMaterialInfoForActorGroup(context);
                if (m_materialGroup)
                {
                    size_t materialCount = m_materialGroup->GetMaterialCount();
                    // So far, we have only added only the default material. For the meshes, we may need to set material indices greater than 0.
                    // To avoid feeding the EMFX mesh builder with invalid material indices, here we push the default material itself a few times
                    // so that the number of total materials added to mesh builder equals the number of materials in m_materialGroup. The actual material
                    // pushed doesn't matter since it is not used for rendering.
                    for (size_t materialIdx = 1; materialIdx < materialCount; ++materialIdx)
                    {
                        actor->AddMaterial(0, defaultMat->Clone());
                    }
                    AZ_Assert(materialCount == actor->GetNumMaterials(0), "Didn't add the desired number of materials to the actor");
                }
                for (auto& nodeIndex : selectedMeshNodeIndices)
                {
                    AZStd::shared_ptr<const SceneDataTypes::IMeshData> nodeMesh = azrtti_cast<const SceneDataTypes::IMeshData*>(graph.GetNodeContent(nodeIndex));
                    AZ_Assert(nodeMesh, "Node is expected to be a mesh, but isn't.");
                    if (nodeMesh)
                    {
                        EMotionFX::Node* emfxNode = actorSkeletonLookup[nodeIndex.AsNumber()];
                        BuildMesh(context, emfxNode, nodeMesh, nodeIndex, boneNameEmfxIndexMap, actorSettings, coordSysConverter);
                    }
                }
                //Process Morph Targets
                {
                    AZStd::vector< AZ::u32>  meshIndices;
                    for (auto& nodeIndex : selectedMeshNodeIndices)
                    {
                        meshIndices.push_back(nodeIndex.AsNumber());
                    }
                    ActorMorphBuilderContext actorMorphBuilderContext(context.m_scene, actorSettings.m_optimizeTriangleList, &meshIndices, context.m_group, context.m_actor, coordSysConverter, AZ::RC::Phase::Construction);
                    result += SceneEvents::Process(actorMorphBuilderContext);
                    result += SceneEvents::Process<ActorMorphBuilderContext>(actorMorphBuilderContext, AZ::RC::Phase::Filling);
                    result += SceneEvents::Process<ActorMorphBuilderContext>(actorMorphBuilderContext, AZ::RC::Phase::Finalizing);
                }
            }

            // Post create actor
            actor->SetUnitType(MCore::Distance::UNITTYPE_METERS);
            actor->SetFileUnitType(MCore::Distance::UNITTYPE_METERS);
            actor->PostCreateInit(false, true, false);

            // Scale the actor
            AZStd::shared_ptr<Rule::IActorScaleRule> scaleRule = actorGroup.GetRuleContainerConst().FindFirstByType<Rule::IActorScaleRule>();
            if (scaleRule)
            {
                const float scaleFactor = scaleRule->GetScaleFactor();
                if (!AZ::IsClose(scaleFactor, 1.0f, FLT_EPSILON)) // If the scale factor is 1, no need to call Scale
                {
                    actor->Scale(scaleFactor);
                }
            }

            if (result.GetResult() != AZ::SceneAPI::Events::ProcessingResult::Failure)
            {
                return SceneEvents::ProcessingResult::Success;
            }
            return AZ::SceneAPI::Events::ProcessingResult::Failure;
        }

        void ActorBuilder::BuildPreExportStructure(const SceneContainers::SceneGraph& graph, const SceneContainers::SceneGraph::NodeIndex& rootBoneNodeIndex, const NodeIndexSet& selectedMeshNodeIndices,
            AZStd::vector<SceneContainers::SceneGraph::NodeIndex>& outNodeIndices, BoneNameEmfxIndexMap& outBoneNameEmfxIndexMap)
        {
            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();
            auto nameContentView = SceneViews::MakePairView(nameStorage, contentStorage);

            // The search begin from the rootBoneNodeIndex.
            auto graphDownwardsRootBoneView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, rootBoneNodeIndex, nameContentView.begin(), true);
            auto it = graphDownwardsRootBoneView.begin();
            if (!it->second)
            {
                // We always skip the first node because it's introduced by scenegraph
                ++it;
                if (!it->second && it != graphDownwardsRootBoneView.end())
                {
                    // In maya / max, we skip 1 root node when it have no content (emotionfx always does this)
                    // However, fbx doesn't restrain itself from having multiple root nodes. We might want to revisit here if it ever become a problem.
                    ++it;
                }
            }

            for (; it != graphDownwardsRootBoneView.end(); ++it)
            {
                const SceneContainers::SceneGraph::NodeIndex& nodeIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());

                // The end point in ly scene graph should not be added to the emfx actor.
                // Note: For example, the end point could be a transform node. We will process that later on its parent node.
                if (graph.IsNodeEndPoint(nodeIndex))
                {
                    continue;
                }

                // If the node is a mesh, we skip it in the first search.
                auto mesh = azrtti_cast<const SceneDataTypes::IMeshData*>(it->second);
                if (mesh)
                {
                    continue;
                }

                // If it's a bone, add it to boneNodeIndices.
                auto bone = azrtti_cast<const SceneDataTypes::IBoneData*>(it->second);
                if (bone)
                {
                    outBoneNameEmfxIndexMap[it->first.GetName()] = aznumeric_cast<AZ::u32>(outNodeIndices.size());
                }

                outNodeIndices.push_back(nodeIndex);
            }

            // We then search from the graph root to find all the meshes that we selected.
            auto graphDownwardsView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, graph.GetRoot(), nameContentView.begin(), true);
            for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
            {
                const SceneContainers::SceneGraph::NodeIndex& nodeIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
                if (!it->second)
                {
                    continue;
                }

                // If the node is a mesh and it is one of the selected ones, add it.
                auto mesh = azrtti_cast<const SceneDataTypes::IMeshData*>(it->second);
                if (mesh && (selectedMeshNodeIndices.find(nodeIndex) != selectedMeshNodeIndices.end()))
                {
                    outNodeIndices.push_back(nodeIndex);
                }
            }
        }

        // This method uses EMFX MeshBuilder class. This MeshBuilder class expects to be fed "Control points" in fbx parlance. However, as of the current (April 2017)
        // implementation of MeshData class and FbxMeshImporterUtilities.cpp, IMeshData does not provide a  way to get all of the original control points obtained
        // from the fbx resource. Specifically, IMeshData has information about only those control points which it uses, i.e., those of the original control points which
        // are part of polygons. So, any unconnected stray vertices or vertices which have just lines between them have all been discarded and we don't have a way
        // to get their positions/normals etc.
        // Given that IMeshData doesn't provide access to all of the control points, we have two choices.
        // Choice 1: Update the fbx pipeline code to provide access to the original control points via IMeshData or some other class.
        // Choice 2: View the subset of the control points that MeshData has as the control points for EMFX MeshBuilder.
        // The code below is based on the second choice. Reasons for this choice are: (a) Less updates to the core fbx pipeline code and hence less risk of breaking
        // existing export paths. (b) Since we ultimately are rendering only polygons anyway, we don't care about such stray vertices which are not part of any
        // polygons.
        // The newly added method IMeshData::GetUsedPointIndexForControlPoint(...) provides unique 0 based contiguous indices to the control points actually used in MeshData.
        //
        void ActorBuilder::BuildMesh(const ActorBuilderContext& context, EMotionFX::Node* emfxNode, AZStd::shared_ptr<const SceneDataTypes::IMeshData> meshData,
            const SceneContainers::SceneGraph::NodeIndex& meshNodeIndex, const BoneNameEmfxIndexMap& boneNameEmfxIndexMap, const ActorSettings& settings, const CoordinateSystemConverter& coordSysConverter)
        {
            SetupMaterialDataForMesh(context, meshNodeIndex);

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            EMotionFX::Actor* actor = context.m_actor;

            // Get the number of triangles (faces)
            const AZ::u32 numFaces = meshData->GetFaceCount();

            // Get the number of orgVerts (control point)
            const AZ::u32 numOrgVerts = aznumeric_cast<AZ::u32>(meshData->GetUsedControlPointCount());
            EMotionFX::MeshBuilder* meshBuilder = EMotionFX::MeshBuilder::Create(emfxNode->GetNodeIndex(), numOrgVerts, false);

            // Import the skinning info if there is any. Otherwise set it to a nullptr.
            EMotionFX::MeshBuilderSkinningInfo* skinningInfo = ExtractSkinningInfo(meshData, graph, meshNodeIndex, boneNameEmfxIndexMap, settings);
            meshBuilder->SetSkinningInfo(skinningInfo);

            // Original vertex numbers
            EMotionFX::MeshBuilderVertexAttributeLayerUInt32* orgVtxLayer = EMotionFX::MeshBuilderVertexAttributeLayerUInt32::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS, false, false);
            meshBuilder->AddLayer(orgVtxLayer);

            // The positions layer
            EMotionFX::MeshBuilderVertexAttributeLayerVector3* posLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector3::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_POSITIONS, false, true);
            meshBuilder->AddLayer(posLayer);

            // The normals layer
            EMotionFX::MeshBuilderVertexAttributeLayerVector3* normalsLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector3::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_NORMALS, false, true);
            meshBuilder->AddLayer(normalsLayer);

            // The UV Layer
            // A Mesh can have multiple children that contain UV data.
            AZStd::vector<AZStd::shared_ptr<const SceneDataTypes::IMeshVertexUVData> > meshUVDatas;
            AZStd::vector<EMotionFX::MeshBuilderVertexAttributeLayerVector2*> uvLayers;

            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();
            auto nameContentView = SceneViews::MakePairView(nameStorage, contentStorage);

            auto meshChildView = SceneViews::MakeSceneGraphChildView<SceneContainers::Views::AcceptEndPointsOnly>(graph, meshNodeIndex,
                    nameContentView.begin(), true);
            for (auto it = meshChildView.begin(); it != meshChildView.end(); ++it)
            {
                AZStd::shared_ptr<const SceneDataTypes::IMeshVertexUVData> uvData = azrtti_cast<const SceneDataTypes::IMeshVertexUVData*>(it->second);
                if (uvData)
                {
                    EMotionFX::MeshBuilderVertexAttributeLayerVector2* uvLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector2::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_UVCOORDS, false, false);
                    uvLayer->SetName(it->first.GetName());
                    meshBuilder->AddLayer(uvLayer);
                    uvLayers.push_back(uvLayer);
                    meshUVDatas.push_back(uvData);
                }
            }

            const AZ::Transform globalTransform = SceneUtil::BuildWorldTransform(graph, meshNodeIndex);
            // Inverse transpose for normal
            const AZ::Transform globalTranformN = globalTransform.GetInverseFull().GetTranspose();

            // Data for each vertex
            AZ::Vector2 uv;
            AZ::Vector3 pos;
            AZ::Vector3 normal;
            for (AZ::u32 i = 0; i < numFaces; ++i)
            {
                AZ::u32 materialID = 0;
                if (m_materialGroup)
                {
                    AZ::u32 meshLocalMaterialID = meshData->GetFaceMaterialId(i);
                    if (meshLocalMaterialID < m_materialIndexMapForMesh.size())
                    {
                        materialID = m_materialIndexMapForMesh[meshLocalMaterialID];
                    }
                    else
                    {
                        AZ_TracePrintf(SceneUtil::WarningWindow, "Invalid value for the material index of the face.\n");
                    }
                }

                // Start the triangle
                meshBuilder->BeginPolygon(materialID);

                // Determine winding.
                AZ::u32 order[3];
                if (coordSysConverter.IsSourceRightHanded() == coordSysConverter.IsTargetRightHanded())
                {
                    order[0] = 0;
                    order[1] = 1;
                    order[2] = 2;
                }
                else
                {
                    order[0] = 2;
                    order[1] = 1;
                    order[2] = 0;
                }

                // Add all triangle points (We are not supporting non-triangle face)
                for (AZ::u32 j = 0; j < 3; ++j)
                {
                    const AZ::u32 vertexIndex = meshData->GetVertexIndex(i, order[j]);
                    const AZ::u32 controlPointIndex = meshData->GetControlPointIndex(vertexIndex);

                    int orgVertexNumber = meshData->GetUsedPointIndexForControlPoint(controlPointIndex);
                    AZ_Assert(orgVertexNumber >= 0, "Invalid vertex number");
                    orgVtxLayer->SetCurrentVertexValue(&orgVertexNumber);

                    pos = meshData->GetPosition(vertexIndex);
                    normal = meshData->GetNormal(vertexIndex);
                    if (skinningInfo)
                    {
                        pos = globalTransform * pos;
                        normal = globalTranformN * normal;
                    }

                    pos = coordSysConverter.ConvertVector3(pos);
                    posLayer->SetCurrentVertexValue(&pos);

                    normal = coordSysConverter.ConvertVector3(normal);
                    normal.Normalize();
                    normalsLayer->SetCurrentVertexValue(&normal);

                    for (AZ::u32 e = 0; e < uvLayers.size(); ++e)
                    {
                        uv = meshUVDatas[e]->GetUV(vertexIndex);
                        uvLayers[e]->SetCurrentVertexValue(&uv);
                    }

                    meshBuilder->AddPolygonVertex(orgVertexNumber);
                }

                // End the triangle
                meshBuilder->EndPolygon();
            }   // End of all triangle

            // Cache optimize the index buffer list
            if (settings.m_optimizeTriangleList)
            {
                meshBuilder->OptimizeTriangleList();
            }
            // Link the mesh to the node
            EMotionFX::Mesh* emfxMesh = meshBuilder->ConvertToEMotionFXMesh();
            actor->SetMesh(0, emfxNode->GetNodeIndex(), emfxMesh);

            if (!skinningInfo && settings.m_loadSkinningInfo)
            {
                CreateSkinningMeshDeformer(actor, emfxNode, emfxMesh, skinningInfo);
            }

            // Calc the tangents for the first UV layer
            emfxMesh->CalcTangents(0);

            meshBuilder->Destroy();
        }

        EMotionFX::MeshBuilderSkinningInfo* ActorBuilder::ExtractSkinningInfo(AZStd::shared_ptr<const SceneDataTypes::IMeshData> meshData,
            const SceneContainers::SceneGraph& graph, const SceneContainers::SceneGraph::NodeIndex& meshNodeIndex,
            const BoneNameEmfxIndexMap& boneNameEmfxIndexMap, const ActorSettings& settings)
        {
            if (!settings.m_loadSkinningInfo)
            {
                return nullptr;
            }

            // Create the new skinning info
            EMotionFX::MeshBuilderSkinningInfo* skinningInfo = nullptr;

            auto meshChildView = SceneViews::MakeSceneGraphChildView<SceneContainers::Views::AcceptEndPointsOnly>(graph, meshNodeIndex,
                    graph.GetContentStorage().begin(), true);
            for (auto meshIt = meshChildView.begin(); meshIt != meshChildView.end(); ++meshIt)
            {
                const SceneDataTypes::ISkinWeightData* skinData = azrtti_cast<const SceneDataTypes::ISkinWeightData*>(meshIt->get());
                if (skinData)
                {
                    const size_t numUsedControlPoints = meshData->GetUsedControlPointCount();
                    if (!skinningInfo)
                    {
                        skinningInfo = EMotionFX::MeshBuilderSkinningInfo::Create(aznumeric_cast<AZ::u32>(numUsedControlPoints));
                    }

                    const size_t controlPointCount = skinData->GetVertexCount();
                    for (size_t controlPointIndex = 0; controlPointIndex < controlPointCount; ++controlPointIndex)
                    {
                        const int usedPointIndex = meshData->GetUsedPointIndexForControlPoint(controlPointIndex);
                        if (usedPointIndex < 0)
                        {
                            continue; // This control point is not used in the mesh
                        }
                        const size_t linkCount = skinData->GetLinkCount(controlPointIndex);
                        if (linkCount == 0)
                        {
                            continue;
                        }

                        EMotionFX::MeshBuilderSkinningInfo::Influence influence;
                        for (size_t linkIndex = 0; linkIndex < linkCount; ++linkIndex)
                        {
                            const SceneDataTypes::ISkinWeightData::Link& link = skinData->GetLink(controlPointIndex, linkIndex);
                            influence.mWeight = link.weight;

                            const AZStd::string& boneName = skinData->GetBoneName(link.boneId);
                            auto boneIt = boneNameEmfxIndexMap.find(boneName);
                            if (boneIt == boneNameEmfxIndexMap.end())
                            {
                                AZ_TraceContext("Missing bone in actor skinning info", boneName.c_str());
                                continue;
                            }
                            influence.mNodeNr = boneIt->second;
                            skinningInfo->AddInfluence(usedPointIndex, influence);
                        }
                    }
                }
            }

            if (skinningInfo)
            {
                skinningInfo->Optimize(settings.m_maxWeightsPerVertex, settings.m_weightThreshold);
            }

            return skinningInfo;
        }

        void ActorBuilder::CreateSkinningMeshDeformer(EMotionFX::Actor* actor, EMotionFX::Node* node, EMotionFX::Mesh* mesh, EMotionFX::MeshBuilderSkinningInfo* skinningInfo)
        {
            if (!skinningInfo)
            {
                return;
            }

            // Check if we already have a stack
            EMotionFX::MeshDeformerStack* deformerStack = actor->GetMeshDeformerStack(0, node->GetNodeIndex());
            if (!deformerStack)
            {
                // Create the stack
                EMotionFX::MeshDeformerStack* newStack = EMotionFX::MeshDeformerStack::Create(mesh);
                actor->SetMeshDeformerStack(0, node->GetNodeIndex(), newStack);
                deformerStack = actor->GetMeshDeformerStack(0, node->GetNodeIndex());
            }

            // Add a skinning deformer (it will later on get reinitialized)
            // For now we always use Linear skinning.
            EMotionFX::SoftSkinDeformer* deformer = EMotionFX::GetSoftSkinManager().CreateDeformer(mesh);
            deformerStack->AddDeformer(deformer);
        }

        void ActorBuilder::ExtractActorSettings(const Group::IActorGroup& actorGroup, ActorSettings& outSettings)
        {
            const AZ::SceneAPI::Containers::RuleContainer& rules = actorGroup.GetRuleContainerConst();

            AZStd::shared_ptr<const Rule::IMeshRule> meshRule = rules.FindFirstByType<Rule::IMeshRule>();
            if (meshRule)
            {
                outSettings.m_optimizeTriangleList = meshRule->GetOptimizeTriangleList();
            }

            AZStd::shared_ptr<const Rule::ISkinRule> skinRule = rules.FindFirstByType<Rule::ISkinRule>();
            if (skinRule)
            {
                outSettings.m_maxWeightsPerVertex = skinRule->GetMaxWeightsPerVertex();
                outSettings.m_weightThreshold = skinRule->GetWeightThreshold();
            }
        }

        bool ActorBuilder::GetMaterialInfoForActorGroup(const ActorBuilderContext& context)
        {
            // Does group contain material rule?
            AZStd::shared_ptr<SceneDataTypes::IMaterialRule> materialRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::IMaterialRule>();
            if (!materialRule)
            {
                m_materialGroup = nullptr;
                return false;
            }

            AZStd::string materialPath = context.m_scene.GetSourceFilename();
            AzFramework::StringFunc::Path::ReplaceExtension(materialPath, AZ::GFxFramework::MaterialExport::g_mtlExtension);
            AZ_TraceContext("Material File", materialPath.c_str());

            m_materialGroup = AZStd::make_shared<AZ::GFxFramework::MaterialGroup>();
            bool fileRead = m_materialGroup->ReadMtlFile(materialPath.c_str());
            if (!fileRead)
            {
                // Otherwise, if the user has never modified the material, it should've been generated by the material exporter
                // and live in the output directory.
                const AZStd::string filePath =
                    SceneUtil::FileUtilities::CreateOutputFileName(context.m_group.GetName(),
                        context.m_outputDirectory, AZ::GFxFramework::MaterialExport::g_mtlExtension);

                fileRead = m_materialGroup->ReadMtlFile(filePath.c_str());
                if (!fileRead)
                {
                    AZ_TracePrintf(SceneUtil::WarningWindow, "Material file could not be loaded.\n");
                    m_materialGroup = nullptr;
                    return false;
                }
            }
            return fileRead;
        }

        void ActorBuilder::SetupMaterialDataForMesh(const ActorBuilderContext& context, const SceneContainers::SceneGraph::NodeIndex& meshNodeIndex)
        {
            m_materialIndexMapForMesh.clear();
            if (!m_materialGroup)
            {
                return;
            }

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            auto view = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(
                    graph, meshNodeIndex, graph.GetContentStorage().begin(), true);
            for (auto it = view.begin(), itEnd = view.end(); it != itEnd; ++it)
            {
                if ((*it) && (*it)->RTTI_IsTypeOf(SceneDataTypes::IMaterialData::TYPEINFO_Uuid()))
                {
                    AZStd::string nodeName = graph.GetNodeName(graph.ConvertToNodeIndex(it.GetHierarchyIterator())).GetName();
                    AZ_TraceContext("Material Node Name", nodeName);

                    size_t index = m_materialGroup->FindMaterialIndex(nodeName);
                    if (index == AZ::GFxFramework::MaterialExport::g_materialNotFound)
                    {
                        AZ_TracePrintf(SceneUtil::ErrorWindow, "Unable to find material named %s in mtl file while building material index map for actor.", nodeName.c_str());
                        index = 0;
                    }
                    m_materialIndexMapForMesh.push_back(aznumeric_cast<AZ::u32>(index));
                }
            }
        }

        void ActorBuilder::GetNodeIndicesOfSelectedMeshes(ActorBuilderContext& context, NodeIndexSet& meshNodeIndexSet) const
        {
            meshNodeIndexSet.clear();

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            const SceneDataTypes::ISceneNodeSelectionList& nodeSelectionList = context.m_group.GetSceneNodeSelectionList();
            for (size_t i = 0, count = nodeSelectionList.GetSelectedNodeCount(); i < count; ++i)
            {
                const AZStd::string& nodePath = nodeSelectionList.GetSelectedNode(i);
                SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(nodePath);
                AZ_Assert(nodeIndex.IsValid(), "Invalid scene graph node index");
                if (nodeIndex.IsValid())
                {
                    AZStd::shared_ptr<const SceneDataTypes::IMeshData> meshData =
                        azrtti_cast<const SceneDataTypes::IMeshData*>(graph.GetNodeContent(nodeIndex));
                    if (meshData)
                    {
                        meshNodeIndexSet.insert(nodeIndex);
                    }
                }
            }
        }
    } // namespace Pipeline
} // namespace EMotionFX