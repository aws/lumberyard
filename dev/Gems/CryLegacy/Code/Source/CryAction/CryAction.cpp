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

#include "CryLegacy_precompiled.h"
#include "CryAction.h"
#include "DebugCamera/DebugCamera.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentApplicationBus.h>


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define CRYACTION_CPP_SECTION_1 1
#define CRYACTION_CPP_SECTION_2 2
#define CRYACTION_CPP_SECTION_3 3
#define CRYACTION_CPP_SECTION_4 4
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYACTION_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryAction_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryAction_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(MOBILE)
#include "PlayerProfiles/PlayerProfileImplConsole.h"
#endif

#if _WIN32_WINNT < 0x0500
  #define _WIN32_WINNT 0x0500
#endif

#define PRODUCT_VERSION_MAX_STRING_LENGTH (256)

//#define CRYACTION_DEBUG_MEM   // debug memory usage

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYACTION_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryAction_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryAction_cpp_provo.inl"
    #endif
#elif defined(WIN32) || defined(WIN64)
#include <CryWindows.h>
#include <ShellApi.h>
#endif

#include <CryLibrary.h>
#include <platform_impl.h>

#include <AzCore/IO/FileIO.h> // for FileIOBase::GetInstance();
#include <AzCore/Jobs/LegacyJobExecutor.h>

#include "AIDebugRenderer.h"
#include "GameRulesSystem.h"
#include "ScriptBind_ActorSystem.h"
#include "ScriptBind_ItemSystem.h"
#include "ScriptBind_Inventory.h"
#include "ScriptBind_ActionMapManager.h"
#include "Network/ScriptBind_Network.h"
#include "ScriptBind_Action.h"
#include "ScriptBind_VehicleSystem.h"

#include "Network/ScriptRMI.h"
#include "Network/GameContext.h"
#include "Network/NetworkCVars.h"
#include "Network/NetworkStallTicker.h"

#include "AI/AIProxyManager.h"
#include "AI/BehaviorTreeNodes_Action.h"
#include <ICommunicationManager.h>
#include <IFactionMap.h>
#include <ISelectionTreeManager.h>
#include <BehaviorTree/IBehaviorTree.h>
#include <INavigationSystem.h>
#include <IEditorGame.h>
#include <IStatoscope.h>
#include <IStreamEngine.h>
#include <ITimeOfDay.h>
#include <IRenderer.h>
#include <CryProfileMarker.h>

#include "Serialization/GameSerialize.h"

#include "Mannequin/AnimationDatabaseManager.h"

#include "DevMode.h"
#include "ActionGame.h"

#include "AIHandler.h"
#include "AIProxy.h"

#include "CryActionCVars.h"

// game object extensions
#include "Inventory.h"

#include "FlowSystem/FlowSystem.h"
#include "IVehicleSystem.h"
#include "GameTokens/GameTokenSystem.h"
#include "EffectSystem/EffectSystem.h"
#include "VehicleSystem/ScriptBind_Vehicle.h"
#include "VehicleSystem/ScriptBind_VehicleSeat.h"
#include "VehicleSystem/Vehicle.h"
#include "AnimationGraph/AnimatedCharacter.h"
#include "AnimationGraph/AnimationGraphCVars.h"
#include "MaterialEffects/MaterialEffects.h"
#include "MaterialEffects/MaterialEffectsCVars.h"
#include "MaterialEffects/ScriptBind_MaterialEffects.h"
#include "GameObjects/GameObjectSystem.h"
#include "ViewSystem/ViewSystem.h"
#include "GameplayRecorder/GameplayRecorder.h"
#include "Analyst.h"
#include "BreakableGlassSystem.h"

#include "ForceFeedbackSystem/ForceFeedbackSystem.h"

#include "DialogSystem/DialogSystem.h"
#include "DialogSystem/ScriptBind_DialogSystem.h"
#include "SubtitleManager.h"

#include "LevelSystem.h"
#include "ActorSystem.h"
#include "ItemSystem.h"
#include "VehicleSystem.h"
#include "SharedParams/SharedParamsManager.h"
#include "ActionMapManager.h"

#include "Statistics/GameStatistics.h"
#include "UIDraw/UIDraw.h"
#include "GameRulesSystem.h"
#include "ActionGame.h"
#include "IGameObject.h"
#include "CallbackTimer.h"
#include "PersistentDebug.h"
#include "ITextModeConsole.h"
#include "TimeOfDayScheduler.h"
#include "CooperativeAnimationManager/CooperativeAnimationManager.h"
#include "Network/CVarListProcessor.h"
#include "CheckPoint/CheckPointSystem.h"

#include "AnimationGraph/DebugHistory.h"

#include "PlayerProfiles/PlayerProfileManager.h"
#include "PlayerProfiles/PlayerProfileImplFS.h"

#include "RemoteControl/RConServerListener.h"
#include "RemoteControl/RConClientListener.h"

#include "VisualLog/VisualLog.h"

#include "SignalTimers/SignalTimers.h"
#include "RangeSignalingSystem/RangeSignaling.h"

#include "LivePreview/RealtimeRemoteUpdate.h"

#include "CustomActions/CustomActionManager.h"
#include "CustomEvents/CustomEventsManager.h"

#ifdef GetUserName
#undef GetUserName
#endif

#include "ITimeDemoRecorder.h"
#include "INetworkService.h"
#include "IPlatformOS.h"

#include "Serialization/XmlSerializeHelper.h"
#include "Serialization/XMLCPBin/BinarySerializeHelper.h"
#include "Serialization/XMLCPBin/Writer/XMLCPB_ZLibCompressor.h"
#include "Serialization/XMLCPBin/XMLCPB_Utils.h"

#include "CryActionPhysicQueues.h"

#include "Mannequin/MannequinInterface.h"

#include "GameVolumes/GameVolumesManager.h"

#include "RuntimeAreas.h"

#include "GameConfigPhysicsSettings.h"

#include <LyShine/ILyShine.h>

#include "LipSync/LipSync_TransitionQueue.h"
#include "LipSync/LipSync_FacialInstance.h"

#include "Components/IComponentSerialization.h"

#include <FeatureTests/FeatureTests.h>

#include <AzCore/Module/Environment.h>
#include <AzCore/Component/ComponentApplication.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include "Prefabs/PrefabManager.h"
#include "Prefabs/ScriptBind_PrefabManager.h"
#include <Graphics/ColorGradientManager.h>
#include <Graphics/ScreenFader.h>

#define DEFAULT_BAN_TIMEOUT (30.0f)

#include <Network/GameContextBridge.h>
#include "MainThreadRenderRequestBus.h"

CCryAction* CCryAction::m_pThis = 0;


#define DLL_INITFUNC_SYSTEM "CreateSystemInterface"

static const int s_saveGameFrameDelay = 3; // enough to render enough frames to display the save warning icon before the save generation

static const float  s_loadSaveDelay = 0.5f; // Delay between load/save operations.

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Action
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_RANDOM_SEED:
            cry_random_seed(gEnv->bNoRandomSeed ? 0 : (uint32)wparam);
            break;

        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;

            if (!gEnv->IsEditor())
            {
                CCryAction::GetCryAction()->GetMannequinInterface().UnloadAll();
            }

            break;
        }
        case ESYSTEM_EVENT_3D_POST_RENDERING_END:
        {
            if (IMaterialEffects* pMaterialEffects = CCryAction::GetCryAction()->GetIMaterialEffects())
            {
                pMaterialEffects->Reset(true);
            }
            break;
        }
        case ESYSTEM_EVENT_FAST_SHUTDOWN:
        {
            IForceFeedbackSystem* pForceFeedbackSystem = CCryAction::GetCryAction() ? CCryAction::GetCryAction()->GetIForceFeedbackSystem() : nullptr;
            if (pForceFeedbackSystem)
            {
                CryLogAlways("ActionGame - Shutting down all force feedback");
                pForceFeedbackSystem->StopAllEffects();
            }
        }
        }
    }
};
static CSystemEventListner_Action g_system_event_listener_action;

// no dot use iterators in first part because of calls of some listners may modify array of listeners (add new)
#define CALL_FRAMEWORK_LISTENERS(func)                                        \
    {                                                                         \
        for (size_t n = 0; n < m_pGFListeners->size(); n++)                   \
        {                                                                     \
            const SGameFrameworkListener& le = m_pGFListeners->operator[](n); \
            if (m_validListeners[n]) {                                        \
                le.pListener->func; }                                         \
        }                                                                     \
        TGameFrameworkListeners::iterator it = m_pGFListeners->begin();       \
        std::vector<bool>::iterator vit = m_validListeners.begin();           \
        while (it != m_pGFListeners->end())                                   \
        {                                                                     \
            if (!*vit)                                                        \
            {                                                                 \
                it = m_pGFListeners->erase(it);                               \
                vit = m_validListeners.erase(vit);                            \
            }                                                                 \
            else{                                                             \
                ++it, ++vit; }                                                \
        }                                                                     \
    }

//------------------------------------------------------------------------
CCryAction::CCryAction()
    : m_paused(false)
    , m_forcedpause(false)
    , m_pSystem(0)
    , m_pNetwork(0)
    , m_p3DEngine(0)
    , m_pScriptSystem(0)
    , m_pEntitySystem(0)
    , m_pTimer(0)
    , m_pLog(0)
    , m_pGame(0)
    , m_pLevelSystem(0)
    , m_pActorSystem(0)
    , m_pItemSystem(0)
    , m_pVehicleSystem(0)
    , m_pSharedParamsManager(0)
    , m_pActionMapManager(0)
    , m_pViewSystem(0)
    , m_pGameplayRecorder(0)
    , m_pGameplayAnalyst(0)
    , m_pGameRulesSystem(0)
    , m_pFlowSystem(0)
    , m_pGameObjectSystem(0)
    , m_pScriptRMI(0)
    , m_pUIDraw(0)
    , m_pAnimationGraphCvars(0)
    , m_pMannequin(0)
    , m_pMaterialEffects(0)
    , m_pBreakableGlassSystem(0)
    , m_pForceFeedBackSystem(0)
    , m_pPlayerProfileManager(0)
    , m_pDialogSystem(0)
    , m_pSubtitleManager(0)
    , m_pGameTokenSystem(0)
    , m_pEffectSystem(0)
    , m_pGameSerialize(0)
    , m_pCallbackTimer(0)
    , m_pLanQueryListener(0)
    , m_pDevMode(0)
    , m_pRuntimeAreaManager(NULL)
    , m_pVisualLog(0)
    , m_pScriptA(0)
    , m_pScriptIS(0)
    , m_pScriptAS(0)
    , m_pScriptNet(0)
    , m_pScriptAMM(0)
    , m_pScriptVS(0)
    , m_pScriptBindVehicle(0)
    , m_pScriptBindVehicleSeat(0)
    , m_pScriptInventory(0)
    , m_pScriptBindDS(0)
    , m_pScriptBindMFX(0)
    , m_pPersistentDebug(0)
    , m_pMaterialEffectsCVars(0)
    , m_pEnableLoadingScreen(0)
    , m_pShowLanBrowserCVAR(0)
    , m_pDebugSignalTimers(0)
    , m_pDebugRangeSignaling(0)
    , m_bShowLanBrowser(false)
    , m_isEditing(false)
    , m_bScheduleLevelEnd(false)
    , m_delayedSaveGameMethod(eSGM_NoSave)
    , m_delayedSaveCountDown(0)
    , m_pLocalAllocs(0)
    , m_bAllowSave(true)
    , m_bAllowLoad(true)
    , m_pGFListeners(0)
    , m_pBreakEventListener(nullptr)
    , m_nextFrameCommand(0)
    , m_lastSaveLoad(0.0f)
    , m_pGameStatistics(0)
    , m_pAIDebugRenderer(0)
    , m_pAINetworkDebugRenderer(0)
    , m_pCooperativeAnimationManager(nullptr)
    , m_pAIProxyManager(0)
    , m_pCustomActionManager(0)
    , m_pCustomEventManager(0)
    , m_colorGradientManager(nullptr)
    , m_screenFaderManager(nullptr)
    , m_pPhysicsQueues(0)
    , m_PreUpdateTicks(0)
    , m_levelPrecachingDone(false)
    , m_usingLevelHeap(false)
    , m_pGameVolumesManager(nullptr)
    , m_pGamePhysicsSettings(nullptr)
#ifndef _RELEASE
    , m_debugCamera(nullptr)
#endif  //_RELEASE
{
    CRY_ASSERT(!m_pThis);
    m_pThis = this;

    m_contextBridge = nullptr;

    m_editorLevelName[0] = 0;
    m_editorLevelFolder[0] = 0;
    cry_strcpy(m_gameGUID, "{00000000-0000-0000-0000-000000000000}");
}

//------------------------------------------------------------------------
CCryAction::~CCryAction()
{
    Shutdown();
}

//------------------------------------------------------------------------
void CCryAction::DumpMapsCmd(IConsoleCmdArgs* args)
{
    int nlevels = GetCryAction()->GetILevelSystem()->GetLevelCount();
    if (!nlevels)
    {
        CryLogAlways("$3No levels found!");
    }
    else
    {
        CryLogAlways("$3Found %d levels:", nlevels);
    }

    for (int i = 0; i < nlevels; i++)
    {
        ILevelInfo* level = GetCryAction()->GetILevelSystem()->GetLevelInfo(i);
        const uint32 scanTag = level->GetScanTag();
        const uint32 levelTag = level->GetLevelTag();

        CryLogAlways("  %s [$9%s$o] Scan:%.4s Level:%.4s", level->GetName(), level->GetPath(), (char*)&scanTag, (char*)&levelTag);
    }
}
//------------------------------------------------------------------------

void CCryAction::ReloadReadabilityXML(IConsoleCmdArgs*)
{
    CAIFaceManager::LoadStatic();
}


//------------------------------------------------------------------------
void CCryAction::UnloadCmd(IConsoleCmdArgs* args)
{
    if (gEnv->IsEditor())
    {
        GameWarning("Won't unload level in editor");
        return;
    }

    CCryAction* pAction = CCryAction::GetCryAction();
    pAction->StartNetworkStallTicker(false);
    // Free context
    pAction->EndGameContext(true);
    pAction->StopNetworkStallTicker();

    if (args->GetArgCount() > 1)
    {
        string arg = args->GetArg(1);
        if (arg.find("r") != string::npos)
        {
            IConsole* pConsole = gEnv->pConsole;
            string tempHost = pConsole->GetCVar("cl_serveraddr")->GetString();
            SGameStartParams params;
            params.flags = eGSF_Client;
            params.hostname = tempHost.c_str();
            params.pContextParams = nullptr;
            gEnv->pGame->GetIGameFramework()->StartGameContext(&params);
        }
    }
}

string ArgsToString(IConsoleCmdArgs* args)
{
    string str;
    for (int idx = 0, count = args->GetArgCount(); idx < count; ++idx)
    {
        if (idx > 0)
        {
            str += " ";
        }
        const char* arg = args->GetArg(idx);
        str += arg;
    }
    return str;
}

