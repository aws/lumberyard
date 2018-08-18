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
#include "Multiplayer_precompiled.h"
#include <AzTest/AzTest.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/chrono/chrono.h>

#include <AzFramework/Script/ScriptComponent.h>

#include <Multiplayer/GridMateServiceWrapper/GridMateLANServiceWrapper.h>
#include <Multiplayer/GridMateServiceWrapper/GridMateServiceWrapper.h>
#include <Multiplayer/BehaviorContext/GridSearchContext.h>
#include <Multiplayer/BehaviorContext/GridSystemContext.h>
#include <MultiplayerGameLiftClient.h>

#include "MultiplayerMocks.h"
#include <gmock/gmock.h>

#include <INetwork.h>
#include <ISystem.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define MULTIPLAYERTEST_CPP_SECTION_1 1
#define MULTIPLAYERTEST_CPP_SECTION_2 2
#endif

// see also: dev/Code/Framework/AzCore/Tests/TestTypes.h
namespace UnitTest
{
    struct TestNetwork : public INetwork
    {
        TestNetwork() : m_gridMate(nullptr)
        {
        }
        GridMate::IGridMate* m_gridMate;

        void Release() override {}
        void GetMemoryStatistics(ICrySizer* pSizer) override {}
        void GetBandwidthStatistics(SBandwidthStats* const pStats) override {}
        void GetPerformanceStatistics(SNetworkPerformance* pSizer) override {}
        void GetProfilingStatistics(SNetworkProfilingStats* const pStats) override {}
        void SyncWithGame(ENetworkGameSync syncType) override {}
        const char* GetHostName() override { return "testhostname"; }
        GridMate::IGridMate* GetGridMate() override
        {
            return m_gridMate;
        }
        ChannelId GetChannelIdForSessionMember(GridMate::GridMember* member) const override { return ChannelId(); }
        ChannelId GetServerChannelId() const override { return ChannelId(); }
        ChannelId GetLocalChannelId() const override { return ChannelId(); }
        CTimeValue GetSessionTime() override { return CTimeValue(); }
        void SetGameContext(IGameContext* gameContext) override {}
        void ChangedAspects(EntityId id, NetworkAspectType aspectBits) override {}
        void SetDelegatableAspectMask(NetworkAspectType aspectBits) override {}
        void SetObjectDelegatedAspectMask(EntityId entityId, NetworkAspectType aspects, bool set) override {}
        void DelegateAuthorityToClient(EntityId entityId, ChannelId clientChannelId) override {}
        void InvokeRMI(IGameObject* gameObject, IRMIRep& rep, void* params, ChannelId targetChannelFilter, uint32 where) override {}
        void InvokeActorRMI(EntityId entityId, uint8 actorExtensionId, ChannelId targetChannelFilter, IActorRMIRep& rep) override {}
        void InvokeScriptRMI(ISerializable* serializable, bool isServerRMI, ChannelId toChannelId = kInvalidChannelId, ChannelId avoidChannelId = kInvalidChannelId) override {}
        void RegisterActorRMI(IActorRMIRep* rep) override {}
        void UnregisterActorRMI(IActorRMIRep* rep) override {}
        EntityId LocalEntityIdToServerEntityId(EntityId localId) const override { return EntityId(); }
        EntityId ServerEntityIdToLocalEntityId(EntityId serverId, bool allowForcedEstablishment = false) const override { return EntityId(); }
    };

    SSystemGlobalEnvironment g_testSystemGlobalEnvironment;

