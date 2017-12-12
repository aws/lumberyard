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

#include "IGame.h"
#include "IGameObject.h"
#include "ILevelSystem.h"

/*!
 * #TODO
 * These defines should be replaced with appropriate name for your game project.
 */
#define GAME_NAME           "FEATURETESTS"
#define GAME_LONGNAME       "LUMBERYARD FEATURE TESTS"

struct IGameFramework;

namespace LYGame
{
    class ScriptBind_Game;
    class IGameInterface;
    class GameCache;
    class GameRules;
    class Actor;

    /*!
     * Platform types that the game can run on.
     */
    enum Platform
    {
        ePlatform_Unknown,
        ePlatform_PC,
        ePlatform_Xbox,
        ePlatform_PS4,
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
        "Xbox",
        "PS4",
        "Android",
        "iOS"
    };

    /*!
     * Initializes, runs, and handles a game's simulation.
     */
    class FeatureTestsGame
        : public IGame
        , public ISystemEventListener
        , public IGameFrameworkListener
        , public ILevelSystemListener
    {
    public:
        FeatureTestsGame();
        virtual ~FeatureTestsGame();

        /*!
         * /return a pointer to the game's IGameFramework instance
         */
        IGameFramework* GetGameFramework() { return m_gameFramework; }

        /// Returns the game cache.
        GameCache& GetGameCache() { CRY_ASSERT_MESSAGE(m_gameCache, "Can't obtain GameCache object until Game::Init() is called!"); return *(m_gameCache); }

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

        //////////////////////////////////////////////////////////////////////////
        //! ISystemEventListener
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
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
                , m_devices(eAID_KeyboardMouse | eAID_XboxPad | eAID_PS4Pad) { }
        };

    protected:
        EntityId                    m_clientEntityId;
        GameRules*                  m_gameRules;
        IGameFramework*             m_gameFramework;
        IActionMap*                 m_defaultActionMap;
        ScriptBind_Game*            m_scriptBind;
        GameCache*                  m_gameCache;
        PlatformInfo                m_platformInfo;
    };

    SC_API extern FeatureTestsGame* g_Game;
} // namespace LYGame