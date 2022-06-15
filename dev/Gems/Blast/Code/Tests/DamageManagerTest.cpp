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

#include <StdAfx.h>

#include <gmock/gmock.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzTest/AzTest.h>

#include <Blast/BlastSystemBus.h>
#include <Family/DamageManager.h>
#include <Tests/Mocks/BlastMocks.h>

using testing::_;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;

namespace Blast
{
    namespace Constants
    {
        static const float DamageAmount = 1.0;
        static const float MinRadius = 0.0;
        static const float MaxRadius = 5.0;
    }

    class DamageManagerTest
        : public testing::Test
        , public FastScopedAllocatorsBase
    {
    public:
    protected:
        void SetUp() override
        {
            m_mockFamily = AZStd::make_shared<FakeBlastFamily>();
            m_actorFactory = AZStd::make_shared<FakeActorFactory>(3);
            m_systemHandler = AZStd::make_shared<MockBlastSystemBusHandler>();
            m_damageManager =
                AZStd::make_unique<DamageManager>(BlastMaterial(BlastMaterialConfiguration()), m_actorTracker);
        }

        void TearDown() override
        {
            m_mockFamily = nullptr;
            m_actorFactory = nullptr;
            m_systemHandler = nullptr;
            m_damageManager = nullptr;
        }

        AZStd::unique_ptr<DamageManager> m_damageManager;
        AZStd::shared_ptr<FakeBlastFamily> m_mockFamily;
        AZStd::shared_ptr<FakeActorFactory> m_actorFactory;
        AZStd::shared_ptr<MockBlastSystemBusHandler> m_systemHandler;
        ActorTracker m_actorTracker;
    };

    TEST_F(DamageManagerTest, RadialDamage)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(*m_systemHandler, AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtRadialDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::RadialDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount, AZ::Vector3{0, 0, 0}, Constants::MinRadius, Constants::MaxRadius);
    }

    TEST_F(DamageManagerTest, CapsuleDamage)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(
                *m_systemHandler, AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtCapsuleRadialDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::CapsuleDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount,
            AZ::Vector3{0, 0, 0}, AZ::Vector3{1, 0, 0}, Constants::MinRadius, Constants::MaxRadius);
    }

    TEST_F(DamageManagerTest, ShearDamage)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(*m_systemHandler, AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtShearDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::ShearDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount,
            AZ::Vector3{0, 0, 0}, Constants::MinRadius, Constants::MaxRadius, AZ ::Vector3{1, 0, 0});
    }

    TEST_F(DamageManagerTest, TriangleDamage)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(
                *m_systemHandler,
                AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtTriangleIntersectionDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::TriangleDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount,
            AZ::Vector3{0, 0, 0}, AZ::Vector3{1, 0, 0}, AZ::Vector3{0, 1, 0});
    }

    TEST_F(DamageManagerTest, ImpactSpreadDamage)
    {
        {
            testing::InSequence dummy;

            EXPECT_CALL(*m_actorFactory->m_mockActors[0], GetFamily()).Times(1).WillOnce(ReturnRef(*m_mockFamily));
            EXPECT_CALL(m_mockFamily->m_pxAsset, getAccelerator()).Times(1).WillOnce(Return(nullptr));
            EXPECT_CALL(*m_actorFactory->m_mockActors[0], Damage(_, _)).Times(1);
            EXPECT_CALL(
                *m_systemHandler, AddDamageDesc(testing::An<AZStd::unique_ptr<NvBlastExtImpactSpreadDamageDesc>>()))
                .Times(1);
            EXPECT_CALL(*m_systemHandler, AddProgramParams(_)).Times(1);
        }

        m_damageManager->Damage(
            DamageManager::ImpactSpreadDamage{}, *m_actorFactory->m_mockActors[0], Constants::DamageAmount,
            AZ::Vector3{0, 0, 0}, Constants::MinRadius, Constants::MaxRadius);
    }
} // namespace Blast