    class TestingNetworkProcessor
        : public GridMate::SessionEventBus::Handler
    {
        GridMate::GridSession* m_session;
        GridMate::IGridMate* m_gridMate;

    public:
        TestingNetworkProcessor()
            : m_session(nullptr)
            , m_gridMate(nullptr)
        {
        }
        ~TestingNetworkProcessor()
        {
            Reset();
        }
        void Reset()
        {
            SetGridMate(nullptr);
            SetSession(nullptr);
        }
        void SetSession(GridMate::GridSession* s)
        {
            if (m_session)
            {
                m_session->Leave(false);
            }
            m_session = s;
        }
        void SetGridMate(GridMate::IGridMate* gm)
        {
            m_gridMate = gm;
            if (m_gridMate)
            {
                BusConnect(m_gridMate);
            }
            else
            {
                BusDisconnect();
            }
        }
        void Update()
        {
            GridMate::ReplicaManager* replicaManager = m_session ? m_session->GetReplicaMgr() : nullptr;
            if (replicaManager)
            {
                replicaManager->Unmarshal();
                replicaManager->UpdateFromReplicas();
                replicaManager->UpdateReplicas();
                replicaManager->Marshal();
            }
            if (m_gridMate)
            {
                m_gridMate->Update();
            }
        }
        void OnSessionCreated(GridMate::GridSession* session) override
        {
            SetSession(session);
        }
        void OnSessionDelete(GridMate::GridSession* session) override
        {
            (void)session;
            SetSession(nullptr);
        }
    };

    class AllocatorsFixture
        : public ::testing::Test
    {
        void* m_memBlock;
        bool m_initSystemAlloc;
        bool m_useMemoryDriller;
        unsigned int m_heapSizeMB;
        GridMate::IGridMate* m_gridMate;
        TestNetwork m_testNetwork;
        SSystemGlobalEnvironment* m_oldEnv;

    public:
        AllocatorsFixture(bool initSystemAlloc = true, unsigned int heapSizeMB = 15, bool isMemoryDriller = true)
            : m_memBlock(nullptr)
            , m_initSystemAlloc(initSystemAlloc)
            , m_useMemoryDriller(isMemoryDriller)
            , m_heapSizeMB(heapSizeMB)
            , m_gridMate(nullptr)
            , m_oldEnv(gEnv)
        {
        }

        virtual ~AllocatorsFixture()
        {
            gEnv = m_oldEnv;
        }

        void SetUp() override
        {
            if (m_initSystemAlloc)
            {
                AZ::SystemAllocator::Descriptor desc;
                desc.m_heap.m_numMemoryBlocks = 1;
                desc.m_heap.m_memoryBlocksByteSize[0] = m_heapSizeMB * 1024 * 1024;
                m_memBlock = DebugAlignAlloc(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
                desc.m_heap.m_memoryBlocks[0] = m_memBlock;
                desc.m_stackRecordLevels = 10;

                AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
            }

            // faking the global environment
            g_testSystemGlobalEnvironment.pNetwork = &m_testNetwork;
            gEnv = &g_testSystemGlobalEnvironment;
        }

        void TearDown() override
        {
            if (m_gridMate)
            {
                GridMate::GridMateDestroy(m_gridMate);
                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
                m_gridMate = nullptr;
                g_testSystemGlobalEnvironment.pNetwork = nullptr;
            }

            if (m_initSystemAlloc)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
                DebugAlignFree(m_memBlock);
            }
        }

        GridMate::IGridMate* GetGridMate()
        {
            if (m_gridMate == nullptr)
            {
                m_gridMate = GridMate::GridMateCreate(GridMate::GridMateDesc());
                m_testNetwork.m_gridMate = m_gridMate;

                GridMate::GridMateAllocatorMP::Descriptor allocDesc;
                allocDesc.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create(allocDesc);
            }
            return m_gridMate;
        }
    };
}

/**
* MultiplayerTest
*/

class MultiplayerTest
    : public UnitTest::AllocatorsFixture
{
public:
    void run()
    {
    }

};

TEST_F(MultiplayerTest, SanityTest)
{
    ASSERT_TRUE(true);
}

/**
  * GridMateServiceWrapper
  */