//------------------------------------------------------------------------
void CCryAction::MapCmd(IConsoleCmdArgs* args)
{
    SLICE_SCOPE_DEFINE();

    LOADING_TIME_PROFILE_SECTION;
    uint32 flags = 0;

    // If we're are client in an MP session and running the map cmd, leave the session.
    if (!gEnv->bServer)
    {
        if (CCryAction::GetCryAction()->m_nextFrameCommand)
        {
            CCryAction::GetCryAction()->m_nextFrameCommand->resize(0);
        }

        gEnv->pConsole->ExecuteString("mpdisconnect");
        CRY_ASSERT_MESSAGE(gEnv->bServer, "We should be offline (and a 'server') after executing 'mpdisconnect'.");
    }

    // not available in the editor
    if (gEnv->IsEditor())
    {
        GameWarning("Won't load level in editor");
        return;
    }

    class CParamCheck
    {
    public:
        void AddParam(const string& param) { m_commands.insert(param); }
        const string* GetFullParam(const string& shortParam)
        {
            m_temp = m_commands;

            for (string::const_iterator pChar = shortParam.begin(); pChar != shortParam.end(); ++pChar)
            {
                typedef std::set<string>::iterator I;
                I next;
                for (I iter = m_temp.begin(); iter != m_temp.end(); )
                {
                    next = iter;
                    ++next;
                    if ((*iter)[pChar - shortParam.begin()] != *pChar)
                    {
                        m_temp.erase(iter);
                    }
                    iter = next;
                }
            }

            const char* warning = 0;
            const string* ret = 0;
            switch (m_temp.size())
            {
            case 0:
                warning = "Unknown command %s";
                break;
            case 1:
                ret = &*m_temp.begin();
                break;
            default:
                warning = "Ambiguous command %s";
                break;
            }
            if (warning)
            {
                GameWarning(warning, shortParam.c_str());
            }

            return ret;
        }

    private:
        std::set<string> m_commands;
        std::set<string> m_temp;
    };

    IConsole* pConsole = gEnv->pConsole;

    // Find the map name
    CryStackStringT<char, 256> currentMapName;
    {
        string mapname;

        // check if a map name was provided
        if (args->GetArgCount() > 1)
        {
            // set sv_map
            mapname = args->GetArg(1);
            mapname.replace("\\", "/");

            if (mapname.find("/") == string::npos)
            {
                const char* gamerules = pConsole->GetCVar("sv_gamerules")->GetString();

                int i = 0;
                const char* loc = 0;
                string tmp;
                while (loc = CCryAction::GetCryAction()->m_pGameRulesSystem->GetGameRulesLevelLocation(gamerules, i++))
                {
                    tmp = loc;
                    tmp.append(mapname);

                    if (CCryAction::GetCryAction()->m_pLevelSystem->GetLevelInfo(tmp.c_str()))
                    {
                        mapname = tmp;
                        break;
                    }
                }
            }

            currentMapName = mapname;
        }
    }

    // If there is not already a deferred map command, defer this command. If multiple
    // map commands are executed in the same frame (often at startup), then the last one
    // will win. Note that we add "deferred" as an argument to the map command so that multiple
    // successive map commands with the same contents don't accidentally cause immediate execution
    CCryAction* cryAction = GetCryAction();
    const string& deferredMapCmd = *cryAction->m_nextFrameCommand;
    string newMapCmd = ArgsToString(args);
    if (newMapCmd != deferredMapCmd)
    {
        *cryAction->m_nextFrameCommand = newMapCmd + " deferred";
        return;
    }

    pConsole->GetCVar("sv_map")->Set(currentMapName);

    const char* tempGameRules = pConsole->GetCVar("sv_gamerules")->GetString();

    if (const char* correctGameRules = CCryAction::GetCryAction()->m_pGameRulesSystem->GetGameRulesName(tempGameRules))
    {
        tempGameRules = correctGameRules;
    }

    string tempLevel = pConsole->GetCVar("sv_map")->GetString();
    string tempDemoFile;

    ILevelInfo* pLevelInfo = CCryAction::GetCryAction()->m_pLevelSystem->GetLevelInfo(tempLevel);
    if (!pLevelInfo)
    {
        GameWarning("Couldn't find map '%s'", tempLevel.c_str());
        gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_ERROR, 0, 0);
        return;
    }
    else
    {
    }

    CryLogAlways("============================ Loading level %s ============================", currentMapName.c_str());
    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_PREPARE, 0, 0);


    SGameContextParams ctx;
    ctx.gameRules = tempGameRules;
    ctx.levelName = tempLevel;

    // check if we want to run a dedicated server
    bool dedicated = false;
    bool server = false;
    bool forceNewContext = false;

    //if running dedicated network server - default nonblocking mode
    bool blocking = true;
    //if (GetCryAction()->StartedGameContext())
    //{
    //  blocking = !::gEnv->IsDedicated();
    //}
    //
    ICVar* pImmersive = gEnv->pConsole->GetCVar("g_immersive");
    if (pImmersive && pImmersive->GetIVal() != 0)
    {
        flags |= eGSF_ImmersiveMultiplayer;
    }
    //
    if (args->GetArgCount() > 2)
    {
        CParamCheck paramCheck;
        paramCheck.AddParam("dedicated");
        paramCheck.AddParam("record");
        paramCheck.AddParam("server");
        paramCheck.AddParam("nonblocking");
        paramCheck.AddParam("nb");
        paramCheck.AddParam("x");
        paramCheck.AddParam("deferred");
        //
        for (int i = 2; i < args->GetArgCount(); i++)
        {
            string param(args->GetArg(i));
            const string* pArg = paramCheck.GetFullParam(param);
            if (!pArg)
            {
                return;
            }
            const char* arg = pArg->c_str();

            // if 'd' or 'dedicated' is specified as a second argument we are server only
            if (!strcmp(arg, "dedicated") || !strcmp(arg, "d"))
            {
                dedicated = true;
                blocking = false;
            }
            else if (!strcmp(arg, "record"))
            {
                int j = i + 1;
                if (j >= args->GetArgCount())
                {
                    continue;
                }
                tempDemoFile = args->GetArg(j);
                i = j;

                ctx.demoRecorderFilename = tempDemoFile.c_str();

                flags |= eGSF_DemoRecorder;
                server = true; // otherwise we will not be able to create more than one GameChannel when starting DemoRecorder
            }
            else if (!strcmp(arg, "server"))
            {
                server = true;
            }
            else if (!strcmp(arg, "nonblocking"))
            {
                blocking = false;
            }
            else if (!strcmp(arg, "nb"))
            {
                flags |= eGSF_NonBlockingConnect;
            }
            else if (!strcmp(arg, "x"))
            {
                flags |= eGSF_ImmersiveMultiplayer;
            }
            else if (!strcmp(arg, "deferred"))
            {
                continue; // no-op, this is just a marker token
            }
            else
            {
                GameWarning("Added parameter %s to paramCheck, but no action performed", arg);
                gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_ERROR, 0, 0);
                return;
            }
        }
    }

#ifndef _RELEASE
    gEnv->pSystem->SetLoadOrigin(ISystem::eLLO_MapCmd);
#endif

    if (blocking)
    {
        flags |= eGSF_BlockingClientConnect | /*eGSF_LocalOnly | eGSF_NoQueries |*/ eGSF_BlockingMapLoad;
        forceNewContext = true;
    }

    if (::gEnv->IsDedicated())
    {
        dedicated = true;
    }

    if (dedicated)
    {
        //      tempLevel = "Multiplayer/" + tempLevel;
        //      ctx.levelName = tempLevel.c_str();
        server = true;
    }

    bool startedContext = false;
    // if we already have a game context, then we just change it
    if (GetCryAction()->StartedGameContext())
    {
        if (forceNewContext)
        {
            GetCryAction()->EndGameContext(false);
        }
        else
        {
            // eventually we will need to call
            // SetContextInfo on the gamecontext
            // make sure our flags are corrct here
            // since we are trying to go into a map
            // we should make sure we are flagged to load
            // a level
            flags |= eGSF_Server;

            if (!dedicated)
            {
                flags |= eGSF_Client;
            }
            if (!server)
            {
                flags |= eGSF_LocalOnly;
            }

            ctx.flags = flags;
            GetCryAction()->ChangeGameContext(&ctx);
            startedContext = true;
        }
    }

    if (ctx.levelName)
    {
        CCryAction::GetCryAction()->GetILevelSystem()->PrepareNextLevel(ctx.levelName);
    }

    CCryAction::GetCryAction()->OnActionEvent(SActionEvent(eAE_mapCmdIssued, 0, currentMapName));

    if (!startedContext)
    {
        CRY_ASSERT(!GetCryAction()->StartedGameContext());

        SGameStartParams params;
        params.flags = flags | eGSF_Server;

        if (!dedicated)
        {
            params.flags |= eGSF_Client;
            params.hostname = "localhost";
        }
        if (server)
        {
            ICVar* max_players = gEnv->pConsole->GetCVar("sv_maxplayers");
            params.maxPlayers = max_players ? max_players->GetIVal() : 16;
            ICVar* loading = gEnv->pConsole->GetCVar("g_enableloadingscreen");
            if (loading)
            {
                loading->Set(0);
            }
            //gEnv->pConsole->GetCVar("g_enableitems")->Set(0);
        }
        else
        {
            params.flags |= eGSF_LocalOnly;
            params.maxPlayers = 1;
        }

        ctx.flags = params.flags;
        params.pContextParams = &ctx;

        CCryAction::GetCryAction()->StartGameContext(&params);
    }
}

//------------------------------------------------------------------------
void CCryAction::PlayCmd(IConsoleCmdArgs* args)
{
    IConsole* pConsole = gEnv->pConsole;

    if (GetCryAction()->StartedGameContext())
    {
        GameWarning("Must stop the game before commencing playback");
        return;
    }
    if (args->GetArgCount() < 2)
    {
        GameWarning("Usage: \\play demofile");
        return;
    }

    SGameStartParams params;
    SGameContextParams context;

    params.pContextParams = &context;
    context.demoPlaybackFilename = args->GetArg(1);
    params.maxPlayers = 1;
    params.flags = eGSF_Client | eGSF_Server | eGSF_DemoPlayback | eGSF_NoGameRules | eGSF_NoLevelLoading;

    GetCryAction()->StartGameContext(&params);
}

//------------------------------------------------------------------------
void CCryAction::VersionCmd(IConsoleCmdArgs* args)
{
    CryLogAlways("-----------------------------------------");
    char myVersion[PRODUCT_VERSION_MAX_STRING_LENGTH];
    gEnv->pSystem->GetProductVersion().ToString(myVersion);
    CryLogAlways("Product version: %s", myVersion);
    gEnv->pSystem->GetFileVersion().ToString(myVersion);
    CryLogAlways("File version: %s", myVersion);
    CryLogAlways("-----------------------------------------");
}

//------------------------------------------------------------------------
void CCryAction::GenStringsSaveGameCmd(IConsoleCmdArgs* pArgs)
{
    CGameSerialize* cgs = GetCryAction()->m_pGameSerialize;
    if (cgs)
    {
        cgs->SaveGame(GetCryAction(), "string-extractor", "SaveGameStrings.cpp", eSGR_Command);
    }
}

//------------------------------------------------------------------------
void CCryAction::SaveGameCmd(IConsoleCmdArgs* args)
{
    // quicksave is disabled for C2 builds, but still useful for debug testing
    if (gEnv->pSystem->IsDevMode())
    {
        string sSavePath("@user@");  // you may not write to the game assets folder
        if (args->GetArgCount() > 1)
        {
            sSavePath.append("/");
            sSavePath.append(args->GetArg(1));
            CryFixedStringT<64> extension(LY_SAVEGAME_FILE_EXT);
            extension.Trim('.');
            sSavePath = PathUtil::ReplaceExtension(sSavePath, extension.c_str());
            GetCryAction()->SaveGame(sSavePath, false, false, eSGR_QuickSave, true);
        }
        else
        {
            GetCryAction()->SaveGame(gEnv->pGame->CreateSaveGameName().c_str(), true, false);
        }
    }
}

//------------------------------------------------------------------------
void CCryAction::LoadGameCmd(IConsoleCmdArgs* args)
{
    if (args->GetArgCount() > 1)
    {
        GetCryAction()->NotifyForceFlashLoadingListeners();
        bool quick = args->GetArgCount() > 2;
        GetCryAction()->LoadGame(args->GetArg(1), quick);
    }
    else
    {
        gEnv->pConsole->ExecuteString("loadLastSave");
    }
}

void CCryAction::OpenURLCmd(IConsoleCmdArgs* args)
{
    if (args->GetArgCount() > 1)
    {
        GetCryAction()->ShowPageInBrowser(args->GetArg(1));
    }
}

void CCryAction::DumpAnalysisStatsCmd(IConsoleCmdArgs* args)
{
    if (CCryAction::GetCryAction()->m_pGameplayAnalyst)
    {
        CCryAction::GetCryAction()->m_pGameplayAnalyst->DumpToTXT();
    }
}

//------------------------------------------------------------------------
IRemoteControlServer* CCryAction::s_rcon_server = nullptr;
IRemoteControlClient* CCryAction::s_rcon_client = nullptr;

CRConClientListener* CCryAction::s_rcon_client_listener = nullptr;

#define RCON_DEFAULT_PORT 9999

//------------------------------------------------------------------------
void CCryAction::rcon_startserver(IConsoleCmdArgs* args)
{
}

//------------------------------------------------------------------------
void CCryAction::rcon_stopserver(IConsoleCmdArgs* args)
{
}

//------------------------------------------------------------------------
void CCryAction::rcon_connect(IConsoleCmdArgs* args)
{
}

//------------------------------------------------------------------------
void CCryAction::rcon_disconnect(IConsoleCmdArgs* args)
{
}

//------------------------------------------------------------------------
void CCryAction::rcon_command(IConsoleCmdArgs* args)
{
}

#define EngineStartProfiler(x)
#define InitTerminationCheck(x)

static inline void InlineInitializationProcessing(const char* sDescription)
{
    EngineStartProfiler(sDescription);
    InitTerminationCheck(sDescription);
    gEnv->pLog->UpdateLoadingScreen(0);
}

//------------------------------------------------------------------------
bool CCryAction::Init(SSystemInitParams& startupParams)
{
    m_pSystem = startupParams.pSystem;

    if (!startupParams.pSystem)
    {
#if !defined(AZ_MONOLITHIC_BUILD)

        AZ::OSString crySystemPath = "CrySystem";

        // Let the application process the path
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::ResolveModulePath, crySystemPath);
        m_crySystemModule = AZ::DynamicModuleHandle::Create(crySystemPath.c_str());
        if (!m_crySystemModule->Load(false))
        {
            return false;
        }


        // We need to inject the environment first thing so that allocators are available immediately
        InjectEnvironmentFunction injectEnv = m_crySystemModule->GetFunction<InjectEnvironmentFunction>(INJECT_ENVIRONMENT_FUNCTION);
        if (injectEnv)
        {
            auto env = AZ::Environment::GetInstance();
            injectEnv(env);
        }


        PFNCREATESYSTEMINTERFACE CreateSystemInterface = m_crySystemModule->GetFunction<PFNCREATESYSTEMINTERFACE>(DLL_INITFUNC_SYSTEM);
        if (CreateSystemInterface)
#endif // AZ_MONOLITHIC_BUILD
        {
            // initialize the system
            m_pSystem = CreateSystemInterface(startupParams);
        }

        if (!m_pSystem)
        {
            return false;
        }
    }

#if !defined(AZ_MONOLITHIC_BUILD)
    // Since CryAction has been moved to a gem, we only need to register factories.
    // ISystem initialization is handled earlier as gems are initialized much later.
    ICryFactoryRegistryImpl* pCryFactoryImpl = static_cast<ICryFactoryRegistryImpl*>(m_pSystem->GetCryFactoryRegistry());
    if (pCryFactoryImpl)
    {
        pCryFactoryImpl->RegisterFactories(g_pHeadToRegFactories);
    }
#endif

    // here we have gEnv and m_pSystem
    LOADING_TIME_PROFILE_SECTION_NAMED("CCryAction::Init() after system");

    InlineInitializationProcessing("CCryAction::Init CrySystem and CryAction init");

    m_pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_action);

    IComponentFactoryRegistry::RegisterAllComponentFactoryNodes(*m_pSystem->GetIEntitySystem()->GetComponentFactoryRegistry());

    // fill in interface pointers
    m_pNetwork = gEnv->pNetwork;
    m_p3DEngine = gEnv->p3DEngine;
    m_pScriptSystem = gEnv->pScriptSystem;
    m_pEntitySystem = gEnv->pEntitySystem;
    m_pTimer = gEnv->pTimer;
    m_pLog = gEnv->pLog;

    InitCVars();
    InitCommands();

    InitGameVolumesManager();

    InlineInitializationProcessing("CCryAction::Init InitCommands");
    if (m_pSystem->IsDevMode())
    {
        m_pDevMode = new CDevMode();
    }
    
    CGameObject::CreateCVars();

    CScriptRMI::RegisterCVars();
    m_pScriptRMI = new CScriptRMI();

    // initialize subsystems
    m_pGameTokenSystem = new CGameTokenSystem;
    m_pEffectSystem = new CEffectSystem;
    m_pEffectSystem->Init();
    m_pUIDraw = new CUIDraw;
    m_pLevelSystem = new CLevelSystem(m_pSystem, "levels");

    InlineInitializationProcessing("CCryAction::Init CLevelSystem");

    m_pActorSystem = new CActorSystem(m_pSystem, m_pEntitySystem);
    m_pItemSystem = new CItemSystem(this, m_pSystem);
    m_pActionMapManager = new CActionMapManager(gEnv->pInput);

    InlineInitializationProcessing("CCryAction::Init CActionMapManager");

    m_pCooperativeAnimationManager = new CCooperativeAnimationManager;

    m_pViewSystem = new CViewSystem(m_pSystem);
    m_pGameplayRecorder = new CGameplayRecorder(this);
    m_pGameRulesSystem = new CGameRulesSystem(m_pSystem, this);
    m_pVehicleSystem = new CVehicleSystem(m_pSystem, m_pEntitySystem);

    m_pSharedParamsManager = new CSharedParamsManager;

    m_pNetworkCVars = new CNetworkCVars();
    m_pCryActionCVars = new CCryActionCVars();

    m_pGamePhysicsSettings = new GameConfigPhysicsSettings();
    m_pGamePhysicsSettings->Init();

    //-- Network Stall ticker thread - PS3 only // ACCEPTED_USE
    if (m_pCryActionCVars->g_gameplayAnalyst)
    {
        m_pGameplayAnalyst = new CGameplayAnalyst();
    }

    InlineInitializationProcessing("CCryAction::Init CGameplayAnalyst");
    m_pGameObjectSystem = new CGameObjectSystem();
    if (!m_pGameObjectSystem->Init())
    {
        return false;
    }
    else
    {
        // init game object events of CryAction
        m_pGameObjectSystem->RegisterEvent(eGFE_PauseGame, "PauseGame");
        m_pGameObjectSystem->RegisterEvent(eGFE_ResumeGame, "ResumeGame");
        m_pGameObjectSystem->RegisterEvent(eGFE_OnCollision, "OnCollision");
        m_pGameObjectSystem->RegisterEvent(eGFE_OnPostStep, "OnPostStep");
        m_pGameObjectSystem->RegisterEvent(eGFE_OnStateChange, "OnStateChange");
        m_pGameObjectSystem->RegisterEvent(eGFE_ResetAnimationGraphs, "ResetAnimationGraphs");
        m_pGameObjectSystem->RegisterEvent(eGFE_OnBreakable2d, "OnBreakable2d");
        m_pGameObjectSystem->RegisterEvent(eGFE_OnBecomeVisible, "OnBecomeVisible");
        m_pGameObjectSystem->RegisterEvent(eGFE_PreShatter, "PreShatter");
        m_pGameObjectSystem->RegisterEvent(eGFE_BecomeLocalPlayer, "BecomeLocalPlayer");
        m_pGameObjectSystem->RegisterEvent(eGFE_DisablePhysics, "DisablePhysics");
        m_pGameObjectSystem->RegisterEvent(eGFE_EnablePhysics, "EnablePhysics");
        m_pGameObjectSystem->RegisterEvent(eGFE_ScriptEvent, "ScriptEvent");
        m_pGameObjectSystem->RegisterEvent(eGFE_QueueRagdollCreation, "QueueRagdollCreation");
        m_pGameObjectSystem->RegisterEvent(eGFE_RagdollPhysicalized, "RagdollPhysicalized");
        m_pGameObjectSystem->RegisterEvent(eGFE_StoodOnChange, "StoodOnChange");
        m_pGameObjectSystem->RegisterEvent(eGFE_EnableBlendRagdoll, "EnableBlendToRagdoll");
        m_pGameObjectSystem->RegisterEvent(eGFE_DisableBlendRagdoll, "DisableBlendToRagdoll");
    }

    m_pAnimationGraphCvars = new CAnimationGraphCVars();
    m_pMannequin = new CMannequinInterface();
    if (gEnv->IsEditor())
    {
        m_pCallbackTimer = new CallbackTimer();
    }
    m_pPersistentDebug = new CPersistentDebug();
    m_pPersistentDebug->Init();
