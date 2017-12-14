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
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Components/BootstrapReaderComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <Tests/TestTypes.h>
// LmbrCentral/Source
#include "Scripting/SpawnerComponent.h"

// Tracks SpawnerComponentNotificationBus events
class SpawnWatcher
    : public LmbrCentral::SpawnerComponentNotificationBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(SpawnWatcher, AZ::SystemAllocator, 0);

    SpawnWatcher(AZ::EntityId spawnerEntityId)
    {
        LmbrCentral::SpawnerComponentNotificationBus::Handler::BusConnect(spawnerEntityId);
    }

    void OnSpawnBegin(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnBegin = true;
    }

    void OnSpawnEnd(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnEnd = true;
    }

    void OnEntitySpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZ::EntityId& spawnedEntity) override
    {
        m_tickets[ticket].m_onEntitySpawned.emplace_back(spawnedEntity);
    }

    void OnEntitiesSpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZStd::vector<AZ::EntityId>& spawnedEntities) override
    {
        m_tickets[ticket].m_onEntitiesSpawned = spawnedEntities;
    }

    void OnSpawnedSliceDestroyed(const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_tickets[ticket].m_onSpawnedSliceDestroyed = true;
    }

    // Which events have fired for a particular ticket
    struct TicketInfo
    {
        bool m_onSpawnBegin = false;
        bool m_onSpawnEnd = false;
        AZStd::vector<AZ::EntityId> m_onEntitySpawned;
        AZStd::vector<AZ::EntityId> m_onEntitiesSpawned;
        bool m_onSpawnedSliceDestroyed = false;
    };

    AZStd::unordered_map<AzFramework::SliceInstantiationTicket, TicketInfo> m_tickets;
};

// Simplified version of AzFramework::Application
class SpawnerApplication
    : public AzFramework::Application
{
    // override and only include system components required for spawner tests.
    AZ::ComponentTypeList GetRequiredSystemComponents() const override
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AZ::MemoryComponent>(),
            azrtti_typeid<AZ::AssetManagerComponent>(),
            azrtti_typeid<AZ::SliceSystemComponent>(),
            azrtti_typeid<AzFramework::BootstrapReaderComponent>(),
            azrtti_typeid<AzFramework::GameEntityContextComponent>(),
            azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
        };
    }
};

class SpawnerComponentTest
    : public testing::Test
{
public:
    void SetUp() override
    {
        // start application
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create(AZ::SystemAllocator::Descriptor());

        AZ::ComponentApplication::Descriptor appDescriptor;
        appDescriptor.m_useExistingAllocator = true;

        m_application = aznew SpawnerApplication();

        m_application->Start(appDescriptor, AZ::ComponentApplication::StartupParameters());

        // create a dynamic slice in the asset system
        AZ::Entity* sliceAssetEntity = aznew AZ::Entity();
        AZ::SliceComponent* sliceAssetComponent = sliceAssetEntity->CreateComponent<AZ::SliceComponent>();
        sliceAssetComponent->SetSerializeContext(m_application->GetSerializeContext());
        sliceAssetEntity->Init();
        sliceAssetEntity->Activate();

        AZ::Entity* entityInSlice1 = aznew AZ::Entity("spawned entity 1");
        entityInSlice1->CreateComponent<AzFramework::TransformComponent>();
        sliceAssetComponent->AddEntity(entityInSlice1);

        AZ::Entity* entityInSlice2 = aznew AZ::Entity("spawned entity 2");
        entityInSlice2->CreateComponent<AzFramework::TransformComponent>();
        sliceAssetComponent->AddEntity(entityInSlice2);

        m_sliceAssetRef = AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(AZ::Data::AssetId("{E47E78B1-FF5E-4191-BE72-A06428D324F3}"));
        m_sliceAssetRef.Get()->SetData(sliceAssetEntity, sliceAssetComponent);

        // create an entity with a spawner component
        AZ::Entity* spawnerEntity = aznew AZ::Entity("spawner");
        m_spawnerComponent = spawnerEntity->CreateComponent<LmbrCentral::SpawnerComponent>();
        spawnerEntity->Init();
        spawnerEntity->Activate();

        // create class that will watch for spawner component notifications
        m_spawnWatcher = aznew SpawnWatcher(m_spawnerComponent->GetEntityId());
    }

    void TearDown() override
    {
        delete m_spawnWatcher;
        m_spawnWatcher = nullptr;

        delete m_application->FindEntity(m_spawnerComponent->GetEntityId());
        m_spawnerComponent = nullptr;

        // reset game context (delete any spawned slices and their entities)
        AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::ResetGameContext);

        m_sliceAssetRef = AZ::Data::Asset<AZ::SliceAsset>();

        m_application->Stop();
        delete m_application;
        m_application = nullptr;

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    // Tick application until 'condition' function returns true.
    // If 'maxTicks' elapse without condition passing, return false.
    bool TickUntil(AZStd::function<bool()> condition, size_t maxTicks=100)
    {
        for (size_t tickI = 0; tickI < maxTicks; ++tickI)
        {
            if (condition())
            {
                return true;
            }

            m_application->Tick();
        }
        return false;
    }

    // Common test operation: Spawn m_sliceAssetRef and tick application until OnSpawnEnd fires.
    AzFramework::SliceInstantiationTicket SpawnDefaultSlice()
    {
        AzFramework::SliceInstantiationTicket ticket = m_spawnerComponent->SpawnSlice(m_sliceAssetRef);
        bool onSpawnEndFired = TickUntil([this, ticket]() { return m_spawnWatcher->m_tickets[ticket].m_onSpawnEnd; });
        EXPECT_TRUE(onSpawnEndFired); // sanity check
        return ticket;
    }

    // Common test operation: Spawn m_sliceAssetRef many times and tick application until OnSpawnEnd fires for each spawn.
    AZStd::vector<AzFramework::SliceInstantiationTicket> SpawnManyDefaultSlices()
    {
        AZStd::vector<AzFramework::SliceInstantiationTicket> tickets;
        for (int i = 0; i < 10; ++i)
        {
            tickets.emplace_back(m_spawnerComponent->SpawnSlice(m_sliceAssetRef));
        }

        bool onSpawnEndFiredForAll = TickUntil(
            [this, &tickets]()
            {
                for (AzFramework::SliceInstantiationTicket& ticket : tickets)
                {
                    if (!m_spawnWatcher->m_tickets[ticket].m_onSpawnEnd)
                    {
                        return false;
                    }
                }
                return true;
            });
        EXPECT_TRUE(onSpawnEndFiredForAll); // sanity check

        return tickets;
    }


    SpawnerApplication*             m_application = nullptr;
    AZ::Data::Asset<AZ::SliceAsset> m_sliceAssetRef; // a slice asset to spawn
    LmbrCentral::SpawnerComponent*  m_spawnerComponent = nullptr; // a spawner component to test
    SpawnWatcher*                   m_spawnWatcher = nullptr; // tracks events from the spawner component
};