class GridMateServiceWrapperTest
    : public UnitTest::AllocatorsFixture
{
public:
    void run()
    {
        GetGridMate();
        LANSimpleTest();
        LANHostAndOnePeer();
    }

    static GridMate::GridSessionParam FetchLANParam(const char* param)
    {
        GridMate::GridSessionParam p;

        if (!strcmp(param, "cl_clientport"))
        {
            p.SetValue(8080);
        }
        else if (!strcmp(param, "gm_ipversion"))
        {

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MULTIPLAYERTEST_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(MultiplayerTest_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            p.SetValue(GridMate::Driver::BSD_AF_INET);
#endif
        }

        return p;
    }

    void LANSimpleTest()
    {
        GridMate::SessionParams sessionParams;
        sessionParams.m_numPublicSlots = 2;
        Multiplayer::GridMateServiceParams serviceParams(sessionParams, FetchLANParam);

        Multiplayer::GridMateLANServiceWrapper gmLANService;
        GridMate::CarrierDesc carrierDesc;
        GridMate::GridSession* session = gmLANService.CreateServer(GetGridMate(), carrierDesc, serviceParams);
        gmLANService.StopSessionService(GetGridMate());
    }

    void LANHostAndOnePeer()
    {
        GridMate::SessionParams sessionParams;
        sessionParams.m_topology = GridMate::ST_CLIENT_SERVER;
        sessionParams.m_numPublicSlots = 2;
        Multiplayer::GridMateServiceParams serviceParams(sessionParams, FetchLANParam);

        Multiplayer::GridMateLANServiceWrapper gmLANService;
        GridMate::CarrierDesc carrierDesc;
        auto hostSession = gmLANService.CreateServer(GetGridMate(), carrierDesc, serviceParams);
        auto search = gmLANService.ListServers(GetGridMate(), serviceParams);

        int i = 0;
        const int kNumTimes = 100;
        for (; i < kNumTimes; ++i)
        {
            GetGridMate()->Update();
            if (search->GetNumResults() > 0)
            {
                break;
            }
        }
        EXPECT_TRUE(i < kNumTimes);

        gmLANService.StopSessionService(GetGridMate());
    }
};

TEST_F(GridMateServiceWrapperTest, Test)
{
    run();
}

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
class MultiplayerGameLiftClientTest
    : public UnitTest::AllocatorsFixture
{
public:
    MultiplayerGameLiftClientTest()
    {
        m_gameLiftParams.m_useGameLiftLocalServer = false;
    }

    void SetUp() override
    {
        AllocatorsFixture::SetUp();
        m_console = AZStd::make_unique<testing::NiceMock<UnitTest::MockConsole>>();
        gEnv->pConsole = m_console.get();

        ON_CALL(m_system, GetIConsole()).WillByDefault(testing::Return(gEnv->pConsole));
        ON_CALL(m_system, GetINetwork()).WillByDefault(testing::Return(gEnv->pNetwork));

        m_gameLiftClientServiceBus.Start(AllocatorsFixture::GetGridMate());
    }

    void TearDown() override
    {
        m_gameLiftClientServiceBus.Stop();

        m_console.reset();
        gEnv->pConsole = nullptr;
        AllocatorsFixture::TearDown();
    }

protected:
    void ApplyCVars()
    {
        // Setting params used in Multiplayer::Utils
        m_console->RegisterCVar("cl_clientport", m_clientPort);
        m_console->RegisterCVar("gm_securityData", m_securityData.c_str());
        m_console->RegisterCVar("gm_ipversion", m_ipVersion.c_str());
        m_console->RegisterCVar("gm_version", m_version.c_str());
        m_console->RegisterCVar("gm_disconnectDetection", m_disconnectDetection);

        // Gamelift specific CVars
        m_console->RegisterCVar("gamelift_aws_access_key", m_gameLiftParams.m_accessKey.c_str());
        m_console->RegisterCVar("gamelift_aws_secret_key", m_gameLiftParams.m_secretKey.c_str());
        m_console->RegisterCVar("gamelift_fleet_id", m_gameLiftParams.m_fleetId.c_str());
        m_console->RegisterCVar("gamelift_endpoint", m_gameLiftParams.m_endpoint.c_str());
        m_console->RegisterCVar("gamelift_aws_region", m_gameLiftParams.m_region.c_str());
        m_console->RegisterCVar("gamelift_alias_id", m_gameLiftParams.m_aliasId.c_str());
        m_console->RegisterCVar("gamelift_player_id", m_gameLiftParams.m_playerId.c_str());
        m_console->RegisterCVar("gamelift_uselocalserver", m_gameLiftParams.m_useGameLiftLocalServer);
    }

    testing::StrictMock<UnitTest::MockGameLiftRequestBus> m_gameLiftRequestBus;
    testing::StrictMock<UnitTest::MockGameLiftClientServiceBus> m_gameLiftClientServiceBus;
    testing::StrictMock<UnitTest::MockMultiplayerRequestBus> m_multiplayerRequestBus;

    // System and console are "nice mocks" because testing their calls explicity is not
    // the aim of this test fixture. Rather, they provide the minimum level of functionality
    // needed to ensure proper functioning of the MultiplayerGameLiftClient and its dependencies.
    testing::NiceMock<UnitTest::MockSystem> m_system;
    AZStd::unique_ptr<testing::NiceMock<UnitTest::MockConsole>> m_console;

    GridMate::GameLiftClientServiceDesc m_gameLiftParams;
    int m_clientPort = 0;
    AZStd::string m_securityData = "";
    AZStd::string m_ipVersion = "IPV4";
    AZStd::string m_version = "";
    int m_disconnectDetection = 0;
};

MATCHER_P3(GameLiftRequestMatch, serverName, mapName, numPlayers, "")
{
    return serverName == arg.m_instanceName
        && numPlayers == arg.m_numPublicSlots
        && 2 == arg.m_numParams
        && "sv_name" == arg.m_params[0].m_id
        && serverName == arg.m_params[0].m_value
        && "sv_map" == arg.m_params[1].m_id
        && mapName == arg.m_params[1].m_value;
}

TEST_F(MultiplayerGameLiftClientTest, GameLiftClient_HostSession)
{
    using testing::_;
    using testing::Invoke;

    ApplyCVars();

    // Start process to host a session on game lift and verify that the client service is started
    EXPECT_CALL(m_multiplayerRequestBus, GetSession()).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, IsNetSecEnabled()).Times(1);
    EXPECT_CALL(m_gameLiftRequestBus, StartClientService(_)).Times(1);
    Multiplayer::MultiplayerGameLiftClient gameLiftClient;
    gameLiftClient.HostGameLiftSession("testServer", "testMap", 12);

    // Simulate successful service start, results in client requesting a session
    EXPECT_CALL(m_gameLiftClientServiceBus, RequestSession(GameLiftRequestMatch("testServer", "testMap", 12))).Times(1);
    GridMate::GameLiftClientServiceEventsBus::Broadcast(
        &GridMate::GameLiftClientServiceEventsBus::Events::OnGameLiftSessionServiceReady, nullptr);

    // Simulate search result complete; this causes the client to join the session and register it
    auto& gameLiftSearch = *m_gameLiftClientServiceBus.m_search;
    EXPECT_CALL(gameLiftSearch, GetNumResults()).Times(1);
    EXPECT_CALL(gameLiftSearch, GetResult(0)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, GetSession()).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, JoinSessionBySearchInfo(_, _)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, GetSimulator()).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, IsNetSecEnabled()).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(testing::NotNull())).Times(1);
    m_gameLiftClientServiceBus.m_search->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_gameLiftClientServiceBus.m_search);

}