#if defined(CONSOLE)
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYACTION_CPP_SECTION_3
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryAction_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryAction_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #else
    m_pPlayerProfileManager = new CPlayerProfileManager(new CPlayerProfileImplConsole());
    #endif
#else
    m_pPlayerProfileManager = new CPlayerProfileManager(new CPlayerProfileImplFSDir());
#endif
    m_pDialogSystem = new CDialogSystem();
    m_pDialogSystem->Init();
    m_pTimeOfDayScheduler = new CTimeOfDayScheduler();
    m_pSubtitleManager = new CSubtitleManager();

    m_pCustomActionManager = new CCustomActionManager();
    m_pCustomEventManager = new CCustomEventManager();

    CRangeSignaling::Create();
    CSignalTimer::Create();
    CVisualLog::Create();

    IMovieSystem* movieSys = gEnv->pMovieSystem;
    if (movieSys != nullptr)
    {
        movieSys->SetUser(m_pViewSystem);
    }

    if (m_pVehicleSystem)
    {
        m_pVehicleSystem->Init();
    }

    REGISTER_FACTORY((IGameFramework*)this, "Inventory", CInventory, false);

    if (m_pLevelSystem && m_pItemSystem)
    {
        m_pLevelSystem->AddListener(m_pItemSystem);
    }

    InitScriptBinds();

    CAIFaceManager::LoadStatic();


    m_pLocalAllocs = new SLocalAllocs();

#if 0
    BeginLanQuery();
#endif

    if (m_pVehicleSystem)
    {
        m_pVehicleSystem->RegisterVehicles(this);
    }
    if (m_pGameObjectSystem)
    {
        m_pGameObjectSystem->RegisterFactories(this);
    }


    // Player profile stuff
    if (m_pPlayerProfileManager)
    {
        bool ok = m_pPlayerProfileManager->Initialize();
        if (!ok)
        {
            GameWarning("[PlayerProfiles] CCryAction::Init: Cannot initialize PlayerProfileManager");
        }
    }

    m_pGFListeners = new TGameFrameworkListeners();

    // These vectors must have enough space allocated up-front so as to guarantee no further allocs
    // If they do exceed this capacity, the level heap mechanism should result in a crash
    m_pGFListeners->reserve(20);
    m_validListeners.reserve(m_pGFListeners->capacity());

    m_nextFrameCommand = new string();

    m_pGameStatistics = new CGameStatistics();

    if (gEnv->pAISystem)
    {
        m_pAIDebugRenderer = new CAIDebugRenderer(gEnv->pRenderer);
        gEnv->pAISystem->SetAIDebugRenderer(m_pAIDebugRenderer);

        RegisterActionBehaviorTreeNodes();
    }

    if (gEnv->IsEditor())
    {
        CreatePhysicsQueues();
    }

    XMLCPB::CDebugUtils::Create();

    if (!gEnv->IsDedicated())
    {
        XMLCPB::InitializeCompressorThread();
    }

#ifndef _RELEASE
    if (!gEnv->IsDedicated())
    {
        m_debugCamera = new DebugCamera;
    }
#endif  //_RELEASE

    m_contextBridge = aznew CGameContextBridge;
    m_contextBridge->Init();

    m_colorGradientManager = new Graphics::CColorGradientManager();

    InlineInitializationProcessing("CCryAction::Init End");
    return true;
}
//------------------------------------------------------------------------

void CCryAction::InitForceFeedbackSystem()
{
    LOADING_TIME_PROFILE_SECTION;
    SAFE_DELETE(m_pForceFeedBackSystem);
    m_pForceFeedBackSystem = new CForceFeedBackSystem();
    m_pForceFeedBackSystem->Initialize();
}

//------------------------------------------------------------------------

void CCryAction::InitGameVolumesManager()
{
    if (m_pGameVolumesManager == nullptr)
    {
        m_pGameVolumesManager = new CGameVolumesManager();
    }
}

//------------------------------------------------------------------------
void CCryAction::InitGameType(bool multiplayer, bool fromInit)
{
    gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_RUNNING);
    SAFE_DELETE(m_pGameSerialize);

    InitForceFeedbackSystem();

#if !defined(DEDICATED_SERVER)
    if (!multiplayer)
    {
        m_pGameSerialize = new CGameSerialize();

        if (m_pGameSerialize)
        {
            m_pGameSerialize->RegisterFactories(this);
        }
    }

    ICVar* pEnableAI = gEnv->pConsole->GetCVar("sv_AISystem");
    if (!multiplayer || (pEnableAI && pEnableAI->GetIVal()))
    {
        if (!m_pAIProxyManager)
        {
            m_pAIProxyManager = new CAIProxyManager;
            m_pAIProxyManager->Init();
        }
    }
    else
#endif
    {
        if (m_pAIProxyManager)
        {
            m_pAIProxyManager->Shutdown();
            SAFE_DELETE(m_pAIProxyManager);
        }
    }
}

static AZStd::fixed_vector<const char*, 2> gs_lipSyncExtensionNamesForExposureToEditor = {
    "LipSync_TransitionQueue",
    "LipSync_FacialInstance"
};

//------------------------------------------------------------------------
bool CCryAction::CompleteInit()
{
    LOADING_TIME_PROFILE_SECTION;

    InlineInitializationProcessing("CCryAction::CompleteInit");

    REGISTER_FACTORY((IGameFramework*)this, "AnimatedCharacter", CAnimatedCharacter, false);


    EndGameContext(false);

    FeatureTests::CreateManager();

    SAFE_DELETE(m_pFlowSystem);
    m_pFlowSystem = new CFlowSystem();
    m_pSystem->SetIFlowSystem(m_pFlowSystem);
    m_pFlowSystem->PreInit();
    m_pSystem->SetIDialogSystem(m_pDialogSystem);

    if (m_pFlowSystem)
    {
        m_pFlowSystem->Init();
    }

    InlineInitializationProcessing("CCryAction::CompleteInit SetDialogSystem");

    if (m_pGameplayAnalyst)
    {
        m_pGameplayRecorder->RegisterListener(m_pGameplayAnalyst);
    }


    CRangeSignaling::ref().Init();
    CSignalTimer::ref().Init();
    // ---------------------------

    m_pVisualLog = &(CVisualLog::ref());
    m_pVisualLog->Init();
    m_pSystem->SetIVisualLog (m_pVisualLog);

    InlineInitializationProcessing("CCryAction::CompleteInit VisualLog");

    if (gEnv->pRenderer)
    {
        m_pMaterialEffects = new CMaterialEffects();
        m_pScriptBindMFX = new CScriptBind_MaterialEffects(m_pSystem, m_pMaterialEffects);
        m_pSystem->SetIMaterialEffects(m_pMaterialEffects);

        InlineInitializationProcessing("CCryAction::CompleteInit MaterialEffects");
    }

    m_pBreakableGlassSystem = new CBreakableGlassSystem();

    InitForceFeedbackSystem();

    ICVar* pEnableAI = gEnv->pConsole->GetCVar("sv_AISystem");
    if (!gEnv->bMultiplayer || (pEnableAI && pEnableAI->GetIVal()))
    {
        m_pAIProxyManager = new CAIProxyManager;
        m_pAIProxyManager->Init();
    }

    // in pure game mode we load the equipment packs from disk
    // in editor mode, this is done in GameEngine.cpp
    if ((m_pItemSystem) && (gEnv->IsEditor() == false))
    {
        LOADING_TIME_PROFILE_SECTION_NAMED("CCryAction::CompleteInit(): EquipmentPacks");
        m_pItemSystem->GetIEquipmentManager()->DeleteAllEquipmentPacks();
        m_pItemSystem->GetIEquipmentManager()->LoadEquipmentPacksFromPath("Libs/EquipmentPacks");
    }

    InlineInitializationProcessing("CCryAction::CompleteInit EquipmentPacks");

    if (gEnv->p3DEngine && gEnv->p3DEngine->GetMaterialManager())
    {
        gEnv->p3DEngine->GetMaterialManager()->GetDefaultLayersMaterial();
    }

    if (IGame* pGame = gEnv->pGame)
    {
        pGame->CompleteInit();
    }

    InlineInitializationProcessing("CCryAction::CompleteInit LoadFlowGraphLibs");

    // load flowgraphs (done after Game has initialized, because it might add additional flownodes)
    if (m_pMaterialEffects)
    {
        m_pMaterialEffects->LoadFlowGraphLibs();
    }

    m_pPrefabManager = std::make_shared<CPrefabManager>();
    m_pScriptBindPF = std::make_unique<CScriptBind_PrefabManager>(m_pPrefabManager, this);

    if (!m_pScriptRMI->Init())
    {
        return false;
    }

    // after everything is initialized, run our main script
    m_pScriptSystem->ExecuteFile("scripts/main.lua");
    m_pScriptSystem->BeginCall("OnInit");
    m_pScriptSystem->EndCall();

    InlineInitializationProcessing("CCryAction::CompleteInit RunMainScript");

    if (gEnv->pAISystem)
    {
        gEnv->pAISystem->CompleteInit();
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_POST_INIT, 0, 0);
    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_POST_INIT_DONE, 0, 0);

    if (gEnv->pMaterialEffects)
    {
        gEnv->pMaterialEffects->CompleteInit();
    }

    if (gEnv->pConsole->GetCVar("g_enableMergedMeshRuntimeAreas")->GetIVal() > 0)
    {
        m_pRuntimeAreaManager = new CRuntimeAreaManager();
    }

    m_screenFaderManager = new Graphics::ScreenFaderManager();

    InlineInitializationProcessing("CCryAction::CompleteInit End");
    return true;
}

//------------------------------------------------------------------------
void CCryAction::InitScriptBinds()
{
    m_pScriptNet = new CScriptBind_Network(m_pSystem, this);
    m_pScriptA = new CScriptBind_Action(this);
    m_pScriptIS = new CScriptBind_ItemSystem(m_pSystem, m_pItemSystem, this);
    m_pScriptAS = new CScriptBind_ActorSystem(m_pSystem, this);
    m_pScriptAMM = new CScriptBind_ActionMapManager(m_pSystem, m_pActionMapManager);

    m_pScriptVS = new CScriptBind_VehicleSystem(m_pSystem, m_pVehicleSystem);
    m_pScriptBindVehicle = new CScriptBind_Vehicle(m_pSystem, this);
    m_pScriptBindVehicleSeat = new CScriptBind_VehicleSeat(m_pSystem, this);

    m_pScriptInventory = new CScriptBind_Inventory(m_pSystem, this);
    m_pScriptBindDS = new CScriptBind_DialogSystem(m_pSystem, m_pDialogSystem);
}

//------------------------------------------------------------------------
void CCryAction::ReleaseScriptBinds()
{
    // before we release script binds call out main "OnShutdown"
    if (m_pScriptSystem)
    {
        m_pScriptSystem->BeginCall("OnShutdown");
        m_pScriptSystem->EndCall();
    }

    SAFE_RELEASE(m_pScriptA);
    SAFE_RELEASE(m_pScriptIS);
    SAFE_RELEASE(m_pScriptAS);
    SAFE_RELEASE(m_pScriptAMM);
    SAFE_RELEASE(m_pScriptVS);
    SAFE_RELEASE(m_pScriptBindVehicle);
    SAFE_RELEASE(m_pScriptBindVehicleSeat);
    SAFE_RELEASE(m_pScriptInventory);
    SAFE_RELEASE(m_pScriptBindDS);
    SAFE_RELEASE(m_pScriptBindMFX);
    m_pScriptBindPF.reset();
}

//------------------------------------------------------------------------
void CCryAction::Shutdown()
{
    XMLCPB::ShutdownCompressorThread();

    SAFE_DELETE(m_pAIDebugRenderer);

    if (s_rcon_server)
    {
        s_rcon_server->Stop();
    }
    if (s_rcon_client)
    {
        s_rcon_client->Disconnect();
    }
    s_rcon_server = nullptr;
    s_rcon_client = nullptr;

    if (m_pGameplayAnalyst)
    {
        m_pGameplayRecorder->UnregisterListener(m_pGameplayAnalyst);
    }

    //NOTE: m_pGFListeners->erase got commented out in UnregisterListener
    //  CRY_ASSERT(0 == m_pGFListeners->size());
    SAFE_DELETE(m_pGFListeners);

    if (gEnv)
    {
        EndGameContext(false);
    }

    if (m_pEntitySystem)
    {
        m_pEntitySystem->Unload();
    }

    if (gEnv)
    {
        IMovieSystem* movieSys = gEnv->pMovieSystem;
        if (movieSys != nullptr)
        {
            movieSys->SetUser(nullptr);
        }
    }

    // profile manager needs to shut down (logout users, ...)
    // while most systems are still up
    if (m_pPlayerProfileManager)
    {
        m_pPlayerProfileManager->Shutdown();
    }

    if (m_pDialogSystem)
    {
        m_pDialogSystem->Shutdown();
    }

    if (m_pFlowSystem)
    {
        m_pFlowSystem->Shutdown();
    }

    FeatureTests::DestroyManager();

    SAFE_DELETE(m_pGamePhysicsSettings);
    SAFE_RELEASE(m_pActionMapManager);
    SAFE_RELEASE(m_pItemSystem);
    SAFE_RELEASE(m_pLevelSystem);
    SAFE_RELEASE(m_pViewSystem);
    SAFE_RELEASE(m_pGameplayRecorder);
    SAFE_RELEASE(m_pGameplayAnalyst);
    SAFE_RELEASE(m_pGameRulesSystem);
    SAFE_RELEASE(m_pSharedParamsManager);
    SAFE_RELEASE(m_pVehicleSystem);
    SAFE_DELETE(m_pMaterialEffects);
    SAFE_DELETE(m_pBreakableGlassSystem);
    SAFE_RELEASE(m_pActorSystem);
    SAFE_DELETE(m_pForceFeedBackSystem);
    SAFE_DELETE(m_pSubtitleManager);
    SAFE_DELETE(m_pUIDraw);
    SAFE_DELETE(m_pGameTokenSystem);
    SAFE_DELETE(m_pEffectSystem);
    SAFE_DELETE(m_pAnimationGraphCvars);
    SAFE_DELETE(m_pGameObjectSystem);
    SAFE_DELETE(m_pMannequin);
    SAFE_RELEASE(m_pFlowSystem);
    SAFE_DELETE(m_pGameSerialize);
    SAFE_DELETE(m_pPersistentDebug);
    SAFE_DELETE(m_pDialogSystem); // maybe delete before
    SAFE_DELETE(m_pTimeOfDayScheduler);
    SAFE_DELETE(m_pLocalAllocs);
    SAFE_DELETE(m_pCooperativeAnimationManager);
    SAFE_DELETE(m_pCustomEventManager)
    SAFE_DELETE(m_pCustomActionManager)

    SAFE_DELETE(m_pGameVolumesManager);
    SAFE_DELETE(m_pScriptRMI);
    SAFE_DELETE(m_pPlayerProfileManager);

    SAFE_DELETE(m_pRuntimeAreaManager);
    SAFE_DELETE(m_colorGradientManager);
    SAFE_DELETE(m_screenFaderManager);

    ReleaseScriptBinds();
    ReleaseCVars();

    SAFE_DELETE(m_pDevMode);
    SAFE_DELETE(m_pCallbackTimer);
    m_pPrefabManager.reset();
#ifndef _RELEASE
    SAFE_DELETE(m_debugCamera);
#endif
    CSignalTimer::Shutdown();
    CRangeSignaling::Shutdown();

    CVisualLog::ref().Shutdown();

    if (m_pAIProxyManager)
    {
        m_pAIProxyManager->Shutdown();
        SAFE_DELETE(m_pAIProxyManager);
    }

    if (m_contextBridge)
    {
        m_contextBridge->Shutdown();
        delete m_contextBridge;
    }

    stl::free_container(m_frameworkExtensions);

    XMLCPB::CDebugUtils::Destroy();

    if (m_pSystem)
    {
        m_pSystem->GetISystemEventDispatcher()->RemoveListener(&g_system_event_listener_action);
    }

    // having a dll handle means we did create the system interface
    // so we must release it
    SAFE_RELEASE(m_pSystem);
    if (m_crySystemModule->IsLoaded())
    {
        typedef void*( * PtrFunc_ModuleShutdownISystem )(ISystem* pSystem);

        PtrFunc_ModuleShutdownISystem pfnModuleShutdownISystem = m_crySystemModule->GetFunction<PtrFunc_ModuleShutdownISystem>("ModuleShutdownISystem");

        if (pfnModuleShutdownISystem)
        {
            pfnModuleShutdownISystem(m_pSystem);
        }

        m_crySystemModule->Unload();
    }

    SAFE_DELETE(m_nextFrameCommand);
    SAFE_DELETE(m_pPhysicsQueues);

    m_pThis = 0;

#ifndef AZ_MONOLITHIC_BUILD
    ModuleShutdownISystem(m_pSystem);
#endif // AZ_MONOLITHIC_BUILD
}


