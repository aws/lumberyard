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
#include "GameContext.h"
#include "IActorSystem.h"
#include "IGame.h"
#include "CryAction.h"
#include "GameRulesSystem.h"
#include "ScriptHelpers.h"
#include "GameObjects/GameObject.h"
#include "ScriptRMI.h"
#include "IPhysics.h"
#include "IEntityRenderState.h"
#include "ActionGame.h"
#include "ILevelSystem.h"
#include "IPlayerProfiles.h"
#include "CVarListProcessor.h"
#include "NetworkCVars.h"
#include "CryActionCVars.h"
#include "INetwork.h"
#include "IPlatformOS.h"

#include "Components/IComponentSerialization.h"

#include "Components/IComponentRender.h"
#include "Components/IComponentPhysics.h"

#define VERBOSE_TRACE_CONTEXT_SPAWNS 0

CGameContext* CGameContext::s_pGameContext = NULL;

CGameContext::CGameContext(CCryAction* pFramework, CScriptRMI* pScriptRMI, CActionGame* pGame)
    : m_pFramework(pFramework)
    , m_pEntitySystem(0)
    , m_pGame(pGame)
    , m_pScriptRMI(pScriptRMI)
    , m_bStarted(false)
    , m_bIsLoadingSaveGame(false)
    , m_bHasSpawnPoint(true)
    , m_broadcastActionEventInGame(-1)
{
    CRY_ASSERT(s_pGameContext == NULL);
    s_pGameContext = this;

    gEnv->pConsole->AddConsoleVarSink(this);

    m_pScriptRMI->SetContext(this);

    // default (permissive) flags until we are initialized correctly
    // ensures cvars can be changed just at the editor start
    m_flags = eGSF_LocalOnly | eGSF_Server | eGSF_Client;

    //  Recently the loading has changed on C2 MP to get clients to load in parallel with the server loading. This unfortunately means that
    //  the message that turns on immersive multiplayer arrives _after_ rigid bodies are created. So they don't have the flag set, and
    //  they don't work. We have one day left to fix as much stuff as possible, so this is being done instead of a nice fix involving
    //  changing which network message things are sent in and such.
    m_flags |= eGSF_ImmersiveMultiplayer;

    m_pPhysicalWorld = gEnv->pPhysicalWorld;
    m_pGame->AddGlobalPhysicsCallback(eEPE_OnCollisionLogged, OnCollision, 0);

    if (!CCryActionCVars::Get().g_syncClassRegistry)
    {
        IEntityClassRegistry* pClassRegistry = gEnv->pEntitySystem->GetClassRegistry();
        IEntityClass* pClass;
        pClassRegistry->IteratorMoveFirst();
        while (pClass = pClassRegistry->IteratorNext())
        {
            m_classRegistry.RegisterClassName(pClass->GetName(), ~uint16(0));
        }
    }
}

CGameContext::~CGameContext()
{
    if (gEnv->pNetwork)
    {
        gEnv->pNetwork->SetGameContext(nullptr);
    }

    gEnv->pConsole->RemoveConsoleVarSink(this);

    if (m_pEntitySystem)
    {
        m_pEntitySystem->RemoveSink(this);
    }

    m_pGame->RemoveGlobalPhysicsCallback(eEPE_OnCollisionLogged, OnCollision, 0);

    m_pScriptRMI->SetContext(NULL);

    s_pGameContext = NULL;
}

void CGameContext::Init(const SGameStartParams* params)
{
    CRY_ASSERT(!m_pEntitySystem);
    m_pEntitySystem = gEnv->pEntitySystem;
    CRY_ASSERT(m_pEntitySystem);

    uint64 onEventSubscriptions = 0;
    onEventSubscriptions |= ENTITY_EVENT_BIT(ENTITY_EVENT_XFORM);
    onEventSubscriptions |= ENTITY_EVENT_BIT(ENTITY_EVENT_ENTER_SCRIPT_STATE);

    m_pEntitySystem->AddSink(this, static_cast<uint32>(IEntitySystem::AllSinkEvents), onEventSubscriptions);

    BackupParams(params);

    if (gEnv->pNetwork)
    {
        gEnv->pNetwork->SetGameContext(this);
    }

    gEnv->pNetwork->SetDelegatableAspectMask(eEA_GameClientDynamic |
        eEA_GameClientStatic |
        eEA_Physics |
        eAF_Delegatable |
        eEA_Aspect31 |
        eEA_GameClientB |
        eEA_GameClientC |
        eEA_GameClientD |
        eEA_GameClientE |
        eEA_GameClientF |
        eEA_GameClientG |
        eEA_GameClientH |
        eEA_GameClientI |
        eEA_GameClientJ |
        eEA_GameClientK |
        eEA_GameClientO |
        eEA_GameClientP);
}

