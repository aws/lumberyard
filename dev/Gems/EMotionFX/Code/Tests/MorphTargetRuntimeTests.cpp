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

#include "SystemComponentFixture.h"

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeMorphTargetNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MeshBuilder.h>
#include <EMotionFX/Source/MeshBuilderVertexAttributeLayers.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Pose.h>
#include <Integration/System/SystemCommon.h>
#include <MCore/Source/ReflectionSerializer.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    void PrintTo(const Vector3& vector, ::std::ostream* os)
    {
        *os << vector.GetX() << ' ' << vector.GetY() << ' ' << vector.GetZ();
    }
}

namespace EMotionFX
{
    class MorphTargetRuntimeFixture
        : public SystemComponentFixture
    {
    public:
        MorphTargetRuntimeFixture()
            : SystemComponentFixture()
            , m_points(
                {
                    AZ::Vector3(-1.0f, -1.0f, 0.0f),
                    AZ::Vector3(1.0f, -1.0f, 0.0f),
                    AZ::Vector3(-1.0f,  1.0f, 0.0f),

                    AZ::Vector3(1.0f, -1.0f, 0.0f),
                    AZ::Vector3(-1.0f,  1.0f, 0.0f),
                    AZ::Vector3(1.0f,  1.0f, 0.0f)
                })
        {
        }

        Mesh* CreatePlane(uint32 nodeIndex) const
        {
            const uint32 vertCount = static_cast<uint32>(m_points.size());
            const uint32 faceCount = vertCount / 3;

            auto meshBuilder = Integration::EMotionFXPtr<MeshBuilder>::MakeFromNew(MeshBuilder::Create(nodeIndex, vertCount, false));

            // Original vertex numbers
            MeshBuilderVertexAttributeLayerUInt32* orgVtxLayer = MeshBuilderVertexAttributeLayerUInt32::Create(
                    vertCount,
                    EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS,
                    false,
                    false
                    );
            meshBuilder->AddLayer(orgVtxLayer);

            // The positions layer
            MeshBuilderVertexAttributeLayerVector3* posLayer = MeshBuilderVertexAttributeLayerVector3::Create(
                    vertCount,
                    EMotionFX::Mesh::ATTRIB_POSITIONS,
                    false,
                    true
                    );
            meshBuilder->AddLayer(posLayer);

            // The normals layer
            MeshBuilderVertexAttributeLayerVector3* normalsLayer = MeshBuilderVertexAttributeLayerVector3::Create(
                    vertCount,
                    EMotionFX::Mesh::ATTRIB_NORMALS,
                    false,
                    true
                    );
            meshBuilder->AddLayer(normalsLayer);

            const int materialId = 0;
            const AZ::Vector3 normalVector(0.0f, 0.0f, 1.0f);
            for (uint32 faceNum = 0; faceNum < faceCount; ++faceNum)
            {
                meshBuilder->BeginPolygon(materialId);
                for (uint32 vertexOfFace = 0; vertexOfFace < 3; ++vertexOfFace)
                {
                    uint32 vertexNum = faceNum * 3 + vertexOfFace;
                    orgVtxLayer->SetCurrentVertexValue(&vertexNum);
                    posLayer->SetCurrentVertexValue(&m_points[vertexNum]);
                    normalsLayer->SetCurrentVertexValue(&normalVector);

                    meshBuilder->AddPolygonVertex(vertexNum);
                }
                meshBuilder->EndPolygon();
            }

            return meshBuilder->ConvertToEMotionFXMesh();
        }

        void ScaleMesh(Mesh* mesh)
        {
            const uint32 vertexCount = mesh->GetNumVertices();
            AZ::PackedVector3f* positions = static_cast<AZ::PackedVector3f*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            for (uint32 vertexNum = 0; vertexNum < vertexCount; ++vertexNum)
            {
                positions[vertexNum].Set(
                    positions[vertexNum].GetX() * m_scaleFactor,
                    positions[vertexNum].GetY() * m_scaleFactor,
                    positions[vertexNum].GetZ() * m_scaleFactor
                    );
            }
        }

        void AddParam(const char* name, const AZ::TypeId& type, const AZStd::string& defaultValue)
        {
            EMotionFX::Parameter* parameter = EMotionFX::ParameterFactory::Create(type);
            parameter->SetName(name);
            MCore::ReflectionSerializer::DeserializeIntoMember(parameter, "defaultValue", defaultValue);
            m_animGraph->AddParameter(parameter);
        }

        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_actor = Integration::EMotionFXPtr<Actor>::MakeFromNew(Actor::Create("testActor"));
            Node* rootNode = Node::Create("rootNode", m_actor->GetSkeleton());
            rootNode->SetNodeIndex(0);
            m_actor->AddNode(rootNode);

            Mesh* mesh = CreatePlane(rootNode->GetNodeIndex());
            m_actor->SetMesh(0, rootNode->GetNodeIndex(), mesh);

            m_morphSetup = MorphSetup::Create();
            m_actor->SetMorphSetup(0, m_morphSetup);

            Actor* morphActor = m_actor->Clone();
            Mesh* morphMesh = morphActor->GetMesh(0, rootNode->GetNodeIndex());
            ScaleMesh(morphMesh);
            MorphTargetStandard* morphTarget = MorphTargetStandard::Create(
                    /*captureTransforms=*/ false,
                    /*captureMeshDeforms=*/ true,
                    m_actor.get(),
                    morphActor,
                    "morphTarget"
                    );
            m_morphSetup->AddMorphTarget(morphTarget);

            // Without this call, the bind pose does not know about newly added
            // morph target (mMorphWeights.GetLength() == 0)
            m_actor->ResizeTransformData();
            m_actor->PostCreateInit(true, true, false);

            m_animGraph.reset(aznew AnimGraph);

            AddParam("FloatParam", azrtti_typeid<EMotionFX::FloatSliderParameter>(), "0.0");

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();

            BlendTreeMorphTargetNode* morphTargetNode = aznew BlendTreeMorphTargetNode();
            morphTargetNode->SetMorphTargetNames({"morphTarget"});

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            BlendTree* blendTree = aznew BlendTree();
            blendTree->SetName("testBlendTree");
            blendTree->AddChildNode(parameterNode);
            blendTree->AddChildNode(morphTargetNode);
            blendTree->AddChildNode(finalNode);
            blendTree->SetFinalNodeId(finalNode->GetId());

            m_stateMachine = aznew AnimGraphStateMachine();
            m_stateMachine->SetName("rootStateMachine");
            m_animGraph->SetRootStateMachine(m_stateMachine);
            m_stateMachine->AddChildNode(blendTree);
            m_stateMachine->SetEntryState(blendTree);

            m_stateMachine->InitAfterLoading(m_animGraph.get());

            // Create the connections once the port indices are known. The
            // parameter node's ports are not known until after
            // InitAfterLoading() is called
            morphTargetNode->AddConnection(
                parameterNode,
                parameterNode->FindOutputPortIndex("FloatParam"),
                BlendTreeMorphTargetNode::PORTID_INPUT_WEIGHT
            );
            finalNode->AddConnection(
                morphTargetNode,
                BlendTreeMorphTargetNode::PORTID_OUTPUT_POSE,
                BlendTreeFinalNode::PORTID_INPUT_POSE
            );

            m_motionSet = AZStd::make_unique<MotionSet>();
            m_motionSet->SetName("testMotionSet");

            m_actorInstance = Integration::EMotionFXPtr<ActorInstance>::MakeFromNew(ActorInstance::Create(m_actor.get()));

            m_animGraphInstance = AnimGraphInstance::Create(m_animGraph.get(), m_actorInstance.get(), m_motionSet.get());

            m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
        }