const size_t kEntitiesInSlice = 2; // number of entities in asset we're testing with

TEST_F(SpawnerComponentTest, SanityCheck)
{
}

TEST_F(SpawnerComponentTest, SpawnSlice_OnSpawnEnd_Fires)
{
    // First test the helper function, which checks for OnSpawnEnd
    SpawnDefaultSlice();
}

TEST_F(SpawnerComponentTest, SpawnSlice_OnSpawnBegin_Fires)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();
    EXPECT_TRUE(m_spawnWatcher->m_tickets[ticket].m_onSpawnBegin);
}

TEST_F(SpawnerComponentTest, SpawnSlice_OnEntitySpawned_FiresOncePerEntity)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    EXPECT_EQ(kEntitiesInSlice, m_spawnWatcher->m_tickets[ticket].m_onEntitySpawned.size());
}

TEST_F(SpawnerComponentTest, SpawnSlice_OnEntitiesSpawned_FiresWithAllEntities)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    EXPECT_EQ(kEntitiesInSlice, m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned.size());
}

TEST_F(SpawnerComponentTest, OnSpawnedSliceDestroyed_FiresAfterEntitiesDeleted)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    for (AZ::EntityId spawnedEntityId : m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
    {
        AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity, spawnedEntityId);
    }

    bool spawnDestructionFired = TickUntil([this, ticket]() { return m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed; });
    EXPECT_TRUE(spawnDestructionFired);
}


