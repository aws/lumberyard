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

#include "PhysX_precompiled.h"
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Tests/EditorTestUtilities.h>
#include <EditorColliderComponent.h>
#include <Physics/PhysicsTests.inl>
#include <EditorShapeColliderComponent.h>
#include <EditorColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <EditorSystemComponent.h>
#include <EditorForceRegionComponent.h>
#include <SystemComponent.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <ComponentDescriptors.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <PhysX/PhysXLocks.h>

namespace PhysXEditorTests
{
    ToolsApplicationMessageHandler* PhysXEditorFixture::s_messageHandler;
    AzToolsFramework::ToolsApplication* PhysXEditorFixture::s_app = nullptr;
    PhysXEditorSystemComponentEntity* PhysXEditorFixture::s_systemComponentEntity = nullptr;

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

    ToolsApplicationMessageHandler::ToolsApplicationMessageHandler()
    {
        m_gridMateMessageHandler = AZStd::make_unique<Physics::ErrorHandler>("GridMate");
        m_enginePathMessageHandler = AZStd::make_unique<Physics::ErrorHandler>("Engine Path");
        m_skippingDriveMessageHandler = AZStd::make_unique<Physics::ErrorHandler>("Skipping drive");
        m_storageDriveMessageHandler = AZStd::make_unique<Physics::ErrorHandler>("Storage drive");
        m_jsonComponentErrorHandler = AZStd::make_unique<Physics::ErrorHandler>("JsonSystem");
    }

    void PhysXEditorFixture::SetUpTestCase()
    {
        if (s_app == nullptr)
        {
            s_messageHandler = new ToolsApplicationMessageHandler();
            PhysX::SystemComponent::InitializePhysXSDK();

            s_app = aznew AzToolsFramework::ToolsApplication;
            AzToolsFramework::ToolsApplication::Descriptor appDescriptor;
            appDescriptor.m_enableDrilling = false;
            appDescriptor.m_useExistingAllocator = true;
            appDescriptor.m_modules.emplace_back(AZ::DynamicModuleDescriptor());
            appDescriptor.m_modules.back().m_dynamicLibraryPath = "Gem.LmbrCentral.Editor.ff06785f7145416b9d46fde39098cb0c.v0.1.0";

            s_app->Start(appDescriptor);
            for (AZ::ComponentDescriptor* descriptor : PhysX::GetDescriptors())
            {
                s_app->RegisterComponentDescriptor(descriptor);
            }
            s_app->RegisterComponentDescriptor(PhysX::EditorSystemComponent::CreateDescriptor());
            s_app->RegisterComponentDescriptor(PhysX::EditorShapeColliderComponent::CreateDescriptor());
            s_app->RegisterComponentDescriptor(PhysX::EditorColliderComponent::CreateDescriptor());
            s_app->RegisterComponentDescriptor(PhysX::EditorRigidBodyComponent::CreateDescriptor());
            s_app->RegisterComponentDescriptor(PhysX::EditorForceRegionComponent::CreateDescriptor());

            s_systemComponentEntity = aznew PhysXEditorSystemComponentEntity();
            PhysX::SystemComponent* systemComponent = s_systemComponentEntity->CreateComponent<PhysX::SystemComponent>();
            s_systemComponentEntity->ActivateComponent(*systemComponent);
            AZ::Component* editorSystemComponent =
                s_systemComponentEntity->CreateComponent(PhysX::EditorSystemComponent::TYPEINFO_Uuid());
            s_systemComponentEntity->ActivateComponent(*editorSystemComponent);
        }
    }

    void PhysXEditorFixture::TearDownTestCase()
    {
        if (s_app)
        {
            delete s_systemComponentEntity;
            s_systemComponentEntity = nullptr;
            s_app->Stop();
            delete s_app;
            s_app = nullptr;
            PhysX::SystemComponent::DestroyPhysXSDK();
            delete s_messageHandler;
            s_messageHandler = nullptr;
        }
    }

    void PhysXEditorFixture::SetUp()
    {
        m_defaultWorld = AZ::Interface<Physics::System>::Get()->CreateWorld(Physics::DefaultPhysicsWorldId);
        m_defaultWorld->SetEventHandler(this);
        Physics::DefaultWorldBus::Handler::BusConnect();

        m_oldConfiguration = AZ::Interface<PhysX::ConfigurationRequests>::Get()->GetConfiguration();
    }

