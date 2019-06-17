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
#include "Vegetation_precompiled.h"

#include "VegetationTest.h"
#include "VegetationMocks.h"

#include <AzTest/AzTest.h>
#include <AzCore/Component/Entity.h>

#include <Source/Components/AreaBlenderComponent.h>
#include <Source/Components/BlockerComponent.h>
#include <Source/Components/MeshBlockerComponent.h>
#include <Source/Components/SpawnerComponent.h>
#include <Source/Components/StaticVegetationBlockerComponent.h>

#include <AzCore/Component/TickBus.h>

namespace UnitTest
{
    struct MockDescriptorProvider 
        : public Vegetation::DescriptorProviderRequestBus::Handler
    {
        AZStd::vector<Vegetation::DescriptorPtr> m_descriptors;
        MockMeshAsset mockMeshAssetData;

        MockDescriptorProvider(size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                m_descriptors.emplace_back(CreateDescriptor());
            }
        }

        Vegetation::DescriptorPtr CreateDescriptor()
        {
            Vegetation::Descriptor descriptor;
            descriptor.m_autoMerge = true;
            descriptor.m_meshAsset = AZ::Data::Asset<MockMeshAsset>(&mockMeshAssetData);
            descriptor.m_meshLoaded = true;

            return AZStd::make_shared<Vegetation::Descriptor>(descriptor);
        }

        void Clear()
        {
            for (auto& descriptor : m_descriptors)
            {
                descriptor->m_meshAsset = nullptr;
            }
            m_descriptors.clear();
        }
       