void CGameContext::SetContextFlags(unsigned flags)
{
    m_flags = flags;
}

void CGameContext::SetContextInfo(unsigned flags, const char* connectionString)
{
    m_flags = flags;
    m_connectionString = connectionString;
}

void CGameContext::ResetMap(bool isServer)
{
}

void CGameContext::CallOnSpawnComplete(IEntity* pEntity)
{
    IScriptTable* pScriptTable(pEntity->GetScriptTable());
    if (!pScriptTable)
    {
        return;
    }

    if (gEnv->bServer)
    {
        SmartScriptTable server;
        if (pScriptTable->GetValue("Server", server))
        {
            if ((server->GetValueType("OnSpawnComplete") == svtFunction) && pScriptTable->GetScriptSystem()->BeginCall(server, "OnSpawnComplete"))
            {
                pScriptTable->GetScriptSystem()->PushFuncParam(pScriptTable);
                pScriptTable->GetScriptSystem()->EndCall();
            }
        }
    }

    if (gEnv->IsClient())
    {
        SmartScriptTable client;
        if (pScriptTable->GetValue("Client", client))
        {
            if ((client->GetValueType("OnSpawnComplete") == svtFunction) && pScriptTable->GetScriptSystem()->BeginCall(client, "OnSpawnComplete"))
            {
                pScriptTable->GetScriptSystem()->PushFuncParam(pScriptTable);
                pScriptTable->GetScriptSystem()->EndCall();
            }
        }
    }
}


static uint32 GetLowResSpacialCoord(float x)
{
    return (uint32)(x / 0.75f);
}

static uint32 GetLowResAngle(float x)
{
    return (uint32)(180.0f * x / gf_PI / 10);
}

//
// IEntitySystemSink
//

bool CGameContext::OnBeforeSpawn(SEntitySpawnParams& params)
{
    /*
    // \todo - Determine if this logic is still needed.
    if (m_isInLevelLoad && !gEnv->IsEditor() && gEnv->bMultiplayer)
    {
    #ifndef _RELEASE
    static ICVar * pLoadAllLayersForResList = 0;
    if (!pLoadAllLayersForResList)
    {
    pLoadAllLayersForResList = gEnv->pConsole->GetCVar("sv_LoadAllLayersForResList");
    }

    if (pLoadAllLayersForResList && pLoadAllLayersForResList->GetIVal() != 0)
    {
    //Bypass the layer filtering. This ensures that an autoresourcelist for generating pak files will contain all the possible assets.
    return true;
    }
    #endif // #ifndef _RELEASE

    static const char prefix[] = "GameType_";
    static const size_t prefixLen = sizeof(prefix)-1;

    // if a layer prefixed with GameType_ exists,
    // then discard it if it's not the current game type
    if (!_strnicmp(params.sLayerName, prefix, prefixLen))
    {
    if (IEntity *pGameRules=CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRulesEntity())
    {
    const char *currentType=pGameRules->GetClass()->GetName();
    const char *gameType=params.sLayerName+prefixLen;
    if (_stricmp(gameType, currentType))
    return false;
    }
    }
    }
    */

    return true;
}

void CGameContext::OnSpawn(IEntity* pEntity, SEntitySpawnParams& params)
{
    if (IGameRules* pGameRules = GetGameRules())
    {
        pGameRules->OnEntitySpawn(pEntity);
    }

    if (params.nFlags & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY))
    {
        return;
    }
    if (pEntity->GetFlags() & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY))
    {
        return;
    }

    //  if (aspects & eEA_Physics)
    //  {
    //      aspects &= ~eEA_Volatile;
    //  }

#if VERBOSE_TRACE_CONTEXT_SPAWNS
    CryLogAlways("CGameContext: Spawn Entity");
    CryLogAlways("  name       : %s", params.sName);
    CryLogAlways("  id         : 0x%.4x", params.id);
    CryLogAlways("  flags      : 0x%.8x", params.nFlags);
    CryLogAlways("  pos        : %.1f %.1f %.1f", params.vPosition.x, params.vPosition.y, params.vPosition.z);
    CryLogAlways("  cls name   : %s", params.pClass->GetName());
    CryLogAlways("  cls flags  : 0x%.8x", params.pClass->GetFlags());
    CryLogAlways("  channel    : %d", channelId);
    CryLogAlways("  net aspects: 0x%.8x", aspects);
