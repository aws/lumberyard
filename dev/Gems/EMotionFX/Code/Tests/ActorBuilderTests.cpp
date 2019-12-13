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
#include "InitSceneAPIFixture.h"
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

#include <SceneAPI/SceneCore/Mocks/Containers/MockScene.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Pipeline/RCExt/Actor/ActorBuilder.h>
#include <EMotionFX/Pipeline/RCExt/ExportContexts.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Groups/ActorGroup.h>
#include <Integration/System/SystemCommon.h>

#include <GFxFramework/MaterialIO/Material.h>

namespace AZ
{
    namespace GFxFramework
    {
        class MockMaterialGroup
            : public MaterialGroup
        {
        public:
            // ActorBuilder::GetMaterialInfoForActorGroup tries to read a source file first, and if that fails 
            //  (likely because it doesn't exist), tries to read one product file, followed by another. If they all fail,
            //  none of those files exist, so the function fails. The material read function called by 
            //  GetMaterialInfoForActorGroup is mocked to return true after a set number of false returns to mimic the
            //  behavior we would see if each given file is on disk.
            bool ReadMtlFile(const char* fileName) override
            {
                static AZ::u8 failCount = 0;
                if (failCount < NumberReadFailsBeforeSuccess)
                {
                    ++failCount;
                    return false;
                }
                else
                {
                    failCount = 0;
                    return true;
                }
            }

            size_t GetMaterialCount() override
            {
                return MaterialCount;
            }
            static size_t MaterialCount;
            static AZ::u8 NumberReadFailsBeforeSuccess;
        };

        size_t MockMaterialGroup::MaterialCount = 0;
        AZ::u8 MockMaterialGroup::NumberReadFailsBeforeSuccess = 0;
    }
}

namespace EMotionFX
{
    namespace Pipeline
    {
        class MockActorBuilder
            : public ActorBuilder
        {
        public:
            AZ_COMPONENT(MockActorBuilder, "{0C2537B5-6628-4076-BB09-CA1E57E59252}", EMotionFX::Pipeline::ActorBuilder)
        
        protected:
            void InstantiateMaterialGroup()
            {
                m_materialGroup = AZStd::make_shared<AZ::GFxFramework::MockMaterialGroup>();
            }
        };

        class MockMaterialRule
            : public AZ::SceneAPI::DataTypes::IMaterialRule
        {
        public:
            bool RemoveUnusedMaterials() const override
            {
                return true;
            }

            bool UpdateMaterials() const override
            {
                return true;
            }
        };
    }
    // This fixture is responsible for creating the scene description used by
    // the actor builder pipeline tests
    using ActorBuilderPipelineFixtureBase = InitSceneAPIFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent,
        EMotionFX::Pipeline::MockActorBuilder
    >;

