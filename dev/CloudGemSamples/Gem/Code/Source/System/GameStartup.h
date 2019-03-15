/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
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
#include <IPlatformOS.h>

#if defined(APPLE)
#define GAME_FRAMEWORK_FILENAME  "libCryAction.dylib"
#elif defined(LINUX)
#define GAME_FRAMEWORK_FILENAME  "libCryAction.so"
#else
#define GAME_FRAMEWORK_FILENAME  "CryAction.dll"
#endif

#define GAME_WINDOW_CLASSNAME    "CloudGemSamples"

namespace LYGame
{
    /*!
     * Handles the initialization, running, and shutting down of the game.
     */
    class GameStartup
        : public IGameStartup
        , public ISystemEventListener
        , public IWindowMessageHandler
        , public IPlatformOS::IPlatformListener
    {
    public:
        friend class CloudGemSamplesSystemComponent;
        static GameStartup* Create();

        // IGameStartup
        IGameRef Init(SSystemInitParams& startupParams) override;
        void Shutdown() override;
        // ~IGameStartup

        // ISystemEventListener
        virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        // ~ISystemEventListener

        // IPlatformOS::IPlatformListener
        void OnPlatformEvent(const IPlatformOS::SPlatformEvent& event);
        // ~IPlatformOS::IPlatformListener

        /*!
         * Re-initializes the Game
         * /return a new instance of LyGame::CloudGemSamplesGame() or nullptr if failed to initialize.
         */
        IGameRef Reset();

    protected:
        GameStartup();
        virtual ~GameStartup();

    private:

#if defined(WIN32)
        bool HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
#endif
        void ExecuteAutoExec();

        IGameFramework*         m_Framework;
        IGame*                  m_Game;
        bool                    m_bExecutedAutoExec;
    };
} // namespace LYGame
