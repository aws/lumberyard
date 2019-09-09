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

#include "IGame.h"
#include "IGameObject.h"
#include "ILevelSystem.h"
#include "IInput.h"

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
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        ePlatform_##PublicName,
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
        AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
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
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        #CodeName,
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
        AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
        "Android",
        "iOS"
    };

    /*!
     * Initializes, runs, and handles a game's simulation.
     */
    class CloudGemSamplesGame
        : public IGame
        , public ISystemEventListener
        , public IGameFrameworkListener
        , public ILevelSystemListener
        , public IInputEventListener
    {
    public:
        CloudGemSamplesGame();
        virtual ~CloudGemSamplesGame();

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

        //////////////////////////////////////////////////////////////////////////
        //! IInput
        bool OnInputEvent(const SInputEvent& event) override;
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

    SC_API extern CloudGemSamplesGame* g_Game;
} // namespace LYGame
