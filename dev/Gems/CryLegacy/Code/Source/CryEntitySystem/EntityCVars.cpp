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
#include "EntityCVars.h"
#include "EntitySystem.h"
#include "AreaManager.h"
#include "EntityPoolManager.h"
#include "ICryAnimation.h"
#include "IComponent.h"
#include "ComponentEventDistributer.h"

ICVar* CVar::pDebug = NULL;
ICVar* CVar::pCharacterIK = NULL;
ICVar* CVar::pProfileEntities = NULL;
//ICVar* CVar::pUpdateInvisibleCharacter = NULL;
//ICVar* CVar::pUpdateBonePositions = NULL;
ICVar* CVar::pUpdateScript = NULL;
ICVar* CVar::pUpdateTimer = NULL;
ICVar* CVar::pUpdateCamera = NULL;
ICVar* CVar::pUpdatePhysics = NULL;
ICVar* CVar::pUpdateAI = NULL;
ICVar* CVar::pUpdateEntities = NULL;
ICVar* CVar::pUpdateCollision = NULL;
ICVar* CVar::pUpdateCollisionScript = NULL;
ICVar* CVar::pUpdateContainer = NULL;
ICVar* CVar::pUpdateCoocooEgg = NULL;
ICVar* CVar::pPiercingCamera = NULL;
ICVar* CVar::pVisCheckForUpdate = NULL;
ICVar* CVar::pEntityBBoxes = NULL;
ICVar* CVar::pEntityHelpers = NULL;
ICVar* CVar::pMinImpulseVel = NULL;
ICVar* CVar::pImpulseScale = NULL;
ICVar* CVar::pMaxImpulseAdjMass = NULL;
ICVar* CVar::pDebrisLifetimeScale = NULL;
ICVar* CVar::pSplashThreshold = NULL;
ICVar* CVar::pSplashTimeout = NULL;
ICVar* CVar::pHitCharacters = NULL;
ICVar* CVar::pHitDeadBodies = NULL;
ICVar* CVar::pCharZOffsetSpeed = NULL;
ICVar* CVar::pEnableFullScriptSave = NULL;
ICVar* CVar::pLogCollisions = NULL;
ICVar* CVar::pNotSeenTimeout = NULL;
ICVar* CVar::pDebugNotSeenTimeout = NULL;
ICVar* CVar::pDrawAreas = NULL;
ICVar* CVar::pDrawAreaGrid = NULL;
ICVar* CVar::pDrawAreaDebug = NULL;
ICVar* CVar::pDrawAudioComponentZRay = NULL;

ICVar* CVar::pMotionBlur = NULL;
ICVar* CVar::pSysSpecLight = NULL;

int CVar::es_DebugTimers = 0;
int CVar::es_DebugFindEntity = 0;
int CVar::es_UsePhysVisibilityChecks = 1;
float CVar::es_MaxPhysDist;
float CVar::es_MaxPhysDistInvisible;
float CVar::es_MaxPhysDistCloth;
float CVar::es_FarPhysTimeout;
int   CVar::es_DebugEvents = 0;
int   CVar::es_SortUpdatesByClass = 0;
int CVar::es_debugEntityLifetime = 0;
int   CVar::es_DisableTriggers = 0;
int CVar::es_DrawProximityTriggers = 0;
int CVar::es_DebugEntityUsage = 0;
const char* CVar::es_DebugEntityUsageFilter = "";
int CVar::es_EnablePoolUse = 0;
int CVar::es_DebugPool = 0;
int CVar::es_TestPoolSignatures = 0;
const char* CVar::es_DebugPoolFilter = "";
int CVar::es_LayerSaveLoadSerialization = 0;
int CVar::es_LayerDebugInfo = 0;
int CVar::es_SaveLoadUseLUANoSaveFlag = 1;
int CVar::es_ClearPoolBookmarksOnLayerUnload = 1;
ICVar* CVar::pUpdateType = NULL;
float CVar::es_EntityUpdatePosDelta = 0.0f;
int CVar::es_debugDrawEntityIDs = 0;

// for editor only
static void OnSysSpecLightChange(ICVar* pVar)
{
    IEntityItPtr it = GetIEntitySystem()->GetEntityIterator();
    it->MoveFirst();

    while (IEntity* pEntity = it->Next())
    {
        IScriptTable* pScriptTable = pEntity->GetScriptTable();
        if (pScriptTable && pScriptTable->HaveValue("OnSysSpecLightChanged"))
        {
            Script::CallMethod(pScriptTable, "OnSysSpecLightChanged");
        }
    }
}

