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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : The game engine for editor


#ifndef CRYINCLUDE_EDITOR_GAMEENGINE_H
#define CRYINCLUDE_EDITOR_GAMEENGINE_H

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include "LogFile.h"
#include "CryListenerSet.h"

struct IEditorGame;
class CStartupLogoDialog;
struct IFlowSystem;
struct IGameTokenSystem;
struct IEquipmentSystemInterface;
struct IInitializeUIInfo;
class CNavigation;

class IGameEngineListener
{
public:
    virtual void OnPreEditorUpdate() = 0;
    virtual void OnPostEditorUpdate() = 0;
};


class ThreadedOnErrorHandler : public QObject
{
    Q_OBJECT
public:
    explicit ThreadedOnErrorHandler(ISystemUserCallback* callback);
    ~ThreadedOnErrorHandler();

public slots:
    bool OnError(const char* error);

private:
    ISystemUserCallback* m_userCallback;
};


//! This class serves as a high-level wrapper for CryEngine game.
class SANDBOX_API CGameEngine
    : public IEditorNotifyListener
{
public:
    CGameEngine();
    ~CGameEngine(void);
    //! Initialize System.
    //! @return successful outcome if initialization succeeded. or failed outcome with error message.
    AZ::Outcome<void, AZStd::string> Init(
        bool bPreviewMode,
        bool bTestMode,
        bool bShaderCacheGen,
        const char* sCmdLine,
        IInitializeUIInfo* logo,
        HWND hwndForInputSystem);
    //! Initialize game.
    //! @return true if initialization succeeded, false otherwise
    bool InitGame(const char* sGameDLL);
    //! Load new terrain level into 3d engine.
    //! Also load AI triangulation for this level.
    bool LoadLevel(
        const QString& mission,
        bool bDeleteAIGraph,
        bool bReleaseResources);
    //!* Reload level if it was already loaded.
    bool ReloadLevel();
    //! Loads AI triangulation.
    bool LoadAI(const QString& levelName, const QString& missionName);
    //! Load new mission.
    bool LoadMission(const QString& mission);
    //! Reload environment settings in currently loaded level.
    bool ReloadEnvironment();
    //! Request to switch In/Out of game mode on next update.
    //! The switch will happen when no sub systems are currently being updated.
    //! @param inGame When true editor switch to game mode.
    void RequestSetGameMode(bool inGame);
    //! Switch In/Out of AI and Physics simulation mode.
    //! @param enabled When true editor switch to simulation mode.
    void SetSimulationMode(bool enabled, bool bOnlyPhysics = false);
    //! Get current simulation mode.
    bool GetSimulationMode() const { return m_bSimulationMode; };
    //! Returns true if level is loaded.
    bool IsLevelLoaded() const { return m_bLevelLoaded; };
    //! Assign new level path name.
    void SetLevelPath(const QString& path);
    //! Assign new current mission name.
    void SetMissionName(const QString& mission);
    //! Return name of currently loaded level.
    const QString& GetLevelName() const { return m_levelName; };
    //! Return extension of currently loaded level.
    const QString& GetLevelExtension() const { return m_levelExtension; };
    //! Return name of currently active mission.
    const QString& GetMissionName() const { return m_missionName; };
    //! Get fully specified level path.
    const QString& GetLevelPath() const { return m_levelPath; };
    //! Query if engine is in game mode.
    bool IsInGameMode() const { return m_bInGameMode; };
    //! Force level loaded variable to true.
    void SetLevelLoaded(bool bLoaded) { m_bLevelLoaded = bLoaded; }
    //! Force level just created variable to true.
    void SetLevelCreated(bool bJustCreated) { m_bJustCreated = bJustCreated; }
    //! Generate All AI data
    void GenerateAiAll();
    //! Generate AI Triangulation of currently loaded data.
    void GenerateAiTriangulation();
    //! Generate AI Waypoint of currently loaded data.
    void GenerateAiWaypoint();
    //! Generate AI Flight navigation of currently loaded data.
    void GenerateAiFlightNavigation();
    //! Generate AI 3D nav data of currently loaded data.
    void GenerateAiNavVolumes();
    //! Generate AI MNM navigation of currently loaded data.
    void GenerateAINavigationMesh();
    //! Generate AI Cover Surfaces of currently loaded data.
    void GenerateAICoverSurfaces();
    //! Loading all the AI navigation data of current level.
    void LoadAINavigationData();
    //! Does some basic sanity checking of the AI navigation
    void ValidateAINavigation();
    //! Clears all AI navigation data from the current level.
    void ClearAllAINavigation();
    //! Generates 3D volume voxels only for debugging
    void Generate3DDebugVoxels();
    void ExportAiData(
        const char* navFileName,
        const char* areasFileName,
        const char* roadsFileName,
        const char* fileNameVerts,
        const char* fileNameVolume,
        const char* fileNameFlight);
    CNavigation* GetNavigation();
    //! Sets equipment pack current used by player.
    void SetPlayerEquipPack(const char* sEqipPackName);
    //! Query ISystem interface.
    ISystem* GetSystem() { return m_pISystem; };
    //! Set player position in game.
    //! @param bEyePos If set then given position is position of player eyes.
    void SetPlayerViewMatrix(const Matrix34& tm, bool bEyePos = true);
    //! When set, player in game will be every frame synchronized with editor camera.
    void SyncPlayerPosition(bool bEnable);
    bool IsSyncPlayerPosition() const { return m_bSyncPlayerPosition; };
    void HideLocalPlayer(bool bHide);
    //! Set game's current Mod name.
    void SetCurrentMOD(const char* sMod);
    //! Returns game's current Mod name.
    QString GetCurrentMOD() const;
    //! Called every frame.
    void Update();
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    struct IEntity* GetPlayerEntity();
    // Retrieve pointer to the game flow system implementation.
    IFlowSystem* GetIFlowSystem() const;
    IGameTokenSystem* GetIGameTokenSystem() const;
    IEquipmentSystemInterface* GetIEquipmentSystemInterface() const;
    IEditorGame* GetIEditorGame() const { return m_pEditorGame; }
    //! When enabled flow system will be updated at editing mode.
    void EnableFlowSystemUpdate(bool bEnable)
    {
        m_bUpdateFlowSystem = bEnable;

        const EEditorNotifyEvent event = bEnable ? eNotify_OnEnableFlowSystemUpdate : eNotify_OnDisableFlowSystemUpdate;
        GetIEditor()->Notify(event);
    }

    bool IsFlowSystemUpdateEnabled() const { return m_bUpdateFlowSystem; }
    void LockResources();
    void UnlockResources();
    void ResetResources();
    bool SupportsMultiplayerGameRules();
    void ToggleMultiplayerGameRules();
    bool BuildEntitySerializationList(XmlNodeRef output);
    void OnTerrainModified(const Vec2& modPosition, float modAreaRadius, bool fullTerrain);
    void OnAreaModified(const AABB& modifiedArea);

    void ExecuteQueuedEvents();

    //! mutex used by other threads to lock up the PAK modification,
    //! so only one thread can modify the PAK at once
    static CryMutex& GetPakModifyMutex()
    {
        //! mutex used to halt copy process while the export to game
        //! or other pak operation is done in the main thread
        static CryMutex s_pakModifyMutex;
        return s_pakModifyMutex;
    }

    inline HMODULE GetGameModule() const
    {
        return m_gameDll;
    }

private:
    void SetGameMode(bool inGame);
    void SwitchToInGame();
    void SwitchToInEditor();
    static void HandleQuitRequest(IConsoleCmdArgs*);

    CLogFile m_logFile;
    QString m_levelName;
    QString m_levelExtension;
    QString m_missionName;
    QString m_levelPath;
    QString m_MOD;
    bool m_bLevelLoaded;
    bool m_bInGameMode;
    bool m_bSimulationMode;
    bool m_bSimulationModeAI;
    bool m_bSyncPlayerPosition;
    bool m_bUpdateFlowSystem;
    bool m_bJustCreated;
    bool m_bIgnoreUpdates;
    ISystem* m_pISystem;
    IAISystem* m_pIAISystem;
    IEntitySystem* m_pIEntitySystem;
    CNavigation* m_pNavigation;
    Matrix34 m_playerViewTM;
    struct SSystemUserCallback* m_pSystemUserCallback;
    HMODULE m_hSystemHandle;
    HMODULE m_gameDll;
    IEditorGame* m_pEditorGame;
    class CGameToEditorInterface* m_pGameToEditorInterface;
    enum EPendingGameMode
    {
        ePGM_NotPending,
        ePGM_SwitchToInGame,
        ePGM_SwitchToInEditor,
    };
    EPendingGameMode m_ePendingGameMode;
};


#endif // CRYINCLUDE_EDITOR_GAMEENGINE_H
