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

// Description : Implementation of the IGameFramework interface. CCryAction
//               provides a generic game framework for action based games
//               such as 1st and 3rd person shooters.

#ifndef CRYINCLUDE_CRYACTION_CRYACTION_H
#define CRYINCLUDE_CRYACTION_CRYACTION_H
#pragma once


#include <ISystem.h>
#include <ICmdLine.h>

#include "IGameFramework.h"
#include "ICryPak.h"
#include "ISaveGame.h"
#include "ITestSystem.h"

struct IFlowSystem;
struct IGameTokenSystem;
struct IEffectSystem;
struct IBreakableGlassSystem;
struct IForceFeedbackSystem;
struct IGameVolumes;

class CAIDebugRenderer;
class CAINetworkDebugRenderer;
class CGameRulesSystem;
class CScriptBind_Action;
class CScriptBind_ActorSystem;
class CScriptBind_ItemSystem;
class CScriptBind_ActionMapManager;
class CScriptBind_Network;
class CScriptBind_VehicleSystem;
class CScriptBind_Vehicle;
class CScriptBind_VehicleSeat;
class CScriptBind_Inventory;
class CScriptBind_DialogSystem;
class CScriptBind_MaterialEffects;
class CScriptBind_PrefabManager;


class CFlowSystem;
class CDevMode;
class CTimeDemoRecorder;
class CGameQueryListener;
class CScriptRMI;
class CGameSerialize;
class CMaterialEffects;
class CMaterialEffectsCVars;
class CGameObjectSystem;
class CActionMapManager;
class CActionGame;
class CActorSystem;
class CallbackTimer;
class CGameContext;
class CItemSystem;
class CLevelSystem;
class CUIDraw;
class CVehicleSystem;
class CViewSystem;
class CGameplayRecorder;
class CPersistentDebug;
class CPlayerProfileManager;
class CDialogSystem;
class CSubtitleManager;
class CGameplayAnalyst;
class CTimeOfDayScheduler;
class CNetworkCVars;
class CCryActionCVars;
class CSignalTimer;
class CRangeSignaling;
class CVisualLog;
class CAIProxy;
class CommunicationVoiceLibrary;
class CCustomActionManager;
class CCustomEventManager;
class CAIProxyManager;
class CForceFeedBackSystem;
class CCryActionPhysicQueues;
class CNetworkStallTickerThread;
class CSharedParamsManager;
struct ICooperativeAnimationManager;
class CRuntimeAreaManager;
class CPrefabManager;

struct CAnimationGraphCVars;
struct IRealtimeRemoteUpdate;
struct ISerializeHelper;

class CSegmentedWorld;
class DebugCamera;

class CGameContextBridge;

namespace Graphics
{
    class CColorGradientManager;
    class ScreenFaderManager;
}

