
#pragma once

#include "IGame.h"
#include "IGameObject.h"
#include "ILevelSystem.h"

/*!
 * #TODO
 * These defines should be replaced with appropriate name for your game project.
 */
#define GAME_NAME           "EMPTYTEMPLATE"
#define GAME_LONGNAME       "LUMBERYARD EMPTY TEMPLATE"

struct IGameFramework;

namespace LYGame
{
    class IGameInterface;
    class GameRules;
    class Actor;

    /*!
     * Platform types that the game can run on.
     */
    enum Platform
    {
        ePlatform_Unknown,
        ePlatform_PC,
        ePlatform_Xbox, // ACCEPTED_USE
        ePlatform_PS4, // ACCEPTED_USE
        ePlatform_Android,
        ePlatform_iOS,
        ePlatform_Count
    };

    /*!
     * Platform names.
     */
    static char const* s_PlatformNames[ePlatform_Count] =
    {
        "Unknown",
        "PC",
        "Xbox", // ACCEPTED_USE
        "PS4", // ACCEPTED_USE
        "Android",
        "iOS"
    };

    /*!
     * Initializes, runs, and handles a game's simulation.
     */
    class NetworkFeatureTestGame
        : public IGame
        , public ISystemEventListener
        , public IGameFrameworkListener
        , public ILevelSystemListener
    {
    public:
        NetworkFeatureTestGame();
        virtual ~NetworkFeatureTestGame();

        /*!
         * /return a pointer to the game's IGameFramework instance
         */
        IGameFramework* GetGameFramework() { return m_gameFramework; }

        //////////////////////////////////////////////////////////////////////////
        //! IGame
        bool Init(IGameFramework* gameFramework) override;
        bool CompleteInit() override;
        void Shutdown() override;
        int Update(bool hasFocus, unsigned int updateFlags) override;
        void PlayerIdSet(EntityId playerId) override;
        IGameFramework* GetIGameFramework() override { return m_gameFramework; }
        const char* GetLongName() override { return GAME_LONGNAME; }
        const char* GetName() override { return GAME_NAME; }
        EntityId GetClientActorId() const override { return m_clientEntityId; }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! IGameFrameworkListener
        void OnActionEvent(const SActionEvent& event) override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        //////////////////////////////////////////////////////////////////////////
        //! IGame
        void LoadActionMaps(const char* fileName) override;
        //////////////////////////////////////////////////////////////////////////

        void ReleaseActionMaps();
        void SetGameRules(GameRules* rules) { m_gameRules = rules; }

        /*!
         * Reads a profile xml node and initializes ActionMapManager for the current platform.
         * /param[in] rootNode a refernece to profile xml node
         * /return returns true if ActionMapManger was succesfully initialized, false if failed
         */
        bool ReadProfile(const XmlNodeRef& rootNode);

        /*!
         * Reads a profile xml node and adds an input device mapping to ActionMapManager
         * /param[in] platformNode a refernece to profile xml node
         * /param[in] platformId current platform
         * /return returns true if mapping device was added, false if failed
         */
        bool ReadProfilePlatform(const XmlNodeRef& platformsNode, Platform platformId);

        Platform GetPlatform() const;
    protected:
        /*!
         * Platform information as defined in defaultProfile.xml.
         */
        struct PlatformInfo
        {
            Platform    m_platformId;
            BYTE        m_devices;

            PlatformInfo(Platform platformId = ePlatform_Unknown)
                : m_platformId(platformId)
                , m_devices(eAID_KeyboardMouse | eAID_XboxPad | eAID_PS4Pad) { } // ACCEPTED_USE
        };

    protected:
        EntityId                    m_clientEntityId;
        GameRules*                  m_gameRules;
        IGameFramework*             m_gameFramework;
        IActionMap*                 m_defaultActionMap;
        PlatformInfo                m_platformInfo;
    };

    SC_API extern NetworkFeatureTestGame* g_Game;
} // namespace LYGame
