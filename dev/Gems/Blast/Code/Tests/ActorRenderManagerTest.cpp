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
#include <gtest/gtest_prod.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzTest/AzTest.h>
#include <Blast/BlastSystemBus.h>
#include <Family/ActorRenderManager.h>
#include <Tests/Mocks/BlastMocks.h>

using testing::_;
using testing::Eq;
using testing::NaggyMock;
using testing::Return;
using testing::StrictMock;

namespace Blast
{
    // Helper class to make it possible to make test a friend without polluting the tested class itself
    class TestableActorRenderManager : public ActorRenderManager
    {
    public:
        TestableActorRenderManager(
            const AZStd::shared_ptr<EntityProvider>& entityProvider, BlastMeshData* meshData, const uint32_t chunkCount)
            : ActorRenderManager(entityProvider, meshData, chunkCount, AZ::Vector3::CreateOne())
        {
        }

        FRIEND_TEST(ActorRenderManagerTest, KeepsTrackOfAndRendersChunks);
    };

    class ActorRenderManagerTest
        : public testing::Test
        , public FastScopedAllocatorsBase
    {
    protected:
        void SetUp() override
        {
            m_chunkCount = 2;
            m_mockMeshData = AZStd::make_shared<MockBlastMeshData>();
            m_actorFactory = AZStd::make_unique<FakeActorFactory>(1);
            m_fakeEntityProvider = AZStd::make_shared<FakeEntityProvider>(m_chunkCount);

            m_actorFactory->m_mockActors[0]->m_chunkIndices = {0, 1};
        }

        AZStd::shared_ptr<MockBlastMeshData> m_mockMeshData;
        AZStd::unique_ptr<FakeActorFactory> m_actorFactory;
        AZStd::shared_ptr<FakeEntityProvider> m_fakeEntityProvider;

        uint32_t m_chunkCount;
    };

    TEST_F(ActorRenderManagerTest, KeepsTrackOfAndRendersChunks)
    {
        MockMeshComponentRequestBusHandler meshHandler;
        MockTransformBusHandler transformHandler;

        for (const auto id : m_fakeEntityProvider->m_createdEntityIds)
        {
            meshHandler.Connect(id);
            transformHandler.Connect(id);
        }
        AZStd::unique_ptr<TestableActorRenderManager> actorRenderManager;

        // ActorRenderManager::ActorRenderManager
        {
            AZ::Data::Asset<LmbrCentral::MeshAsset> asset{AZ::Data::AssetLoadBehavior::NoLoad};
            EXPECT_CALL(*m_mockMeshData, GetMeshAsset(_)).Times(m_chunkCount).WillRepeatedly(testing::ReturnRef(asset));
            EXPECT_CALL(meshHandler, SetVisibility(_)).Times(m_chunkCount);
            EXPECT_CALL(meshHandler, SetMeshAsset(_)).Times(m_chunkCount);

            actorRenderManager =
                AZStd::make_unique<TestableActorRenderManager>(m_fakeEntityProvider, m_mockMeshData.get(), m_chunkCount);
            EXPECT_EQ(actorRenderManager->m_chunkEntities.size(), m_chunkCount);
        }

        // ActorRenderManager::OnActorCreated
        {
            actorRenderManager->OnActorCreated(*m_actorFactory->m_mockActors[0]);
            for (auto chunkId : m_actorFactory->m_mockActors[0]->m_chunkIndices)
            {
                EXPECT_EQ(
                    actorRenderManager->m_chunkActors[chunkId],
                    static_cast<BlastActor*>(m_actorFactory->m_mockActors[0]));
            }
        }

        // ActorRenderManager::SyncMeshes
        {
            EXPECT_CALL(transformHandler, SetWorldTM(_)).Times(m_chunkCount);
            EXPECT_CALL(meshHandler, SetVisibility(_)).Times(m_chunkCount);
            actorRenderManager->SyncMeshes();
        }

        // ActorRenderManager::OnActorDestroyed
        {
            actorRenderManager->OnActorDestroyed(*m_actorFactory->m_mockActors[0]);
            for (auto chunkId : m_actorFactory->m_mockActors[0]->m_chunkIndices)
            {
                EXPECT_EQ(actorRenderManager->m_chunkActors[chunkId], nullptr);
            }
        }
    }
} // namespace Blast