#endif

    if (pEntity->GetComponent<IComponentScript>())
    {
        m_pScriptRMI->SetupEntity(params.id, pEntity, true, true);
    }

    CallOnSpawnComplete(pEntity);
}

bool CGameContext::OnRemove(IEntity* pEntity)
{
    EntityId id = pEntity->GetId();
    if (IGameRules* pGameRules = GetGameRules())
    {
        pGameRules->OnEntityRemoved(pEntity);
    }

    bool ok = true;

    if (pEntity->GetFlags() & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY | ENTITY_FLAG_UNREMOVABLE))
    {
        return true;
    }

    if (ok)
    {
        m_pScriptRMI->RemoveEntity(id);
    }

    return ok;
}

void CGameContext::OnReused(IEntity* pEntity, SEntitySpawnParams& params)
{
    if (IGameRules* pGameRules = GetGameRules())
    {
        pGameRules->OnEntityReused(pEntity, params, params.prevId);
    }

    m_pScriptRMI->RemoveEntity(params.prevId);

    if (pEntity->GetComponent<IComponentScript>())
    {
        m_pScriptRMI->SetupEntity(params.id, pEntity, true, true);
    }

    if (CGameObjectPtr pGO = pEntity->GetComponent<CGameObject>())
    {
        pGO->BindToNetwork(eBTNM_NowInitialized);
    }
}

void CGameContext::OnEvent(IEntity* pEntity, SEntityEvent& event)
{
    struct SGetEntId
    {
        ILINE SGetEntId(IEntity* entity)
            : m_pEntity(entity)
            , m_id(0) {}

        ILINE operator EntityId()
        {
            if (!m_id && m_pEntity)
            {
                m_id = m_pEntity->GetId();
            }
            return m_id;
        }

        IEntity* m_pEntity;
        EntityId m_id;
    };
    SGetEntId entId(pEntity);

    struct SEntIsMP
    {
        ILINE SEntIsMP(IEntity* entity)
            : m_pEntity(entity)
            , m_got(false) {}

        ILINE operator bool()
        {
            if (!m_got)
            {
                m_is = !(m_pEntity->GetFlags() & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY));
                m_got = true;
            }
            return m_is;
        }

        IEntity* m_pEntity;
        bool m_got;
        bool m_is;
    };
    SEntIsMP entIsMP(pEntity);

    switch (event.event)
    {
    case ENTITY_EVENT_XFORM:
        if (gEnv->bMultiplayer && entIsMP)
        {
            bool doAspectUpdate = true;
            if (event.nParam[0] & (ENTITY_XFORM_FROM_PARENT | ENTITY_XFORM_NO_PROPOGATE))
            {
                doAspectUpdate = false;
            }
            // position has changed, best let other people know about it
            // disabled volatile... see OnSpawn for reasoning
            if (doAspectUpdate)
            {
                if (gEnv && gEnv->pNetwork)
                {
                    gEnv->pNetwork->ChangedAspects(pEntity->GetId(), eEA_Physics);
                }
            }
#if FULL_ON_SCHEDULING
            float drawDistance = -1;
            if (IComponentRenderPtr pRP = pEntity->GetComponent<IComponentRender>())
            {
                if (IRenderNode* pRN = pRP->GetRenderNode())
                {
                    drawDistance = pRN->GetMaxViewDist();
                }
            }
            m_pNetContext->ChangedTransform(entId, pEntity->GetWorldPos(), pEntity->GetWorldRotation(), drawDistance);
#endif
        }
        break;
    case ENTITY_EVENT_ENTER_SCRIPT_STATE:
        if (entIsMP)
        {
            if (gEnv && gEnv->pNetwork)
            {
                gEnv->pNetwork->ChangedAspects(pEntity->GetId(), eEA_Script);
            }
        }
        break;
    }
}

//
// physics synchronization
//

void CGameContext::OnCollision(const EventPhys* pEvent, void*)
{
    const EventPhysCollision* pCEvent = static_cast<const EventPhysCollision*>(pEvent);
    //IGameObject *pSrc = pCollision->iForeignData[0]==PHYS_FOREIGN_ID_ENTITY ? s_this->GetEntityGameObject((IEntity*)pCollision->pForeignData[0]):0;
    //IGameObject *pTrg = pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? s_this->GetEntityGameObject((IEntity*)pCollision->pForeignData[1]):0;
    IEntity* pEntitySrc = GetEntity(pCEvent->iForeignData[0], pCEvent->pForeignData[0]);
    IEntity* pEntityTrg = GetEntity(pCEvent->iForeignData[1], pCEvent->pForeignData[1]);

    if (!pEntitySrc || !pEntityTrg)
    {
        return;
    }
}

