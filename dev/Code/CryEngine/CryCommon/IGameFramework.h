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

// Description : This is the interface which the launcher.exe will interact
//               with to start the game framework  For an implementation of
//               this interface refer to CryAction


#pragma once

#include <IEntitySystem.h>
#include <IGameStartup.h> // <> required for Interfuscator
#include <IGameFrameworkExtension.h>
#include "Cry_Color.h"
#include "TimeValue.h"

struct pe_explosion;
struct IPhysicalEntity;
struct EventPhysRemoveEntityParts;
struct IVisualLog;
struct ICombatLog;
struct IAIActorProxy;
struct ICooperativeAnimationManager;
struct IGameSessionHandler;
struct IRealtimeRemoteUpdate;
struct IForceFeedbackSystem;
struct ICommunicationVoiceLibrary;
struct ICustomActionManager;
struct ICustomEventManager;
struct ISerializeHelper;
struct IGameVolumes;
struct IGamePhysicsSettings;
class DebugCamera;

namespace Serialization
{
    class INetScriptMarshaler;
}

//Define to control the logging of breakability code
#define BREAK_LOGGING 0

#if BREAK_LOGGING
#define BreakLogAlways CryLogAlways
#else
#define BreakLogAlways(...) ((void)0)
#endif

// Summary
//   Generic factory creation
// Description
//   This macro is used to register new game object extension classes.
#define REGISTER_FACTORY(host, name, impl, isAI)                \
    (host)->RegisterFactory((name), (impl*)0, (isAI), (impl*)0) \


#define DECLARE_GAMEOBJECT_FACTORY(impl)                                         \
public:                                                                          \
    virtual void RegisterFactory(const char* name, impl * (*)(), bool isAI) = 0; \
    template <class T>                                                           \
    void RegisterFactory(const char* name, impl*, bool isAI, T*)                 \
    {                                                                            \
        struct Factory                                                           \
        {                                                                        \
            static impl* Create()                                                \
            {                                                                    \
                return new T();                                                  \
            }                                                                    \
        };                                                                       \
        RegisterFactory(name, Factory::Create, isAI);                            \
    }

// game object extensions need more information than the generic interface can provide
struct IGameObjectExtension;
DECLARE_COMPONENT_POINTERS(IGameObjectExtension);

struct IGameObjectExtensionCreatorBase
{
    // <interfuscator:shuffle>
    virtual ~IGameObjectExtensionCreatorBase() {}
    virtual IGameObjectExtensionPtr  Create() = 0;
    virtual void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount) = 0;
    // </interfuscator:shuffle>

    void GetMemoryUsage(ICrySizer* pSizer) const { /*LATER*/ }
};

