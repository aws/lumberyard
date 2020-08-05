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

#include <GameStateSamples/GameStateMainMenu.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace StarterGame
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class StarterGameStateMainMenu : public GameStateSamples::GameStateMainMenu
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(StarterGameStateMainMenu, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(StarterGameStateMainMenu, "{434ECE7E-8B60-46C3-ADC7-A07D284BABA0}", GameStateMainMenu);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        StarterGameStateMainMenu() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~StarterGameStateMainMenu() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateMainMenu::OnPushed
        void OnPushed() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateMainMenu::OnPopped
        void OnPopped() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateMainMenu::OnEnter
        void OnEnter() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateMainMenu::OnExit
        void OnExit() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateMainMenu::LoadMainMenuCanvas
        void LoadMainMenuCanvas() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateMainMenu::UnloadMainMenuCanvas
        void UnloadMainMenuCanvas() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameStateSamples::GameStateMainMenu::GetMainMenuCanvasAssetPath
        const char* GetMainMenuCanvasAssetPath() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Play the main menu music loop if it is not already playing
        void PlayMainMenuMusicLoop();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Stop the main menu music loop if it is already playing
        void StopMainMenuMusicLoop();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Is the main menu music loop playing?
        bool m_isMainMenuMusicLoopPlaying = false;
    };
} // namespace StarterGame