static void OnUpdateTypeChange(ICVar* pVar)
{
    CEntitySystem* pES = static_cast<CEntitySystem*>(GetIEntitySystem());
    CComponentEventDistributer* pEntityEventDist = pES->GetEventDistributer();
    pEntityEventDist->SetFlags(pVar->GetIVal());
}

void CVar::Init(struct IConsole* pConsole)
{
    assert(gEnv->pConsole);
    PREFAST_ASSUME(gEnv->pConsole);

    REGISTER_COMMAND("es_dump_entities", (ConsoleCommandFunc)DumpEntities, 0, "Dumps current entities and their states!");
    REGISTER_COMMAND("es_dump_entity_classes_in_use", (ConsoleCommandFunc)DumpEntityClassesInUse, 0, "Dumps all used entity classes");
    REGISTER_COMMAND("es_compile_area_grid", (ConsoleCommandFunc)CompileAreaGrid, 0, "Trigger a recompile of the area grid");
    REGISTER_COMMAND("es_dump_bookmarks", (ConsoleCommandFunc)DumpEntityBookmarks, 0, "Dumps information about all bookmarked entities");
    REGISTER_CVAR(es_SortUpdatesByClass, 0, 0, "Sort entity updates by class (possible optimization)");
    pDebug = REGISTER_INT("es_debug", 0, VF_CHEAT,
            "Enable entity debugging info\n"
            "Usage: es_debug [0/1]\n"
            "Default is 0 (on).");
    pCharacterIK = REGISTER_INT("p_CharacterIK", 1, VF_CHEAT,
            "Toggles character IK.\n"
            "Usage: p_characterik [0/1]\n"
            "Default is 1 (on). Set to 0 to disable inverse kinematics.");
    pEntityBBoxes = REGISTER_INT("es_bboxes", 0, VF_CHEAT,
            "Toggles entity bounding boxes.\n"
            "Usage: es_bboxes [0/1]\n"
            "Default is 0 (off). Set to 1 to display bounding boxes.");
    pEntityHelpers = REGISTER_INT("es_helpers", 0, VF_CHEAT,
            "Toggles helpers.\n"
            "Usage: es_helpers [0/1]\n"
            "Default is 0 (off). Set to 1 to display entity helpers.");
    pProfileEntities = REGISTER_INT("es_profileentities", 0, VF_CHEAT,
            "Usage: es_profileentities 1,2,3\n"
            "Default is 0 (off).");
    /*  pUpdateInvisibleCharacter = REGISTER_INT("es_UpdateInvisibleCharacter",0,VF_CHEAT,
            "Usage: \n"
            "Default is 0 (off).");
        pUpdateBonePositions = REGISTER_INT("es_UpdateBonePositions",1,VF_CHEAT,
            "Usage: \n"
            "Default is 1 (on).");
    */pUpdateScript = REGISTER_INT("es_UpdateScript", 1, VF_CHEAT,
            "Usage: es_UpdateScript [0/1]\n"
            "Default is 1 (on).");
    pUpdatePhysics = REGISTER_INT("es_UpdatePhysics", 1, VF_CHEAT,
            "Toggles updating of entity physics.\n"
            "Usage: es_UpdatePhysics [0/1]\n"
            "Default is 1 (on). Set to 0 to prevent entity physics from updating.");
    pUpdateAI = REGISTER_INT("es_UpdateAI", 1, VF_CHEAT,
            "Toggles updating of AI entities.\n"
            "Usage: es_UpdateAI [0/1]\n"
            "Default is 1 (on). Set to 0 to prevent AI entities from updating.");
    pUpdateEntities = REGISTER_INT("es_UpdateEntities", 1, VF_CHEAT,
            "Toggles entity updating.\n"
            "Usage: es_UpdateEntities [0/1]\n"
            "Default is 1 (on). Set to 0 to prevent all entities from updating.");
    pUpdateCollision = REGISTER_INT("es_UpdateCollision", 1, VF_CHEAT,
            "Toggles updating of entity collisions.\n"
            "Usage: es_UpdateCollision [0/1]\n"
            "Default is 1 (on). Set to 0 to disable entity collision updating.");
    pUpdateContainer = REGISTER_INT("es_UpdateContainer", 1, VF_CHEAT,
            "Usage: es_UpdateContainer [0/1]\n"
            "Default is 1 (on).");
    pUpdateTimer = REGISTER_INT("es_UpdateTimer", 1, VF_CHEAT,
            "Usage: es_UpdateTimer [0/1]\n"
            "Default is 1 (on).");
    pUpdateCollisionScript = REGISTER_INT("es_UpdateCollisionScript", 1, VF_CHEAT,
            "Usage: es_UpdateCollisionScript [0/1]\n"
            "Default is 1 (on).");
    pVisCheckForUpdate = REGISTER_INT("es_VisCheckForUpdate", 1, VF_CHEAT,
            "Usage: es_VisCheckForUpdate [0/1]\n"
            "Default is 1 (on).");
    pMinImpulseVel = REGISTER_FLOAT("es_MinImpulseVel", 0.0f, VF_CHEAT,
            "Usage: es_MinImpulseVel 0.0");
    pImpulseScale = REGISTER_FLOAT("es_ImpulseScale", 0.0f, VF_CHEAT,
            "Usage: es_ImpulseScale 0.0");
    pMaxImpulseAdjMass = REGISTER_FLOAT("es_MaxImpulseAdjMass", 2000.0f, VF_CHEAT,
            "Usage: es_MaxImpulseAdjMass 2000.0");
    pDebrisLifetimeScale = REGISTER_FLOAT("es_DebrisLifetimeScale", 1.0f, 0,
            "Usage: es_DebrisLifetimeScale 1.0");
    pSplashThreshold = REGISTER_FLOAT("es_SplashThreshold", 1.0f, VF_CHEAT,
            "minimum instantaneous water resistance that is detected as a splash"
            "Usage: es_SplashThreshold 200.0");
    pSplashTimeout = REGISTER_FLOAT("es_SplashTimeout", 3.0f, VF_CHEAT,
            "minimum time interval between consecutive splashes"
            "Usage: es_SplashTimeout 3.0");
    pHitCharacters = REGISTER_INT("es_HitCharacters", 1, 0,
            "specifies whether alive characters are affected by bullet hits (0 or 1)");
    pHitDeadBodies = REGISTER_INT("es_HitDeadBodies", 1, 0,
            "specifies whether dead bodies are affected by bullet hits (0 or 1)");
    pCharZOffsetSpeed = REGISTER_FLOAT("es_CharZOffsetSpeed", 2.0f, VF_DUMPTODISK,
            "sets the character Z-offset change speed (in m/s), used for IK");

    pNotSeenTimeout = REGISTER_INT("es_not_seen_timeout", 30, VF_DUMPTODISK,
            "number of seconds after which to cleanup temporary render buffers in entity");
    pDebugNotSeenTimeout = REGISTER_INT("es_debug_not_seen_timeout", 0, VF_DUMPTODISK,
            "if true, log messages when entities undergo not seen timeout");

    pEnableFullScriptSave = REGISTER_INT("es_enable_full_script_save", 0,
            VF_DUMPTODISK, "Enable (experimental) full script save functionality");

    pLogCollisions = REGISTER_INT("es_log_collisions", 0, 0, "Enables collision events logging");
    REGISTER_CVAR(es_DebugTimers, 0, VF_CHEAT,
        "This is for profiling and debugging (for game coders and level designer)\n"
        "By enabling this you get a lot of console printouts that show all entities that receive OnTimer\n"
        "events - it's good to minimize the call count. Certain entities might require this feature and\n"
        "using less active entities can often be defined by the level designer.\n"
        "Usage: es_DebugTimers 0/1");
    REGISTER_CVAR(es_DebugFindEntity, 0, VF_CHEAT, "");
    REGISTER_CVAR(es_DebugEvents, 0, VF_CHEAT, "Enables logging of entity events");
    REGISTER_CVAR(es_DisableTriggers, 0, 0, "Disable enter/leave events for proximity and area triggers");
    REGISTER_CVAR(es_DrawProximityTriggers, 0, 0,
        "Shows Proximity Triggers.\n"
        "Usage: es_DrawProximityTriggers [0-255].  The parameter sets the transparency (alpha) level.\n"
        "Value 1 will be changed to 70.\n"
        "Default is 0 (off)\n");

    REGISTER_CVAR(es_DebugEntityUsage, 0, 0,
        "Draws information to the screen to show how entities are being used, per class, including total, active and hidden counts and memory usage"
        "\nUsage: es_DebugEntityUsage update_rate"
        "\nupdate_rate - Time in ms to refresh memory usage calculation or 0 to disable");
    REGISTER_CVAR(es_DebugEntityUsageFilter, "", 0, "Filter entity usage debugging to classes which have this string in their name");

    REGISTER_CVAR(es_EnablePoolUse, -1, 0,
        "Force toggle the use of entity pools on/off.\n"
        "Usage: es_EnablePoolUse 1\n"
        "Default is -1, or normal behavior. 0 forces system off. 1 forces system on.");

    REGISTER_CVAR(es_DebugPool, 0, 0, "Enable debug drawing of entity pools");
    REGISTER_CVAR(es_TestPoolSignatures, 0, VF_CHEAT, "Enable signature testing on entity classes the first time they're prepared from an entity pool");
    REGISTER_CVAR(es_DebugPoolFilter, "", 0, "Filter entity pool debugging for just this pool and draw more info about it");

    REGISTER_CVAR(es_LayerSaveLoadSerialization, 0, VF_CHEAT,
        "Switches layer entity serialization: \n"
        "0 - serialize all \n"
        "1 - automatically ignore entities on disabled layers \n"
        "2 - only ignore entities on non-save layers.");
    REGISTER_CVAR(es_LayerDebugInfo, 0, VF_CHEAT,
        "Render debug info on active layers: \n"
        "0 - inactive \n"
        "1 - active brush layers \n"
        "2 - all layer info \n"
        "3 - all layer and all layer pak info");
    REGISTER_CVAR(es_SaveLoadUseLUANoSaveFlag, 0, VF_CHEAT, "Save&Load optimization : use lua flag to not serialize entities, for example rigid bodies.");

    REGISTER_CVAR(es_ClearPoolBookmarksOnLayerUnload, 1, VF_CHEAT, "Clear pool bookmarks when a layer is unloaded (saves memory and makes smaller saves)");
    pUpdateType = REGISTER_INT_CB("es_updateType", CComponentEventDistributer::EEventUpdatePolicy_UseDistributer, VF_CHEAT, "Defines how we update type for the entities", OnUpdateTypeChange);

    pDrawAreas = REGISTER_INT("es_DrawAreas", 0, VF_CHEAT, "Enables drawing of Areas");
    pDrawAreaGrid = REGISTER_INT("es_DrawAreaGrid", 0, VF_CHEAT, "Enables drawing of Area Grid");
    pDrawAreaDebug = REGISTER_INT("es_DrawAreaDebug", 0, VF_CHEAT, "Enables debug drawing of Areas, set 2 for log details");
    pDrawAudioComponentZRay = REGISTER_INT("es_DrawAudioComponentZRay", 0, VF_CHEAT, "Enables drawing of Z ray on check for Z visibility");

    REGISTER_CVAR(es_UsePhysVisibilityChecks, 1, 0,
        "Activates physics quality degradation and forceful sleeping for invisible and faraway entities");
    REGISTER_CVAR(es_MaxPhysDist, 300.0f, 0,
        "Physical entities farther from the camera than this are forcefully deactivated");
    REGISTER_CVAR(es_MaxPhysDistCloth, 300.0f, 0,
        "Cloth entities farther from the camera than this are forcefully deactivated");
    REGISTER_CVAR(es_MaxPhysDistInvisible, 40.0f, 0,
        "Invisible physical entities farther from the camera than this are forcefully deactivated");
    REGISTER_CVAR(es_FarPhysTimeout, 4.0f, 0,
        "Timeout for faraway physics forceful deactivation");

    pMotionBlur = gEnv->pConsole->GetCVar("r_MotionBlur");
    pSysSpecLight = gEnv->pConsole->GetCVar("e_LightQuality");
    if (pSysSpecLight && gEnv->IsEditor())
    {
        pSysSpecLight->SetOnChangeCallback(OnSysSpecLightChange);
    }

    REGISTER_CVAR(es_debugEntityLifetime, 0, 0,
        "Debug entities creation and deletion time");

    REGISTER_COMMAND("es_debugAnim", (ConsoleCommandFunc)EnableDebugAnimText, 0, "Debug entity animation (toggle on off)");

    REGISTER_CVAR(es_EntityUpdatePosDelta, 0.1f, 0,
        "Indicates the position delta by which an entity must move before the AreaManager updates position relevant data.\n"
        "Default: 0.1 (10 cm)");

    REGISTER_CVAR(es_debugDrawEntityIDs, 0, VF_CHEAT,
        "Displays the EntityId of all entities.\n"
        "Default is 0 (off), any other number enables it.\n"
        "Note: es_debug must be set to 1 also (or else the EntityId won't be displayed)");
}


