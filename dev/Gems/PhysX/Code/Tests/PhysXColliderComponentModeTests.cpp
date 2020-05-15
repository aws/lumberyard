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

#include <PhysX_precompiled.h>

#include "TestColliderComponent.h"

#include <AzToolsFramework/UnitTest/ComponentModeTestFixture.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>

namespace UnitTest
{
    class PhysXColliderComponentModeTest
        : public ComponentModeTestFixture
    {
    protected:
        using EntityPtr = AZStd::unique_ptr<AZ::Entity>;
        
        void SetUp() override
        {
            ComponentModeTestFixture::SetUp();

            m_app.RegisterComponentDescriptor(TestColliderComponentMode::CreateDescriptor());

            SetupInteractionHandler();
        }

        void TearDown() override
        {
            ComponentModeTestFixture::TearDown();
        }

        void SetupInteractionHandler()
        {
            const auto viewportHandlerBuilder =
                [this](const AzToolsFramework::EditorVisibleEntityDataCache* entityDataCache)
            {
                // create the default viewport (handles ComponentMode)
                AZStd::unique_ptr<AzToolsFramework::EditorDefaultSelection> defaultSelection =
                    AZStd::make_unique<AzToolsFramework::EditorDefaultSelection>(entityDataCache);

                // override the phantom widget so we can use our custom test widget
                defaultSelection->SetOverridePhantomWidget(&m_editorActions.m_testWidget);

                return defaultSelection;
            };

            // setup default editor interaction model with the phantom widget overridden
            AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
                AzToolsFramework::GetEntityContextId(),
                &AzToolsFramework::EditorInteractionSystemViewportSelection::SetHandler, viewportHandlerBuilder);
        }

        EntityPtr CreateColliderComponent()
        {
            AZ::Entity* entity = nullptr;
            AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

            entity->Deactivate();

            // Add placeholder component which implements component mode.
            entity->CreateComponent<TestColliderComponentMode>();

            entity->Activate();

            SelectEntities({ entityId });

            AZStd::unique_ptr<AZ::Entity> ptr;
            ptr.reset(entity);
            return AZStd::move(ptr);
        }
    };

    TEST_F(PhysXColliderComponentModeTest, MouseWheelUpShouldSetNextMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the mouse wheel is scrolled while holding ctrl
        AzToolsFramework::ViewportInteraction::MouseInteractionEvent
            interactionEvent(AzToolsFramework::ViewportInteraction::MouseInteraction(), 1.0f);
        interactionEvent.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Wheel;
        interactionEvent.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers =
            static_cast<AZ::u32>(AzToolsFramework::ViewportInteraction::KeyboardModifier::Ctrl);

        bool handled = false;
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::BroadcastResult(handled,
            &AzToolsFramework::ViewportInteraction::MouseViewportRequests::HandleMouseInteraction,
            interactionEvent);

        // Then the component mode is cycled.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_TRUE(handled);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, MouseWheelDownShouldSetPreviousMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the mouse wheel is scrolled while holding ctrl
        AzToolsFramework::ViewportInteraction::MouseInteractionEvent
            interactionEvent(AzToolsFramework::ViewportInteraction::MouseInteraction(), -1.0f);
        interactionEvent.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Wheel;
        interactionEvent.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers =
            static_cast<AZ::u32>(AzToolsFramework::ViewportInteraction::KeyboardModifier::Ctrl);

        bool handled = false;
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::BroadcastResult(handled,
            &AzToolsFramework::ViewportInteraction::MouseViewportRequests::HandleMouseInteraction,
            interactionEvent);

        // Then the component mode is cycled.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_TRUE(handled);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Rotation, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey1ShouldSetSizeMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the '1' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_1);

        // Then the component mode is set to Size.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey2ShouldSetSizeMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the '2' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_2);

        // Then the component mode is set to Offset.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey3ShouldSetSizeMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the '3' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_3);

        // Then the component mode is set to Rotation.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Rotation, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetSphereRadius)
    {
        // Given there is a sphere collider in component mode.
        auto colliderEntity = CreateColliderComponent();
        float radius = 5.0f;
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::Sphere);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetSphereRadius(radius);

        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // The the sphere radius should be reset.
        radius = colliderEntity->FindComponent<TestColliderComponentMode>()->GetSphereRadius();
        EXPECT_FLOAT_EQ(0.5f, radius);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetCapsuleSize)
    {
        // Given there is a capsule collider in component mode.
        auto colliderEntity = CreateColliderComponent();
        float height = 10.0f;
        float radius = 2.5f;
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::Capsule);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetCapsuleHeight(height);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetCapsuleRadius(radius);

        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // Then the capsule size should be reset.
        height = colliderEntity->FindComponent<TestColliderComponentMode>()->GetCapsuleHeight();
        radius = colliderEntity->FindComponent<TestColliderComponentMode>()->GetCapsuleRadius();
        EXPECT_FLOAT_EQ(1.0f, height);
        EXPECT_FLOAT_EQ(0.25f, radius);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetAssetScale)
    {
        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Vector3 assetScale(10.0f, 10.0f, 10.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::PhysicsAsset);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetAssetScale(assetScale);

        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // Then the asset scale should be reset.
        assetScale = colliderEntity->FindComponent<TestColliderComponentMode>()->GetAssetScale();
        EXPECT_THAT(assetScale, IsClose(AZ::Vector3::CreateOne()));
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetOffset)
    {
        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Vector3 offset(5.0f, 6.0f, 7.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetColliderOffset(offset);
        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Offset);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // Then the collider offset should be reset.
        offset = colliderEntity->FindComponent<TestColliderComponentMode>()->GetColliderOffset();
        EXPECT_THAT(offset, IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetRotation)
    {
        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Quaternion rotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), 45.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetColliderRotation(rotation);
        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Rotation);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // Then the collider rotation should be reset.
        rotation = colliderEntity->FindComponent<TestColliderComponentMode>()->GetColliderRotation();
        EXPECT_THAT(rotation, IsClose(AZ::Quaternion::CreateIdentity()));
    }
} // namespace UnitTest
