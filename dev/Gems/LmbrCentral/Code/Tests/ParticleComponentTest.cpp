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
#include "Mocks/ISystemMock.h"

#include <AzFramework/Components/TransformComponent.h>
#include "Rendering/ParticleComponent.h"

class ParticleComponentTests
    : public UnitTest::AllocatorsTestFixture
{
public:
    ParticleComponentTests()
        : AllocatorsTestFixture()
    {}

protected:
    void SetUp() override
    {
        AllocatorsTestFixture::SetUp();

        m_priorEnv = gEnv;
        m_data.reset(new DataMembers);

        memset(&m_data->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
        m_data->m_stubEnv.pSystem = &m_data->m_system;

        gEnv = &m_data->m_stubEnv;

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_particleComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::ParticleComponent::CreateDescriptor());

        m_transformComponentDescriptor->Reflect(m_serializeContext.get());
        m_particleComponentDescriptor->Reflect(m_serializeContext.get());
    }

    void TearDown() override
    {
        m_particleComponentDescriptor.reset();
        m_transformComponentDescriptor.reset();

        m_serializeContext.reset();

        gEnv = m_priorEnv;
        m_data.reset();

        AllocatorsTestFixture::TearDown();
    }

    void CreateDefaultSetup(AZ::Entity& entity, bool autoActivate)
    {
        entity.CreateComponent<AzFramework::TransformComponent>();

        entity.CreateComponent<LmbrCentral::ParticleComponent>();

        entity.Init();
        entity.Activate();
    }

    struct DataMembers
    {
        ::testing::NiceMock<SystemMock> m_system;
        SSystemGlobalEnvironment m_stubEnv;
    };

    AZStd::unique_ptr<DataMembers> m_data;
    SSystemGlobalEnvironment* m_priorEnv = nullptr;

private:
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_particleComponentDescriptor;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
};

TEST_F(ParticleComponentTests, Visibility_ValidateSettings_Success_FT)
{
    AZ::Entity entity;
    CreateDefaultSetup(entity, false);
    LmbrCentral::ParticleComponent* particleComponent = entity.FindComponent<LmbrCentral::ParticleComponent>();
    EXPECT_TRUE(particleComponent != nullptr);
    particleComponent->Show();
    EXPECT_TRUE(particleComponent->GetVisibility());
    particleComponent->Hide();
    EXPECT_FALSE(particleComponent->GetVisibility());
    particleComponent->SetVisibility(true);
    EXPECT_TRUE(particleComponent->GetVisibility());
    particleComponent->SetVisibility(false);
    EXPECT_FALSE(particleComponent->GetVisibility());
}