void CVar::DumpEntities(IConsoleCmdArgs* args)
{
    GetIEntitySystem()->DumpEntities();
}

void CVar::DumpEntityClassesInUse(IConsoleCmdArgs* args)
{
    IEntityItPtr it = GetIEntitySystem()->GetEntityIterator();
    it->MoveFirst();

    std::map<string, int> classes;
    while (IEntity* pEntity = it->Next())
    {
        classes[pEntity->GetClass()->GetName()]++;
    }

    CryLogAlways("--------------------------------------------------------------------------------");
    for (std::map<string, int>::iterator iter = classes.begin(); iter != classes.end(); ++iter)
    {
        CryLogAlways("%s: %d instances", iter->first.c_str(), iter->second);
    }
}

void CVar::CompileAreaGrid(IConsoleCmdArgs*)
{
    CEntitySystem* pEntitySystem = GetIEntitySystem();
    CAreaManager* pAreaManager = (pEntitySystem ? static_cast<CAreaManager*>(pEntitySystem->GetAreaManager()) : 0);
    if (pAreaManager)
    {
        pAreaManager->SetAreasDirty();
    }
}

void CVar::DumpEntityBookmarks(IConsoleCmdArgs* pArgs)
{
    const char* szFilterName = 0;
    bool bWriteSIDToDisk = false;
    if (pArgs && pArgs->GetArgCount() > 1)
    {
        szFilterName = pArgs->GetArg(1);
        if (szFilterName && szFilterName[0] && 0 == _stricmp(szFilterName, "all"))
        {
            szFilterName = 0;
        }

        if (pArgs->GetArgCount() > 2)
        {
            bWriteSIDToDisk = (atoi(pArgs->GetArg(2)) != 0);
        }
    }

    CEntitySystem* pEntitySystem = GetIEntitySystem();
    CEntityPoolManager* pEntityPoolManager = (pEntitySystem ? pEntitySystem->GetEntityPoolManager() : NULL);
    if (pEntityPoolManager)
    {
        pEntityPoolManager->DumpEntityBookmarks(szFilterName, bWriteSIDToDisk);
    }
}

