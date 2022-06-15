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
#include "LmbrCentral_precompiled.h"

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Mocks/IRendererMock.h>
#include <Mocks/I3DEngineMock.h>
#include "Mocks/ISystemMock.h"

#include <Shape/PolygonPrismShapeComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <LmbrCentral/Rendering/ClipVolumeComponentBus.h>
#include <Rendering/ClipVolumeComponent.h>

namespace UnitTest
{
    class ClipVolumeComponentTests
        : public UnitTest::AllocatorsTestFixture
        , public LmbrCentral::ClipVolumeComponentNotificationBus::Handler
    {
    public:
        ClipVolumeComponentTests()
            : AllocatorsTestFixture()
        {}

    protected:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_priorEnv = gEnv;
            m_data.reset(new DataMembers);

            memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_data->m_stubEnv.pRenderer = &m_data->m_renderer;
            m_data->m_stubEnv.p3DEngine = &m_data->m_3DEngine;
            m_data->m_stubEnv.pSystem = &m_data->m_system;
            
            ON_CALL(m_data->m_3DEngine, CreateClipVolume())
                    .WillByDefault(::testing::Return(m_expectedClipVolumePtr));

            gEnv = &m_data->m_stubEnv;

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_clipVolumeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::ClipVolumeComponent::CreateDescriptor());
            m_polygonPrismComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::PolygonPrismShapeComponent::CreateDescriptor());

            m_transformComponentDescriptor->Reflect(m_serializeContext.get());
            m_clipVolumeComponentDescriptor->Reflect(m_serializeContext.get());
            m_polygonPrismComponentDescriptor->Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            LmbrCentral::ClipVolumeComponentNotificationBus::Handler::BusDisconnect();
            
            m_clipVolumeComponentDescriptor.reset();
            m_transformComponentDescriptor.reset();
            m_polygonPrismComponentDescriptor.reset();

            m_serializeContext.reset();

            gEnv = m_priorEnv;
            m_data.reset();

            AllocatorsTestFixture::TearDown();
        }

        void CreateDefaultSetup(AZ::Entity& entity)
        {
            entity.CreateComponent<AzFramework::TransformComponent>();
            entity.CreateComponent<LmbrCentral::PolygonPrismShapeComponent>();
            entity.CreateComponent<LmbrCentral::ClipVolumeComponent>();

            entity.Init();
            entity.Activate();
        }

        struct DataMembers
        {
            ::testing::NiceMock<SystemMock> m_system;
            ::testing::NiceMock<IRendererMock> m_renderer;
            ::testing::NiceMock<I3DEngineMock> m_3DEngine;
            SSystemGlobalEnvironment m_stubEnv;
        };
        
        void ConnectBus(AZ::EntityId entityId)
        {
            LmbrCentral::ClipVolumeComponentNotificationBus::Handler::BusConnect(entityId);
        }
        
        void OnClipVolumeCreated(IClipVolume* clipVolume) override
        {
            m_clipVolume = clipVolume;
        }

        void OnClipVolumeDestroyed(IClipVolume* clipVolume) override
        {
            m_receivedClipVolumeDestroy = true;
        }

        IClipVolume* m_expectedClipVolumePtr = reinterpret_cast<IClipVolume*>(0x1234);
        AZStd::unique_ptr<DataMembers> m_data;
        SSystemGlobalEnvironment* m_priorEnv = nullptr;
        IClipVolume* m_clipVolume = nullptr;
        bool m_receivedClipVolumeDestroy = false;

    private:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_clipVolumeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_polygonPrismComponentDescriptor;
    };

    TEST_F(ClipVolumeComponentTests, CreateClipVolumeComponent_FT)
    {
        AZ::Entity entity;
        CreateDefaultSetup(entity);
        LmbrCentral::ClipVolumeComponent* clipVolumeComponent = entity.FindComponent<LmbrCentral::ClipVolumeComponent>();
        EXPECT_TRUE(clipVolumeComponent != nullptr);
    }

    TEST_F(ClipVolumeComponentTests, ComponentReceivesClipVolume_FT)
    {
        AZ::Entity entity;
        ConnectBus(entity.GetId());
        EXPECT_FALSE(m_clipVolume == m_expectedClipVolumePtr);
        CreateDefaultSetup(entity);
        EXPECT_TRUE(m_clipVolume == m_expectedClipVolumePtr);
        EXPECT_FALSE(m_receivedClipVolumeDestroy);
        entity.Deactivate();
        EXPECT_TRUE(m_receivedClipVolumeDestroy);
    }
}