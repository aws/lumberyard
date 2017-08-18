#pragma once

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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <SceneAPI/SceneCore/Events/CallProcessorBinder.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

struct IConvertContext;

namespace EMotionFX
{
    class Actor;
    class Node;
    class Mesh;
    class MeshBuilder;
    class MeshBuilderSkinningInfo;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMeshData;
            class IActorGroup;
        }
    }
    namespace GFxFramework
    {
        class IMaterialGroup;
    }

    class Transform;
}

namespace MotionCanvasPipeline
{
    struct ActorBuilderContext;

    struct ActorSettings
    {
        bool m_loadMeshes;					    /* Set to false if you wish to disable loading any meshes. */
        bool m_loadStandardMaterialLayers;	    /* Set to false if you wish to disable loading any standard material layers. */
        bool m_loadMorphTargets;				/* Set to false if you wish to disable loading any  morph targets. */
        bool m_loadSkinningInfo;
        bool m_autoCreateTrajectoryNode;		/* Set to false if the trajectory node should not be created automatically in case there is no trajectory node set in the loaded actor. */
        bool m_optimizeTriangleList;
        AZ::u32 m_maxWeightsPerVertex;
        float m_weightThreshold;                /* removes skinning influences below this threshold and re-normalize others. */

        /**
        * Constructor.
        */
        ActorSettings()
            : m_loadMeshes(true)
            , m_loadStandardMaterialLayers(true)
            , m_loadSkinningInfo(true)
            , m_loadMorphTargets(true)  /* Not supported from emfx yet. */
            , m_autoCreateTrajectoryNode(true)
            , m_optimizeTriangleList(true)
            , m_maxWeightsPerVertex(4)
            , m_weightThreshold(0.001f)
        {
        }
    };

    class ActorBuilder
        : public AZ::SceneAPI::Events::CallProcessorBinder
    {
    public:
        using BoneNameEmfxIndexMap = AZStd::unordered_map<AZStd::string, AZ::u32>;

        ActorBuilder(IConvertContext* convertContext);
        ~ActorBuilder() override = default;

        AZ::SceneAPI::Events::ProcessingResult BuildActor(ActorBuilderContext& context);

    private:
        struct NodeIndexHasher
        {
            AZStd::size_t operator()(const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
            {
                return nodeIndex.AsNumber();
            }
        };
        struct NodeIndexComparator
        {
            bool operator()(const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& a,
                const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& b) const
            {
                return a == b;
            }
        };
        using NodeIndexSet = AZStd::unordered_set<AZ::SceneAPI::Containers::SceneGraph::NodeIndex, NodeIndexHasher, NodeIndexComparator>;

    private:

        void BuildPreExportStructure(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& rootBoneNodeIndex, const NodeIndexSet& selectedMeshNodeIndices,
            AZStd::vector<AZ::SceneAPI::Containers::SceneGraph::NodeIndex>& outNodeIndices, BoneNameEmfxIndexMap& outBoneNameEmfxIndexMap);

        void BuildMesh(const ActorBuilderContext& context, EMotionFX::Node* emfxNode, AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshData> meshData,
            const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex, const BoneNameEmfxIndexMap& boneNameEmfxIndexMap, const ActorSettings& settings);

        EMotionFX::MeshBuilderSkinningInfo* ExtractSkinningInfo(EMotionFX::Actor* actor, AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshData> meshData,
            const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex, const BoneNameEmfxIndexMap& boneNameEmfxIndexMap, const ActorSettings& settings);

        void CreateSkinningMeshDeformer(EMotionFX::Actor* actor, EMotionFX::Node* node, EMotionFX::Mesh* mesh, EMotionFX::MeshBuilderSkinningInfo* skinningInfo, const ActorSettings& settings);

        void ExtractActorSettings(const AZ::SceneAPI::DataTypes::IActorGroup& actorGroup, ActorSettings& outSettings);

        void GatherGlobalTransform(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::Transform& outTransform);

        bool GetMaterialInfoForActorGroup(const ActorBuilderContext& context);
        void SetupMaterialDataForMesh(const ActorBuilderContext& context, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& meshNodeIndex);

        void GetNodeIndicesOfSelectedMeshes(ActorBuilderContext& context, NodeIndexSet& meshNodeIndexSet) const;

    private:
        AZStd::shared_ptr<AZ::GFxFramework::IMaterialGroup> m_materialGroup;
        AZStd::vector<AZ::u32> m_materialIndexMapForMesh;
        IConvertContext* m_convertContext;
    };

} // MotionCanvasPipeline
#endif //MOTIONCANVAS_GEM_ENABLED