TEST_F(MultiplayerGameLiftClientTest, GameLiftClient_JoinSession)
{
    using testing::_;
    using testing::Invoke;

    ApplyCVars();

    // Start process to host a session on game lift and verify that the client service is started
    EXPECT_CALL(m_multiplayerRequestBus, GetSession()).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, IsNetSecEnabled()).Times(1);
    EXPECT_CALL(m_gameLiftRequestBus, StartClientService(_)).Times(1);
    Multiplayer::MultiplayerGameLiftClient gameLiftClient;
    gameLiftClient.JoinGameLiftSession();

    // Simulate successful service start, results in client querying for list of sessions
    EXPECT_CALL(m_gameLiftClientServiceBus, StartSearch(_)).Times(1);
    GridMate::GameLiftClientServiceEventsBus::Broadcast(
        &GridMate::GameLiftClientServiceEventsBus::Events::OnGameLiftSessionServiceReady, nullptr);

    // Simulate search result complete; this cause the client to join the session and register it
    auto& gameLiftSearch = *m_gameLiftClientServiceBus.m_search;
    EXPECT_CALL(gameLiftSearch, GetNumResults()).Times(1);
    EXPECT_CALL(gameLiftSearch, GetResult(0)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, GetSession()).Times(1);
    EXPECT_CALL(m_gameLiftClientServiceBus, JoinSessionBySearchInfo(_, _)).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, GetSimulator()).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, IsNetSecEnabled()).Times(1);
    EXPECT_CALL(m_multiplayerRequestBus, RegisterSession(testing::NotNull())).Times(1);
    m_gameLiftClientServiceBus.m_search->AddSearchResult();
    GridMate::SessionEventBus::Broadcast(
        &GridMate::SessionEventBus::Events::OnGridSearchComplete, m_gameLiftClientServiceBus.m_search);
}
#endif

