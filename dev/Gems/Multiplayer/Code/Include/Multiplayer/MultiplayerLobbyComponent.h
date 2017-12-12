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

#ifndef GEMS_MULTIPLAYER_MULTIPLAYERLOBBYCOMPONENT_H
#define GEMS_MULTIPLAYER_MULTIPLAYERLOBBYCOMPONENT_H

#include <ActionMap.h>
#include <AzCore/Component/Component.h>

#include <GridMate/GridMate.h>
#include <GridMate/Session/Session.h>

#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <GameLift/Session/GameLiftClientServiceEventsBus.h>
#pragma warning(default: 4355)

#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCanvasBus.h>

#include "Multiplayer/MultiplayerLobbyBus.h"
#include "Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyServiceWrapper.h"

namespace Multiplayer
{
    class MultiplayerLobbyComponent
        : public AZ::Component
        , public GridMate::SessionEventBus::Handler
        , public Multiplayer::MultiplayerLobbyBus::Handler
        , public UiCanvasNotificationBus::Handler
        , public IActionListener
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
        , public GridMate::GameLiftClientServiceEventsBus::Handler
#endif
    {
    private:
        enum class LobbyMode
        {
            Unknown,
            LobbySelection,
            GameliftLobby,
            ServiceWrapperLobby
        };

        class ServerListingResultRow
        {
        public:
            ServerListingResultRow(const AZ::EntityId& canvas, int row, int text, int highlight);

            int GetRowID();

            void Select();
            void Deselect();

            void DisplayResult(const GridMate::SearchInfo* result);

            void ResetDisplay();

        private:

            void SetTitle(const char* title);

            AZ::EntityId m_canvas;
            int m_row;
            int m_text;
            int m_highlight;
        };

    public:
        AZ_COMPONENT(MultiplayerLobbyComponent,"{916E8722-7CCF-4FBA-B2B2-81A7407B2272}");
        static void Reflect(AZ::ReflectContext* reflectContext);

        MultiplayerLobbyComponent();
        virtual ~MultiplayerLobbyComponent();

        // AZ::Component
        void Init();
        void Activate();
        void Deactivate();

        // UiCanvasActionNotification
        void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;

        // IActionListener
        void OnAction(const ActionId& action, int activationMode, float value) override;

        // SessionEventBus
        void OnSessionCreated(GridMate::GridSession* session) override;
        void OnSessionError(GridMate::GridSession* session, const GridMate::string& errorMsg) override;
        void OnGridSearchComplete(GridMate::GridSearch* search) override;

        // MultiplayerLobbyBus
        int GetGamePort() const;

        void ShowError(const char* error);
        void DismissError(bool force = false);

        void ShowBusyScreen();
        void DismissBusyScreen(bool force = false);

        void ConfigureSessionParams(GridMate::SessionParams& sessionParams);

    protected:

        // MultiplayerLobbyComponent
        void ShowSelectionLobby();
        void ShowLobby(LobbyMode lobbyType);
        void HideLobby();

        bool StartSessionService(LobbyMode lobbyType);
        void StopSessionService();

        void CreateServer();

        void ListServers();
        void ClearServerListings();
        void ClearSearches();
        void SelectId(int rowId);

        void JoinServer();
        bool JoinSession(const GridMate::SearchInfo* searchInfo);

        bool SanityCheck();

        void ShowQueuedErrorMessage();

        AZ::EntityId LoadCanvas(const char* canvasName) const;
        void SetElementEnabled(const AZ::EntityId& canvasID, const char* elementName, bool enabled);
        void SetElementInputEnabled(const AZ::EntityId& canvasID, const char* elementName, bool enabled);
        void SetElementText(const AZ::EntityId& canvasID, const char* elementName, const char* text);
        LyShine::StringType GetElementText(const AZ::EntityId& canvasID, const char* elementName) const;

        template<class T>
        void RegisterServiceWrapper()
        {
            if (m_multiplayerLobbyServiceWrapper)
            {
                delete m_multiplayerLobbyServiceWrapper;
            }

            m_multiplayerLobbyServiceWrapper = aznew T(GetEntityId());
        }

    private:

        // Server Listing Methods
        void OnLobbySelectionAction(const ActionId& action, int activationMode, float value);
        void OnServerListingAction(const ActionId& action, int activationMode, float value);

        // Functions used by the ServerListing View
        void SetupServerListingDisplay();
        LyShine::StringType GetServerName() const;
        LyShine::StringType GetMapName() const;

        // ServiceWrapperLobby Functions
        bool SanityCheckWrappedSessionService();
        void CreateServerForWrappedService();
        void ListServersForWrappedService();

        // GameLift Functions
        const char* GetGameLiftParam(const char* param);
        void SetGameLiftParam(const char* param, const char* value);

        void ShowGameLiftConfig();
        void DismissGameLiftConfig();
        void SaveGameLiftConfig();

        bool SanityCheckGameLift();

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
        bool StartGameLiftSession();
        void StopGameLiftSession();
        void CreateServerForGameLift();
        void ListServersForGameLift();

        // GridMate::GameLiftSessionServiceEventsBus::Handler
        void OnGameLiftSessionServiceReady(GridMate::GameLiftClientService*) override;
        void OnGameLiftSessionServiceFailed(GridMate::GameLiftClientService*, const AZStd::string& message) override;
#endif

        // External Configuration
        int m_maxPlayers;
        int m_port;
        bool m_enableDisconnectDetection;
        unsigned int m_connectionTimeoutMS;

        AZStd::string m_defaultMap;
        AZStd::string m_defaultServerName;

        // Internal Configuration
        AZ::EntityId m_selectionLobbyID;
        AZ::EntityId m_serverListingID;

        bool m_unregisterGameliftServiceOnErrorDismiss;
        bool m_hasGameliftSession;
        bool m_isShowingBusy;
        bool m_isShowingError;
        bool m_isShowingGameLiftConfig;
        LobbyMode m_lobbyMode;

        int m_selectedServerResult;

        // Shared Configuration
        GridMate::GridSearch* m_listSearch;
        GridMate::GridSearch* m_joinSearch;

        // Wrapped Session Service
        MultiplayerLobbyServiceWrapper* m_multiplayerLobbyServiceWrapper;

        // Gamelift Configuration
        GridMate::GridSearch* m_gameliftCreationSearch;

        AZStd::vector< ServerListingResultRow > m_listingRows;
        AZStd::vector< AZStd::string > m_errorMessageQueue;
    };
}

#endif