        void TearDown() override
        {
            m_actor.reset();
            m_actorInstance.reset();
            m_motionSet.reset();
            m_animGraph.reset();

            SystemComponentFixture::TearDown();
        }

        // The members that are EMotionFXPtrs are the ones that are owned by
        // the test fixture. The others are created by the fixture but owned by
        // the EMotionFX runtime.
        Integration::EMotionFXPtr<Actor> m_actor;
        MorphSetup* m_morphSetup = nullptr;
        AZStd::unique_ptr<AnimGraph> m_animGraph;
        AnimGraphStateMachine* m_stateMachine = nullptr;
        AZStd::unique_ptr<MotionSet> m_motionSet;
        Integration::EMotionFXPtr<ActorInstance> m_actorInstance;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        const float m_scaleFactor = 10.0f;
        const AZStd::fixed_vector<AZ::Vector3, 6> m_points;
    };

    MATCHER(AZVector3Eq, "")
    {
        using ::testing::get;
        return get<0>(arg).IsClose(get<1>(arg), 0.001f);
    }

    TEST_F(MorphTargetRuntimeFixture, TestMorphTargetMeshRuntime)
    {
        const float fps = 30.0f;
        const float updateInterval = 1.0f / fps;

        const Mesh* mesh = m_actor->GetMesh(0, 0);
        const uint32 vertexCount = mesh->GetNumOrgVertices();
        const AZ::PackedVector3f* positions = static_cast<AZ::PackedVector3f*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
        const AZStd::array<float, 4> weights {
            {0.0f, 0.5f, 1.0f, 0.0f}
        };
        AZStd::vector<AZ::Vector3> gotWeightedPoints;
        AZStd::vector<AZ::Vector3> expectedWeightedPoints;
        for (const float weight : weights)
        {
            gotWeightedPoints.clear();
            expectedWeightedPoints.clear();

            static_cast<MCore::AttributeFloat*>(m_animGraphInstance->FindParameter("FloatParam"))->SetValue(weight);
            GetEMotionFX().Update(updateInterval);
            m_actorInstance->UpdateMeshDeformers(updateInterval);

            for (uint32 vertexNum = 0; vertexNum < vertexCount; ++vertexNum)
            {
                gotWeightedPoints.emplace_back(AZ::Vector3(positions[vertexNum]));
            }

            for (const AZ::Vector3& neutralPoint : m_points)
            {
                const AZ::Vector3 delta = (neutralPoint * m_scaleFactor) - neutralPoint;
                expectedWeightedPoints.emplace_back(neutralPoint + delta * weight);
            }
            EXPECT_THAT(gotWeightedPoints, ::testing::Pointwise(AZVector3Eq(), expectedWeightedPoints));
        }
    }
}
