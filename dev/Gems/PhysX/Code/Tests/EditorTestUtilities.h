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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <Physics/PhysicsTests.h>
#include <PhysX/ConfigurationBus.h>

namespace PhysXEditorTests
{
    using EntityPtr = AZStd::unique_ptr<AZ::Entity>;

    //! Creates a default editor entity in an inactive state.
    EntityPtr CreateInactiveEditorEntity(const char* entityName);

    //! Creates and activates a game entity from an editor entity.
    EntityPtr CreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity);

    //! Used to intercept various messages which get printed to the console during the startup of a tools application
    //! but are not relevant for testing.
    class ToolsApplicationMessageHandler
    {
    public:
        ToolsApplicationMessageHandler();
    private:
        AZStd::unique_ptr<Physics::ErrorHandler> m_gridMateMessageHandler;
        AZStd::unique_ptr<Physics::ErrorHandler> m_enginePathMessageHandler;
        AZStd::unique_ptr<Physics::ErrorHandler> m_skippingDriveMessageHandler;
        AZStd::unique_ptr<Physics::ErrorHandler> m_storageDriveMessageHandler;
        AZStd::unique_ptr<Physics::ErrorHandler> m_jsonComponentErrorHandler;
    };

    //! Class used for loading system components from this gem.
    class PhysXEditorSystemComponentEntity :
        public AZ::Entity
    {
        friend class PhysXEditorFixture;
    };

    //! Test fixture which creates a tools application, loads the PhysX runtime gem and creates a default physics world.
    //! The application is created for the whole test case, rather than individually for each test, due to a known
    //! problem with buses when repeatedly loading and unloading gems. A new default world is created for each test.
    class PhysXEditorFixture
        : public UnitTest::AllocatorsTestFixture
        , public Physics::DefaultWorldBus::Handler
        , public Physics::WorldEventHandler
    {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override;

        // WorldEventHandler
        void OnTriggerEnter(const Physics::TriggerEvent& triggerEvent) override;
        void OnTriggerExit(const Physics::TriggerEvent& triggerEvent) override;
        void OnCollisionBegin(const Physics::CollisionEvent& event) override;
        void OnCollisionPersist(const Physics::CollisionEvent& event) override;
        void OnCollisionEnd(const Physics::CollisionEvent& event) override;

        static AzToolsFramework::ToolsApplication* s_app;
        static PhysXEditorSystemComponentEntity* s_systemComponentEntity;
        static ToolsApplicationMessageHandler* s_messageHandler;
        AZStd::shared_ptr<Physics::World> m_defaultWorld;

        PhysX::Configuration m_oldConfiguration;

        // workaround for parameterized tests causing issues with this (and any derived) fixture
        void ValidateInvalidEditorShapeColliderComponentParams(float radius, float height);
    };
} // namespace PhysXEditorTests
