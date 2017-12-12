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
#include "MetastreamTest/MetastreamTest.h"

#if defined(APPLE)
#define GAME_FRAMEWORK_FILENAME  "libCryAction.dylib"
#elif defined(LINUX)
#define GAME_FRAMEWORK_FILENAME  "libCryAction.so"
#else
#define GAME_FRAMEWORK_FILENAME  "CryAction.dll"
#endif

#define GAME_WINDOW_CLASSNAME    "SamplesProject"

namespace LYGame
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Handles the initialization, running, and shutting down of the game.
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class GameStartup
        : public IGameStartup
        , public ISystemEventListener
        , public IWindowMessageHandler
    {
    public:
        // IGameStartup
        virtual IGameRef Init(SSystemInitParams& startupParams) override;
        virtual void Shutdown() override;
        virtual int Update(bool hasFocus, unsigned int updateFlags) override;
        virtual bool GetRestartLevel(char** levelName) override { return false; }
        virtual const char* GetPatch() const override { return NULL; }
        virtual bool GetRestartMod(char* modName, int nameLengthMax) override { return false; }
        virtual int Run(const char* autoStartLevelName) override;
        // ~IGameStartup

        // ISystemEventListener
        virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        // ~ISystemEventListener

        virtual IGameRef Reset(const char* modName);
        bool InitFramework(SSystemInitParams& startupParams);

        static GameStartup* Create();

    protected:
        GameStartup();
        virtual ~GameStartup();

    private:
        void ShutdownFramework();

#if defined(WIN32)
        bool HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
#endif
        IGameFramework*                             m_Framework;
        IGame*                                      m_Game;
        HMODULE                                     m_FrameworkDll;
        MetastreamTest::HandlerPtr                  m_metaStreamTest;
        bool                                        m_bFrameworkHasInitialized;
    };
} // namespace LYGame