#define DECLARE_GAMEOBJECTEXTENSION_FACTORY(name)                                     \
    struct I##name##Creator                                                           \
        : public IGameObjectExtensionCreatorBase                                      \
    {                                                                                 \
    };                                                                                \
    template <class T>                                                                \
    struct C##name##Creator                                                           \
        : public I##name##Creator                                                     \
    {                                                                                 \
        IGameObjectExtensionPtr Create()                                              \
        {                                                                             \
            return gEnv->pEntitySystem->CreateComponent<T>();                         \
        }                                                                             \
        void GetGameObjectExtensionRMIData(void** ppRMI, size_t * nCount)             \
        {                                                                             \
            T::GetGameObjectExtensionRMIData(ppRMI, nCount);                          \
        }                                                                             \
    };                                                                                \
    virtual void RegisterFactory(const char* name, I##name##Creator*, bool isAI) = 0; \
    template <class T>                                                                \
    void RegisterFactory(const char* name, I##name*, bool isAI, T*)                   \
    {                                                                                 \
        static C##name##Creator<T> creator;                                           \
        RegisterFactory(name, &creator, isAI);                                        \
    }

struct ISystem;
struct IUIDraw;
struct ILanQueryListener;
struct IActor;
struct IActorSystem;
struct IItem;
struct IGameRules;
struct IWeapon;
struct IItemSystem;
struct ILevelSystem;
struct IActionMapManager;
struct IGameChannel;
struct IViewSystem;
struct IVehicle;
struct IVehicleSystem;
struct IGameRulesSystem;
struct IFlowSystem;
struct IGameTokenSystem;
struct IEffectSystem;
struct IGameObject;
struct IGameObjectExtension;
struct IGameObjectSystem;
struct IGameplayRecorder;
struct ISaveGame;
struct ILoadGame;
struct IGameObject;
struct IMaterialEffects;
struct INetChannel;
struct IPlayerProfileManager;
struct IAnimationGraphState;
struct IDebugHistoryManager;
struct ISubtitleManager;
struct IDialogSystem;
struct IGameStatistics;
struct IFaceGen;
struct ICheckpointSystem;
struct IGameToEditorInterface;
struct IMannequin;
struct IScriptTable;
struct ITimeDemoRecorder;

class ISharedParamsManager;
class IPrefabManager;

enum EGameStartFlags
{
    eGSF_NoLevelLoading = 0x0001,
    eGSF_Server = 0x0002,
    eGSF_Client = 0x0004,
    eGSF_NoDelayedStart = 0x0008,
    eGSF_BlockingClientConnect = 0x0010,
    eGSF_NoGameRules = 0x0020,
    eGSF_LocalOnly = 0x0040,
    eGSF_NoQueries = 0x0080,
    eGSF_NoSpawnPlayer = 0x0100,
    eGSF_BlockingMapLoad = 0x0200,

    eGSF_DemoRecorder = 0x0400,
    eGSF_DemoPlayback = 0x0800,

    eGSF_ImmersiveMultiplayer = 0x1000,
    eGSF_RequireController = 0x2000,
    eGSF_RequireKeyboardMouse = 0x4000,

    eGSF_HostMigrated = 0x8000,
    eGSF_NonBlockingConnect = 0x10000,
    eGSF_InitClientActor = 0x20000,
};

enum ESaveGameReason
{
    eSGR_LevelStart,
    eSGR_FlowGraph,
    eSGR_Command,
    eSGR_QuickSave
};

enum ELoadGameResult
{
    eLGR_Ok,
    eLGR_Failed,
    eLGR_FailedAndDestroyedState,
    eLGR_CantQuick_NeedFullLoad
};


static const EntityId LOCAL_PLAYER_ENTITY_ID = 0x7777u; // 30583 between static and dynamic EntityIDs

struct SGameContextParams
{
    const char* levelName;
    const char* gameRules;
    const char* demoRecorderFilename;
    const char* demoPlaybackFilename;
    uint32 flags;

    SGameContextParams()
    {
        levelName = 0;
        gameRules = 0;
        demoRecorderFilename = 0;
        demoPlaybackFilename = 0;
        flags = 0;
    }
};

struct SGameStartParams
{
    // ip address/hostname of server to connect to - needed if bClient==true
    const char* hostname;
    // optional connection string for client
    const char* connectionString;
    // context parameters - needed if bServer==true
    const SGameContextParams* pContextParams;
    // a combination of EGameStartFlags - needed if bServer==true
    uint32 flags;
    // maximum players to allow to connect
    int32 maxPlayers;

    SGameStartParams()
    {
        flags = 0;
        hostname = 0;
        connectionString = 0;
        pContextParams = NULL;
        maxPlayers = MAXIMUM_NUMBER_OF_CONNECTIONS;
    }
};

struct SEntityTagParams
{
    EntityId entity;
    string text;
    float size;                     // font size
    float visibleTime;      // seconds before starting fade, >= 0
    float fadeTime;             // seconds to fade over, >= 0
    float viewDistance;     // maximum distance of entity from camera to show tag
    string staticId;            // when nonempty string, display first for entity, and only most recent one (for continuous info like health display)
    int column;                     // For multicolumn tag display (0 or 1 defaults to standard 1 column display)
    ColorF color;
    string tagContext;

    SEntityTagParams() { Init(); }
    SEntityTagParams(EntityId _entity, const char* _text)
    {
        Init();
        this->entity = _entity;
        this->text = _text ? _text : "";
    }
    SEntityTagParams(EntityId _entity, const char* _text, float _size, const ColorF& _color, float duration)
    {
        Init();
        this->entity = _entity;
        this->text = _text ? _text : "";
        this->size = _size;
        this->color = _color;
        this->fadeTime = duration;
    }

private:
    void Init()
    {
        entity = 0;
        text = "";
        size = 18.0f;
        visibleTime = 2.f;
        fadeTime = 1.f;
        viewDistance = 1000.f;
        staticId = "";
        column = 1;
        color = ColorF(1.f, 1.f, 1.f, 1.f);
        tagContext = "";
    }
};

typedef uint32 THUDWarningId;
struct IGameWarningsListener
{
    // <interfuscator:shuffle>
    virtual ~IGameWarningsListener() {}
    virtual bool OnWarningReturn(THUDWarningId id, const char* returnValue) { return true; }
    virtual void OnWarningRemoved(THUDWarningId id) {}
    // </interfuscator:shuffle>
};

/////////////////////////////////
// SRenderNodeCloneLookup is used to associate original IRenderNodes (used in the game) with
//  cloned IRenderNodes (used for the [redacted]), to allow breaks to be played back.
struct SRenderNodeCloneLookup
{
    SRenderNodeCloneLookup()
    {
        pOriginalNodes = NULL;
        pClonedNodes = NULL;
        iNumPairs = 0;
    }

    void UpdateStoragePointers(std::vector<IRenderNode*>& originalNodes, std::vector<IRenderNode*>& clonedNodes)
    {
        pOriginalNodes = originalNodes.empty() ? NULL : &(originalNodes[0]);
        pClonedNodes = clonedNodes.empty() ? NULL : &(clonedNodes[0]);
    }

    void AddNodePair(IRenderNode* originalNode, IRenderNode* clonedNode)
    {
        pOriginalNodes[iNumPairs] = originalNode;
        pClonedNodes[iNumPairs] = clonedNode;
        iNumPairs++;
    }

    void Reset()
    {
        iNumPairs = 0;
        pOriginalNodes = NULL;
        pClonedNodes = NULL;
    }

    IRenderNode** pOriginalNodes;
    IRenderNode** pClonedNodes;
    int iNumPairs;
};

struct IPersistentDebug
{
    // <interfuscator:shuffle>
    virtual ~IPersistentDebug() {}
    virtual void Begin(const char* name, bool clear) = 0;
    virtual void AddSphere(const Vec3& pos, float radius, ColorF clr, float timeout) = 0;
    virtual void AddDirection(const Vec3& pos, float radius, const Vec3& dir, ColorF clr, float timeout) = 0;
    virtual void AddLine(const Vec3& pos1, const Vec3& pos2, ColorF clr, float timeout) = 0;
    virtual void AddPlanarDisc(const Vec3& pos, float innerRadius, float outerRadius, ColorF clr, float timeout) = 0;
    virtual void AddCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, ColorF clr, float timeout) = 0;
    virtual void AddCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, ColorF clr, float timeout) = 0;
    virtual void AddQuat(const Vec3& pos, const Quat& q, float r, ColorF clr, float timeout) = 0;
    virtual void AddAABB(const Vec3& min, const Vec3& max, ColorF clr, float timeout) = 0;

    virtual void AddText(float x, float y, float fontSize, ColorF clr, float timeout, const char* fmt, ...) = 0;
    virtual void Add2DText(const char* text, float fontSize, ColorF clr, float timeout) = 0;
    virtual void Add2DLine(float x1, float y1, float x2, float y2, ColorF clr, float timeout) = 0;
    virtual void Add2DCircle(float x, float y, float radius, ColorF clr, float timeout) = 0;
    virtual void Add2DRect(float x, float y, float width, float height, ColorF clr, float timeout) = 0;

    virtual void AddEntityTag(const SEntityTagParams& params, const char* tagContext = "") = 0;
    virtual void ClearEntityTags(EntityId entityId) = 0;
    virtual void ClearStaticTag(EntityId entityId, const char* staticId) = 0;
    virtual void ClearTagContext(const char* tagContext) = 0;
    virtual void ClearTagContext(const char* tagContext, EntityId entityId) = 0;
    virtual void Reset() = 0;
    // </interfuscator:shuffle>
};