bool CCryAction::PreUpdate(bool haveFocus, unsigned int updateFlags)
{
    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    if (updateFlags & ESYSUPDATE_UPDATE_VIEW_ONLY)
    {
        if (m_pViewSystem)
        {
            m_pViewSystem->Update(min(gEnv->pTimer->GetFrameTime(), 0.1f));
        }
        return true;
    }

    if (!m_nextFrameCommand->empty())
    {
        gEnv->pConsole->ExecuteString(*m_nextFrameCommand);
        m_nextFrameCommand->resize(0);
    }

    CheckEndLevelSchedule();

    if (ITextModeConsole* pTextModeConsole = gEnv->pSystem->GetITextModeConsole())
    {
        pTextModeConsole->BeginDraw();
    }

    bool gameRunning = IsGameStarted();

    bool bGameIsPaused = !gameRunning || IsGamePaused(); // slightly different from m_paused (check's gEnv->pTimer as well)
    if (!IsGamePaused())
    {
        TimeDemoRecorderBus::Broadcast(&TimeDemoRecorderBus::Events::PreUpdate);
    }

    // update the callback system
    if (!bGameIsPaused)
    {
        if (m_pCallbackTimer)
        {
            m_pCallbackTimer->Update();
        }
    }

    CALL_FRAMEWORK_LISTENERS(OnPreUpdate());
    
    bool bRetRun = true;

    if (!(updateFlags & ESYSUPDATE_EDITOR))
    {
        gEnv->pFrameProfileSystem->StartFrame();
    }

    m_pSystem->RenderBegin();


    // when we are updated by the editor, we should not update the system
    if (!(updateFlags & ESYSUPDATE_EDITOR))
    {
        int updateLoopPaused = (!gameRunning || m_paused) ? 1 : 0;
        if (gEnv->pRenderer->IsPost3DRendererEnabled())
        {
            updateLoopPaused = 0;
            updateFlags |= ESYSUPDATE_IGNORE_AI;
        }

        bRetRun = m_pSystem->UpdatePreTickBus(updateFlags, updateLoopPaused);
    }

    return bRetRun;
}

//------------------------------------------------------------------------
bool CCryAction::PostUpdate(bool haveFocus, unsigned int updateFlags)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

    bool bRetRun = true;
    bool gameRunning = IsGameStarted();
    bool bGameIsPaused = !gameRunning || IsGamePaused(); // slightly different from m_paused (check's gEnv->pTimer as well)
    float frameTime(gEnv->pTimer->GetFrameTime());

    LARGE_INTEGER updateStart, updateEnd;
    updateStart.QuadPart = 0;
    updateEnd.QuadPart = 0;

    // when we are updated by the editor, we should not update the system
    if (!(updateFlags & ESYSUPDATE_EDITOR))
    {
        int updateLoopPaused = (!gameRunning || m_paused) ? 1 : 0;
        if (gEnv->pRenderer->IsPost3DRendererEnabled())
        {
            updateLoopPaused = 0;
            updateFlags |= ESYSUPDATE_IGNORE_AI;
        }

        const bool bGameWasPaused = bGameIsPaused;

        bRetRun = m_pSystem->UpdatePostTickBus(updateFlags, updateLoopPaused);

#ifdef ENABLE_LW_PROFILERS
        CryProfile::PushProfilingMarker("ActionPreUpdate");
        QueryPerformanceCounter(&updateStart);
#endif
        // notify listeners
        OnActionEvent(SActionEvent(eAE_earlyPreUpdate));

        // during m_pSystem->Update call the Game might have been paused or un-paused
        gameRunning = IsGameStarted() && m_pGame && m_pGame->IsInited();
        bGameIsPaused = !gameRunning || IsGamePaused();

        if (!bGameIsPaused && !bGameWasPaused) // don't update gameplayrecorder if paused
        {
            if (m_pGameplayRecorder)
            {
                m_pGameplayRecorder->Update(frameTime);
            }
        }

        if (!bGameIsPaused && gameRunning)
        {
            if (m_pFlowSystem)
            {
                m_pFlowSystem->Update();
            }
        }

        gEnv->pMovieSystem->ControlCapture();

        // if the Game had been paused and is now unpaused, don't update viewsystem until next frame
        // if the Game had been unpaused and is now paused, don't update viewsystem until next frame
        if (m_pViewSystem)
        {
            const bool useDeferredViewSystemUpdate = m_pViewSystem->UseDeferredViewSystemUpdate();
            if (!useDeferredViewSystemUpdate)
            {
                if (!bGameIsPaused && !bGameWasPaused) // don't update view if paused
                {
                    m_pViewSystem->Update(min(frameTime, 0.1f));
                }
            }
        }
    }

    m_pActionMapManager->Update();

    m_pForceFeedBackSystem->Update(frameTime);

    if (m_pPhysicsQueues)
    {
        m_pPhysicsQueues->Update(frameTime);
    }

    if (!bGameIsPaused)
    {
        if (m_pItemSystem)
        {
            m_pItemSystem->Update();
        }

        if (m_pMaterialEffects)
        {
            m_pMaterialEffects->Update(frameTime);
        }

        if (m_pBreakableGlassSystem)
        {
            m_pBreakableGlassSystem->Update(frameTime);
        }

        if (m_pDialogSystem)
        {
            m_pDialogSystem->Update(frameTime);
        }

        if (m_pVehicleSystem)
        {
            m_pVehicleSystem->Update(frameTime);
        }

        if (m_pCooperativeAnimationManager)
        {
            m_pCooperativeAnimationManager->Update(frameTime);
        }

        if (m_colorGradientManager)
        {
            m_colorGradientManager->UpdateForThisFrame(frameTime);
        }
    }

    CRConServerListener::GetSingleton().Update();

#if !defined(_RELEASE)
    CRealtimeRemoteUpdateListener::GetRealtimeRemoteUpdateListener().Update();
#endif //_RELEASE

    if (!(updateFlags & ESYSUPDATE_EDITOR))
    {
#ifdef ENABLE_LW_PROFILERS
        QueryPerformanceCounter(&updateEnd);
        m_PreUpdateTicks += (uint32)(updateEnd.QuadPart - updateStart.QuadPart);
        CryProfile::PopProfilingMarker();
#endif
    }

    if (updateFlags & ESYSUPDATE_UPDATE_VIEW_ONLY)
    {
        m_pSystem->RenderBegin();
        gEnv->p3DEngine->PrepareOcclusion(m_pSystem->GetViewCamera());
        m_pSystem->Render();
        gEnv->p3DEngine->EndOcclusion();
        m_pSystem->RenderEnd();
        return bRetRun;
    }

    FeatureTests::UpdateCamera();

    if (updateFlags & ESYSUPDATE_EDITOR_ONLY)
    {
        return bRetRun;
    }

    if (updateFlags & ESYSUPDATE_EDITOR_AI_PHYSICS)
    {
        float delta = gEnv->pTimer->GetFrameTime();

        if (!gEnv->bMultiplayer)
        {
            CRangeSignaling::ref().SetDebug(m_pDebugRangeSignaling->GetIVal() == 1);
            CRangeSignaling::ref().Update(delta);

            CSignalTimer::ref().SetDebug(m_pDebugSignalTimers->GetIVal() == 1);
            CSignalTimer::ref().Update(delta);
        }

        // begin occlusion job after setting the correct camera
        gEnv->p3DEngine->PrepareOcclusion(m_pSystem->GetViewCamera());

        // synchronize all animations so ensure that their computation have finished
        gEnv->pCharacterManager->SyncAllAnimations();

        m_pSystem->Render();

        if (m_pPersistentDebug)
        {
            m_pPersistentDebug->PostUpdate(delta);
        }

        if (m_pGameObjectSystem)
        {
            m_pGameObjectSystem->PostUpdate(delta);
        }

        return bRetRun;
    }

    if (gEnv->pLyShine)
    {
        // Tell the UI system the size of the viewport we are rendering to - this drives the
        // canvas size for full screen UI canvases. It needs to be set before either pLyShine->Update or
        // pLyShine->Render are called. It must match the viewport size that the input system is using.
        AZ::Vector2 viewportSize;
        viewportSize.SetX(static_cast<float>(gEnv->pRenderer->GetOverlayWidth()));
        viewportSize.SetY(static_cast<float>(gEnv->pRenderer->GetOverlayHeight()));
        gEnv->pLyShine->SetViewportSize(viewportSize);

        bool isUiPaused = gEnv->pTimer->IsTimerPaused(ITimer::ETIMER_UI);
        if (!isUiPaused)
        {
            gEnv->pLyShine->Update(gEnv->pTimer->GetFrameTime(ITimer::ETIMER_UI));
        }
    }

    float delta = gEnv->pTimer->GetFrameTime();

    if (!IsGamePaused())
    {
        if (m_pEffectSystem)
        {
            m_pEffectSystem->Update(delta);
        }
    }

    if (m_lastSaveLoad)
    {
        if (m_lastSaveLoad < 0.0f)
        {
            m_lastSaveLoad = 0.0f;
        }
    }

    // synchronize all animations so ensure that their computation have finished
    if (gEnv->pCharacterManager)
    {
        gEnv->pCharacterManager->SyncAllAnimations();
    }

    const bool useDeferredViewSystemUpdate = m_pViewSystem->UseDeferredViewSystemUpdate();
    if (useDeferredViewSystemUpdate)
    {
        m_pViewSystem->Update(min(delta, 0.1f));
    }

    // Begin occlusion job after setting the correct camera.
    gEnv->p3DEngine->PrepareOcclusion(m_pSystem->GetViewCamera());

    CALL_FRAMEWORK_LISTENERS(OnPreRender());

    // Also broadcast for anyone else that needs to draw global debug to do so now
    AzFramework::DebugDisplayEventBus::Broadcast(&AzFramework::DebugDisplayEvents::DrawGlobalDebugInfo);

    m_pSystem->Render();

    gEnv->p3DEngine->EndOcclusion();

    if (m_pPersistentDebug)
    {
        m_pPersistentDebug->PostUpdate(delta);
    }

    CALL_FRAMEWORK_LISTENERS(OnPostUpdate(delta));

    m_pGameObjectSystem->PostUpdate(delta);

    CRangeSignaling::ref().SetDebug(m_pDebugRangeSignaling->GetIVal() == 1);
    CRangeSignaling::ref().Update(delta);

    CSignalTimer::ref().SetDebug(m_pDebugSignalTimers->GetIVal() == 1);
    CSignalTimer::ref().Update(delta);

    m_pSystem->RenderEnd();

    if (m_pGame)
    {
        if (m_pGame->Update())
        {
            // we need to do disconnect otherwise GameDLL does not realize that
            // the game has gone
            if (CCryActionCVars::Get().g_allowDisconnectIfUpdateFails)
            {
                gEnv->pConsole->ExecuteString("disconnect");
            }
            else
            {
                CryLog("m_pGame->Update has failed but cannot disconnect as g_allowDisconnectIfUpdateFails is not set");
            }
        }
    }

    if (!IsGamePaused())
    {
        TimeDemoRecorderBus::Broadcast(&TimeDemoRecorderBus::Events::PostUpdate);
    }

    if (!(updateFlags & ESYSUPDATE_EDITOR))
    {
        gEnv->pFrameProfileSystem->EndFrame();
    }

    if (gEnv->pStatoscope)
    {
        gEnv->pStatoscope->Tick();
    }

    if (m_delayedSaveCountDown)
    {
        --m_delayedSaveCountDown;
    }
    else if (m_delayedSaveGameMethod != eSGM_NoSave && m_pLocalAllocs)
    {
        const bool quick = m_delayedSaveGameMethod == eSGM_QuickSave ? true : false;
        SaveGame(m_pLocalAllocs->m_delayedSaveGameName, quick, true, m_delayedSaveGameReason, false, m_pLocalAllocs->m_checkPointName.c_str());
        m_delayedSaveGameMethod = eSGM_NoSave;
        m_pLocalAllocs->m_delayedSaveGameName.assign ("");
    }


    if (ITextModeConsole* pTextModeConsole = gEnv->pSystem->GetITextModeConsole())
    {
        pTextModeConsole->EndDraw();
    }

    CGameObject::UpdateSchedulingProfiles();

    gEnv->p3DEngine->SyncProcessStreamingUpdate();

    if (m_pSystem->NeedDoWorkDuringOcclusionChecks())
    {
        m_pSystem->DoWorkDuringOcclusionChecks();
    }
    FeatureTests::Update();
    // Sync the work that must be done in the main thread by the end of frame.
    gEnv->pRenderer->GetGenerateShadowRendItemJobExecutor()->WaitForCompletion();
    gEnv->pRenderer->GetGenerateRendItemJobExecutor()->WaitForCompletion();
        

    return bRetRun;
}

uint32 CCryAction::GetPreUpdateTicks()
{
    uint32 ticks = m_PreUpdateTicks;
    m_PreUpdateTicks = 0;
    return ticks;
}

void CCryAction::Reset(bool clients)
{
    CGameContext* pGC = GetGameContext();
    if (pGC && pGC->HasContextFlag(eGSF_Server))
    {
        pGC->ResetMap(true);
    }

    if (m_pGameplayRecorder)
    {
        m_pGameplayRecorder->Event(0, GameplayEvent(eGE_GameReset));
    }
}

void CCryAction::PauseGame(bool pause, bool force, unsigned int nFadeOutInMS)
{
    // we should generate some events here
    // who is up to receive them ?

    if (!force && m_paused && m_forcedpause)
    {
        return;
    }

    if (m_paused != pause || m_forcedpause != force)
    {
        gEnv->pTimer->PauseTimer(ITimer::ETIMER_GAME, pause);

        // no game input should happen during pause
        // LEAVE THIS COMMENTED CODE IN - we might still need to uncommented it if this would give any issues
        // This has been commented out since 8/22/2014.
        //m_pActionMapManager->Enable(!pause);

        // Audio: notify the audio system!

        if (pause && gEnv->pInput)   //disable rumble
        {
            gEnv->pInput->ForceFeedbackEvent(SFFOutputEvent(eIDT_Gamepad, eFF_Rumble_Basic, SFFTriggerOutputData::Initial::ZeroIt, 0.0f, 0.0f, 0.0f));
        }

        gEnv->p3DEngine->GetTimeOfDay()->SetPaused(pause);

        // pause EntitySystem Timers
        gEnv->pEntitySystem->PauseTimers(pause);

        m_paused = pause;
        m_forcedpause = m_paused ? force : false;

        if (gEnv->pMovieSystem)
        {
            if (pause)
            {
                gEnv->pMovieSystem->Pause();
            }
            else
            {
                gEnv->pMovieSystem->Resume();
            }
        }

        if (m_paused)
        {
            SGameObjectEvent evt(eGFE_PauseGame, eGOEF_ToAll);
            m_pGameObjectSystem->BroadcastEvent(evt);
            GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_PAUSED, 0, 0);
        }
        else
        {
            SGameObjectEvent evt(eGFE_ResumeGame, eGOEF_ToAll);
            m_pGameObjectSystem->BroadcastEvent(evt);
            GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_RESUMED, 0, 0);
        }
    }
}

bool CCryAction::IsGamePaused()
{
    return m_paused || !gEnv->pTimer->IsTimerEnabled();
}

bool CCryAction::IsGameStarted()
{
    CGameContext* pGameContext = m_pGame ?
        m_pGame->GetGameContext() :
        nullptr;

    bool bStarted = pGameContext ?
        pGameContext->IsGameStarted() :
        false;

    return bStarted;
}

void CCryAction::MarkGameStarted(bool started)
{
    if (m_pGame && m_pGame->GetGameContext())
    {
        m_pGame->GetGameContext()->SetGameStarted(started);
    }
}

