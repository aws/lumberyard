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
#pragma once

#include <Multiplayer_precompiled.h>
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/GameLiftBus.h>
#include <Multiplayer/MultiplayerUtils.h>

namespace Multiplayer
{
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
    class MultiplayerGameLiftClientRequests : public AZ::EBusTraits
    {
    public:
        // EBus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~MultiplayerGameLiftClientRequests() AZ_DEFAULT_METHOD;

        // Handler Interface
        virtual void HostGameLiftSession(const char* serverName, const char* mapName, const AZ::u32 maxPlayers) = 0;
        virtual void JoinGameLiftSession() = 0;
        virtual void StopGameLiftClientService() = 0;
    };

    using MultiplayerGameLiftClientBus = AZ::EBus<MultiplayerGameLiftClientRequests>;

    class MultiplayerGameLiftClient
        : public MultiplayerGameLiftClientBus::Handler
        , public GridMate::GameLiftClientServiceEventsBus::Handler
        , public GridMate::SessionEventBus::Handler
    {
    public:
        MultiplayerGameLiftClient();
        virtual ~MultiplayerGameLiftClient();

        // MultiplayerGameLiftClientBus
        void HostGameLiftSession(const char* serverName, const char* mapName, const AZ::u32 maxPlayers) override;
        void JoinGameLiftSession() override;
        void StopGameLiftClientService() override;

    private:
        enum class Mode
        {
            None,
            Join,
            Host,
        };

        enum class ServiceStatus
        {
            Stopped,
            Starting,
            Started,
        };

        virtual IConsole* GetConsole();
        virtual GridMate::IGridMate* GetGridMate();

        const char* GetConsoleParam(const char* paramName);
        const bool GetConsoleBoolParam(const char* paramName);
        void AddRequestParameter(GridMate::GameLiftSessionRequestParams& params, const char* name, const char* value);

        void StartGameLiftClientService();
        void HostGameLiftSessionInternal();
        void JoinGameLiftSessionInternal(const GridMate::GameLiftSearchInfo& searchInfo);
        void QueryGameLiftServers();
        void HandleGameLiftRequestByMode();

        // GameLiftClientServiceEventsBus
        void OnGameLiftSessionServiceReady(GridMate::GameLiftClientService*) override;
        void OnGameLiftSessionServiceFailed(GridMate::GameLiftClientService*, const AZStd::string& message) override;

        // SessionEventBus
        void OnGridSearchComplete(GridMate::GridSearch* gridSearch) override;

        Mode m_mode;
        ServiceStatus m_serviceStatus;

        IConsole* m_console;
        GridMate::IGridMate* m_gridMate;
        GridMate::GridSearch* m_search;

        AZStd::string m_serverName;
        AZStd::string m_mapName;
        AZ::u32 m_maxPlayers;
    };
#endif
}