//! This is the order in which GOEs receive events.
enum EEntityEventPriority
{
    EEntityEventPriority_GameObject = 0,
    EEntityEventPriority_StartAnimProc,
    EEntityEventPriority_AnimatedCharacter,
    EEntityEventPriority_Vehicle, // Vehicles can potentially create move request too!
    EEntityEventPriority_Actor, // Actor must always be higher than AnimatedCharacter.
    EEntityEventPriority_PrepareAnimatedCharacterForUpdate,

    EEntityEventPriority_Last,

    EEntityEventPriority_Client = 100// special variable for the client to tag onto priorities when needed.
};

// When you add stuff here, also update in CCryAction::Init .
enum EGameFrameworkEvent
{
    eGFE_PauseGame,
    eGFE_ResumeGame,
    eGFE_OnCollision,
    eGFE_OnPostStep,
    eGFE_OnStateChange,
    eGFE_ResetAnimationGraphs,
    eGFE_OnBreakable2d,
    eGFE_OnBecomeVisible,
    eGFE_PreShatter,
    eGFE_BecomeLocalPlayer,
    eGFE_DisablePhysics,
    eGFE_EnablePhysics,
    eGFE_ScriptEvent,
    eGFE_StoodOnChange,
    eGFE_QueueRagdollCreation, // Queue the ragdoll for creation so the engine can do it at the best time.
    eGFE_QueueBlendFromRagdoll, // Queue the blend from ragdoll event (i.e. standup)
    eGFE_RagdollPhysicalized,  // Dispatched when the queued ragdoll is physicalized.
    eGFE_RagdollUnPhysicalized,  // Dispatched when the queued ragdoll is unphysicalized (i.e. Stoodup)
    eGFE_EnableBlendRagdoll,   // Enable blend with ragdoll mode (will blend with the currently active animation).
    eGFE_DisableBlendRagdoll,  // Disable blend with ragdoll (will blend out the ragdoll with the currently active animation).

    eGFE_Last
};

// All events game should be aware of need to be added here.
enum EActionEvent
{
    // Map resetting.
    eAE_resetBegin,
    eAE_resetEnd,
    eAE_resetProgress,
    eAE_preSaveGame,  // m_value -> ESaveGameReason
    eAE_postSaveGame, // m_value -> ESaveGameReason, m_description: 0 (failed), != 0 (successful)
    eAE_inGame,

    eAE_earlyPreUpdate,  // Called from CryAction's PreUpdate loop after System has been updated, but before subsystems.
    eAE_demoRecorderCreated,
    eAE_mapCmdIssued,
    eAE_unloadLevel,
    eAE_postUnloadLevel,
    eAE_loadLevel,
};

struct SActionEvent
{
    SActionEvent(EActionEvent e, int val = 0, const char* des = 0)
        : m_event(e)
        , m_value(val)
        , m_description(des)
    {
    }
    EActionEvent  m_event;
    int           m_value;
    const char*   m_description;
};