        void GetDescriptors(Vegetation::DescriptorPtrVec& descriptors) const override
        {
            descriptors = m_descriptors;
        }
    };

    struct VegetationComponentOperationTests
        : public VegetationComponentTests
    {
        void RegisterComponentDescriptors() override
        {
            m_app.RegisterComponentDescriptor(MockShapeServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockVegetationAreaServiceComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(MockMeshServiceComponent::CreateDescriptor());
        }

        void BasicAreaTests(const AZ::EntityId& areaId)
        {
            float priority = -1.0f;
            Vegetation::AreaInfoBus::EventResult(priority, areaId, &Vegetation::AreaInfoBus::Events::GetPriority);

            EXPECT_NE(-1.0f, priority);

            auto aabb =  AZ::Aabb::CreateNull();
            Vegetation::AreaInfoBus::EventResult(aabb, areaId, &Vegetation::AreaInfoBus::Events::GetEncompassingAabb);

            EXPECT_TRUE(aabb.IsValid());

            // with area not connected
            {
                Vegetation::AreaNotificationBus::Event(areaId, &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);

                size_t numHandlers = Vegetation::AreaRequestBus::GetNumOfEventHandlers(areaId);

                EXPECT_EQ(numHandlers, 0);
            }

            // now with area connected
            {
                Vegetation::AreaNotificationBus::Event(areaId, &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

                size_t numHandlers = Vegetation::AreaRequestBus::GetNumOfEventHandlers(areaId);

                EXPECT_EQ(numHandlers, 1);
            }
        }

        void ConnectToAreaBuses(AZ::Entity& entity)
        {
            if (!m_connected)
            {
                entity.Deactivate();
                m_mockMeshRequestBus.BusConnect(entity.GetId());
                m_mockTransformBus.BusConnect(entity.GetId());
                m_mockShapeBus.BusConnect(entity.GetId());
                entity.Activate();

                BasicAreaTests(entity.GetId());
                m_connected = true;
            }

        }

        void ReleaseFromAreaBuses()
        {
            if (m_connected)
            {
                m_mockMeshRequestBus.m_GetMeshAssetOutput = nullptr;
                m_mockMeshRequestBus.BusDisconnect();
                m_mockTransformBus.BusDisconnect();
                m_mockShapeBus.BusDisconnect();
                m_connected = false;
            }
        }

        AZ::Data::Asset<MockMeshAsset> CreateMockMeshAsset()
        {
            AZ::Data::Asset<MockMeshAsset> mockAsset(&m_mockMeshAssetData);
            return mockAsset;
        }

        void DestroyMockMeshAsset(AZ::Data::Asset<MockMeshAsset>& mockAsset)
        {
            mockAsset.Release();
        }

        AZStd::unique_ptr<AZ::Entity> CreateMockMeshEntity(const AZ::Data::Asset<MockMeshAsset>& mockAsset, AZ::Vector3 position, 
                                                           AZ::Vector3 aabbMin, AZ::Vector3 aabbMax, float meshPercentMin, float meshPercentMax)
        {
            m_mockMeshRequestBus.m_GetMeshAssetOutput = mockAsset;
            m_mockMeshRequestBus.m_GetWorldBoundsOutput = AZ::Aabb::CreateFromMinMax(aabbMin, aabbMax);
            m_mockMeshRequestBus.m_GetVisibilityOutput = true;

            m_mockTransformBus.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(position);

            Vegetation::MeshBlockerConfig config;
            config.m_blockWhenInvisible = true;
            config.m_priority = 2;
            config.m_meshHeightPercentMin = meshPercentMin;
            config.m_meshHeightPercentMax = meshPercentMax;

            Vegetation::MeshBlockerComponent* component = nullptr;
            auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
            {
                e->CreateComponent<MockMeshServiceComponent>();
            });

            return entity;
        }

        void TestMeshBlockerPoint(AZStd::unique_ptr<AZ::Entity>& meshBlockerEntity, AZ::Vector3 testPoint, int numPointsBlocked)
        {
            AreaBusScope scope(*this, *meshBlockerEntity.get());

            Vegetation::AreaNotificationBus::Event(meshBlockerEntity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

            bool prepared = false;
            Vegetation::EntityIdStack idStack;
            Vegetation::AreaRequestBus::EventResult(prepared, meshBlockerEntity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
            EXPECT_TRUE(prepared);

            Vegetation::ClaimContext context = CreateContext<1>({ testPoint });
            Vegetation::AreaRequestBus::Event(meshBlockerEntity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);
            EXPECT_EQ(numPointsBlocked, m_createdCallbackCount);

            Vegetation::AreaNotificationBus::Event(meshBlockerEntity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);
        }

        struct AreaBusScope
        {
            VegetationComponentOperationTests& m_block;

            AreaBusScope(VegetationComponentOperationTests& block, AZ::Entity& entity) 
                : m_block(block)
            {
                m_block.ConnectToAreaBuses(entity);
            }

            ~AreaBusScope()
            {
                m_block.ReleaseFromAreaBuses();
            }
        };

        bool m_connected = false;
        MockMeshRequestBus m_mockMeshRequestBus;
        MockTransformBus m_mockTransformBus;
        MockShapeComponentNotificationsBus m_mockShapeBus;
        MockMeshAsset m_mockMeshAssetData;
    };

    TEST_F(VegetationComponentOperationTests, MeshBlockerComponent)
    {
        // Create a mock mesh blocker at (0, 0, 0) that extends from (-1, -1, -1) to (1, 1, 1)
        auto mockAsset = CreateMockMeshAsset();
        auto entity = CreateMockMeshEntity(mockAsset, AZ::Vector3::CreateZero(), 
                                           AZ::Vector3(-1.0f), AZ::Vector3(1.0f), 0.0f, 1.0f);

        // Test the point at (0, 0, 0).  It should be blocked.
        const int numPointsBlocked = 1;
        TestMeshBlockerPoint(entity, AZ::Vector3::CreateZero(), numPointsBlocked);

        DestroyMockMeshAsset(mockAsset);
    }

    TEST_F(VegetationComponentOperationTests, LY96037_MeshBlockerIntersectionShouldUseZAxis)
    {
        // Create a mock mesh blocker at (0, 0, 0) that extends from (-1, -1, -1) to (1, 10, 1)
        auto mockAsset = CreateMockMeshAsset();
        auto entity = CreateMockMeshEntity(mockAsset, AZ::Vector3::CreateZero(),
                                           AZ::Vector3(-1.0f, -1.0f, -1.0f), AZ::Vector3(1.0f, 10.0f, 1.0f),
                                           0.0f, 1.0f);

        // Test the point at (0.5, 0.5, 2.0).  It should *not* be blocked.  
        // Bug LY96037 was that the Y axis is used for height instead of Z, which would cause the point to be blocked.
        // That would make this test fail.
        const int numPointsBlocked = 0;
        TestMeshBlockerPoint(entity, AZ::Vector3(0.5f, 0.5f, 2.0f), numPointsBlocked);

        DestroyMockMeshAsset(mockAsset);
    }

    TEST_F(VegetationComponentOperationTests, LY96068_MeshBlockerHandlesChangedClaimPoints)
    {
        // Create a mock mesh blocker at (0, 0, 0) that extends from (-1, -1, -1) to (1, 1, 1)
        auto mockAsset = CreateMockMeshAsset();
        auto entity = CreateMockMeshEntity(mockAsset, AZ::Vector3::CreateZero(),
                                           AZ::Vector3(-1.0f, -1.0f, -1.0f), AZ::Vector3(1.0f, 1.0f, 1.0f),
                                           0.0f, 1.0f);

        AreaBusScope scope(*this, *entity.get());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        EXPECT_TRUE(prepared);

        // Create two different contexts that "reuse" the same claim handle for different points.
        // The first one has a point at (0, 0, 0) that will be successfully blocked.
        // The second one has a point at (2, 2, 2) that should *not* be successfully blocked.
        // Bug LY96068 is that claim handles that change location don't refresh correctly in the Mesh Blocker component.
        AZ::Vector3 claimPosition1 = AZ::Vector3::CreateZero();
        AZ::Vector3 claimPosition2 = AZ::Vector3(2.0f);
        Vegetation::ClaimContext context = CreateContext<1>({ claimPosition1 });
        Vegetation::ClaimContext contextWithReusedHandle = CreateContext<1>({ claimPosition2 });
        contextWithReusedHandle.m_availablePoints[0].m_handle = context.m_availablePoints[0].m_handle;
            
        // The first time we try with this claim handler, the point should be claimed by the MeshBlocker.
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);
        EXPECT_EQ(1, m_createdCallbackCount);

        // Clear out our results
        m_createdCallbackCount = 0;

        // Send out a "surface changed" notification, as will as a tick bus tick, to give our mesh blocker a chance to refresh.
        SurfaceData::SurfaceDataSystemNotificationBus::Broadcast(&SurfaceData::SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entity->GetId(), 
                                                                 AZ::Aabb::CreateFromMinMax(claimPosition1, claimPosition2));
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.f, AZ::ScriptTimePoint{});

        // Try claiming again, this time with the same claim handle, but a different location.
        // This should *not* be claimed by the MeshBlocker.
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, contextWithReusedHandle);
        EXPECT_EQ(0, m_createdCallbackCount);

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);

        DestroyMockMeshAsset(mockAsset);
    }

    TEST_F(VegetationComponentOperationTests, StaticVegetationBlockerComponent)
    {
        struct MockStaticVegetationBus 
            : public Vegetation::StaticVegetationRequestBus::Handler
        {
            Vegetation::StaticVegetationMap m_map;// = AZStd::unordered_map<IRenderNode*, StaticVegetationData>;
            AZStd::shared_mutex vegMapLock;
            AZStd::array<IRenderNode*, 5> m_renderNodes;

            MockStaticVegetationBus()
            {
                Vegetation::StaticVegetationRequestBus::Handler::BusConnect();

                for(size_t i = 0; i < m_renderNodes.size(); ++i)
                {
                    m_renderNodes[i] = reinterpret_cast<IRenderNode*>(i);
                    m_map[m_renderNodes[i]] = { AZ::Vector3(0.0f,0.0f,0.0f), AZ::Aabb::CreateCenterRadius(AZ::Vector3(0.0f,0.0f,0.0f), 10) };
                }
            }

            ~MockStaticVegetationBus()
            {
                Vegetation::StaticVegetationRequestBus::Handler::BusDisconnect();
            }

            Vegetation::MapHandle GetStaticVegetation() override
            {
                return Vegetation::MapHandle(&m_map, m_map.size(), &vegMapLock);
            }
        };
        MockStaticVegetationBus mockStaticVegetationBus;
        m_mockShapeBus.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3(0.0f, 0.0f, 0.0f), 100);

        Vegetation::StaticVegetationBlockerConfig config;

        Vegetation::StaticVegetationBlockerComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        AreaBusScope scope(*this, *entity.get());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        EXPECT_TRUE(prepared);

        Vegetation::ClaimContext context = CreateContext<32>({ AZ::Vector3(0, 0, 0) });
        const auto previousPointsCount = context.m_availablePoints.size();
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);
        EXPECT_NE(previousPointsCount, context.m_availablePoints.size());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);
    }

    TEST_F(VegetationComponentOperationTests, SpawnerComponent)
    {
        m_mockShapeBus.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ_FLT_MAX);
        MockDescriptorProvider mockDescriptorProviderBus(8);
        MockInstanceSystemRequestBus mockInstanceSystemRequestBus;

        Vegetation::SpawnerConfig config;

        Vegetation::SpawnerComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        AreaBusScope scope(*this, *entity.get());
        mockDescriptorProviderBus.BusConnect(entity->GetId());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        EXPECT_TRUE(prepared);

        Vegetation::ClaimContext context = CreateContext<32>({ AZ::Vector3(0, 0, 0) });
        const Vegetation::ClaimHandle theClaimHandle = context.m_availablePoints[0].m_handle;
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);
        EXPECT_EQ(32, m_createdCallbackCount);
        EXPECT_EQ(32, mockInstanceSystemRequestBus.m_created);

        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::UnclaimPosition, theClaimHandle);
        EXPECT_EQ(33, mockInstanceSystemRequestBus.m_count);

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);

        mockDescriptorProviderBus.Clear();
        mockDescriptorProviderBus.BusDisconnect();
    }

    TEST_F(VegetationComponentOperationTests, AreaBlenderComponent)
    {
        auto entityBlocker = CreateEntity<Vegetation::BlockerComponent>(Vegetation::BlockerConfig(), nullptr, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        MockMeshRequestBus mockMeshRequestBusForBlocker;
        mockMeshRequestBusForBlocker.m_GetWorldBoundsOutput = AZ::Aabb::CreateCenterRadius(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ_FLT_MAX);
        mockMeshRequestBusForBlocker.m_GetVisibilityOutput = true;
        mockMeshRequestBusForBlocker.BusConnect(entityBlocker->GetId());

        MockTransformBus mockTransformBusForBlocker;
        mockTransformBusForBlocker.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f));
        mockMeshRequestBusForBlocker.BusConnect(entityBlocker->GetId());

        MockShapeComponentNotificationsBus mockShapeBusForBlocker;
        mockShapeBusForBlocker.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ_FLT_MAX);
        mockMeshRequestBusForBlocker.BusConnect(entityBlocker->GetId());
        
        Vegetation::AreaBlenderConfig config;
        config.m_vegetationAreaIds.push_back(entityBlocker->GetId());

        Vegetation::AreaBlenderComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        AreaBusScope scope(*this, *entity.get());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        EXPECT_TRUE(prepared);

        Vegetation::ClaimContext context = CreateContext<32>({ AZ::Vector3(0, 0, 0) });
        const auto previousPointsCount = context.m_availablePoints.size();
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, context);
        EXPECT_NE(previousPointsCount, context.m_availablePoints.size());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);

        mockMeshRequestBusForBlocker.BusDisconnect();
        mockMeshRequestBusForBlocker.BusDisconnect();
        mockMeshRequestBusForBlocker.BusDisconnect();
    }

    TEST_F(VegetationComponentOperationTests, BlockerComponent)
    {
        m_mockMeshRequestBus.m_GetWorldBoundsOutput = AZ::Aabb::CreateCenterRadius(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ_FLT_MAX);
        m_mockMeshRequestBus.m_GetVisibilityOutput = true;

        m_mockTransformBus.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f));

        m_mockShapeBus.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ_FLT_MAX);

        Vegetation::BlockerConfig config;
        config.m_inheritBehavior = false;

        Vegetation::BlockerComponent* component = nullptr;
        auto entity = CreateEntity(config, &component, [](AZ::Entity* e)
        {
            e->CreateComponent<MockShapeServiceComponent>();
        });

        AreaBusScope scope(*this, *entity.get());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaConnect);

        bool prepared = false;
        Vegetation::EntityIdStack idStack;
        Vegetation::AreaRequestBus::EventResult(prepared, entity->GetId(), &Vegetation::AreaRequestBus::Events::PrepareToClaim, idStack);
        EXPECT_TRUE(prepared);

        Vegetation::EntityIdStack stack;
        Vegetation::ClaimContext claimContext = CreateContext<32>({ AZ::Vector3(0, 0, 0) });
        Vegetation::AreaRequestBus::Event(entity->GetId(), &Vegetation::AreaRequestBus::Events::ClaimPositions, idStack, claimContext);
        EXPECT_EQ(32, m_createdCallbackCount);
        EXPECT_TRUE(claimContext.m_availablePoints.empty());

        Vegetation::AreaNotificationBus::Event(entity->GetId(), &Vegetation::AreaNotificationBus::Events::OnAreaDisconnect);
    }
}
