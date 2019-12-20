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
            using namespace AzToolsFramework;

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
            using namespace AzToolsFramework;

            const auto viewportHandlerBuilder =
                [this](const AzToolsFramework::EditorVisibleEntityDataCache* entityDataCache)
            {
                // create the default viewport (handles ComponentMode)
                AZStd::unique_ptr<EditorDefaultSelection> defaultSelection =
                    AZStd::make_unique<EditorDefaultSelection>(entityDataCache);

                // override the phantom widget so we can use our custom test widget
                defaultSelection->SetOverridePhantomWidget(&m_editorActions.m_testWidget);

                return defaultSelection;
            };

            // setup default editor interaction model with the phantom widget overridden
            EditorInteractionSystemViewportSelectionRequestBus::Event(
                GetEntityContextId(), &EditorInteractionSystemViewportSelection::SetHandler, viewportHandlerBuilder);

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
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequests::SubMode subMode = ColliderComponentModeRequests::SubMode::NumModes;
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the mouse wheel is scrolled while holding ctrl
        ViewportInteraction::MouseInteractionEvent interactionEvent(ViewportInteraction::MouseInteraction(), 1.0f);
        interactionEvent.m_mouseEvent = ViewportInteraction::MouseEvent::Wheel;
        interactionEvent.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers = static_cast<AZ::u32>(ViewportInteraction::KeyboardModifier::Ctrl);

        bool handled = false;
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::BroadcastResult(handled,
            &AzToolsFramework::ViewportInteraction::MouseViewportRequests::HandleMouseInteraction,
            interactionEvent);

        // Then the component mode is cycled.
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_TRUE(handled);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Offset, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, MouseWheelDownShouldSetPreviousMode)
    {
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequests::SubMode subMode = ColliderComponentModeRequests::SubMode::NumModes;
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the mouse wheel is scrolled while holding ctrl
        ViewportInteraction::MouseInteractionEvent interactionEvent(ViewportInteraction::MouseInteraction(), -1.0f);
        interactionEvent.m_mouseEvent = ViewportInteraction::MouseEvent::Wheel;
        interactionEvent.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers = static_cast<AZ::u32>(ViewportInteraction::KeyboardModifier::Ctrl);

        bool handled = false;
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::BroadcastResult(handled,
            &AzToolsFramework::ViewportInteraction::MouseViewportRequests::HandleMouseInteraction,
            interactionEvent);

        // Then the component mode is cycled.
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_TRUE(handled);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Rotation, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey1ShouldSetSizeMode)
    {
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequests::SubMode subMode = ColliderComponentModeRequests::SubMode::NumModes;
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the '1' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_1);

        // Then the component mode is set to Size.
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Dimensions, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey2ShouldSetSizeMode)
    {
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequests::SubMode subMode = ColliderComponentModeRequests::SubMode::NumModes;
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the '2' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_2);

        // Then the component mode is set to Offset.
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Offset, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey3ShouldSetSizeMode)
    {
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequests::SubMode subMode = ColliderComponentModeRequests::SubMode::NumModes;
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Dimensions, subMode);

        // When the '3' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_3);

        // Then the component mode is set to Rotation.
        ColliderComponentModeRequestBus::BroadcastResult(subMode, &ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(ColliderComponentModeRequests::SubMode::Rotation, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetSphereRadius)
    {
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a sphere collider in component mode.
        auto colliderEntity = CreateColliderComponent();
        float radius = 5.0f;
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::Sphere);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetSphereRadius(radius);

        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequestBus::Broadcast(&ColliderComponentModeRequests::SetCurrentMode, ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // The the sphere radius should be reset.
        radius = colliderEntity->FindComponent<TestColliderComponentMode>()->GetSphereRadius();
        EXPECT_FLOAT_EQ(0.5f, radius);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetCapsuleSize)
    {
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a capsule collider in component mode.
        auto colliderEntity = CreateColliderComponent();
        float height = 10.0f;
        float radius = 2.5f;
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::Capsule);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetCapsuleHeight(height);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetCapsuleRadius(radius);

        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequestBus::Broadcast(&ColliderComponentModeRequests::SetCurrentMode, ColliderComponentModeRequests::SubMode::Dimensions);

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
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Vector3 assetScale(10.0f, 10.0f, 10.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::PhysicsAsset);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetAssetScale(assetScale);

        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequestBus::Broadcast(&ColliderComponentModeRequests::SetCurrentMode, ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // Then the asset scale should be reset.
        assetScale = colliderEntity->FindComponent<TestColliderComponentMode>()->GetAssetScale();
        EXPECT_THAT(assetScale, IsClose(AZ::Vector3::CreateOne()));
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetOffset)
    {
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Vector3 offset(5.0f, 6.0f, 7.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetColliderOffset(offset);
        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequestBus::Broadcast(&ColliderComponentModeRequests::SetCurrentMode, ColliderComponentModeRequests::SubMode::Offset);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // Then the collider offset should be reset.
        offset = colliderEntity->FindComponent<TestColliderComponentMode>()->GetColliderOffset();
        EXPECT_THAT(offset, IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetRotation)
    {
        using namespace PhysX;
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;

        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Quaternion rotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), 45.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetColliderRotation(rotation);
        SelectEntities({ colliderEntity->GetId() });
        EnterComponentMode<TestColliderComponentMode>();

        ColliderComponentModeRequestBus::Broadcast(&ColliderComponentModeRequests::SetCurrentMode, ColliderComponentModeRequests::SubMode::Rotation);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_testWidget, Qt::Key_R);

        // Then the collider rotation should be reset.
        rotation = colliderEntity->FindComponent<TestColliderComponentMode>()->GetColliderRotation();
        EXPECT_THAT(rotation, IsClose(AZ::Quaternion::CreateIdentity()));
    }
} // namespace UnitTest