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
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include "Source/Rendering/EditorParticleComponent.h"

namespace UnitTest
{
    // Provides public access to protected/private members to allow for testing.
    class Test_EditorParticleComponent :
        public LmbrCentral::EditorParticleComponent
    {
    public:
        AZ_COMPONENT(Test_EditorParticleComponent, "{9A104116-AE4F-49B6-8827-D9AB75B260B2}", LmbrCentral::EditorParticleComponent);

        Test_EditorParticleComponent() : EditorParticleComponent()
        {

        }

        ~Test_EditorParticleComponent() override
        {

        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            if (serializeContext)
            {
                serializeContext->Class<Test_EditorParticleComponent, EditorParticleComponent>()->
                    Version(1);
            }
        }

        LmbrCentral::ParticleEmitterSettings Public_GetSettingsForEmitter() const
        {
            return GetSettingsForEmitter();
        }
    };

    class MockParticleHandlerAndCatalog
        : public AZ::Data::AssetHandler
        , public AZ::Data::AssetCatalog
    {
    public:
        AZ_CLASS_ALLOCATOR(MockParticleHandlerAndCatalog, AZ::SystemAllocator, 0);

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& /*type*/) override
        {
            return aznew LmbrCentral::ParticleAsset(id);
        }

        bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/, AZ::IO::GenericStream* /*stream*/, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/) override
        {
            return true;
        }

        bool LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/, const char* /*assetPath*/, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/) override
        {
            return true;
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(AZ::AzTypeInfo<LmbrCentral::ParticleAsset>::Uuid());
        }

        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& /*type*/) override
        {
            AZ::Data::AssetStreamInfo info;
            // Set valid stream info. This ensures the asset load doesn't result in an
            // error on another thread that can occur during shutdown.
            info.m_streamName = AZStd::string::format("MockParticleHandlerAndCatalog%s", assetId.ToString<AZStd::string>().c_str());
            info.m_dataOffset = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;
            info.m_isCustomStreamType = true;
            return info;
        }
    };

    class EditorParticleComponentTests
        : public ::testing::Test
        , public UnitTest::TraceBusRedirector
        , public AZ::Data::AssetBus::MultiHandler
    {
    protected:

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::MultiHandler

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            EXPECT_EQ(asset.GetId(), m_waitForAssetIdLoad);
            m_waitForAssetIdLoad.SetInvalid();
            m_assetLoadSemaphore.release();
        }

        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            // No asset errors should happen during this test.
            EXPECT_TRUE(false);
        }

        //////////////////////////////////////////////////////////////////////////

        void SetUp() override
        {
            m_app.Start(m_descriptor);

            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_editorParticleComponentDescriptor = LmbrCentral::EditorParticleComponent::CreateDescriptor();
            m_testEditorParticleComponentDescriptor = Test_EditorParticleComponent::CreateDescriptor();
            m_particleComponentDescriptor = LmbrCentral::ParticleComponent::CreateDescriptor();

            m_testEditorParticleComponentDescriptor->Reflect(m_app.GetSerializeContext());
            m_particleComponentDescriptor->Reflect(m_app.GetSerializeContext());
            AZ::Debug::TraceMessageBus::Handler::BusConnect();

            AZ::Data::AssetManager::Instance().RegisterHandler(&m_particleHandlerAndCatalog, AZ::AzTypeInfo<LmbrCentral::ParticleAsset>::Uuid());
            AZ::Data::AssetManager::Instance().RegisterCatalog(&m_particleHandlerAndCatalog, AZ::AzTypeInfo<LmbrCentral::ParticleAsset>::Uuid());

        }

        void TearDown() override
        {
            if (m_waitForAssetIdLoad.IsValid())
            {
                const int assetLoadSleepMS = 20;
                int totalWaitTimeMS = 5000;
                bool assetLoaded = false;
                while (!assetLoaded && totalWaitTimeMS > 0)
                {
                    assetLoaded = m_assetLoadSemaphore.try_acquire_for(AZStd::chrono::milliseconds(assetLoadSleepMS));
                    AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
                    if (!assetLoaded)
                    {
                        totalWaitTimeMS -= assetLoadSleepMS;
                    }
                }
                EXPECT_GT(totalWaitTimeMS, 0);
                EXPECT_TRUE(assetLoaded);
                AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            }

            AZ::Data::AssetManager::Instance().UnregisterHandler(&m_particleHandlerAndCatalog);
            AZ::Data::AssetManager::Instance().UnregisterCatalog(&m_particleHandlerAndCatalog);

            m_editorParticleComponentDescriptor->ReleaseDescriptor();
            m_testEditorParticleComponentDescriptor->ReleaseDescriptor();
            m_particleComponentDescriptor->ReleaseDescriptor();
            m_app.Stop();

            // Disconnect last, ToolsApplication::Stop() can result in messages that we are using 
            // TraceMessageBus to handle.
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        void SetupParticleEntity(AZ::Entity& entity)
        {
            entity.CreateComponent<AzFramework::TransformComponent>();
            entity.CreateComponent<Test_EditorParticleComponent>();

            entity.Init();
            entity.Activate();
        }

        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;

        MockParticleHandlerAndCatalog m_particleHandlerAndCatalog;

        AZ::ComponentDescriptor* m_editorParticleComponentDescriptor = nullptr;
        AZ::ComponentDescriptor* m_testEditorParticleComponentDescriptor = nullptr;
        AZ::ComponentDescriptor* m_particleComponentDescriptor = nullptr;

        // If this is set to a valid asset ID, teardown won't finish until
        // this asset has been loaded. If this takes too long, teardown will error out.
        AZ::Data::AssetId m_waitForAssetIdLoad;
        AZStd::binary_semaphore m_assetLoadSemaphore;
    };

    TEST_F(EditorParticleComponentTests, EditorParticleComponent_AddEditorParticleComponent_ComponentExists)
    {
        AZ::Entity entity;
        SetupParticleEntity(entity);

        Test_EditorParticleComponent* particleComponent =
            entity.FindComponent<Test_EditorParticleComponent>();
        EXPECT_TRUE(particleComponent != nullptr);
    }

    TEST_F(EditorParticleComponentTests, EditorParticleComponent_SetAssetId_GetAssetIdMatchesSet)
    {
        AZ::Entity entity;
        SetupParticleEntity(entity);

        // Use an arbitrary, non-default asset ID to verify the set & get work.
        AZ::Data::AssetId assetIdToSet(AZ::Uuid("{44329170-A798-4ED7-8BE4-8BD849B278B0}"), 5);
        // m_waitForAssetIdLoad is cleared when the asset load finishes, so keep a local copy of the asset ID.
        m_waitForAssetIdLoad = assetIdToSet;
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_waitForAssetIdLoad);

        Test_EditorParticleComponent* particleComponent =
            entity.FindComponent<Test_EditorParticleComponent>();
        particleComponent->SetPrimaryAsset(assetIdToSet);

        LmbrCentral::ParticleEmitterSettings emitterSettings =
            particleComponent->Public_GetSettingsForEmitter();
        EXPECT_EQ(emitterSettings.m_asset.GetId(), assetIdToSet);
    }

    TEST_F(EditorParticleComponentTests, EditorParticleComponent_SetAssetIdBuildGameEntity_GameEntityHasAssetId)
    {
        AZ::Entity entity;
        SetupParticleEntity(entity);

        // Use an arbitrary, non-default asset ID to verify the set & get work.
        AZ::Data::AssetId assetIdToSet(AZ::Uuid("{64583C07-DB70-4E5C-8844-2F9565D1250C}"), 6);
        // m_waitForAssetIdLoad is cleared when the asset load finishes, so keep a local copy of the asset ID.
        m_waitForAssetIdLoad = assetIdToSet;
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_waitForAssetIdLoad);
        Test_EditorParticleComponent* particleComponent =
            entity.FindComponent<Test_EditorParticleComponent>();

        particleComponent->SetPrimaryAsset(assetIdToSet);
        
        AZ::Entity gameEntity;
        particleComponent->BuildGameEntity(&gameEntity);
        LmbrCentral::ParticleComponent* gameComponent =
            gameEntity.FindComponent<LmbrCentral::ParticleComponent>();
        EXPECT_TRUE(gameComponent != nullptr);
        EXPECT_EQ(gameComponent->GetEmitterSettings().m_asset.GetId(), assetIdToSet);
        
    }
} // namespace UnitTest