TEST_F(SpawnerComponentTest, DISABLED_OnSpawnedSliceDestroyed_FiresWhenSpawningBadAssets) // disabled because AZ_TEST_START_ASSERTTEST isn't currently suppressing the asserts
{
    // ID is made up and not registered with asset manager
    auto nonexistentAsset = AZ::Data::Asset<AZ::SliceAsset>(AZ::Data::AssetId("{9E3862CC-B6DF-485F-A9D8-5F4A966DE88B}"), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
    AzFramework::SliceInstantiationTicket ticket = m_spawnerComponent->SpawnSlice(nonexistentAsset);

    AZ_TEST_START_ASSERTTEST;
    bool spawnDestructionFired = TickUntil([this, ticket]() { return m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed; });
    AZ_TEST_STOP_ASSERTTEST(1);
    EXPECT_TRUE(spawnDestructionFired);
}

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_EntitiesFromSpawn_AreDeleted)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    m_spawnerComponent->DestroySpawnedSlice(ticket);
    bool entitiesRemoved = TickUntil(
        [this, ticket]()
        {
            for (AZ::EntityId entityId : m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
            {
                if (m_application->FindEntity(entityId))
                {
                    return false;
                }
            }
            return true;
        });

    EXPECT_TRUE(entitiesRemoved);
}

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_OnSpawnedSliceDestroyed_Fires)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    m_spawnerComponent->DestroySpawnedSlice(ticket);
    bool onSpawnedSliceDestroyed = TickUntil([this, ticket](){ return m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed; });

    EXPECT_TRUE(onSpawnedSliceDestroyed);
}

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_BeforeOnSpawnBegin_PreventsInstantiation)
{
    AzFramework::SliceInstantiationTicket ticket = m_spawnerComponent->SpawnSlice(m_sliceAssetRef);
    m_spawnerComponent->DestroySpawnedSlice(ticket);

    // wait a long time, just to be sure no queued entity instantiation takes place
    for (int i = 0; i < 100; ++i)
    {
        m_application->Tick();
    }

    EXPECT_FALSE(m_spawnWatcher->m_tickets[ticket].m_onSpawnBegin);
    EXPECT_TRUE(m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed);
}

class GameEntityContextWatcher : public AzFramework::GameEntityContextEventBus::Handler
{
public:
    GameEntityContextWatcher()
    {
        AzFramework::GameEntityContextEventBus::Handler::BusConnect();
    }

    void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*instance*/, const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_onSliceInstantiatedTickets.emplace(ticket);
    }

    void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/, const AzFramework::SliceInstantiationTicket& ticket) override
    {
        m_onSliceInstantiationFailedTickets.emplace(ticket);
    }
    
    AZStd::unordered_set<AzFramework::SliceInstantiationTicket> m_onSliceInstantiatedTickets;
    AZStd::unordered_set<AzFramework::SliceInstantiationTicket> m_onSliceInstantiationFailedTickets;
};

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_BeforeOnSpawnBegin_ContextFiresOnSliceInstantiationFailed)
{
    // context should send out instantiation failure message, even if ticket is explicitly cancelled.
    // others might be listening to the context and not know about the cancellation.

    GameEntityContextWatcher contextWatcher;
    AzFramework::SliceInstantiationTicket ticket = m_spawnerComponent->SpawnSlice(m_sliceAssetRef);
    m_spawnerComponent->DestroySpawnedSlice(ticket);

    bool onSpawnedSliceDestroyed = TickUntil([this, ticket]() { return m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed; });

    bool onSliceInstantiationFailed = contextWatcher.m_onSliceInstantiationFailedTickets.count(ticket) > 0;
    EXPECT_TRUE(onSliceInstantiationFailed);

    bool onSliceInstantiated = contextWatcher.m_onSliceInstantiatedTickets.count(ticket) > 0;
    EXPECT_FALSE(onSliceInstantiated);
}

TEST_F(SpawnerComponentTest, DestroySpawnedSlice_WhenManySpawnsInProgress_DoesntAffectOtherSpawns)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets;
    for (int i = 0; i < 10; ++i)
    {
        tickets.emplace_back(m_spawnerComponent->SpawnSlice(m_sliceAssetRef));
    }

    m_spawnerComponent->DestroySpawnedSlice(tickets[0]);

    // check that other slices finish spawning
    bool entitiesSpawnedInOtherSlices = TickUntil(
        [this, &tickets]()
        {
            for (size_t i = 1; i < tickets.size(); ++i)
            {
                if (m_spawnWatcher->m_tickets[tickets[i]].m_onEntitiesSpawned.size() > 0)
                {
                    return false;
                }
            }
            return true;
        });

    EXPECT_TRUE(entitiesSpawnedInOtherSlices);

    // check that one slice destroyed
    bool sliceDestroyed = TickUntil([this, &tickets]()
    {
        return m_spawnWatcher->m_tickets[tickets[0]].m_onSpawnedSliceDestroyed;
    });
    EXPECT_TRUE(sliceDestroyed);

    // make sure no other slice get destroyed
    bool anyOtherSliceDestroyed = false;
    for (size_t i = 1; i < tickets.size(); ++i)
    {
        if (m_spawnWatcher->m_tickets[tickets[i]].m_onSpawnedSliceDestroyed)
        {
            anyOtherSliceDestroyed = true;
        }
    }
    EXPECT_FALSE(anyOtherSliceDestroyed);
}

