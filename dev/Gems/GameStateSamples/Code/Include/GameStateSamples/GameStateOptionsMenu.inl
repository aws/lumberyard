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

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStateOptionsMenu.h>

#include <LocalUser/LocalUserRequestBus.h>

#include <SaveData/SaveDataRequestBus.h>

#include <LyShine/Bus/UiButtonBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/UiCursorBus.h>
#include <LyShine/Bus/UiSliderBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::OnPushed()
    {
        LoadOptionsMenuCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::OnPopped()
    {
        UnloadOptionsMenuCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::OnEnter()
    {
        GameOptionRequestBus::BroadcastResult(m_gameOptions, &GameOptionRequests::GetGameOptions);
        RefreshOptionsMenuCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::OnExit()
    {
        SaveGameOptionsToPersistentStorage();
        m_gameOptions.reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::LoadOptionsMenuCanvas()
    {
        // Load the UI canvas
        const char* uiCanvasAssetPath = GetOptionsMenuCanvasAssetPath();
        UiCanvasManagerBus::BroadcastResult(m_optionsMenuCanvasEntityId,
                                            &UiCanvasManagerInterface::LoadCanvas,
                                            uiCanvasAssetPath);
        if (!m_optionsMenuCanvasEntityId.IsValid())
        {
            AZ_Warning("GameStateOptionsMenu", false, "Could not load %s", uiCanvasAssetPath);
            return;
        }

        // Display the options menu
        UiCanvasBus::Event(m_optionsMenuCanvasEntityId, &UiCanvasInterface::SetEnabled, true);
        SetOptionsMenuCanvasDrawOrder();

        // Display the UI cursor
        UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);

        // Setup the 'Back' button to return to the previous menu (either the main menu or the pause menu)
        AZ::EntityId backButtonElementId;
        UiCanvasBus::EventResult(backButtonElementId,
                                 m_optionsMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "BackButton");
        UiButtonBus::Event(backButtonElementId,
                           &UiButtonInterface::SetOnClickCallback,
                           [](AZ::EntityId clickedEntityId, AZ::Vector2 point)
        {
            AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStateOptionsMenu>(),
                      "The active game state is not an instance of GameStateOptionsMenu");
            GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
        });
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::UnloadOptionsMenuCanvas()
    {
        if (m_optionsMenuCanvasEntityId.IsValid())
        {
            // Hide the UI cursor
            UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);

            // Unload the pause menu
            UiCanvasManagerBus::Broadcast(&UiCanvasManagerInterface::UnloadCanvas,
                                          m_optionsMenuCanvasEntityId);
            m_optionsMenuCanvasEntityId.SetInvalid();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::RefreshOptionsMenuCanvas()
    {
        if (!m_gameOptions)
        {
            return;
        }

        // Refresh the ambient volume slider
        AZ::EntityId ambientVolumeSliderElementId;
        UiCanvasBus::EventResult(ambientVolumeSliderElementId,
                                 m_optionsMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "AmbientVolumeSlider");
        UiSliderBus::Event(ambientVolumeSliderElementId,
                           &UiSliderInterface::SetValue,
                           m_gameOptions->GetAmbientVolume());
        UiSliderBus::Event(ambientVolumeSliderElementId,
                           &UiSliderInterface::SetValueChangingCallback,
                           [gameOptions = m_gameOptions](AZ::EntityId entityId, float value)
        {
            gameOptions->SetAmbientVolume(value);
        });

        // Refresh the effects volume slider
        AZ::EntityId effectsVolumeSliderElementId;
        UiCanvasBus::EventResult(effectsVolumeSliderElementId,
                                 m_optionsMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "EffectsVolumeSlider");
        UiSliderBus::Event(effectsVolumeSliderElementId,
                           &UiSliderInterface::SetValue,
                           m_gameOptions->GetEffectsVolume());
        UiSliderBus::Event(effectsVolumeSliderElementId,
                           &UiSliderInterface::SetValueChangingCallback,
                           [gameOptions = m_gameOptions](AZ::EntityId entityId, float value)
        {
            gameOptions->SetEffectsVolume(value);
        });

        // Refresh the master volume slider
        AZ::EntityId masterVolumeSliderElementId;
        UiCanvasBus::EventResult(masterVolumeSliderElementId,
                                 m_optionsMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "MasterVolumeSlider");
        UiSliderBus::Event(masterVolumeSliderElementId,
                           &UiSliderInterface::SetValue,
                           m_gameOptions->GetMasterVolume());
        UiSliderBus::Event(masterVolumeSliderElementId,
                           &UiSliderInterface::SetValueChangingCallback,
                           [gameOptions = m_gameOptions](AZ::EntityId entityId, float value)
        {
            gameOptions->SetMasterVolume(value);
        });

        // Refresh the music volume slider
        AZ::EntityId musicVolumeSliderElementId;
        UiCanvasBus::EventResult(musicVolumeSliderElementId,
                                 m_optionsMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "MusicVolumeSlider");
        UiSliderBus::Event(musicVolumeSliderElementId,
                           &UiSliderInterface::SetValue,
                           m_gameOptions->GetMusicVolume());
        UiSliderBus::Event(musicVolumeSliderElementId,
                           &UiSliderInterface::SetValueChangingCallback,
                           [gameOptions = m_gameOptions](AZ::EntityId entityId, float value)
        {
            gameOptions->SetMusicVolume(value);
        });
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::SetOptionsMenuCanvasDrawOrder()
    {
        // Loaded canvases are already stored sorted by draw order...
        UiCanvasManagerInterface::CanvasEntityList canvases;
        UiCanvasManagerBus::BroadcastResult(canvases, &UiCanvasManagerInterface::GetLoadedCanvases);

        // ...so get the draw order of the top-most displayed UI canvas...
        int highestDrawOrder = 0;
        UiCanvasBus::EventResult(highestDrawOrder, canvases.back(), &UiCanvasInterface::GetDrawOrder);

        // ...and increment it by one unless it's already set to int max...
        if (highestDrawOrder != std::numeric_limits<int>::max())
        {
            ++highestDrawOrder;
        }

        // ...ensuring the pause menu gets displayed on top of all other loaded canvases,
        // with the exception of 'special' ones like message popups or the loading screen
        // that use a draw order of int max.
        UiCanvasBus::Event(m_optionsMenuCanvasEntityId, &UiCanvasInterface::SetDrawOrder, highestDrawOrder);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline const char* GameStateOptionsMenu::GetOptionsMenuCanvasAssetPath()
    {
        return "@assets@/ui/canvases/defaultoptionsmenuscreen.uicanvas";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateOptionsMenu::SaveGameOptionsToPersistentStorage()
    {
        SaveData::SaveDataRequests::SaveOrLoadObjectParams<GameOptions> saveObjectParams;
        saveObjectParams.serializableObject = m_gameOptions;
        saveObjectParams.dataBufferName = GameOptions::SaveDataBufferName;
        saveObjectParams.localUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        SaveData::SaveDataRequests::SaveObject(saveObjectParams);
    }
}
