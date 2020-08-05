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

#include <GameStates/StarterGameStateMainMenu.h>
#include <GameServices/PlayerData.h>

#include <LmbrCentral/Audio/AudioSystemComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace StarterGame
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateMainMenu::OnPushed()
    {
        GameStateMainMenu::OnPushed();

        // Load the primary user's player data
        PlayerDataRequestsBus::Broadcast(&PlayerDataRequests::LoadPlayerDataFromPersistentStorage,
                                         LocalUser::LocalUserRequests::GetPrimaryLocalUserId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateMainMenu::OnPopped()
    {
        StopMainMenuMusicLoop();
        GameStateMainMenu::OnPopped();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateMainMenu::OnEnter()
    {
        GameStateMainMenu::OnEnter();
        PlayMainMenuMusicLoop();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateMainMenu::OnExit()
    {
        GameStateMainMenu::OnExit();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateMainMenu::LoadMainMenuCanvas()
    {
        GameStateMainMenu::LoadMainMenuCanvas();

        // Setup the 'SinglePlayer' button to load the first level
        AZ::EntityId playButtonElementId;
        UiCanvasBus::EventResult(playButtonElementId,
                                 m_mainMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "SinglePlayerButton");
        UiButtonBus::Event(playButtonElementId, &UiButtonInterface::SetOnClickCallback,
                           [this](AZ::EntityId clickedEntityId, AZ::Vector2 point)
        {
            StopMainMenuMusicLoop();
            AZStd::string mapCommand = "map SinglePlayer";  
            GetISystem()->GetGlobalEnvironment()->pConsole->ExecuteString(mapCommand.c_str());
        });

        // Setup the PVP Button
        UiCanvasBus::EventResult(playButtonElementId,
                                m_mainMenuCanvasEntityId,
                                &UiCanvasInterface::FindElementEntityIdByName,
                                "PVPButton");
        UiButtonBus::Event(playButtonElementId, &UiButtonInterface::SetOnClickCallback,
            [this](AZ::EntityId clickedEntityId, AZ::Vector2 point)
        {
            StopMainMenuMusicLoop();
            AZStd::string mapCommand = "map empty_level";
            GetISystem()->GetGlobalEnvironment()->pConsole->ExecuteString(mapCommand.c_str());
        });

        // Ensure the Options button is enabled (in case it was disabled in GameStateMainMenu::LoadMainMenuCanvas)
        AZ::EntityId optionsButtonElementId;
        UiCanvasBus::EventResult(optionsButtonElementId,
                                 m_mainMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "OptionsButton");
        UiElementBus::Event(optionsButtonElementId, &UiElementInterface::SetIsEnabled, true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateMainMenu::UnloadMainMenuCanvas()
    {
        GameStateMainMenu::UnloadMainMenuCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const char* StarterGameStateMainMenu::GetMainMenuCanvasAssetPath()
    {
        return "@assets@/ui/menus/mainmenu.uicanvas";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateMainMenu::PlayMainMenuMusicLoop()
    {
        if (!m_isMainMenuMusicLoopPlaying)
        {
            LmbrCentral::AudioSystemComponentRequestBus::Broadcast(
                &LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger,
                "Play_startergame_mainmenu",
                AZ::EntityId());
            m_isMainMenuMusicLoopPlaying = true;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void StarterGameStateMainMenu::StopMainMenuMusicLoop()
    {
        if (m_isMainMenuMusicLoopPlaying)
        {
            LmbrCentral::AudioSystemComponentRequestBus::Broadcast(
                &LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalExecuteAudioTrigger,
                "Stop_startergame_mainmenu",
                AZ::EntityId());
            m_isMainMenuMusicLoopPlaying = false;
        }
    }
}
