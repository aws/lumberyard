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

#include <NvCloth_precompiled.h>

#include <AzTest/AzTest.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <Components/ClothComponent.h>
#include <Components/EditorClothComponent.h>
#include <Utils/Allocators.h>

namespace NvCloth
{
    namespace
    {
        using EntityPtr = AZStd::unique_ptr<AZ::Entity>;

        const AZ::Uuid EditorMeshComponentTypeId = "{FC315B86-3280-4D03-B4F0-5553D7D08432}";
        const AZ::Uuid MeshComponentTypeId = "{2F4BAD46-C857-4DCB-A454-C412DE67852A}";
        const AZ::Uuid EditorActorComponentTypeId = "{A863EE1B-8CFD-4EDD-BA0D-1CEC2879AD44}";
        const AZ::Uuid ActorComponentTypeId = "{BDC97E7F-A054-448B-A26F-EA2B5D78E377}";
    }

    class NvClothEditorTest
        : public ::testing::Test
    {
    protected:
        static void SetUpTestCase()
        {
            if (s_app == nullptr)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
                AZ::AllocatorInstance<AzClothAllocator>::Create();

                s_app = aznew AzToolsFramework::ToolsApplication;
                AzToolsFramework::ToolsApplication::Descriptor appDescriptor;
                appDescriptor.m_useExistingAllocator = true;
                appDescriptor.m_modules.emplace_back(AZ::DynamicModuleDescriptor());
                appDescriptor.m_modules.back().m_dynamicLibraryPath = "Gem.LmbrCentral.Editor.ff06785f7145416b9d46fde39098cb0c.v0.1.0";
                appDescriptor.m_modules.emplace_back(AZ::DynamicModuleDescriptor());
                appDescriptor.m_modules.back().m_dynamicLibraryPath = "Gem.EMotionFX.Editor.044a63ea67d04479aa5daf62ded9d9ca.v0.1.0";

                s_app->Start(appDescriptor);

                s_app->RegisterComponentDescriptor(ClothComponent::CreateDescriptor());
                s_app->RegisterComponentDescriptor(EditorClothComponent::CreateDescriptor());
            }
        }

        static void TearDownTestCase()
        {
            if (s_app)
            {
                s_app->Stop();
                delete s_app;
                s_app = nullptr;

                AZ::AllocatorInstance<AzClothAllocator>::Destroy();
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
        }

        static AzToolsFramework::ToolsApplication* s_app;
    };

    AzToolsFramework::ToolsApplication* NvClothEditorTest::s_app = nullptr;

    EntityPtr CreateInactiveEditorEntity(const char* entityName)
    {
        AZ::Entity* entity = nullptr;
        UnitTest::CreateDefaultEditorEntity(entityName, &entity);
        entity->Deactivate();

        return AZStd::unique_ptr<AZ::Entity>(entity);
    }

    EntityPtr CreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity)
    {
        EntityPtr gameEntity = AZStd::make_unique<AZ::Entity>();
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::PreExportEntity, *editorEntity, *gameEntity);
        gameEntity->Init();
        gameEntity->Activate();
        return gameEntity;
    }

    TEST_F(NvClothEditorTest, EditorClothComponent_DependencyMissing_EntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        entity->CreateComponent<EditorClothComponent>();

        // the entity should not be in a valid state because the cloth component requires a mesh or an actor component
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(NvClothEditorTest, EditorClothComponent_MeshDependencySatisfied_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        entity->CreateComponent<EditorClothComponent>();
        entity->CreateComponent(EditorMeshComponentTypeId);

        // the entity should be in a valid state because the cloth component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(NvClothEditorTest, EditorClothComponent_ActorDependencySatisfied_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        entity->CreateComponent<EditorClothComponent>();
        entity->CreateComponent(EditorActorComponentTypeId);

        // the entity should be in a valid state because the cloth component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(NvClothEditorTest, EditorClothComponent_MultipleClothComponents_EntityIsValid)
    {

        EntityPtr entity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        entity->CreateComponent<EditorClothComponent>();
        entity->CreateComponent(EditorMeshComponentTypeId);

        // the cloth component should be compatible with multiple cloth components
        entity->CreateComponent<EditorClothComponent>();
        entity->CreateComponent<EditorClothComponent>();

        // the entity should be in a valid state because the cloth component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(NvClothEditorTest, EditorClothComponent_ClothWithMesh_CorrectRuntimeComponents)
    {
        // create an editor entity with a cloth component and a mesh component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        editorEntity->CreateComponent<EditorClothComponent>();
        editorEntity->CreateComponent(EditorMeshComponentTypeId);
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<ClothComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent(MeshComponentTypeId) != nullptr);
    }

    TEST_F(NvClothEditorTest, EditorClothComponent_ClothWithActor_CorrectRuntimeComponents)
    {
        // create an editor entity with a cloth component and an actor component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ClothComponentEditorEntity");
        editorEntity->CreateComponent<EditorClothComponent>();
        editorEntity->CreateComponent(EditorActorComponentTypeId);
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<ClothComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent(ActorComponentTypeId) != nullptr);
    }

    AZ_UNIT_TEST_HOOK();
    
} // namespace NvCloth
