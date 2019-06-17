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

#include <AzTest/AzTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>

namespace AzToolsFramework
{
    class EditorEntityContextComponentTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_app.Start(m_descriptor);
            // Without this, the user settings component would sometimes attempt to save
            // changes on shutdown. In some cases this would cause a crash while the unit test
            // was running, because the environment wasn't setup for it to save these settings.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }
        void TearDown() override
        {
            m_app.Stop();
        }
        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;
    };

    TEST_F(EditorEntityContextComponentTests, EditorEntityContextTests_CreateEditorEntity_CreatesValidEntity)
    {
        AZStd::string entityName("TestName");
        AZ::Entity* createdEntity = nullptr;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntity,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateEditorEntity,
            entityName.c_str());
        EXPECT_NE(createdEntity, nullptr);
        EXPECT_EQ(entityName.compare(createdEntity->GetName()), 0);
        EXPECT_TRUE(createdEntity->GetId().IsValid());
    }

    TEST_F(EditorEntityContextComponentTests, EditorEntityContextTests_CreateEditorEntityWithValidId_CreatesValidEntity)
    {
        AZ::EntityId validId(AZ::Entity::MakeId());
        EXPECT_TRUE(validId.IsValid());
        AZStd::string entityName("TestName");
        AZ::Entity* createdEntity = nullptr;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntity,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateEditorEntityWithId,
            entityName.c_str(),
            validId);
        EXPECT_NE(createdEntity, nullptr);
        EXPECT_EQ(entityName.compare(createdEntity->GetName()), 0);
        EXPECT_EQ(createdEntity->GetId(), validId);
    }

    TEST_F(EditorEntityContextComponentTests, EditorEntityContextTests_CreateEditorEntityWithInvalidId_NoEntityCreated)
    {
        AZ::EntityId invalidId;
        EXPECT_FALSE(invalidId.IsValid());
        AZStd::string entityName("TestName");
        AZ::Entity* createdEntity = nullptr;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntity,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateEditorEntityWithId,
            entityName.c_str(),
            invalidId);
        EXPECT_EQ(createdEntity, nullptr);
    }

    TEST_F(EditorEntityContextComponentTests, EditorEntityContextTests_CreateEditorEntityWithInUseId_NoEntityCreated)
    {
        // Create an entity so we can grab an in-use entity ID.
        AZStd::string entityName("TestName");
        AZ::Entity* createdEntity = nullptr;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntity,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateEditorEntity,
            entityName.c_str());
        EXPECT_NE(createdEntity, nullptr);
        EXPECT_TRUE(createdEntity->GetId().IsValid());

        // Attempt to create another entity with the same ID, and verify this call fails.
        AZ::Entity* secondEntity = nullptr;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            secondEntity,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateEditorEntityWithId,
            entityName.c_str(),
            createdEntity->GetId());
        EXPECT_EQ(secondEntity, nullptr);
    }
}
