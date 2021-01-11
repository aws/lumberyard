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
#include "Camera_precompiled.h"

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include "Mocks/ISystemMock.h"
#include "Mocks/IViewSystemMock.h"

#include <AzFramework/Components/TransformComponent.h>
#include "CameraComponent.h"

class CameraComponentTests
    : public UnitTest::AllocatorsTestFixture
{
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_cameraComponentDescriptor;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;

public:
    CameraComponentTests()
        : AllocatorsTestFixture()
    {}

protected:
    void SetUp() override
    {
        AllocatorsTestFixture::SetUp();

        m_priorEnv = gEnv;
        m_data.reset(new DataMembers);

        ON_CALL(m_data->m_system, GetIViewSystem())
            .WillByDefault(::testing::Return(&m_data->m_viewSystem));

        memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
        m_data->m_stubEnv.pSystem = &m_data->m_system;

        gEnv = &m_data->m_stubEnv;

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_cameraComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(Camera::CameraComponent::CreateDescriptor());

        m_transformComponentDescriptor->Reflect(m_serializeContext.get());
        m_cameraComponentDescriptor->Reflect(m_serializeContext.get());
    }

    void TearDown() override
    {
        m_cameraComponentDescriptor.reset();
        m_transformComponentDescriptor.reset();

        m_serializeContext.reset();

        gEnv = m_priorEnv;
        m_data.reset();

        AllocatorsTestFixture::TearDown();
    }

    void CreateDefaultSetup(AZ::Entity& entity, bool autoActivate)
    {
        entity.CreateComponent<AzFramework::TransformComponent>();

        Camera::CameraProperties cameraProperties;
        cameraProperties.m_fov = Camera::s_defaultFoV;
        cameraProperties.m_nearClipDistance = Camera::s_defaultNearPlaneDistance;
        cameraProperties.m_farClipDistance = Camera::s_defaultFarClipPlaneDistance;
        cameraProperties.m_specifyFrustumDimensions = false;
        cameraProperties.m_frustumWidth = Camera::s_defaultFrustumDimension;
        cameraProperties.m_frustumHeight = Camera::s_defaultFrustumDimension;
        cameraProperties.m_autoActivate = autoActivate;

        entity.CreateComponent<Camera::CameraComponent>(cameraProperties);

        entity.Init();
        entity.Activate();
    }

    struct DataMembers
    {
        ::testing::NiceMock<SystemMock> m_system;
        ::testing::NiceMock<ViewSystemMock> m_viewSystem;
        SSystemGlobalEnvironment m_stubEnv;
    };

    AZStd::unique_ptr<DataMembers> m_data;
    SSystemGlobalEnvironment* m_priorEnv = nullptr;

};

TEST_F(CameraComponentTests, SetFOVDegrees_Validate_Clamped_FT)
{
    AZ::Entity entity;
    CreateDefaultSetup(entity, false);
    Camera::CameraComponent* cameraComponent = entity.FindComponent<Camera::CameraComponent>();
    EXPECT_TRUE(cameraComponent != nullptr);
    cameraComponent->SetFovDegrees(-5.f);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_minFoV);
    cameraComponent->SetFovDegrees(0.f);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_minFoV);
    cameraComponent->SetFovDegrees(Camera::s_minFoV);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_minFoV);
    cameraComponent->SetFovDegrees(75.f);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), 75.f);
    cameraComponent->SetFovDegrees(180.f);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_maxFoV);
    cameraComponent->SetFovDegrees(200.f);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_maxFoV);
}

TEST_F(CameraComponentTests, SetFOVRadians_Validate_Clamped_FT)
{
    AZ::Entity entity;
    CreateDefaultSetup(entity, false);
    Camera::CameraComponent* cameraComponent = entity.FindComponent<Camera::CameraComponent>();
    EXPECT_TRUE(cameraComponent != nullptr);
    cameraComponent->SetFovRadians(-5.f);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_minFoV);
    cameraComponent->SetFovRadians(0.f);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_minFoV);
    cameraComponent->SetFovRadians(1.f);
    EXPECT_FLOAT_EQ(cameraComponent->GetFovRadians(), 1.f);
    cameraComponent->SetFovRadians(AZ::Constants::Pi);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_maxFoV);
    cameraComponent->SetFovRadians(10.f);
    EXPECT_EQ(cameraComponent->GetFovDegrees(), Camera::s_maxFoV);
}

TEST_F(CameraComponentTests, AutoActivate_False_NotActiveView_FT)
{
    AZ::Entity entity;
    CreateDefaultSetup(entity, false);
    Camera::CameraComponent* cameraComponent = entity.FindComponent<Camera::CameraComponent>();
    EXPECT_TRUE(cameraComponent != nullptr);
    EXPECT_EQ(m_data->m_viewSystem.GetActiveView(), nullptr);
    cameraComponent->SetActiveView(true);
    EXPECT_NE(m_data->m_viewSystem.GetActiveView(), nullptr);
}