//
// internal functions
//

bool CGameContext::RegisterClassName(const string& name, uint16 id)
{
    return m_classRegistry.RegisterClassName(name, id);
}

uint32 CGameContext::GetClassesHash()
{
    return m_classRegistry.GetHash();
}

void CGameContext::DumpClasses()
{
    m_classRegistry.DumpClasses();
}

bool CGameContext::ClassIdFromName(uint16& id, const string& name) const
{
    return m_classRegistry.ClassIdFromName(id, name);
}

bool CGameContext::ClassNameFromId(string& name, uint16 id) const
{
    return m_classRegistry.ClassNameFromId(name, id);
}

bool CGameContext::ChangeContext(bool isServer, const SGameContextParams* params)
{
    if (HasContextFlag(eGSF_DemoRecorder) && HasContextFlag(eGSF_DemoPlayback))
    {
        GameWarning("Cannot both playback a demo file and record a demo at the same time.");
        return false;
    }

    if (!HasContextFlag(eGSF_NoLevelLoading))
    {
        if (!params->levelName)
        {
            GameWarning("No level specified: not changing context");
            return false;
        }

        ILevelInfo* pLevelInfo = m_pFramework->GetILevelSystem()->GetLevelInfo(params->levelName);
        if (!pLevelInfo)
        {
            GameWarning("Level %s not found", params->levelName);

            m_pFramework->GetILevelSystem()->OnLevelNotFound(params->levelName);

            return false;
        }

#ifndef _DEBUG
        string gameMode = params->gameRules;
        if ((pLevelInfo->GetGameTypeCount() > 0) && (gameMode != "SinglePlayer") && !pLevelInfo->SupportsGameType(gameMode) && !gEnv->IsEditor() && !gEnv->pSystem->IsDevMode())
        {
            GameWarning("Level %s does not support %s gamerules.", params->levelName, gameMode.c_str());

            return false;
        }
#endif
    }

    if (!HasContextFlag(eGSF_NoGameRules))
    {
        if (!params->gameRules)
        {
            GameWarning("No rules specified: not changing context");
            return false;
        }

        if (!m_pFramework->GetIGameRulesSystem()->HaveGameRules(params->gameRules))
        {
            GameWarning("Game rules %s not found; not changing context", params->gameRules);
            return false;
        }
    }

    if (params->levelName)
    {
        m_levelName = params->levelName;
    }
    else
    {
        m_levelName.clear();
    }
    if (params->gameRules)
    {
        m_gameRules = params->gameRules;
    }
    else
    {
        m_gameRules.clear();
    }

    //if (HasContextFlag(eGSF_Server) && HasContextFlag(eGSF_Client))
    while (HasContextFlag(eGSF_Server))
    {
        if (HasContextFlag(eGSF_DemoRecorder))
        {
            if (params->demoRecorderFilename == NULL)
            {
                break;
            }
            CCryAction::GetCryAction()->OnActionEvent(SActionEvent(eAE_demoRecorderCreated, 0));
            break;
        }

        break;
    }

    BackupParams(params);

    return true;
}

void CGameContext::BackupParams(const SGameStartParams* params)
{
    if (params)
    {
        m_appliedParams.Backup(*params);

        BackupParams(params->pContextParams);
    }
    else
    {
        m_appliedParams = BackedUpGameStartParams();
    }
}

void CGameContext::BackupParams(const SGameContextParams* contextParams)
{
    m_appliedParams.pContextParams = &m_appliedContextParams;

    if (contextParams)
    {
        if (contextParams)
        {
            m_appliedContextParams.Backup(*contextParams);
        }

        m_appliedParams.pContextParams = &m_appliedContextParams;
    }
}

bool CGameContext::Update()
{
    float white[] = {1, 1, 1, 1};
    if (!m_bHasSpawnPoint)
    {
        gEnv->pRenderer->Draw2dLabel(10, 10, 4, white, false, "NO SPAWN POINT");
    }

    bool ret = false;

    if (0 == (m_broadcastActionEventInGame -= (m_broadcastActionEventInGame != -1)))
    {
        CCryAction::GetCryAction()->OnActionEvent(eAE_inGame);
    }

    return ret;
}