bool CCryAction::IsLevelPrecachingDone() const
{
    return m_levelPrecachingDone || gEnv->IsEditor();
}

void CCryAction::SetLevelPrecachingDone(bool bValue)
{
    m_levelPrecachingDone = bValue;
}

bool CCryAction::StartGameContext(const SGameStartParams* pGameStartParams)
{
    LOADING_TIME_PROFILE_SECTION;

    // Clear the load failed flag
    GetILevelSystem()->SetLevelLoadFailed(false);

    if (gEnv->IsEditor())
    {
        if (!m_pGame)
        {
            CryFatalError("Must have game around always for editor");
        }
    }
    else
    {
        m_pGame = new CActionGame(m_pScriptRMI);
    }

    // Unlock shared parameters manager. This must happen after the level heap is initialised and before level load.

    if (m_pSharedParamsManager)
    {
        m_pSharedParamsManager->Unlock();
    }

    m_contextBridge->OnStartGameContext(pGameStartParams);
    if (!m_pGame->Init(pGameStartParams))
    {
        GameWarning("Failed initializing game");

        GetILevelSystem()->SetLevelLoadFailed(true);
        gEnv->pRenderer->OnLevelLoadFailed();

        EndGameContext(false);
        return false;
    }

    return true;
}

bool CCryAction::BlockingSpawnPlayer()
{
    CRY_ASSERT(0 && "BlockingSpawnPlayer() Function Deprecated");
    return true;
}

void CCryAction::ResetBrokenGameObjects()
{
    if (m_pGame)
    {
        m_pGame->FixBrokenObjects(true);
        m_pGame->ClearBreakHistory();
    }
}

/////////////////////////////////////////
// CloneBrokenObjectsAndRevertToStateAtTime() is called by the kill cam. It takes a list of indices into
//      the break events array, which it uses to find the objects that were broken during the kill cam, and
//      therefore need to be cloned for playback. It clones the broken objects, and hides the originals. It
//      then reverts the cloned objects to their unbroken state, and re-applies any breaks that occured up
//      to the start of the kill cam.
void CCryAction::CloneBrokenObjectsAndRevertToStateAtTime(int32 iFirstBreakEventIndex, uint16* pBreakEventIndices, int32& iNumBreakEvents, IRenderNode** outClonedNodes, int32& iNumClonedNodes, SRenderNodeCloneLookup& renderNodeLookup)
{
    if (m_pGame)
    {
        m_pGame->CloneBrokenObjectsByIndex(pBreakEventIndices, iNumBreakEvents, outClonedNodes, iNumClonedNodes, renderNodeLookup);

        m_pGame->HideBrokenObjectsByIndex(pBreakEventIndices, iNumBreakEvents);

        m_pGame->ApplyBreaksUntilTimeToObjectList(iFirstBreakEventIndex, renderNodeLookup);
    }
}

/////////////////////////////////////////
// ApplySingleProceduralBreakFromEventIndex() re-applies a single break to a cloned object, for use in the
//      kill cam during playback
void CCryAction::ApplySingleProceduralBreakFromEventIndex(uint16 uBreakEventIndex, const SRenderNodeCloneLookup& renderNodeLookup)
{
    if (m_pGame)
    {
        m_pGame->ApplySingleProceduralBreakFromEventIndex(uBreakEventIndex, renderNodeLookup);
    }
}

/////////////////////////////////////////
// UnhideBrokenObjectsByIndex() is provided a list of indices into the Break Event array, that it uses
//      to look up and unhide broken objects assocated with the break events. This is used by the kill
//      cam when it finishes playback
void CCryAction::UnhideBrokenObjectsByIndex(uint16* pObjectIndicies, int32 iNumObjectIndices)
{
    if (m_pGame)
    {
        m_pGame->UnhideBrokenObjectsByIndex(pObjectIndicies, iNumObjectIndices);
    }
}

bool CCryAction::ChangeGameContext(const SGameContextParams* pGameContextParams)
{
    if (!m_pGame)
    {
        return false;
    }
    return m_pGame->ChangeGameContext(pGameContextParams);
}

void CCryAction::EndGameContext(bool loadEmptyLevel)
{
    LOADING_TIME_PROFILE_SECTION;
    if (!gEnv) // SCA fix
    {
        return;
    }

#ifndef _RELEASE
    CryLog ("Ending game context...");
    INDENT_LOG_DURING_SCOPE();
#endif

    // to make this function re-entrant, m_pGame pointer must be set to 0
    // BEFORE the destructor of CActionGame is invoked (Craig)
    _smart_ptr<CActionGame> pGame = m_pGame;

    // Flush core buses. We're about to unload Cry modules and need to ensure we don't have module-owned functions left behind.
    AZ::Data::AssetBus::ExecuteQueuedEvents();
    AZ::TickBus::ExecuteQueuedEvents();
    AZ::MainThreadRenderRequestBus::ExecuteQueuedEvents();

    m_contextBridge->OnEndGameContext();
    m_pGame = 0;
    pGame = 0;

    if (m_pSharedParamsManager)
    {
        m_pSharedParamsManager->Reset();
    }

    if (m_pVehicleSystem)
    {
        m_pVehicleSystem->Reset();
    }

    if (m_pScriptRMI)
    {
        m_pScriptRMI->UnloadLevel();
    }

    // Perform level unload procedures for the LyShine UI system
    if (gEnv && gEnv->pLyShine)
    {
        gEnv->pLyShine->OnLevelUnload();
    }

    if (gEnv && gEnv->IsEditor())
    {
        m_pGame = new CActionGame(m_pScriptRMI);
    }

    if (m_pActorSystem)
    {
        m_pActorSystem->Reset();
    }

    if (m_nextFrameCommand)
    {
        stl::free_container(*m_nextFrameCommand);
    }

    if (m_pLocalAllocs)
    {
        stl::free_container(m_pLocalAllocs->m_delayedSaveGameName);
        stl::free_container(m_pLocalAllocs->m_checkPointName);
        stl::free_container(m_pLocalAllocs->m_nextLevelToLoad);
        m_pLocalAllocs->m_clearRenderBufferToBlackOnUnload = true;
    }

    if (gEnv && gEnv->pSystem)
    {
        // clear all error messages to prevent stalling due to runtime file access check during chainloading
        gEnv->pSystem->ClearErrorMessages();
    }

    if (gEnv && gEnv->pCryPak)
    {
        gEnv->pCryPak->DisableRuntimeFileAccess(false);
    }

    // Reset and lock shared parameters manager. This must happen after level unload and before the level heap is released.

    if (m_pSharedParamsManager)
    {
        m_pSharedParamsManager->Reset();

        m_pSharedParamsManager->Lock();
    }

    if (loadEmptyLevel && m_p3DEngine)
    {
        m_p3DEngine->LoadEmptyLevel();
    }
}

void CCryAction::InitEditor(IGameToEditorInterface* pGameToEditor)
{
    uint32 commConfigCount = gEnv->pAISystem->GetCommunicationManager()->GetConfigCount();
    if (commConfigCount)
    {
        std::vector<const char*> configNames;
        configNames.resize(commConfigCount);

        for (uint i = 0; i < commConfigCount; ++i)
        {
            configNames[i] = gEnv->pAISystem->GetCommunicationManager()->GetConfigName(i);
        }

        pGameToEditor->SetUIEnums("CommConfig", &configNames.front(), commConfigCount);
    }

    const char* behaviorSelectionTreeType = "BehaviorSelectionTree";
    uint32 behaviorSelectionTreeCount = gEnv->pAISystem->GetSelectionTreeManager()
            ->GetSelectionTreeCountOfType(behaviorSelectionTreeType);

    if (behaviorSelectionTreeCount)
    {
        std::vector<const char*> selectionTreeNames;
        selectionTreeNames.resize(behaviorSelectionTreeCount + 1);
        selectionTreeNames[0] = "None";

        for (uint i = 0; i < behaviorSelectionTreeCount; ++i)
        {
            selectionTreeNames[i + 1] = gEnv->pAISystem->GetSelectionTreeManager()
                    ->GetSelectionTreeNameOfType(behaviorSelectionTreeType, i);
        }

        pGameToEditor->SetUIEnums(behaviorSelectionTreeType, &selectionTreeNames.front(), behaviorSelectionTreeCount + 1);
    }

    uint32 factionCount = gEnv->pAISystem->GetFactionMap().GetFactionCount();
    if (factionCount)
    {
        std::vector<const char*> factionNames;
        factionNames.resize(factionCount + 1);
        factionNames[0] = "";

        for (uint32 i = 0; i < factionCount; ++i)
        {
            factionNames[i + 1] = gEnv->pAISystem->GetFactionMap().GetFactionName(i);
        }

        pGameToEditor->SetUIEnums("Faction", &factionNames.front(), factionCount + 1);
        pGameToEditor->SetUIEnums("FactionFilter", &factionNames.front(), factionCount + 1);
    }

    if (factionCount)
    {
        const uint32 reactionCount = 4;
        const char* reactions[reactionCount] = {
            "",
            "Hostile",
            "Neutral",
            "Friendly",
        };

        pGameToEditor->SetUIEnums("Reaction", reactions, reactionCount);
        pGameToEditor->SetUIEnums("ReactionFilter", reactions, reactionCount);
    }

    uint32 agentTypeCount = gEnv->pAISystem->GetNavigationSystem()->GetAgentTypeCount();
    if (agentTypeCount)
    {
        std::vector<const char*> agentTypeNames;
        agentTypeNames.resize(agentTypeCount + 1);
        agentTypeNames[0] = "";

        for (size_t i = 0; i < agentTypeCount; ++i)
        {
            agentTypeNames[i + 1] = gEnv->pAISystem->GetNavigationSystem()->GetAgentTypeName(
                    gEnv->pAISystem->GetNavigationSystem()->GetAgentTypeID(i));
        }

        pGameToEditor->SetUIEnums("NavigationType", &agentTypeNames.front(), agentTypeCount + 1);
    }

    uint32 lipSyncCount = gs_lipSyncExtensionNamesForExposureToEditor.size();
    if (lipSyncCount)
    {
        pGameToEditor->SetUIEnums("LipSyncType", &gs_lipSyncExtensionNamesForExposureToEditor.front(), lipSyncCount);
    }

    // intialize audio-specific UI enums...
    std::vector<const char*> audioEnums {
        "Ignore=0", "SingleRay=1", "MultipleRays=2"
    };
    pGameToEditor->SetUIEnums("SoundObstructionType", audioEnums.data(), audioEnums.size());
}

//------------------------------------------------------------------------
void CCryAction::SetEditorLevel(const char* levelName, const char* levelFolder)
{
    cry_strcpy(m_editorLevelName, levelName);
    cry_strcpy(m_editorLevelFolder, levelFolder);
}

//------------------------------------------------------------------------
void CCryAction::GetEditorLevel(char** levelName, char** levelFolder)
{
    if (levelName)
    {
        *levelName = &m_editorLevelName[0];
    }
    if (levelFolder)
    {
        *levelFolder = &m_editorLevelFolder[0];
    }
}

//------------------------------------------------------------------------
const char* CCryAction::GetStartLevelSaveGameName()
{
    // GetStartLevelSaveGameName is deprecated because it is used in only one place in the engine,
    // and it is not a useful place. In TimeDemoRecorder, when playback of a session loops, it attempts to
    // load a file with this name. Nothing in the engine ever actually saves a file with this name.
    AZ_Warning("Deprecation", false, "CCryAction::GetStartLevelSaveGameName is deprecated.");
    static string levelstart;
#if defined(CONSOLE)
    levelstart = LY_SAVEGAME_FILENAME;
#else
    levelstart = (gEnv->pGame->GetIGameFramework()->GetLevelName());
    if (const char* mappedName = gEnv->pGame->GetMappedLevelName(levelstart.c_str()))
    {
        levelstart = mappedName;
    }
    levelstart.append("_");
    // IGame::GetName is deprecated.
    const char *gameName = gEnv->pGame->GetName();
    if (gameName == nullptr)
    {
        // Using sys_dll_game instead of sys_game_name because dll_game should be safe to use as a file name,
        // but sys_game_name can have invalid characters for file names.
        ICVar* gameNameCVar = gEnv->pConsole->GetCVar("sys_dll_game");
        if (gameNameCVar)
        {
            gameName = gameNameCVar->GetString();
        }
        if (gameName == nullptr)
        {
            gameName = LY_SAVEGAME_FILENAME;
        }
    }
#endif
    levelstart.append(LY_SAVEGAME_FILE_EXT);
    return levelstart.c_str();
}

static void BroadcastCheckpointEvent(ESaveGameReason)
{
}


//------------------------------------------------------------------------
bool CCryAction::SaveGame(const char* path, bool bQuick, bool bForceImmediate, ESaveGameReason reason, bool ignoreDelay, const char* checkPointName)
{
    CryLog ("[SAVE GAME] %s to '%s'%s%s - checkpoint=\"%s\"", bQuick ? "Quick-saving" : "Saving", path, bForceImmediate ? " immediately" : "Delayed", ignoreDelay ? " ignoring delay" : "", checkPointName);
    INDENT_LOG_DURING_SCOPE();

    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    if (gEnv->bMultiplayer)
    {
        return false;
    }

    IActor* pClientActor = GetClientActor();
    if (!pClientActor)
    {
        return false;
    }

    if (pClientActor->IsDead())
    {
        //don't save when the player already died - savegame will be corrupt
        GameWarning("Saving failed : player is dead!");
        return false;
    }

    if (CanSave() == false)
    {
        // When in time demo but not chain loading we need to allow the level start save
        bool timeDemoChainLoading = false;
        TimeDemoRecorderBus::BroadcastResult(timeDemoChainLoading, &TimeDemoRecorderBus::Events::IsChainLoading);

        bool isTimeDemoActive = false;
        TimeDemoRecorderBus::BroadcastResult(isTimeDemoActive, &TimeDemoRecorderBus::Events::IsTimeDemoActive);

        bool bIsInTimeDemoButNotChainLoading = isTimeDemoActive && !timeDemoChainLoading;
        if (!(reason == eSGR_LevelStart && bIsInTimeDemoButNotChainLoading))
        {
            ICVar* saveLoadEnabled = gEnv->pConsole->GetCVar("g_EnableLoadSave");
            bool enabled = saveLoadEnabled->GetIVal() == 1;
            if (enabled)
            {
                GameWarning("CCryAction::SaveGame: Suppressing QS (likely due to timedemo or cutscene)");
            }
            else
            {
                GameWarning("CCryAction::SaveGame: Suppressing QS (g_EnableLoadSave is disabled)");
            }
            return false;
        }
    }

    if (m_lastSaveLoad > 0.0f)
    {
        if (ignoreDelay)
        {
            m_lastSaveLoad = 0.0f;
        }
        else
        {
            return false;
        }
    }

    bool bRet = true;
    if (bForceImmediate)
    {
        // Ensure we have the save checkpoint icon going on by telling IPlatformOS that the following writes are for a checkpoint.
        if (m_delayedSaveGameMethod == eSGM_NoSave)
        {
            BroadcastCheckpointEvent(reason);
        }

        // Best save profile or we'll lose persistent stats
        if (m_pPlayerProfileManager)
        {
            IPlayerProfileManager::EProfileOperationResult result;
            m_pPlayerProfileManager->SaveProfile(m_pPlayerProfileManager->GetCurrentUser(), result, ePR_Game);
        }

#if ENABLE_STATOSCOPE
        if (gEnv->pStatoscope)
        {
            CryFixedStringT<128> s;
            switch (reason)
            {
            case eSGR_Command:
                s = "Command";
                break;
            case eSGR_FlowGraph:
                s = "FlowGraph";
                break;
            case eSGR_LevelStart:
                s = "LevelStart";
                break;
            case eSGR_QuickSave:
                s = "QuickSave";
                break;
            default:
                s = "Unknown Reason";
                break;
            }
            if (checkPointName && *checkPointName)
            {
                s += " - ";
                s += checkPointName;
            }
            gEnv->pStatoscope->AddUserMarker("SaveGame", s.c_str());
        }
#endif

        // check, if preSaveGame has been called already
        if (m_pLocalAllocs && m_pLocalAllocs->m_delayedSaveGameName.empty())
        {
            OnActionEvent(SActionEvent(eAE_preSaveGame, (int) reason));
        }
        CTimeValue elapsed = -gEnv->pTimer->GetAsyncTime();
        gEnv->pSystem->SerializingFile(bQuick ? 1 : 2);
        bRet = m_pGameSerialize ? m_pGameSerialize->SaveGame(this, "xml", path, reason, checkPointName) : false;
        gEnv->pSystem->SerializingFile(0);
        OnActionEvent(SActionEvent(eAE_postSaveGame, (int) reason, bRet ? (checkPointName ? checkPointName : "") : 0));
        m_lastSaveLoad = s_loadSaveDelay;
        elapsed += gEnv->pTimer->GetAsyncTime();

        if (!bRet)
        {
            GameWarning("[CryAction] SaveGame: '%s' %s. [Duration=%.4f secs]", path, "failed", elapsed.GetSeconds());
        }
        else
        {
            CryLog("SaveGame: '%s' %s. [Duration=%.4f secs]", path, "done", elapsed.GetSeconds());
        }

#if !defined(_RELEASE)
        ICVar* pCVar = gEnv->pConsole->GetCVar("g_displayCheckpointName");

        if (pCVar && pCVar->GetIVal())
        {
            IPersistentDebug* pPersistentDebug = GetIPersistentDebug();

            if (pPersistentDebug)
            {
                static const ColorF colour(1.0f, 0.0f, 0.0f, 1.0f);

                char                                text[128];

                azsnprintf(text, sizeof(text), "Saving Checkpoint '%s'\n", checkPointName ? checkPointName : "unknown");

                pPersistentDebug->Add2DText(text, 32.f, colour, 10.0f);
            }
        }
#endif //_RELEASE
    }
    else
    {
        // Delayed save. Allows us to render the saving warning icon briefly before committing
        BroadcastCheckpointEvent(reason);

        m_delayedSaveGameMethod = bQuick ? eSGM_QuickSave : eSGM_Save;
        m_delayedSaveGameReason = reason;
        m_delayedSaveCountDown  = s_saveGameFrameDelay;
        if (m_pLocalAllocs)
        {
            m_pLocalAllocs->m_delayedSaveGameName = path;
            if (checkPointName)
            {
                m_pLocalAllocs->m_checkPointName = checkPointName;
            }
            else
            {
                m_pLocalAllocs->m_checkPointName.clear();
            }
        }
        OnActionEvent(SActionEvent(eAE_preSaveGame, (int) reason));
    }
    return bRet;
}