// We must take care of order in which listeners are called.
// Priority order is from low to high.
// As an example, menu must follow hud as it must be drawn on top of the rest.
enum EFRAMEWORKLISTENERPRIORITY
{
    // Default priority should not be used unless you don't care about order (it will be called first)
    FRAMEWORKLISTENERPRIORITY_DEFAULT,

    // Add your order somewhere here if you need to be called between one of them
    FRAMEWORKLISTENERPRIORITY_GAME,
    FRAMEWORKLISTENERPRIORITY_HUD,
    FRAMEWORKLISTENERPRIORITY_MENU
};

struct IGameFrameworkListener
{
    virtual ~IGameFrameworkListener() {}
    //! Called before frame is created.
    virtual void OnPreUpdate() { }
    //! Called after Render, before PostUpdate.
    virtual void OnPostUpdate(float fDeltaTime) { }
    //! Called before the game is saved.
    virtual void OnSaveGame(ISaveGame* pSaveGame) { }
    //! Called during the game load process (after OnSavegameFileLoadedInMemory).
    virtual void OnLoadGame(ILoadGame* pLoadGame) { }
    //! Called after a level is completed and unloaded, before next level is loaded.
    virtual void OnLevelEnd(const char* nextLevel) { }
    //! Called whenever an ActionEvent is emitted.
    virtual void OnActionEvent(const SActionEvent& event) { }
    //! Called after Update, but before Render.
    virtual void OnPreRender() { }
    //! Called after save game data is loaded in memory, but before the data is deserilialized (before OnLoadGame).
    virtual void OnSavegameFileLoadedInMemory(const char* pLevelName) { }
    //! Called before a game is loaded with the "load" command.
    virtual void OnForceLoadingWithFlash() { }
};

struct IBreakEventListener
{
    virtual ~IBreakEventListener() {}
    virtual void OnBreakEvent(uint16 uBreakEventIndex) = 0;
    virtual void OnPartRemoveEvent(int32 iPartRemoveEventIndex) = 0;
    virtual void OnEntityDrawSlot(IEntity* pEntity, int32 slot, int32 flags) = 0;
    virtual void OnEntityChangeStatObj(IEntity* pEntity, int32 iBrokenObjectIndex, int32 slot, IStatObj* pOldStatObj, IStatObj* pNewStatObj) = 0;
    virtual void OnSetSubObjHideMask(IEntity* pEntity, int nSlot, uint64 nSubObjHideMask) = 0;
};

// Summary
//   Interface which exposes the CryAction subsystems
struct IGameFramework
{
    DECLARE_GAMEOBJECT_FACTORY(ISaveGame);
    DECLARE_GAMEOBJECT_FACTORY(ILoadGame);
    DECLARE_GAMEOBJECTEXTENSION_FACTORY(Actor);
    DECLARE_GAMEOBJECTEXTENSION_FACTORY(Item);
    DECLARE_GAMEOBJECTEXTENSION_FACTORY(Vehicle);
    DECLARE_GAMEOBJECTEXTENSION_FACTORY(GameObjectExtension);

    typedef uint32 TimerID;
    typedef Functor2<void*, TimerID> TimerCallback;

    // <interfuscator:shuffle>
    virtual ~IGameFramework() {}

    // Summary
    //   Entry function to the game framework
    // Description
    //   Entry function used to create a new instance of the game framework from
    //   outside its own DLL.
    // Returns
    //   a new instance of the game framework
    typedef IGameFramework*( * TEntryFunction )();

    // Summary:
    //      Initialize CryENGINE with every system needed for a general action game.
    //      Independently of the success of this method, Shutdown must be called.
    // Arguments:
    //      startupParams - Pointer to SSystemInitParams structure containing system initialization setup!
    // Return Value:
    //      0 if something went wrong with initialization, non-zero otherwise.
    virtual bool Init(SSystemInitParams& startupParams) = 0;

    // Description:
    //    Used to notify the framework that we're switching between single and multi player
    virtual void InitGameType(bool multiplayer, bool fromInit) = 0;

    // Summary:
    //    Complete initialization of game framework with things that can only be done
    //    after all entities have been registered.
    virtual bool CompleteInit() = 0;

    // Description:
    //      Shuts down CryENGINE and any other subsystem created during initialization.
    virtual void Shutdown() = 0;

    // Description:
    //      Updates CryENGINE before starting a game frame.
    // Arguments:
    //      haveFocus - Boolean describing if the game has the input focus or not.
    //      updateFlags - Flags specifying how to update.
    // Return Value:
    //      true if we should continue running the game loop, false otherwise.
    virtual bool PreUpdate(bool haveFocus, unsigned int updateFlags) = 0;

    // Description:
    //      Updates CryENGINE after a game frame.
    // Arguments:
    //      haveFocus - Boolean describing if the game has the input focus or not.
    //      updateFlags - Flags specifying how to update.
    // Return Value:
    //      true if we should continue running the game loop, false otherwise.
    virtual bool PostUpdate(bool haveFocus, unsigned int updateFlags) = 0;

    // Description:
    //      Resets the current game
    virtual void Reset(bool clients) = 0;