namespace LuaTesting
{
    //////////////////////////////////////////////////////////////////////////
    // helper methods/classes

    static void RunLuaScript(uint32_t numTicks, AZStd::function<const char*(AZ::BehaviorContext*)> onSetUp, AZStd::function<bool()> onUpdate, AZStd::function<void()> onTearDown)
    {
        AZ::ComponentApplication app;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
        appDesc.m_stackRecordLevels = 10;
        AZ::Entity* systemEntity = app.Create(appDesc);

        systemEntity->CreateComponent<AZ::MemoryComponent>();
        systemEntity->CreateComponent("{CAE3A025-FAC9-4537-B39E-0A800A2326DF}"); // JobManager component
        systemEntity->CreateComponent<AZ::StreamerComponent>();
        systemEntity->CreateComponent<AZ::AssetManagerComponent>();
        systemEntity->CreateComponent("{A316662A-6C3E-43E6-BC61-4B375D0D83B4}"); // Usersettings component
        systemEntity->CreateComponent("{22FC6380-C34F-4a59-86B4-21C0276BCEE3}"); // ObjectStream component
        systemEntity->CreateComponent<AZ::ScriptSystemComponent>();

        systemEntity->Init();
        systemEntity->Activate();

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

        AZStd::string script(onSetUp(behaviorContext));

        Multiplayer::MultiplayerEventsComponent::Reflect(behaviorContext);

        AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = AZ::Data::AssetManager::Instance().CreateAsset<AZ::ScriptAsset>(AZ::Uuid::CreateRandom());
        // put the script into the script asset by all means including a const_cast<>!
        AZStd::vector<char>& buffer = const_cast<AZStd::vector<char>&>(scriptAsset.Get()->GetScriptBuffer());
        buffer.assign(script.begin(), script.end());

        AZ::Data::AssetManagerBus::Broadcast(&AZ::Data::AssetManagerBus::Events::OnAssetReady, scriptAsset);
        app.TickSystem();

        AZ::Entity* entity = aznew AZ::Entity();
        entity->CreateComponent<AzFramework::ScriptComponent>()->SetScript(scriptAsset);
        entity->Init();
        entity->Activate();

        while (numTicks--)
        {
            app.TickSystem();
            if (onUpdate())
            {
                // all done?
                break;
            }
        }

        delete entity;
        AZStd::string().swap(script);
        AZStd::vector<char>().swap(buffer);
        scriptAsset.Release();
        scriptAsset = nullptr;

        onTearDown();
        app.Destroy();
    }

    static GridMate::GridSessionParam FetchLANParam(const char* param)
    {
        GridMate::GridSessionParam p;

        if (!strcmp(param, "cl_clientport"))
        {
            p.SetValue(8080);
        }
        else if (!strcmp(param, "gm_ipversion"))
        {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MULTIPLAYERTEST_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(MultiplayerTest_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            p.SetValue(GridMate::Driver::BSD_AF_INET);
#endif
        }

        return p;
    }

    /**
    * an event bus for the networking tests to emit events to Lua scripts
    */
    class LuaNetworkTestingEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~LuaNetworkTestingEvents() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef AZ::EntityId BusIdType;
        //////////////////////////////////////////////////////////////////////////

        // Sink events
        virtual void OnTestEvent(const AZStd::string& name, const AZStd::string& data) = 0;
    };
    using LuaNetworkTestingBus = AZ::EBus<LuaNetworkTestingEvents>;

    class LuaNetworkTestingBusHandler
        : public LuaNetworkTestingBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(LuaNetworkTestingBusHandler, "{35EEFAD9-E9E4-46DE-95DC-E253F247726D}", AZ::SystemAllocator
            , OnTestEvent
        );

