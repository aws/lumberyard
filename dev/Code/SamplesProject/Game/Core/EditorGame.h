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

#include <IGameRef.h>
#include <IEditorGame.h>

namespace LYGame
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Handles the game instance while using Sandbox.
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class EditorGame
        : public IEditorGame
    {
    public:
        EditorGame();
        virtual ~EditorGame() {}

        // IEditorGame
        virtual bool Init(ISystem* system, IGameToEditorInterface* gameToEditorInterface) override;
        virtual int Update(bool hasFocus, unsigned int updateFlags) override;
        virtual void Shutdown() override;
        virtual bool SetGameMode(bool isInGame) override;
        virtual IEntity* GetPlayer() override;
        virtual void SetPlayerPosAng(Vec3 position, Vec3 viewDirection) override;
        virtual void HidePlayer(bool hide) override;
        virtual void OnBeforeLevelLoad() override;
        virtual void OnAfterLevelLoad(const char* levelName, const char* levelFolder) override;
        virtual IFlowSystem* GetIFlowSystem() override;
        virtual IGameTokenSystem* GetIGameTokenSystem() override;
        // ~IEditorGame

    protected:
        enum NetContextConfiguration
        {
            eNetContextConfiguration_Enable,    ///< Enables the net context.
            eNetContextConfiguration_Disable,   ///< Disables the net context.
            eNetContextConfiguration_Reset      ///< Disables, then Enables the net context.
        };

    protected:
        bool ConfigureNetContext(NetContextConfiguration configuration);

    protected:
        /// Reference to the game instance.
        IGameRef m_Game;

        /// Responsible for spinning up the game instance.
        IGameStartup* m_GameStartup;

        /// Are we in-game, or editing?
        bool m_InGame;

        /// Whether or not the net context is currently enabled.
        bool m_NetContextEnabled;
    };
} // namespace LYGame