    // Description:
    //      Pauses the game
    // Arguments:
    //      pause - Boolean describing if it's pausing or not.
    //      nFadeOutInMS - Time SFX and Voice will be faded out over in MilliSec.
    virtual void PauseGame(bool pause, bool force, unsigned int nFadeOutInMS = 0) = 0;

    // Description:
    //      Returns the pause status
    // Return Value:
    //      true - if the game is pause, false - otherwise
    virtual bool IsGamePaused() = 0;
    // Description:
    //    Are we completely into game mode?
    virtual bool IsGameStarted() = 0;

    // Description:
    //    Marks the game started.
    virtual void MarkGameStarted(bool started) = 0;

    // Description:
    //      Check if the game is allowed to start the actual gameplay
    virtual bool IsLevelPrecachingDone() const = 0;
    // Description:
    //    Inform game that it is allowed to start the gameplay
    virtual void SetLevelPrecachingDone(bool bValue) = 0;

    // Description:
    //      Returns a pointer to the ISystem interface.
    // Return Value:
    //      Pointer to ISystem interface.
    virtual ISystem* GetISystem() = 0;

    // Description:
    //      Retrieve a pointer to the ILanQueryListener interface
    // Return Value:
    //      Pointer to ILanQueryListener interface.
    virtual ILanQueryListener* GetILanQueryListener() = 0;

    // Description:
    //      Returns a pointer to the IUIDraw interface.
    // Return Value:
    //      Pointer to IUIDraw interface.
    virtual IUIDraw* GetIUIDraw() = 0;

    virtual IMannequin& GetMannequinInterface() = 0;

    // Description:
    //      Returns a pointer to the IGameObjectSystem interface.
    // Return Value:
    //      Pointer to IGameObjectSystem interface.
    virtual IGameObjectSystem* GetIGameObjectSystem() = 0;
    // Description:
    //      Returns a pointer to the ILevelSystem interface.
    // Return Value:
    //      Pointer to ILevelSystem interface.
    virtual ILevelSystem* GetILevelSystem() = 0;
    // Description:
    //      Returns a pointer to the IActorSystem interface.
    // Return Value:
    //      Pointer to IActorSystem interface.
    virtual IActorSystem* GetIActorSystem() = 0;
    // Description:
    //      Returns a pointer to the IItemSystem interface.
    // Return Value:
    //      Pointer to IItemSystem interface.
    virtual IItemSystem* GetIItemSystem() = 0;
    // Description:
    //      Returns a pointer to the IActionMapManager interface.
    // Return Value:
    //      Pointer to IActionMapManager interface.
    virtual IActionMapManager* GetIActionMapManager() = 0;
    // Description:
    //      Returns a pointer to the IViewSystem interface.
    // Return Value:
    //      Pointer to IViewSystem interface.
    virtual IViewSystem* GetIViewSystem() = 0;
    // Description:
    //      Returns a pointer to the IGameplayRecorder interface.
    // Return Value:
    //      Pointer to IGameplayRecorder interface.
    virtual IGameplayRecorder* GetIGameplayRecorder() = 0;
    // Description:
    //      Returns a pointer to the IVehicleSystem interface.
    // Return Value:
    //      Pointer to IVehicleSystem interface.
    virtual IVehicleSystem* GetIVehicleSystem() = 0;
    // Description:
    //      Returns a pointer to the IGameRulesSystem interface.
    // Return Value:
    //      Pointer to IGameRulesSystem interface.
    virtual IGameRulesSystem* GetIGameRulesSystem() = 0;
    // Description:
    //      Returns a pointer to the IFlowSystem interface.
    // Return Value:
    //      Pointer to IFlowSystem interface.
    virtual IFlowSystem* GetIFlowSystem() = 0;
    // Description:
    //      Returns a pointer to the IGameTokenSystem interface
    // Return Value:
    //      Pointer to IGameTokenSystem interface.
    virtual IGameTokenSystem* GetIGameTokenSystem() = 0;
    // Description:
    //      Returns a pointer to the IEffectSystem interface
    // Return Value:
    //      Pointer to IEffectSystem interface.
    virtual IEffectSystem* GetIEffectSystem() = 0;

    // Description:
    //      Returns a pointer to the IMaterialEffects interface.
    // Return Value:
    //      Pointer to IMaterialEffects interface.
    virtual IMaterialEffects* GetIMaterialEffects() = 0;

    // Description:
    //      Returns a pointer to the IDialogSystem interface
    // Return Value:
    //      Pointer to IDialogSystem interface.
    virtual IDialogSystem* GetIDialogSystem() = 0;

    // Description:
    //      Returns a pointer to the IPlayerProfileManager interface.
    // Return Value:
    //      Pointer to IPlayerProfileManager interface.
    virtual IPlayerProfileManager* GetIPlayerProfileManager() = 0;

    // Description:
    //      Returns a pointer to the ISubtitleManager interface.
    // Return Value:
    //      Pointer to ISubtitleManager interface.
    virtual ISubtitleManager* GetISubtitleManager() = 0;