//------------------------------------------------------------------------
ELoadGameResult CCryAction::LoadGame(const char* path, bool quick, bool ignoreDelay)
{
    CryLog ("[LOAD GAME] %s saved game '%s'%s", quick ? "Quick-loading" : "Loading", path, ignoreDelay ? " ignoring delay" : "");
    INDENT_LOG_DURING_SCOPE();

    if (gEnv->bMultiplayer)
    {
        return eLGR_Failed;
    }

    if (m_lastSaveLoad > 0.0f)
    {
        if (ignoreDelay)
        {
            m_lastSaveLoad = 0.0f;
        }
        else
        {
            return eLGR_Failed;
        }
    }

    if (CanLoad() == false)
    {
        ICVar* saveLoadEnabled = gEnv->pConsole->GetCVar("g_EnableLoadSave");
        bool enabled = saveLoadEnabled->GetIVal() == 1;
        if (enabled)
        {
            GameWarning("CCryAction::SaveGame: Suppressing QL (likely manually disabled)");
        }
        else
        {
            GameWarning("CCryAction::SaveGame: Suppressing QL (g_EnableLoadSave is disabled)");
        }
        return eLGR_Failed;
    }

    CTimeValue elapsed = -gEnv->pTimer->GetAsyncTime();

    gEnv->pSystem->SerializingFile(quick ? 1 : 2);

    SGameStartParams params;
    params.flags = eGSF_Server | eGSF_Client;
    params.hostname = "localhost";
    params.maxPlayers = 1;

    //pause entity event timers update
    gEnv->pEntitySystem->PauseTimers(true, false);

    // Restore game persistent stats from profile to avoid exploits
    if (m_pPlayerProfileManager)
    {
        const char* userName = m_pPlayerProfileManager->GetCurrentUser();
        IPlayerProfile* pProfile = m_pPlayerProfileManager->GetCurrentProfile(userName);
        if (pProfile)
        {
            m_pPlayerProfileManager->ReloadProfile(pProfile, ePR_Game);
        }
    }

    GameWarning("[CryAction] LoadGame: '%s'", path);

    ELoadGameResult loadResult = m_pGameSerialize ? m_pGameSerialize->LoadGame(this, "xml", path, params, quick) : eLGR_Failed;

    switch (loadResult)
    {
    case eLGR_Ok:
        gEnv->pEntitySystem->PauseTimers(false, false);
        GetISystem()->SerializingFile(0);

        // AllowSave never needs to be serialized, but reset here, because
        // if we had saved a game before it is obvious that at that point saving was not prohibited
        // otherwise we could not have saved it beforehand
        AllowSave(true);

        elapsed += gEnv->pTimer->GetAsyncTime();
        CryLog("[LOAD GAME] LoadGame: '%s' done. [Duration=%.4f secs]", path, elapsed.GetSeconds());
        m_lastSaveLoad = s_loadSaveDelay;
#ifndef _RELEASE
        GetISystem()->IncreaseCheckpointLoadCount();
#endif
        return eLGR_Ok;
    default:
        gEnv->pEntitySystem->PauseTimers(false, false);
        GameWarning("Unknown result code from CGameSerialize::LoadGame");
    // fall through
    case eLGR_FailedAndDestroyedState:
        GameWarning("[CryAction] LoadGame: '%s' failed. Ending GameContext", path);
        EndGameContext(false);
    // fall through
    case eLGR_Failed:
        GetISystem()->SerializingFile(0);
        // Unpause all streams in streaming engine.
        GetISystem()->GetStreamEngine()->PauseStreaming(false, -1);
        return eLGR_Failed;
    case eLGR_CantQuick_NeedFullLoad:
        GetISystem()->SerializingFile(0);
        // Unpause all streams in streaming engine.
        GetISystem()->GetStreamEngine()->PauseStreaming(false, -1);
        return eLGR_CantQuick_NeedFullLoad;
    }
}

//------------------------------------------------------------------------
void CCryAction::OnEditorSetGameMode(int iMode)
{
    if (iMode < 2)
    {

        if (iMode == 0)
        {
            TimeDemoRecorderBus::Broadcast(&TimeDemoRecorderBus::Events::Reset);
        }

        if (m_pGame)
        {
            m_pGame->OnEditorSetGameMode(iMode != 0);
        }

        if (m_pMaterialEffects)
        {
            if (iMode && m_isEditing)
            {
                m_pMaterialEffects->PreLoadAssets();
            }
            m_pMaterialEffects->SetUpdateMode(iMode != 0);
        }
        m_isEditing = !iMode;
    }
    else if (m_pGame)
    {
        m_pGame->FixBrokenObjects(true);
        m_pGame->ClearBreakHistory();
    }

    // reset any pending camera blending
    if (m_pViewSystem)
    {
        m_pViewSystem->SetBlendParams(0, 0, 0);
        IView* pView = m_pViewSystem->GetActiveView();
        if (pView)
        {
            pView->ResetBlending();
        }
    }

    if (GetIForceFeedbackSystem())
    {
        GetIForceFeedbackSystem()->StopAllEffects();
    }

    CRangeSignaling::ref().OnEditorSetGameMode(iMode != 0);
    CSignalTimer::ref().OnEditorSetGameMode(iMode != 0);

    if (m_pCooperativeAnimationManager)
    {
        m_pCooperativeAnimationManager->Reset();
    }
}

//------------------------------------------------------------------------
IFlowSystem* CCryAction::GetIFlowSystem()
{
    return m_pFlowSystem;
}

//------------------------------------------------------------------------
void CCryAction::InitCVars()
{
    IConsole* pC = ::gEnv->pConsole;
    assert(pC);
    m_pEnableLoadingScreen = REGISTER_INT("g_enableloadingscreen", 1, VF_DUMPTODISK, "Enable/disable the loading screen");
    REGISTER_INT("g_enableitems", 1, 0, "Enable/disable the item system");
    m_pShowLanBrowserCVAR = REGISTER_INT("net_lanbrowser", 0, VF_CHEAT, "enable lan games browser");
    REGISTER_INT("g_aimdebug", 0, VF_CHEAT, "Enable/disable debug drawing for aiming direction");
    REGISTER_INT("g_groundeffectsdebug", 0, 0, "Enable/disable logging for GroundEffects (2 = use non-deferred ray-casting)");
    REGISTER_FLOAT("g_breakImpulseScale", 1.0f, VF_REQUIRE_LEVEL_RELOAD, "How big do explosions need to be to break things?");
    REGISTER_INT("g_breakage_particles_limit", 200, 0, "Imposes a limit on particles generated during 2d surfaces breaking");
    REGISTER_FLOAT("c_shakeMult", 1.0f, VF_CHEAT, "");

    m_pDebugSignalTimers = pC->RegisterInt("ai_DebugSignalTimers", 0, VF_CHEAT, "Enable Signal Timers Debug Screen");
    m_pDebugRangeSignaling = pC->RegisterInt("ai_DebugRangeSignaling", 0, VF_CHEAT, "Enable Range Signaling Debug Screen");

    REGISTER_INT("cl_packetRate", 30, 0, "Packet rate on client");
    REGISTER_INT("sv_packetRate", 30, 0, "Packet rate on server");
    REGISTER_INT("cl_bandwidth", 50000, 0, "Bit rate on client");
    REGISTER_INT("sv_bandwidth", 50000, 0, "Bit rate on server");
    REGISTER_FLOAT("g_localPacketRate", 50, 0, "Packet rate locally on faked network connection");
    REGISTER_INT("sv_timeout_disconnect", 0, 0, "Timeout for fully disconnecting timeout connections");

    //REGISTER_CVAR2( "cl_voice_playback",&m_VoicePlaybackEnabled);
    // NB this should be false by default - enabled when user holds down CapsLock
    REGISTER_CVAR2("cl_voice_recording", &m_VoiceRecordingEnabled, 0, 0, "Enable client voice recording");

    REGISTER_INT("cl_clientport", 0, VF_DUMPTODISK, "Client port");
    REGISTER_STRING("cl_serveraddr", "127.0.0.1", VF_DUMPTODISK, "Server address");
    REGISTER_INT("cl_serverport", SERVER_DEFAULT_PORT, VF_DUMPTODISK, "Server port");
    REGISTER_STRING("cl_serverpassword", "", VF_DUMPTODISK, "Server password");
    REGISTER_STRING("sv_map", "nolevel", 0, "The map the server should load");

    REGISTER_STRING("sv_levelrotation", "levelrotation", 0, "Sequence of levels to load after each game ends");

    REGISTER_STRING("sv_requireinputdevice", "dontcare", VF_DUMPTODISK | VF_REQUIRE_LEVEL_RELOAD, "Which input devices to require at connection (dontcare, none, gamepad, keyboard)");
    ICVar* pDefaultGameRules = REGISTER_STRING("sv_gamerulesdefault", "DummyRules", 0, "The game rules that the server default to when disconnecting");
    const char* currentGameRules = pDefaultGameRules ? pDefaultGameRules->GetString() : "";
    REGISTER_STRING("sv_gamerules", currentGameRules, 0, "The game rules that the server should use");
    REGISTER_INT("sv_port", SERVER_DEFAULT_PORT, VF_DUMPTODISK, "Server address");
    REGISTER_STRING("sv_password", "", VF_DUMPTODISK, "Server password");
    REGISTER_INT("sv_lanonly", 0, VF_DUMPTODISK, "Set for LAN games");

    REGISTER_STRING("cl_nickname", "", VF_DUMPTODISK, "Nickname for player on connect.");

    REGISTER_STRING("sv_bind", "0.0.0.0", VF_REQUIRE_LEVEL_RELOAD, "Bind the server to a specific IP address");

    REGISTER_STRING("sv_servername", "", VF_DUMPTODISK, "Server name will be displayed in server list. If empty, machine name will be used.");
    REGISTER_INT_CB("sv_maxplayers", 32, VF_DUMPTODISK, "Maximum number of players allowed to join server.", VerifyMaxPlayers);
    REGISTER_INT("sv_maxspectators", 32, VF_DUMPTODISK, "Maximum number of players allowed to be spectators during the game.");
    REGISTER_INT("ban_timeout", 30, VF_DUMPTODISK, "Ban timeout in minutes");
    REGISTER_FLOAT("sv_timeofdaylength", 1.0f, VF_DUMPTODISK, "Sets time of day changing speed.");
    REGISTER_FLOAT("sv_timeofdaystart", 12.0f, VF_DUMPTODISK, "Sets time of day start time.");
    REGISTER_INT("sv_timeofdayenable", 0, VF_DUMPTODISK, "Enables time of day simulation.");

    pC->RegisterInt("g_immersive", 1, 0, "If set, multiplayer physics will be enabled");

    pC->RegisterInt("sv_dumpstats", 1, 0, "Enables/disables dumping of level and player statistics, positions, etc. to files");
    pC->RegisterInt("sv_dumpstatsperiod", 1000, 0, "Time period of statistics dumping in milliseconds");
    pC->RegisterInt("g_EnableLoadSave", 1, 0, "Enables/disables saving and loading of savegames");

    REGISTER_STRING("http_password", "password", 0, "Password for http administration");
    REGISTER_STRING("rcon_password", "", 0, "Sets password for the RCON system");

#if !defined(_RELEASE)
    REGISTER_INT("connect_repeatedly_num_attempts", 5, 0, "the number of attempts to connect that connect_repeatedly tries");
    REGISTER_FLOAT("connect_repeatedly_time_between_attempts", 10.0f, 0, "the time between connect attempts for connect_repeatedly");
    REGISTER_INT("g_displayCheckpointName", 0, VF_NULL, "Display checkpoint name when game is saved");
#endif

    REGISTER_INT_CB("cl_comment", (gEnv->IsEditor() ? 1 : 0), VF_NULL, "Hide/Unhide comments in game-mode", ResetComments);

    m_pMaterialEffectsCVars = new CMaterialEffectsCVars();

    CActionGame::RegisterCVars();
}

//------------------------------------------------------------------------
void CCryAction::ReleaseCVars()
{
    SAFE_DELETE(m_pMaterialEffectsCVars);
    SAFE_DELETE(m_pCryActionCVars);
}

void CCryAction::InitCommands()
{
    // create built-in commands
    REGISTER_COMMAND("map", MapCmd, VF_BLOCKFRAME, "Load a map");
    // for testing purposes
    REGISTER_COMMAND("readabilityReload", ReloadReadabilityXML, 0, "Reloads readability xml files.");
    REGISTER_COMMAND("unload", UnloadCmd, 0, "Unload current map");
    REGISTER_COMMAND("dump_maps", DumpMapsCmd, 0, "Dumps currently scanned maps");
    REGISTER_COMMAND("play", PlayCmd, 0, "Play back a recorded game");
    REGISTER_COMMAND("version", VersionCmd, 0, "Shows game version number");
    REGISTER_COMMAND("save", SaveGameCmd, VF_RESTRICTEDMODE, "Save game");
    REGISTER_COMMAND("save_genstrings", GenStringsSaveGameCmd, VF_CHEAT, "");
    REGISTER_COMMAND("load", LoadGameCmd, VF_RESTRICTEDMODE, "Load game");
    REGISTER_COMMAND("test_reset", TestResetCmd, VF_CHEAT, "");
    REGISTER_COMMAND("open_url", OpenURLCmd, VF_NULL, "");

    REGISTER_COMMAND("g_dump_stats", DumpStatsCmd, VF_CHEAT, "");

    REGISTER_COMMAND("dump_stats", DumpAnalysisStatsCmd, 0, "Dumps some player statistics");
}

//------------------------------------------------------------------------
void CCryAction::VerifyMaxPlayers(ICVar* pVar)
{
    int nPlayers = pVar->GetIVal();
    if (nPlayers < 2 || nPlayers > MAXIMUM_NUMBER_OF_CONNECTIONS)
    {
        nPlayers = CLAMP(nPlayers, 2, MAXIMUM_NUMBER_OF_CONNECTIONS);
        pVar->Set(nPlayers);
    }
}

//------------------------------------------------------------------------
void CCryAction::ResetComments(ICVar* pVar)
{
    // Iterate through all comment entities and reset them. This will activate/deactivate them as required.
    SEntityEvent event(ENTITY_EVENT_RESET);
    IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Comment");

    if (pClass)
    {
        IEntityItPtr it = gEnv->pEntitySystem->GetEntityIterator();
        it->MoveFirst();
        while (IEntity* pEntity = it->Next())
        {
            if (pEntity->GetClass() == pClass)
            {
                pEntity->SendEvent(event);
            }
        }
    }
}

//------------------------------------------------------------------------
bool CCryAction::LoadingScreenEnabled() const
{
    return m_pEnableLoadingScreen ? m_pEnableLoadingScreen->GetIVal() != 0 : true;
}

int CCryAction::NetworkExposeClass(IFunctionHandler* pFH)
{
    return m_pScriptRMI->ExposeClass(pFH);
}

//------------------------------------------------------------------------
void CCryAction::RegisterFactory(const char* name, ISaveGame*(*func)(), bool)
{
    if (m_pGameSerialize)
    {
        m_pGameSerialize->RegisterSaveGameFactory(name, func);
    }
}

//------------------------------------------------------------------------
void CCryAction::RegisterFactory(const char* name, ILoadGame*(*func)(), bool)
{
    if (m_pGameSerialize)
    {
        m_pGameSerialize->RegisterLoadGameFactory(name, func);
    }
}

void CCryAction::HandleGridMateScriptRMI(TSerialize serialize, ChannelId toChannelId, ChannelId avoidChannelId)
{
    if (m_pScriptRMI)
    {
        m_pScriptRMI->HandleGridMateScriptRMI(serialize, toChannelId, avoidChannelId);
    }
}

