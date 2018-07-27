#pragma once

#include <GameLift/GameLiftBus.h>
#include <GameLift/Session/GameLiftClientServiceBus.h>
#include <Multiplayer/IMultiplayerGem.h>
#include <GameLift/Session/GameLiftSearch.h>
#include <Mocks/ISystemMock.h>
#include <Mocks/IConsoleMock.h>
#include <SFunctor.h>

#include <gmock/gmock.h>

// Warning 4373 can occur based on how gmock creates stubs
#pragma warning(push)
#pragma warning(disable:4373)

namespace UnitTest
{
    using testing::_;
    using testing::Invoke;
    using testing::Return;

    class MockGameLiftRequestBus
        : public GameLift::GameLiftRequestBus::Handler
    {
    public:
        MockGameLiftRequestBus()
        {
            GameLift::GameLiftRequestBus::Handler::BusConnect();
        }

        ~MockGameLiftRequestBus()
        {
            GameLift::GameLiftRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(IsGameLiftServer, bool());

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
        MOCK_METHOD1(StartClientService, GridMate::GameLiftClientService*(const GridMate::GameLiftClientServiceDesc& desc));
        MOCK_METHOD0(StopClientService, void());
        MOCK_METHOD0(GetClientService, GridMate::GameLiftClientService*());
#endif

#if BUILD_GAMELIFT_SERVER
        MOCK_METHOD1(StartServerService, GridMate::GameLiftServerService*(const GridMate::GameLiftServerServiceDesc& desc));
        MOCK_METHOD0(StopServerService, void());
        MOCK_METHOD0(GetServerService, GridMate::GameLiftServerService*());
#endif
    };

    class MockSession
        : public GridMate::GridSession
    {
    public:
        MockSession(GridMate::SessionService* service)
            : GridSession(service)
        {
        }

        MOCK_METHOD4(CreateRemoteMember, GridMate::GridMember*(const GridMate::string&, GridMate::ReadBuffer&, GridMate::RemotePeerMode, GridMate::ConnectionID));
        MOCK_METHOD1(OnSessionParamChanged, void(const GridMate::GridSessionParam&));
        MOCK_METHOD1(OnSessionParamRemoved, void(const GridMate::string&));
    };

    class MockSessionService
        : public GridMate::SessionService
    {
    public:
        MockSessionService()
            : SessionService(GridMate::SessionServiceDesc())
        {
        }

        ~MockSessionService()
        {
            m_activeSearches.clear();
            m_gridMate = nullptr;
        }

        MOCK_CONST_METHOD0(IsReady, bool());
    };

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
    class MockGameLiftSearch
        : public GridMate::GridSearch
    {
    public:
        GM_CLASS_ALLOCATOR(MockGameLiftSearch);

        MockGameLiftSearch(GridMate::SessionService* sessionService)
            : GridSearch(sessionService)
        {
            ON_CALL(*this, GetNumResults()).WillByDefault(Invoke([this]() { return m_results.size(); }));
            ON_CALL(*this, GetResult(_)).WillByDefault(Invoke(
                [this](unsigned int index) -> const GridMate::SearchInfo*
            {
                return &m_results[index];
            }));
        }

        void AddSearchResult()
        {
            m_results.push_back();
        }

        MOCK_CONST_METHOD0(GetNumResults, unsigned int());
        MOCK_CONST_METHOD1(GetResult, const GridMate::SearchInfo*(unsigned int index));
        MOCK_METHOD0(AbortSearch, void());

        GridMate::vector<GridMate::GameLiftSearchInfo> m_results;
    };

    class MockGameLiftClientServiceBus
        : public GridMate::GameLiftClientServiceBus::Handler
    {
    public:
        MockGameLiftClientServiceBus()
        {
            ON_CALL(*this, JoinSessionBySearchInfo(_, _)).WillByDefault(Invoke(this, &MockGameLiftClientServiceBus::DefaultJoinSessionBySearchInfo));
            ON_CALL(*this, RequestSession(_)).WillByDefault(Invoke(this, &MockGameLiftClientServiceBus::DefaultRequestSession));
            ON_CALL(*this, StartSearch(_)).WillByDefault(Invoke(this, &MockGameLiftClientServiceBus::DefaultStartSearch));
        }

        MOCK_METHOD2(JoinSessionBySearchInfo, GridMate::GridSession*(const GridMate::GameLiftSearchInfo& params, const GridMate::CarrierDesc& carrierDesc));
        MOCK_METHOD1(RequestSession, GridMate::GameLiftSessionRequest*(const GridMate::GameLiftSessionRequestParams& params));
        MOCK_METHOD1(StartSearch, GridMate::GameLiftSearch*(const GridMate::GameLiftSearchParams& params));
        MOCK_METHOD1(QueryGameLiftSession, GridMate::GameLiftClientSession*(const GridMate::GridSession* session));
        MOCK_METHOD1(QueryGameLiftSearch, GridMate::GameLiftSearch*(const GridMate::GridSearch* search));

        void Start(GridMate::IGridMate* gridMate)
        {
            m_sessionService = AZStd::make_unique<MockSessionService>();
            GridMate::GameLiftClientServiceBus::Handler::BusConnect(gridMate);
        }

        void Stop()
        {
            m_sessionService.reset();
            GridMate::GameLiftClientServiceBus::Handler::BusDisconnect();
        }

        GridMate::GridSession* DefaultJoinSessionBySearchInfo(const GridMate::GameLiftSearchInfo& params, const GridMate::CarrierDesc& carrierDesc)
        {
            m_session = AZStd::make_unique<MockSession>(m_sessionService.get());
            return m_session.get();
        }

        GridMate::GameLiftSessionRequest* DefaultRequestSession(const GridMate::GameLiftSessionRequestParams& params)
        {
            EXPECT_EQ(nullptr, m_search);

            m_search = aznew MockGameLiftSearch(m_sessionService.get());
            return reinterpret_cast<GridMate::GameLiftSessionRequest*>(m_search);
        }

        GridMate::GameLiftSearch* DefaultStartSearch(const GridMate::GameLiftSearchParams& params)
        {
            EXPECT_EQ(nullptr, m_search);

            m_search = aznew MockGameLiftSearch(m_sessionService.get());
            return reinterpret_cast<GridMate::GameLiftSearch*>(m_search);
        }

        MockGameLiftSearch* m_search = nullptr;

        AZStd::unique_ptr<MockSession> m_session;

        AZStd::unique_ptr<MockSessionService> m_sessionService;
    };
#endif

    class MockMultiplayerRequestBus
        : public Multiplayer::MultiplayerRequestBus::Handler
    {
    public:
        MockMultiplayerRequestBus()
        {
            ON_CALL(*this, GetSession()).WillByDefault(Return(m_session));
            ON_CALL(*this, RegisterSession(_)).WillByDefault(Invoke(this, &MockMultiplayerRequestBus::DefaultRegisterSession));
            Multiplayer::MultiplayerRequestBus::Handler::BusConnect();
        }

        ~MockMultiplayerRequestBus()
        {
            Multiplayer::MultiplayerRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(IsNetSecEnabled, bool());
        MOCK_CONST_METHOD0(IsNetSecVerifyClient, bool());
        MOCK_METHOD1(RegisterSecureDriver, void(GridMate::SecureSocketDriver* driver));
        MOCK_METHOD0(GetSession, GridMate::GridSession*());
        MOCK_METHOD1(RegisterSession, void(GridMate::GridSession* gridSession));
        MOCK_METHOD0(GetSimulator, GridMate::Simulator*());
        MOCK_METHOD0(EnableSimulator, void());
        MOCK_METHOD0(DisableSimulator, void());

        void DefaultRegisterSession(GridMate::GridSession* gridSession)
        {
            m_session = gridSession;
        }

        GridMate::GridSession* m_session = nullptr;
    };

    class MockCVar : public ICVar {
    public:
        MockCVar() {}
        MockCVar(const char* name, const char* value) : m_name(name), m_type(CVAR_STRING), m_strVal(value) { InitDefaultBehavior(); }
        MockCVar(const char* name, int value) : m_name(name), m_type(CVAR_INT), m_intVal(value) { InitDefaultBehavior(); }
        MockCVar(const char* name, int64 value) : m_name(name), m_type(CVAR_INT), m_intVal(value) { InitDefaultBehavior(); }
        MockCVar(const char* name, float value) : m_name(name), m_type(CVAR_FLOAT), m_floatVal(value) { InitDefaultBehavior(); }

        void InitDefaultBehavior()
        {
            using testing::Return;

            ON_CALL(*this, GetIVal()).WillByDefault(Return(static_cast<int>(m_intVal)));
            ON_CALL(*this, GetI64Val()).WillByDefault(Return(m_intVal));
            ON_CALL(*this, GetFVal()).WillByDefault(Return(m_floatVal));
            ON_CALL(*this, GetString()).WillByDefault(Return(m_strVal.c_str()));
        }

        MOCK_METHOD0(Release, void());
        MOCK_CONST_METHOD0(GetIVal, int());
        MOCK_CONST_METHOD0(GetI64Val, int64());
        MOCK_CONST_METHOD0(GetFVal, float());
        MOCK_CONST_METHOD0(GetString, const char*());
        MOCK_CONST_METHOD0(GetDataProbeString, const char*());
        MOCK_METHOD0(Reset, void());
        MOCK_METHOD1(Set, void(const char* s));
        MOCK_METHOD1(ForceSet, void(const char* s));
        MOCK_METHOD1(Set, void(const float f));
        MOCK_METHOD1(Set, void(const int i));
        MOCK_METHOD1(ClearFlags, void(int flags));
        MOCK_CONST_METHOD0(GetFlags, int());
        MOCK_METHOD1(SetFlags, int(int flags));
        MOCK_METHOD0(GetType, int());
        MOCK_CONST_METHOD0(GetName, const char*());
        MOCK_METHOD0(GetHelp, const char*());
        MOCK_CONST_METHOD0(IsConstCVar, bool());
        MOCK_METHOD1(SetOnChangeCallback, void(ConsoleVarFunc pChangeFunc));
        MOCK_METHOD1(AddOnChangeFunctor, uint64(const SFunctor& pChangeFunctor));
        MOCK_CONST_METHOD0(GetNumberOfOnChangeFunctors, uint64());
        MOCK_CONST_METHOD1(GetOnChangeFunctor, const SFunctor&(uint64 nFunctorId));
        MOCK_METHOD1(RemoveOnChangeFunctor, bool(const uint64 nFunctorId));
        MOCK_CONST_METHOD0(GetOnChangeCallback, ConsoleVarFunc());
        MOCK_CONST_METHOD1(GetMemoryUsage, void(class ICrySizer* pSizer));
        MOCK_CONST_METHOD0(GetRealIVal, int());
        MOCK_METHOD2(SetLimits, void(float min, float max));
        MOCK_METHOD2(GetLimits, void(float& min, float& max));
        MOCK_METHOD0(HasCustomLimits, bool());
        MOCK_CONST_METHOD2(DebugLog, void(const int iExpectedValue, const EConsoleLogMode mode));
#if defined(DEDICATED_SERVER)
        MOCK_METHOD1(SetDataProbeString, void(const char* pDataProbeString));
#endif

        AZStd::string m_name;
        int m_type = 0;
        int64 m_intVal = 0;
        float m_floatVal = 0;
        AZStd::string m_strVal;
    };

    class MockConsole : public ConsoleMock {
    public:
        MockConsole()
        {
            ON_CALL(*this, RegisterString(_, _, _, _, _)).WillByDefault(Invoke(this, &MockConsole::RegisterCVar<const char*>));
            ON_CALL(*this, RegisterInt(_, _, _, _, _)).WillByDefault(Invoke(this, &MockConsole::RegisterCVar<int>));
            ON_CALL(*this, RegisterInt64(_, _, _, _, _)).WillByDefault(Invoke(this, &MockConsole::RegisterCVar<int64>));
            ON_CALL(*this, RegisterFloat(_, _, _, _, _)).WillByDefault(Invoke(this, &MockConsole::RegisterCVar<float>));

            ON_CALL(*this, GetCVar(_)).WillByDefault(Invoke(
                [this](const char* name) -> ICVar*
                {
                    auto it = m_cvars.find(name);
                    if (it == m_cvars.end())
                    {
                        return nullptr;
                    }

                    return it->second.get();
                }));
        }

        template <class T>
        ICVar* RegisterCVar(const char* name, T value, int flags = 0, const char* help = "", ConsoleVarFunc changeFunc = nullptr)
        {
            auto inserted = m_cvars.emplace(name, AZStd::make_unique<testing::NiceMock<MockCVar>>(name, value));
            return inserted.first->second.get();
        }

        using MockCVarContainer = AZStd::unordered_map<AZStd::string, AZStd::unique_ptr<MockCVar>>;
        MockCVarContainer m_cvars;
    };

    class MockSystem
        : public SystemMock
        , public CrySystemRequestBus::Handler
    {
    public:
        MockSystem()
        {
            CrySystemRequestBus::Handler::BusConnect();
        }

        ~MockSystem()
        {
            CrySystemRequestBus::Handler::BusDisconnect();
        }

        // CrySystemRequestBus override
        ISystem* GetCrySystem() override
        {
            return this;
        }
    };
} // namespace UnitTest

#pragma warning(pop)