    // Description
    //  Returns a pointer to the new ITweakMenuController interface
    // Description
    // returns a pointer to the IRealtimeUpdate Interface
    virtual IRealtimeRemoteUpdate* GetIRealTimeRemoteUpdate() = 0;

    // Description
    //  Returns a pointer to the IGameStatistics interface
    virtual IGameStatistics* GetIGameStatistics() = 0;

    // Description:
    //      Returns a pointer to the IVisualLog interface.
    // Return Value:
    //      Pointer to IIVisualLog interface.
    virtual IVisualLog* GetIVisualLog() = 0;

    // Description:
    //      Pointer to ICooperativeAnimationManager interface.
    virtual ICooperativeAnimationManager* GetICooperativeAnimationManager() = 0;

    // Description:
    //      Pointer to ICheckpointSystem interface.
    virtual ICheckpointSystem* GetICheckpointSystem() = 0;

    // Description:
    //      Pointer to IForceFeedbackSystem interface.
    virtual IForceFeedbackSystem* GetIForceFeedbackSystem() const = 0;

    // Description:
    //      Pointer to ICustomActionManager interface.
    virtual ICustomActionManager* GetICustomActionManager() const = 0;

    // Description:
    //      Pointer to ICustomEventManager interface.
    virtual ICustomEventManager* GetICustomEventManager() const = 0;

    // Description:
    //    Get pointer to Shared Parameters manager interface class.
    virtual ISharedParamsManager* GetISharedParamsManager() = 0;

    // Description:
    //    Get pointer to IPrefabManager interface.
    virtual std::shared_ptr<IPrefabManager> GetIPrefabManager() const = 0;

    // Description:
    //    Initialises a game context
    // Arguments:
    //    pGameStartParams - parameters for configuring the game
    // Return Value:
    //    true if successful
    virtual bool StartGameContext(const SGameStartParams* pGameStartParams) = 0;
    // Description:
    //    Changes a game context (levels and rules, etc); only allowed on the server
    // Arguments:
    //    pGameContextParams - parameters for configuring the context
    // Return Value:
    //    true if successful
    virtual bool ChangeGameContext(const SGameContextParams* pGameContextParams) = 0;
    // Description:
    //    Finished a game context (no game running anymore)
    virtual void EndGameContext(bool loadEmptyLevel) = 0;
    // Description:
    //    Detect if a context is currently running
    // Return Value:
    //    true if a game context is running
    virtual bool StartedGameContext() const = 0;
    // Description:
    //    Detect if a context is currently starting
    // Return Value:
    //    true if a game context is starting
    virtual bool StartingGameContext() const = 0;

    // Description:
    //    For the editor: spawn a player and wait for connection
    virtual bool BlockingSpawnPlayer() = 0;

    // Description:
    //    Remove broken entity parts
    virtual void FlushBreakableObjects() = 0;

    // Description:
    //    For the game : fix the broken game objects (to restart the map)
    virtual void ResetBrokenGameObjects() = 0;

    // Description:
    //    For the kill cam : clone the list of objects specified in the break events indexed
    virtual void CloneBrokenObjectsAndRevertToStateAtTime(int32 iFirstBreakEventIndex, uint16* pBreakEventIndices, int32& iNumBreakEvents, IRenderNode** outClonedNodes, int32& iNumClonedNodes, SRenderNodeCloneLookup& renderNodeLookup) = 0;

    // Description:
    //    For the kill cam: apply a single break event from an index
    virtual void ApplySingleProceduralBreakFromEventIndex(uint16 uBreakEventIndex, const SRenderNodeCloneLookup& renderNodeLookup) = 0;

    // Description:
    //    For the game : unhide the broken game objects (at the end of the kill cam)
    virtual void UnhideBrokenObjectsByIndex(uint16* ObjectIndicies, int32 iNumObjectIndices) = 0;

    // Description:
    //      Let the GameFramework initialize with the editor
    virtual void InitEditor(IGameToEditorInterface* pGameToEditor) = 0;

    // Description:
    //      Inform the GameFramework of the current level loaded in the editor.
    virtual void SetEditorLevel(const char* levelName, const char* levelFolder) = 0;

    // Description:
    //      Retrieves the current level loaded by the editor.
    // Arguments:
    //      Pointers to receive the level infos.
    virtual void GetEditorLevel(char** levelName, char** levelFolder) = 0;

    // Description:
    //    Returns the Actor associated with the client (or NULL)
    virtual IActor* GetClientActor() const = 0;
    // Description:
    //    Returns the Actor Id associated with the client (or NULL)
    virtual EntityId GetClientActorId() const = 0;

    // Description:
    //    Returns the Entity associated with the client (or NULL)
    virtual IEntity* GetClientEntity() const = 0;

    // Description:
    //    Sets the current client actor, and sets up action maps if desired.
    //    Note: Action maps are deprecated as of Lumberyard 1.11.
    virtual void SetClientActor(EntityId id, bool setupActionMaps = true) = 0;

#ifndef _RELEASE
    // Description:
    //    Gets the debug camera
    virtual DebugCamera* GetDebugCamera() { return nullptr; }
#endif

