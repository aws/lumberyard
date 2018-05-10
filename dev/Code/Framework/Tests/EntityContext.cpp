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

#include <Tests/TestTypes.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzFramework/Entity/EntityContext.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    class EntityContextBasicTest
        : public AllocatorsFixture
        , public EntityContextEventBus::Handler
    {
    public:

        EntityContextBasicTest()
            : AllocatorsFixture(15, false)
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            Data::AssetManager::Descriptor desc;
            Data::AssetManager::Create(desc);
        }

        virtual ~EntityContextBasicTest()
        {
        }

        void TearDown() override
        {
            Data::AssetManager::Destroy();

            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<ThreadPoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void run()
        {
            ComponentApplication app;
            ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            app.Create(desc);

            Data::AssetManager::Instance().RegisterHandler(aznew SliceAssetHandler(app.GetSerializeContext()), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());

            EntityContext context(AZ::Uuid::CreateRandom(), app.GetSerializeContext());
            context.InitContext();
            context.GetRootSlice()->SetSerializeContext(app.GetSerializeContext());

            EntityContextEventBus::Handler::BusConnect(context.GetContextId());

            AZ_TEST_ASSERT(context.GetRootSlice()); // Should have a root prefab.

            AZ::Entity* entity = context.CreateEntity("MyEntity");
            AZ_TEST_ASSERT(entity); // Should have created the entity.
            AZ::SliceComponent::EntityList entities;
            context.GetRootSlice()->GetEntities(entities);
            AZ_TEST_ASSERT(!entities.empty());
            AZ_TEST_ASSERT(*entities.begin() == entity);
            AZ_TEST_ASSERT(m_createEntityEvents == 1);

            AZ::Uuid contextId = AZ::Uuid::CreateNull();
            EBUS_EVENT_ID_RESULT(contextId, entity->GetId(), EntityIdContextQueryBus, GetOwningContextId);

            AZ_TEST_ASSERT(contextId == context.GetContextId()); // Context properly associated with entity?

            AZ_TEST_ASSERT(context.DestroyEntity(entity));
            AZ_TEST_ASSERT(m_destroyEntityEvents == 1);

            AZ::Entity* sliceEntity = aznew AZ::Entity();
            AZ::SliceComponent* sliceComponent = sliceEntity->CreateComponent<AZ::SliceComponent>();
            sliceComponent->SetSerializeContext(app.GetSerializeContext());
            sliceComponent->AddEntity(aznew AZ::Entity());

            Data::Asset<SliceAsset> sliceAssetHolder = Data::AssetManager::Instance().CreateAsset<SliceAsset>(Data::AssetId(Uuid::CreateRandom()));
            SliceAsset* sliceAsset = sliceAssetHolder.Get();
            sliceAsset->SetData(sliceEntity, sliceComponent);

            context.InstantiateSlice(sliceAssetHolder);
            AZ::TickBus::ExecuteQueuedEvents();
            entities.clear();
            context.GetRootSlice()->GetEntities(entities);
            AZ_TEST_ASSERT(entities.size() == 1);

            app.Destroy();
        }

        void OnEntityContextCreateEntity(AZ::Entity& entity) override
        {
            (void)entity;
            ++m_createEntityEvents;
        }

        void OnEntityContextDestroyEntity(const AZ::EntityId& entity) override
        {
            (void)entity;
            ++m_destroyEntityEvents;
        }

        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance) override
        {
            (void)sliceAssetId;
            (void)instance;
            ++m_prefabSuccesses;
            prefabResults = instance.second->GetInstantiated()->m_entities;
        }

        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
        {
            (void)sliceAssetId;
            ++m_prefabFailures;
        }

        size_t m_createEntityEvents = 0;
        size_t m_destroyEntityEvents = 0;
        size_t m_prefabSuccesses = 0;
        size_t m_prefabFailures = 0;
        AZStd::vector<AZ::Entity*> prefabResults;
    };

    TEST_F(EntityContextBasicTest, Test)
    {
        run();
    }
}