TEST_F(CameraComponentTests, AutoActivate_True_NotActiveView_FT)
{
    AZ::Entity entity;
    CreateDefaultSetup(entity, true);
    Camera::CameraComponent* cameraComponent = entity.FindComponent<Camera::CameraComponent>();
    EXPECT_TRUE(cameraComponent != nullptr);
    EXPECT_NE(m_data->m_viewSystem.GetActiveView(), nullptr);
}

TEST_F(CameraComponentTests, ProjectionBuses_FT)
{
    AZ::Entity entity;
    CreateDefaultSetup(entity, true);
    Camera::CameraComponent* cameraComponent = entity.FindComponent<Camera::CameraComponent>();
    cameraComponent->SetActiveView(true);

    AZ::Vector3 worldPos = AZ::Vector3::CreateZero();
    AZ::Vector3 screenPos = AZ::Vector3::CreateZero();

    //All these bus calls are hitting the mock functions in ViewMock, so we do not expect actual projection math output
    bool result = false;
    Camera::CameraRequestBus::EventResult(result, cameraComponent->GetEntityId(), &Camera::CameraComponentRequests::ProjectWorldPointToScreen, worldPos, screenPos);
    EXPECT_TRUE(result); 

    result = false;
    Camera::CameraRequestBus::EventResult(result, cameraComponent->GetEntityId(), &Camera::CameraComponentRequests::UnprojectScreenPointToWorld, screenPos, worldPos);
    EXPECT_TRUE(result);

    AZ::Vector4 viewport = AZ::Vector4::CreateZero();
    result = false;
    Camera::CameraRequestBus::EventResult(result, cameraComponent->GetEntityId(), &Camera::CameraComponentRequests::ProjectWorldPointToViewport, worldPos, viewport, screenPos);
    EXPECT_TRUE(result);

    result = false;
    Camera::CameraRequestBus::EventResult(result, cameraComponent->GetEntityId(), &Camera::CameraComponentRequests::UnprojectViewportPointToWorld, screenPos, viewport, worldPos);
    EXPECT_TRUE(result);

    AZ::Matrix4x4 projMat = AZ::Matrix4x4::CreateZero();
    Camera::CameraRequestBus::Event(cameraComponent->GetEntityId(), &Camera::CameraComponentRequests::GetProjectionMatrix, projMat);
    EXPECT_EQ(projMat, AZ::Matrix4x4::CreateIdentity());
}

TEST_F(CameraComponentTests, SetActiveView_BackToPrevious_Success_FT)
{
    AZ::Entity entity1;
    CreateDefaultSetup(entity1, false);
    Camera::CameraComponent* cameraComponent1 = entity1.FindComponent<Camera::CameraComponent>();
    EXPECT_TRUE(cameraComponent1 != nullptr);
    EXPECT_EQ(m_data->m_viewSystem.GetActiveView(), nullptr);
    cameraComponent1->SetActiveView(true);
    EXPECT_NE(m_data->m_viewSystem.GetActiveView(), nullptr);
    EXPECT_TRUE(cameraComponent1->IsActiveView());

    // Set second camera to give it a prevViewId
    AZ::Entity entity2;
    CreateDefaultSetup(entity2, false);
    Camera::CameraComponent* cameraComponent2 = entity2.FindComponent<Camera::CameraComponent>();
    EXPECT_TRUE(cameraComponent2 != nullptr);
    cameraComponent2->SetActiveView(true);
    EXPECT_NE(m_data->m_viewSystem.GetActiveView(), nullptr);
    EXPECT_FALSE(cameraComponent1->IsActiveView());
    EXPECT_TRUE(cameraComponent2->IsActiveView());

    // Set third camera to give it a prevViewId
    AZ::Entity entity3;
    CreateDefaultSetup(entity3, false);
    Camera::CameraComponent* cameraComponent3 = entity3.FindComponent<Camera::CameraComponent>();
    EXPECT_TRUE(cameraComponent3 != nullptr);
    cameraComponent3->SetActiveView(true);
    EXPECT_NE(m_data->m_viewSystem.GetActiveView(), nullptr);
    EXPECT_FALSE(cameraComponent1->IsActiveView());
    EXPECT_FALSE(cameraComponent2->IsActiveView());
    EXPECT_TRUE(cameraComponent3->IsActiveView());

    // Set inactive camera (#2) with previousViewId to false
    cameraComponent2->SetActiveView(false);
    EXPECT_FALSE(cameraComponent1->IsActiveView());
    EXPECT_FALSE(cameraComponent2->IsActiveView());
    EXPECT_TRUE(cameraComponent3->IsActiveView());

    // Set active view back to previous (cameraComponent2)
    cameraComponent3->SetActiveView(false);
    EXPECT_FALSE(cameraComponent1->IsActiveView());
    EXPECT_TRUE(cameraComponent2->IsActiveView());
    EXPECT_FALSE(cameraComponent3->IsActiveView());
}

TEST_F(CameraComponentTests, ExampleTest)
{
    ASSERT_TRUE(true);
}


AZ_UNIT_TEST_HOOK();