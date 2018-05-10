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
#include "Multiplayer/BehaviorContext/GridSystemContext.h"
#include "Multiplayer/BehaviorContext/GridSearchContext.h"

#include <GridMate/NetworkGridMate.h>

#include "Multiplayer/GridMateServiceWrapper/GridMateLANServiceWrapper.h"

#if defined(AZ_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define GRIDSYSTEMCONTEXT_CPP_SECTION_1 1
#define GRIDSYSTEMCONTEXT_CPP_SECTION_2 2
#define GRIDSYSTEMCONTEXT_CPP_SECTION_3 3
#define GRIDSYSTEMCONTEXT_CPP_SECTION_4 4
#define GRIDSYSTEMCONTEXT_CPP_SECTION_5 5
#define GRIDSYSTEMCONTEXT_CPP_SECTION_6 6
#define GRIDSYSTEMCONTEXT_CPP_SECTION_7 7
#define GRIDSYSTEMCONTEXT_CPP_SECTION_8 8
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#include "Multiplayer/MultiplayerUtils.h"

namespace Multiplayer
{

    /**
     * used to capture SessionEventBus and MultiplayerLobbyBus bus events for this behavior to forward onto the SessionManagerHandler
     */
    struct SessionManagerHandlerImpl
        : public GridMate::SessionEventBus::Handler
    {
        SessionManagerHandlerImpl() 
            : m_gridMate(nullptr)
        {
            if (gEnv->pNetwork)
            {
                m_gridMate = gEnv->pNetwork->GetGridMate();
            }
        }
        ~SessionManagerHandlerImpl()
        {
            Disconnect();
        }
        void Connect(SessionDesc sessionDesc)
        {
            m_sessionDesc = sessionDesc;
            if (m_gridMate)
            {
                GridMate::SessionEventBus::Handler::BusConnect(m_gridMate);
            }
        }
        void Disconnect()
        {
            GridMate::SessionEventBus::Handler::BusDisconnect();
        }

        //
        // GridMate::SessionEventBus::Handler
        //
        void OnSessionCreated(GridMate::GridSession* session) override
        {
            (void)session;
        }
        void OnSessionStart(GridMate::GridSession* session) override
        {
            EBUS_EVENT(SessionManagerBus, OnHostSessionStarted, session);
        }
        void OnSessionEnd(GridMate::GridSession* session) override
        {
            (void)session;
        }
        void OnSessionDelete(GridMate::GridSession* session) override
        {
            (void)session;
        }
        void OnSessionError(GridMate::GridSession* session, const GridMate::string& errorMsg) override
        {
            (void)session;
            (void)errorMsg;
        }

        //
        // shared resource(s)
        //
        GridMate::IGridMate* GetGridMate() const
        {
            AZ_Assert(m_gridMate, "IGridMate missing.");
            return m_gridMate;
        }

    private:
        GridMate::IGridMate* m_gridMate;
        SessionDesc m_sessionDesc;
    };

    /** 
      * The high level bus manager for GridMate Lua behaviors
      */
    class SessionManagerHandler
        : public SessionManagerBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(SessionManagerHandler, "{97F049D6-1C49-4661-A88C-1AE63E0554B3}", AZ::SystemAllocator
            , StartHost
            , Close
            , OnHostSessionStarted
        );

        //
        // SessionManagerBus::Handler
        //
        bool StartHost(const SessionDesc& sessionDesc) override
        {
            m_sessionDesc = sessionDesc;

            Close();

            m_gridMateSessionHandler.Connect(m_sessionDesc);
            m_gridMateServiceWrapper.reset(GridMateSystemContext::RegisterServiceWrapper(m_sessionDesc.m_serviceType));
            if (m_gridMateServiceWrapper)
            {
                if (m_gridMateServiceWrapper->StartSessionService(m_gridMateSessionHandler.GetGridMate()))
                {
                    return CreateServerForWrappedService(sessionDesc);
                }
            }
            return false;
        }
        bool Close() override
        {
            // disable further EBus communications with GridMate::SessionEventBus::Handler or MultiplayerLobbyBus::Handler
            m_gridMateSessionHandler.Disconnect();

            // handle GridMate resources
            if (m_gridMateServiceWrapper)
            {
                m_gridMateServiceWrapper->StopSessionService(m_gridMateSessionHandler.GetGridMate());
                m_gridMateServiceWrapper.reset(nullptr);
                return true;
            }
            return false;
        }
        void OnHostSessionStarted(GridMate::GridSession* session) override
        {
            Call(FN_OnHostSessionStarted, session);
        }

