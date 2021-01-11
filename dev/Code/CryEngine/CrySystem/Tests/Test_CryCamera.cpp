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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <ViewSystem/View.h>
#include "Mocks/ISystemMock.h"
#include "Mocks/IViewSystemMock.h"
#include <Mocks/IConsoleMock.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Matrix4x4.h>


namespace Test_CryCamera
{
    using namespace testing;
    using ::testing::NiceMock;

    using SystemAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator>;

    class CameraViewTests
        : public ::testing::Test
        , public SystemAllocatorScope
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;

    public:
        void SetUp() override
        {
            SystemAllocatorScope::ActivateAllocators();

            m_priorEnv = gEnv;
            m_data.reset(new DataMembers);
            
            memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_data->m_stubEnv.pSystem = &m_data->m_system;
            m_data->m_stubEnv.pConsole = &m_data->m_console;

            gEnv = &m_data->m_stubEnv;

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        }

        void TearDown() override
        {
            m_serializeContext.reset();

            gEnv = m_priorEnv;
            m_data.reset();

            SystemAllocatorScope::DeactivateAllocators();
        }

        struct DataMembers
        {
            ::testing::NiceMock<SystemMock> m_system;
            ::testing::NiceMock<ConsoleMock> m_console;
            SSystemGlobalEnvironment m_stubEnv;
        };

        AZStd::unique_ptr<DataMembers> m_data;
        SSystemGlobalEnvironment* m_priorEnv = nullptr;
    };
    
    TEST_F(CameraViewTests, WorldScreenProjection_FT)
    {
        //set default camera at origin looking down at +y
        LegacyViewSystem::CView cView(&m_data->m_system);
        CCamera camera = cView.GetCamera();
        constexpr float ScreenWidth = 1920.f;
        constexpr float ScreenHeight = 1080.f;
        camera.SetFrustum(aznumeric_caster(ScreenWidth), aznumeric_caster(ScreenHeight), AZ::DegToRad(30.0f), 1.f, 1000.f, 16.0f / 9.0f);
        AZ::Vector4 viewport(0.f, 0.f, ScreenWidth, ScreenHeight);

        //get projected screen coordinates right in front of the camera
        AZ::Vector3 worldPos(0.f, 5.f, 0.f);
        AZ::Vector3 projectedScreenPos(ScreenWidth / 2, ScreenHeight / 2, 0.f);
        bool inView = cView.ProjectWorldPointToViewport(worldPos, viewport, projectedScreenPos);
        EXPECT_TRUE(inView);
        EXPECT_NEAR(projectedScreenPos.GetX(), ScreenWidth / 2, 1e-3f);
        EXPECT_NEAR(projectedScreenPos.GetY(), ScreenHeight /2, 1e-3f);

        //attempt to get projected screen coordinates outside of the view frustum
        worldPos = AZ::Vector3(0.f, -100.f, 0.f);
        bool notInView = cView.ProjectWorldPointToViewport(worldPos, viewport, projectedScreenPos);
        EXPECT_FALSE(notInView);

        //get projected world position of screen center
        AZ::Vector3 screenPos(ScreenWidth * 0.5f, ScreenHeight * 0.5f, 0.f);
        AZ::Vector3 projectedWorldPos(0.f, 0.f, 0.f);
        inView = cView.UnprojectViewportPointToWorld(screenPos, viewport, projectedWorldPos);
        EXPECT_TRUE(inView);
        EXPECT_NEAR(projectedWorldPos.GetX(), 0.f, 1e-3f);
        EXPECT_NEAR(projectedWorldPos.GetY(), 0.2f, 1e-3f);
        EXPECT_NEAR(projectedWorldPos.GetZ(), 0.f, 1e-3f);
    }

    TEST_F(CameraViewTests, ProjectionMatrix_FT)
    {
        LegacyViewSystem::CView cView(&m_data->m_system);
        CCamera camera = cView.GetCamera();
        constexpr float NearPlane = 1.f;
        constexpr float FarPlane = 1000.f;
        camera.SetFrustum(1920, 1080, AZ::DegToRad(30.0f), NearPlane, FarPlane, 16.0f / 9.0f);
        
        //validate vector values in matrix
        AZ::Matrix4x4 m1 = AZ::Matrix4x4::CreateIdentity();
        cView.GetProjectionMatrix(NearPlane, FarPlane, m1);
        EXPECT_TRUE(m1.GetRow(0).IsClose(AZ::Vector4(0.977f, 0.0f, 0.0f, 0.0f)));
        EXPECT_TRUE(m1.GetRow(1).IsClose(AZ::Vector4(0.0f, 1.303f, 0.0f, 0.0f)));
        EXPECT_TRUE(m1.GetRow(2).IsClose(AZ::Vector4(0.0f, 0.0f, -1.001f, -1.001f)));
        EXPECT_TRUE(m1.GetRow(3).IsClose(AZ::Vector4(0.0f, 0.0f, -1.0f, 0.0f)));
    }

}