TEST_F(SpawnerComponentTest, DestroyAllSpawnedSlices_AllSpawnedEntities_AreDestroyed)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets = SpawnManyDefaultSlices();

    m_spawnerComponent->DestroyAllSpawnedSlices();

    bool allEntitiesDestroyed = TickUntil(
        [this, &tickets]()
        {
            for (AzFramework::SliceInstantiationTicket& ticket : tickets)
            {
                for (AZ::EntityId spawnedEntityId : m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned)
                {
                    if (m_application->FindEntity(spawnedEntityId))
                    {
                        return false;
                    }
                }
            }
            return true;
        });
    EXPECT_TRUE(allEntitiesDestroyed);
}

TEST_F(SpawnerComponentTest, DestroyAllSpawnedSlices_OnSpawnedSliceDestroyed_FiresForAll)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets = SpawnManyDefaultSlices();

    m_spawnerComponent->DestroyAllSpawnedSlices();

    bool onSpawnedSliceDestroyedFiresForAll = TickUntil(
        [this, &tickets]()
        {
            for (AzFramework::SliceInstantiationTicket& ticket : tickets)
            {
                if (!m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed)
                {
                    return false;
                }
            }
            return true;
        });
    EXPECT_TRUE(onSpawnedSliceDestroyedFiresForAll);
}

TEST_F(SpawnerComponentTest, DestroyAllSpawnedSlices_BeforeOnSpawnBegin_PreventsInstantiation)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets;
    for (int i = 0; i < 10; ++i)
    {
        tickets.emplace_back(m_spawnerComponent->SpawnSlice(m_sliceAssetRef));
    }

    m_spawnerComponent->DestroyAllSpawnedSlices();

    // wait a long time, to ensure no queued activity results in an instantiation
    for (int i = 0; i < 100; ++i)
    {
        m_application->Tick();
    }

    bool anyOnSpawnBegan = false;
    bool allOnSpawnedSliceDestroyed = true;
    for (AzFramework::SliceInstantiationTicket& ticket : tickets)
    {
        anyOnSpawnBegan |= m_spawnWatcher->m_tickets[ticket].m_onSpawnBegin;
        allOnSpawnedSliceDestroyed &= m_spawnWatcher->m_tickets[ticket].m_onSpawnedSliceDestroyed;
    }

    EXPECT_FALSE(anyOnSpawnBegan);
    EXPECT_TRUE(allOnSpawnedSliceDestroyed);
}

TEST_F(SpawnerComponentTest, GetCurrentEntitiesFromSpawnedSlice_ReturnsEntities)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();
    AZStd::vector<AZ::EntityId> entities = m_spawnerComponent->GetCurrentEntitiesFromSpawnedSlice(ticket);

    EXPECT_EQ(m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned.size(), entities.size());
}

TEST_F(SpawnerComponentTest, GetCurrentEntitiesFromSpawnedSlice_WithEntityDeleted_DoesNotReturnDeletedEntity)
{
    AzFramework::SliceInstantiationTicket ticket = SpawnDefaultSlice();

    AZStd::vector<AZ::EntityId>& entitiesBeforeDelete = m_spawnWatcher->m_tickets[ticket].m_onEntitiesSpawned;

    AZ::EntityId entityToDelete = entitiesBeforeDelete[0];
    delete m_application->FindEntity(entityToDelete);

    AZStd::vector<AZ::EntityId> entitiesAfterDelete = m_spawnerComponent->GetCurrentEntitiesFromSpawnedSlice(ticket);

    EXPECT_EQ(entitiesBeforeDelete.size() - 1, entitiesAfterDelete.size());

    bool deletedEntityPresent = AZStd::find(entitiesAfterDelete.begin(), entitiesAfterDelete.end(), entityToDelete) != entitiesAfterDelete.end();
    EXPECT_FALSE(deletedEntityPresent);
}

TEST_F(SpawnerComponentTest, GetAllCurrentlySpawnedEntities_ReturnsEntities)
{
    AZStd::vector<AzFramework::SliceInstantiationTicket> tickets = SpawnManyDefaultSlices();

    AZStd::vector<AZ::EntityId> entities = m_spawnerComponent->GetAllCurrentlySpawnedEntities();

    bool allEntitiesFound = true;
    size_t numEntities = 0;

    // compare against entities from OnEntitiesSpawned event
    for (auto& ticketInfoPair : m_spawnWatcher->m_tickets)
    {
        for (AZ::EntityId spawnedEntity : ticketInfoPair.second.m_onEntitiesSpawned)
        {
            ++numEntities;
            allEntitiesFound &= AZStd::find(entities.begin(), entities.end(), spawnedEntity) != entities.end();
        }
    }

    EXPECT_EQ(numEntities, entities.size());
    EXPECT_TRUE(allEntitiesFound);
}