void CCryAction::SerializeScript(TSerialize ser, IEntity* entity)
{
    IComponentSerializationPtr serializationComponent = entity->GetComponent<IComponentSerialization>();
    if (serializationComponent)
    {
        serializationComponent->SerializeOnly(ser, { IComponentScript::Type() });
    }

    // If we are serializing for the network, we want to use our custom serialization pathway for that.
    if (m_pScriptRMI && ser.GetSerializationTarget() != eST_Network)
    {
        m_pScriptRMI->SerializeScript(ser, entity);
    }
}

void CCryAction::NetMarshalScript(Serialization::INetScriptMarshaler* scriptMarshaler, IEntity* entity)
{
    if (m_pScriptRMI)
    {
        m_pScriptRMI->NetMarshalScript(scriptMarshaler, entity);
    }
}

void CCryAction::NetUnmarshalScript(TSerialize ser, IEntity* entity)
{
    if (m_pScriptRMI)
    {
        m_pScriptRMI->NetUnmarshalScript(ser, entity);
    }
}

IActor* CCryAction::GetClientActor() const
{
    return m_pGame ? m_pGame->GetClientActor() : nullptr;
}

EntityId CCryAction::GetClientActorId() const
{
    return (gEnv && gEnv->pGame) ? gEnv->pGame->GetClientActorId() : 0;
}

IEntity* CCryAction::GetClientEntity() const
{
    if (IActor* actor = GetClientActor())
    {
        return actor->GetEntity();
    }
    return nullptr;
}

void CCryAction::SetClientActor(EntityId id, bool setupActionMaps)
{
    if (CGameContext* pGameContext = GetGameContext())
    {
        pGameContext->PlayerIdSet(id);

        if (setupActionMaps)
        {
            SetupActionMaps();
        }
        SetupLocalView();
    }
}

IGameObject* CCryAction::GetGameObject(EntityId id)
{
    if (IEntity* pEnt = gEnv->pEntitySystem->GetEntity(id))
    {
        if (CGameObjectPtr pGameObject = pEnt->GetComponent<CGameObject>())
        {
            return pGameObject.get();
        }
    }

    return nullptr;
}

bool CCryAction::GetNetworkSafeClassId(uint16& id, const char* className)
{
    if (CGameContext* pGameContext = GetGameContext())
    {
        return pGameContext->ClassIdFromName(id, className);
    }
    else
    {
        return false;
    }
}

bool CCryAction::GetNetworkSafeClassName(char* className, size_t maxn, uint16 id)
{
    string name;
    if (CGameContext* pGameContext = GetGameContext())
    {
        if (pGameContext->ClassNameFromId(name, id))
        {
            azstrncpy(className, maxn, name.c_str(), maxn);
            return true;
        }
    }

    return false;
}


IGameObjectExtension* CCryAction::QueryGameObjectExtension(EntityId id, const char* name)
{
    if (IGameObject* pObj = GetGameObject(id))
    {
        return pObj->QueryExtension(name);
    }
    else
    {
        return nullptr;
    }
}

CTimeValue CCryAction::GetServerTime()
{
    if (gEnv->pNetwork)
    {
        return gEnv->pNetwork->GetSessionTime();
    }
    return gEnv->pTimer->GetFrameStartTime();
    ;
}

CGameContext* CCryAction::GetGameContext()
{
    return m_pGame ? m_pGame->GetGameContext() : nullptr;
}

void CCryAction::RegisterFactory(const char* name, IActorCreator* pCreator, bool isAI)
{
    m_pActorSystem->RegisterActorClass(name, pCreator, isAI);
}

void CCryAction::RegisterFactory(const char* name, IItemCreator* pCreator, bool isAI)
{
    if (m_pItemSystem)
    {
        m_pItemSystem->RegisterItemClass(name, pCreator);
    }
}

void CCryAction::RegisterFactory(const char* name, IVehicleCreator* pCreator, bool isAI)
{
    if (m_pVehicleSystem)
    {
        m_pVehicleSystem->RegisterVehicleClass(name, pCreator, isAI);
    }
}

void CCryAction::RegisterFactory(const char* name, IGameObjectExtensionCreator* pCreator, bool isAI)
{
    m_pGameObjectSystem->RegisterExtension(name, pCreator, nullptr);
}

IActionMapManager* CCryAction::GetIActionMapManager()
{
    return m_pActionMapManager;
}

IUIDraw* CCryAction::GetIUIDraw()
{
    return m_pUIDraw;
}

IMannequin& CCryAction::GetMannequinInterface()
{
    CRY_ASSERT(m_pMannequin != nullptr);
    return *m_pMannequin;
}

ILevelSystem* CCryAction::GetILevelSystem()
{
    return m_pLevelSystem;
}

IActorSystem* CCryAction::GetIActorSystem()
{
    return m_pActorSystem;
}

IItemSystem* CCryAction::GetIItemSystem()
{
    return m_pItemSystem;
}

IVehicleSystem* CCryAction::GetIVehicleSystem()
{
    return m_pVehicleSystem;
}

ISharedParamsManager* CCryAction::GetISharedParamsManager()
{
    return m_pSharedParamsManager;
}

IViewSystem* CCryAction::GetIViewSystem()
{
    return m_pViewSystem;
}

IGameplayRecorder* CCryAction::GetIGameplayRecorder()
{
    return m_pGameplayRecorder;
}


IGameRulesSystem* CCryAction::GetIGameRulesSystem()
{
    return m_pGameRulesSystem;
}

IGameObjectSystem* CCryAction::GetIGameObjectSystem()
{
    return m_pGameObjectSystem;
}

IGameTokenSystem* CCryAction::GetIGameTokenSystem()
{
    return m_pGameTokenSystem;
}

IEffectSystem* CCryAction::GetIEffectSystem()
{
    return m_pEffectSystem;
}

IMaterialEffects* CCryAction::GetIMaterialEffects()
{
    return m_pMaterialEffects;
}

IBreakableGlassSystem* CCryAction::GetIBreakableGlassSystem()
{
    return m_pBreakableGlassSystem;
}

IDialogSystem* CCryAction::GetIDialogSystem()
{
    return m_pDialogSystem;
}

IVisualLog* CCryAction::GetIVisualLog()
{
    return m_pVisualLog;
}

IRealtimeRemoteUpdate* CCryAction::GetIRealTimeRemoteUpdate()
{
    return &CRealtimeRemoteUpdateListener::GetRealtimeRemoteUpdateListener();
}

IGamePhysicsSettings* CCryAction::GetIGamePhysicsSettings()
{
    return m_pGamePhysicsSettings;
}

IPlayerProfileManager* CCryAction::GetIPlayerProfileManager()
{
    return m_pPlayerProfileManager;
}

IGameVolumes* CCryAction::GetIGameVolumesManager() const
{
    return m_pGameVolumesManager;
}

std::shared_ptr<IPrefabManager> CCryAction::GetIPrefabManager() const
{
    return m_pPrefabManager;
}

void CCryAction::PreloadAnimatedCharacter(IScriptTable* pEntityScript)
{
    animatedcharacter::Preload(pEntityScript);
}

//------------------------------------------------------------------------
// NOTE: This function must be thread-safe.
void CCryAction::PrePhysicsTimeStep(float deltaTime)
{
    if (m_pVehicleSystem)
    {
        m_pVehicleSystem->OnPrePhysicsTimeStep(deltaTime);
    }
}

void CCryAction::RegisterExtension(ICryUnknownPtr pExtension)
{
    CRY_ASSERT(pExtension != NULL);

    m_frameworkExtensions.push_back(pExtension);
}

void CCryAction::ReleaseExtensions()
{
    m_frameworkExtensions.clear();
}

ICryUnknownPtr CCryAction::QueryExtensionInterfaceById(const CryInterfaceID& interfaceID) const
{
    ICryUnknownPtr pExtension;
    for (TFrameworkExtensions::const_iterator it = m_frameworkExtensions.begin(), endIt = m_frameworkExtensions.end(); it != endIt; ++it)
    {
        if ((*it)->GetFactory()->ClassSupports(interfaceID))
        {
            pExtension = *it;
            break;
        }
    }

    return pExtension;
}

void CCryAction::ClearTimers()
{
    m_pCallbackTimer->Clear();
}

IGameFramework::TimerID CCryAction::AddTimer(CTimeValue interval, bool repeating, TimerCallback callback, void* userdata)
{
    return m_pCallbackTimer->AddTimer(interval, repeating, callback, userdata);
}

void* CCryAction::RemoveTimer(TimerID timerID)
{
    return m_pCallbackTimer->RemoveTimer(timerID);
}

const char* CCryAction::GetLevelName()
{
    if (gEnv->IsEditor())
    {
        char* levelFolder = nullptr;
        char* levelName = nullptr;
        GetEditorLevel(&levelName, &levelFolder);

        return levelName;
    }
    else
    {
        if (StartedGameContext() || (m_pGame && m_pGame->IsIniting()))
        {
            if (ILevel* pLevel = GetILevelSystem()->GetCurrentLevel())
            {
                if (ILevelInfo* pLevelInfo = pLevel->GetLevelInfo())
                {
                    return pLevelInfo->GetName();
                }
            }
        }
    }

    return nullptr;
}

const char* CCryAction::GetAbsLevelPath(char* const pPath, const uint32 cPathMaxLen)
{
    // initial (bad implementation) - need to be improved by Craig
    CRY_ASSERT(pPath);

    if (gEnv->IsEditor())
    {
        char* levelFolder = 0;

        GetEditorLevel(0, &levelFolder);

        azstrcpy(pPath, cPathMaxLen, levelFolder);

        // todo: abs path
        return pPath;
    }
    else
    {
        if (StartedGameContext())
        {
            if (ILevel* pLevel = GetILevelSystem()->GetCurrentLevel())
            {
                if (ILevelInfo* pLevelInfo = pLevel->GetLevelInfo())
                {
                    azstrcpy(pPath, cPathMaxLen, pLevelInfo->GetPath());
                    return pPath;
                }
            }
        }
    }

    return "";          // no level loaded
}

bool CCryAction::IsLoadingSaveGame()
{
    if (CGameContext* pGameContext = GetGameContext())
    {
        return pGameContext->IsLoadingSaveGame();
    }
    return false;
}

bool CCryAction::CanCheat()
{
    return !gEnv->bMultiplayer || gEnv->pSystem->IsDevMode();
}

IPersistentDebug* CCryAction::GetIPersistentDebug()
{
    return m_pPersistentDebug;
}

void CCryAction::TestResetCmd(IConsoleCmdArgs* args)
{
    GetCryAction()->Reset(true);
    //  if (CGameContext * pCtx = GetCryAction()->GetGameContext())
    //      pCtx->GetNetContext()->RequestReconfigureGame();
}

void CCryAction::DumpStatsCmd(IConsoleCmdArgs* args)
{
    CActionGame* pG = CActionGame::Get();
    if (!pG)
    {
        return;
    }
    pG->DumpStats();
}

void CCryAction::AddBreakEventListener(IBreakEventListener* pListener)
{
    assert(m_pBreakEventListener == nullptr);
    m_pBreakEventListener = pListener;
}

void CCryAction::RemoveBreakEventListener(IBreakEventListener* pListener)
{
    assert(m_pBreakEventListener == pListener);
    m_pBreakEventListener = nullptr;
}

void CCryAction::OnBreakEvent(uint16 uBreakEventIndex)
{
    if (m_pBreakEventListener)
    {
        m_pBreakEventListener->OnBreakEvent(uBreakEventIndex);
    }
}

void CCryAction::OnPartRemoveEvent(int32 iPartRemoveEventIndex)
{
    if (m_pBreakEventListener)
    {
        m_pBreakEventListener->OnPartRemoveEvent(iPartRemoveEventIndex);
    }
}

void CCryAction::RegisterListener(IGameFrameworkListener* pGameFrameworkListener, const char* name, EFRAMEWORKLISTENERPRIORITY eFrameworkListenerPriority)
{
    // listeners must be unique
    for (int i = 0; i < m_pGFListeners->size(); ++i)
    {
        if ((*m_pGFListeners)[i].pListener == pGameFrameworkListener && m_validListeners[i])
        {
            //CRY_ASSERT(false);
            return;
        }
    }

    SGameFrameworkListener listener;
    listener.pListener = pGameFrameworkListener;
    listener.name = name;
    listener.eFrameworkListenerPriority = eFrameworkListenerPriority;

    TGameFrameworkListeners::iterator iter = m_pGFListeners->begin();
    std::vector<bool>::iterator vit = m_validListeners.begin();
    for (; iter != m_pGFListeners->end(); ++iter, ++vit)
    {
        if ((*iter).eFrameworkListenerPriority > eFrameworkListenerPriority)
        {
            m_pGFListeners->insert(iter, listener);
            m_validListeners.insert(vit, true);
            return;
        }
    }
    m_pGFListeners->push_back(listener);
    m_validListeners.push_back(true);
}

void CCryAction::UnregisterListener(IGameFrameworkListener* pGameFrameworkListener)
{
    /*for (TGameFrameworkListeners::iterator iter = m_pGFListeners->begin(); iter != m_pGFListeners->end(); ++iter)
    {
        if (iter->pListener == pGFListener)
        {
            m_pGFListeners->erase(iter);
            return;
        }
    }*/
    for (int i = 0; i < m_pGFListeners->size(); i++)
    {
        if ((*m_pGFListeners)[i].pListener == pGameFrameworkListener)
        {
            (*m_pGFListeners)[i].pListener = nullptr;
            m_validListeners[i] = false;
            return;
        }
    }
}

IGameStatistics* CCryAction::GetIGameStatistics()
{
    return m_pGameStatistics;
}

void CCryAction::ScheduleEndLevel(const char* nextLevel, bool clearRenderBufferToBlack)
{
    if (!gEnv->pGame->GameEndLevel(nextLevel))
    {
        ScheduleEndLevelNow(nextLevel, clearRenderBufferToBlack);
    }

#ifndef _RELEASE
    gEnv->pSystem->SetLoadOrigin(ISystem::eLLO_Level2Level);
#endif
}

void CCryAction::ScheduleEndLevelNow(const char* nextLevel, bool clearRenderBufferToBlack)
{
    m_bScheduleLevelEnd = true;
    if (!m_pLocalAllocs)
    {
        return;
    }
    m_pLocalAllocs->m_nextLevelToLoad = nextLevel;
    m_pLocalAllocs->m_clearRenderBufferToBlackOnUnload = clearRenderBufferToBlack;

#ifndef _RELEASE
    gEnv->pSystem->SetLoadOrigin(ISystem::eLLO_Level2Level);
#endif
}

void CCryAction::CheckEndLevelSchedule()
{
    if (!m_bScheduleLevelEnd)
    {
        return;
    }
    m_bScheduleLevelEnd = false;
    if (m_pLocalAllocs == 0)
    {
        assert (false);
        return;
    }

    const bool bHaveNextLevel = (m_pLocalAllocs->m_nextLevelToLoad.empty() == false);

    IEntity* pGameRules = CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRulesEntity();
    if (pGameRules != 0)
    {
        IScriptSystem* pSS = gEnv->pScriptSystem;
        SmartScriptTable table = pGameRules->GetScriptTable();
        SmartScriptTable params(pSS);
        if (bHaveNextLevel)
        {
            params->SetValue("nextlevel", m_pLocalAllocs->m_nextLevelToLoad.c_str());
        }
        Script::CallMethod(table, "EndLevel", params);
    }

    CALL_FRAMEWORK_LISTENERS(OnLevelEnd(m_pLocalAllocs->m_nextLevelToLoad.c_str()));

    if (bHaveNextLevel)
    {
        CryFixedStringT<256> cmd;
        cmd.Format("map %s nb", m_pLocalAllocs->m_nextLevelToLoad.c_str());
        if (gEnv->IsEditor())
        {
            GameWarning("CCryAction: Suppressing loading of next level '%s' in Editor!", m_pLocalAllocs->m_nextLevelToLoad.c_str());
            m_pLocalAllocs->m_nextLevelToLoad.assign("");
        }
        else
        {
            GameWarning("CCryAction: Loading next level '%s'.", m_pLocalAllocs->m_nextLevelToLoad.c_str());
            m_pLocalAllocs->m_nextLevelToLoad.assign("");
            gEnv->pConsole->ExecuteString(cmd.c_str());

#ifndef _RELEASE
            gEnv->pSystem->SetLoadOrigin(ISystem::eLLO_Level2Level);
#endif
        }
    }
    else
    {
        GameWarning("CCryAction:LevelEnd");
    }
}

void CCryAction::ExecuteCommandNextFrame(const char* cmd)
{
    CRY_ASSERT(m_nextFrameCommand && m_nextFrameCommand->empty());
    (*m_nextFrameCommand) = cmd;
}

const char* CCryAction::GetNextFrameCommand() const
{
    return *m_nextFrameCommand;
}

void CCryAction::ClearNextFrameCommand()
{
    if (!m_nextFrameCommand->empty())
    {
        m_nextFrameCommand->resize(0);
    }
}

void CCryAction::PrefetchLevelAssets(const bool bEnforceAll)
{
    if (m_pItemSystem)
    {
        m_pItemSystem->PrecacheLevel();
    }
}

void CCryAction::ShowPageInBrowser(const char* URL)
{
#if (defined(WIN32) || defined(WIN64))
    ShellExecute(0, 0, URL, 0, 0, SW_SHOWNORMAL);
#endif
}