        void OnTestEvent(const AZStd::string& name, const AZStd::string& data) override
        {
            Call(FN_OnTestEvent, name, data);
        }

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext);
            if (behaviorContext)
            {
                behaviorContext->EBus<LuaNetworkTestingBus>("LuaNetworkTestingBus")
                    ->Handler<LuaNetworkTestingBusHandler>()
                    ;
            }
        }
    };


    //////////////////////////////////////////////////////////////////////////
    // GridMateLuaTesting

    class GridMateLuaTesting
        : public UnitTest::AllocatorsFixture
    {
    public:

        const char* k_LuaScript;

        GridMateLuaTesting()
            : UnitTest::AllocatorsFixture(false)
            , k_LuaScript(nullptr)
        {
            k_LuaScript = R"(
local testlua =
{
}

function testlua:OnActivate()
	local desc = SessionDesc();
	desc.gamePort = 3333;
	desc.mapName = "foo";
	desc.serverName = "bar";
	desc.maxPlayerSlots = 4;
	desc.serviceType = GridServiceType.LAN;
	desc.enableDisconnectDetection = true;
	desc.connectionTimeoutMS = 499;
	desc.threadUpdateTimeMS = 51;

	self.sessionManager = SessionManagerBus.Connect(self, self.entityId);
    SessionManagerBus.Event.StartHost(self.entityId, desc);
end

function testlua:OnDeactivate()
    SessionManagerBus.Event.Close(self.entityId);
    self.sessionManager:Disconnect()
end

return testlua;
)";
        }

        void run()
        {
            AZ::ComponentApplication app;
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_stackRecordLevels = 10;
            AZ::Entity* systemEntity = app.Create(appDesc);
            GetGridMate();

            systemEntity->CreateComponent<AZ::MemoryComponent>();
            systemEntity->CreateComponent("{CAE3A025-FAC9-4537-B39E-0A800A2326DF}"); // JobManager component
            systemEntity->CreateComponent<AZ::StreamerComponent>();
            systemEntity->CreateComponent<AZ::AssetManagerComponent>();
            systemEntity->CreateComponent("{A316662A-6C3E-43E6-BC61-4B375D0D83B4}"); // Usersettings component
            systemEntity->CreateComponent("{22FC6380-C34F-4a59-86B4-21C0276BCEE3}"); // ObjectStream component
            systemEntity->CreateComponent<AZ::ScriptSystemComponent>();

            systemEntity->Init();
            systemEntity->Activate();

            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

            Multiplayer::MultiplayerEventsComponent::Reflect(behaviorContext);

            AZStd::string script(k_LuaScript);

            AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = AZ::Data::AssetManager::Instance().CreateAsset<AZ::ScriptAsset>(AZ::Uuid::CreateRandom());
            // put the script into the script asset by all means including a const_cast<>!
            AZStd::vector<char>& buffer = const_cast<AZStd::vector<char>&>(scriptAsset.Get()->GetScriptBuffer());
            buffer.assign(script.begin(), script.end());

            AZ::Data::AssetManagerBus::Broadcast(&AZ::Data::AssetManagerBus::Events::OnAssetReady, scriptAsset);
            app.Tick();

            AZ::Entity* entity1 = aznew AZ::Entity();
            entity1->CreateComponent<AzFramework::ScriptComponent>()->SetScript(scriptAsset);
            entity1->Init();
            entity1->Activate();

            delete entity1;
            AZStd::string().swap(script);
            AZStd::vector<char>().swap(buffer);
            scriptAsset.Release();
            scriptAsset = nullptr;

            TearDown();
            app.Destroy();
        }
    };

    TEST_F(GridMateLuaTesting, GridMateLua_Testing)
    {
        run();
    }

    //////////////////////////////////////////////////////////////////////////
    // very basic Lua test that starts and shuts down a GridSearch

    class GridMateLuaSearchTesting
        : public UnitTest::AllocatorsFixture
    {
    public:

        GridMateLuaSearchTesting()
            : UnitTest::AllocatorsFixture(false)
        {
        }
        void run()
        {
            auto setup = [&](AZ::BehaviorContext*)
            {
                GetGridMate();
                return R"(
                            local testlua =
                            {
                            }

                            function testlua:OnActivate()
	                            local desc = SessionDesc();
	                            desc.gamePort = 3333;
	                            desc.mapName = "foo";
	                            desc.serverName = "bar";
	                            desc.maxPlayerSlots = 4;
	                            desc.serviceType = GridServiceType.LAN;
	                            desc.enableDisconnectDetection = true;
	                            desc.connectionTimeoutMS = 499;
	                            desc.threadUpdateTimeMS = 51;

	                            self.searchManager = GridSearchBusHandler.Connect(self, self.entityId);
                                self.ticket = GridSearchBusHandler.Event.StartSearch(self.entityId, desc);
                            end

                            function testlua:OnDeactivate()
                                GridSearchBusHandler.Event.StopSearch(self.entityId, self.ticket);
                                self.searchManager:Disconnect()
                            end

                            return testlua;
                            )";
            };

            auto update = [&]()
            {
                GetGridMate()->Update();
                return false;
            };

            auto teardown = [&]()
            {
                TearDown();
            };

            RunLuaScript(3, setup, update, teardown);
        }
    };

    TEST_F(GridMateLuaSearchTesting, GridMateLua_SearchTesting)
    {
        run();
    }

    //////////////////////////////////////////////////////////////////////////
    // finds one Grid SearchInfo after a few tries

    class GridMateLuaListSesionsTesting
        : public UnitTest::AllocatorsFixture
    {
    public:

        static AZStd::atomic_int s_count;
        const char* k_LuaScript;

        GridMateLuaListSesionsTesting()
            : UnitTest::AllocatorsFixture(false)
            , k_LuaScript(nullptr)
        {
            k_LuaScript = R"(
local testlua =
{
}

function testlua:OnActivate()
	local desc = SessionDesc();
	desc.gamePort = 8080;
	desc.serviceType = GridServiceType.LAN;

	self.searchManager = GridSearchBusHandler.Connect(self, self.entityId);
    self.ticket = GridSearchBusHandler.Event.StartSearch(self.entityId, desc);
end

function testlua:OnDeactivate()
    GridSearchBusHandler.Event.StopSearch(self.entityId, self.ticket);
    self.searchManager:Disconnect()
end

function testlua:OnSearchInfo(info)
    PingFlag();
end

return testlua;
)";
        }

        void run()
        {
            Multiplayer::GridMateLANServiceWrapper gmLANService;

            GridMate::SessionParams sessionParams;
            sessionParams.m_topology = GridMate::ST_CLIENT_SERVER;
            sessionParams.m_numPublicSlots = 2;
            Multiplayer::GridMateServiceParams serviceParams(sessionParams, FetchLANParam);
            GridMate::CarrierDesc carrierDesc;

            auto setup = [&](AZ::BehaviorContext* bc)
            {
                GetGridMate();

                // used to assert the event happened
                GridMateLuaListSesionsTesting::s_count = 0;
                static const auto fnPingFlag = []()
                {
                    ++GridMateLuaListSesionsTesting::s_count;
                };
                bc->Method("PingFlag", fnPingFlag);

                gmLANService.CreateServer(GetGridMate(), carrierDesc, serviceParams);

                return k_LuaScript;
            };

            auto update = [&]()
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                GetGridMate()->Update();
                return GridMateLuaListSesionsTesting::s_count > 0;
            };

            auto teardown = [&]()
            {
                gmLANService.StopSessionService(GetGridMate());
                TearDown();
            };

            // it can take an average of 2000ms to actually register as session, so we give it up to 500x10ms here.
            RunLuaScript(400, setup, update, teardown);
            AZ_TEST_ASSERT(GridMateLuaListSesionsTesting::s_count > 0);
        }
    };
    AZStd::atomic_int GridMateLuaListSesionsTesting::s_count;

    TEST_F(GridMateLuaListSesionsTesting, GridMateLua_ListSesionsTesting)
    {
        run();
    }

    //////////////////////////////////////////////////////////////////////////
    //

    class GridMateLuaHostListFindAndJoinTesting
        : public UnitTest::AllocatorsFixture
    {
    public:

        struct TestGridMateSessionEventBusHandler
            : public GridMate::SessionEventBus::Handler
        {
            int& m_Joined;
            int& m_Left;
            GridMate::IGridMate* m_gridMate;
            Multiplayer::GridMateLANServiceWrapper gmLANService;
            UnitTest::TestingNetworkProcessor m_processor;

            TestGridMateSessionEventBusHandler(int& joined, int& left)
                : m_Joined(joined)
                , m_Left(left)
                , m_gridMate(nullptr)
            {
            }

            void Start()
            {
                m_gridMate = GridMate::GridMateCreate(GridMate::GridMateDesc());
                m_processor.SetGridMate(m_gridMate);

                BusConnect(m_gridMate);

                GridMate::SessionParams sessionParams;
                sessionParams.m_topology = GridMate::ST_CLIENT_SERVER;
                sessionParams.m_numPublicSlots = 2;
                Multiplayer::GridMateServiceParams serviceParams(sessionParams, FetchLANParam);
                GridMate::CarrierDesc carrierDesc;

                gmLANService.CreateServer(m_gridMate, carrierDesc, serviceParams);
            }
            void Update()
            {
                m_processor.Update();
            }
            void Stop()
            {
                m_processor.Reset();
                BusDisconnect();
                gmLANService.StopSessionService(m_gridMate);
                GridMate::GridMateDestroy(m_gridMate);
                m_gridMate = nullptr;
            }
            void OnMemberJoined(GridMate::GridSession* session, GridMate::GridMember* member) override
            {
                (void)session;
                (void)member;
                m_Joined++;
            }
            void OnMemberLeaving(GridMate::GridSession* session, GridMate::GridMember* member) override
            {
                (void)session;
                (void)member;
                m_Left++;
            }
        };

        const char* k_LuaScript;

        GridMateLuaHostListFindAndJoinTesting()
            : UnitTest::AllocatorsFixture(false)
            , k_LuaScript(nullptr)
        {
            k_LuaScript = R"(
local testlua =
{
}

function testlua:OnActivate()
	local desc = SessionDesc();
	desc.gamePort = 8080;
	desc.serviceType = GridServiceType.LAN;

    self.testingBus = LuaNetworkTestingBus.Connect(self, self.entityId);
	self.searchManager = GridSearchBusHandler.Connect(self, self.entityId);
    self.ticket = GridSearchBusHandler.Event.StartSearch(self.entityId, desc);
end

function testlua:OnDeactivate()
    Debug:Log("OnDeactivate \n");
    GridSearchBusHandler.Event.StopSearch(self.entityId, self.ticket);
    self.searchManager:Disconnect();
    self.testingBus:Disconnect();
end

function testlua:OnSearchInfo(info)
    Debug:Log("OnSearchInfo \n");
    GridSearchBusHandler.Event.JoinSession(self.entityId, info);
end

function testlua:OnJoinComplete(session)
    self.session = session;
end

function testlua:OnTestEvent(name, data)
    if self.session ~= nil then
        Debug:Log("OnEvent \n");
        self.session:Leave(0);
        self.session = nil;
    end
end

return testlua;
)";
        }

        void run()
        {
            int numMembersAdded = 0;
            int numMembersLeft = 0;
            TestGridMateSessionEventBusHandler testGridMateSessionEventBusHandler(numMembersAdded, numMembersLeft);
            UnitTest::TestingNetworkProcessor clientProcessor;

            auto setup = [&](AZ::BehaviorContext* bc)
            {
                clientProcessor.SetGridMate(GetGridMate());
                testGridMateSessionEventBusHandler.Start();
                LuaNetworkTestingBusHandler::Reflect(bc);
                return k_LuaScript;
            };

            auto update = [&]()
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(25));
                testGridMateSessionEventBusHandler.Update();
                clientProcessor.Update();
                if (numMembersAdded > 1)
                {
                    if (numMembersLeft > 0)
                    {
                        // all done
                        return true;
                    }
                    EBUS_EVENT(LuaNetworkTestingBus, OnTestEvent, "connected", "1");
                }
                return false;
            };

            auto teardown = [&]()
            {
                clientProcessor.Reset();
                testGridMateSessionEventBusHandler.Stop();
                TearDown();
            };

            RunLuaScript(200, setup, update, teardown);
            AZ_TEST_ASSERT(numMembersAdded > 0);
            AZ_TEST_ASSERT(numMembersLeft > 0);
        }
    };

    TEST_F(GridMateLuaHostListFindAndJoinTesting, GridMateLua_HostListFindAndJoinTesting)
    {
        run();
    }
}

AZ_UNIT_TEST_HOOK();