class CCryAction
    : public IGameFramework
{
public:
    CCryAction();
    virtual ~CCryAction();

    // IGameFramework
    void ClearTimers();
    TimerID AddTimer(CTimeValue interval, bool repeat, TimerCallback callback, void* userdata) override;
    void* RemoveTimer(TimerID timerID) override;

    uint32 GetPreUpdateTicks() override;

    void RegisterFactory(const char* name, IActorCreator* pCreator, bool isAI) override;
    void RegisterFactory(const char* name, IItemCreator* pCreator, bool isAI) override;
    void RegisterFactory(const char* name, IVehicleCreator* pCreator, bool isAI) override;
    void RegisterFactory(const char* name, IGameObjectExtensionCreator* pCreator, bool isAI) override;
    void RegisterFactory(const char* name, ISaveGame*(*func)(), bool) override;
    void RegisterFactory(const char* name, ILoadGame*(*func)(), bool) override;

    bool Init(SSystemInitParams& startupParams) override;
    void InitGameType(bool multiplayer, bool fromInit) override;
    bool CompleteInit() override;
    void Shutdown() override;
    bool PreUpdate(bool haveFocus, unsigned int updateFlags) override;
    bool PostUpdate(bool haveFocus, unsigned int updateFlags) override;
    void Reset(bool clients) override;
    void GetMemoryUsage(ICrySizer* pSizer) const override;

    void PauseGame(bool pause, bool force, unsigned int nFadeOutInMS = 0) override;
    bool IsGamePaused() override;
    bool IsGameStarted() override;
    void MarkGameStarted() override;
    bool IsLoadingSaveGame() override;
    const char* GetLevelName() override;
    const char* GetAbsLevelPath(char* const pPath, const uint32 cPathMaxLen) override;
    bool IsInTimeDemo() override;    // Check if time demo is in progress (either playing or recording)
    bool IsTimeDemoRecording() override; // Check if time demo is recording

    bool IsLevelPrecachingDone() const override;
    void SetLevelPrecachingDone(bool bValue) override;

    void HandleGridMateScriptRMI(TSerialize serialize, ChannelId toChannelId, ChannelId avoidChannelId) override;
    void SerializeScript(TSerialize ser, IEntity* entity) override;

    void NetMarshalScript(Serialization::INetScriptMarshaler* scriptMarshaler, IEntity* entity) override;
    void NetUnmarshalScript(TSerialize ser, IEntity* entity) override;

    ISystem* GetISystem() override { return m_pSystem; }
    ILanQueryListener* GetILanQueryListener() override {return m_pLanQueryListener; }
    IUIDraw* GetIUIDraw() override;
    IMannequin& GetMannequinInterface() override;
    ILevelSystem* GetILevelSystem() override;
    IActorSystem* GetIActorSystem() override;
    IItemSystem* GetIItemSystem() override;
    IVehicleSystem* GetIVehicleSystem() override;
    IActionMapManager* GetIActionMapManager() override;
    IViewSystem* GetIViewSystem() override;
    IGameplayRecorder* GetIGameplayRecorder() override;
    IGameRulesSystem* GetIGameRulesSystem() override;
    IGameObjectSystem* GetIGameObjectSystem() override;
    IFlowSystem* GetIFlowSystem() override;
    IGameTokenSystem* GetIGameTokenSystem() override;
    IEffectSystem* GetIEffectSystem() override;
    IMaterialEffects* GetIMaterialEffects() override;
    IPlayerProfileManager* GetIPlayerProfileManager() override;
    ISubtitleManager* GetISubtitleManager() override;
    IDialogSystem* GetIDialogSystem() override;
    ICooperativeAnimationManager* GetICooperativeAnimationManager() override;
    ICheckpointSystem* GetICheckpointSystem() override;
    IForceFeedbackSystem* GetIForceFeedbackSystem() const override;
    ICustomActionManager* GetICustomActionManager() const override;
    ICustomEventManager* GetICustomEventManager() const override;
    IRealtimeRemoteUpdate* GetIRealTimeRemoteUpdate() override;
    IGamePhysicsSettings* GetIGamePhysicsSettings() override;
    const char* GetStartLevelSaveGameName() override;

    bool StartGameContext(const SGameStartParams* pGameStartParams) override;
    bool ChangeGameContext(const SGameContextParams* pGameContextParams) override;
    void EndGameContext(bool loadEmptyLevel) override;
    bool StartedGameContext() const override;
    bool StartingGameContext() const override;
    bool BlockingSpawnPlayer() override;

    void ResetBrokenGameObjects() override;
    void CloneBrokenObjectsAndRevertToStateAtTime(int32 iFirstBreakEventIndex, 
        uint16* pBreakEventIndices, 
        int32& iNumBreakEvents, 
        IRenderNode** outClonedNodes, 
        int32& iNumClonedNodes, 
        SRenderNodeCloneLookup& renderNodeLookup) override;
    void ApplySingleProceduralBreakFromEventIndex(uint16 uBreakEventIndex, const SRenderNodeCloneLookup& renderNodeLookup) override;
    void UnhideBrokenObjectsByIndex(uint16* ObjectIndicies, int32 iNumObjectIndices) override;

    void Serialize(TSerialize ser); // defined in ActionGame.cpp
    void FlushBreakableObjects() override;  // defined in ActionGame.cpp
    void ClearBreakHistory();

    void InitEditor(IGameToEditorInterface* pGameToEditor) override;
    void SetEditorLevel(const char* levelName, const char* levelFolder) override;
    void GetEditorLevel(char** levelName, char** levelFolder) override;

    IActor* GetClientActor() const override;
    EntityId GetClientActorId() const override;
    IEntity* GetClientEntity() const override;
    void SetClientActor(EntityId id, bool setupActionMaps = true) override;
    CTimeValue GetServerTime() override;
    IGameObject* GetGameObject(EntityId id) override;
    bool GetNetworkSafeClassId(uint16& id, const char* className) override;
    bool GetNetworkSafeClassName(char* className, size_t classNameSizeInBytes, uint16 id) override;
    IGameObjectExtension* QueryGameObjectExtension(EntityId id, const char* name) override;

    bool SaveGame(const char* path, 
        bool bQuick = false, 
        bool bForceImmediate = false, 
        ESaveGameReason reason = eSGR_QuickSave, 
        bool ignoreDelay = false, 
        const char* checkpointName = NULL) override;
    ELoadGameResult LoadGame(const char* path, bool quick = false, bool ignoreDelay = false) override;
    void ScheduleEndLevelNow(const char* nextLevel, bool clearRenderBufferToBlack = true) override;

    void OnEditorSetGameMode(int iMode) override;
    bool IsEditing() override {return m_isEditing; }

    void OnBreakageSpawnedEntity(IEntity* pEntity, IPhysicalEntity* pPhysEntity, IPhysicalEntity* pSrcPhysEntity) override;

    bool IsImmersiveMPEnabled();

    void AllowSave(bool bAllow = true) override
    {
        m_bAllowSave = bAllow;
    }

    void AllowLoad(bool bAllow = true) override
    {
        m_bAllowLoad = bAllow;
    }

    bool CanSave() override;
    bool CanLoad() override;

    ISerializeHelper* GetSerializeHelper() const override;

    bool CanCheat() override;

    void SetGameGUID(const char* gameGUID);
    const char* GetGameGUID() { return m_gameGUID; }

    bool IsVoiceRecordingEnabled() override {return m_VoiceRecordingEnabled != 0; }

    ISharedParamsManager* GetISharedParamsManager() override;
    float GetLoadSaveDelay() const override {return m_lastSaveLoad; }

    IGameVolumes* GetIGameVolumesManager() const override;
    std::shared_ptr<IPrefabManager> GetIPrefabManager() const override;

    void PreloadAnimatedCharacter(IScriptTable* pEntityScript) override;
    void PrePhysicsTimeStep(float deltaTime) override;

    void RegisterExtension(ICryUnknownPtr pExtension) override;
    void ReleaseExtensions() override;

    IBreakableGlassSystem* GetIBreakableGlassSystem();
    void ScheduleEndLevel(const char* nextLevel = "", bool clearRenderBufferToBlack = true);
protected:
    ICryUnknownPtr QueryExtensionInterfaceById(const CryInterfaceID& interfaceID) const override;
    // ~IGameFramework

public:

    static CCryAction* GetCryAction() { return m_pThis; }

    CGameContext* GetGameContext();
    CScriptBind_Vehicle* GetVehicleScriptBind() { return m_pScriptBindVehicle; }
    CScriptBind_VehicleSeat* GetVehicleSeatScriptBind() { return m_pScriptBindVehicleSeat; }
    CScriptBind_Inventory* GetInventoryScriptBind() { return m_pScriptInventory; }
    CPersistentDebug* GetPersistentDebug() { return m_pPersistentDebug; }
    CSignalTimer* GetSignalTimer();
    CRangeSignaling* GetRangeSignaling();
    IPersistentDebug* GetIPersistentDebug() override;
    IVisualLog* GetIVisualLog() override;
    CVisualLog* GetVisualLog() { return m_pVisualLog; }
    CGameContextBridge* GetGameContextBridge() { return m_contextBridge; }
    Graphics::CColorGradientManager* GetColorGradientManager() { return m_colorGradientManager; }

#ifndef _RELEASE
    DebugCamera* GetDebugCamera() override { return m_debugCamera; }
#endif

    void    AddBreakEventListener(IBreakEventListener* pListener) override;
    void    RemoveBreakEventListener(IBreakEventListener* pListener) override;

    void OnBreakEvent(uint16 uBreakEventIndex);
    void OnPartRemoveEvent(int32 iPartRemoveEventIndex);

    void RegisterListener       (IGameFrameworkListener* pGameFrameworkListener, const char* name, EFRAMEWORKLISTENERPRIORITY eFrameworkListenerPriority) override;
    void UnregisterListener (IGameFrameworkListener* pGameFrameworkListener) override;

    CDialogSystem* GetDialogSystem() { return m_pDialogSystem; }
    CTimeOfDayScheduler* GetTimeOfDayScheduler() const { return m_pTimeOfDayScheduler; }

    IGameStatistics* GetIGameStatistics();

    bool LoadingScreenEnabled() const;

    int NetworkExposeClass(IFunctionHandler* pFH);

    void NotifyGameFrameworkListeners(ISaveGame* pSaveGame);
    void NotifyGameFrameworkListeners(ILoadGame* pLoadGame);
    void NotifySavegameFileLoadedToListeners(const char* pLevelName);
    void NotifyForceFlashLoadingListeners();
    void EnableVoiceRecording(const bool enable) override;
    IDebugHistoryManager* CreateDebugHistoryManager() override;
    void ExecuteCommandNextFrame(const char* cmd) override;
    const char* GetNextFrameCommand() const override;
    void ClearNextFrameCommand() override;
    void PrefetchLevelAssets(const bool bEnforceAll) override;

    void ShowPageInBrowser(const char* URL) override;
    bool StartProcess(const char* cmd_line) override;
    bool SaveServerConfig(const char* path) override;

    void  OnActionEvent(const SActionEvent& ev);

    void DumpMemInfo(const char* format, ...) PRINTF_PARAMS(2, 3);
    
    IAIActorProxy* GetAIActorProxy(EntityId entityid) const;
    CAIProxyManager* GetAIProxyManager() { return m_pAIProxyManager; }
    const CAIProxyManager* GetAIProxyManager() const { return m_pAIProxyManager; }

    void CreatePhysicsQueues();
    void ClearPhysicsQueues();
    virtual CCryActionPhysicQueues& GetPhysicQueues();

    void SwitchToLevelHeap(const char* acLevelName);

    void StartNetworkStallTicker(bool includeMinimalUpdate);
    void StopNetworkStallTicker();

    inline bool ShouldClearRenderBufferToBlackOnUnload()
    {
        return !m_pLocalAllocs || m_pLocalAllocs->m_clearRenderBufferToBlackOnUnload;
    }

private:
    void InitScriptBinds();
    void ReleaseScriptBinds();
    void InitForceFeedbackSystem();
    void InitGameVolumesManager();
    void SetupActionMaps();
    void SetupLocalView();

    void InitCVars();
    void ReleaseCVars();

    void InitCommands();

    // TODO: remove
    static void FlowTest(IConsoleCmdArgs*);

    // console commands provided by CryAction
    static void DumpMapsCmd(IConsoleCmdArgs* args);
    static void MapCmd(IConsoleCmdArgs* args);
    static void ReloadReadabilityXML(IConsoleCmdArgs* args);
    static void UnloadCmd(IConsoleCmdArgs* args);
    static void PlayCmd(IConsoleCmdArgs* args);
    static void VersionCmd(IConsoleCmdArgs* args);
    static void SaveTagCmd(IConsoleCmdArgs* args);
    static void LoadTagCmd(IConsoleCmdArgs* args);
    static void SaveGameCmd(IConsoleCmdArgs* args);
    static void GenStringsSaveGameCmd(IConsoleCmdArgs* args);
    static void LoadGameCmd(IConsoleCmdArgs* args);
    static void OpenURLCmd(IConsoleCmdArgs* args);
    static void TestResetCmd(IConsoleCmdArgs* args);

    static void DumpAnalysisStatsCmd(IConsoleCmdArgs* args);

    static void TestTimeout(IConsoleCmdArgs* args);
    static void TestNSServerBrowser(IConsoleCmdArgs* args);
    static void TestNSServerReport(IConsoleCmdArgs* args);
    static void TestNSChat(IConsoleCmdArgs* args);
    static void TestNSStats(IConsoleCmdArgs* args);
    static void TestNSNat(IConsoleCmdArgs* args);
    static void DumpStatsCmd(IConsoleCmdArgs* args);

    // console commands for the remote control system
    static void rcon_startserver(IConsoleCmdArgs* args);
    static void rcon_stopserver(IConsoleCmdArgs* args);
    static void rcon_connect(IConsoleCmdArgs* args);
    static void rcon_disconnect(IConsoleCmdArgs* args);
    static void rcon_command(IConsoleCmdArgs* args);

    static struct IRemoteControlServer* s_rcon_server;
    static struct IRemoteControlClient* s_rcon_client;

    static class CRConClientListener* s_rcon_client_listener;

    void CheckEndLevelSchedule();


    static void VerifyMaxPlayers(ICVar* pVar);
    static void ResetComments(ICVar* pVar);

    static void StaticSetPbSvEnabled(IConsoleCmdArgs* pArgs);
    static void StaticSetPbClEnabled(IConsoleCmdArgs* pArgs);

    // NOTE: anything owned by this class should be a pointer or a simple
    // type - nothing that will need its constructor called when CryAction's
    // constructor is called (we don't have access to malloc() at that stage)

    bool                            m_paused;
    bool                            m_forcedpause;

    bool                            m_levelPrecachingDone;
    bool                            m_usingLevelHeap;

    static CCryAction* m_pThis;

    ISystem* m_pSystem;
    INetwork* m_pNetwork;
    I3DEngine* m_p3DEngine;
    IScriptSystem* m_pScriptSystem;
    IEntitySystem* m_pEntitySystem;
    ITimer* m_pTimer;
    ILog* m_pLog;
    void* m_systemDll;

    _smart_ptr<CActionGame>       m_pGame;

    char                            m_editorLevelName[512]; // to avoid having to call string constructor, or allocating memory.
    char                            m_editorLevelFolder[512];
    char              m_gameGUID[128];

    CLevelSystem* m_pLevelSystem;
    CActorSystem* m_pActorSystem;
    CItemSystem* m_pItemSystem;
    CVehicleSystem* m_pVehicleSystem;
    CSharedParamsManager* m_pSharedParamsManager;
    CActionMapManager* m_pActionMapManager;
    CViewSystem* m_pViewSystem;
    CGameplayRecorder* m_pGameplayRecorder;
    CGameRulesSystem* m_pGameRulesSystem;
    CFlowSystem* m_pFlowSystem;
    CGameObjectSystem* m_pGameObjectSystem;
    CUIDraw* m_pUIDraw;
    CScriptRMI* m_pScriptRMI;
    CAnimationGraphCVars* m_pAnimationGraphCvars;
    IMannequin* m_pMannequin;
    CMaterialEffects* m_pMaterialEffects;
    IBreakableGlassSystem* m_pBreakableGlassSystem;
    CPlayerProfileManager* m_pPlayerProfileManager;
    CDialogSystem* m_pDialogSystem;
    CSubtitleManager* m_pSubtitleManager;
    IGameTokenSystem* m_pGameTokenSystem;
    IEffectSystem* m_pEffectSystem;
    CGameSerialize* m_pGameSerialize;
    CallbackTimer* m_pCallbackTimer;
    CGameplayAnalyst* m_pGameplayAnalyst;
    CVisualLog* m_pVisualLog;
    CForceFeedBackSystem* m_pForceFeedBackSystem;
    //  INetQueryListener *m_pLanQueryListener;
    ILanQueryListener* m_pLanQueryListener;
    CCustomActionManager* m_pCustomActionManager;
    CCustomEventManager* m_pCustomEventManager;
    Graphics::CColorGradientManager* m_colorGradientManager = nullptr;
    Graphics::ScreenFaderManager* m_screenFaderManager = nullptr;

    IGameStatistics* m_pGameStatistics;

    ICooperativeAnimationManager*   m_pCooperativeAnimationManager;

    CAIProxyManager* m_pAIProxyManager;

    IGameVolumes* m_pGameVolumesManager;

    std::shared_ptr<CPrefabManager> m_pPrefabManager;

    IGamePhysicsSettings* m_pGamePhysicsSettings;

#ifndef _RELEASE
    DebugCamera* m_debugCamera;
#endif  //_RELEASE

    // developer mode
    CDevMode* m_pDevMode;
    
    // Currently handles the automatic creation of vegetation areas
    CRuntimeAreaManager* m_pRuntimeAreaManager;

    // script binds
    CScriptBind_Action* m_pScriptA;
    CScriptBind_ItemSystem* m_pScriptIS;
    CScriptBind_ActorSystem* m_pScriptAS;
    CScriptBind_Network* m_pScriptNet;
    CScriptBind_ActionMapManager* m_pScriptAMM;
    CScriptBind_VehicleSystem* m_pScriptVS;
    CScriptBind_Vehicle* m_pScriptBindVehicle;
    CScriptBind_VehicleSeat* m_pScriptBindVehicleSeat;
    CScriptBind_Inventory* m_pScriptInventory;
    CScriptBind_DialogSystem* m_pScriptBindDS;
    CScriptBind_MaterialEffects* m_pScriptBindMFX;
    std::unique_ptr<CScriptBind_PrefabManager> m_pScriptBindPF;
    CTimeOfDayScheduler* m_pTimeOfDayScheduler;
    CPersistentDebug* m_pPersistentDebug;

    CAIDebugRenderer* m_pAIDebugRenderer;
    CAINetworkDebugRenderer* m_pAINetworkDebugRenderer;

    CNetworkCVars* m_pNetworkCVars;
    CCryActionCVars* m_pCryActionCVars;

    //-- Network Stall ticker thread
#ifdef USE_NETWORK_STALL_TICKER_THREAD
    CNetworkStallTickerThread* m_pNetworkStallTickerThread = nullptr;
    uint32                                              m_networkStallTickerReferences;
#endif // #ifdef USE_NETWORK_STALL_TICKER_THREAD

    // Console Variables with some CryAction as owner
    CMaterialEffectsCVars* m_pMaterialEffectsCVars;

    CCryActionPhysicQueues* m_pPhysicsQueues;

    typedef std::vector<ICryUnknownPtr> TFrameworkExtensions;
    TFrameworkExtensions m_frameworkExtensions;

    // console variables
    ICVar* m_pEnableLoadingScreen;
    ICVar* m_pCheats;
    ICVar* m_pShowLanBrowserCVAR;
    ICVar* m_pDebugSignalTimers;
    ICVar* m_pDebugRangeSignaling;

    bool m_bShowLanBrowser;
    //
    bool m_isEditing;
    bool m_bScheduleLevelEnd;

    enum ESaveGameMethod
    {
        eSGM_NoSave = 0,
        eSGM_QuickSave,
        eSGM_Save
    };

    ESaveGameMethod m_delayedSaveGameMethod;     // 0 -> no save, 1=quick save, 2=save, not quick
    ESaveGameReason m_delayedSaveGameReason;
    int m_delayedSaveCountDown;

    struct SLocalAllocs
    {
        string m_delayedSaveGameName;
        string m_checkPointName;
        string m_nextLevelToLoad;
        bool m_clearRenderBufferToBlackOnUnload;
    };
    SLocalAllocs* m_pLocalAllocs;

    struct SGameFrameworkListener
    {
        IGameFrameworkListener* pListener;
        CryStackStringT<char, 64> name;
        EFRAMEWORKLISTENERPRIORITY eFrameworkListenerPriority;
        SGameFrameworkListener()
            : pListener(0)
            , eFrameworkListenerPriority(FRAMEWORKLISTENERPRIORITY_DEFAULT) {}
        void GetMemoryUsage(ICrySizer* pSizer) const {}
    };

    typedef std::vector<SGameFrameworkListener> TGameFrameworkListeners;
    TGameFrameworkListeners* m_pGFListeners;
    IBreakEventListener* m_pBreakEventListener;
    std::vector<bool> m_validListeners;

    int m_VoiceRecordingEnabled;

    bool m_bAllowSave;
    bool m_bAllowLoad;
    string* m_nextFrameCommand;
    string* m_connectServer;

#if !defined(_RELEASE)
    struct SConnectRepeatedly
    {
        bool m_enabled;
        int m_numAttemptsLeft;
        float m_timeForNextAttempt;

        SConnectRepeatedly()
            : m_enabled(false)
            , m_numAttemptsLeft(0)
            , m_timeForNextAttempt(0.0f) {}
    } m_connectRepeatedly;
#endif

    float       m_lastSaveLoad;

    uint32 m_PreUpdateTicks;

    CGameContextBridge* m_contextBridge;
};

#endif // CRYINCLUDE_CRYACTION_CRYACTION_H