    // Description:
    //    Returns the (synched) time of the server (so use this for timed events, such as MP round times)
    virtual CTimeValue GetServerTime() = 0;
    // Description:
    //    Retrieve an IGameObject from an entity id
    //  Return Value:
    //      Pointer to IGameObject of the entity if it exists (or NULL otherwise)
    virtual IGameObject* GetGameObject(EntityId id) = 0;

    // Description:
    //    Retrieve a network safe entity class id, that will be the same in client and server
    //  Return Value:
    //      true if an entity class with this name has been registered
    virtual bool GetNetworkSafeClassId(uint16& id, const char* className) = 0;
    // Description:
    //    Retrieve a network safe entity class name, that will be the same in client and server
    //  Return Value:
    //      true if an entity class with this id has been registered
    virtual bool GetNetworkSafeClassName(char* className, size_t maxn, uint16 id) = 0;

    // Description:
    //    Retrieve an IGameObjectExtension by name from an entity
    //  Return Value:
    //      Pointer to IGameObjectExtension of the entity if it exists (or NULL otherwise)
    virtual IGameObjectExtension* QueryGameObjectExtension(EntityId id, const char* name) = 0;

    // Description:
    //    Retrieve pointer to the IGamePhysicsSettings (or NULL)
    virtual IGamePhysicsSettings* GetIGamePhysicsSettings() = 0;

    // Description:
    //      Deprecated function for requesting a string that can be used as a file name.
    virtual const char* GetStartLevelSaveGameName() = 0;

    // Description:
    //    Save the current game to disk
    virtual bool SaveGame(const char* path, bool quick = false, bool bForceImmediate = true, ESaveGameReason reason = eSGR_QuickSave, bool ignoreDelay = false, const char* checkPoint = NULL) = 0;
    // Description:
    //    Load a game from disk (calls StartGameContext...)
    virtual ELoadGameResult LoadGame(const char* path, bool quick = false, bool ignoreDelay = false) = 0;

    // Description:
    //    Schedules the level load for the next level
    virtual void ScheduleEndLevelNow(const char* nextLevel, bool clearRenderBufferToBlack = true) = 0;

    // Description:
    //    Notification that game mode is being entered/exited
    //    iMode values: 0-leave game mode, 1-enter game mode, 3-leave AI/Physics mode, 4-enter AI/Physics mode
    virtual void OnEditorSetGameMode(int iMode) = 0;

    virtual bool IsEditing() = 0;

    virtual bool IsLoadingSaveGame() = 0;

    // These functions are deprecated, please use the EBus defined in ITimeDemoRecorder directly.
    virtual bool IsInTimeDemo() = 0;
    virtual bool IsTimeDemoRecording() = 0;

    virtual void AllowSave(bool bAllow = true) = 0;
    virtual void AllowLoad(bool bAllow = true) = 0;
    virtual bool CanSave() = 0;
    virtual bool CanLoad() = 0;

    // Description:
    //    Gets a serialization helper for read/write usage based on settings
    virtual ISerializeHelper* GetSerializeHelper() const = 0;

    // Description:
    //      Check if the current game can activate cheats (flymode, godmode, nextspawn)
    virtual bool CanCheat() = 0;

    // Returns:
    //    path relative to the levels folder e.g. "Multiplayer\PS\Shore"
    virtual const char* GetLevelName() = 0;

    // OUTDATED: Description:
    // OUTDATED:   absolute because downloaded content might anywhere
    // OUTDATED:   e.g. "c:/MasterCD/Game/Levels/Testy"
    // Description:
    //   relative to the MasterCD folder e.g. "game/levels/!Code/AutoTest0"
    // Returns
    //   0 if no level is loaded
    virtual const char* GetAbsLevelPath(char* const pPath, const uint32 cPathMaxLen) = 0;

    virtual IPersistentDebug* GetIPersistentDebug() = 0;

    //Description:
    //      Adds a listener for break events
    virtual void    AddBreakEventListener(IBreakEventListener* pListener) = 0;

    //Description:
    //      Removes a listener for break events
    virtual void    RemoveBreakEventListener(IBreakEventListener* pListener) = 0;


    virtual void RegisterListener(IGameFrameworkListener* pGameFrameworkListener, const char* name, EFRAMEWORKLISTENERPRIORITY eFrameworkListenerPriority) = 0;
    virtual void UnregisterListener(IGameFrameworkListener* pGameFrameworkListener) = 0;

    virtual void SetGameGUID(const char* gameGUID) = 0;
    virtual const char* GetGameGUID() = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    virtual void EnableVoiceRecording(const bool enable) = 0;

    virtual IDebugHistoryManager* CreateDebugHistoryManager() = 0;

    // Description:
    //      Check whether the client actor is using voice communication.
    virtual bool IsVoiceRecordingEnabled() = 0;

    virtual bool IsImmersiveMPEnabled() = 0;

    // Description:
    //      Executes console command on next frame's beginning
    virtual void ExecuteCommandNextFrame(const char*) = 0;

    virtual const char* GetNextFrameCommand() const = 0;

    virtual void ClearNextFrameCommand() = 0;