bool CGameContext::OnBeforeVarChange(ICVar* pVar, const char* sNewValue)
{
    int flags = pVar->GetFlags();
    bool netSynced = ((flags & VF_NET_SYNCED) != 0);
    bool isServer = HasContextFlag(eGSF_Server);
    bool allowChange = (!netSynced) || isServer;

#if LOG_CVAR_USAGE
    if (!allowChange)
    {
        CryLog("[CVARS]: [CHANGED] CGameContext::OnBeforeVarChange(): variable [%s] is VF_NET_SYNCED with a value of [%s]; CLIENT changing attempting to set to [%s] - IGNORING",
            pVar->GetName(),
            pVar->GetString(),
            sNewValue);
    }
#endif // LOG_CVAR_USAGE

    return allowChange;
}

void CGameContext::OnAfterVarChange(ICVar* pVar)
{
}

static XmlNodeRef Child(XmlNodeRef& from, const char* name)
{
    if (XmlNodeRef found = from->findChild(name))
    {
        return found;
    }
    else
    {
        XmlNodeRef newNode = from->createNode(name);
        from->addChild(newNode);
        return newNode;
    }
}

XmlNodeRef CGameContext::GetGameState()
{
    XmlNodeRef root = GetISystem()->CreateXmlNode("root");

    // add server/network properties
    XmlNodeRef serverProperties = Child(root, "server");

    // add game properties
    XmlNodeRef gameProperties = Child(root, "game");
    gameProperties->setAttr("levelName", m_levelName.c_str());
    gameProperties->setAttr("gameRules", m_gameRules.c_str());
    char buffer[32];
    GetISystem()->GetProductVersion().ToShortString(buffer);
    gameProperties->setAttr("version", string(buffer).c_str());

    return root;
}

IGameRules* CGameContext::GetGameRules()
{
    IEntity* pGameRules = m_pFramework->GetIGameRulesSystem()->GetCurrentGameRulesEntity();
    if (pGameRules)
    {
        return 0;
    }

    return 0;
}

void CGameContext::OnBrokeSomething(const SBreakEvent& be, EventPhysMono* pMono, bool isPlane)
{
}

void CGameContext::PlayerIdSet(EntityId id)
{
    IEntity* pEnt = gEnv->pEntitySystem->GetEntity(id);

    // Formerly handled in GameClientChannel. It actually makes more sense here anyway.
    if (IScriptSystem* scriptSystem = gEnv->pScriptSystem)
    {
        static const char* kLocalActorVariable = "g_localActor";
        static const char* kLocalChannelIdVariable = "g_localChannelId";
        static const char* kLocalActorIdVariable = "g_localActorId";

        if (pEnt)
        {
            ScriptHandle handle;
            handle.n = id;
            scriptSystem->SetGlobalValue(kLocalActorIdVariable, handle);
            scriptSystem->SetGlobalValue(kLocalActorVariable, pEnt->GetScriptTable());
            scriptSystem->SetGlobalValue(kLocalChannelIdVariable, gEnv->pNetwork->GetLocalChannelId());
        }
        else
        {
            scriptSystem->SetGlobalToNull(kLocalChannelIdVariable);
            scriptSystem->SetGlobalToNull(kLocalActorVariable);
            scriptSystem->SetGlobalToNull(kLocalChannelIdVariable);
        }
    }

    if (IActor* pActor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(id))
    {
        pActor->InitLocalPlayer();
    }

    if (gEnv && gEnv->pGame)
    {
        gEnv->pGame->PlayerIdSet(id);
    }

    if (pEnt)
    {
        pEnt->AddFlags(ENTITY_FLAG_LOCAL_PLAYER | ENTITY_FLAG_TRIGGER_AREAS);

        if (pEnt->IsInitialized())
        {
            if (CGameObjectPtr pGO = pEnt->GetComponent<CGameObject>())
            {
                SGameObjectEvent goe(eGFE_BecomeLocalPlayer, eGOEF_ToAll);
                pGO->SendEvent(goe);
            }
        }
    }
}

int CGameContext::GetPlayerCount()
{
    IActorSystem* actorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
    return actorSystem->GetActorCount();
}

void CGameContext::GetMemoryUsage(ICrySizer* s) const
{
    s->Add(*this);
    m_classRegistry.GetMemoryStatistics(s);
    s->AddObject(m_levelName);
    s->AddObject(m_gameRules);
    s->AddObject(m_connectionString);
}
