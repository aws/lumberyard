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
#define GAME_NAME           "EMPTYTEMPLATE"
#define GAME_LONGNAME       "LUMBERYARD EMPTY TEMPLATE"

struct IGameFramework;

namespace LYGame
{
    class IGameInterface;
    class GameRules;
    class Actor;

    /*!
     * Initializes, runs, and handles a game's simulation.
     */
    class StarterGameGame
        : public IGame
        , public ISystemEventListener
        , public IGameFrameworkListener
        , public ILevelSystemListener
    {
    public:
        StarterGameGame();
        virtual ~StarterGameGame();

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

    protected:
        EntityId                    m_clientEntityId;
        GameRules*                  m_gameRules;
        IGameFramework*             m_gameFramework;
        IActionMap*                 m_defaultActionMap;
    };

    SC_API extern StarterGameGame* g_Game;
} // namespace LYGame