    void PhysXEditorFixture::TearDown()
    {
        // Have to reset config to the old one, because other tests will pick up this config, but they rely
        // on old configurations instead. This can be deleted once asset directory is isolated between tests.
        // https://jira.agscollab.com/browse/LY-105551
        AZ::Interface<PhysX::ConfigurationRequests>::Get()->SetConfiguration(m_oldConfiguration);

        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultWorld.reset();

        // prevents warnings from the undo cache on subsequent tests
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::FlushUndo);
    }

    // DefaultWorldBus
    AZStd::shared_ptr<Physics::World> PhysXEditorFixture::GetDefaultWorld()
    {
        return m_defaultWorld;
    }

    // WorldEventHandler
    void PhysXEditorFixture::OnTriggerEnter(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(),
            &Physics::TriggerNotifications::OnTriggerEnter, triggerEvent);
    }

    void PhysXEditorFixture::OnTriggerExit(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(),
            &Physics::TriggerNotifications::OnTriggerExit, triggerEvent);
    }

    void PhysXEditorFixture::OnCollisionBegin(const Physics::CollisionEvent& event)
    {
        Physics::CollisionEvent collisionEvent = event;
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionBegin, collisionEvent);
        AZStd::swap(collisionEvent.m_body1, collisionEvent.m_body2);
        AZStd::swap(collisionEvent.m_shape1, collisionEvent.m_shape2);
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionBegin, collisionEvent);
    }

    void PhysXEditorFixture::OnCollisionPersist(const Physics::CollisionEvent& event)
    {
        Physics::CollisionEvent collisionEvent = event;
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionPersist, collisionEvent);
        AZStd::swap(collisionEvent.m_body1, collisionEvent.m_body2);
        AZStd::swap(collisionEvent.m_shape1, collisionEvent.m_shape2);
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionPersist, collisionEvent);
    }

    void PhysXEditorFixture::OnCollisionEnd(const Physics::CollisionEvent& event)
    {
        Physics::CollisionEvent collisionEvent = event;
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionEnd, collisionEvent);
        AZStd::swap(collisionEvent.m_body1, collisionEvent.m_body2);
        AZStd::swap(collisionEvent.m_shape1, collisionEvent.m_shape2);
        Physics::CollisionNotificationBus::QueueEvent(collisionEvent.m_body1->GetEntityId(),
            &Physics::CollisionNotifications::OnCollisionEnd, collisionEvent);
    }

    void PhysXEditorFixture::ValidateInvalidEditorShapeColliderComponentParams(const float radius, const float height)
    {
        // create an editor entity with a shape collider component and a cylinder shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorCylinderShapeComponentTypeId);
        editorEntity->Activate();
        //
        //{
        //    Physics::ErrorHandler warningHandler("Negative or zero cylinder dimensions are invalid");
        //    LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
        //        &LmbrCentral::CylinderShapeComponentRequests::SetRadius, radius);
        //
        //    // expect a warning if the radius is invalid
        //    int expectedWarningCount = radius <= 0.f ? 1 : 0;
        //    EXPECT_EQ(warningHandler.GetWarningCount(), expectedWarningCount);
        //}
        //
        //{
        //    Physics::ErrorHandler warningHandler("Negative or zero cylinder dimensions are invalid");
        //    LmbrCentral::CylinderShapeComponentRequestsBus::Event(editorEntity->GetId(),
        //        &LmbrCentral::CylinderShapeComponentRequests::SetHeight, height);
        //
        //    // expect a warning if the radius or height is invalid
        //    int expectedWarningCount = radius <= 0.f || height <= 0.f ? 1 : 0;
        //    EXPECT_EQ(warningHandler.GetWarningCount(), expectedWarningCount);
        //}
        //
        //EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        //
        //// since there was no editor rigid body component, the runtime entity should have a static rigid body
        //const auto* staticBody = gameEntity->FindComponent<PhysX::ShapeColliderComponent>()->GetStaticRigidBody();
        //const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());
        //
        //PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());
        //
        //// there should be no shapes on the rigid body because the cylinder radius and/or height is invalid
        //EXPECT_EQ(pxRigidStatic->getNbShapes(), 0);
    }
} // namespace PhysXEditorTests