    // Description:
    //      Opens a page in default browser
    virtual void ShowPageInBrowser(const char* URL) = 0;

    // Description:
    //      Opens a page in default browser
    virtual bool StartProcess(const char* cmd_line) = 0;

    // Description:
    //      Saves dedicated server console variables in server config file
    virtual bool SaveServerConfig(const char* path) = 0;

    // Description:
    //    to avoid stalls during gameplay and to get a list of all assets needed for the level (bEnforceAll=true)
    // Arguments:
    //   bEnforceAll - true to ensure all possible assets become registered (list should not be too conservative - to support level stripification)
    virtual void PrefetchLevelAssets(const bool bEnforceAll) = 0;

    // Description:
    //    Inform that an IEntity was spawned from breakage
    virtual void OnBreakageSpawnedEntity(IEntity* pEntity, IPhysicalEntity* pPhysEntity, IPhysicalEntity* pSrcPhysEntity) = 0;

    // Description:
    //      Adds a timer that will trigger a callback function passed by parameter. Allows to pass some user data pointer
    //      that will be one of the parameters for the callback function. The signature for the callback function is:
    //      void (void*, int). It allows member functions by using CE functors
    //
    //      It returns the handle to the timer created
    virtual IGameFramework::TimerID AddTimer(CTimeValue interval, bool repeat, TimerCallback callback, void* userdata = 0) = 0;

    // Description:
    //      Remove an existing timer by using its handle, returns user data
    virtual void* RemoveTimer(TimerID timerID) = 0;

    // Description:
    //      Return ticks last preupdate took
    virtual uint32 GetPreUpdateTicks() = 0;

    // Description:
    //      Get the time left when we are allowed to load a new game
    //      When this returns 0, we are allowed to load a new game
    virtual float GetLoadSaveDelay() const = 0;

    // Description:
    //    Allows the network code to keep ticking in the event of a stall on the main thread.
    virtual void StartNetworkStallTicker(bool includeMinimalUpdate) = 0;
    virtual void StopNetworkStallTicker() = 0;

    // Description
    // Retrieves manager which handles game objects tied to editor shapes and volumes
    virtual IGameVolumes* GetIGameVolumesManager() const = 0;

    virtual void PreloadAnimatedCharacter(IScriptTable* pEntityScript) = 0;

    // Description:
    //      Gets called from the physics thread just before doing a time step.
    // Arguments:
    //      deltaTime - the time interval that will be simulated
    virtual void PrePhysicsTimeStep(float deltaTime) = 0;

    virtual void HandleGridMateScriptRMI(TSerialize serialize, ChannelId toChannelId, ChannelId avoidChannelId) = 0;
    virtual void SerializeScript(TSerialize ser, IEntity* entity) = 0;

    // These functions handle the custom breaking apart of the ScriptAspect into multiple parts.
    virtual void NetMarshalScript(Serialization::INetScriptMarshaler* scriptMarshaler, IEntity* entity) = 0;
    virtual void NetUnmarshalScript(TSerialize ser, IEntity* entity) = 0;

    // Description:
    //      Register an extension to the game framework and makes it accessible through it
    //Arguments:
    //      pExtension - Extension to be added to the game framework
    virtual void RegisterExtension(ICryUnknownPtr pExtension) = 0;

    virtual void ReleaseExtensions() = 0;

    // Description:
    //      Retrieves an extension interface if registered with the framework
    template<typename ExtensionInterface>
    ExtensionInterface* QueryExtension() const
    {
        const CryInterfaceID interfaceId = cryiidof<ExtensionInterface>();
        return cryinterface_cast<ExtensionInterface>(QueryExtensionInterfaceById(interfaceId)).get();
    }

protected:
    // Description:
    //      Retrieves an extension interface by interface id
    //      Internal, client uses 'QueryExtension<ExtensionInterface>()
    //Arguments:
    //      interfaceID - Interface id
    virtual ICryUnknownPtr QueryExtensionInterfaceById(const CryInterfaceID& interfaceID) const = 0;
    // </interfuscator:shuffle>
};

ILINE bool IsDemoPlayback()
{
    return false;
}

class CryGameFrameworkRequests
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    /*!
    * Creates an IGameFramework instance, but does not initialize the framework or load the
    * CryGame module like InitFramework. Needed to support legacy games that still call the
    * DLL_EXPORT version of CreateGameFramework defined in CryAction/Main.cpp, which now in
    * turn calls this EBus method directly.
    * /return returns Pointer to the game framework if it was created, nullptr otherwise
    */
    virtual IGameFramework* CreateFramework() = 0;

    /*!
    * Creates an IGameFramework instance, initialize the framework and load CryGame module
    * /param[in] startupParams various parameters related to game startup
    * /return returns Pointer to the game framework if it was created and initialized, nullptr otherwise
    */
    virtual IGameFramework* InitFramework(SSystemInitParams& startupParams) = 0;

    /*!
    * Performs a proper shutdown of the game framework that was initialized in the 
    * InitFramework call
    */
    virtual void ShutdownFramework() = 0;
};

typedef AZ::EBus<CryGameFrameworkRequests> CryGameFrameworkBus;