    class ActorBuilderPipelineFixture
        : public ActorBuilderPipelineFixtureBase
    {
    public:
        void SetUp() override
        {
            ActorBuilderPipelineFixtureBase::SetUp();

            m_actor = EMotionFX::Integration::EMotionFXPtr<EMotionFX::Actor>::MakeFromNew(EMotionFX::Actor::Create("TestActor"));

            // Set up the scene graph
            m_scene = new AZ::SceneAPI::Containers::MockScene("MockScene");
            m_scene->SetOriginalSceneOrientation(AZ::SceneAPI::Containers::Scene::SceneOrientation::ZUp);
            AZStd::string testSourceFile;
            AzFramework::StringFunc::Path::Join(GetWorkingDirectory(), "TestFile.fbx", testSourceFile);
            AzFramework::StringFunc::AssetDatabasePath::Normalize(testSourceFile);
            m_scene->SetSource(testSourceFile, AZ::Uuid::CreateRandom());

            AZ::SceneAPI::Containers::SceneGraph& graph = m_scene->GetGraph();

            AZStd::shared_ptr<AZ::SceneData::GraphData::BoneData> boneData = AZStd::make_shared<AZ::SceneData::GraphData::BoneData>();
            graph.AddChild(graph.GetRoot(), "testRootBone", boneData);

            // Set up our base shape
            AZStd::shared_ptr<AZ::SceneData::GraphData::MeshData> meshData = AZStd::make_shared<AZ::SceneData::GraphData::MeshData>();
            AZStd::vector<AZ::Vector3> unmorphedVerticies
            {
                AZ::Vector3(0, 0, 0),
                AZ::Vector3(1, 0, 0),
                AZ::Vector3(0, 1, 0)
            };
            for (const AZ::Vector3& vertex : unmorphedVerticies)
            {
                meshData->AddPosition(vertex);
            }
            meshData->AddNormal(AZ::Vector3(0,
                0, 1));
            meshData->AddNormal(AZ::Vector3(0, 0, 1));
            meshData->AddNormal(AZ::Vector3(0, 0, 1));
            meshData->SetVertexIndexToControlPointIndexMap(0, 0);
            meshData->SetVertexIndexToControlPointIndexMap(1, 1);
            meshData->SetVertexIndexToControlPointIndexMap(2, 2);
            meshData->AddFace(0, 1, 2);
            AZ::SceneAPI::Containers::SceneGraph::NodeIndex meshNodeIndex = graph.AddChild(graph.GetRoot(), "testMesh", meshData);
        }

        AZ::SceneAPI::Events::ProcessingResult Process(EMotionFX::Pipeline::Group::ActorGroup& actorGroup, AZStd::vector<AZStd::string>& materialReferences)
        {
            AZ::SceneAPI::Events::ProcessingResultCombiner result;
            AZStd::string workingDir = GetWorkingDirectory();
            EMotionFX::Pipeline::ActorBuilderContext actorBuilderContext(*m_scene, workingDir, actorGroup, m_actor.get(), materialReferences, AZ::RC::Phase::Construction);
            result += AZ::SceneAPI::Events::Process(actorBuilderContext);
            result += AZ::SceneAPI::Events::Process<EMotionFX::Pipeline::ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Filling);
            result += AZ::SceneAPI::Events::Process<EMotionFX::Pipeline::ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Finalizing);
            return result.GetResult();
        }

        void TearDown() override
        {
            m_actor.reset();
            delete m_scene;
            m_scene = nullptr;

            ActorBuilderPipelineFixtureBase::TearDown();
        }

        void TestSuccessCase(const AZStd::vector<AZStd::string>& expectedMaterialReferences)
        {
            // Set up the actor group, which controls which parts of the scene graph
            // are used to generate the actor
            EMotionFX::Pipeline::Group::ActorGroup actorGroup;
            actorGroup.SetName("testActor");
            actorGroup.SetSelectedRootBone("testRootBone");
            actorGroup.GetSceneNodeSelectionList().AddSelectedNode("testMesh");
            actorGroup.GetBaseNodeSelectionList().AddSelectedNode("testMesh");

            // do something here to make sure there are material rules in the actor?
            if (expectedMaterialReferences.size() > 0)
            {
                AZStd::shared_ptr<EMotionFX::Pipeline::MockMaterialRule> materialRule = AZStd::make_shared<EMotionFX::Pipeline::MockMaterialRule>();
                actorGroup.GetRuleContainer().AddRule(materialRule);
            }            

            AZStd::vector<AZStd::string> materialReferences;

            const AZ::SceneAPI::Events::ProcessingResult result = Process(actorGroup, materialReferences);
            ASSERT_EQ(result, AZ::SceneAPI::Events::ProcessingResult::Success) << "Failed to build actor";

            ASSERT_EQ(materialReferences.size(), expectedMaterialReferences.size());

            if (expectedMaterialReferences.size() > 0)
            {
                for (const AZStd::string& expectedMaterialPath : expectedMaterialReferences)
                {
                    ASSERT_TRUE(AZStd::find(materialReferences.begin(), materialReferences.end(), expectedMaterialPath) != materialReferences.end());
                }
            }
        }

        void TestSuccessCase(const char* expectedMaterialReference)
        {
            AZStd::vector<AZStd::string> expectedMaterialReferences;
            expectedMaterialReferences.push_back(expectedMaterialReference);
            TestSuccessCase(expectedMaterialReferences);
        }

        void TestSuccessCaseNoDependencies()
        {
            AZStd::vector<AZStd::string> expectedMaterialReferences;
            TestSuccessCase(expectedMaterialReferences);
        }

        EMotionFX::Integration::EMotionFXPtr<EMotionFX::Actor> m_actor;
        AZ::SceneAPI::Containers::Scene* m_scene = nullptr;
    };

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_MaterialReferences_NoReferences)
    {
        // Set up the actor group, which controls which parts of the scene graph
        // are used to generate the actor
        TestSuccessCaseNoDependencies();
    }

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_MaterialReferences_OneSourceReference_ExpectAbsolutePath)
    {
        AZStd::string expectedMaterialReference;
        AzFramework::StringFunc::AssetDatabasePath::Join(GetWorkingDirectory(), "TestFile.mtl", expectedMaterialReference);
        
        AZ::GFxFramework::MockMaterialGroup::NumberReadFailsBeforeSuccess = 0;
        AZ::GFxFramework::MockMaterialGroup::MaterialCount = 1;
        TestSuccessCase(expectedMaterialReference.c_str());
    }

    // The following two tests are commented out due to needing emfx tests to create an AzFramework::Application instead
    //  instead of a AZ::ComponentApplication. These tests passed when the switch was made, but other tests in other
    //  test fixtures began to fail with allocator cleanup issues. These tests should be uncommented when those allocator 
    //  issues are fixed.

    // TEST_F(ActorBuilderPipelineFixture, ActorBuilder_MaterialReferences_OneProductReference_ExpectRelativeMaterialPath)
    // {
    //     AZStd::string normalizedWorkingDir = GetWorkingDirectory();
    //     AzFramework::StringFunc::Path::Normalize(normalizedWorkingDir);
    // 
    //     AZStd::string workingDirLastComponent;
    //     AzFramework::StringFunc::Path::GetComponent(normalizedWorkingDir.c_str(), workingDirLastComponent, 1, true);
    // 
    //     AZStd::string expectedMaterialReference;
    //     AzFramework::StringFunc::AssetDatabasePath::Join(workingDirLastComponent.c_str(), "testActor.mtl", expectedMaterialReference);
    //     AZStd::to_lower(expectedMaterialReference.begin(), expectedMaterialReference.end());
    //     
    //     AZ::GFxFramework::MockMaterialGroup::NumberReadFailsBeforeSuccess = 1;
    //     AZ::GFxFramework::MockMaterialGroup::MaterialCount = 1;
    //     TestSuccessCase(expectedMaterialReference.c_str());
    // }
    
    // TEST_F(ActorBuilderPipelineFixture, ActorBuilder_MaterialReferences_OneProductReference_ExpectRelativeDccPath)
    // {
    //     AZStd::string normalizedWorkingDir = GetWorkingDirectory();
    //     AzFramework::StringFunc::Path::Normalize(normalizedWorkingDir);
    // 
    //     AZStd::string workingDirLastComponent;
    //     AzFramework::StringFunc::Path::GetComponent(normalizedWorkingDir.c_str(), workingDirLastComponent, 1, true);
    // 
    //     AZStd::string expectedMaterialReference;
    //     AzFramework::StringFunc::AssetDatabasePath::Join(workingDirLastComponent.c_str(), "testActor.dccmtl", expectedMaterialReference);
    //     AZStd::to_lower(expectedMaterialReference.begin(), expectedMaterialReference.end());
    // 
    //     AZ::GFxFramework::MockMaterialGroup::NumberReadFailsBeforeSuccess = 2;
    //     AZ::GFxFramework::MockMaterialGroup::MaterialCount = 1;
    //     TestSuccessCase(expectedMaterialReference.c_str());
    // }
}