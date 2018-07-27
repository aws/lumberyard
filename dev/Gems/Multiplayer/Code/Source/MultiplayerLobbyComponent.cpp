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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

#include <GameLift/GameLiftBus.h>
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftSessionDefs.h>
#include <GameLift/Session/GameLiftSessionRequest.h>

#include <GridMate/Carrier/Driver.h>
#include <GridMate/NetworkGridMate.h>

#include <LyShine/Bus/UiButtonBus.h>
#include <LyShine/Bus/UiCursorBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiTextInputBus.h>

#include "Multiplayer/MultiplayerLobbyComponent.h"

#include "Multiplayer/IMultiplayerGem.h"
#include "Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyLANServiceWrapper.h"

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_1 1
#define MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_2 2
#define MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_3 3
#define MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_4 4
#define MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_5 5
#define MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_6 6
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(MultiplayerLobbyComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#include "Multiplayer/MultiplayerUtils.h"


namespace Multiplayer
{
    ///////////////////////////
    // ServerListingResultRow
    ///////////////////////////

    MultiplayerLobbyComponent::ServerListingResultRow::ServerListingResultRow(const AZ::EntityId& canvas, int row, int text, int highlight)
        : m_canvas(canvas)
        , m_row(row)
        , m_text(text)
        , m_highlight(highlight)
    {
    }

    int MultiplayerLobbyComponent::ServerListingResultRow::GetRowID()
    {
        return m_row;
    }

    void MultiplayerLobbyComponent::ServerListingResultRow::Select()
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element,m_canvas,UiCanvasBus, FindElementById, m_highlight);

        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(), UiElementBus, SetIsEnabled, true);
        }
    }

    void MultiplayerLobbyComponent::ServerListingResultRow::Deselect()
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element,m_canvas,UiCanvasBus, FindElementById, m_highlight);

        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(), UiElementBus, SetIsEnabled, false);
        }
    }

    void MultiplayerLobbyComponent::ServerListingResultRow::DisplayResult(const GridMate::SearchInfo* searchInfo)
    {
        char displayString[64];

        const char* serverName = nullptr;

        for (unsigned int i=0; i < searchInfo->m_numParams; ++i)
        {
            const GridMate::GridSessionParam& param = searchInfo->m_params[i];

            if (param.m_id == "sv_name")
            {
                serverName = param.m_value.c_str();
                break;
            }
        }

        azsnprintf(displayString,AZ_ARRAY_SIZE(displayString),"%s (%u/%u)",serverName,searchInfo->m_numUsedPublicSlots,searchInfo->m_numFreePublicSlots + searchInfo->m_numUsedPublicSlots);

        AZ::Entity* element = nullptr;

        EBUS_EVENT_ID_RESULT(element,m_canvas,UiCanvasBus, FindElementById, m_text);

        if (element != nullptr)
        {
            LyShine::StringType textString(displayString);
            EBUS_EVENT_ID(element->GetId(),UiTextBus,SetText,textString);
        }
    }

    void MultiplayerLobbyComponent::ServerListingResultRow::ResetDisplay()
    {
        SetTitle("");
        Deselect();
    }

    void MultiplayerLobbyComponent::ServerListingResultRow::SetTitle(const char* title)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element,m_canvas,UiCanvasBus, FindElementById, m_text);

        if (element != nullptr)
        {
            LyShine::StringType textString(title);
            EBUS_EVENT_ID(element->GetId(),UiTextBus,SetText,textString);
        }
    }

    //////////////////////////////
    // MultiplayerLobbyComponent
    //////////////////////////////

    // List of widgets we want to specifically references

    // Lobby Selection
    static const char* k_lobbySelectionLANButton = "LANButton";
    static const char* k_lobbySelectionGameliftButton = "GameliftButton";
    static const char* k_lobbySelectionXboxButton = "XboxLiveButton"; // ACCEPTED_USE
    static const char* k_lobbySelectionPSNButton = "PSNButton"; // ACCEPTED_USE
    static const char* k_lobbySelectionErrorWindow = "ErrorWindow";
    static const char* k_lobbySelectionErrorMessage = "ErrorMessage";
    static const char* k_lobbySelectionBusyScreen = "BusyScreen";
    static const char* k_lobbySelectionGameLiftConfigWindow = "GameLiftConfig";

    // Gamelift Config Controls
    static const char* k_lobbySelectionGameliftAWSAccesKeyInput = "AWSAccessKey";
    static const char* k_lobbySelectionGameliftAWSSecretKeyInput = "AWSSecretKey";
    static const char* k_lobbySelectionGameliftAWSRegionInput = "AWSRegion";
    static const char* k_lobbySelectionGameliftFleetIDInput = "FleetId";
    static const char* k_lobbySelectionGameliftEndPointInput = "EndPoint";
    static const char* k_lobbySelectionGameliftAliasIDInput = "AliasId";
    static const char* k_lobbySelectionGameliftPlayerIDInput = "PlayerId";
    static const char* k_lobbySelectionGameliftConnectButton = "GameliftConnectButton";
    static const char* k_lobbySelectionGameliftCancelButton = "GameliftCancelButton";

    // ServerListing Lobby
    static const char* k_serverListingServerNameTextBox = "ServerNameTextBox";
    static const char* k_serverListingMapNameTextBox = "MapNameTextBox";
    static const char* k_serverListingErrorWindow = "ErrorWindow";
    static const char* k_serverListingErrorMessage = "ErrorMessage";
    static const char* k_serverListingBusyScreen = "BusyScreen";
    static const char* k_serverListingJoinButton = "JoinButton";
    static const char* k_serverListingTitle = "Title";

    void MultiplayerLobbyComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serialize)
        {
            serialize->Class<MultiplayerLobbyComponent,AZ::Component>()
                ->Version(1)
                ->Field("MaxPlayers",&MultiplayerLobbyComponent::m_maxPlayers)
                ->Field("Port",&MultiplayerLobbyComponent::m_port)
                ->Field("EnableDisconnectDetection",&MultiplayerLobbyComponent::m_enableDisconnectDetection)
                ->Field("ConnectionTimeout",&MultiplayerLobbyComponent::m_connectionTimeoutMS)
                ->Field("DefaultMap",&MultiplayerLobbyComponent::m_defaultMap)
                ->Field("DefaultServer",&MultiplayerLobbyComponent::m_defaultServerName)
            ;

            AZ::EditContext* editContext = serialize->GetEditContext();

            if (editContext)
            {
                editContext->Class<MultiplayerLobbyComponent>("Multiplayer Lobby Component","This component will load up and manage a simple lobby for connecting for LAN and GameLift sessions.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData,"")
                        ->Attribute(AZ::Edit::Attributes::Category,"MultiplayerSample")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand,true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_maxPlayers,"Max Players","The total number of players that can join in the game.")
                        ->Attribute(AZ::Edit::Attributes::Min,0)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_port,"Port","The port on which the game service will create connections through.")
                        ->Attribute(AZ::Edit::Attributes::Min,1)
                        ->Attribute(AZ::Edit::Attributes::Max,65534)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_enableDisconnectDetection,"Enable Disconnect Detection","Enables disconnecting players if they do not respond within the Timeout window.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_connectionTimeoutMS,"Timeout","The length of time a client has to respond before being disconnected(if disconnection detection is enabled.")
                        ->Attribute(AZ::Edit::Attributes::Suffix,"ms")
                        ->Attribute(AZ::Edit::Attributes::Min,0)
                        ->Attribute(AZ::Edit::Attributes::Max,60000)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_defaultMap,"DefaultMap", "The default value that will be added to the map field when loading the lobby.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MultiplayerLobbyComponent::m_defaultServerName,"DefaultServerName","The default value that will be added to the server name field when loading the lobby.")
                ;
            }
        }
    }

    MultiplayerLobbyComponent::MultiplayerLobbyComponent()
        : m_maxPlayers(8)
        , m_port(SERVER_DEFAULT_PORT)
        , m_enableDisconnectDetection(true)
        , m_connectionTimeoutMS(500)
        , m_defaultMap("")
        , m_defaultServerName("MyServer")
        , m_selectionLobbyID()
        , m_serverListingID()
        , m_unregisterGameliftServiceOnErrorDismiss(false)
        , m_hasGameliftSession(false)
        , m_isShowingBusy(true)
        , m_isShowingError(true)
        , m_isShowingGameLiftConfig(true)
        , m_lobbyMode(LobbyMode::Unknown)
        , m_selectedServerResult(-1)
        , m_joinSearch(nullptr)
        , m_listSearch(nullptr)
        , m_multiplayerLobbyServiceWrapper(nullptr)
        , m_gameliftCreationSearch(nullptr)
    {
    }

    MultiplayerLobbyComponent::~MultiplayerLobbyComponent()
    {
    }

    void MultiplayerLobbyComponent::Init()
    {
    }

    void MultiplayerLobbyComponent::Activate()
    {
        Multiplayer::MultiplayerLobbyBus::Handler::BusConnect(GetEntityId());

        IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : nullptr;
        IActionMapManager* actionMapManager = pGameFramework ? pGameFramework->GetIActionMapManager() : nullptr;

        if (actionMapManager)
        {
            actionMapManager->EnableActionMap("lobby",true);
            actionMapManager->AddExtraActionListener(this, "lobby");
        }

        m_selectionLobbyID = LoadCanvas("ui/Canvases/selection_lobby.uicanvas");
        AZ_Error("MultiplayerLobbyComponent",m_selectionLobbyID.IsValid(),"Missing UI file for ServerType Selection Lobby.");

        m_serverListingID = LoadCanvas("ui/Canvases/listing_lobby.uicanvas");
        AZ_Error("MultiplayerLobbyComponent",m_serverListingID.IsValid(),"Missing UI file for Server Listing Lobby.");

        if (m_serverListingID.IsValid())
        {
            m_listingRows.emplace_back(m_serverListingID,10,11,32);
            m_listingRows.emplace_back(m_serverListingID,12,13,33);
            m_listingRows.emplace_back(m_serverListingID,14,15,34);
            m_listingRows.emplace_back(m_serverListingID,16,17,35);
            m_listingRows.emplace_back(m_serverListingID,18,19,36);
        }

        SetElementInputEnabled(m_selectionLobbyID,k_lobbySelectionLANButton,true);

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
        SetElementInputEnabled(m_selectionLobbyID,k_lobbySelectionGameliftButton,true);
#else
        SetElementInputEnabled(m_selectionLobbyID, k_lobbySelectionGameliftButton, false);
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(MultiplayerLobbyComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
		SetElementInputEnabled(m_selectionLobbyID, k_lobbySelectionXboxButton, false); // ACCEPTED_USE
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(MultiplayerLobbyComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
		SetElementInputEnabled(m_selectionLobbyID, k_lobbySelectionPSNButton, false); // ACCEPTED_USE
#endif

        ShowSelectionLobby();

        UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);

        if (gEnv->pNetwork)
        {
            GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

            if (gridMate)
            {
                GridMate::SessionEventBus::Handler::BusConnect(gridMate);
            }
        }
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_4
#include AZ_RESTRICTED_FILE(MultiplayerLobbyComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
    }

    void MultiplayerLobbyComponent::Deactivate()
    {
        UiCanvasNotificationBus::Handler::BusDisconnect();
        GridMate::SessionEventBus::Handler::BusDisconnect();

        if (m_selectionLobbyID.IsValid())
        {
            gEnv->pLyShine->ReleaseCanvas(m_selectionLobbyID, false);
        }

        if (m_serverListingID.IsValid())
        {
            gEnv->pLyShine->ReleaseCanvas(m_serverListingID, false);
        }

        IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : nullptr;
        IActionMapManager* actionMapManager = pGameFramework ? pGameFramework->GetIActionMapManager() : nullptr;

        if (actionMapManager)
        {
            actionMapManager->EnableActionMap("lobby",false);
            actionMapManager->RemoveExtraActionListener(this, "lobby");
        }

        UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);

        ClearSearches();

        delete m_multiplayerLobbyServiceWrapper;
        m_multiplayerLobbyServiceWrapper = nullptr;
    }

    void MultiplayerLobbyComponent::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
    {
        // Enforcing modality here, because there is currently not a good way to do modality in the UI System.
        // Once that gets entered, most of this should be condensed down for just a safety measure.
        if (m_isShowingBusy)
        {
            return;
        }
        else if (m_isShowingError)
        {
            if (actionName == "OnDismissErrorMessage")
            {
                if (m_unregisterGameliftServiceOnErrorDismiss)
                {
                    m_unregisterGameliftServiceOnErrorDismiss = false;

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
                    StopGameLiftSession();
#endif
                }

                DismissError();
            }

            return;
        }
        else if (m_isShowingGameLiftConfig)
        {
            if (actionName == "OnGameliftConnect")
            {
                SaveGameLiftConfig();
                DismissGameLiftConfig();

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
                ShowLobby(LobbyMode::GameliftLobby);
#else
                AZ_Assert(false,"Trying to use GameLift on unsupported Platform.");
#endif
            }
            else if (actionName == "OnGameliftCancel")
            {
                DismissGameLiftConfig();
            }

            return;
        }

        if (actionName == "OnCreateServer")
        {
            CreateServer();
        }
        else if (actionName == "OnSelectServer")
        {
            LyShine::ElementId elementId = 0;
            EBUS_EVENT_ID_RESULT(elementId, entityId, UiElementBus, GetElementId);

            SelectId(elementId);
        }
        else if (actionName == "OnJoinServer")
        {
            JoinServer();
        }
        else if (actionName == "OnRefresh")
        {
            ListServers();
        }
        else if (actionName == "OnListGameliftServers")
        {
            ShowGameLiftConfig();

        }
        else if (actionName == "OnListLANServers")
        {
            RegisterServiceWrapper<MultiplayerLobbyLANServiceWrapper>();
            ShowLobby(LobbyMode::ServiceWrapperLobby);
        }
        else if (actionName == "OnListXboxServers") // ACCEPTED_USE
        {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_5
#include AZ_RESTRICTED_FILE(MultiplayerLobbyComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            AZ_Assert(false,"Trying to use XBox Session Services without compiling for Xbone."); // ACCEPTED_USE
#endif
        }
        else if (actionName == "OnListPSNServers") // ACCEPTED_USE
        {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION MULTIPLAYERLOBBYCOMPONENT_CPP_SECTION_6
#include AZ_RESTRICTED_FILE(MultiplayerLobbyComponent_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            AZ_Assert(false,"Trying to use PSN Session services without compiling for PS4"); // ACCEPTED_USE
#endif
        }
        else if (actionName == "OnDismissErrorMessage")
        {

        }
        else if (actionName == "OnReturn")
        {
            ShowSelectionLobby();
        }
    }

    void MultiplayerLobbyComponent::OnAction(const ActionId& action, int activationMode, float value)
    {
        if (m_isShowingBusy)
        {
            return;
        }

        if (m_lobbyMode == LobbyMode::LobbySelection)
        {
            OnLobbySelectionAction(action,activationMode,value);
        }
        else
        {
            OnServerListingAction(action, activationMode, value);
        }
    }

    void MultiplayerLobbyComponent::OnSessionCreated(GridMate::GridSession* session)
    {
        GridMate::GridSession* gridSession = nullptr;
        EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);

        if (gridSession == session && session->IsHost())
        {
            Multiplayer::Utils::SynchronizeSessionState(session);
        }
    }

    void MultiplayerLobbyComponent::OnSessionError(GridMate::GridSession* session,const GridMate::string& errorMsg)
    {
        ShowError(errorMsg.c_str());
    }

    void MultiplayerLobbyComponent::OnGridSearchComplete(GridMate::GridSearch* search)
    {
        if (search == m_gameliftCreationSearch)
        {
            DismissBusyScreen();

            GridMate::Network* network = static_cast<GridMate::Network*>(gEnv->pNetwork);

            if (network)
            {
                if (search->GetNumResults() == 0)
                {
                    ShowError("Error creating GameLift Session");
                }
                else
                {
                    const GridMate::SearchInfo* searchInfo = m_gameliftCreationSearch->GetResult(0);
                    JoinSession(searchInfo);
                }
            }

            m_gameliftCreationSearch->Release();
            m_gameliftCreationSearch = nullptr;
        }
        else if (search == m_listSearch)
        {
            for (unsigned int i=0; i < search->GetNumResults(); ++i)
            {
                // Screen is currently not dynamically populated, so we are stuck with a fixed size
                // amount of results for now.
                if (i >= m_listingRows.size())
                {
                    break;
                }

                const GridMate::SearchInfo* searchInfo = search->GetResult(i);

                ServerListingResultRow& resultRow = m_listingRows[i];

                resultRow.DisplayResult(searchInfo);
            }

            DismissBusyScreen();
        }
    }

    int MultiplayerLobbyComponent::GetGamePort() const
    {
        return m_port;
    }

    void MultiplayerLobbyComponent::ShowError(const char* message)
    {
        if (m_isShowingBusy)
        {
            DismissBusyScreen();
        }

        if (!m_isShowingError)
        {
            const char* errorWindow = nullptr;
            const char* errorMessage = nullptr;
            AZ::EntityId canvasID;

            m_isShowingError = true;

            if (m_lobbyMode == LobbyMode::GameliftLobby || m_lobbyMode == LobbyMode::ServiceWrapperLobby)
            {
                errorWindow = k_serverListingErrorWindow;
                errorMessage = k_serverListingErrorMessage;
                canvasID = m_serverListingID;
            }
            else if (m_lobbyMode == LobbyMode::LobbySelection)
            {
                errorWindow = k_lobbySelectionErrorWindow;
                errorMessage = k_lobbySelectionErrorMessage;
                canvasID = m_selectionLobbyID;
            }

            SetElementEnabled(canvasID,errorWindow,true);

            AZ::Entity* element = nullptr;
            EBUS_EVENT_ID_RESULT(element,canvasID,UiCanvasBus,FindElementByName,errorMessage);

            if (element != nullptr)
            {
                LyShine::StringType textString(message);
                EBUS_EVENT_ID(element->GetId(),UiTextBus,SetText,textString);
            }
            else
            {
                AZ_Error("MultiplayerLobby",false,"Failed to show error message(%s)", message);
                m_isShowingError = false;
            }
        }
        else
        {
            m_errorMessageQueue.emplace_back(message);
        }
    }

    void MultiplayerLobbyComponent::DismissError(bool force)
    {
        if (m_isShowingError || force)
        {
            const char* errorWindow = nullptr;
            AZ::EntityId canvasID;

            m_isShowingError = false;

            if (m_lobbyMode == LobbyMode::GameliftLobby || m_lobbyMode == LobbyMode::ServiceWrapperLobby)
            {
                errorWindow = k_serverListingErrorWindow;
                canvasID = m_serverListingID;
            }
            else if (m_lobbyMode == LobbyMode::LobbySelection)
            {
                errorWindow = k_lobbySelectionErrorWindow;
                canvasID = m_selectionLobbyID;
            }

            SetElementEnabled(canvasID,errorWindow,false);

            if (force)
            {
                m_errorMessageQueue.clear();
            }
            else
            {
                ShowQueuedErrorMessage();
            }
        }
    }

    void MultiplayerLobbyComponent::ShowBusyScreen()
    {
        if (!m_isShowingBusy)
        {
            const char* busyWindow = nullptr;
            AZ::EntityId canvasID;

            m_isShowingBusy = true;

            if (m_lobbyMode == LobbyMode::GameliftLobby || m_lobbyMode == LobbyMode::ServiceWrapperLobby)
            {
                busyWindow = k_serverListingBusyScreen;
                canvasID = m_serverListingID;
            }
            else if (m_lobbyMode == LobbyMode::LobbySelection)
            {
                busyWindow = k_lobbySelectionBusyScreen;
                canvasID = m_selectionLobbyID;
            }

            SetElementEnabled(canvasID,busyWindow,true);
        }
    }

    void MultiplayerLobbyComponent::DismissBusyScreen(bool force)
    {
        if (m_isShowingBusy || force)
        {
            const char* busyWindow = nullptr;
            AZ::EntityId canvasID;

            m_isShowingBusy = false;

            if (m_lobbyMode == LobbyMode::GameliftLobby || m_lobbyMode == LobbyMode::ServiceWrapperLobby)
            {
                busyWindow = k_serverListingBusyScreen;
                canvasID = m_serverListingID;
            }
            else if (m_lobbyMode == LobbyMode::LobbySelection)
            {
                busyWindow = k_lobbySelectionBusyScreen;
                canvasID = m_selectionLobbyID;
            }

            SetElementEnabled(canvasID,busyWindow,false);
        }
    }

    void MultiplayerLobbyComponent::ConfigureSessionParams(GridMate::SessionParams& sessionParams)
    {
        sessionParams.m_topology = GridMate::ST_CLIENT_SERVER;
        sessionParams.m_numPublicSlots = m_maxPlayers + (gEnv->IsDedicated() ? 1 : 0); // One slot for server member.
        sessionParams.m_numPrivateSlots = 0;
        sessionParams.m_peerToPeerTimeout = 60000;
        sessionParams.m_flags = 0;

        sessionParams.m_numParams = 0;
        sessionParams.m_params[sessionParams.m_numParams].m_id = "sv_name";
        sessionParams.m_params[sessionParams.m_numParams].SetValue(GetServerName().c_str());
        sessionParams.m_numParams++;

        sessionParams.m_params[sessionParams.m_numParams].m_id = "sv_map";
        sessionParams.m_params[sessionParams.m_numParams].SetValue(GetMapName().c_str());
        sessionParams.m_numParams++;
    }

    void MultiplayerLobbyComponent::ShowSelectionLobby()
    {
        const bool forceHide = true;

        if (m_lobbyMode != LobbyMode::LobbySelection)
        {
            ClearSearches();
            StopSessionService();

            if (m_multiplayerLobbyServiceWrapper)
            {
                delete m_multiplayerLobbyServiceWrapper;
                m_multiplayerLobbyServiceWrapper = nullptr;
            }

            HideLobby();
            m_lobbyMode = LobbyMode::LobbySelection;
            UiCanvasNotificationBus::Handler::BusConnect(m_selectionLobbyID);

            EBUS_EVENT_ID(m_selectionLobbyID, UiCanvasBus, SetEnabled, true);

            DismissGameLiftConfig();
            DismissError(forceHide);
            DismissBusyScreen(forceHide);
        }
    }

    void MultiplayerLobbyComponent::ShowLobby(LobbyMode lobbyMode)
    {
        if (lobbyMode == LobbyMode::LobbySelection)
        {
            ShowSelectionLobby();
        }
        else if (m_lobbyMode == LobbyMode::LobbySelection)
        {
            bool showLobby = StartSessionService(lobbyMode);

            if (showLobby)
            {
                HideLobby();
                m_lobbyMode = lobbyMode;
                UiCanvasNotificationBus::Handler::BusConnect(m_serverListingID);

                ClearServerListings();
                EBUS_EVENT_ID(m_serverListingID, UiCanvasBus, SetEnabled, true);

                const bool forceHide = true;
                DismissError(forceHide);
                DismissBusyScreen(forceHide);

                SetupServerListingDisplay();
            }
        }
    }

    void MultiplayerLobbyComponent::HideLobby()
    {
        UiCanvasNotificationBus::Handler::BusDisconnect();
        m_lobbyMode = LobbyMode::Unknown;

        EBUS_EVENT_ID(m_selectionLobbyID, UiCanvasBus, SetEnabled, false);
        EBUS_EVENT_ID(m_serverListingID, UiCanvasBus, SetEnabled, false);
    }

    bool MultiplayerLobbyComponent::StartSessionService(LobbyMode lobbyMode)
    {
        bool startedService = false;

        if (lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            if (gEnv->pNetwork && gEnv->pNetwork->GetGridMate())
            {
                if (SanityCheckWrappedSessionService())
                {
                    startedService = m_multiplayerLobbyServiceWrapper->StartSessionService(gEnv->pNetwork->GetGridMate());
                }
            }
        }
        else if (lobbyMode == LobbyMode::GameliftLobby)
        {
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
            startedService = StartGameLiftSession();
#else
            startedService = false;
#endif
        }

        return startedService;
    }

    void MultiplayerLobbyComponent::StopSessionService()
    {
        // Stop whatever session we may have been using, if any
        if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            if (gEnv->pNetwork && gEnv->pNetwork->GetGridMate())
            {
                if (SanityCheckWrappedSessionService())
                {
                    m_multiplayerLobbyServiceWrapper->StopSessionService(gEnv->pNetwork->GetGridMate());
                }
            }
        }
        else if (m_lobbyMode == LobbyMode::GameliftLobby)
        {
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
            StopGameLiftSession();
            m_hasGameliftSession = false;
#else
            AZ_Assert(false,"Trying to use Gamelift on Unsupported platform.");
#endif
        }
    }

    void MultiplayerLobbyComponent::CreateServer()
    {
        if (m_lobbyMode == LobbyMode::LobbySelection)
        {
            return;
        }
        else if (SanityCheck())
        {
            if (GetMapName().empty())
            {
                ShowError("Invalid Map Name");
            }
            else if (GetServerName().empty())
            {
                ShowError("Invalid Server Name");
            }
            else
            {
                bool netSecEnabled = false;
                EBUS_EVENT_RESULT(netSecEnabled, Multiplayer::MultiplayerRequestBus,IsNetSecEnabled);

                if (netSecEnabled)
                {
                    if (!NetSec::CanCreateSecureSocketForHosting())
                    {
                        ShowError("Invalid Secure Socket configuration given for hosting a session.\nEnsure that a Public and Private key are being supplied.");
                        return;
                    }
                }

                if (m_lobbyMode == LobbyMode::GameliftLobby)
                {
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
                    if (SanityCheckGameLift())
                    {
                        CreateServerForGameLift();
                    }
#else
                    AZ_Assert(false,"Trying to use Gamelift on unsupported platform.");
#endif
                }
                else if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
                {
                    if (SanityCheckWrappedSessionService())
                    {
                        CreateServerForWrappedService();
                    }
                }
            }
        }
    }

    void MultiplayerLobbyComponent::ListServers()
    {
        ClearServerListings();

        if (m_lobbyMode == LobbyMode::GameliftLobby)
        {
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
            if (SanityCheckGameLift())
            {
                ListServersForGameLift();
            }
#else
            AZ_Assert(false,"Trying to use Gamelift lobby on unsupported platform.")
#endif
        }
        else if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            ListServersForWrappedService();
        }
    }

    void MultiplayerLobbyComponent::ClearServerListings()
    {
        m_selectedServerResult = -1;

        for (ServerListingResultRow& serverResultRow : m_listingRows)
        {
            serverResultRow.ResetDisplay();
        }

        SetElementInputEnabled(m_serverListingID,k_serverListingJoinButton,false);
    }

    void MultiplayerLobbyComponent::ClearSearches()
    {
        if (m_listSearch)
        {
            if (!m_listSearch->IsDone())
            {
                m_listSearch->AbortSearch();
            }

            m_listSearch->Release();
            m_listSearch = nullptr;
        }

        if (m_gameliftCreationSearch)
        {
            if (!m_gameliftCreationSearch->IsDone())
            {
                m_gameliftCreationSearch->AbortSearch();
            }

            m_gameliftCreationSearch->Release();
            m_gameliftCreationSearch = nullptr;
        }
    }

    void MultiplayerLobbyComponent::SelectId(int rowId)
    {
        if (m_listSearch == nullptr || !m_listSearch->IsDone())
        {
            return;
        }

        SetElementInputEnabled(m_serverListingID,k_serverListingJoinButton,false);
        int lastSelection = m_selectedServerResult;

        m_selectedServerResult = -1;
        for (int i=0; i < static_cast<int>(m_listingRows.size()); ++i)
        {
            ServerListingResultRow& resultRow = m_listingRows[i];

            if (resultRow.GetRowID() == rowId)
            {
                if (i < m_listSearch->GetNumResults())
                {
                    SetElementInputEnabled(m_serverListingID,k_serverListingJoinButton,true);
                    m_selectedServerResult = i;
                    resultRow.Select();
                }
                else
                {
                    resultRow.Deselect();
                }
            }
            else
            {
                resultRow.Deselect();
            }
        }

        // Double click to join.
        if (m_selectedServerResult >= 0 && lastSelection == m_selectedServerResult)
        {
            JoinServer();
        }
    }

    void MultiplayerLobbyComponent::JoinServer()
    {
        if (m_lobbyMode == LobbyMode::LobbySelection)
        {
            return;
        }

        if (   m_listSearch == nullptr
            || !m_listSearch->IsDone()
            || m_selectedServerResult < 0
            || m_listSearch->GetNumResults() <= m_selectedServerResult)
        {
            ShowError("No Server Selected to Join.");
            return;
        }

        const GridMate::SearchInfo* searchInfo = m_listSearch->GetResult(m_selectedServerResult);

        if (!SanityCheck())
        {
            return;
        }
        else if (searchInfo == nullptr)
        {
            ShowError("Invalid Server Selection.");
            return;
        }
        else
        {
            bool netSecEnabled = false;
            EBUS_EVENT_RESULT(netSecEnabled, Multiplayer::MultiplayerRequestBus,IsNetSecEnabled);

            if (netSecEnabled)
            {
                if (!NetSec::CanCreateSecureSocketForJoining())
                {
                    ShowError("Invalid Secure Socket configuration given for joining an encrypted session.\nEnsure that a Certificate Authority is being supplied.");
                    return;
                }
            }

            if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
            {
                if (!SanityCheckWrappedSessionService())
                {
                    return;
                }
            }
            else if (m_lobbyMode == LobbyMode::GameliftLobby)
            {
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
                if (!SanityCheckGameLift())
                {
                    return;
                }
#else
                AZ_Assert(false,"Trying to use Gamelift lobby on unsupported platform.")
                return;
#endif
            }
        }

        ShowBusyScreen();

        if (!JoinSession(searchInfo))
        {
            ShowError("Found a game session, but failed to join.");
        }
    }

    bool MultiplayerLobbyComponent::JoinSession(const GridMate::SearchInfo* searchInfo)
    {
        GridMate::GridSession* session = nullptr;
        GridMate::CarrierDesc carrierDesc;

        Multiplayer::Utils::InitCarrierDesc(carrierDesc);
        Multiplayer::NetSec::ConfigureCarrierDescForJoin(carrierDesc);

        GridMate::JoinParams joinParams;

        if (m_lobbyMode == LobbyMode::ServiceWrapperLobby)
        {
            if (SanityCheckWrappedSessionService())
            {
                session = m_multiplayerLobbyServiceWrapper->JoinSession(gEnv->pNetwork->GetGridMate(),carrierDesc,searchInfo);
            }
        }
        else if (m_lobbyMode == LobbyMode::GameliftLobby)
        {
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
            const GridMate::GameLiftSearchInfo& gameliftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo&>(*searchInfo);
            EBUS_EVENT_ID_RESULT(session, gEnv->pNetwork->GetGridMate(), GridMate::GameLiftClientServiceBus, JoinSessionBySearchInfo, gameliftSearchInfo, carrierDesc);
#endif
        }

        if (session != nullptr)
        {
            EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSession,session);
        }
        else
        {
            Multiplayer::NetSec::OnSessionFailedToCreate(carrierDesc);
        }

        return session != nullptr;
    }

    bool MultiplayerLobbyComponent::SanityCheck()
    {
        if (gEnv->IsEditor())
        {
            ShowError("Unsupported action inside of Editor.");
            return false;
        }
        else if (gEnv->pNetwork == nullptr)
        {
            ShowError("Network Environment is null");
            return false;
        }
        else
        {
            GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

            if (gridMate == nullptr)
            {
                ShowError("GridMate is null.");
                return false;
            }
        }

        return true;
    }

    void MultiplayerLobbyComponent::ShowQueuedErrorMessage()
    {
        if (!m_errorMessageQueue.empty())
        {
            AZStd::string errorMessage = m_errorMessageQueue.front();
            m_errorMessageQueue.erase(m_errorMessageQueue.begin());

            ShowError(errorMessage.c_str());
        }
    }

    AZ::EntityId MultiplayerLobbyComponent::LoadCanvas(const char* canvasName) const
    {
        if (gEnv->pLyShine)
        {
            AZ::EntityId canvasEntityId = gEnv->pLyShine->LoadCanvas(canvasName);

            return canvasEntityId;
        }
        return AZ::EntityId();
    }

    void MultiplayerLobbyComponent::SetElementEnabled(const AZ::EntityId& canvasID, const char* elementName, bool enabled)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element,canvasID,UiCanvasBus, FindElementByName, elementName);

        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(),UiElementBus,SetIsEnabled,enabled);
        }
    }

    void MultiplayerLobbyComponent::SetElementInputEnabled(const AZ::EntityId& canvasID, const char* elementName, bool enabled)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element,canvasID,UiCanvasBus, FindElementByName, elementName);

        if (element != nullptr)
        {
            EBUS_EVENT_ID(element->GetId(),UiInteractableBus,SetIsHandlingEvents,enabled);
        }
    }

    void MultiplayerLobbyComponent::SetElementText(const AZ::EntityId& canvasID, const char* elementName, const char* text)
    {
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element,canvasID,UiCanvasBus, FindElementByName, elementName);

        if (element != nullptr)
        {
            if (UiTextInputBus::FindFirstHandler(element->GetId()))
            {
                EBUS_EVENT_ID(element->GetId(),UiTextInputBus,SetText,text);
            }
            else if (UiTextBus::FindFirstHandler(element->GetId()))
            {
                EBUS_EVENT_ID(element->GetId(),UiTextBus,SetText,text);
            }
        }
    }

    LyShine::StringType MultiplayerLobbyComponent::GetElementText(const AZ::EntityId& canvasID, const char* elementName) const
    {
        LyShine::StringType retVal;
        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element,canvasID,UiCanvasBus, FindElementByName, elementName);

        if (element != nullptr)
        {
            if (UiTextInputBus::FindFirstHandler(element->GetId()))
            {
                EBUS_EVENT_ID_RESULT(retVal,element->GetId(),UiTextInputBus,GetText);
            }
            else if (UiTextBus::FindFirstHandler(element->GetId()))
            {
                EBUS_EVENT_ID_RESULT(retVal,element->GetId(),UiTextBus,GetText);
            }
        }

        return retVal;
    }

    void MultiplayerLobbyComponent::OnLobbySelectionAction(const ActionId& action, int activationMode, float value)
    {

    }

    void MultiplayerLobbyComponent::OnServerListingAction(const ActionId& action, int activationMode, float value)
    {

    }

    void MultiplayerLobbyComponent::SetupServerListingDisplay()
    {
        AZ::Entity* element = nullptr;
        LyShine::StringType textString;

        EBUS_EVENT_ID_RESULT(element,m_serverListingID,UiCanvasBus,FindElementByName,k_serverListingTitle);
        if (element)
        {
            if (m_lobbyMode == LobbyMode::GameliftLobby)
            {
                textString = "GameLift Servers";
            }
            else if (m_lobbyMode == LobbyMode::ServiceWrapperLobby && SanityCheckWrappedSessionService())
            {
                textString = m_multiplayerLobbyServiceWrapper->GetLobbyTitle();
            }
            else
            {
                textString = "Unknown";
            }
            EBUS_EVENT_ID(element->GetId(), UiTextBus, SetText, textString);
        }

        element = nullptr;

        EBUS_EVENT_ID_RESULT(element,m_serverListingID,UiCanvasBus,FindElementByName,k_serverListingMapNameTextBox);

        if (element)
        {
            textString = m_defaultMap.c_str();

            EBUS_EVENT_ID(element->GetId(),UiTextInputBus,SetText,textString);
        }

        element = nullptr;

        EBUS_EVENT_ID_RESULT(element,m_serverListingID,UiCanvasBus,FindElementByName,k_serverListingServerNameTextBox);

        if (element)
        {
            textString = m_defaultServerName.c_str();

            EBUS_EVENT_ID(element->GetId(),UiTextInputBus,SetText,textString);
        }
    }

    LyShine::StringType MultiplayerLobbyComponent::GetServerName() const
    {
        return GetElementText(m_serverListingID,k_serverListingServerNameTextBox);
    }

    LyShine::StringType MultiplayerLobbyComponent::GetMapName() const
    {
        return GetElementText(m_serverListingID,k_serverListingMapNameTextBox);
    }

    bool MultiplayerLobbyComponent::SanityCheckWrappedSessionService()
    {
        return m_multiplayerLobbyServiceWrapper != nullptr;
    }

    void MultiplayerLobbyComponent::CreateServerForWrappedService()
    {
        GridMate::GridSession* gridSession = nullptr;
        EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);

        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        if (!gridSession && SanityCheckWrappedSessionService())
        {
            GridMate::CarrierDesc carrierDesc;

            Multiplayer::Utils::InitCarrierDesc(carrierDesc);
            Multiplayer::NetSec::ConfigureCarrierDescForHost(carrierDesc);

            carrierDesc.m_port = m_port;
            carrierDesc.m_enableDisconnectDetection = m_enableDisconnectDetection;
            carrierDesc.m_connectionTimeoutMS = m_connectionTimeoutMS;
            carrierDesc.m_threadUpdateTimeMS = 30;

            ShowBusyScreen();

            GridMate::GridSession* session = m_multiplayerLobbyServiceWrapper->CreateServer(gridMate,carrierDesc);

            if (session == nullptr)
            {
                Multiplayer::NetSec::OnSessionFailedToCreate(carrierDesc);
                ShowError("Error while hosting Session.");
            }
            else
            {
                EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSession,session);
            }
        }
        else
        {
            ShowError("Invalid Gem Session");
        }
    }

    void MultiplayerLobbyComponent::ListServersForWrappedService()
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        if (gridMate && SanityCheck() && SanityCheckWrappedSessionService())
        {
            ShowBusyScreen();

            if (m_listSearch)
            {
                m_listSearch->AbortSearch();
                m_listSearch->Release();
                m_listSearch = nullptr;
            }

            m_listSearch = m_multiplayerLobbyServiceWrapper->ListServers(gridMate);

            if (m_listSearch == nullptr)
            {
                ShowError("ListServers failed to start a GridSearch.");
            }
        }
        else
        {
            ShowError("Missing Online Service.");
        }
    }

    const char* MultiplayerLobbyComponent::GetGameLiftParam(const char* param)
    {
        ICVar* cvar = gEnv->pConsole->GetCVar(param);

        if (cvar)
        {
            return cvar->GetString();
        }

        return "";
    }

    bool MultiplayerLobbyComponent::GetGameLiftBoolParam(const char* param)
    {
        bool value = false;
        ICVar* cvar = gEnv->pConsole->GetCVar(param);

        if (cvar)
        {
            if( cvar->GetI64Val() )
            {
                value = true;
            }
        }

        return value;
    }

    void MultiplayerLobbyComponent::SetGameLiftParam(const char* param, const char* value)
    {
        ICVar* cvar = gEnv->pConsole->GetCVar(param);

        if (cvar)
        {
            cvar->Set(value);
        }
    }

    void MultiplayerLobbyComponent::ShowGameLiftConfig()
    {
        if (!m_isShowingGameLiftConfig)
        {
            m_isShowingGameLiftConfig = true;

            SetElementEnabled(m_selectionLobbyID,k_lobbySelectionGameLiftConfigWindow,true);
            SetElementText(m_selectionLobbyID,k_lobbySelectionGameliftAWSAccesKeyInput,GetGameLiftParam("gamelift_aws_access_key"));
            SetElementText(m_selectionLobbyID,k_lobbySelectionGameliftAWSSecretKeyInput,GetGameLiftParam("gamelift_aws_secret_key"));
            SetElementText(m_selectionLobbyID,k_lobbySelectionGameliftAWSRegionInput,GetGameLiftParam("gamelift_aws_region"));
            SetElementText(m_selectionLobbyID,k_lobbySelectionGameliftFleetIDInput,GetGameLiftParam("gamelift_fleet_id"));
            SetElementText(m_selectionLobbyID,k_lobbySelectionGameliftEndPointInput,GetGameLiftParam("gamelift_endpoint"));
            SetElementText(m_selectionLobbyID,k_lobbySelectionGameliftAliasIDInput,GetGameLiftParam("gamelift_alias_id"));
            SetElementText(m_selectionLobbyID,k_lobbySelectionGameliftPlayerIDInput,GetGameLiftParam("gamelift_player_id"));
        }
    }

    void MultiplayerLobbyComponent::DismissGameLiftConfig()
    {
        if (m_isShowingGameLiftConfig)
        {
            m_isShowingGameLiftConfig = false;

            SetElementEnabled(m_selectionLobbyID,k_lobbySelectionGameLiftConfigWindow,false);
        }
    }

    void MultiplayerLobbyComponent::SaveGameLiftConfig()
    {
        LyShine::StringType param;

        param = GetElementText(m_selectionLobbyID,k_lobbySelectionGameliftAWSAccesKeyInput);
        SetGameLiftParam("gamelift_aws_access_key",param.c_str());

        param = GetElementText(m_selectionLobbyID,k_lobbySelectionGameliftAWSSecretKeyInput);
        SetGameLiftParam("gamelift_aws_secret_key",param.c_str());

        param = GetElementText(m_selectionLobbyID,k_lobbySelectionGameliftAWSRegionInput);
        SetGameLiftParam("gamelift_aws_region",param.c_str());

        param = GetElementText(m_selectionLobbyID,k_lobbySelectionGameliftFleetIDInput);
        SetGameLiftParam("gamelift_fleet_id",param.c_str());

        param = GetElementText(m_selectionLobbyID,k_lobbySelectionGameliftEndPointInput);
        SetGameLiftParam("gamelift_endpoint",param.c_str());

        param = GetElementText(m_selectionLobbyID,k_lobbySelectionGameliftAliasIDInput);
        SetGameLiftParam("gamelift_alias_id",param.c_str());

        param = GetElementText(m_selectionLobbyID,k_lobbySelectionGameliftPlayerIDInput);
        SetGameLiftParam("gamelift_player_id",param.c_str());
    }

    bool MultiplayerLobbyComponent::SanityCheckGameLift()
    {
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        // This should already be errored by the previous sanity check.
        if (gridMate == nullptr)
        {
            return false;
        }
        else if (!GridMate::HasGridMateService<GridMate::GameLiftClientService>(gridMate))
        {
            ShowError("MultiplayerService is missing.");
            return false;
        }

        return true;
#else
        return false;
#endif
    }

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
    bool MultiplayerLobbyComponent::StartGameLiftSession()
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        // Not sure what happens if we start this once and it fails to be created...
        // calling it again causes an assert.
        if (gridMate && !m_hasGameliftSession)
        {
            ShowBusyScreen();

            GridMate::GameLiftClientServiceEventsBus::Handler::BusConnect(gridMate);

            GridMate::GameLiftClientServiceDesc serviceDesc;
            serviceDesc.m_accessKey = GetGameLiftParam("gamelift_aws_access_key");
            serviceDesc.m_secretKey = GetGameLiftParam("gamelift_aws_secret_key");
            serviceDesc.m_fleetId = GetGameLiftParam("gamelift_fleet_id");
            serviceDesc.m_endpoint = GetGameLiftParam("gamelift_endpoint");
            serviceDesc.m_region = GetGameLiftParam("gamelift_aws_region");
            serviceDesc.m_aliasId = GetGameLiftParam("gamelift_alias_id");
            serviceDesc.m_playerId = GetGameLiftParam("gamelift_player_id");
            serviceDesc.m_useGameLiftLocalServer = GetGameLiftBoolParam("gamelift_uselocalserver");

            EBUS_EVENT(GameLift::GameLiftRequestBus, StartClientService, serviceDesc);
        }

        return m_hasGameliftSession;
    }

    void MultiplayerLobbyComponent::StopGameLiftSession()
    {
        EBUS_EVENT(GameLift::GameLiftRequestBus, StopClientService);
    }

    void MultiplayerLobbyComponent::CreateServerForGameLift()
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();

        if (m_gameliftCreationSearch)
        {
            m_gameliftCreationSearch->AbortSearch();
            m_gameliftCreationSearch->Release();
            m_gameliftCreationSearch = nullptr;
        }

        GridMate::GameLiftSessionRequestParams reqParams;
        ConfigureSessionParams(reqParams);
        reqParams.m_instanceName = GetServerName().c_str();

        ShowBusyScreen();

        EBUS_EVENT_ID_RESULT(m_gameliftCreationSearch, gridMate, GridMate::GameLiftClientServiceBus, RequestSession, reqParams);
    }

    void MultiplayerLobbyComponent::ListServersForGameLift()
    {
        ShowBusyScreen();
        GridMate::Network* network = static_cast<GridMate::Network*>(gEnv->pNetwork);
        GridMate::IGridMate* gridMate = network->GetGridMate();

        if (m_listSearch)
        {
            m_listSearch->AbortSearch();
            m_listSearch->Release();
            m_listSearch = nullptr;
        }

        EBUS_EVENT_ID_RESULT(m_listSearch,gridMate,GridMate::GameLiftClientServiceBus, StartSearch, GridMate::GameLiftSearchParams());

        if (m_listSearch == nullptr)
        {
            ShowError("Failed to start a GridSearch");
        }
    }

    void MultiplayerLobbyComponent::OnGameLiftSessionServiceReady(GridMate::GameLiftClientService* service)
    {
        AZ_UNUSED(service);
        DismissBusyScreen();

        m_hasGameliftSession = true;
        ShowLobby(LobbyMode::GameliftLobby);
    }

    void MultiplayerLobbyComponent::OnGameLiftSessionServiceFailed(GridMate::GameLiftClientService* service, const AZStd::string& message)
    {
        AZ_UNUSED(service);

        DismissBusyScreen();

        m_hasGameliftSession = false;

        m_unregisterGameliftServiceOnErrorDismiss = true;

        AZStd::string errorMessage("GameLift Error: ");
        errorMessage += message;
        ShowError(errorMessage.c_str());
    }
#endif
}
