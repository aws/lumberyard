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


#include <IGameFramework.h>
#include <IWindowMessageHandler.h>

#include <AzCore/Module/DynamicModuleHandle.h>

namespace LYGame
{
    /*!
     * Handles the initialization, running, and shutting down of the game.
     */
    class GameStartup
        : public IGameStartup
        , public ISystemEventListener
        , public IWindowMessageHandler
    {
    public:
        static GameStartup* Create();

        // IGameStartup
        IGameRef Init(SSystemInitParams& startupParams) override;
        void Shutdown() override;
        // ~IGameStartup

        // ISystemEventListener
        virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        // ~ISystemEventListener

        /*!
         * Re-initializes the Game
         * /return a new instance of LyGame::StarterGameGame() or nullptr if failed to initialize.
         */
        IGameRef Reset();

        /*!
         * Creates an IGameFramework instance, initialize the framework and load CryGame module
         * /param[in] startupParams various parameters related to game startup
         * /return returns true if the game framework initialized, false if failed
         */
        bool InitFramework(SSystemInitParams& startupParams);

    protected:
        GameStartup();
        virtual ~GameStartup();

    private:
        void ShutdownFramework();

#if defined(WIN32)
        bool HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
#endif
        IGameFramework*         m_Framework;
        IGame*                  m_Game;
    };
} // namespace LYGame