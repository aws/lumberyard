
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
        static GameStartup* Create();

        // IGameStartup
        IGameRef Init(SSystemInitParams& startupParams) override;
        void Shutdown() override;
        int Update(bool hasFocus, unsigned int updateFlags) override;
        int Run(const char* autoStartLevelName) override;
        // ~IGameStartup

        // ISystemEventListener
        virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        // ~ISystemEventListener

        // IPlatformOS::IPlatformListener
        void OnPlatformEvent(const IPlatformOS::SPlatformEvent& event);
        // ~IPlatformOS::IPlatformListener

        /*!
         * Re-initializes the Game
         * /return a new instance of LyGame::NetworkFeatureTestGame() or nullptr if failed to initialize.
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
        void ExecuteAutoExec();

        IGameFramework*         m_Framework;
        IGame*                  m_Game;
        HMODULE                 m_FrameworkDll;
        bool                    m_bExecutedAutoExec;
    };
} // namespace LYGame