bool CCryAction::StartProcess(const char* cmd_line)
{
#if  defined(WIN32) || defined (WIN64)
    //save all stuff
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    return 0 != CreateProcess(nullptr, const_cast<char*>(cmd_line), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
#endif
    return false;
}

bool CCryAction::SaveServerConfig(const char* path)
{
    class CConfigWriter
        : public ICVarListProcessorCallback
    {
    public:
        CConfigWriter(const char* configPath)
        {
            m_fileHandle = gEnv->pCryPak->FOpen(configPath, "wb");
        }
        ~CConfigWriter()
        {
            gEnv->pCryPak->FClose(m_fileHandle);
        }
        void OnCVar(ICVar* pV)
        {
            string szValue = pV->GetString();
            int pos;

            // replace \ with \\ in the string
            pos = 1;
            for (;; )
            {
                pos = szValue.find_first_of("\\", pos);

                if (pos == string::npos)
                {
                    break;
                }

                szValue.replace(pos, 1, "\\\\", 2);
                pos += 2;
            }

            // replace " with \"
            pos = 1;
            for (;; )
            {
                pos = szValue.find_first_of("\"", pos);

                if (pos == string::npos)
                {
                    break;
                }

                szValue.replace(pos, 1, "\\\"", 2);
                pos += 2;
            }

            string szLine = pV->GetName();

            if (pV->GetType() == CVAR_STRING)
            {
                szLine += " = \"" + szValue + "\"\r\n";
            }
            else
            {
                szLine += " = " + szValue + "\r\n";
            }

            gEnv->pCryPak->FWrite(szLine.c_str(), szLine.length(), m_fileHandle);
        }
        bool IsOk() const
        {
            return m_fileHandle != AZ::IO::InvalidHandle;
        }
        AZ::IO::HandleType m_fileHandle;
    };
    CCVarListProcessor p("@user@/Scripts/Network/server_cvars.txt"); // @user@ is writable, but @assets@ is not!
    CConfigWriter cw(path);
    if (cw.IsOk())
    {
        p.Process(&cw);
        return true;
    }
    return false;
}

void  CCryAction::OnActionEvent(const SActionEvent& ev)
{
    switch (ev.m_event)
    {
    case eAE_loadLevel:
    {
        if (!m_pCallbackTimer)
        {
            m_pCallbackTimer = new CallbackTimer();
        }
        TimeDemoRecorderBus::Broadcast(&TimeDemoRecorderBus::Events::Reset);
    }
    break;
    case eAE_unloadLevel:
    {
        if (m_colorGradientManager)
        {
            m_colorGradientManager->Reset();
        }
    }
    default:
        break;
    }

    CALL_FRAMEWORK_LISTENERS(OnActionEvent(ev));

    switch (ev.m_event)
    {
    case eAE_postUnloadLevel:
        if (!gEnv->IsEditor())
        {
            SAFE_DELETE(m_pCallbackTimer);
        }
        break;
    default:
        break;
    }
}

void CCryAction::NotifyGameFrameworkListeners(ISaveGame* pSaveGame)
{
    CALL_FRAMEWORK_LISTENERS(OnSaveGame(pSaveGame));
}

void CCryAction::NotifyGameFrameworkListeners(ILoadGame* pLoadGame)
{
    CALL_FRAMEWORK_LISTENERS(OnLoadGame(pLoadGame));
}

void CCryAction::NotifySavegameFileLoadedToListeners(const char* pLevelName)
{
    CALL_FRAMEWORK_LISTENERS(OnSavegameFileLoadedInMemory(pLevelName));
}

void CCryAction::NotifyForceFlashLoadingListeners()
{
    CALL_FRAMEWORK_LISTENERS(OnForceLoadingWithFlash());
}

void CCryAction::SetGameGUID(const char* gameGUID)
{
    cry_strcpy(m_gameGUID, gameGUID);
}

void CCryAction::EnableVoiceRecording(const bool enable)
{
    m_VoiceRecordingEnabled = enable ? 1 : 0;
}

IDebugHistoryManager* CCryAction::CreateDebugHistoryManager()
{
    return new CDebugHistoryManager();
}

void CCryAction::GetMemoryUsage(ICrySizer* s) const
{
    //s->Add(*this);
#define CHILD_STATISTICS(x) if (x) (x)->GetMemoryStatistics(s)
    s->AddObject(m_pGame);
    s->AddObject(m_pLevelSystem);
    s->AddObject(m_pCustomActionManager);
    s->AddObject(m_pCustomEventManager);
    CHILD_STATISTICS(m_pActorSystem);
    s->AddObject(m_pItemSystem);
    CHILD_STATISTICS(m_pVehicleSystem);
    CHILD_STATISTICS(m_pSharedParamsManager);
    CHILD_STATISTICS(m_pActionMapManager);
    s->AddObject(m_pViewSystem);
    CHILD_STATISTICS(m_pGameRulesSystem);
    s->AddObject(m_pFlowSystem);
    CHILD_STATISTICS(m_pUIDraw);
    s->AddObject(m_pGameObjectSystem);
    if (m_pAnimationGraphCvars)
    {
        s->Add(*m_pAnimationGraphCvars);
    }
    s->AddObject(m_pMaterialEffects);
    s->AddObject(m_pBreakableGlassSystem);
    CHILD_STATISTICS(m_pDialogSystem);
    CHILD_STATISTICS(m_pGameTokenSystem);
    CHILD_STATISTICS(m_pEffectSystem);
    CHILD_STATISTICS(m_pGameSerialize);
    CHILD_STATISTICS(m_pCallbackTimer);
    CHILD_STATISTICS(m_pDevMode);
    CHILD_STATISTICS(m_pGameplayRecorder);
    CHILD_STATISTICS(m_pGameplayAnalyst);
    CHILD_STATISTICS(m_pTimeOfDayScheduler);
    s->AddObject(m_pFlowSystem);
    CHILD_STATISTICS(m_pGameStatistics);
    s->Add(*m_pScriptA);
    s->Add(*m_pScriptIS);
    s->Add(*m_pScriptAS);
    s->Add(*m_pScriptAMM);
    s->Add(*m_pScriptVS);
    s->Add(*m_pScriptBindVehicle);
    s->Add(*m_pScriptBindVehicleSeat);
    s->Add(*m_pScriptInventory);
    s->Add(*m_pScriptBindDS);
    s->Add(*m_pScriptBindMFX);
    s->Add(*m_pMaterialEffectsCVars);
    s->AddObject(*m_pGFListeners);
    s->Add(*m_nextFrameCommand);
    CHILD_STATISTICS(m_pScriptRMI);
    CHILD_STATISTICS(m_pPlayerProfileManager);
}

ISubtitleManager* CCryAction::GetISubtitleManager()
{
    return m_pSubtitleManager;
}

bool CCryAction::IsImmersiveMPEnabled()
{
    if (CGameContext* pGameContext = GetGameContext())
    {
        return pGameContext->HasContextFlag(eGSF_ImmersiveMultiplayer);
    }
    else
    {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CCryAction::IsInTimeDemo()
{
    AZ_Warning("Deprecation", 
        false, 
        "CCryAction::IsInTimeDemo is deprecated. Call IsTimeDemoActive on the TimeDemoRecorder EBus directly.");
    bool isTimeDemoActive = false;
    TimeDemoRecorderBus::BroadcastResult(isTimeDemoActive, &TimeDemoRecorderBus::Events::IsTimeDemoActive);
    return isTimeDemoActive;
}

//////////////////////////////////////////////////////////////////////////
bool CCryAction::IsTimeDemoRecording()
{
    AZ_Warning("Deprecation",
        false,
        "CCryAction::IsTimeDemoRecording is deprecated. Call IsRecording on the TimeDemoRecorder EBus directly.");
    bool isRecording = false;
    TimeDemoRecorderBus::BroadcastResult(isRecording, &TimeDemoRecorderBus::Events::IsRecording);
    return isRecording;
}

//////////////////////////////////////////////////////////////////////////
bool CCryAction::CanSave()
{
    ICVar* saveLoadEnabled = gEnv->pConsole->GetCVar("g_EnableLoadSave");
    bool enabled = saveLoadEnabled->GetIVal() == 1;

    const bool bViewSystemAllows = m_pViewSystem ? m_pViewSystem->IsPlayingCutScene() == false : true;

    bool isTimeDemoActive = false;
    TimeDemoRecorderBus::BroadcastResult(isTimeDemoActive, &TimeDemoRecorderBus::Events::IsTimeDemoActive);
    return enabled && bViewSystemAllows && m_bAllowSave && !isTimeDemoActive;
}

//////////////////////////////////////////////////////////////////////////
bool CCryAction::CanLoad()
{
    ICVar* saveLoadEnabled = gEnv->pConsole->GetCVar("g_EnableLoadSave");
    bool enabled = saveLoadEnabled->GetIVal() == 1;

    return enabled && m_bAllowLoad;
}

//////////////////////////////////////////////////////////////////////////
ISerializeHelper* CCryAction::GetSerializeHelper() const
{
    bool useXMLCPBin = (CCryActionCVars::Get().g_useXMLCPBinForSaveLoad == 1);

    if (useXMLCPBin)
    {
        return new CBinarySerializeHelper();
    }

    return new CXmlSerializeHelper();
}

CSignalTimer* CCryAction::GetSignalTimer()
{
    return (&(CSignalTimer::ref()));
}

ICooperativeAnimationManager* CCryAction::GetICooperativeAnimationManager()
{
    return m_pCooperativeAnimationManager;
}

ICheckpointSystem* CCryAction::GetICheckpointSystem()
{
    return(CCheckpointSystem::GetInstance());
}

IForceFeedbackSystem* CCryAction::GetIForceFeedbackSystem() const
{
    return m_pForceFeedBackSystem;
}

ICustomActionManager* CCryAction::GetICustomActionManager() const
{
    return m_pCustomActionManager;
}

ICustomEventManager* CCryAction::GetICustomEventManager() const
{
    return m_pCustomEventManager;
}

CRangeSignaling* CCryAction::GetRangeSignaling()
{
    return (&(CRangeSignaling::ref()));
}

IAIActorProxy* CCryAction::GetAIActorProxy(EntityId id) const
{
    assert(m_pAIProxyManager);
    return m_pAIProxyManager->GetAIActorProxy(id);
}

void CCryAction::OnBreakageSpawnedEntity(IEntity* pEntity, IPhysicalEntity* pPhysEntity, IPhysicalEntity* pSrcPhysEntity)
{
}

bool CCryAction::StartedGameContext(void) const
{
    return (m_pGame != 0) && (m_pGame->IsInited());
}

bool CCryAction::StartingGameContext(void) const
{
    return (m_pGame != 0) && (m_pGame->IsIniting());
}

void CCryAction::CreatePhysicsQueues()
{
    if (!m_pPhysicsQueues)
    {
        m_pPhysicsQueues = new CCryActionPhysicQueues();
    }
}

void CCryAction::ClearPhysicsQueues()
{
    SAFE_DELETE(m_pPhysicsQueues);
}

CCryActionPhysicQueues& CCryAction::GetPhysicQueues()
{
    CRY_ASSERT(m_pPhysicsQueues);

    return *m_pPhysicsQueues;
}

void CCryAction::StartNetworkStallTicker(bool includeMinimalUpdate)
{
#ifdef USE_NETWORK_STALL_TICKER_THREAD
    CRY_ASSERT(m_pNetworkStallTickerThread == nullptr);
    if (m_networkStallTickerReferences == 0)
    {
        if (includeMinimalUpdate)
        {
            gEnv->pNetwork->SyncWithGame(eNGS_AllowMinimalUpdate);
        }

        // Spawn thread to tick needed tasks
        
        m_pNetworkStallTickerThread = new CNetworkStallTickerThread();
        m_pNetworkStallTickerThread->Start(0, "NetStallTicker");
    }

    m_networkStallTickerReferences++;
#endif // #ifdef USE_NETWORK_STALL_TICKER_THREAD
}


void CCryAction::StopNetworkStallTicker()
{
#ifdef USE_NETWORK_STALL_TICKER_THREAD
    if (m_networkStallTickerReferences > 0)
    {
        m_networkStallTickerReferences--;
    }

    if (m_networkStallTickerReferences == 0)
    {
        if (m_pNetworkStallTickerThread)
        {
            m_pNetworkStallTickerThread->FlagStop();
            m_pNetworkStallTickerThread->WaitForThread();
            delete m_pNetworkStallTickerThread;
        }

        m_pNetworkStallTickerThread = nullptr;

        if (gEnv && gEnv->pNetwork)
        {
            gEnv->pNetwork->SyncWithGame(eNGS_DenyMinimalUpdate);
        }
    }
#endif // #ifdef USE_NETWORK_STALL_TICKER_THREAD
}

void CCryAction::SetupActionMaps()
{
    // Contents lifted from CryNetwork's CET_ActionMap.

    if (IActor* clientActor = GetClientActor())
    {
        IPlayerProfileManager* pPPMgr = GetIPlayerProfileManager();

        if (!pPPMgr)
        {
            return;
        }

        IActionMapManager* pActionMapMan = GetIActionMapManager();
        IPlayerProfile* pProfile = nullptr;
        const char* userId = "UNKNOWN";

        // Try to get IPlayerProfile
        if (pPPMgr->GetUserCount() == 0)
        {
            pProfile = pPPMgr->GetDefaultProfile();
            if (pProfile != nullptr)
            {
                userId = pProfile->GetUserId();
            }
        }
        else
        {
            IPlayerProfileManager::SUserInfo info;
            pPPMgr->GetUserInfo(0, info);
            pProfile = pPPMgr->GetCurrentProfile(info.userId);
            userId = info.userId;
        }

        if (pProfile == nullptr)
        {
            CryFatalError("[PlayerProfiles] CGameContext::StartGame: User '%s' has no active profile!", userId);
            return;
        }

        IActionMap* pDefaultActionMap = pProfile->GetActionMap("default");
        IActionMap* pPlayerActionMap = pProfile->GetActionMap("player");
        IActionMap* pDebugActionMap = pProfile->GetActionMap("debug");
        if (pDefaultActionMap == nullptr && pPlayerActionMap == nullptr)
        {
            // Create empty action maps so that at least we warn and don't crash.
            if (!pDefaultActionMap)
            {
                pDefaultActionMap = GetIActionMapManager()->CreateActionMap("default");
            }

            if (!pPlayerActionMap)
            {
                pPlayerActionMap = GetIActionMapManager()->CreateActionMap("player");
            }

            CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Could not find a player or default action map.  Controls might not function.  Check ActionMaps.xml or defaultProfile.xml for errors.  Check game or editor logs in %s for XML parsing errors.", AZ::IO::FileIOBase::GetInstance()->GetAlias("@log@"));
        }

        const EntityId entityId = clientActor->GetEntityId();

        if (pDefaultActionMap != nullptr)
        {
            pDefaultActionMap->SetActionListener(entityId);
        }
        if (pPlayerActionMap != nullptr)
        {
            pPlayerActionMap->SetActionListener(entityId);
        }
        if (pDebugActionMap != nullptr)
        {
            pDebugActionMap->SetActionListener(entityId);
        }

        if (IActionMap* pPlayerGamemodeActionMap = pActionMapMan->GetActionMap(gEnv->bMultiplayer ? "player_mp" : "player_sp"))
        {
            pPlayerGamemodeActionMap->SetActionListener(entityId);
        }

        pActionMapMan->EnableActionMap("player_sp", !gEnv->bMultiplayer);
        pActionMapMan->EnableActionMap("player_mp", gEnv->bMultiplayer);
        pActionMapMan->Enable(true);
    }
}

void CCryAction::SetupLocalView()
{
    if (IActor* clientActor = GetClientActor())
    {
        // Setup default view (previously handled by CET_* logic).
        IViewSystem* pVS = static_cast<IViewSystem*>(GetIViewSystem());
        if (pVS && !pVS->GetActiveView())
        {
            IView* pView = pVS->CreateView();
            pView->LinkTo(clientActor->GetEntity());
            pVS->SetActiveView(pView);

            // This is critical. Hacks in the renderer bypass work if the camera's at the origin,
            // but the logic isn't consistent, causing asymmetry, and all sorts of failures.
            // Updating the ViewSystem immediately gets the camera in the correct position now
            // so the renderer doesn't fall apart.
            // CET_InitView did this as well, which I've determined was by necessity.
            pVS->ForceUpdate(0.f);
        }
    }
}

// TypeInfo implementations for CryAction
#ifndef AZ_MONOLITHIC_BUILD
    #include "Common_TypeInfo.h"
    STRUCT_INFO_T_INSTANTIATE(Color_tpl, <uint8>)
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CRYACTION_CPP_SECTION_4
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryAction_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryAction_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#if !defined(LINUX) && !defined(APPLE)
STRUCT_INFO_T_INSTANTIATE(Color_tpl, <float>)
#endif
STRUCT_INFO_T_INSTANTIATE(Quat_tpl, <float>)
#endif
#endif // AZ_MONOLITHIC_BUILD
#include "TypeInfo_impl.h"
#include "PlayerProfiles/RichSaveGameTypes_info.h"


