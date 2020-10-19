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

#include "StdAfx.h"
#include <AzTest/AzTest.h>
#include <AzFramework/Components/TransformComponent.h>
#include <DebugDrawSystemComponent.h>
#include <Mocks/IRenderAuxGeomMock.h>
#include <Mocks/IRendererMock.h>
#include <AzFramework/Application/Application.h>
#include <AzCore/Component/Entity.h>

namespace UnitTest
{
    class DebugDrawTestApplication
        : public AzFramework::Application
    {
        // override and only include system components required for tests.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<DebugDraw::DebugDrawSystemComponent>()
            };
        }

        void RegisterCoreComponents() override
        {
            AzFramework::Application::RegisterCoreComponents();
            RegisterComponentDescriptor(DebugDraw::DebugDrawSystemComponent::CreateDescriptor());
        }
    };

    class DebugDrawTest
        : public testing::Test
    {
    protected:
        struct DataMembers
        {
            SSystemGlobalEnvironment m_stubEnv;
            testing::NiceMock<IRendererMock> m_renderer;
            testing::NiceMock<IRenderAuxGeomMock> m_renderAuxGeom;
        };

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(AZ::SystemAllocator::Descriptor());

            AZ::ComponentApplication::Descriptor appDescriptor;
            appDescriptor.m_useExistingAllocator = true;

            m_application = aznew DebugDrawTestApplication();
            m_application->Start(appDescriptor, AZ::ComponentApplication::StartupParameters());

            // Setup gEnv
            m_data = AZStd::make_unique<DataMembers>();
            memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_data->m_stubEnv.pRenderer = &m_data->m_renderer;
            gEnv = &m_data->m_stubEnv;
        }

        void TearDown() override
        {
            m_application->Stop();
            delete m_application;
            m_application = nullptr;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

        AZStd::unique_ptr<DataMembers> m_data;
        DebugDrawTestApplication* m_application = nullptr;
    };

    MATCHER(IsUsingAlphaBlend, "")
    {
        *result_listener << "Where the SAuxGeomRenderFlags = " << (arg.m_renderFlags);
        return arg.GetAlphaBlendMode() == e_AlphaBlended;
    }

    TEST_F(DebugDrawTest, DebugDraw_EnablesAlphaBlending_FT)
    {
        // Route calls from gEnv->Renderer->GetIRenderAuxGeom to our MockRenderAuxGeom
        ON_CALL(m_data->m_renderer, GetIRenderAuxGeom(nullptr)).WillByDefault(testing::Return(&m_data->m_renderAuxGeom));

        // SetRenderFlags may be called any amount of times, but at least once it should be enabling alpha blending
        EXPECT_CALL(m_data->m_renderAuxGeom, SetRenderFlags).Times(testing::AnyNumber());
        EXPECT_CALL(m_data->m_renderAuxGeom, SetRenderFlags(IsUsingAlphaBlend())).Times(testing::AtLeast(1));

        // Ask the debug draw system component to render a sphere (rendering won't happen until OnTick())
        AZ::Entity entity;
        DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequestBus::Events::DrawSphereOnEntity, entity.GetId(), 0.5f, AZ::Colors::White, 0.f);
        
        // Debug draw system component draws everything on tick
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.1f, AZ::ScriptTimePoint());
    }
} //namespace UnitTest

AZ_UNIT_TEST_HOOK()
