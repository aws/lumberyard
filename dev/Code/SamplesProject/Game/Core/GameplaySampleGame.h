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

#define GAME_NAME           "GAMEPLAYSAMPLE"
#define GAME_LONGNAME       "LUMBERYARD GAMEPLAY SAMPLE PROJECT"

struct IGameFramework;

namespace LYGame
{
    class IGameInterface;
    class GameRules;
    class Actor;

    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Platform types that the game can run on.
    ///
    //////////////////////////////////////////////////////////////////////////////////////
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

    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Initializes, runs, and handles a game's simulation.
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class GameplaySampleGame
        : public IGame
        , public ISystemEventListener
        , public IGameFrameworkListener
        , public ILevelSystemListener
    {
    public:
        GameplaySampleGame();
        virtual ~GameplaySampleGame();

        /// Returns Entity ID of the Actor being controlled by the local client.
        EntityId GetClientActorId() const override { return m_clientEntityId; }

        /// Returns IGameFramwork.
        IGameFramework* GetGameFramework() { return m_gameFramework; }

        // IGame overrides.
        bool Init(IGameFramework* gameFramework) override;
        bool CompleteInit() override;
        void Shutdown() override;
        int Update(bool hasFocus, unsigned int updateFlags) override;
        void PlayerIdSet(EntityId playerId) override;
        IGameFramework* GetIGameFramework() override { return m_gameFramework; }
        const char* GetLongName() override { return GAME_LONGNAME; }
        const char* GetName() override { return GAME_NAME; }
        void GetMemoryStatistics(ICrySizer* sizer) override;

        // IGame stubs. Fill these out as necessary for your project.
        void EditorResetGame(bool start) override { }
        void OnClearPlayerIds() override { }
        TSaveGameName CreateSaveGameName() override { return NULL; }
        const char* GetMappedLevelName(const char* levelName) const override { return ""; }
        IGameStateRecorder* CreateGameStateRecorder(IGameplayListener* gameplayListener) override { return NULL; }
        const bool DoInitialSavegame() const override { return true; }
        THUDWarningId AddGameWarning(const char* stringId, const char* paramMessage, IGameWarningsListener* gameWarningsListener = NULL) override { return 0; }
        void RemoveGameWarning(const char* stringId) override { }
        void RenderGameWarnings() override { }
        void OnRenderScene(const SRenderingPassInfo& renderingPassInfo) override { }
        bool GameEndLevel(const char* stringId) override { return false; }
        void SetUserProfileChanged(bool hasChanged) override { }
        ExportFilesInfo ExportLevelData(const char* levelName, const char* missionName) const override { return ExportFilesInfo("NoName", 0); }
        void LoadExportedLevelData(const char* levelName, const char* missionName) override { }
        void FullSerialize(TSerialize serializer) override { }
        void PostSerialize() override { }
        IAntiCheatManager* GetAntiCheatManager() override { return NULL; }
        void* GetGameInterface(void) override { return nullptr; }
        void OnLoadingLevelEntitiesStart(ILevelInfo*) override { }
        void InitEditor(IGameToEditorInterface*) override { }

        // IGameFrameworkListener
        void OnActionEvent(const SActionEvent& event) override;

        // IGameFrameworkListener stubs. Fill these out as necessary for your project.
        void OnPostUpdate(float frameTime) override { }
        void OnSaveGame(ISaveGame* saveGame) override { }
        void OnLoadGame(ILoadGame* loadGame) override { }
        void OnLevelEnd(const char* nextLevel) override { }

    protected:
        // ISystemEventListener
        void OnSystemEvent(ESystemEvent event, UINT_PTR wParam, UINT_PTR lParam) override;

        // ILevelSystemListener stubs. Fill these out as necessary for your project.
        void OnLevelNotFound(const char* levelName) override { }
        void OnLoadingStart(ILevelInfo* levelInfo) override { }
        void OnLoadingComplete(ILevel* level) override { }
        void OnLoadingError(ILevelInfo* levelInfo, const char* error) override { }
        void OnLoadingProgress(ILevelInfo* levelInfo, int progressAmount) override { }
        void OnUnloadComplete(ILevel* level) override { }

        // IGame
        void LoadActionMaps(const char* fileName) override;

        // IGame stubs. Fill these out as necessary for your project.
        void RegisterGameFlowNodes() override { }

        void ReleaseActionMaps();
        void SetGameRules(GameRules* rules) { m_gameRules = rules; }
        bool ReadProfile(XmlNodeRef& rootNode);
        bool ReadProfilePlatform(XmlNodeRef& platformsNode, LYGame::Platform platformId);

        LYGame::Platform GetPlatform() const;
        template<typename T>
        void ForEachExtension(const T& functor);
        template<typename T>
        void ForEachExtension(const T& functor) const;

    protected:
        /// Platform information as defined in defaultProfile.xml.
        struct PlatformInfo
        {
            LYGame::Platform    PlatformId;
            BYTE        Devices;    ///< Devices to use when registering actions

            PlatformInfo(LYGame::Platform platformId = ePlatform_Unknown)
                : PlatformId(platformId)
                , Devices(eAID_KeyboardMouse | eAID_XboxPad | eAID_PS4Pad) { }
        };

    protected:
        EntityId                    m_clientEntityId;
        GameRules*                  m_gameRules;
        IGameFramework*             m_gameFramework;
        IActionMap*                 m_defaultActionMap;
        PlatformInfo                m_platformInfo;
    };

    SC_API extern GameplaySampleGame* g_Game;
} // namespace LYGame