void CVar::EnableDebugAnimText(IConsoleCmdArgs* args)
{
    if (args && args->GetArgCount() > 1)
    {
        const char* szFilterName = args->GetArg(1);
        bool enable = true;
        if (args->GetArgCount() > 2)
        {
            enable = (strcmp(args->GetArg(2), "0") != 0);
        }

        CEntitySystem* pEntitySystem = GetIEntitySystem();
        IEntity* entity = pEntitySystem->FindEntityByName(szFilterName);

        if (entity)
        {
            uint32 numSlots = entity->GetSlotCount();
            for (uint32 i = 0; i < numSlots; i++)
            {
                ICharacterInstance* charInst = entity->GetCharacter(i);
                if (charInst)
                {
                    charInst->GetISkeletonAnim()->SetDebugging(enable);
                    IAttachmentManager* pAttachmentManager = charInst->GetIAttachmentManager();
                    for (int32 attachmentIndex = 0; attachmentIndex < pAttachmentManager->GetAttachmentCount(); ++attachmentIndex)
                    {
                        IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(attachmentIndex);
                        assert(pAttachment);

                        IAttachmentObject* pObject = pAttachment->GetIAttachmentObject();
                        if (pObject)
                        {
                            ICharacterInstance* pObjectCharInst = pObject->GetICharacterInstance();
                            if (pObjectCharInst)
                            {
                                pObjectCharInst->GetISkeletonAnim()->SetDebugging(enable);
                            }
                        }
                    }
                }
            }
        }
    }
}
