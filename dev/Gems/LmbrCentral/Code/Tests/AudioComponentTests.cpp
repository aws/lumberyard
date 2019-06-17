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

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Audio/AudioMultiPositionComponent.h>
#include <Audio/AudioProxyComponent.h>
#include <Audio/AudioTriggerComponent.h>


using namespace AZ;
using namespace AzFramework;
using namespace LmbrCentral;

namespace UnitTest
{
    class AudioMultiPositionComponentTests
        : public AllocatorsTestFixture
    {
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_audioProxyComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_audioTriggerComponentDescriptor;
        AZStd::unique_ptr<ComponentDescriptor> m_audioMultiPosComponentDescriptor;

    public:
        AudioMultiPositionComponentTests()
            : AllocatorsTestFixture()
        {}

    protected:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();
            m_serializeContext = AZStd::make_unique<SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(TransformComponent::CreateDescriptor());
            m_audioProxyComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(AudioProxyComponent::CreateDescriptor());
            m_audioTriggerComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(AudioTriggerComponent::CreateDescriptor());
            m_audioMultiPosComponentDescriptor = AZStd::unique_ptr<ComponentDescriptor>(AudioMultiPositionComponent::CreateDescriptor());

            m_transformComponentDescriptor->Reflect(m_serializeContext.get());
            m_audioProxyComponentDescriptor->Reflect(m_serializeContext.get());
            m_audioTriggerComponentDescriptor->Reflect(m_serializeContext.get());
            m_audioMultiPosComponentDescriptor->Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            m_audioMultiPosComponentDescriptor.reset();
            m_audioTriggerComponentDescriptor.reset();
            m_audioMultiPosComponentDescriptor.reset();
            m_audioProxyComponentDescriptor.reset();
            m_transformComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsTestFixture::TearDown();
        }

        void CreateDefaultSetup(Entity& entity)
        {
            entity.CreateComponent<TransformComponent>();
            entity.CreateComponent<AudioProxyComponent>();
            entity.CreateComponent<AudioTriggerComponent>();
            entity.CreateComponent<AudioMultiPositionComponent>();

            entity.Init();
            entity.Activate();
        }
    };

    TEST_F(AudioMultiPositionComponentTests, MultiPositionComponent_ComponentExists)
    {
        Entity entity;
        CreateDefaultSetup(entity);

        AudioMultiPositionComponent* multiPosComponent = entity.FindComponent<AudioMultiPositionComponent>();
        EXPECT_TRUE(multiPosComponent != nullptr);
    }

    TEST_F(AudioMultiPositionComponentTests, MultiPositionComponent_AddAndRemoveEntity)
    {
        Entity entity;
        CreateDefaultSetup(entity);

        AudioMultiPositionComponent* multiPosComponent = entity.FindComponent<AudioMultiPositionComponent>();
        ASSERT_TRUE(multiPosComponent != nullptr);
        if (multiPosComponent)
        {
            Entity entity1;
            Entity entity2;

            EXPECT_EQ(multiPosComponent->GetNumEntityRefs(), 0);
            AudioMultiPositionComponentRequestBus::Event(entity.GetId(), &AudioMultiPositionComponentRequestBus::Events::AddEntity, entity1.GetId());
            EXPECT_EQ(multiPosComponent->GetNumEntityRefs(), 1);

            // Remove non-Added
            AudioMultiPositionComponentRequestBus::Event(entity.GetId(), &AudioMultiPositionComponentRequestBus::Events::RemoveEntity, entity2.GetId());
            EXPECT_EQ(multiPosComponent->GetNumEntityRefs(), 1);

            // Remove Added
            AudioMultiPositionComponentRequestBus::Event(entity.GetId(), &AudioMultiPositionComponentRequestBus::Events::RemoveEntity, entity1.GetId());
            EXPECT_EQ(multiPosComponent->GetNumEntityRefs(), 0);
        }
    }

    TEST_F(AudioMultiPositionComponentTests, MultiPositionComponent_EntityActivatedObtainsPosition)
    {
        Entity entity;
        CreateDefaultSetup(entity);

        AudioMultiPositionComponent* multiPosComponent = entity.FindComponent<AudioMultiPositionComponent>();
        ASSERT_TRUE(multiPosComponent != nullptr);
        if (multiPosComponent)
        {
            Entity entity1;
            entity1.Init();
            entity1.Activate();

            AudioMultiPositionComponentRequestBus::Event(entity.GetId(), &AudioMultiPositionComponentRequestBus::Events::AddEntity, entity1.GetId());

            // Send OnEntityActivated
            AZ::EntityBus::Event(entity1.GetId(), &AZ::EntityBus::Events::OnEntityActivated, entity1.GetId());

            EXPECT_EQ(multiPosComponent->GetNumEntityPositions(), 1);
        }
    }

} // namespace UnitTest