    protected:
        void InitCarrierDesc(GridMate::CarrierDesc& carrierDesc, const GridMateServiceParams& gridMateServiceParams)
        {
            if (!carrierDesc.m_simulator)
            {
                EBUS_EVENT_RESULT(carrierDesc.m_simulator, Multiplayer::MultiplayerRequestBus, GetSimulator);
            }

            m_securityString = gridMateServiceParams.FetchString("gm_securityData");

            carrierDesc.m_port = gridMateServiceParams.FetchValueOrDefault<int>("cl_clientport", 0);
            carrierDesc.m_connectionTimeoutMS = 10000;
            carrierDesc.m_threadUpdateTimeMS = 30;
            carrierDesc.m_threadInstantResponse = true;
            carrierDesc.m_driverIsCrossPlatform = true;
            carrierDesc.m_securityData = m_securityString.c_str();
            carrierDesc.m_familyType = gridMateServiceParams.FetchValueOrDefault<int>("gm_ipversion", 1);
            carrierDesc.m_version = gridMateServiceParams.m_version;

            carrierDesc.m_enableDisconnectDetection = !!gridMateServiceParams.FetchValueOrDefault<int>("gm_disconnectDetection", 1);
            carrierDesc.m_disconnectDetectionRttThreshold = gridMateServiceParams.FetchValueOrDefault<float>("gm_disconnectDetectionRttThreshold", 500.0f);
            carrierDesc.m_disconnectDetectionPacketLossThreshold = gridMateServiceParams.FetchValueOrDefault<float>("gm_disconnectDetectionPacketLossThreshold", 0.3f);

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, AZ_RESTRICTED_PLATFORM)
#endif
        }

        bool CreateServerForWrappedService(const SessionDesc& sessionDesc)
        {
            GridMate::GridSession* gridSession = nullptr;
            EBUS_EVENT_RESULT(gridSession, Multiplayer::MultiplayerRequestBus, GetSession);

            if (!gridSession)
            {
                GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

                GridMate::SessionParams sessionParams;
                sessionParams.m_topology = GridMate::ST_CLIENT_SERVER;
                sessionParams.m_numPublicSlots = m_sessionDesc.m_maxPlayerSlots + (gEnv->IsDedicated() ? 1 : 0); // One slot for server member.
                sessionParams.m_numPrivateSlots = 0;
                sessionParams.m_peerToPeerTimeout = 60000;
                sessionParams.m_flags = 0;

                sessionParams.m_numParams = 0;
                sessionParams.m_params[sessionParams.m_numParams].m_id = "sv_name";
                sessionParams.m_params[sessionParams.m_numParams].SetValue(m_sessionDesc.m_serverName.c_str());
                sessionParams.m_numParams++;

                sessionParams.m_params[sessionParams.m_numParams].m_id = "sv_map";
                sessionParams.m_params[sessionParams.m_numParams].SetValue(m_sessionDesc.m_mapName.c_str());
                sessionParams.m_numParams++;

                std::string securityString;

                auto fetchParams = [&](const char* param)
                {
                    GridMate::GridSessionParam p;

                    if (!strcmp(param, "cl_clientport"))
                    {
                        p.SetValue(m_sessionDesc.m_gamePort);
                    }
                    else if (!strcmp(param, "gm_ipversion"))
                    {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                        p.SetValue(GridMate::Driver::BSD_AF_INET);
#endif
                    }
                    return p;
                };

                GridMateServiceParams gridMateServiceParams(sessionParams, fetchParams);

                GridMate::CarrierDesc carrierDesc;
                InitCarrierDesc(carrierDesc, gridMateServiceParams);
                Multiplayer::NetSec::ConfigureCarrierDescForHost(carrierDesc);

                carrierDesc.m_port = m_sessionDesc.m_gamePort;
                carrierDesc.m_enableDisconnectDetection = m_sessionDesc.m_enableDisconnectDetection;
                carrierDesc.m_connectionTimeoutMS = m_sessionDesc.m_connectionTimeoutMS;
                carrierDesc.m_threadUpdateTimeMS = m_sessionDesc.m_threadUpdateTimeMS;

                GridMate::GridSession* session = m_gridMateServiceWrapper->CreateServer(gridMate, carrierDesc, gridMateServiceParams);

                if (session == nullptr)
                {
                    Multiplayer::NetSec::OnSessionFailedToCreate(carrierDesc);
                    EBUS_EVENT(GridMate::SessionEventBus, OnSessionError, gridSession, "Error while hosting Session.");
                }
                else
                {
                    EBUS_EVENT(Multiplayer::MultiplayerRequestBus, RegisterSession, session);
                }
            }
            else
            {
                EBUS_EVENT(GridMate::SessionEventBus, OnSessionError, gridSession, "Invalid Gem Session");
                return false;
            }
            return true;
        }

        SessionDesc m_sessionDesc;
        SessionManagerHandlerImpl m_gridMateSessionHandler;
        AZStd::unique_ptr<GridMateServiceWrapper> m_gridMateServiceWrapper;
        AZStd::string m_securityString;
    };


    //////////////////////////////////////////////////////////////////////////
    // GridMateSystemContext
    namespace GridMateSystemContext
    {
        void Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
            if (serializeContext)
            {
                serializeContext->Class<SessionDesc>()
                    ->Version(1)
                    ->Field("Port", &SessionDesc::m_gamePort)
                    ->Field("MaxPlayerSlots", &SessionDesc::m_maxPlayerSlots)
                    ->Field("EnableDisconnectDetection", &SessionDesc::m_enableDisconnectDetection)
                    ->Field("ConnectionTimeoutMS", &SessionDesc::m_connectionTimeoutMS)
                    ->Field("ThreadUpdateTimeMS", &SessionDesc::m_threadUpdateTimeMS)
                    ->Field("MapName", &SessionDesc::m_mapName)
                    ->Field("ServerName", &SessionDesc::m_serverName)
                    ->Field("ServiceType", &SessionDesc::m_serviceType)
                ;
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext);
            if (behaviorContext)
            {
                behaviorContext->Class<GridMate::ServiceType>("GridServiceType")
                    ->Enum<(int)GridMate::ST_LAN>("LAN")
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XBONE)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_8
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, TOOLS_SUPPORT_XBONE)
#endif
#if defined(TOOLS_SUPPORT_PS4)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_8
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, TOOLS_SUPPORT_PS4)
#endif
#endif
                    ;

                // expose the parameters for a GridSession
                behaviorContext->Class<SessionDesc>("SessionDesc")
                    ->Property("gamePort", BehaviorValueProperty(&SessionDesc::m_gamePort))
                    ->Property("mapName", BehaviorValueProperty(&SessionDesc::m_mapName))
                    ->Property("maxPlayerSlots", BehaviorValueProperty(&SessionDesc::m_maxPlayerSlots))
                    ->Property("serverName", BehaviorValueProperty(&SessionDesc::m_serverName))
                    ->Property("enableDisconnectDetection", BehaviorValueProperty(&SessionDesc::m_enableDisconnectDetection))
                    ->Property("connectionTimeoutMS", BehaviorValueProperty(&SessionDesc::m_connectionTimeoutMS))
                    ->Property("threadUpdateTimeMS", BehaviorValueProperty(&SessionDesc::m_threadUpdateTimeMS))
                    ->Property("serviceType", [](SessionDesc* that) -> int { return that->m_serviceType; }, [](SessionDesc* that, int value) { that->m_serviceType = static_cast<GridMate::ServiceType>(value); });
                ;

                behaviorContext->EBus<SessionManagerBus>("SessionManagerBus")
                    ->Handler<SessionManagerHandler>()
                    ->Event("StartHost", &SessionManagerBus::Events::StartHost)
                    ->Event("Close", &SessionManagerBus::Events::Close)
                    ;

                GridSearchBehavior::Reflect(behaviorContext);
            }
        }

        // STATIC
        bool FetchParam(const char * key, SessionDesc sessionDesc, GridMate::GridSessionParam& p)
        {
            if (!strcmp(key, "cl_clientport"))
            {
                if (sessionDesc.m_gamePort == 0)
                {
                    if (gEnv && gEnv->pConsole && gEnv->pConsole->GetCVar("cl_clientport"))
                    {
                        p.SetValue(gEnv->pConsole->GetCVar("cl_clientport")->GetIVal());
                    }
                }
                else
                {
                    p.SetValue(sessionDesc.m_gamePort);
                }
            }
            else if (!strcmp(key, "gm_ipversion"))
            {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_4
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
                p.SetValue(GridMate::Driver::BSD_AF_INET);
#endif
            }
            else if (!strcmp(key, "gm_disconnectDetection"))
            {
                p.SetValue(sessionDesc.m_enableDisconnectDetection);
            }
            else if (!strcmp(key, "gm_disconnectDetectionRttThreshold"))
            {
                if (gEnv && gEnv->pConsole && gEnv->pConsole->GetCVar("gm_disconnectDetectionRttThreshold"))
                {
                    p.SetValue(gEnv->pConsole->GetCVar("gm_disconnectDetectionRttThreshold")->GetFVal());
                }
            }
            else if (!strcmp(key, "gm_disconnectDetectionPacketLossThreshold"))
            {
                if (gEnv && gEnv->pConsole && gEnv->pConsole->GetCVar("gm_disconnectDetectionPacketLossThreshold"))
                {
                    p.SetValue(gEnv->pConsole->GetCVar("gm_disconnectDetectionPacketLossThreshold")->GetFVal());
                }
            }
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_5
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, AZ_RESTRICTED_PLATFORM)
#endif
            return !p.m_value.empty();
        }

        void InitCarrierDesc(const GridMateServiceParams& gridMateServiceParams, GridMate::CarrierDesc& carrierDesc)
        {
            if (!carrierDesc.m_simulator)
            {
                EBUS_EVENT_RESULT(carrierDesc.m_simulator, Multiplayer::MultiplayerRequestBus, GetSimulator);
            }

            carrierDesc.m_port = gridMateServiceParams.FetchValueOrDefault<int>("cl_clientport", 0);
            carrierDesc.m_connectionTimeoutMS = 10000;
            carrierDesc.m_threadUpdateTimeMS = 30;
            carrierDesc.m_threadInstantResponse = true;
            carrierDesc.m_driverIsCrossPlatform = true;
            carrierDesc.m_securityData = gridMateServiceParams.FetchString("gm_securityData").c_str();
            carrierDesc.m_familyType = gridMateServiceParams.FetchValueOrDefault<int>("gm_ipversion", 1);
            carrierDesc.m_version = gridMateServiceParams.m_version;

            carrierDesc.m_enableDisconnectDetection = !!gridMateServiceParams.FetchValueOrDefault<int>("gm_disconnectDetection", 1);
            if (carrierDesc.m_enableDisconnectDetection)
            {
                carrierDesc.m_disconnectDetectionRttThreshold = gridMateServiceParams.FetchValueOrDefault<float>("gm_disconnectDetectionRttThreshold", 500.0f);
                carrierDesc.m_disconnectDetectionPacketLossThreshold = gridMateServiceParams.FetchValueOrDefault<float>("gm_disconnectDetectionPacketLossThreshold", 0.3f);
            }

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_6
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, AZ_RESTRICTED_PLATFORM)
#endif
        }

        /**
        * Helper function to translate GridMate::ServiceType to the proper MultiplayerLobbyServiceWrapper
        */
        GridMateServiceWrapper* RegisterServiceWrapper(GridMate::ServiceType gridServiceType)
        {
            switch (gridServiceType)
            {
            case GridMate::ST_LAN:
                return aznew GridMateLANServiceWrapper();
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDSYSTEMCONTEXT_CPP_SECTION_7
#include AZ_RESTRICTED_FILE(GridSystemContext_cpp, AZ_RESTRICTED_PLATFORM)
#endif
            default:
                AZ_Assert(false, "Unsupported GridMate::ServiceType of %d", gridServiceType);
            }
            return nullptr;
        }

    } // namespace GridMateSystemContext
};

